/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support.
 */

#include <logging/event_logger.h>

#ifndef __KERNEL_EVENT_LOGGER_H__
#define __KERNEL_EVENT_LOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* pre-defined event types */

#define KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID             0x0001
#define KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID                  0x0002
#define KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID                      0x0003

#ifndef _ASMLANGUAGE

extern struct event_logger sys_k_event_logger;
extern int _sys_k_event_logger_mask;

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
extern void _sys_k_event_logger_enter_sleep(void);
extern void _sys_k_event_logger_exit_sleep(void);
#else
static inline void _sys_k_event_logger_enter_sleep(void) {};
static inline void  _sys_k_event_logger_exit_sleep(void) {};
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
extern void _sys_k_event_logger_interrupt(void);
#else
static inline void _sys_k_event_logger_interrupt(void) {};
#endif

/**
 * @brief Kernel Event Logger
 * @defgroup kernel_event_logger Kernel Event Logger
 * @{
 */

/**
 * @typedef sys_k_timer_func_t
 * @brief Event timestamp generator function type.
 *
 * A timestamp generator function is executed when the kernel event logger
 * generates an event containing a timestamp.
 *
 * @return Timestamp value (application-defined).
 */
typedef uint32_t (*sys_k_timer_func_t)(void);

/**
 * @cond INTERNAL_HIDDEN
 */

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP
extern sys_k_timer_func_t _sys_k_get_time;
#else
#define _sys_k_get_time k_cycle_get_32
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP */

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Set kernel event logger timestamp function.
 *
 * This routine instructs the kernel event logger to call @a func
 * whenever it needs to generate an event timestamp. By default,
 * the kernel's hardware timer is used.
 *
 * @note
 * On some boards the hardware timer is not a pure hardware up counter,
 * which can lead to timestamp errors. For example, boards using the LOAPIC
 * timer can run it in periodic mode, which requires software to update
 * a count of accumulated cycles each time the timer hardware resets itself
 * to zero. This can result in an incorrect timestamp being generated
 * if it occurs after the timer hardware has reset but before the timer ISR
 * has updated accumulated cycle count.
 *
 * @param func Address of timestamp function to be used.
 *
 * @return N/A
 */
#ifdef CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP
static inline void sys_k_event_logger_set_timer(sys_k_timer_func_t func)
{
	_sys_k_get_time = func;
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP */

/**
 * @brief Set kernel event logger filtering mask.
 *
 * This routine specifies which events are recorded by the kernel event logger.
 * It can only be used when dynamic event logging has been configured.
 *
 * Each mask bit corresponds to a kernel event type. The least significant
 * mask bit corresponds to event type 1, the next bit to event type 2,
 * and so on.
 *
 * @param value Bitmask indicating events to be recorded.
 *
 * @return N/A
 */
#ifdef CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
static inline void sys_k_event_logger_set_mask(int value)
{
	_sys_k_event_logger_mask = value;
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC */

/**
 * @brief Get kernel event logger filtering mask.
 *
 * This routine indicates which events are currently being recorded by
 * the kernel event logger. It can only be used when dynamic event logging
 * has been configured. By default, no events are recorded.
 *
 * @return Bitmask indicating events that are being recorded.
 */
#ifdef CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
static inline int sys_k_event_logger_get_mask(void)
{
	return _sys_k_event_logger_mask;
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC */

/**
 * @brief Indicate if an event type is currently being recorded.
 *
 * This routine indicates if event type @a event_type should be recorded
 * by the kernel event logger when the event occurs. The routine should be
 * used by code that writes an event to the kernel event logger to ensure
 * that only events of interest to the application are recorded.
 *
 * @param event_type Event ID.
 *
 * @return 1 if event should be recorded, or 0 if not.
 *
 */
static inline int sys_k_must_log_event(int event_type)
{
#ifdef CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
	return !!(_sys_k_event_logger_mask & (1 << (event_type - 1)));
#else
	ARG_UNUSED(event_type);

	return 1;
#endif
}

/**
 * @brief Write an event to the kernel event logger.
 *
 * This routine writes an event message to the kernel event logger.
 *
 * @param event_id   Event ID.
 * @param event_data Address of event data.
 * @param data_size  Size of event data (number of 32-bit words).
 *
 * @return N/A
 */
static inline void sys_k_event_logger_put(uint16_t event_id,
					  uint32_t *event_data,
					  uint8_t data_size)
{
#ifdef CONFIG_KERNEL_EVENT_LOGGER
	sys_event_logger_put(&sys_k_event_logger, event_id,
			     event_data, data_size);
#else
	ARG_UNUSED(event_id);
	ARG_UNUSED(event_data);
	ARG_UNUSED(data_size);
#endif /* CONFIG_KERNEL_EVENT_LOGGER */
};

/**
 * @brief Write an event to the kernel event logger (with timestamp only).
 *
 * This routine writes an event message to the kernel event logger.
 * The event records a single 32-bit word containing a timestamp.
 *
 * @param event_id Event ID.
 *
 * @return N/A
 */
#ifdef CONFIG_KERNEL_EVENT_LOGGER
extern void sys_k_event_logger_put_timed(uint16_t event_id);
#else
static inline void sys_k_event_logger_put_timed(uint16_t event_id)
{
	ARG_UNUSED(event_id);
};
#endif /* CONFIG_KERNEL_EVENT_LOGGER */

/**
 * @brief Retrieves a kernel event message, or returns without waiting.
 *
 * This routine retrieves the next recorded event from the kernel event logger,
 * or returns immediately if no such event exists.
 *
 * @param event_id     Area to store event type ID.
 * @param dropped      Area to store number of events that were dropped between
 *                     the previous event and the retrieved event.
 * @param event_data   Buffer to store event data.
 * @param data_size    Size of event data buffer (number of 32-bit words).
 *
 * @retval positive_integer Number of event data words retrieved;
 *         @a event_id, @a dropped, and @a buffer have been updated.
 * @retval 0 Returned without waiting; no event was retrieved.
 * @retval -EMSGSIZE Buffer too small; @a data_size now indicates
 *         the size of the event to be retrieved.
 */
#ifdef CONFIG_KERNEL_EVENT_LOGGER
static inline int sys_k_event_logger_get(uint16_t *event_id, uint8_t *dropped,
				     uint32_t *event_data, uint8_t *data_size)
{
	return sys_event_logger_get(&sys_k_event_logger, event_id, dropped,
				    event_data, data_size);
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER */

/**
 * @brief Retrieves a kernel event message.
 *
 * This routine retrieves the next recorded event from the kernel event logger.
 * If there is no such event the caller pends until it is available.
 *
 * @param event_id     Area to store event type ID.
 * @param dropped      Area to store number of events that were dropped between
 *                     the previous event and the retrieved event.
 * @param event_data   Buffer to store event data.
 * @param data_size    Size of event data buffer (number of 32-bit words).
 *
 * @retval positive_integer Number of event data words retrieved;
 *         @a event_id, @a dropped, and @a buffer have been updated.
 * @retval -EMSGSIZE Buffer too small; @a data_size now indicates
 *         the size of the event to be retrieved.
 */
#ifdef CONFIG_KERNEL_EVENT_LOGGER
static inline int sys_k_event_logger_get_wait(uint16_t *event_id,
		uint8_t *dropped, uint32_t *event_data, uint8_t *data_size)
{
	return sys_event_logger_get_wait(&sys_k_event_logger, event_id, dropped,
					 event_data, data_size);
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER */


/**
 * @brief Retrieves a kernel event message, or waits for a specified time.
 *
 * This routine retrieves the next recorded event from the kernel event logger.
 * If there is no such event the caller pends until it is available or until
 * the specified timeout expires.
 *
 * @param event_id     Area to store event type ID.
 * @param dropped      Area to store number of events that were dropped between
 *                     the previous event and the retrieved event.
 * @param event_data   Buffer to store event data.
 * @param data_size    Size of event data buffer (number of 32-bit words).
 * @param timeout      Timeout in system clock ticks.
 *
 * @retval positive_integer Number of event data words retrieved;
 *         @a event_id, @a dropped, and @a buffer have been updated.
 * @retval 0 Waiting period timed out; no event was retrieved.
 * @retval -EMSGSIZE Buffer too small; @a data_size now indicates
 *         the size of the event to be retrieved.
 */
#if defined(CONFIG_KERNEL_EVENT_LOGGER) && defined(CONFIG_NANO_TIMEOUTS)
static inline int sys_k_event_logger_get_wait_timeout(uint16_t *event_id,
			uint8_t *dropped, uint32_t *event_data,
			uint8_t *data_size, uint32_t timeout)
{
	return sys_event_logger_get_wait_timeout(&sys_k_event_logger, event_id,
						 dropped, event_data,
						 data_size, timeout);
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER && CONFIG_NANO_TIMEOUTS */

/**
 * @brief Register thread that retrieves kernel events.
 *
 * This routine instructs the kernel event logger not to record context
 * switch events for the calling thread. It is typically called by the thread
 * that retrieves events from the kernel event logger.
 *
 * @return N/A
 */
#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
void sys_k_event_logger_register_as_collector(void);
#else
static inline void sys_k_event_logger_register_as_collector(void) {};
#endif

/**
* @} end defgroup kernel_event_logger
*/

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __KERNEL_EVENT_LOGGER_H__ */
