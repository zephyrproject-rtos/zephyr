/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_FIFO_H__
#define __TEST_FIFO_H__

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest_error_hook.h>

extern struct k_heap test_pool;

typedef struct qdata {
	sys_snode_t snode;
	uint32_t data;
	bool allocated;
} qdata_t;
#endif
