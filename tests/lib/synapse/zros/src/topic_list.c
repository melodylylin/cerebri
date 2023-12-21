#include "topic_list.h"

#include <zros/private/zros_topic_struct.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(topic_list, LOG_LEVEL_DBG);

// This example will define a new topic in this file. This is not
// the standard procedure, as typcially all topics are declared in
// lib/synapse/topic/include/synapse_topic_list.c. We do it here to
// make the example self-contained ane make the overall API clear.

ZROS_TOPIC_DEFINE(example, my_struct_t);

// This function is called at boot before any applications are run, it declare that the
// new topic exists, by adding it to the broker, this is useful for zros topic list.
// This is called for all topics in one file (see above).
static int set_topic_list()
{
    LOG_INF("adding topic to broker list");
    return zros_broker_add_topic(&topic_example);
}

// This registeres the set_topic_list to be called at boot, and is also done for standard
// Cerebri apps in lib/synapse/topic/src/synapse_topic_list.c
SYS_INIT(set_topic_list, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

// vi: ts=4 sw=4 et
