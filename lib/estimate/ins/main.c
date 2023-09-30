/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cerebri/synapse/zbus/common.h>
#include <cerebri/synapse/zbus/syn_pub_sub.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define casadi_real double
#define casadi_int int64_t

#include "casadi/strapdown_ins.h"

LOG_MODULE_REGISTER(estimate_ins, CONFIG_CEREBRI_ESTIMATE_INS_LOG_LEVEL);

#define MY_STACK_SIZE 4096
#define MY_PRIORITY 4

// private context
typedef struct _context {
    syn_node_t node;
    synapse_msgs_Imu imu;
    synapse_msgs_Odometry odometry;
    syn_sub_t sub_imu;
    syn_pub_t pub_odometry;
    double x[9];
} context;

// private initialization
static context g_ctx = {
    .imu = synapse_msgs_Imu_init_default,
    .odometry = {
        .has_header = true,
        .header.frame_id = "odom",
        .header.has_stamp = true,
        .header.stamp = synapse_msgs_Time_init_default,
        .child_frame_id = "base_link",
        .has_pose = true,
        .pose.has_pose = true,
        .pose.pose.has_position = true,
        .pose.pose.has_orientation = true,
        .has_twist = true,
        .twist.has_twist = true,
    },
    .sub_imu = { 0 },
    .pub_odometry = { 0 },
    .x = { 0 },
};

static void estimate_ins_init(context* ctx)
{
    syn_node_init(&ctx->node, "estimate_ins");
    syn_node_add_sub(
        &ctx->node, &ctx->sub_imu, &ctx->imu, &chan_out_imu);
    syn_node_add_pub(&ctx->node, &ctx->pub_odometry, &ctx->odometry, &chan_out_odometry);
}

void listener_estimate_ins_callback(const struct zbus_channel* chan)
{
    syn_node_listen(&g_ctx.node, chan, K_MSEC(1));
}
ZBUS_LISTENER_DEFINE(listener_estimate_ins, listener_estimate_ins_callback);
ZBUS_CHAN_ADD_OBS(chan_out_imu, listener_estimate_ins, 1);

void log_x(double* x)
{
    LOG_DBG("%10.4f %10.4f %10.4f %10.4f %10.4f %10.4f %10.4f",
        x[0], x[1], x[2], x[3], x[4], x[5], x[6]);
}

bool all_finite(double* src, size_t n)
{
    for (int i = 0; i < n; i++) {
        if (!isfinite(src[i])) {
            return false;
        }
    }
    return true;
}

void handle_update(double* x1)
{
    bool x1_finite = all_finite(x1, sizeof(g_ctx.x));

    if (!x1_finite) {
        LOG_WRN("x1 update not finite");
    }

    if (x1_finite) {
        memcpy(g_ctx.x, x1, sizeof(g_ctx.x));
    }
}

static void estimate_ins_entry_point(context* ctx)
{
    estimate_ins_init(ctx);

    // parameters
    const double g = -9.8;
    int64_t ticks_last = k_uptime_ticks();

    double y_accel[3], y_gyro[3];

    // estimator state and sqrt covariance
    while (true) {

        RC(syn_sub_poll(&ctx->sub_imu, K_MSEC(100)),
            LOG_DBG("not receiving imu");
            continue);

        int64_t ticks_now = k_uptime_ticks();
        double dt = (double)(ticks_now - ticks_last) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
        ticks_last = k_uptime_ticks();

        if (dt < 0 || dt > 0.1) {
            LOG_DBG("dt out of range: %10.4f", dt);
            continue;
        }

        // good data, lock pubs/subs
        syn_node_lock_all(&ctx->node, K_MSEC(1));

        // get data
        y_accel[0] = ctx->imu.linear_acceleration.x;
        y_accel[1] = ctx->imu.linear_acceleration.y;
        y_accel[2] = ctx->imu.linear_acceleration.z;
        LOG_DBG("accel: %10.4f %10.4f %10.4f", y_accel[0], y_accel[1], y_accel[2]);

        y_gyro[0] = ctx->imu.angular_velocity.x;
        y_gyro[1] = ctx->imu.angular_velocity.y;
        y_gyro[2] = ctx->imu.angular_velocity.z;
        LOG_DBG("gyro: %10.4f %10.4f %10.4f", y_gyro[0], y_gyro[1], y_gyro[2]);

        // strapdown_ins_propagate:(x0[9],a_b[3],omega_b[3],g,dt)->(x1[9])
        {
            LOG_DBG("predict: dt: %10.4f gyro z: %10.4f", dt, y_gyro[2]);

            // memory
            static casadi_int iw[strapdown_ins_propagate_SZ_IW];
            static casadi_real w[strapdown_ins_propagate_SZ_W];

            // input
            const double* args[] = { g_ctx.x, y_accel, y_gyro, &g, &dt };

            // output
            double x1[9];
            double* res[] = { x1 };

            // evaluate
            strapdown_ins_propagate(args, res, iw, w, 0);

            // lock position
            x1[0] = 0;
            x1[1] = 0;
            x1[2] = 0;

            // lock velocity
            x1[3] = 0;
            x1[4] = 0;
            x1[5] = 0;

            // update x
            handle_update(x1);
        }

        // publish xyz
        {
            // position
            ctx->odometry.pose.pose.position.x = g_ctx.x[0];
            ctx->odometry.pose.pose.position.y = g_ctx.x[1];
            ctx->odometry.pose.pose.position.z = g_ctx.x[2];

            // TODO rotate linear vel to body frame
            ctx->odometry.twist.twist.linear.x = g_ctx.x[3];
            ctx->odometry.twist.twist.linear.y = g_ctx.x[4];
            ctx->odometry.twist.twist.linear.z = g_ctx.x[5];

            ctx->odometry.twist.twist.angular.x = y_gyro[0];
            ctx->odometry.twist.twist.angular.y = y_gyro[1];
            ctx->odometry.twist.twist.angular.z = y_gyro[2];

            // quat_from_mrp:(r[3])->(q[4])
            {
                // memory
                static casadi_int iw[quat_from_mrp_SZ_IW];
                static casadi_real w[quat_from_mrp_SZ_W];

                // input
                const double* args[] = { &g_ctx.x[6] };

                // output
                double q[4];
                double* res[] = { q };

                // evaluate
                quat_from_mrp(args, res, iw, w, 0);
                ctx->odometry.pose.pose.orientation.w = q[0];
                ctx->odometry.pose.pose.orientation.x = q[1];
                ctx->odometry.pose.pose.orientation.y = q[2];
                ctx->odometry.pose.pose.orientation.z = q[3];
            }
        }

        stamp_header(&ctx->odometry.header, ticks_now);

        log_x(g_ctx.x);

        syn_node_publish_all(&ctx->node, K_MSEC(1));
        syn_node_unlock_all(&ctx->node);
    }
}

K_THREAD_DEFINE(estimate_ins, MY_STACK_SIZE,
    estimate_ins_entry_point, &g_ctx, NULL, NULL,
    MY_PRIORITY, 0, 0);

/* vi: ts=4 sw=4 et */
