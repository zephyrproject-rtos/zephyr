/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/time.c
 * @brief	NuttX libmetal time handling.
 */

#include <metal/time.h>
#include <nuttx/clock.h>

unsigned long long metal_get_timestamp(void)
{
	unsigned long long t = 0;
	struct timespec tp;
	int r;

	r = clock_systimespec(&tp);
	if (!r) {
		t = (unsigned long long)tp.tv_sec * NSEC_PER_SEC;
		t += tp.tv_nsec;
	}
	return t;
}
