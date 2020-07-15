/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016-2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure time
 *
 */
#include <kernel.h>
#include <zephyr.h>
#include <ksched.h>

#include "footprint.h"

void run_heap_malloc_free(void)
{
	void *allocated_mem = k_malloc(10);

	if (allocated_mem == NULL) {
		printk("\n Malloc failed\n");
		return;
	}

	k_free(allocated_mem);
}
