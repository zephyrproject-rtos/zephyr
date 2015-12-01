/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
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
 * @brief Microkernel mutex header file
 */

/**
 * @brief Microkernel Mutexes
 * @defgroup microkernel_mutex Microkernel Mutexes
 * @ingroup microkernel_services
 * @{
 */

#ifndef MUTEX_H
#define MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include  <microkernel/base_api.h>

extern void _task_mutex_unlock(kmutex_t mutex);

/*
 * Initializer for mutexes
 */
#define __MUTEX_DEFAULT \
	{ \
	  .owner = ANYTASK, \
	  .current_owner_priority = 64, \
	  .original_owner_priority = 64, \
	  .level = 0, \
	  .waiters = NULL, \
	  .count = 0, \
	  .num_conflicts = 0, \
	}

/**
 * @brief Lock mutex
 *
 * This routine locks mutex @a mutex. If the mutex is currently locked by
 * another task the routine either waits until it becomes available, or until
 * the specified time limit is reached.
 *
 * A task is permitted to lock a mutex it has already locked; in such a case
 * this routine immediately succeeds.
 *
 * @param mutex Mutex name.
 * @param timeout Affects the action taken should the mutex already be locked.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise wait up to the specified number of ticks before
 * timing out.
 *
 * @retval RC_OK Successfully locked mutex
 * @retval RC_TIME Timed out while waiting for mutex
 * @retval RC_FAIL Failed to immediately lock mutex when
 * @a timeout = TICKS_NONE
 */
extern int task_mutex_lock(kmutex_t mutex, int32_t timeout);

/**
 * @brief Unlock mutex.
 *
 * This routine unlocks mutex @a m. The mutex must currently be locked by the
 * requesting task.
 *
 * The mutex cannot be claimed by another task until it has been unlocked by
 * the requesting task as many times as it was locked by that task.
 *
 * @param m Mutex name.
 *
 * @return N/A
 */
#define task_mutex_unlock(m) _task_mutex_unlock(m)

/**
 * @brief Define a private mutex.
 *
 * @param name Mutex name.
 */
#define DEFINE_MUTEX(name) \
	struct _k_mutex_struct _k_mutex_obj_##name = __MUTEX_DEFAULT; \
	const kmutex_t name = (kmutex_t)&_k_mutex_obj_##name;

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* MUTEX_H */
