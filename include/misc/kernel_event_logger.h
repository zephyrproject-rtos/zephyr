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

/**
 * @file
 * @brief Kernel event logger support.
 */


#include <misc/event_logger.h>

#ifndef __KERNEL_EVENT_LOGGER_H__
#define __KERNEL_EVENT_LOGGER_H__

/**
 * @brief Kernel Event Logger
 * @defgroup nanokernel_event_logger Kernel Event Logger
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef CONFIG_TASK_MONITOR
#define KERNEL_EVENT_LOGGER_TASK_MON_TASK_STATE_CHANGE_EVENT_ID 0x0004
#define KERNEL_EVENT_LOGGER_TASK_MON_CMD_PACKET_EVENT_ID        0x0005
#define KERNEL_EVENT_LOGGER_TASK_MON_KEVENT_EVENT_ID            0x0006
#endif

#ifndef _ASMLANGUAGE

/**
 * Global variable of the ring buffer that allows user to implement
 * their own reading routine.
 */
struct event_logger sys_k_event_logger;


#ifdef CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP

/**
 * Callback used to set event timestamp
 */
typedef uint32_t (*sys_k_timer_func)(void);
extern sys_k_timer_func timer_func;

static inline uint32_t _sys_k_get_time(void)
{
	if (timer_func)
		return timer_func();
	else
		return sys_cycle_get_32();
}

/**
 * @brief Set kernel event logger timestamp function
 *
 * @details Calling this function permits to set the function
 * to be called by kernel event logger for setting the event
 * timestamp. By default, kernel event logger is using the
 * system timer. But on some boards where the timer driver
 * maintains the system timer cycle accumulator in software,
 * such as ones using the LOAPIC timer, the system timer behavior
 * leads to timestamp errors. For example, the timer interrupt is
 * logged with a wrong timestamp since the HW timer value has been
 * reset (periodic mode) but accumulated value not updated yet
 * (done later in the ISR).
 *
 * @param func Pointer to a function returning a 32-bit timer
 *             Prototype: uint32_t (*func)(void)
 */
void sys_k_event_logger_set_timer(sys_k_timer_func func);
#else
static inline uint32_t _sys_k_get_time(void)
{
	return sys_cycle_get_32();
}
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC

extern int _sys_k_event_logger_mask;

/**
 * @brief Set kernel event logger filtering mask
 *
 * @details Calling this macro sets the mask used to select which events
 * to store in the kernel event logger ring buffer. This flag can be set
 * at runtime and at any moment.
 * This capability is only available when CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
 * is set. If enabled, no event is enabled for logging at initialization.
 * The mask bits shall be set according to events ID defined in
 * kernel_event_logger.h
 * For example, to enable interrupt logging the following shall be done:
 * sys_k_event_logger_set_mask(sys_k_event_logger_get_mask |
 *                (1 << (KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID - 1)))
 * To disable it:
 * sys_k_event_logger_set_mask(sys_k_event_logger_get_mask &
 *                ~(1 << (KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID - 1)))
 *
 * WARNING: task monitor events are not covered by this API. Please refer
 * to sys_k_event_logger_set_monitor_mask / sys_k_event_logger_get_monitor_mask
 */
static inline void sys_k_event_logger_set_mask(int value)
{
	_sys_k_event_logger_mask = value;
}

/**
 * @brief Get kernel event logger filtering mask
 *
 * @details Calling this macro permits to read the mask used to select which
 * events are stored in the kernel event logger ring buffer. This macro can be
 * used at runtime and at any moment.
 * This capability is only available when CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
 * is set. If enabled, no event is enabled for logging at initialization.
 *
 * @see sys_k_event_logger_set_mask(value) for details
 *
 * WARNING: task monitor events are not covered by this API. Please refer
 * to sys_k_event_logger_set_monitor_mask / sys_k_event_logger_get_monitor_mask
 */
static inline int sys_k_event_logger_get_mask(void)
{
	return _sys_k_event_logger_mask;
}

#ifdef CONFIG_TASK_MONITOR

extern int _k_monitor_mask;

/**
 * @brief Set task monitor filtering mask
 *
 * @details Calling this function sets the mask used to select which task monitor
 * events to store in the kernel event logger ring buffer. This flag can be set
 * at runtime and at any moment.
 * This capability is only available when CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
 * is set. If enabled, no event is enabled for logging at initialization
 * so CONFIG_TASK_MONITOR_MASK is ignored
 *
 * The mask bits shall be set according to monitor events defined in
 * micro_private.h
 *
 * For example, to enable k_swapper cmd logging the following shall be done:
 * sys_k_event_logger_set_monitor_mask(sys_k_event_logger_get_monitor_mask |
 *                (1 << (MON_KSERV - 1)))
 * To disable it:
 * sys_k_event_logger_set_mask(sys_k_event_logger_get_mask &
 *                ~(1 << (MON_KSERV - 1)))
 *
 */
static inline void sys_k_event_logger_set_monitor_mask(int value)
{
	_k_monitor_mask = value;
}

/**
 * @brief Get task monitor filtering mask
 *
 * @details Calling this function permits to read the mask used to select which
 * task monitor events to store in the kernel event logger ring buffer. This
 * function can be used at runtime and at any moment.
 * This capability is only available when CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
 * is set. If enabled, no event is enabled for logging at initialization
 * so CONFIG_TASK_MONITOR_MASK is ignored
 *
 * @see sys_k_event_logger_set_monitor_mask() for details
 *
 */
static inline int sys_k_event_logger_get_monitor_mask(void)
{
	return _k_monitor_mask;
}

#endif /* CONFIG_TASK_MONITOR */
#endif /* CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC */

/**
 * @brief Check if an event type has to be logged or not
 *
 * @details This function must be used before calling any sys_k_event_logger_put*
 * function. In case CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC is enabled, that function
 * permits to enable or disable the logging of each individual event at runtime
 *
 * @param event_type   The identification of the event.
 *
 */

static inline int sys_k_must_log_event(int event_type)
{
#ifdef CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
	return !!(_sys_k_event_logger_mask & (1 << (event_type - 1)));
#else
	return 1;
#endif
}

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
 * @param dropped      Pointer to how many events were dropped
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
 * @param dropped      Pointer to how many events were dropped
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
 * @param dropped      Pointer to how many events were dropped
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
	sys_event_logger_get_wait_timeout(&sys_k_event_logger, event_id, \
					  dropped, buffer, \
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
#else /* !CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH */
static inline void sys_k_event_logger_register_as_collector(void) {};
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH */

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
void _sys_k_event_logger_enter_sleep(void);
#else
static inline void _sys_k_event_logger_enter_sleep(void) {};
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
void _sys_k_event_logger_interrupt(void);
#else
static inline void _sys_k_event_logger_interrupt(void) {};
#endif

#endif /* _ASMLANGUAGE */

#else /* !CONFIG_KERNEL_EVENT_LOGGER */

#ifndef _ASMLANGUAGE

static inline void sys_k_event_logger_put(uint16_t event_id, uint32_t *event_data,
	uint8_t data_size) {};
static inline void sys_k_event_logger_put_timed(uint16_t event_id) {};
static inline void _sys_k_event_logger_enter_sleep(void) {};

#endif /* _ASMLANGUAGE */

#endif /* CONFIG_KERNEL_EVENT_LOGGER */

#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif /* __KERNEL_EVENT_LOGGER_H__ */
