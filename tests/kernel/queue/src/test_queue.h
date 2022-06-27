/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_FIFO_H__
#define __TEST_FIFO_H__

#include <ztest.h>
#include <zephyr/irq_offload.h>
#include <ztest_error_hook.h>

extern void test_queue_thread2thread(void);
extern void test_queue_thread2isr(void);
extern void test_queue_isr2thread(void);
extern void test_queue_get_2threads(void);
extern void test_queue_get_fail(void);
extern void test_queue_loop(void);
#ifdef CONFIG_USERSPACE
extern void test_queue_supv_to_user(void);
extern void test_auto_free(void);
extern void test_queue_alloc_prepend_user(void);
extern void test_queue_alloc_append_user(void);
extern void test_queue_init_null(void);
extern void test_queue_alloc_append_null(void);
extern void test_queue_alloc_prepend_null(void);
extern void test_queue_get_null(void);
extern void test_queue_is_empty_null(void);
extern void test_queue_peek_head_null(void);
extern void test_queue_peek_tail_null(void);
extern void test_queue_cancel_wait_error(void);
#endif
extern void test_queue_alloc(void);
extern void test_queue_poll_race(void);
extern void test_multiple_queues(void);
extern void test_queue_multithread_competition(void);
extern void test_access_kernel_obj_with_priv_data(void);
extern void test_queue_append_list_error(void);
extern void test_queue_merge_list_error(void);
extern void test_queue_unique_append(void);

extern struct k_heap test_pool;

typedef struct qdata {
	sys_snode_t snode;
	uint32_t data;
	bool allocated;
} qdata_t;
#endif
