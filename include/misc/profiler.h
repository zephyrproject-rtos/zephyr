/*
 * Copyright (c) 2015 Intel Corporation
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
#endif

#ifdef CONFIG_PROFILER_INTERRUPT
#define PROFILER_INTERRUPT_EVENT_ID                  0x0002
#endif

#ifdef CONFIG_PROFILER_SLEEP
#define PROFILER_SLEEP_EVENT_ID                      0x0003
#endif

#ifndef _ASMLANGUAGE
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
 * @param event_id     Pointer to the id of the event fetched
 * @param dropped_event_count Pointer to how many events were dropped
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size,
 * the amount of 32-bit words copied or zero if there are no profile event
 * messages available.
 */
#define sys_profiler_get(event_id, dropped, buffer, buffer_size) \
	sys_event_logger_get(&sys_profiler_logger, event_id, dropped, buffer, \
			     buffer_size)


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
 * @param event_id     Pointer to the id of the event fetched
 * @param dropped_event_count Pointer to how many events were dropped
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size, or
 * the amount of 32-bit words copied.
 */
#define sys_profiler_get_wait(event_id, dropped, buffer, buffer_size) \
	sys_event_logger_get_wait(&sys_profiler_logger, event_id, dropped, \
				  buffer, buffer_size)


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
 * @param event_id     Pointer to the id of the event fetched
 * @param dropped_event_count Pointer to how many events were dropped
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words.
 * @param timeout      Timeout in ticks.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size, the
 * amount of 32-bit words copied or zero if the timeout expires and the was no
 * message available.
 */
#define sys_profiler_get_wait_timeout(event_id, dropped, buffer, buffer_size, \
				      timeout) \
	sys_event_logger_get_wait_timeout(event_id, dropped, \
					  &sys_profiler_logger, buffer, \
					  buffer_size, timeout)
#endif /* CONFIG_NANO_TIMEOUTS */


#ifdef CONFIG_PROFILER_CONTEXT_SWITCH

/**
 * @brief Register the fiber that calls the function as collector
 *
 * @details Initialize internal profiling data. This avoid registering the
 * context switch of the collector fiber when CONFIG_PROFILE_THREAD_SWITCH
 * is enable.
 *
 * @return No return value.
 */
void sys_profiler_register_as_collector(void);
#else /* !CONFIG_PROFILER_CONTEXT_SWITCH */
static inline void sys_profiler_register_as_collector(void) {};
#endif /* CONFIG_PROFILER_CONTEXT_SWITCH */

#endif /* _ASMLANGUAGE */

#else /* !CONFIG_KERNEL_PROFILER */

#ifndef _ASMLANGUAGE

static inline void sys_profiler_put(uint16_t event_id, uint32_t *event_data,
	uint8_t data_size) {};
static inline void sys_profiler_put_timed(uint16_t event_id) {};

#endif /* _ASMLANGUAGE */

#endif /* CONFIG_KERNEL_PROFILER */

#endif /* __PROFILE_H__ */
