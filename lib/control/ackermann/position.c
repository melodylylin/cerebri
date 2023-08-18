/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <synapse/zbus/common.h>
#include <synapse/zbus/syn_pub_sub.h>
#include <zephyr/logging/log.h>

#include "casadi/rover.h"

#define MY_STACK_SIZE 3072
#define MY_PRIORITY 4

LOG_MODULE_DECLARE(control_ackermann);

typedef struct _context {
    synapse_msgs_Fsm fsm;
    synapse_msgs_BezierTrajectory bezier_trajectory;
    synapse_msgs_Time clock_offset;
    synapse_msgs_Odometry pose;
    synapse_msgs_Twist cmd_vel;
    struct syn_sub sub_fsm, sub_clock_offset, sub_pose, sub_bezier_trajectory;
    struct syn_pub pub_cmd_vel;
    const double wheel_base;
    const double gain_along_track;
    const double gain_cross_track;
    const double gain_heading;
} context;

static context g_ctx = {
    .fsm = synapse_msgs_Fsm_init_default,
    .bezier_trajectory = synapse_msgs_BezierTrajectory_init_default,
    .clock_offset = synapse_msgs_Time_init_default,
    .pose = synapse_msgs_Odometry_init_default,
    .cmd_vel = {
        .has_angular = true,
        .has_linear = true,
        .linear = synapse_msgs_Vector3_init_default,
        .angular = synapse_msgs_Vector3_init_default,
    },
    .sub_fsm = { 0 },
    .sub_clock_offset = { 0 },
    .sub_pose = { 0 },
    .sub_bezier_trajectory = { 0 },
    .pub_cmd_vel = { 0 },
    .wheel_base = CONFIG_CONTROL_ACKERMANN_WHEEL_BASE_MM / 1000.0,
    .gain_along_track = CONFIG_CONTROL_ACKERMANN_GAIN_ALONG_TRACK / 1000.0,
    .gain_cross_track = CONFIG_CONTROL_ACKERMANN_GAIN_CROSS_TRACK / 1000.0,
    .gain_heading = CONFIG_CONTROL_ACKERMANN_GAIN_HEADING / 1000.0,
};

static void init(context* ctx)
{
    syn_sub_init(&ctx->sub_fsm, &ctx->fsm, &chan_out_fsm);
    syn_sub_init(&ctx->sub_clock_offset, &ctx->clock_offset, &chan_in_clock_offset);
    syn_sub_init(&ctx->sub_pose, &ctx->pose, &chan_out_odometry);
    syn_sub_init(&ctx->sub_bezier_trajectory, &ctx->bezier_trajectory, &chan_in_bezier_trajectory);
    syn_pub_init(&ctx->pub_cmd_vel, &ctx->cmd_vel, &chan_out_cmd_vel);
}

static void stop(context* ctx)
{
    ctx->cmd_vel.linear.x = 0;
    ctx->cmd_vel.angular.z = 0;
}

// computes thrust/steering in auto mode
static void auto_mode(context* ctx)
{
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
        stop(ctx);
        return;
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
            // LOG_ERR("curve index exceeds bounds");
            stop(ctx);
            return;
        }
    }

    double T = (time_stop_nsec - time_start_nsec) * 1e-9;
    double t = (time_nsec - time_start_nsec) * 1e-9;
    double x, y, psi, V, omega = 0;
    double e[3] = {}; // e_x, e_y, e_theta

    double PX[6], PY[6];
    for (int i = 0; i < 6; i++) {
        PX[i] = ctx->bezier_trajectory.curves[curve_index].x[i];
        PY[i] = ctx->bezier_trajectory.curves[curve_index].y[i];
    }

    // casadi mem args
    casadi_int* iw = NULL;
    casadi_real* w = NULL;
    int mem = 0;

    /* bezier6_rover:(t,T,PX[1x6],PY[1x6],L)->(x,y,psi,V,omega) */
    {
        const casadi_real* args[5];
        casadi_real* res[5];
        args[0] = &t;
        args[1] = &T;
        args[2] = PX;
        args[3] = PY;
        args[4] = &ctx->wheel_base;
        res[0] = &x;
        res[1] = &y;
        res[2] = &psi;
        res[3] = &V;
        res[4] = &omega;
        bezier6_rover(args, res, iw, w, mem);
    }

    /* se2_error:(p[3],r[3])->(error[3]) */
    {
        const casadi_real* args[2];
        casadi_real* res[1];

        double p[3], r[3];

        // vehicle position
        p[0] = ctx->pose.pose.pose.position.x;
        p[1] = ctx->pose.pose.pose.position.y;
        p[2] = 2 * atan2(ctx->pose.pose.pose.orientation.z, ctx->pose.pose.pose.orientation.w);

        // reference position
        r[0] = x;
        r[1] = y;
        r[2] = psi;

        // call function
        args[0] = p;
        args[1] = r;
        res[0] = e;
        se2_error(args, res, iw, w, mem);
    }

    // compute twist
    ctx->cmd_vel.linear.x = V + ctx->gain_along_track * e[0];
    ctx->cmd_vel.angular.z = omega + ctx->gain_cross_track * e[1] + ctx->gain_heading * e[2];
}

static void run(context* ctx)
{
    init(ctx);

    while (true) {

        // LOG_INF("polling on pose");
        int ret = syn_sub_poll(&ctx->sub_pose, K_MSEC(1000));

        if (ctx->fsm.mode != synapse_msgs_Fsm_Mode_AUTO) {
            // LOG_INF("not auto mode");
            continue;
        }

        // LOG_INF("in auto mode");

        // lock topics
        syn_sub_claim(&ctx->sub_fsm, K_MSEC(1));
        syn_sub_claim(&ctx->sub_clock_offset, K_MSEC(1));
        syn_sub_claim(&ctx->sub_pose, K_MSEC(1));
        syn_sub_claim(&ctx->sub_bezier_trajectory, K_MSEC(1));
        syn_pub_claim(&ctx->pub_cmd_vel, K_MSEC(1));

        if (ret != 0) {
            LOG_ERR("not receiving odometry");
            continue;
        }
        auto_mode(ctx);

        // unlock topics
        syn_sub_finish(&ctx->sub_fsm);
        syn_sub_finish(&ctx->sub_clock_offset);
        syn_sub_finish(&ctx->sub_pose);
        syn_sub_finish(&ctx->sub_bezier_trajectory);
        syn_pub_finish(&ctx->pub_cmd_vel);

        syn_pub_publish(&ctx->pub_cmd_vel, K_MSEC(1));
    }
}

K_THREAD_DEFINE(control_ackermann_pos, MY_STACK_SIZE,
    run, &g_ctx, NULL, NULL,
    MY_PRIORITY, 0, 0);

static void listener_control_ackermann_pos_callback(const struct zbus_channel* chan)
{
    syn_sub_listen(&g_ctx.sub_fsm, chan, K_MSEC(1));
    syn_sub_listen(&g_ctx.sub_clock_offset, chan, K_MSEC(1));
    syn_sub_listen(&g_ctx.sub_pose, chan, K_MSEC(1));
    syn_sub_listen(&g_ctx.sub_bezier_trajectory, chan, K_MSEC(1));
    if (chan == &chan_in_cmd_vel && g_ctx.fsm.mode == synapse_msgs_Fsm_Mode_CMD_VEL) {
        syn_pub_claim(&g_ctx.pub_cmd_vel, K_MSEC(1));
        memcpy(&g_ctx.cmd_vel, chan->message, chan->message_size);
        syn_pub_finish(&g_ctx.pub_cmd_vel);
        syn_pub_publish(&g_ctx.pub_cmd_vel, K_MSEC(1));
    }
}
ZBUS_LISTENER_DEFINE(listener_control_ackermann_pos, listener_control_ackermann_pos_callback);
ZBUS_CHAN_ADD_OBS(chan_out_fsm, listener_control_ackermann_pos, 1);
ZBUS_CHAN_ADD_OBS(chan_in_clock_offset, listener_control_ackermann_pos, 1);
ZBUS_CHAN_ADD_OBS(chan_out_odometry, listener_control_ackermann_pos, 1);
ZBUS_CHAN_ADD_OBS(chan_in_bezier_trajectory, listener_control_ackermann_pos, 1);
ZBUS_CHAN_ADD_OBS(chan_in_cmd_vel, listener_control_ackermann_pos, 1);
/* vi: ts=4 sw=4 et */
