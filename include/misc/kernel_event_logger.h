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
 * @brief Kernel event logger support.
 */


#include <misc/event_logger.h>

#ifndef __KERNEL_EVENT_LOGGER_H__
#define __KERNEL_EVENT_LOGGER_H__

#ifdef CONFIG_KERNEL_EVENT_LOGGER

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
#define KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID             0x0001
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
#define KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID                  0x0002
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
#define KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID                      0x0003
#endif

/**
 * @brief Kernel Event Logger
 * @defgroup nanokernel_event_logger Kernel Event Logger
 * @{
 */

#ifndef _ASMLANGUAGE

/**
 * Global variable of the ring buffer that allows user to implement
 * their own reading routine.
 */
struct event_logger sys_k_event_logger;


/**
 * @brief Sends a event message to the kernel event logger.
 *
 * @details Sends a event message to the kernel event logger
 * and informs that there are messages available.
 *
 * @param event_id   The identification of the event.
 * @param data       Pointer to the data of the message.
 * @param data_size  Size of the data in 32-bit words.
 *
 * @return No return value.
 */
#define sys_k_event_logger_put(event_id, data, data_size) \
	sys_event_logger_put(&sys_k_event_logger, event_id, data, data_size)


/**
 * @brief Sends a event message to the kernel event logger with the current
 * timestamp.
 *
 * @details Sends a event message to the kernel event logger and informs that
 * there messages available. The timestamp when the event occurred is stored
 * as part of the event message.
 *
 * @param event_id   The identification of the event.
 *
 * @return No return value.
 */
void sys_k_event_logger_put_timed(uint16_t event_id);


/**
 * @brief Retrieves a kernel event message.
 *
 * @details Retrieves a kernel event message copying it to the provided
 * buffer. If the buffer is smaller than the message size the function returns
 * an error. The function retrieves messages in FIFO order.
 *
 * @param event_id     Pointer to the id of the event fetched
 * @param dropped_event_count Pointer to how many events were dropped
 * @param buffer       Pointer to the buffer where the message will be copied.
 * @param buffer_size  Size of the buffer in 32-bit words.
 *
 * @return -EMSGSIZE if the buffer size is smaller than the message size,
 * the amount of 32-bit words copied or zero if there are no kernel event
 * messages available.
 */
#define sys_k_event_logger_get(event_id, dropped, buffer, buffer_size) \
	sys_event_logger_get(&sys_k_event_logger, event_id, dropped, buffer, \
			     buffer_size)


/**
 * @brief Retrieves a kernel event message, wait if there is no message
 * available.
 *
 * @details Retrieves a kernel event message copying it to the provided
 * buffer. If the buffer is smaller than the message size the function returns
 * an error. The function retrieves messages in FIFO order. If there is no
 * kernel event message available the caller pends until a new message is
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
#define sys_k_event_logger_get_wait(event_id, dropped, buffer, buffer_size) \
	sys_event_logger_get_wait(&sys_k_event_logger, event_id, dropped, \
				  buffer, buffer_size)


#ifdef CONFIG_NANO_TIMEOUTS

/**
 * @brief Retrieves a kernel event message, wait with a timeout if there is
 * no profiling event messages available.
 *
 * @details Retrieves a kernel event message copying it to the provided
 * buffer. If the buffer is smaller than the message size the function returns
 * an error. The function retrieves messages in FIFO order. If there are no
 * kernel event messages available the caller pends until a new message is
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
#define sys_k_event_logger_get_wait_timeout(event_id, dropped, buffer, buffer_size, \
				      timeout) \
	sys_event_logger_get_wait_timeout(event_id, dropped, \
					  &sys_k_event_logger, buffer, \
					  buffer_size, timeout)
#endif /* CONFIG_NANO_TIMEOUTS */


#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH

/**
 * @brief Register the fiber that calls the function as collector
 *
 * @details Initialize internal profiling data. This avoid registering
 * the context switch of the collector fiber when
 * CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH is enable.
 *
 * @return No return value.
 */
void sys_k_event_logger_register_as_collector(void);

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
void _sys_k_event_logger_enter_sleep(void);
#else
static inline void _sys_k_event_logger_enter_sleep(void) {};
#endif

#else /* !CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH */
static inline void sys_k_event_logger_register_as_collector(void) {};
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH */

#endif /* _ASMLANGUAGE */

#else /* !CONFIG_KERNEL_EVENT_LOGGER */

#ifndef _ASMLANGUAGE

static inline void sys_k_event_logger_put(uint16_t event_id, uint32_t *event_data,
	uint8_t data_size) {};
static inline void sys_k_event_logger_put_timed(uint16_t event_id) {};
static inline void _sys_k_event_logger_enter_sleep(void) {};

#endif /* _ASMLANGUAGE */

/**
* @}
*/

#endif /* CONFIG_KERNEL_EVENT_LOGGER */

#endif /* __KERNEL_EVENT_LOGGER_H__ */
