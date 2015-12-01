/*
 * Copyright (c) 1997-2010, 2012-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 *@brief Microkernel semaphore header file.
 */

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

/**
 * @brief Microkernel Semaphores
 * @defgroup microkernel_semaphore Microkernel Semaphores
 * @ingroup microkernel_services
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/base_api.h>

extern void _k_sem_struct_value_update(int n, struct _k_sem_struct *S);

/*
 * Initializer for semaphore
 */
#define __K_SEMAPHORE_DEFAULT \
	{ \
	  .waiters = NULL, \
	  .level = 0, \
	  .count = 0, \
	}

/**
 *
 * @brief Give semaphore (from an ISR).
 *
 * This routine gives semaphore @a sema from an ISR, rather than a task.
 *
 * @param sema Semaphore name.
 *
 * @return N/A
 */
extern void isr_sem_give(ksem_t sema);

/**
 *
 * @brief Give semaphore (from a fiber).
 *
 * This routine gives semaphore @a sema from a fiber, rather than a task.
 *
 * @param sema Semaphore name.
 *
 * @return N/A
 */
extern void fiber_sem_give(ksem_t sema);

/**
 *
 * @brief Give semaphore.
 *
 * This routine gives semaphore @a sema.
 *
 * @param sema Semaphore name.
 *
 * @return N/A
 */
extern void task_sem_give(ksem_t sema);

/**
 *
 * @brief Give a group of semaphores.
 *
 * This routine gives each semaphore in sempahore group @a semagroup.
 * This method is faster than giving the semaphores individually, and
 * ensures that all the semaphores are given before any waiting tasks run.
 *
 * @param semagroup Array of semaphore names, terminated by ENDLIST.
 *
 * @return N/A
 */
extern void task_sem_group_give(ksemg_t semagroup);

/**
 *
 * @brief Read a semaphore's count.
 *
 * This routine reads the current count of semaphore @a sema.
 *
 * @param sema Semaphore name.
 *
 * @return Semaphore count.
 */
extern int task_sem_count_get(ksem_t sema);

/**
 *
 * @brief Reset semaphore count.
 *
 * This routine resets the count of semaphore @a sema to zero.
 *
 * @param sema Semaphore name.
 *
 * @return N/A
 */
extern void task_sem_reset(ksem_t sema);

/**
 *
 * @brief Reset a group of semaphores
 *
 * This routine resets the count for each semaphore in sempahore group
 * @a semagroup to zero. This method is faster than resetting the semaphores
 * individually.
 *
 * @param semagroup Array of semaphore names, terminated by ENDLIST.
 *
 * @return N/A
 */
extern void task_sem_group_reset(ksemg_t semagroup);

/**
 *
 * @brief Take semaphore or fail.
 *
 * This routine takes semaphore @a sema. If the semaphore's count is zero the
 * routine immediately returns a failure indication.
 *
 * @param sema Semaphore name.
 * @param timeout Affects the action taken should the semaphore be unavailable.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise wait up to the specified number of ticks before
 * timing out.
 *
 * @retval RC_OK Successfully took semaphore
 * @retval RC_TIME Timed out while waiting for semaphore
 * @retval RC_FAIL Failed to immediately take semaphore when
 * @a timeout = TICKS_NONE
 */
extern int task_sem_take(ksem_t sema, int32_t timeout);

/**
 *
 * @brief Wait for a semaphore from the semaphore group
 *
 * This routine waits up to @a timeout ticks to take a semaphore from the
 * semaphore group @a group.
 *
 * @param group Array of semaphore names, terminated by ENDLIST.
 * @param timeout Affects the action taken should no semaphore be available.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait
 * as long as necessary. Otherwise wait up to the specified number of ticks
 * before timing out.
 *
 * @return Name of semaphore that was taken if successful, else ENDLIST
 */
extern ksem_t task_sem_group_take(ksemg_t group, int32_t timeout);

/**
 * @brief Define a private microkernel semaphore
 *
 * @param name Semaphore name.
 */
#define DEFINE_SEMAPHORE(name) \
	struct _k_sem_struct _k_sem_obj_##name = __K_SEMAPHORE_DEFAULT; \
	const ksem_t name = (ksem_t)&_k_sem_obj_##name;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* _SEMAPHORE_H */
