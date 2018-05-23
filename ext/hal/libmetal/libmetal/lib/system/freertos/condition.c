/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/condition.c
 * @brief	Generic libmetal condition variable handling.
 */

#include <metal/condition.h>

int metal_condition_wait(struct metal_condition *cv,
			 metal_mutex_t *m)
{
	/* TODO: Implement condition variable for FreeRTOS */
	(void)cv;
	(void)m;
	return 0;
}
