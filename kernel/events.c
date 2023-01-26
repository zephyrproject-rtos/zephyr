/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file event objects library
 *
 * Event objects are used to signal one or more threads that a custom set of
 * events has occurred. Threads wait on event objects until another thread or
 * ISR posts the desired set of events to the event object. Each time events
 * are posted to an event object, all threads waiting on that event object are
 * processed to determine if there is a match. All threads that whose wait
 * conditions match the current set of events now belonging to the event object
 * are awakened.
 *
 * Threads waiting on an event object have the option of either waking once
 * any or all of the events it desires have been posted to the event object.
 *
 * @brief Kernel event object
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#include <zephyr/toolchain.h>
#include <zephyr/wait_q.h>
#include <zephyr/sys/dlist.h>
#include <ksched.h>
#include <zephyr/init.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/sys/check.h>

#define K_EVENT_WAIT_ANY      0x00   /* Wait for any events */
#define K_EVENT_WAIT_ALL      0x01   /* Wait for all events */
#define K_EVENT_WAIT_MASK     0x01

#define K_EVENT_WAIT_RESET    0x02   /* Reset events prior to waiting */

void z_impl_k_event_init(struct k_event *event)
{
	event->events = 0;
	event->lock = (struct k_spinlock) {};

	SYS_PORT_TRACING_OBJ_INIT(k_event, event);

	z_waitq_init(&event->wait_q);

	z_object_init(event);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_event_init(struct k_event *event)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(event, K_OBJ_EVENT));
	z_impl_k_event_init(event);
}
#include <syscalls/k_event_init_mrsh.c>
#endif

/**
 * @brief determine if desired set of events been satisfied
 *
 * This routine determines if the current set of events satisfies the desired
 * set of events. If @a wait_condition is K_EVENT_WAIT_ALL, then at least
 * all the desired events must be present to satisfy the request. If @a
 * wait_condition is not K_EVENT_WAIT_ALL, it is assumed to be K_EVENT_WAIT_ANY.
 * In the K_EVENT_WAIT_ANY case, the request is satisfied when any of the
 * current set of events are present in the desired set of events.
 */
static bool are_wait_conditions_met(uint32_t desired, uint32_t current,
				    unsigned int wait_condition)
{
	uint32_t  match = current & desired;

	if (wait_condition == K_EVENT_WAIT_ALL) {
		return match == desired;
	}

	/* wait_condition assumed to be K_EVENT_WAIT_ANY */

	return match != 0;
}

static void k_event_post_internal(struct k_event *event, uint32_t events,
				  uint32_t events_mask)
{
	k_spinlock_key_t  key;
	struct k_thread  *thread;
	unsigned int      wait_condition;
	struct k_thread  *head = NULL;

	key = k_spin_lock(&event->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_event, post, event, events,
					events_mask);

	events = (event->events & ~events_mask) |
		 (events & events_mask);
	event->events = events;

	/*
	 * Posting an event has the potential to wake multiple pended threads.
	 * It is desirable to unpend all affected threads simultaneously. To
	 * do so, this must be done in three steps as it is unsafe to unpend
	 * threads from within the _WAIT_Q_FOR_EACH() loop.
	 *
	 * 1. Create a linked list of threads to unpend.
	 * 2. Unpend each of the threads in the linked list
	 * 3. Ready each of the threads in the linked list
	 */

	_WAIT_Q_FOR_EACH(&event->wait_q, thread) {
		wait_condition = thread->event_options & K_EVENT_WAIT_MASK;

		if (are_wait_conditions_met(thread->events, events,
					    wait_condition)) {
			/*
			 * The wait conditions have been satisfied. Add this
			 * thread to the list of threads to unpend.
			 */

			thread->next_event_link = head;
			head = thread;
		}
	}

	if (head != NULL) {
		thread = head;
		do {
			z_unpend_thread(thread);
			arch_thread_return_value_set(thread, 0);
			thread->events = events;
			z_ready_thread(thread);
			thread = thread->next_event_link;
		} while (thread != NULL);
	}

	z_reschedule(&event->lock, key);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_event, post, event, events,
				       events_mask);
}

void z_impl_k_event_post(struct k_event *event, uint32_t events)
{
	k_event_post_internal(event, events, events);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_event_post(struct k_event *event, uint32_t events)
{
	Z_OOPS(Z_SYSCALL_OBJ(event, K_OBJ_EVENT));
	z_impl_k_event_post(event, events);
}
#include <syscalls/k_event_post_mrsh.c>
#endif

void z_impl_k_event_set(struct k_event *event, uint32_t events)
{
	k_event_post_internal(event, events, ~0);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_event_set(struct k_event *event, uint32_t events)
{
	Z_OOPS(Z_SYSCALL_OBJ(event, K_OBJ_EVENT));
	z_impl_k_event_set(event, events);
}
#include <syscalls/k_event_set_mrsh.c>
#endif

void z_impl_k_event_set_masked(struct k_event *event, uint32_t events,
			       uint32_t events_mask)
{
	k_event_post_internal(event, events, events_mask);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_event_set_masked(struct k_event *event, uint32_t events,
			       uint32_t events_mask)
{
	Z_OOPS(Z_SYSCALL_OBJ(event, K_OBJ_EVENT));
	z_impl_k_event_set_masked(event, events, events_mask);
}
#include <syscalls/k_event_set_masked_mrsh.c>
#endif

void z_impl_k_event_clear(struct k_event *event, uint32_t events)
{
	k_event_post_internal(event, 0, events);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_event_clear(struct k_event *event, uint32_t events)
{
	Z_OOPS(Z_SYSCALL_OBJ(event, K_OBJ_EVENT));
	z_impl_k_event_clear(event, events);
}
#include <syscalls/k_event_clear_mrsh.c>
#endif

static uint32_t k_event_wait_internal(struct k_event *event, uint32_t events,
				      unsigned int options, k_timeout_t timeout)
{
	uint32_t  rv = 0;
	unsigned int  wait_condition;
	struct k_thread  *thread;

	__ASSERT(((arch_is_in_isr() == false) ||
		  K_TIMEOUT_EQ(timeout, K_NO_WAIT)), "");

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_event, wait, event, events,
					options, timeout);

	if (events == 0) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_event, wait, event, events, 0);
		return 0;
	}

	wait_condition = options & K_EVENT_WAIT_MASK;
	thread = z_current_get();

	k_spinlock_key_t  key = k_spin_lock(&event->lock);

	if (options & K_EVENT_WAIT_RESET) {
		event->events = 0;
	}

	/* Test if the wait conditions have already been met. */

	if (are_wait_conditions_met(events, event->events, wait_condition)) {
		rv = event->events;

		k_spin_unlock(&event->lock, key);
		goto out;
	}

	/* Match conditions have not been met. */

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&event->lock, key);
		goto out;
	}

	/*
	 * The caller must pend to wait for the match. Save the desired
	 * set of events in the k_thread structure.
	 */

	thread->events = events;
	thread->event_options = options;

	SYS_PORT_TRACING_OBJ_FUNC_BLOCKING(k_event, wait, event, events,
					   options, timeout);

	if (z_pend_curr(&event->lock, key, &event->wait_q, timeout) == 0) {
		/* Retrieve the set of events that woke the thread */
		rv = thread->events;
	}

out:
	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_event, wait, event,
				       events, rv & events);

	return rv & events;
}

/**
 * Wait for any of the specified events
 */
uint32_t z_impl_k_event_wait(struct k_event *event, uint32_t events,
			     bool reset, k_timeout_t timeout)
{
	uint32_t options = reset ? K_EVENT_WAIT_RESET : 0;

	return k_event_wait_internal(event, events, options, timeout);
}
#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_k_event_wait(struct k_event *event, uint32_t events,
				    bool reset, k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait(event, events, reset, timeout);
}
#include <syscalls/k_event_wait_mrsh.c>
#endif

/**
 * Wait for all of the specified events
 */
uint32_t z_impl_k_event_wait_all(struct k_event *event, uint32_t events,
				 bool reset, k_timeout_t timeout)
{
	uint32_t options = reset ? (K_EVENT_WAIT_RESET | K_EVENT_WAIT_ALL)
				 : K_EVENT_WAIT_ALL;

	return k_event_wait_internal(event, events, options, timeout);
}

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_k_event_wait_all(struct k_event *event, uint32_t events,
					bool reset, k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait_all(event, events, reset, timeout);
}
#include <syscalls/k_event_wait_all_mrsh.c>
#endif
