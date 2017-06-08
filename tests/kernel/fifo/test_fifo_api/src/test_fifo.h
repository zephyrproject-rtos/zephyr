/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_FIFO_H__
#define __TEST_FIFO_H__

#include <ztest.h>
#include <irq_offload.h>

typedef struct fdata {
	sys_snode_t snode;
	u32_t data;
} fdata_t;
#endif
