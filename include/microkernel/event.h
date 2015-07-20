/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Event header file.
*/

#ifndef EVENT_H
#define EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/command_packet.h>

/* well-known events */

#define TICK_EVENT	0

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
 * K_swapper fiber) when the associated event is signaled. Specifying a non-NULL
 * handler installs a new handler, while specifying a NULL event handler removes
 * the existing event handler.
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
extern int task_event_set_handler(kevent_t event, kevent_handler_t handler);

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
 * @brief Test for event request
 *
 * This routine tests an event to see if it has been signaled.
 *
 * @param event Event for which to test.
 * @param time Maximum number of ticks to wait for event.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
extern int _task_event_recv(kevent_t event, int32_t time);

/**
 *
 * @brief Test for event request
 *
 * This routine tests an event to see if it has been signaled.
 *
 * @param event Event for which to test.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_event_recv(event) _task_event_recv(event, TICKS_NONE)

/**
 *
 * @brief Test for event request with wait
 *
 * This routine tests an event to see if it has been signaled.
 *
 * @param event Event for which to test.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_event_recv_wait(event) _task_event_recv(event, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 *
 * @brief Test for event request with time out
 *
 * This routine tests an event to see if it has been signaled.
 *
 * @param event Event for which to test.
 * @param time Maximum number of ticks to wait for event.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_event_recv_wait_timeout(event, time) _task_event_recv(event, time)
#endif /* CONFIG_SYS_CLOCK_EXISTS */

#ifdef __cplusplus
}
#endif

#endif /* EVENT_H */
