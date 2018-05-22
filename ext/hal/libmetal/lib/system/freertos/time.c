/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/time.c
 * @brief	freertos libmetal time handling.
 */

#include <FreeRTOS.h>
#include <task.h>
#include <metal/time.h>

unsigned long long metal_get_timestamp(void)
{
	return (unsigned long long)xTaskGetTickCount();
}

