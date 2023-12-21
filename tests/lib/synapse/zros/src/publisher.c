#include <stdio.h>
#include <zephyr/kernel.h>

// for this example, we will use a self-contained topic list,
// typically you would use a pre-defined topic in
// #include <synapse_topic_list.h>
#include "topic_list.h"

#include <zros/zros_node.h>
#include <zros/zros_pub.h>

// private headers are necessary when you must
// allocate a zros data structure of the given type
#include <zros/private/zros_node_struct.h>
#include <zros/private/zros_pub_struct.h>

// to enable ZROS logging
#include <zephyr/logging/log.h>

#define MY_STACK_SIZE 2048
#define MY_PRIORITY 5

LOG_MODULE_REGISTER(publisher, LOG_LEVEL_DBG);

static struct zros_node node;
static struct zros_pub pub;

// declare a local msg that is copied to the topic during publish
static my_struct_t my_data = {
    .buf = "hello",
};

// declare the pub entry point thread
void pub_entry_point(void* p0, void* p1, void* p2)
{
    ARG_UNUSED(p0);
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);

    // initialize a node
    zros_node_init(&node, "my_node");
    // initialize a publisher on the node
    int rc = zros_pub_init(&pub, &node, &topic_example, &my_data);
    if (rc < 0) {
        LOG_ERR("pub init failed");
    }

    // loop and publish
    for (int i = 0; i < 10; i++) {
        // wait one second
        k_msleep(100);
        // increment sequence
        snprintf(my_data.buf, sizeof(my_data.buf), "hello world %d", i);
        // publish data tied to publisher to the topic data
        // this is thread safe
        zros_pub_update(&pub);
        LOG_INF("%s", my_data.buf);
    }
};

// vi: ts=4 sw=4 et
