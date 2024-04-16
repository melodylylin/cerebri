/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <math.h>
#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_pub_struct.h>
#include <zros/private/zros_sub_struct.h>
#include <zros/zros_node.h>
#include <zros/zros_pub.h>
#include <zros/zros_sub.h>

#include <synapse_topic_list.h>

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "casadi/gen/rdd2.h"

#include <cerebri/core/casadi.h>

#define MY_STACK_SIZE 3072
#define MY_PRIORITY 4

LOG_MODULE_REGISTER(rdd2_position, CONFIG_CEREBRI_RDD2_LOG_LEVEL);

static K_THREAD_STACK_DEFINE(g_my_stack_area, MY_STACK_SIZE);

struct context {
    struct zros_node node;
    synapse_msgs_Status status;
    synapse_msgs_BezierTrajectory bezier_trajectory;
    synapse_msgs_Time clock_offset;
    synapse_msgs_Odometry pose, estimator_odometry, external_odometry;
    synapse_msgs_Twist cmd_vel;
    synapse_msgs_Vector3 force_sp, velocity_sp, attitude_sp, orientation_sp;
    struct zros_sub sub_status, sub_clock_offset, sub_pose, sub_bezier_trajectory, sub_velocity_sp, sub_estimator_odometry, sub_external_odometry, sub_orientation_sp;
    struct zros_pub pub_cmd_vel, pub_force_sp, pub_attitude_sp;
    atomic_t running;
    size_t stack_size;
    k_thread_stack_t* stack_area;
    struct k_thread thread_data;
};

static struct context g_ctx = {
    .status = synapse_msgs_Status_init_default,
    .bezier_trajectory = synapse_msgs_BezierTrajectory_init_default,
    .clock_offset = synapse_msgs_Time_init_default,
    .pose = synapse_msgs_Odometry_init_default,
    .cmd_vel = {
        .has_angular = true,
        .has_linear = true,
        .linear = synapse_msgs_Vector3_init_default,
        .angular = synapse_msgs_Vector3_init_default,
    },
    .force_sp = synapse_msgs_Vector3_init_default,
    .estimator_odometry = synapse_msgs_Odometry_init_default,
    .external_odometry = synapse_msgs_Odometry_init_default,
    .attitude_sp = synapse_msgs_Vector3_init_default,
    .orientation_sp = synapse_msgs_Vector3_init_default,
    .sub_status = {},
    .sub_clock_offset = {},
    .sub_pose = {},
    .sub_bezier_trajectory = {},
    .sub_velocity_sp = {},
    .sub_estimator_odometry = {},
    .sub_external_odometry = {},
    .sub_orientation_sp = {},
    .pub_cmd_vel = {},
    .pub_force_sp = {},
    .pub_attitude_sp = {},
    .running = ATOMIC_INIT(0),
    .stack_size = MY_STACK_SIZE,
    .stack_area = g_my_stack_area,
    .thread_data = {},
};

static void rdd2_position_init(struct context* ctx)
{
    LOG_INF("init");
    zros_node_init(&ctx->node, "rdd2_position");
    zros_sub_init(&ctx->sub_status, &ctx->node, &topic_status, &ctx->status, 10);
    zros_sub_init(&ctx->sub_clock_offset, &ctx->node, &topic_clock_offset, &ctx->clock_offset, 10);
    zros_sub_init(&ctx->sub_pose, &ctx->node, &topic_estimator_odometry, &ctx->pose, 100);
    zros_sub_init(&ctx->sub_bezier_trajectory, &ctx->node, &topic_bezier_trajectory, &ctx->bezier_trajectory, 10);
    zros_sub_init(&ctx->sub_velocity_sp, &ctx->node, &topic_velocity_sp, &ctx->velocity_sp, 100);
    zros_sub_init(&ctx->sub_estimator_odometry, &ctx->node, &topic_estimator_odometry, &ctx->estimator_odometry, 10);
    zros_sub_init(&ctx->sub_external_odometry, &ctx->node, &topic_external_odometry, &ctx->external_odometry, 10);
    zros_sub_init(&ctx->sub_orientation_sp, &ctx->node, &topic_orientation_sp, &ctx->orientation_sp, 100);
    zros_pub_init(&ctx->pub_cmd_vel, &ctx->node, &topic_cmd_vel, &ctx->cmd_vel);
    zros_pub_init(&ctx->pub_force_sp, &ctx->node, &topic_force_sp, &ctx->force_sp);
    zros_pub_init(&ctx->pub_attitude_sp, &ctx->node, &topic_attitude_sp, &ctx->attitude_sp);
    atomic_set(&ctx->running, 1);
}

static void rdd2_position_fini(struct context* ctx)
{
    LOG_INF("fini");
    zros_node_fini(&ctx->node);
    zros_sub_fini(&ctx->sub_status);
    zros_sub_fini(&ctx->sub_clock_offset);
    zros_sub_fini(&ctx->sub_pose);
    zros_sub_fini(&ctx->sub_bezier_trajectory);
    zros_sub_fini(&ctx->sub_velocity_sp);
    zros_sub_fini(&ctx->sub_estimator_odometry);
    zros_sub_fini(&ctx->sub_external_odometry);
    zros_sub_fini(&ctx->sub_orientation_sp);
    zros_pub_fini(&ctx->pub_cmd_vel);
    atomic_set(&ctx->running, 0);
    zros_pub_fini(&ctx->pub_force_sp);
    zros_pub_fini(&ctx->pub_attitude_sp);
}

static void rdd2_position_run(void* p0, void* p1, void* p2)
{
    struct context* ctx = p0;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);

    rdd2_position_init(ctx);

    struct k_poll_event events[] = {
        *zros_sub_get_event(&ctx->sub_pose),
    };

    while (atomic_get(&ctx->running)) {
        int rc = 0;
        rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(1000));
        if (rc != 0) {
            LOG_DBG("pos not receiving  pose");
            continue;
        }

        if (zros_sub_update_available(&ctx->sub_bezier_trajectory)) {
            zros_sub_update(&ctx->sub_bezier_trajectory);
        }

        if (zros_sub_update_available(&ctx->sub_status)) {
            zros_sub_update(&ctx->sub_status);
        }

        if (zros_sub_update_available(&ctx->sub_pose)) {
            zros_sub_update(&ctx->sub_pose);
        }

        if (zros_sub_update_available(&ctx->sub_clock_offset)) {
            zros_sub_update(&ctx->sub_clock_offset);
        }

        if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_AUTO) {
            zros_pub_update(&ctx->pub_cmd_vel);
        }

        if (zros_sub_update_available(&ctx->sub_velocity_sp)) {
            zros_sub_update(&ctx->sub_velocity_sp);
        }

        if (zros_sub_update_available(&ctx->sub_orientation_sp)) {
            zros_sub_update(&ctx->sub_orientation_sp);
        }

        if (zros_sub_update_available(&ctx->sub_estimator_odometry)) {
            zros_sub_update(&ctx->sub_estimator_odometry);
        }

        if (zros_sub_update_available(&ctx->sub_external_odometry)) {
            zros_sub_update(&ctx->sub_external_odometry);
        }

        if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_AUTO) {
            /* position_control:(pt[3],vt[3],yt,Kp,Kv,pos[3],vel[3])->(nT,q[3]) */
            CASADI_FUNC_ARGS(position_control)
            double pt[3];
            double vt[3];
            double yt = ctx->orientation_sp.x; // desired yaw
            double Kp = 0.1;
            double Kv = 0.1;
            double pos[3];
            double vel[3];
            double q[4];

            double thrust;
            double euler[3];

            // set position as current position for now, kills pos feedback
            pt[0] = 0;
            pt[1] = 0;
            pt[2] = 2;
            vt[0] = 0;
            vt[1] = 0;
            vt[2] = 0;
            q[0] = ctx->estimator_odometry.pose.pose.orientation.w;
            q[1] = ctx->estimator_odometry.pose.pose.orientation.x;
            q[2] = ctx->estimator_odometry.pose.pose.orientation.y;
            q[3] = ctx->estimator_odometry.pose.pose.orientation.z;
            pos[0] = ctx->estimator_odometry.pose.pose.position.x;
            pos[1] = ctx->estimator_odometry.pose.pose.position.y;
            pos[2] = ctx->estimator_odometry.pose.pose.position.z;
            vel[0] = ctx->external_odometry.twist.twist.linear.y;
            vel[1] = ctx->external_odometry.twist.twist.linear.x;
            vel[2] = ctx->external_odometry.twist.twist.linear.z;

            args[0] = pt;
            args[1] = vt;
            args[2] = &yt;
            args[3] = &Kp;
            args[4] = &Kv;
            args[5] = pos;
            args[6] = vel;
            args[7] = q;
            res[0] = &thrust;
            res[1] = euler;

            CASADI_FUNC_CALL(position_control)

            ctx->attitude_sp.x = euler[0];
            ctx->attitude_sp.y = euler[1];
            ctx->attitude_sp.z = euler[2];
            zros_pub_update(&ctx->pub_attitude_sp);

            ctx->force_sp.z = thrust/(2*9.8);
            zros_pub_update(&ctx->pub_force_sp);
        }
    }
    rdd2_position_fini(ctx);
}

static int start(struct context* ctx)
{
    k_tid_t tid = k_thread_create(&ctx->thread_data, ctx->stack_area,
        ctx->stack_size,
        rdd2_position_run,
        ctx, NULL, NULL,
        MY_PRIORITY, 0, K_FOREVER);
    k_thread_name_set(tid, "rdd2_position");
    k_thread_start(tid);
    return 0;
}

static int rdd2_position_cmd_handler(const struct shell* sh,
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

SHELL_SUBCMD_DICT_SET_CREATE(sub_rdd2_position, rdd2_position_cmd_handler,
    (start, &g_ctx, "start"),
    (stop, &g_ctx, "stop"),
    (status, &g_ctx, "status"));

SHELL_CMD_REGISTER(rdd2_position, &sub_rdd2_position, "rdd2 position commands", NULL);

static int rdd2_position_sys_init(void)
{
    return start(&g_ctx);
};

SYS_INIT(rdd2_position_sys_init, APPLICATION, 4);

// vi: ts=4 sw=4 et
