/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include "casadi/gen/b3rb.h"
#include <math.h>

#include <zephyr/logging/log.h>

#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_pub_struct.h>
#include <zros/private/zros_sub_struct.h>
#include <zros/zros_node.h>
#include <zros/zros_pub.h>
#include <zros/zros_sub.h>

#include "mixing.h"

#define MY_STACK_SIZE 3072
#define MY_PRIORITY 4

LOG_MODULE_REGISTER(b3rb_velocity, CONFIG_CEREBRI_B3RB_LOG_LEVEL);

typedef struct _context {
    struct zros_node node;
    synapse_msgs_Twist cmd_vel;
    synapse_msgs_Fsm fsm;
    synapse_msgs_Actuators actuators;
    synapse_msgs_Actuators actuators_manual;
    struct zros_sub sub_fsm, sub_cmd_vel, sub_actuators_manual;
    struct zros_pub pub_actuators;
    const double wheel_radius;
    const double wheel_base;
} context;

static context g_ctx = {
    .node = {},
    .cmd_vel = synapse_msgs_Twist_init_default,
    .fsm = synapse_msgs_Fsm_init_default,
    .actuators = synapse_msgs_Actuators_init_default,
    .actuators_manual = synapse_msgs_Actuators_init_default,
    .sub_fsm = {},
    .sub_cmd_vel = {},
    .sub_actuators_manual = {},
    .pub_actuators = {},
    .wheel_radius = CONFIG_CEREBRI_B3RB_WHEEL_RADIUS_MM / 1000.0,
    .wheel_base = CONFIG_CEREBRI_B3RB_WHEEL_BASE_MM / 1000.0,
};

static void init_b3rb_vel(context* ctx)
{
    LOG_DBG("init vel");
    zros_node_init(&ctx->node, "b3rb_velocity");
    zros_sub_init(&ctx->sub_cmd_vel, &ctx->node, &topic_cmd_vel, &ctx->cmd_vel, 10);
    zros_sub_init(&ctx->sub_fsm, &ctx->node, &topic_fsm, &ctx->fsm, 10);
    zros_sub_init(&ctx->sub_actuators_manual, &ctx->node,
        &topic_actuators_manual, &ctx->actuators_manual, 10);
    zros_pub_init(&ctx->pub_actuators, &ctx->node, &topic_actuators, &ctx->actuators);
}

// computes rc_input from V, omega
void update_cmd_vel(context* ctx)
{
    double turn_angle = 0;
    double omega_fwd = 0;
    double V = ctx->cmd_vel.linear.x;
    double omega = ctx->cmd_vel.angular.z;
    casadi_int* iw = NULL;
    casadi_real* w = NULL;
    int mem = 0;
    double delta = 0;
    const casadi_real* args[3];
    casadi_real* res[1];
    args[0] = &ctx->wheel_base;
    args[1] = &omega;
    args[2] = &V;
    res[0] = &delta;
    ackermann_steering(args, res, iw, w, mem);
    omega_fwd = V / ctx->wheel_radius;
    if (fabs(V) > 0.01) {
        turn_angle = delta;
    }
    b3rb_set_actuators(&ctx->actuators, turn_angle, omega_fwd);
}

static void stop(context* ctx)
{
    b3rb_set_actuators(&ctx->actuators, 0, 0);
}

static void b3rb_velocity_entry_point(void* p0, void* p1, void* p2)
{
    context* ctx = p0;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);

    init_b3rb_vel(ctx);

    while (true) {
        synapse_msgs_Fsm_Mode mode = ctx->fsm.mode;

        int rc = 0;
        if (mode == synapse_msgs_Fsm_Mode_MANUAL) {
            struct k_poll_event events[] = {
                *zros_sub_get_event(&ctx->sub_actuators_manual),
            };
            rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(1000));
            if (rc != 0) {
                LOG_DBG("not receiving manual actuators");
            }
        } else {
            struct k_poll_event events[] = {
                *zros_sub_get_event(&ctx->sub_cmd_vel),
            };
            rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(1000));
            if (rc != 0) {
                LOG_DBG("not receiving cmd_vel");
            }
        }

        if (zros_sub_update_available(&ctx->sub_fsm)) {
            zros_sub_update(&ctx->sub_fsm);
        }

        if (zros_sub_update_available(&ctx->sub_cmd_vel)) {
            zros_sub_update(&ctx->sub_cmd_vel);
        }

        if (zros_sub_update_available(&ctx->sub_actuators_manual)) {
            zros_sub_update(&ctx->sub_actuators_manual);
        }

        // handle modes
        if (rc < 0) {
            stop(ctx);
            LOG_DBG("no data, stopped");
        } else if (ctx->fsm.armed != synapse_msgs_Fsm_Armed_ARMED) {
            stop(ctx);
            LOG_DBG("not armed, stopped");
        } else if (ctx->fsm.mode == synapse_msgs_Fsm_Mode_MANUAL) {
            LOG_DBG("manual mode");
            ctx->actuators = ctx->actuators_manual;
        } else {
            update_cmd_vel(ctx);
        }

        // publish
        zros_pub_update(&ctx->pub_actuators);
    }
}

K_THREAD_DEFINE(b3rb_velocity, MY_STACK_SIZE,
    b3rb_velocity_entry_point, &g_ctx, NULL, NULL,
    MY_PRIORITY, 0, 0);

/* vi: ts=4 sw=4 et */