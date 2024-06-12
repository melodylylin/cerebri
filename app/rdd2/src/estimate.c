/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <time.h>

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

#include "casadi/gen/rdd2.h"

#define MY_STACK_SIZE 4096
#define MY_PRIORITY 4

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LOG_MODULE_REGISTER(rdd2_estimate, CONFIG_CEREBRI_RDD2_LOG_LEVEL);

static K_THREAD_STACK_DEFINE(g_my_stack_area, MY_STACK_SIZE);

// private context
struct context {
    struct zros_node node;
    synapse_msgs_Odometry offboard_odometry;
    synapse_msgs_Imu imu;
    synapse_msgs_Odometry odometry;
    struct zros_sub sub_offboard_odometry, sub_imu;
    struct zros_pub pub_odometry;
    double x[3];
    struct k_sem running;
    size_t stack_size;
    k_thread_stack_t* stack_area;
    struct k_thread thread_data;
};

// private initialization
static struct context g_ctx = {
    .node = {},
    .offboard_odometry = synapse_msgs_Odometry_init_default,
    .imu = synapse_msgs_Imu_init_default,
    .odometry = {
        .child_frame_id = "base_link",
        .has_header = true,
        .header.frame_id = "odom",
        .has_pose = true,
        .has_twist = true,
        .pose.has_pose = true,
        .pose.pose.has_position = true,
        .pose.pose.has_orientation = true,
        .twist.has_twist = true,
        .twist.twist.has_angular = true,
        .twist.twist.has_linear = true,
    },
    .sub_offboard_odometry = {},
    .sub_imu = {},
    .pub_odometry = {},
    .x = {},
    .running = Z_SEM_INITIALIZER(g_ctx.running, 1, 1),
    .stack_size = MY_STACK_SIZE,
    .stack_area = g_my_stack_area,
    .thread_data = {},
};

static void rdd2_estimate_init(struct context* ctx)
{
    LOG_INF("init");
    zros_node_init(&ctx->node, "rdd2_estimate");
    zros_sub_init(&ctx->sub_imu, &ctx->node, &topic_imu, &ctx->imu, 300);
    zros_sub_init(&ctx->sub_offboard_odometry, &ctx->node, &topic_offboard_odometry,
        &ctx->offboard_odometry, 10);
    zros_pub_init(&ctx->pub_odometry, &ctx->node, &topic_estimator_odometry, &ctx->odometry);
    k_sem_take(&ctx->running, K_FOREVER);
}

static void rdd2_estimate_fini(struct context* ctx)
{
    LOG_INF("fini");
    zros_sub_fini(&ctx->sub_imu);
    zros_sub_fini(&ctx->sub_offboard_odometry);
    zros_pub_fini(&ctx->pub_odometry);
    zros_node_fini(&ctx->node);
    k_sem_give(&ctx->running);
}

static void rdd2_estimate_run(void* p0, void* p1, void* p2)
{
    struct context* ctx = p0;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);

    int rc = 0;

    // LOG_DBG("started");
    rdd2_estimate_init(ctx);

    // variables
    int32_t seq = 0;

    struct k_poll_event events[1] = {};

    // wait for imu
    LOG_DBG("waiting for imu");
    events[0] = *zros_sub_get_event(&ctx->sub_imu);
    rc = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
    if (rc != 0) {
        LOG_DBG("did not receive imu");
        return;
    }
    if (zros_sub_update_available(&ctx->sub_imu)) {
        zros_sub_update(&ctx->sub_imu);
    }

    double dt = 0;
    int64_t ticks_last = k_uptime_ticks();

    // estimator states
    double x[10] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };

    // poll on imu
    events[0] = *zros_sub_get_event(&ctx->sub_imu);

    // int j = 0;

    while (k_sem_take(&ctx->running, K_NO_WAIT) < 0) {

        // j += 1;

        // poll for imu
        rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(1000));
        if (rc != 0) {
            LOG_DBG("not receiving imu");
            continue;
        }

        if (zros_sub_update_available(&ctx->sub_imu)) {
            zros_sub_update(&ctx->sub_imu);
        }

        /*
        if (j % 100 == 0) {
            int offset = 0;
            static char buf[1024];
            int n = 1024;
            offset += snprintf(buf + offset, n - offset, "x: ");
            for (int i=0; i<10;i++) {
                offset += snprintf(buf + offset, n - offset, " %6.2f", x[i]);
            }
            LOG_INF("%s", buf);
        }
        */

        if (zros_sub_update_available(&ctx->sub_offboard_odometry)) {
            // LOG_INF("correct offboard odometry");
            zros_sub_update(&ctx->sub_offboard_odometry);

#if defined(CONFIG_CEREBRI_RDD2_ESTIMATE_OFFBOARD_ODOMETRY)
            __ASSERT(fabs((ctx->offboard_odometry.pose.pose.orientation.w * ctx->offboard_odometry.pose.pose.orientation.w
                              + ctx->offboard_odometry.pose.pose.orientation.x * ctx->offboard_odometry.pose.pose.orientation.x
                              + ctx->offboard_odometry.pose.pose.orientation.y * ctx->offboard_odometry.pose.pose.orientation.y
                              + ctx->offboard_odometry.pose.pose.orientation.z * ctx->offboard_odometry.pose.pose.orientation.z)
                         - 1)
                    < 1e-2,
                "quaternion normal error");

            // use offboard odometry to reset position
            x[0] = ctx->offboard_odometry.pose.pose.position.x;
            x[1] = ctx->offboard_odometry.pose.pose.position.y;
            x[2] = ctx->offboard_odometry.pose.pose.position.z;

            // use offboard odometry to reset velocity
            x[3] = ctx->offboard_odometry.twist.twist.linear.x;
            x[4] = ctx->offboard_odometry.twist.twist.linear.y;
            x[5] = ctx->offboard_odometry.twist.twist.linear.z;

            // use offboard odometry to reset orientation
            x[6] = ctx->offboard_odometry.pose.pose.orientation.w;
            x[7] = ctx->offboard_odometry.pose.pose.orientation.x;
            x[8] = ctx->offboard_odometry.pose.pose.orientation.y;
            x[9] = ctx->offboard_odometry.pose.pose.orientation.z;
#endif
        }

        // calculate dt
        int64_t ticks_now = k_uptime_ticks();
        dt = (double)(ticks_now - ticks_last) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
        ticks_last = ticks_now;
        if (dt <= 0 || dt > 0.5) {
            LOG_WRN("imu update rate too low");
            continue;
        }

        {
            CASADI_FUNC_ARGS(strapdown_ins_propagate)
            /* strapdown_ins_propagate:(x0[10],a_b[3],omega_b[3],g,dt)->(x1[10]) */
            const double g = 9.8;
            double a_b[3] = {
                ctx->imu.linear_acceleration.x,
                ctx->imu.linear_acceleration.y,
                ctx->imu.linear_acceleration.z
            };
            double omega_b[3] = {
                ctx->imu.angular_velocity.x,
                ctx->imu.angular_velocity.y,
                ctx->imu.angular_velocity.z
            };
            args[0] = x;
            args[1] = a_b;
            args[2] = omega_b;
            args[3] = &g;
            args[4] = &dt;
            res[0] = x;
            CASADI_FUNC_CALL(strapdown_ins_propagate)
        }

        bool data_ok = true;
        for (int i = 0; i < 10; i++) {
            if (!isfinite(x[i])) {
                LOG_ERR("x[%d] is not finite", i);
                // TODO reinitialize
                x[i] = 0;
                data_ok = false;
                break;
            }
        }

        // publish odometry
        if (data_ok) {
            stamp_header(&ctx->odometry.header, k_uptime_ticks());
            ctx->odometry.header.seq = seq++;
            ctx->odometry.pose.pose.position.x = x[0];
            ctx->odometry.pose.pose.position.y = x[1];
            ctx->odometry.pose.pose.position.z = x[2];
            ctx->odometry.twist.twist.linear.x = x[3];
            ctx->odometry.twist.twist.linear.y = x[4];
            ctx->odometry.twist.twist.linear.z = x[5];
            ctx->odometry.pose.pose.orientation.w = x[6];
            ctx->odometry.pose.pose.orientation.x = x[7];
            ctx->odometry.pose.pose.orientation.y = x[8];
            ctx->odometry.pose.pose.orientation.z = x[9];
            ctx->odometry.twist.twist.angular.x = ctx->imu.angular_velocity.x;
            ctx->odometry.twist.twist.angular.y = ctx->imu.angular_velocity.y;
            ctx->odometry.twist.twist.angular.z = ctx->imu.angular_velocity.z;

            // check quaternion normal
            __ASSERT(fabs((ctx->odometry.pose.pose.orientation.w * ctx->odometry.pose.pose.orientation.w
                              + ctx->odometry.pose.pose.orientation.x * ctx->odometry.pose.pose.orientation.x
                              + ctx->odometry.pose.pose.orientation.y * ctx->odometry.pose.pose.orientation.y
                              + ctx->odometry.pose.pose.orientation.z * ctx->odometry.pose.pose.orientation.z)
                         - 1)
                    < 1e-2,
                "quaternion normal error");
            zros_pub_update(&ctx->pub_odometry);
        }
    }

    rdd2_estimate_fini(ctx);
}

static int start(struct context* ctx)
{
    k_tid_t tid = k_thread_create(&ctx->thread_data, ctx->stack_area,
        ctx->stack_size,
        rdd2_estimate_run,
        ctx, NULL, NULL,
        MY_PRIORITY, 0, K_FOREVER);
    k_thread_name_set(tid, "rdd2_estimate");
    k_thread_start(tid);
    return 0;
}

static int rdd2_estimate_cmd_handler(const struct shell* sh,
    size_t argc, char** argv, void* data)
{
    ARG_UNUSED(argc);
    struct context* ctx = data;

    if (strcmp(argv[0], "start") == 0) {
        if(k_sem_count_get(&g_ctx.running) == 0) {
            shell_print(sh, "already running");
        } else {
            start(ctx);
        }
    } else if (strcmp(argv[0], "stop") == 0) {
        if(k_sem_count_get(&g_ctx.running) == 0) {
            k_sem_give(&g_ctx.running);
        } else {
            shell_print(sh, "not running");
        }
    } else if (strcmp(argv[0], "status") == 0) {
        shell_print(sh, "running: %d", (int)k_sem_count_get(&g_ctx.running) == 0);
    }
    return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_rdd2_estimate, rdd2_estimate_cmd_handler,
    (start, &g_ctx, "start"),
    (stop, &g_ctx, "stop"),
    (status, &g_ctx, "status"));

SHELL_CMD_REGISTER(rdd2_estimate, &sub_rdd2_estimate, "rdd2 estimate commands", NULL);

static int rdd2_estimate_sys_init(void)
{
    return start(&g_ctx);
};

SYS_INIT(rdd2_estimate_sys_init, APPLICATION, 1);

// vi: ts=4 sw=4 et
