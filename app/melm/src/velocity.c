/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include "casadi/gen/melm.h"
#include "math.h"

#include <zephyr/logging/log.h>

#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_pub_struct.h>
#include <zros/private/zros_sub_struct.h>
#include <zros/zros_node.h>
#include <zros/zros_pub.h>
#include <zros/zros_sub.h>

#include <cerebri/core/casadi.h>

#include "mixing.h"

#define MY_STACK_SIZE 3072
#define MY_PRIORITY 4

LOG_MODULE_REGISTER(melm_velocity, CONFIG_CEREBRI_MELM_LOG_LEVEL);

typedef struct _context {
    struct zros_node node;
    synapse_msgs_Twist cmd_vel;
    synapse_msgs_Status status;
    synapse_msgs_Actuators actuators;
    synapse_msgs_Actuators actuators_manual;
    struct zros_sub sub_status, sub_cmd_vel, sub_actuators_manual;
    struct zros_pub pub_actuators;
    const double wheel_radius;
    const double wheel_base;
    const double max_velocity;
    const double wheel_separation;
} context;

static context g_ctx = {
    .node = {},
    .cmd_vel = synapse_msgs_Twist_init_default,
    .status = synapse_msgs_Status_init_default,
    .actuators = synapse_msgs_Actuators_init_default,
    .actuators_manual = synapse_msgs_Actuators_init_default,
    .sub_status = {},
    .sub_cmd_vel = {},
    .sub_actuators_manual = {},
    .pub_actuators = {},
    .wheel_radius = CONFIG_CEREBRI_MELM_WHEEL_RADIUS_MM / 1000.0,
    .wheel_base = CONFIG_CEREBRI_MELM_WHEEL_BASE_MM / 1000.0,
    .max_velocity = CONFIG_CEREBRI_MELM_MAX_VELOCITY_MM_S / 1000.0,
    .wheel_separation = CONFIG_CEREBRI_MELM_WHEEL_SEPARATION_MM / 1000.0,
};

static void init_melm_vel(context* ctx)
{
    LOG_DBG("init vel");
    zros_node_init(&ctx->node, "melm_velocity");
    zros_sub_init(&ctx->sub_cmd_vel, &ctx->node, &topic_cmd_vel, &ctx->cmd_vel, 10);
    zros_sub_init(&ctx->sub_status, &ctx->node, &topic_status, &ctx->status, 10);
    zros_sub_init(&ctx->sub_actuators_manual, &ctx->node,
        &topic_actuators_manual, &ctx->actuators_manual, 10);
    zros_pub_init(&ctx->pub_actuators, &ctx->node, &topic_actuators, &ctx->actuators);
}

// computes rc_input from V, omega
void update_cmd_vel(context* ctx)
{
    double V = ctx->cmd_vel.linear.x;
    double omega = ctx->cmd_vel.angular.z;
    double Vw = 0;
    CASADI_FUNC_ARGS(differential_steering);
    args[0] = &ctx->wheel_base;
    args[1] = &omega;
    args[2] = &ctx->wheel_separation;
    res[0] = &Vw;
    CASADI_FUNC_CALL(differential_steering);

    double omega_fwd = V / ctx->wheel_radius;
    double omega_turn = Vw / ctx->wheel_radius;
    melm_set_actuators(&ctx->actuators, omega_fwd, omega_turn);
}

static void stop(context* ctx)
{
    melm_set_actuators(&ctx->actuators, 0, 0);
}

static void melm_velocity_entry_point(void* p0, void* p1, void* p2)
{
    context* ctx = p0;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);

    init_melm_vel(ctx);

    while (true) {
        synapse_msgs_Status_Mode mode = ctx->status.mode;

        int rc = 0;
        if (mode == synapse_msgs_Status_Mode_MODE_MANUAL) {
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

        if (zros_sub_update_available(&ctx->sub_status)) {
            zros_sub_update(&ctx->sub_status);
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
        } else if (ctx->status.arming != synapse_msgs_Status_Arming_ARMING_ARMED) {
            stop(ctx);
            LOG_DBG("not armed, stopped");
        } else if (ctx->status.mode == synapse_msgs_Status_Mode_MODE_MANUAL) {
            LOG_DBG("manual mode");
            ctx->actuators = ctx->actuators_manual;
        } else {
            update_cmd_vel(ctx);
        }

        // publish
        zros_pub_update(&ctx->pub_actuators);
    }
}

K_THREAD_DEFINE(melm_velocity, MY_STACK_SIZE,
    melm_velocity_entry_point, &g_ctx, NULL, NULL,
    MY_PRIORITY, 0, 0);

/* vi: ts=4 sw=4 et */
