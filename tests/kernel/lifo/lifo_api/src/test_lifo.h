/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_LIFO_H__
#define __TEST_LIFO_H__

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>

typedef struct ldata {
	sys_snode_t snode;
	uint32_t data;
} ldata_t;
#endif
