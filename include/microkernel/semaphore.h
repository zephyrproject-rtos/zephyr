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
extern int _task_sem_take(ksem_t sema, int32_t ticks);
extern ksem_t _task_sem_group_take(ksemg_t semagroup, int32_t ticks);

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
 * This routine takes semaphore @a s. If the semaphore's count is zero the
 * routine immediately returns a failure indication.
 *
 * @param s Semaphore name.
 *
 * @return RC_OK on success, or RC_FAIL on failure.
 */
#define task_sem_take(s) _task_sem_take(s, TICKS_NONE)

/**
 *
 * @brief Take semaphore or wait.
 *
 * This routine takes semaphore @a s. If the semaphore's count is zero the
 * routine waits until it is given.
 *
 * @param s Semaphore name.
 *
 * @return RC_OK.
 */
#define task_sem_take_wait(s) _task_sem_take(s, TICKS_UNLIMITED)

/**
 *
 * @brief Take semaphore from group or wait.
 *
 * This routine takes a semaphore from semaphore group @a g. If all semaphores
 * in the group have a count of zero the routine waits until one of themn
 * can be taken.
 *
 * @param g Array of semaphore names, terminated by ENDLIST.
 *
 * @return Name of semaphore that was taken.
 */
#define task_sem_group_take_wait(g) _task_sem_group_take(g, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 *
 * @brief Take semaphore or timeout.
 *
 * This routine takes semaphore @a s. If the semaphore's count is zero the
 * routine waits until it is given, or until the specified time limit
 * is reached.
 *
 * @param s Semaphore name.
 * @param t Maximum number of ticks to wait.
 *
 * @return RC_OK on success, or RC_TIME on timeout.
 */
#define task_sem_take_wait_timeout(s, t) _task_sem_take(s, t)

/**
 *
 * @brief Take semaphore from group or timeout.
 *
 * This routine takes a semaphore from semaphore group @a g. If all semaphores
 * in the group have a count of zero the routine waits until one of them
 * can be taken, or until the specified time limit is reached.
 *
 * @param g Array of semaphore names, terminated by ENDLIST.
 * @param t Maximum number of ticks to wait.
 *
 * @return Name of semaphore that was taken, or ENDLIST on timeout.
 */
#define task_sem_group_take_wait_timeout(g, t) _task_sem_group_take(g, t)
#endif


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
