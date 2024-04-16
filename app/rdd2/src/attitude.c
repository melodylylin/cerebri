/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include "casadi/gen/rdd2.h"

#include <assert.h>

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_pub_struct.h>
#include <zros/private/zros_sub_struct.h>
#include <zros/zros_node.h>
#include <zros/zros_pub.h>
#include <zros/zros_sub.h>

#include <cerebri/core/casadi.h>

#include <synapse_topic_list.h>

#define MY_STACK_SIZE 3072
#define MY_PRIORITY 4

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LOG_MODULE_REGISTER(rdd2_attitude, CONFIG_CEREBRI_RDD2_LOG_LEVEL);

static K_THREAD_STACK_DEFINE(g_my_stack_area, MY_STACK_SIZE);

struct context {
    struct zros_node node;
    synapse_msgs_Status status;
    synapse_msgs_Odometry estimator_odometry;
    synapse_msgs_Vector3 attitude_sp, angular_velocity_sp;
    struct zros_sub sub_status, sub_attitude_sp, sub_estimator_odometry;
    struct zros_pub pub_angular_velocity_sp;
    atomic_t running;
    size_t stack_size;
    k_thread_stack_t* stack_area;
    struct k_thread thread_data;
};

static struct context g_ctx = {
    .node = {},
    .status = synapse_msgs_Status_init_default,
    .attitude_sp = synapse_msgs_Vector3_init_default,
    .angular_velocity_sp = synapse_msgs_Vector3_init_default,
    .estimator_odometry = synapse_msgs_Odometry_init_default,
    .sub_status = {},
    .sub_attitude_sp = {},
    .sub_estimator_odometry = {},
    .pub_angular_velocity_sp = {},
    .running = ATOMIC_INIT(0),
    .stack_size = MY_STACK_SIZE,
    .stack_area = g_my_stack_area,
    .thread_data = {},
};

static void rdd2_attitude_init(struct context* ctx)
{
    LOG_INF("init");
    zros_node_init(&ctx->node, "rdd2_attiude");
    zros_sub_init(&ctx->sub_status, &ctx->node, &topic_status, &ctx->status, 10);
    zros_sub_init(&ctx->sub_attitude_sp, &ctx->node,
        &topic_attitude_sp, &ctx->attitude_sp, 100);
    zros_sub_init(&ctx->sub_estimator_odometry, &ctx->node,
        &topic_estimator_odometry, &ctx->estimator_odometry, 100);
    zros_pub_init(&ctx->pub_angular_velocity_sp, &ctx->node, &topic_angular_velocity_sp, &ctx->angular_velocity_sp);
    atomic_set(&ctx->running, 1);
}

static void rdd2_attitude_fini(struct context* ctx)
{
    LOG_INF("fini");
    zros_node_fini(&ctx->node);
    zros_sub_fini(&ctx->sub_status);
    zros_sub_fini(&ctx->sub_attitude_sp);
    zros_sub_fini(&ctx->sub_estimator_odometry);
    zros_pub_fini(&ctx->pub_angular_velocity_sp);
    atomic_set(&ctx->running, 0);
}

static void rdd2_attitude_run(void* p0, void* p1, void* p2)
{
    struct context* ctx = p0;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);

    rdd2_attitude_init(ctx);

    struct k_poll_event events[] = {
        *zros_sub_get_event(&ctx->sub_estimator_odometry),
    };

    while (atomic_get(&ctx->running)) {
        int rc = 0;
        rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(1000));
        if (rc != 0) {
            LOG_DBG("not receiving estimator_odometry");
        }

        if (zros_sub_update_available(&ctx->sub_status)) {
            zros_sub_update(&ctx->sub_status);
        }

        if (zros_sub_update_available(&ctx->sub_estimator_odometry)) {
            zros_sub_update(&ctx->sub_estimator_odometry);
        }

        if (zros_sub_update_available(&ctx->sub_attitude_sp)) {
            zros_sub_update(&ctx->sub_attitude_sp);
        }

        if (ctx->status.mode != synapse_msgs_Status_Mode_MODE_MANUAL) {
            /* attitude_error:(q[4],yaw_r,pitch_r,roll_r)->(omega[3]) */
            CASADI_FUNC_ARGS(attitude_error);
            double q[4];
            double angle_error[3];

            double yaw_sp = ctx->attitude_sp.x; // yaw
            double pitch_sp = ctx->attitude_sp.y; // pitch
            double roll_sp = ctx->attitude_sp.z; // roll

            q[0] = ctx->estimator_odometry.pose.pose.orientation.w;
            q[1] = ctx->estimator_odometry.pose.pose.orientation.x;
            q[2] = ctx->estimator_odometry.pose.pose.orientation.y;
            q[3] = ctx->estimator_odometry.pose.pose.orientation.z;
            args[0] = q;
            args[1] = &yaw_sp;
            args[2] = &pitch_sp;
            args[3] = &roll_sp;
            res[0] = angle_error;
            CASADI_FUNC_CALL(attitude_error);

            LOG_DBG("q: %10.4f %10.4f %10.4f %10.4f", q[0], q[1], q[2], q[3]);
            LOG_DBG("yaw: %10.4f pitch: %10.4f roll: %10.4f", yaw_sp, pitch_sp, roll_sp);
            LOG_DBG("angle_error: %10.4f %10.4f %10.4f", angle_error[0], angle_error[1], angle_error[2]);

            // set rate setpoints

            ctx->angular_velocity_sp.x = 2 * angle_error[0];
            ctx->angular_velocity_sp.y = 2 * angle_error[1];
            ctx->angular_velocity_sp.z = 2 * angle_error[2];

            // publish
            zros_pub_update(&ctx->pub_angular_velocity_sp);
        }
    }

    rdd2_attitude_fini(ctx);
}

static int start(struct context* ctx)
{
    k_tid_t tid = k_thread_create(&ctx->thread_data, ctx->stack_area,
        ctx->stack_size,
        rdd2_attitude_run,
        ctx, NULL, NULL,
        MY_PRIORITY, 0, K_FOREVER);
    k_thread_name_set(tid, "rdd2_attitude");
    k_thread_start(tid);
    return 0;
}

static int rdd2_attitude_cmd_handler(const struct shell* sh,
    size_t argc, char** argv, void* data)
{
    struct context* ctx = data;
    assert(argc == 1);

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

SHELL_SUBCMD_DICT_SET_CREATE(sub_rdd2_attitude, rdd2_attitude_cmd_handler,
    (start, &g_ctx, "start"),
    (stop, &g_ctx, "stop"),
    (status, &g_ctx, "status"));

SHELL_CMD_REGISTER(rdd2_attitude, &sub_rdd2_attitude, "rdd2 attitude commands", NULL);

static int rdd2_attitude_sys_init(void)
{
    return start(&g_ctx);
};

SYS_INIT(rdd2_attitude_sys_init, APPLICATION, 3);

// vi: ts=4 sw=4 et
