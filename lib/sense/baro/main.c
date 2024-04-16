/*
 * Copyright CogniPilot Foundation 2023
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_pub_struct.h>
#include <zros/private/zros_sub_struct.h>
#include <zros/zros_node.h>
#include <zros/zros_pub.h>
#include <zros/zros_sub.h>

#include <synapse_topic_list.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <math.h>

LOG_MODULE_REGISTER(sense_baro, CONFIG_CEREBRI_SENSE_BARO_LOG_LEVEL);

#define MY_STACK_SIZE 4096
#define MY_PRIORITY 6

void baro_timer_handler(struct k_timer* dummy);
void baro_work_handler(struct k_work* work);

typedef struct context_t {
    // work
    struct k_work work_item;
    struct k_timer timer;
    // node
    struct zros_node node;
    // data
    synapse_msgs_Altimeter altimeter;
    // publications
    struct zros_pub pub;
    // devices
    const struct device* baro_dev[CONFIG_CEREBRI_SENSE_BARO_COUNT];
    // raw readings
    double baro_raw[CONFIG_CEREBRI_SENSE_BARO_COUNT];
} context_t;

static context_t g_ctx = {
    .work_item = Z_WORK_INITIALIZER(baro_work_handler),
    .timer = Z_TIMER_INITIALIZER(g_ctx.timer, baro_timer_handler, NULL),
    .node = {},
    .altimeter = {
        .has_header = true,
        .header = {
            .frame_id = "wgs84",
            .has_stamp = true,
            .seq = 0 },
        .vertical_position = 0,
        .vertical_reference = 0,
        .vertical_velocity = 0,
    },
    .baro_dev = {},
    .baro_raw = {},
};

extern struct k_work_q g_low_priority_work_q;

static const struct device* sensor_check(const struct device* const dev)
{
    if (dev == NULL) {
        /* No such node, or the node does not have status "okay". */
        LOG_ERR("no device found");
        return NULL;
    }

    if (!device_is_ready(dev)) {
        LOG_ERR("device %s is not ready, check the driver initialization logs for errors",
            dev->name);
        return NULL;
    }

    LOG_INF("baro found device %s", dev->name);
    return dev;
}

void baro_work_handler(struct k_work* work_item)
{
    context_t* ctx = CONTAINER_OF(work_item, context_t, work_item);
    double baro_data_array[CONFIG_CEREBRI_SENSE_BARO_COUNT][2] = {};
    for (int i = 0; i < CONFIG_CEREBRI_SENSE_BARO_COUNT; i++) {
        // default all data to zero
        struct sensor_value baro_press = {};
        struct sensor_value baro_temp = {};

        // get accel if device present
        if (ctx->baro_dev[i] != NULL) {
            sensor_sample_fetch(ctx->baro_dev[i]);
            sensor_channel_get(ctx->baro_dev[i], SENSOR_CHAN_PRESS, &baro_press);
            sensor_channel_get(ctx->baro_dev[i], SENSOR_CHAN_AMBIENT_TEMP, &baro_temp);
            LOG_DBG("baro %d: %d.%06d %d.%06d", i,
                baro_press.val1, baro_press.val2,
                baro_temp.val1, baro_temp.val2);
        }

        baro_data_array[i][0] = baro_press.val1 + baro_press.val2 * 1e-6;
        baro_data_array[i][1] = baro_temp.val1 + baro_temp.val2 * 1e-6;
    }

    // select first baro for data for now: TODO implement voting
    // TODO add barmetric formula equation
    double press = baro_data_array[0][0];
    double temp = 15.0; // standard atmosphere temp in C
    const double sea_press = 101.325;
    double alt = ((pow((sea_press / press), 1 / 5.257) - 1.0) * (temp + 273.15)) / 0.0065;
    // LOG_DBG("press %10.4f, temp: %10.4f, alt: %10.4f", press, temp, alt);

    // publish altimeter
    ctx->altimeter.vertical_reference = alt;
    stamp_header(&ctx->altimeter.header, k_uptime_ticks());
    ctx->altimeter.header.seq++;
    ctx->altimeter.vertical_position = alt;
    ctx->altimeter.vertical_velocity = 0;
    ctx->altimeter.vertical_reference = 0;
    zros_pub_update(&ctx->pub);
}

K_WORK_DEFINE(baro_work, baro_work_handler);

void baro_timer_handler(struct k_timer* dummy)
{
    k_work_submit_to_queue(&g_low_priority_work_q, &baro_work);
}

K_TIMER_DEFINE(baro_timer, baro_timer_handler, NULL);

int sense_baro_entry_point(void* p0, void* p1, void* p2)
{
    LOG_INF("init");
    context_t* ctx = p0;
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ctx->baro_dev[0] = sensor_check(DEVICE_DT_GET(DT_ALIAS(baro0)));
#if CONFIG_CEREBRI_SENSE_BARO_COUNT >= 2
    ctx->baro_dev[1] = sensor_check(DEVICE_DT_GET(DT_ALIAS(baro1)));
#elif CONFIG_CEREBRI_SENSE_BARO_COUNT >= 3
    ctx->baro_dev[2] = sensor_check(DEVICE_DT_GET(DT_ALIAS(baro2)));
#elif CONFIG_CEREBRI_SENSE_BARO_COUNT == 4
    ctrx->baro_dev[3] = sensor_check(DEVICE_DT_GET(DT_ALIAS(baro3)));
#endif

    k_timer_start(&baro_timer, K_MSEC(100), K_MSEC(100));
    return 0;
}

K_THREAD_DEFINE(sense_baro, MY_STACK_SIZE,
    sense_baro_entry_point, &g_ctx, NULL, NULL,
    MY_PRIORITY, 0, 100);

// vi: ts=4 sw=4 et
