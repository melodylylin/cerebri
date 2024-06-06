/*
 * Copyright CogniPilot Foundation 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_pub_struct.h>
#include <zros/private/zros_sub_struct.h>
#include <zros/zros_node.h>
#include <zros/zros_pub.h>
#include <zros/zros_sub.h>

#include <synapse_topic_list.h>

#include <cerebri/core/casadi.h>

#include "casadi/gen/bezier.h"
#include "casadi/gen/common.h"
#include "casadi/gen/rdd2.h"

#define MY_STACK_SIZE 4096
#define MY_PRIORITY 4

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LOG_MODULE_REGISTER(rdd2_command, CONFIG_CEREBRI_RDD2_LOG_LEVEL);

static K_THREAD_STACK_DEFINE(g_my_stack_area, MY_STACK_SIZE);
static const double deg2rad = M_PI / 180.0;
static const double thrust_trim = CONFIG_CEREBRI_RDD2_THRUST_TRIM * 1e-3;
static const double thrust_delta = CONFIG_CEREBRI_RDD2_THRUST_DELTA * 1e-3;

struct context {
    struct zros_node node;
    synapse_msgs_Joy joy;
    synapse_msgs_Vector3 angular_velocity_ff, force_sp, accel_ff, moment_ff, velocity_sp, position_sp;
    synapse_msgs_Quaternion attitude_sp, orientation_sp;
    synapse_msgs_BezierTrajectory bezier_trajectory;
    synapse_msgs_Time clock_offset;
    synapse_msgs_Status status;
    synapse_msgs_Status last_status;
    synapse_msgs_Odometry estimator_odometry;
    synapse_msgs_Twist cmd_vel;
    struct zros_sub sub_offboard_bezier_trajectory, sub_status, sub_joy, sub_offboard_joy,
        sub_estimator_odometry, sub_offboard_cmd_vel, sub_offboard_clock_offset;
    struct zros_pub pub_attitude_sp, pub_angular_velocity_ff, pub_force_sp, pub_accel_ff, pub_moment_ff,
        pub_velocity_sp, pub_orientation_sp, pub_position_sp;
    atomic_t running;
    size_t stack_size;
    k_thread_stack_t* stack_area;
    struct k_thread thread_data;
    double camera_yaw;
};

static struct context g_ctx = {
    .node = {},
    .joy = synapse_msgs_Joy_init_default,
    .attitude_sp = synapse_msgs_Quaternion_init_default,
    .angular_velocity_ff = synapse_msgs_Vector3_init_default,
    .force_sp = synapse_msgs_Vector3_init_default,
    .bezier_trajectory = synapse_msgs_BezierTrajectory_init_default,
    .status = synapse_msgs_Status_init_default,
    .last_status = synapse_msgs_Status_init_default,
    .velocity_sp = synapse_msgs_Vector3_init_default,
    .accel_ff = synapse_msgs_Vector3_init_default,
    .moment_ff = synapse_msgs_Vector3_init_default,
    .orientation_sp = synapse_msgs_Quaternion_init_default,
    .position_sp = synapse_msgs_Vector3_init_default,
    .cmd_vel = synapse_msgs_Twist_init_default,
    .sub_offboard_joy = {},
    .sub_joy = {},
    .sub_status = {},
    .sub_offboard_bezier_trajectory = {},
    .sub_offboard_clock_offset = {},
    .sub_estimator_odometry = {},
    .sub_offboard_cmd_vel = {},
    .pub_attitude_sp = {},
    .pub_angular_velocity_ff = {},
    .pub_force_sp = {},
    .pub_velocity_sp = {},
    .pub_accel_ff = {},
    .pub_moment_ff = {},
    .pub_orientation_sp = {},
    .pub_position_sp = {},
    .running = ATOMIC_INIT(0),
    .stack_size = MY_STACK_SIZE,
    .stack_area = g_my_stack_area,
    .thread_data = {},
    .camera_yaw = 0,
};

static void rdd2_command_init(struct context* ctx)
{
    LOG_INF("init");
    zros_node_init(&ctx->node, "rdd2_command");
    zros_sub_init(&ctx->sub_offboard_joy, &ctx->node, &topic_offboard_joy, &ctx->joy, 10);
    zros_sub_init(&ctx->sub_joy, &ctx->node, &topic_joy, &ctx->joy, 10);
    zros_sub_init(&ctx->sub_status, &ctx->node, &topic_status, &ctx->status, 10);
    zros_sub_init(&ctx->sub_offboard_bezier_trajectory,
        &ctx->node, &topic_offboard_bezier_trajectory, &ctx->bezier_trajectory, 10);
    zros_sub_init(&ctx->sub_offboard_clock_offset,
        &ctx->node, &topic_offboard_clock_offset, &ctx->clock_offset, 10);
    zros_sub_init(&ctx->sub_estimator_odometry, &ctx->node, &topic_estimator_odometry, &ctx->estimator_odometry, 10);
    zros_sub_init(&ctx->sub_offboard_cmd_vel, &ctx->node, &topic_offboard_cmd_vel, &ctx->cmd_vel, 10);
    zros_pub_init(&ctx->pub_attitude_sp, &ctx->node,
        &topic_attitude_sp, &ctx->attitude_sp);
    zros_pub_init(&ctx->pub_angular_velocity_ff, &ctx->node,
        &topic_angular_velocity_sp, &ctx->angular_velocity_ff);
    zros_pub_init(&ctx->pub_force_sp, &ctx->node,
        &topic_force_sp, &ctx->force_sp);
    zros_pub_init(&ctx->pub_velocity_sp, &ctx->node,
        &topic_velocity_sp, &ctx->velocity_sp);
    zros_pub_init(&ctx->pub_accel_ff, &ctx->node,
        &topic_accel_sp, &ctx->accel_ff);
    zros_pub_init(&ctx->pub_moment_ff, &ctx->node,
        &topic_moment_ff, &ctx->moment_ff);
    zros_pub_init(&ctx->pub_orientation_sp, &ctx->node,
        &topic_orientation_sp, &ctx->orientation_sp);
    zros_pub_init(&ctx->pub_position_sp, &ctx->node,
        &topic_position_sp, &ctx->position_sp);
    atomic_set(&ctx->running, 1);
}

static void rdd2_command_fini(struct context* ctx)
{
    LOG_INF("fini");
    zros_node_fini(&ctx->node);
    zros_sub_fini(&ctx->sub_offboard_joy);
    zros_sub_fini(&ctx->sub_joy);
    zros_sub_fini(&ctx->sub_status);
    zros_sub_fini(&ctx->sub_offboard_bezier_trajectory);
    zros_sub_fini(&ctx->sub_estimator_odometry);
    zros_sub_fini(&ctx->sub_offboard_cmd_vel);
    zros_pub_fini(&ctx->pub_attitude_sp);
    zros_pub_fini(&ctx->pub_angular_velocity_ff);
    zros_pub_fini(&ctx->pub_velocity_sp);
    zros_pub_fini(&ctx->pub_accel_ff);
    zros_pub_fini(&ctx->pub_force_sp);
    zros_pub_fini(&ctx->pub_orientation_sp);
    zros_pub_fini(&ctx->pub_position_sp);
    atomic_set(&ctx->running, 0);
}

static void rdd2_command_run(void* p0, void* p1, void* p2)
{
    struct context* ctx = p0;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);

    rdd2_command_init(ctx);

    struct k_poll_event events[] = {
        *zros_sub_get_event(&ctx->sub_offboard_joy),
        *zros_sub_get_event(&ctx->sub_joy),
        *zros_sub_get_event(&ctx->sub_offboard_cmd_vel),
        *zros_sub_get_event(&ctx->sub_offboard_bezier_trajectory),
    };

    double dt = 0;
    int64_t ticks_last = k_uptime_ticks();

    while (atomic_get(&ctx->running)) {
        // wait for joystick input event, publish at 1 Hz regardless
        int rc = 0;
        rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(1000));
        if (rc != 0) {
            // LOG_DBG("not receiving joy");
            ctx->status.mode = synapse_msgs_Status_Mode_MODE_ATTITUDE;
            ctx->joy.axes[JOY_AXES_LEFT_STICK_LEFT] = 0;
            ctx->joy.axes[JOY_AXES_LEFT_STICK_UP] = 0;
            ctx->joy.axes[JOY_AXES_RIGHT_STICK_LEFT] = 0;
            ctx->joy.axes[JOY_AXES_RIGHT_STICK_UP] = 0;
            ctx->cmd_vel.linear.x = 0;
            ctx->cmd_vel.linear.y = 0;
            ctx->cmd_vel.linear.z = 0;
            ctx->cmd_vel.angular.x = 0;
            ctx->cmd_vel.angular.y = 0;
            ctx->cmd_vel.angular.z = 0;
            ctx->cmd_vel.has_linear = true;
            ctx->cmd_vel.has_angular = true;
        }

        // prioritize onboard joy
        if (zros_sub_update_available(&ctx->sub_joy)) {
            zros_sub_update(&ctx->sub_joy);
        } else if (zros_sub_update_available(&ctx->sub_offboard_joy)) {
            zros_sub_update(&ctx->sub_offboard_joy);
        }

        if (zros_sub_update_available(&ctx->sub_status)) {
            // record last status
            ctx->last_status = ctx->status;
            zros_sub_update(&ctx->sub_status);
        }

        if (zros_sub_update_available(&ctx->sub_offboard_bezier_trajectory)) {
            zros_sub_update(&ctx->sub_offboard_bezier_trajectory);
        }

        if (zros_sub_update_available(&ctx->sub_estimator_odometry)) {
            zros_sub_update(&ctx->sub_estimator_odometry);
        }

        if (zros_sub_update_available(&ctx->sub_offboard_cmd_vel)) {
            zros_sub_update(&ctx->sub_offboard_cmd_vel);
        }

        // calculate dt
        int64_t ticks_now = k_uptime_ticks();
        dt = (double)(ticks_now - ticks_last) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
        ticks_last = ticks_now;
        if (dt < 0 || dt > 0.5) {
            LOG_DBG("joy update rate too low");
            continue;
        }

        // joy data
        double joy_roll = -(double)ctx->joy.axes[JOY_AXES_RIGHT_STICK_LEFT];
        double joy_pitch = (double)ctx->joy.axes[JOY_AXES_RIGHT_STICK_UP];
        double joy_yaw = (double)ctx->joy.axes[JOY_AXES_LEFT_STICK_LEFT];
        double joy_thrust = (double)ctx->joy.axes[JOY_AXES_LEFT_STICK_UP];

        // estimated attitude quaternion
        double q[4] = {
            ctx->estimator_odometry.pose.pose.orientation.w,
            ctx->estimator_odometry.pose.pose.orientation.x,
            ctx->estimator_odometry.pose.pose.orientation.y,
            ctx->estimator_odometry.pose.pose.orientation.z
        };

        // handle joy based on mode
        if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_ATTITUDE_RATE) {
            double omega[3];
            double thrust;
            {
                // joy_acro:(thrust_trim,thrust_delta,joy_roll,joy_pitch,joy_yaw,joy_thrust)->(omega[3],thrust)
                CASADI_FUNC_ARGS(joy_acro);

                args[0] = &thrust_trim;
                args[1] = &thrust_delta;
                args[2] = &joy_roll;
                args[3] = &joy_pitch;
                args[4] = &joy_yaw;
                args[5] = &joy_thrust;

                res[0] = omega;
                res[1] = &thrust;

                CASADI_FUNC_CALL(joy_acro);
            }

            bool data_ok = true;
            for (int i = 0; i < 3; i++) {
                if (!isfinite(omega[i])) {
                    LOG_ERR("omega[%d] not finite: %10.4f", i, omega[i]);
                    data_ok = false;
                    break;
                }
            }
            if (!isfinite(thrust)) {
                LOG_ERR("thrust not finite: %10.4f", thrust);
                data_ok = false;
            }

            if (data_ok) {
                // angular velocity set point
                ctx->angular_velocity_ff.x = omega[0];
                ctx->angular_velocity_ff.y = omega[1];
                ctx->angular_velocity_ff.z = omega[2];
                zros_pub_update(&ctx->pub_angular_velocity_ff);

                // thrust pass through
                ctx->force_sp.z = thrust;
                zros_pub_update(&ctx->pub_force_sp);
            }

        } else if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_ATTITUDE) {
            double qr[4];
            double thrust;
            {
                /* joy_auto_level:(thrust_trim,thrust_delta,joy_roll,joy_pitch,joy_yaw,joy_thrust,q[4])->(q_r[4],thrust) */
                CASADI_FUNC_ARGS(joy_auto_level);

                args[0] = &thrust_trim;
                args[1] = &thrust_delta;
                args[2] = &joy_roll;
                args[3] = &joy_pitch;
                args[4] = &joy_yaw;
                args[5] = &joy_thrust;
                args[6] = q;

                res[0] = qr;
                res[1] = &thrust;

                CASADI_FUNC_CALL(joy_auto_level);
            }

            bool data_ok = true;
            for (int i = 0; i < 3; i++) {
                if (!isfinite(qr[i])) {
                    LOG_ERR("qr[%d] not finite: %10.4f", i, qr[i]);
                    data_ok = false;
                    break;
                }
            }

            if (!isfinite(thrust)) {
                LOG_ERR("thrust not finite: %10.4f", thrust);
                data_ok = false;
            }

            // attitude set point
            if (data_ok) {
                ctx->attitude_sp.w = qr[0];
                ctx->attitude_sp.x = qr[1];
                ctx->attitude_sp.y = qr[2];
                ctx->attitude_sp.z = qr[3];
                zros_pub_update(&ctx->pub_attitude_sp);

                // thrust pass through
                ctx->force_sp.z = thrust;
                zros_pub_update(&ctx->pub_force_sp);
            }

        } else if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_VELOCITY) {

            bool now_vel = ctx->last_status.mode != synapse_msgs_Status_Mode_MODE_VELOCITY;
            bool now_armed = (ctx->status.arming == synapse_msgs_Status_Arming_ARMING_ARMED) && (ctx->last_status.arming != synapse_msgs_Status_Arming_ARMING_ARMED);

            double yaw, pitch, roll;
            {
                /* quat_to_eulerB321:(q[4])->(yaw, pitch, roll) */
                CASADI_FUNC_ARGS(quat_to_eulerB321);

                args[0] = q;

                res[0] = &yaw;
                res[1] = &pitch;
                res[2] = &roll;

                CASADI_FUNC_CALL(quat_to_eulerB321);
            }

            // reset position setpoint if now auto or now armed
            if (now_vel || now_armed) {
                LOG_INF("position_sp, camera_yaw reset");
                ctx->position_sp.x = ctx->estimator_odometry.pose.pose.position.x;
                ctx->position_sp.y = ctx->estimator_odometry.pose.pose.position.y;
                ctx->position_sp.z = ctx->estimator_odometry.pose.pose.position.z;
                ctx->camera_yaw = yaw;
            }

            double yaw_rate = 0;
            double vb[3] = { 0, 0, 0 };

            if (ctx->status.topic_source == synapse_msgs_Status_TopicSource_TOPIC_SOURCE_JOY) {
                yaw_rate = 60 * deg2rad * joy_yaw;
                vb[0] = 2.0 * joy_pitch;
                vb[1] = -2.0 * joy_roll; // positive roll is negative y
                vb[2] = joy_thrust;
                // LOG_INF("onboard yawrate: %10.4f vbx: %10.4f %10.4f %10.4f", yaw_rate, vbx, vby, vbz);
            } else if (ctx->status.topic_source == synapse_msgs_Status_TopicSource_TOPIC_SOURCE_ETHERNET) {
                yaw_rate = ctx->cmd_vel.angular.z;
                vb[0] = ctx->cmd_vel.linear.x;
                vb[1] = ctx->cmd_vel.linear.y;
                vb[2] = joy_thrust;
                // LOG_INF("offboard yawrate: %10.4f vbx: %10.4f %10.4f %10.4f", yaw_rate, vbx, vby, vbz);
            }

            double vw[3] = {
                vb[0] * cos(yaw) - vb[1] * sin(yaw),
                vb[0] * sin(yaw) + vb[1] * cos(yaw),
                vb[2]
            };

            // position
            ctx->position_sp.x += dt * vw[0];
            ctx->position_sp.y += dt * vw[1];
            ctx->position_sp.z += dt * vw[2];

            double e[3] = {
                ctx->position_sp.x - ctx->estimator_odometry.pose.pose.position.x,
                ctx->position_sp.y - ctx->estimator_odometry.pose.pose.position.y,
                ctx->position_sp.z - ctx->estimator_odometry.pose.pose.position.z
            };

            double norm_e = sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);

            const double pos_error_max = 2.0;

            // saturate position setpoint distance from vehicle
            if (norm_e > pos_error_max) {
                ctx->position_sp.x = ctx->estimator_odometry.pose.pose.position.x + e[0] * pos_error_max / norm_e;
                ctx->position_sp.y = ctx->estimator_odometry.pose.pose.position.y + e[1] * pos_error_max / norm_e;
                ctx->position_sp.z = ctx->estimator_odometry.pose.pose.position.z + e[2] * pos_error_max / norm_e;
            }

            // desired camera direction
            ctx->camera_yaw += dt * yaw_rate;

            // LOG_INF("camera yaw: %10.4f", ctx->camera_yaw);

            double qc[4];
            {
                /* eulerB321_to_quat:(yaw, pitch, roll)->(q[4]) */
                CASADI_FUNC_ARGS(eulerB321_to_quat);
                double pitch_r = 0;
                double roll_r = 0;
                args[0] = &ctx->camera_yaw;
                args[1] = &pitch_r;
                args[2] = &roll_r;
                res[0] = qc;
                CASADI_FUNC_CALL(eulerB321_to_quat);
            }

            // position setpoint
            zros_pub_update(&ctx->pub_position_sp);

            // velocity setpoint
            ctx->velocity_sp.x = vw[0];
            ctx->velocity_sp.y = vw[1];
            ctx->velocity_sp.z = vw[2];
            zros_pub_update(&ctx->pub_velocity_sp);

            // orientation setpoint
            ctx->orientation_sp.w = qc[0];
            ctx->orientation_sp.x = qc[1];
            ctx->orientation_sp.y = qc[2];
            ctx->orientation_sp.z = qc[3];
            zros_pub_update(&ctx->pub_orientation_sp);

            // acceleration setpoint
            ctx->accel_ff.x = 0;
            ctx->accel_ff.y = 0;
            ctx->accel_ff.z = 0;
            zros_pub_update(&ctx->pub_accel_ff);

        } else if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_BEZIER) {
            // goal -> given position goal, find cmd_vel
            uint64_t time_start_nsec = ctx->bezier_trajectory.time_start;
            uint64_t time_stop_nsec = time_start_nsec;

            // get current time
            uint64_t time_nsec = k_uptime_get() * 1e6 + ctx->clock_offset.sec * 1e9 + ctx->clock_offset.nanosec;

            if (time_nsec < time_start_nsec) {
                LOG_DBG("time current: %" PRIu64
                        " ns < time start: %" PRIu64
                        "  ns, time out of range of trajectory\n",
                    time_nsec, time_start_nsec);
                // stop(ctx);
                continue;
            }

            // find current trajectory index, time_start, and time_stop
            int curve_index = 0;

            while (true) {

                // check if time handled by current trajectory
                if (time_nsec < ctx->bezier_trajectory.curves[curve_index].time_stop) {
                    time_stop_nsec = ctx->bezier_trajectory.curves[curve_index].time_stop;
                    if (curve_index > 0) {
                        time_start_nsec = ctx->bezier_trajectory.curves[curve_index - 1].time_stop;
                    }
                    break;
                }

                // next index
                curve_index++;

                // check if index exceeds bounds
                if (curve_index >= ctx->bezier_trajectory.curves_count) {
                    // LOG_DBG("curve index exceeds bounds");
                    // stop(ctx);
                    continue;
                }
            }

            double T = (time_stop_nsec - time_start_nsec) * 1e-9;
            double t = (time_nsec - time_start_nsec) * 1e-9;
            double x, y, z, psi, dpsi, ddpsi = 0;
            double v[3], a[3], j[3], s[3];
            double PX[8], PY[8], PZ[8], Ppsi[4];
            for (int i = 0; i < 8; i++) {
                PX[i] = ctx->bezier_trajectory.curves[curve_index].x[i];
                PY[i] = ctx->bezier_trajectory.curves[curve_index].y[i];
                PZ[i] = ctx->bezier_trajectory.curves[curve_index].z[i];
            }

            for (int i = 0; i < 4; i++) {
                Ppsi[i] = ctx->bezier_trajectory.curves[curve_index].yaw[i];
            }

            // bezier_multirotor:(t,T,PX[1x8],PY[1x8],PX[1x8],Ppsi[1x4])
            // ->(x,y,z,psi,dpsi,ddpsi,V,a,j,s)
            {
                CASADI_FUNC_ARGS(bezier_multirotor);
                args[0] = &t;
                args[1] = &T;
                args[2] = PX;
                args[3] = PY;
                args[4] = PZ;
                args[5] = Ppsi;
                res[0] = &x;
                res[1] = &y;
                res[2] = &z;
                res[3] = &psi;
                res[4] = &dpsi;
                res[5] = &ddpsi;
                res[6] = v;
                res[7] = a;
                res[8] = j;
                res[9] = s;
                CASADI_FUNC_CALL(bezier_multirotor);
            }

            double q_att[4], omega[3], M[3];
            // world to body
            // f_ref:(psi,psi_dot,psi_ddot,v_e[3],a_e[3],j_e[3],s_e[3])
            // ->(v_b[3],quat[4],omega_eb_b[3],omega_dot_eb_b[3],M_b[3],T)
            {
                CASADI_FUNC_ARGS(f_ref);
                args[0] = &psi;
                args[1] = &dpsi;
                args[2] = &ddpsi;
                args[3] = v;
                args[4] = a;
                args[5] = j;
                args[6] = s;
                res[1] = q_att;
                res[2] = omega;
                res[4] = M;
                CASADI_FUNC_CALL(f_ref);
            }

            /* euler to quat */
            double q_orientation[4];
            {
                CASADI_FUNC_ARGS(eulerB321_to_quat);
                double phi = 0;
                double theta = 0;
                args[0] = &psi;
                args[1] = &theta;
                args[2] = &phi;

                res[0] = q_orientation;
                CASADI_FUNC_CALL(eulerB321_to_quat);
            }

            // position sp
            ctx->position_sp.x = x;
            ctx->position_sp.y = y;
            ctx->position_sp.z = z;
            zros_pub_update(&ctx->pub_position_sp);

            // velocity sp
            ctx->velocity_sp.x = v[0];
            ctx->velocity_sp.y = v[1];
            ctx->velocity_sp.z = v[2];
            zros_pub_update(&ctx->pub_velocity_sp);

            // acceleration ff
            ctx->accel_ff.x = a[0];
            ctx->accel_ff.y = a[1];
            ctx->accel_ff.z = a[2];
            zros_pub_update(&ctx->pub_accel_ff);

            // attitude sp
            // ctx->attitude_sp.w = q_att[0];
            // ctx->attitude_sp.x = q_att[1];
            // ctx->attitude_sp.y = q_att[2];
            // ctx->attitude_sp.z = q_att[3];

            // angular velocity ff
            ctx->angular_velocity_ff.x = omega[0];
            ctx->angular_velocity_ff.y = omega[1];
            ctx->angular_velocity_ff.z = omega[2];
            zros_pub_update(&ctx->pub_angular_velocity_ff);

            // moment ff
            ctx->moment_ff.x = 0; // M[0];
            ctx->moment_ff.y = 0; // M[1];
            ctx->moment_ff.z = 0; // M[2];
            zros_pub_update(&ctx->pub_moment_ff);

            // orientation sp
            ctx->orientation_sp.w = q_orientation[0];
            ctx->orientation_sp.x = q_orientation[1];
            ctx->orientation_sp.y = q_orientation[2];
            ctx->orientation_sp.z = q_orientation[3];
            zros_pub_update(&ctx->pub_orientation_sp);

        } else if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_UNKNOWN) {
            // LOG_ERR("unknown mode");
        }
    }

    rdd2_command_fini(ctx);
}

static int start(struct context* ctx)
{
    k_tid_t tid = k_thread_create(&ctx->thread_data, ctx->stack_area,
        ctx->stack_size,
        rdd2_command_run,
        ctx, NULL, NULL,
        MY_PRIORITY, 0, K_FOREVER);
    k_thread_name_set(tid, "rdd2_command");
    k_thread_start(tid);
    return 0;
}

static int rdd2_command_cmd_handler(const struct shell* sh,
    size_t argc, char** argv, void* data)
{
    struct context* ctx = data;
    if (argc != 1) {
        LOG_ERR("must have one argument");
        return -1;
    }

    if (strcmp(argv[0], "start") == 0) {
        if (atomic_get(&ctx->running)) {
            shell_print(sh, "already running");
        } else {
            start(ctx);
        }
    } else if (strcmp(argv[0], "stop") == 0) {
        if (atomic_get(&ctx->running)) {
            atomic_set(&ctx->running, 0);
        } else {
            shell_print(sh, "not running");
        }
    } else if (strcmp(argv[0], "status") == 0) {
        shell_print(sh, "running: %d", (int)atomic_get(&ctx->running));
    }
    return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_rdd2_command, rdd2_command_cmd_handler,
    (start, &g_ctx, "start"),
    (stop, &g_ctx, "stop"),
    (status, &g_ctx, "status"));

SHELL_CMD_REGISTER(rdd2_command, &sub_rdd2_command, "rdd2 command arguments", NULL);

static int rdd2_command_sys_init(void)
{
    return start(&g_ctx);
};

SYS_INIT(rdd2_command_sys_init, APPLICATION, 1);

// vi: ts=4 sw=4 et
