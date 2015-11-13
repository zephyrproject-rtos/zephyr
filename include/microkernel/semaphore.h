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

/**
 *
 * @brief Signal a semaphore from an ISR
 *
 * This routine (to only be called from an ISR) signals a semaphore.
 * To signal a semaphore from a task, task_sem_give() should be used instead.
 *
 * @param sema   Semaphore to signal.
 *
 * @return N/A
 */
extern void isr_sem_give(ksem_t sema);

/**
 *
 * @brief Signal a semaphore from a fiber
 *
 * This routine (to only be called from a fiber) signals a semaphore.
 * To signal a semaphore from a task, task_sem_give() should be used instead.
 *
 * @param sema   Semaphore to signal.
 *
 * @return N/A
 */
extern void fiber_sem_give(ksem_t sema);

/**
 *
 * @brief Signal a semaphore
 *
 * This routine signals the specified semaphore.
 *
 * @param sema   Semaphore to signal.
 *
 * @return N/A
 */
extern void task_sem_give(ksem_t sema);

/**
 *
 * @brief Signal a group of semaphores
 *
 * This routine signals a group of semaphores. A semaphore group is an array of
 * semaphore names terminated by the predefined constant ENDLIST.
 *
 * If the semaphore list of waiting tasks is empty, the signal count is
 * incremented, otherwise the highest priority waiting task is released.
 *
 * Using task_sem_group_give() is faster than using multiple single signals,
 * and ensures all signals take place before other tasks run.
 *
 * @param semagroup   Group of semaphores to signal.
 *
 * @return N/A
 */
extern void task_sem_group_give(ksemg_t semagroup);

/**
 *
 * @brief Read the semaphore signal count
 *
 * This routine reads the signal count of the specified semaphore.
 *
 * @param sema   Semaphore to query.
 *
 * @return signal count
 */
extern int task_sem_count_get(ksem_t sema);

/**
 *
 * @brief Reset semaphore count to zero
 *
 * This routine resets the signal count of the specified semaphore to zero.
 *
 * @param sema   Semaphore to reset.
 *
 * @return N/A
 */
extern void task_sem_reset(ksem_t sema);

/**
 *
 * @brief Reset a group of semaphores
 *
 * This routine resets the signal count for a group of semaphores. A semaphore
 * group is an array of semaphore names terminated by the predefined constant
 * ENDLIST.
 *
 * @param semagroup   Group of semaphores to reset.
 *
 * @return N/A
 */
extern void task_sem_group_reset(ksemg_t semagroup);



/**
 * @cond internal
 */

/**
 *
 * @brief Update value of semaphore structure
 *
 * This routine updates the value of the semaphore by 0 or more units, then
 * gives the semaphore to any waiting tasks that can now be satisfied.
 *
 * @param n      Number of additional times semaphore has been given.
 * @param sema   Semaphore structure to update.
 *
 * @return N/A
 */
extern void _k_sem_struct_value_update(int n, struct _k_sem_struct *S);

/**
 *
 * @brief Test a semaphore
 *
 * This routine tests a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented.
 *
 * @param sema   Semaphore to test.
 * @param time   Maximum number of ticks to wait.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
extern int _task_sem_take(ksem_t sema, int32_t time);

/**
 *
 * @brief Test multiple semaphores
 *
 * This routine tests a group of semaphores. A semaphore group is an array of
 * semaphore names terminated by the predefined constant ENDLIST.
 *
 * It returns the ID of the first semaphore in the group whose signal count is
 * greater than zero, and decrements the signal count.
 *
 * @param group   Group of semaphores to test.
 * @param time    Maximum number of ticks to wait.
 *
 * @return N/A
 */
extern ksem_t _task_sem_group_take(ksemg_t semagroup, int32_t time);

/**
 * @brief Initializer for microkernel semaphores
 */
#define __K_SEMAPHORE_DEFAULT \
	{ \
	  .waiters = NULL, \
	  .level = 0, \
	  .count = 0, \
	}

/**
 * @endcond
 */

/**
 *
 * @brief Test a semaphore
 *
 * This routine tests a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented. I waits for none ticks.
 *
 * @param s Semaphore to test.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_sem_take(s) _task_sem_take(s, TICKS_NONE)

/**
 *
 * @brief Test a semaphore
 *
 * This routine tests a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented. I waits for unlimited ticks.
 *
 * @param s Semaphore to test.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_sem_take_wait(s) _task_sem_take(s, TICKS_UNLIMITED)

/**
 *
 * @brief Test multiple semaphores
 *
 * This routine tests a group of semaphores. A semaphore group is an array of
 * semaphore names terminated by the predefined constant ENDLIST.
 *
 * It returns the ID of the first semaphore in the group whose signal count is
 * greater than zero, and decrements the signal count.
 * It waits for unlimited ticks.
 *
 * @param g Group of semaphores to test.
 *
 * @return N/A
 */
#define task_sem_group_take_wait(g) _task_sem_group_take(g, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 *
 * @brief Test a semaphore
 *
 * This routine tests a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented.
 *
 * @param s Semaphore to test.
 * @param t Maximum number of ticks to wait.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_sem_take_wait_timeout(s, t) _task_sem_take(s, t)

/**
 *
 * @brief Test multiple semaphores
 *
 * This routine tests a group of semaphores. A semaphore group is an array of
 * semaphore names terminated by the predefined constant ENDLIST.
 *
 * It returns the ID of the first semaphore in the group whose signal count is
 * greater than zero, and decrements the signal count.
 *
 * @param g Group of semaphores to test.
 * @param t Maximum number of ticks to wait.
 *
 * @return N/A
 */
#define task_sem_group_take_wait_timeout(g, t) _task_sem_group_take(g, t)
#endif


/**
 * @brief Define a private microkernel semaphore
 *
 * This declares and initializes a private semaphore. The new semaphore
 * can be passed to the microkernel semaphore functions.
 *
 * @param name Name of the semaphore
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
