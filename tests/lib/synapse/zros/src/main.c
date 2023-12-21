#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define MY_STACK_SIZE 1024
#define MY_PRIORITY 5

extern void pub_entry_point(void*, void*, void*);
extern void sub_entry_point(void*, void*, void*);

K_THREAD_STACK_DEFINE(pub_stack_area, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(sub_stack_area, MY_STACK_SIZE);

ZTEST(zros, test_pub_sub)
{
    struct k_thread pub_thread;
    k_tid_t pub_tid = k_thread_create(
        &pub_thread, pub_stack_area,
        K_THREAD_STACK_SIZEOF(pub_stack_area),
        pub_entry_point,
        NULL, NULL, NULL,
        MY_PRIORITY, 0, K_FOREVER);

    struct k_thread sub_thread;
    k_tid_t sub_tid = k_thread_create(
        &sub_thread, sub_stack_area,
        K_THREAD_STACK_SIZEOF(sub_stack_area),
        sub_entry_point,
        NULL, NULL, NULL,
        MY_PRIORITY, 0, K_FOREVER);

    int rc = 0;
    LOG_INF("starting pub");
    k_thread_start(pub_tid);
    LOG_INF("starting sub");
    k_thread_start(sub_tid);
    LOG_INF("joining pub");
    rc = k_thread_join(pub_tid, K_MSEC(10000));
    zassert_true(rc == 0, "failed to join pub thread");
    LOG_INF("joining sub");
    rc = k_thread_join(sub_tid, K_MSEC(10000));
    zassert_true(rc == 0, "failed to join sub thread");
}

ZTEST_SUITE(zros, NULL, NULL, NULL, NULL, NULL);

// vi: ts=4 sw=4 et
