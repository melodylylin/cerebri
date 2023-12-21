#include <zephyr/kernel.h>

// for this example, we will use a self-contained topic list,
// typically you would use a pre-defined topic in
// #include <synapse_topic_list.h>
#include "topic_list.h"
#include <zephyr/ztest.h>

#define MY_STACK_SIZE 2048
#define MY_PRIORITY 5

#include <zros/zros_node.h>
#include <zros/zros_sub.h>

// private headers are necessary when you must
// allocate a zros data structure of the given type
#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_sub_struct.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(subscriber, LOG_LEVEL_DBG);

static struct zros_node node;
static struct zros_sub sub;

// declare a local msg that is copied to during sub update
static my_struct_t my_data = {};

void sub_entry_point(void* p0, void* p1, void* p2)
{
    zros_node_init(&node, "sub");
    int rc = 0;
    rc = zros_sub_init(&sub, &node, &topic_example, &my_data, 10);
    zassert_true((rc == 0), "failed initializaing sub");

    struct k_poll_event events[] = {
        *zros_sub_get_event(&sub),
    };

    for (int i = 0; i < 10; i++) {
        // wait for publication signal
        int rc = 0;
        rc = k_poll(events, ARRAY_SIZE(events), K_MSEC(10000));
        zassert_true((rc == 0), "poll timeout");
        if (rc != 0) {
            LOG_WRN("poll timeout while waiting for publication");
        }

        if (zros_sub_update_available(&sub)) {
            zros_sub_update(&sub);
            LOG_INF("%s", my_data.buf);
        }
    }
};

// vi: ts=4 sw=4 et
