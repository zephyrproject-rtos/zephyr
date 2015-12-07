/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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
 * @brief Event header file.
*/

#ifndef EVENT_H
#define EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Microkernel Events
 * @defgroup microkernel_event Microkernel Events
 * @ingroup microkernel_services
 * @{
 */

#include <microkernel/base_api.h>

/* well-known events */
extern const kevent_t TICK_EVENT;

/**
 *
 * @brief Signal an event from an ISR
 *
 * This routine does NOT validate the specified event number.
 *
 * @param event Event to signal.
 *
 * @return N/A
 */
extern void isr_event_send(kevent_t event);

/**
 *
 * @brief Signal an event from a fiber
 *
 * This routine does NOT validate the specified event number.
 *
 * @param event Event to signal.
 *
 * @return N/A
 */
extern void fiber_event_send(kevent_t event);

/**
 *
 * @brief Set event handler request
 *
 * This routine specifies the event handler that runs (in the context of the
 * microkernel server fiber) when the associated event is signaled. Specifying
 * a non-NULL handler installs a new handler, while specifying a NULL event
 * handler removes the existing event handler.
 *
 * A new event handler cannot be installed if one already exists for that event;
 * the old handler must be removed first. However, it is permitted to replace
 * the NULL event handler with itself.
 *
 * @param event Event upon which to reigster.
 * @param handler Function pointer to handler.
 *
 * @return RC_FAIL if an event handler exists or the event number is invalid,
 *          else RC_OK
 */
extern int task_event_handler_set(kevent_t event, kevent_handler_t handler);

/**
 *
 * @brief Signal an event request
 *
 * This routine signals the specified event from a task. If an event handler
 * is installed for that event, it will run; if no event handler is installed,
 * any task waiting on the event is released.
 *
 * @param event Event to signal.
 *
 * @return RC_FAIL if event number is invalid, else RC_OK
 */
extern int task_event_send(kevent_t event);

/**
 *
 * @brief Test for event request with timeout
 *
 * This routine tests an event to see if it has been signaled.
 *
 * @param event Event for which to test.
 * @param timeout Affects the action taken should the event not yet be
 * signaled. If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then
 * wait as long as necessary. Otherwise wait up to the specified number of
 * ticks before timing out.
 *
 * @retval RC_OK Successfully received signaled event
 * @retval RC_TIME Timed out while waiting for signaled event
 * @retval RC_FAIL Failed to immediately receive signaled event when
 * @a timeout = TICKS_NONE
 */
extern int task_event_recv(kevent_t event, int32_t timeout);

/**
 * @}
 */

#define _K_EVENT_INITIALIZER(handler) \
	{ \
		.status = 0, \
		.func = (kevent_handler_t)handler, \
		.waiter = NULL, \
		.count = 0, \
	}

/**
 * @brief Define a private microkernel event
 *
 * This declares and initializes a private event. The new event
 * can be passed to the microkernel event functions.
 *
 * @param name Name of the event
 * @param handler Function to handle the event (can be NULL)
 */
#define DEFINE_EVENT(name, handler) \
	struct _k_event_struct _k_event_obj_##name \
		__in_section(_k_event_list, event, name) = \
		_K_EVENT_INITIALIZER(handler); \
	const kevent_t name = (kevent_t)&_k_event_obj_##name;

#ifdef __cplusplus
}
#endif

#endif /* EVENT_H */
