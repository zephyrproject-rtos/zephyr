/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	mutex.h
 * @brief	Mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#define __METAL_MUTEX__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup mutex Mutex Interfaces
 *  @{ */

#include <metal/system/@PROJECT_SYSTEM@/mutex.h>

/**
 * @brief	Initialize a libmetal mutex.
 * @param[in]	mutex	Mutex to initialize.
 */
static inline void metal_mutex_init(metal_mutex_t *mutex)
{
	__metal_mutex_init(mutex);
}

/**
 * @brief	Deinitialize a libmetal mutex.
 * @param[in]	mutex	Mutex to deinitialize.
 */
static inline void metal_mutex_deinit(metal_mutex_t *mutex)
{
	__metal_mutex_deinit(mutex);
}

/**
 * @brief	Try to acquire a mutex 
 * @param[in]	mutex	Mutex to mutex.
 * @return	0 on failure to acquire, non-zero on success.
 */
static inline int metal_mutex_try_acquire(metal_mutex_t *mutex)
{
	return __metal_mutex_try_acquire(mutex);
}

/**
 * @brief	Acquire a mutex 
 * @param[in]	mutex	Mutex to mutex.
 */
static inline void metal_mutex_acquire(metal_mutex_t *mutex)
{
	__metal_mutex_acquire(mutex);
}

/**
 * @brief	Release a previously acquired mutex.
 * @param[in]	mutex	Mutex to mutex.
 * @see metal_mutex_try_acquire, metal_mutex_acquire
 */
static inline void metal_mutex_release(metal_mutex_t *mutex)
{
	__metal_mutex_release(mutex);
}

/**
 * @brief	Checked if a mutex has been acquired.
 * @param[in]	mutex	mutex to check.
 * @see metal_mutex_try_acquire, metal_mutex_acquire
 */
static inline int metal_mutex_is_acquired(metal_mutex_t *mutex)
{
	return __metal_mutex_is_acquired(mutex);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_MUTEX__H__ */
