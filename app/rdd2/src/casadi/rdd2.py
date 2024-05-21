#!/usr/bin/env python3
import os
import sys
import math
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import casadi as ca
import cyecca.lie as lie

from cyecca.symbolic import SERIES
from cyecca.lie.group_so3 import SO3Quat, SO3EulerB321
from cyecca.lie.group_se23 import SE23LieGroupElement, SE23LieAlgebraElement, SE23Quat


def derive_attitude_error():
    import casadi as ca

    # actual attitude, expressed as quaternion
    q = ca.SX.sym('q', 4)
    X = SO3Quat.elem(q)

    # in auto level mode
    # user will input desired roll angle and desired pitch angel with 
    # sticks, these desired angles will not approach roll/pitch = 90 deg
    # as the desired angles will be saturated
    yaw_r = ca.SX.sym('yaw_r')
    pitch_r = ca.SX.sym('pitch_r')
    roll_r = ca.SX.sym('roll_r')
    R = SO3Quat.from_Euler(SO3EulerB321.elem(ca.vertcat(yaw_r, pitch_r, roll_r)))

    # the difference between vehicle attitudes will use Lie log map
    # to get the angular velocity to get between the current and 
    # desired attitude in 1 sec, this will be used to drive the desired body 
    # rates for the inner rate loop

    # Lie algebra
    omega = (X.inverse() * R).log()  # angular velocity to get to desired att in 1 sec
    # input to acro (rate loop)

    f_attitude_error = ca.Function(
        "attitude_error",
        [q, yaw_r, pitch_r, roll_r],
        [omega.param],
        ["q", "yaw_r", "pitch_r", "roll_r"],
        ["omega"])


    e = SO3EulerB321.from_Quat(X).param

    f_quaternion_to_euler = ca.Function(
       "quaternion_to_euler",
       [q], [e], ["q"], ["e"])

    f_euler_to_quaternion = ca.Function(
       "quaternion_to_euler",
       [roll_r, pitch_r, yaw_r], [R.param], ["yaw", "pitch", "roll"], ["q_wb"])

    eqs = {
            "attitude_error": f_attitude_error,
            "quaternion_to_euler": f_quaternion_to_euler,
            "euler_to_quaternion": f_euler_to_quaternion,
            }
    return eqs

def calculate_N(v: SE23LieAlgebraElement, B: ca.SX):
    omega = v.Omega
    Omega = omega.to_Matrix()
    OmegaSq = Omega @ Omega
    A = ca.sparsify(ca.horzcat(v.a_b.param, v.v_b.param))
    B = ca.sparsify(B)
    theta = ca.norm_2(omega.param)
    C1 = SERIES["(1 - cos(x))/x^2"](theta)
    C2 = SERIES["(x - sin(x))/x^3"](theta)
    C3 = SERIES["(x^2/2 + cos(x) - 1)/x^4"](theta)
    AB = A @ B
    I3 = ca.SX.eye(3)
    return (
        A
        + AB / 2
        + Omega @ A @ (C1 * np.eye(2) + C2 * B)
        + Omega @ Omega @ A @ (C2 * np.eye(2) + C3 * B)
    )


def exp_mixed(
    X0: SE23LieGroupElement,
    l: SE23LieAlgebraElement,
    r: SE23LieAlgebraElement,
    B: ca.SX,
):
    P0 = ca.horzcat(X0.v.param, X0.p.param)
    Pl = calculate_N(l, B)
    Pr = calculate_N(r, -B)
    R0 = X0.R
    Rl = (l).Omega.exp(lie.SO3Quat)
    Rr = (r).Omega.exp(lie.SO3Quat)
    Rr0 = Rr * R0
    R1 = Rr0 * Rl

    I2 = ca.SX.eye(2)
    P1 = Rr0.to_Matrix() @ Pl + (Rr.to_Matrix() @ P0 + Pr) @ (I2 + B)
    return lie.SE23Quat.elem(ca.vertcat(P1[:, 1], P1[:, 0], R1.param))


def derive_strapdown_ins_propagation():
    dt = ca.SX.sym("dt")
    X0 = lie.SE23Quat.elem(ca.SX.sym("X0", 10))
    a_b = ca.SX.sym("a_b", 3)
    g = ca.SX.sym("g")
    omega_b = ca.SX.sym("omega_b", 3)
    l = lie.se23.elem(ca.vertcat(0, 0, 0, a_b, omega_b))
    r = lie.se23.elem(ca.vertcat(0, 0, 0, 0, 0, g, 0, 0, 0))
    B = ca.sparsify(ca.SX([[0, 1], [0, 0]]))
    X1 = exp_mixed(X0, l * dt, r * dt, B * dt)
    r1 = lie.SO3Quat.elem(X1.param[6:10])
    #lie.SO3EulerB321.shadow_if_necessary(r1)
    X1.param[6:10] = r1.param
    f_ins =  ca.Function(
        "strapdown_ins_propagate",
        [X0.param, a_b, omega_b, g, dt],
        [X1.param],
        ["x0", "a_b", "omega_b", "g", "dt"],
        ["x1"],
    )
    eqs = {
        "strapdown_ins_propagate" : f_ins
    }
    return eqs


def position_control():
    #inputs: position trajectory, velocity trajectory, desired Yaw vel, dt, Kp, Kv
    #state inputs: position, orientation, velocity, and angular velocity
    #outputs: thrust force, angular errors
    m = ca.SX.sym('m') # mass of vehicle
    pt_w = ca.SX.sym('pt_w', 3) # desired position world frame
    vt_w = ca.SX.sym('vt_w', 3) # desired velocity world frame
    at_w = ca.SX.sym('at_w', 3) # desired acceleration world frame

    yt = ca.SX.sym('yt') # desired yaw angle
    Kp = ca.SX.sym('Kp') # position proportional gain
    Kv = ca.SX.sym('Kv') # velocity proportional gain

    g = 9.8
    
    p_w = ca.SX.sym('p_w', 3) # position in world frame
    v_b = ca.SX.sym('v_b', 3) # velocity in body frame
    q_wb = SO3Quat.elem(ca.SX.sym('q_wb', 4))
    R_wb = q_wb.to_Matrix()
    
    v_w = R_wb @ v_b

    e_p = p_w - pt_w
    e_v = v_w - vt_w
    
    xW = ca.SX([1, 0, 0])
    yW = ca.SX([0, 1, 0])
    zW = ca.SX([0, 0, 1])

    # thrust vector
    T = -Kp * e_p - Kv * e_v + g*zW + m * at_w

    # thrust
    nT = ca.norm_2(T)
    
    # body up is aligned with thrust
    zB = ca.if_else(nT > 1e-3, T/nT, zW)

    # point y using desired camera direction
    xC = ca.vertcat(ca.cos(yt), ca.sin(yt), 0)
    yB = ca.cross(zB, xC)
    nyB = ca.norm_2(yB)
    yB = ca.if_else(nyB > 1e-3, yB/nyB, xW)

    # point x using cross product of unit vectors
    xB = ca.cross(yB, zB)

    # desired attitude matrix
    Rd_wb = ca.horzcat(xB, yB, zB)
    # [bx_wx by_wx bz_wx]
    # [bx_wy by_wy bz_wy]
    # [bx_wz by_wz bz_wz]

    # deisred euler angles
    # note using euler angles as set point is not problematic
    # using Lie group approach for control
    euler = SO3EulerB321.from_Matrix(Rd_wb)

    f_get_u = ca.Function(
        "position_control",
        [m, pt_w, vt_w, at_w, yt, Kp, Kv, p_w, v_b, q_wb.param], [nT, euler.param], 
        ['m', 'pt_w', 'vt_w', 'at_w', 'yt', 'Kp', 'Kv', 'p_w', 'v_b', 'q_wb'], 
        ['nT', 'euler'])
    
    eqs = {
        "position_control" : f_get_u
    }

    return eqs

def generate_code(eqs: dict, filename, dest_dir: str, **kwargs):
    dest_dir = Path(dest_dir)
    dest_dir.mkdir(exist_ok=True)
    p = {
        "verbose": True,
        "mex": False,
        "cpp": False,
        "main": False,
        "with_header": True,
        "with_mem": False,
        "with_export": False,
        "with_import": False,
        "include_math": True,
        "avoid_stack": True,
    }
    for k, v in kwargs.items():
        assert k in p.keys()
        p[k] = v

    gen = ca.CodeGenerator(filename, p)
    for name, eq in eqs.items():
        gen.add(eq)
    gen.generate(str(dest_dir) + os.sep)

if __name__ == "__main__":
    print("generating casadi equations")
    eqs = {}
    eqs.update(derive_attitude_error())
    eqs.update(position_control())
    eqs.update(derive_strapdown_ins_propagation())

    for name, eq in eqs.items():
        print('eq: ', name)

    generate_code(eqs, filename="rdd2.c", dest_dir="gen")
    print("complete")
