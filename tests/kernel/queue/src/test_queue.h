/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_FIFO_H__
#define __TEST_FIFO_H__

#include <ztest.h>
#include <irq_offload.h>

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
#endif
extern void test_queue_alloc(void);
extern struct k_mem_pool test_pool;

typedef struct qdata {
	sys_snode_t snode;
	u32_t data;
	bool allocated;
} qdata_t;
#endif
