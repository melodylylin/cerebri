#ifndef ZROS_MINIMAL_TOPIC_LIST_H
#define ZROS_MINIMAL_TOPIC_LIST_H

// This example will declare a new topic in this file. This is not
// the standard procedure, as typcially all topics are declared in
// lib/synapse/topic/include/synapse_topic_list.h. We do it here to
// make the example self-contained ane make the overall API clear.

#include <zros/zros_broker.h>
#include <zros/zros_topic.h>

typedef struct my_struct {
    char buf[30];
} my_struct_t;

ZROS_TOPIC_DECLARE(topic_example, my_struct);

// vi: ts=4 sw=4 et

#endif // ZROS_MINIMAL_TOPIC_LIST_H
