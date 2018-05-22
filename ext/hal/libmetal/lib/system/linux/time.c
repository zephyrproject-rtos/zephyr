/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/time.c
 * @brief	Linux libmetal time handling.
 */

#include <unistd.h>
#include <time.h>
#include <metal/time.h>

#define NS_PER_S        (1000 * 1000 * 1000)

unsigned long long metal_get_timestamp(void)
{
	unsigned long long t = 0;
	struct timespec tp;
	int r;

	r = clock_gettime(CLOCK_MONOTONIC, &tp);
	if (r == -1) {
		metal_log(METAL_LOG_ERROR,"clock_gettime failed!\n");
		return t;
	} else {
		t = tp.tv_sec * (NS_PER_S);
		t += tp.tv_nsec;
	}
	return t;
}

