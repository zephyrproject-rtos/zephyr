/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/condition.h
 * @brief	Generic condition variable primitives for libmetal.
 */

#ifndef __METAL_CONDITION__H__
#error "Include metal/condition.h instead of metal/freertos/condition.h"
#endif

#ifndef __METAL_FREERTOS_CONDITION__H__
#define __METAL_FREERTOS_CONDITION__H__

#include <metal/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

struct metal_condition {
	metal_mutex_t *m; /**< mutex.
	                       The condition variable is attached to
	                       this mutex when it is waiting.
	                       It is also used to check correctness
	                       in case there are multiple waiters. */

	atomic_int v; /**< condition variable value. */
};

/** Static metal condition variable initialization. */
#define METAL_CONDITION_INIT		{ NULL, ATOMIC_VAR_INIT(0) }

static inline void metal_condition_init(struct metal_condition *cv)
{
	/* TODO: Implement condition variable for FreeRTOS */
	(void)cv;
	return;
}

static inline int metal_condition_signal(struct metal_condition *cv)
{
	/* TODO: Implement condition variable for FreeRTOS */
	(void)cv;
	return 0;
}

static inline int metal_condition_broadcast(struct metal_condition *cv)
{
	/* TODO: Implement condition variable for FreeRTOS */
	(void)cv;
	return 0;
}


#ifdef __cplusplus
}
#endif

#endif /* __METAL_FREERTOS_CONDITION__H__ */
