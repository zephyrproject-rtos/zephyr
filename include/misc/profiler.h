/*
 * Copyright (c) 2015 Intel Corporation
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
 * 3) Neither the name of Intel Corporation nor the names of its contributors
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

/*
 * @file
 * @brief Profiler support.
 */


#include <misc/event_logger.h>

#ifndef __PROFILE_H__
#define __PROFILE_H__

#ifdef CONFIG_KERNEL_PROFILER

#ifdef CONFIG_PROFILER_CONTEXT_SWITCH
#define PROFILER_CONTEXT_SWITCH_EVENT_ID             0x0001
#endif /* CONFIG_PROFILER_CONTEXT_SWITCH */

/**
 * Global variable of the ring buffer that allows user to implement
 * their own reading routine.
 */
struct event_logger sys_profiler_logger;


/**
 * @brief Sends a profile event message to the profiler.
 *
 * @details Sends a profile message to the profiler logger
 * and informs that there are messages available.
 *
 * @param event_id   The identification of the profiler event.
 * @param data       Pointer to the data of the message.
 * @param data_size  Size of the data in 32-bit words.
 *
 * @return No return value.
 */
#define sys_profiler_put(event_id, data, data_size) \
	sys_event_logger_put(&sys_profiler_logger, event_id, data, data_size)


/**
 * @brief Sends a profile event message to the profiler with the current
 * timestamp.
 *
 * @details Sends a profile message to the profiler logger and informs that
 * there messages available. The timestamp when the event occurred is stored
 * as part of the event message.
 *
 * @param event_id   The identification of the profiler event.
 *
 * @return No return value.
 */
void sys_profiler_put_timed(uint16_t event_id);


/**
 * @brief Retrieves a profiler event message.
 *
 * @details Retrieves a profiler event message copying it to the provided
 * buffer. If the buffer is smaller than the message size the function returns
 * an error. The function retrieves messages in FIFO order.
 *
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size,
 * the amount of 32-bit words copied or zero if there are no profile event
 * messages available.
 */
#define sys_profiler_get(buffer, buffer_size) \
	sys_event_logger_get(&sys_profiler_logger, buffer, buffer_size)


/**
 * @brief Retrieves a profiler event message, wait if there is no message
 * available.
 *
 * @details Retrieves a profiler event message copying it to the provided
 * buffer. If the buffer is smaller than the message size the function returns
 * an error. The function retrieves messages in FIFO order. If there is no
 * profiler event message available the caller pends until a new message is
 * logged.
 *
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size, or
 * the amount of 32-bit words copied.
 */
#define sys_profiler_get_wait(buffer, buffer_size) \
	sys_event_logger_get_wait(&sys_profiler_logger, buffer, buffer_size)


#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief Retrieves a profiler event message, wait with a timeout if there is
 * no profiling event messages available.
 *
 * @details Retrieves a profiler event message copying it to the provided
 * buffer. If the buffer is smaller than the message size the function returns
 * an error. The function retrieves messages in FIFO order. If there are no
 * profiler event messages available the caller pends until a new message is
 * logged or the timeout expires.
 *
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words.
 * @param timeout      Timeout in ticks.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size, the
 * amount of 32-bit words copied or zero if the timeout expires and the was no
 * message available.
 */
#define sys_profiler_get_wait_timeout(buffer, buffer_size, timeout) \
	sys_event_logger_get_wait_timeout(&sys_profiler_logger, buffer, \
	buffer_size, timeout)
#endif /* CONFIG_NANO_TIMEOUTS */


#ifdef CONFIG_PROFILER_CONTEXT_SWITCH

/**
 * @brief Register the fiber that calls the function as collector
 *
 * @details Initialize internal profiling data. This avoid registering the
 * context switch of the collector fiber when CONFIG_PROFILE_CONTEXT_SWITCH
 * is enable.
 *
 * @return No return value.
 */
void sys_profiler_register_as_collector(void);
#else /* !CONFIG_PROFILER_CONTEXT_SWITCH */
static inline void sys_profiler_register_as_collector(void) {};
#endif /* CONFIG_PROFILER_CONTEXT_SWITCH */

#else /* !CONFIG_KERNEL_PROFILER */

static inline void sys_profiler_put(uint16_t event_id, uint32_t *event_data,
	uint8_t data_size) {};
static inline void sys_profiler_put_timed(uint16_t event_id) {};
#endif /* CONFIG_KERNEL_PROFILER */

#endif /* __PROFILE_H__ */
