/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/mutex.h
 * @brief	FreeRTOS mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/freertos/mutex.h"
#endif

#ifndef __METAL_FREERTOS_MUTEX__H__
#define __METAL_FREERTOS_MUTEX__H__

#include <metal/assert.h>
#include "FreeRTOS.h"
#include "semphr.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SemaphoreHandle_t m;
} metal_mutex_t;

/*
 * METAL_MUTEX_INIT - used for initializing an mutex elmenet in a static struct
 * or global
 */
#define METAL_MUTEX_INIT(m) { NULL }
/*
 * METAL_MUTEX_DEFINE - used for defining and initializing a global or
 * static singleton mutex
 */
#define METAL_MUTEX_DEFINE(m) metal_mutex_t m = METAL_MUTEX_INIT(m)

static inline void __metal_mutex_init(metal_mutex_t *mutex)
{
	metal_assert(mutex);
	mutex->m = xSemaphoreCreateMutex();
	metal_assert(mutex->m != NULL);
}

static inline void __metal_mutex_deinit(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m != NULL);
	vSemaphoreDelete(mutex->m);
	mutex->m=NULL;
}

static inline int __metal_mutex_try_acquire(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m != NULL);
	return xSemaphoreTake(mutex->m, ( TickType_t ) 0 );
}

static inline void __metal_mutex_acquire(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m != NULL);
	xSemaphoreTake(mutex->m, portMAX_DELAY);
}

static inline void __metal_mutex_release(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m != NULL);
	xSemaphoreGive(mutex->m);
}

static inline int __metal_mutex_is_acquired(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m != NULL);
	return (NULL == xSemaphoreGetMutexHolder(mutex->m)) ? 0 : 1;
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_FREERTOS_MUTEX__H__ */
