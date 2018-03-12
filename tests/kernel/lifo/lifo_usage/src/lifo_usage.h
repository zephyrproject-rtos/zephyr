/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_LIFO_USAGE_H__
#define __TEST_LIFO_USAGE_H__
#include <misc/slist.h>

typedef struct ldata {
	sys_snode_t snode;
	u32_t data;
} ldata_t;
#endif
