/**
 * @file
 * @brief microkernel mutex header file
 */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MUTEX_H
#define MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include  <microkernel/base_api.h>

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
 * @brief Initializer for mutexes
 */
#define __MUTEX_DEFAULT \
	{ \
	  .owner = ANYTASK, \
	  .current_owner_priority = 64, \
	  .original_owner_priority = 64, \
	  .level = 0, \
	  .Waiters = NULL, \
	  .Count = 0, \
	  .Confl = 0, \
	}

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

#ifdef __cplusplus
}
#endif

#endif /* MUTEX_H */
