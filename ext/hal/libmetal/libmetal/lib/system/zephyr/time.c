/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/time.c
 * @brief	Zephyr libmetal time handling.
 */

#include <metal/time.h>
#include <sys_clock.h>

extern volatile u64_t _sys_clock_tick_count;

unsigned long long metal_get_timestamp(void)
{
	return (unsigned long long)_sys_clock_tick_count;
}

