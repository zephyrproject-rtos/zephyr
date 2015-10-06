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

/**
 * @cond internal
 */
/**
 * @brief This routine is the entry to the mutex lock kernel service.
 *
 * @param mutex Mutex object
 * @param time Timeout value (in ticks)
 *
 * @return RC_OK on success, RC_FAIL on error, RC_TIME on timeout
 */
extern int _task_mutex_lock(kmutex_t mutex, int32_t time);

/**
 * @brief This routine is the entry to the mutex unlock kernel service.
 *
 * @param mutex Mutex
 *
 * @return N/A
 */
extern void _task_mutex_unlock(kmutex_t mutex);

/**
 * @brief Initializer for mutexes
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
 * @endcond
 */

/**
 * @brief Try to lock mutex.
 *
 * This tries to lock mutex. If the mutex cannot be locked
 * immediately, it returns RC_FAIL.
 *
 * @param m Mutex
 *
 * @return RC_OK on success, RC_FAIL on error
 */
#define task_mutex_lock(m) _task_mutex_lock(m, TICKS_NONE)

/**
 * @brief Wait indefinitely to lock mutex.
 *
 * This waits indefinitely until the mutex can be locked. This is
 * a blocking call.
 *
 * @param m Mutex
 *
 * @return RC_OK on success, RC_FAIL on error
 */
#define task_mutex_lock_wait(m) _task_mutex_lock(m, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief Try to lock mutex with timeout.
 *
 * If the mutex cannot be locked immediately, this will keep trying
 * to lock mutex until certain time specified in t has passed.
 * The timeout value is in ticks.
 *
 * @param m Mutex object
 * @param t Timeout value (in ticks)
 *
 * @return RC_OK on success, RC_FAIL on error, RC_TIME on timeout
 */
#define task_mutex_lock_wait_timeout(m, t) _task_mutex_lock(m, t)
#endif

/**
 * @brief Unlock mutex
 *
 * @param m Mutex
 *
 * @return N/A
 */
#define task_mutex_unlock(m) _task_mutex_unlock(m)

/**
 * @brief Define a private mutex
 *
 * This declares and initializes a private mutex. The new mutex can be
 * passed to the microkernel mutex functions.
 *
 * @param name Name of the mutex
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
