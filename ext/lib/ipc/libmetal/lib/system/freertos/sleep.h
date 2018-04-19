/*
 * Copyright (c) 2018, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/sleep.h
 * @brief	FreeRTOS sleep primitives for libmetal.
 */

#ifndef __METAL_SLEEP__H__
#error "Include metal/sleep.h instead of metal/freertos/sleep.h"
#endif

#ifndef __METAL_FREERTOS_SLEEP__H__
#define __METAL_FREERTOS_SLEEP__H__

#include <FreeRTOS.h>
#include <task.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int __metal_sleep_usec(unsigned int usec)
{
	const TickType_t xDelay = usec / portTICK_PERIOD_MS;
	vTaskDelay(xDelay);
	return 0;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_FREERTOS_SLEEP__H__ */
