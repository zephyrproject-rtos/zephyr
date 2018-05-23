/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	condition.h
 * @brief	Condition variable for libmetal.
 */

#ifndef __METAL_CONDITION__H__
#define __METAL_CONDITION__H__

#include <metal/mutex.h>
#include <metal/utilities.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup condition Condition Variable Interfaces
 *  @{ */

/** Opaque libmetal condition variable data structure. */
struct metal_condition;

/**
 * @brief        Initialize a libmetal condition variable.
 * @param[in]	 cv	condition variable to initialize.
 */
static inline void metal_condition_init(struct metal_condition *cv);

/**
 * @brief        Notify one waiter.
 *               Before calling this function, the caller
 *               should have acquired the mutex.
 * @param[in]    cv    condition variable
 * @return       zero on no errors, non-zero on errors
 * @see metal_condition_wait, metal_condition_broadcast
 */
static inline int metal_condition_signal(struct metal_condition *cv);

/**
 * @brief        Notify all waiters.
 *               Before calling this function, the caller
 *               should have acquired the mutex.
 * @param[in]    cv    condition variable
 * @return       zero on no errors, non-zero on errors
 * @see metal_condition_wait, metal_condition_signal
 */
static inline int metal_condition_broadcast(struct metal_condition *cv);

/**
 * @brief        Block until the condition variable is notified.
 *               Before calling this function, the caller should
 *               have acquired the mutex.
 * @param[in]    cv    condition variable
 * @param[in]    m     mutex
 * @return	 0 on success, non-zero on failure.
 * @see metal_condition_signal
 */
int metal_condition_wait(struct metal_condition *cv, metal_mutex_t *m);

#include <metal/system/@PROJECT_SYSTEM@/condition.h>

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_CONDITION__H__ */
