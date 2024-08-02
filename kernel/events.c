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
#include <zephyr/sys/dlist.h>
#include <zephyr/init.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/sys/check.h>
/* private kernel APIs */
#include <wait_q.h>
#include <ksched.h>

#define K_EVENT_WAIT_ANY      0x00   /* Wait for any events */
#define K_EVENT_WAIT_ALL      0x01   /* Wait for all events */
#define K_EVENT_WAIT_MASK     0x01

#define K_EVENT_WAIT_RESET    0x02   /* Reset events prior to waiting */

struct event_walk_data {
	struct k_thread  *head;
	uint32_t events;
};

#ifdef CONFIG_OBJ_CORE_EVENT
static struct k_obj_type obj_type_event;
#endif /* CONFIG_OBJ_CORE_EVENT */

void z_impl_k_event_init(struct k_event *event)
{
	event->events = 0;
	event->lock = (struct k_spinlock) {};

	SYS_PORT_TRACING_OBJ_INIT(k_event, event);

	z_waitq_init(&event->wait_q);

	k_object_init(event);

#ifdef CONFIG_OBJ_CORE_EVENT
	k_obj_core_init_and_link(K_OBJ_CORE(event), &obj_type_event);
#endif /* CONFIG_OBJ_CORE_EVENT */
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_event_init(struct k_event *event)
{
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(event, K_OBJ_EVENT));
	z_impl_k_event_init(event);
}
#include <zephyr/syscalls/k_event_init_mrsh.c>
#endif /* CONFIG_USERSPACE */

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

static int event_walk_op(struct k_thread *thread, void *data)
{
	unsigned int      wait_condition;
	struct event_walk_data *event_data = data;

	wait_condition = thread->event_options & K_EVENT_WAIT_MASK;

	if (are_wait_conditions_met(thread->events, event_data->events,
				    wait_condition)) {

		/*
		 * Events create a list of threads to wake up. We do
		 * not want z_thread_timeout to wake these threads; they
		 * will be woken up by k_event_post_internal once they
		 * have been processed.
		 */
		thread->no_wake_on_timeout = true;

		/*
		 * The wait conditions have been satisfied. Add this
		 * thread to the list of threads to unpend.
		 */
		thread->next_event_link = event_data->head;
		event_data->head = thread;
		z_abort_timeout(&thread->base.timeout);
	}

	return 0;
}

static uint32_t k_event_post_internal(struct k_event *event, uint32_t events,
				  uint32_t events_mask)
{
	k_spinlock_key_t  key;
	struct k_thread  *thread;
	struct event_walk_data data;
	uint32_t previous_events;

	data.head = NULL;
	key = k_spin_lock(&event->lock);

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_event, post, event, events,
					events_mask);

	previous_events = event->events & events_mask;
	events = (event->events & ~events_mask) |
		 (events & events_mask);
	event->events = events;
	data.events = events;
	/*
	 * Posting an event has the potential to wake multiple pended threads.
	 * It is desirable to unpend all affected threads simultaneously. This
	 * is done in three steps:
	 *
	 * 1. Walk the waitq and create a linked list of threads to unpend.
	 * 2. Unpend each of the threads in the linked list
	 * 3. Ready each of the threads in the linked list
	 */

	z_sched_waitq_walk(&event->wait_q, event_walk_op, &data);

	if (data.head != NULL) {
		thread = data.head;
		struct k_thread *next;
		do {
			arch_thread_return_value_set(thread, 0);
			thread->events = events;
			next = thread->next_event_link;
			z_sched_wake_thread(thread, false);
			thread = next;
		} while (thread != NULL);
	}

	z_reschedule(&event->lock, key);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_event, post, event, events,
				       events_mask);

	return previous_events;
}

uint32_t z_impl_k_event_post(struct k_event *event, uint32_t events)
{
	return k_event_post_internal(event, events, events);
}

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_k_event_post(struct k_event *event, uint32_t events)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_post(event, events);
}
#include <zephyr/syscalls/k_event_post_mrsh.c>
#endif /* CONFIG_USERSPACE */

uint32_t z_impl_k_event_set(struct k_event *event, uint32_t events)
{
	return k_event_post_internal(event, events, ~0);
}

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_k_event_set(struct k_event *event, uint32_t events)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_set(event, events);
}
#include <zephyr/syscalls/k_event_set_mrsh.c>
#endif /* CONFIG_USERSPACE */

uint32_t z_impl_k_event_set_masked(struct k_event *event, uint32_t events,
			       uint32_t events_mask)
{
	return k_event_post_internal(event, events, events_mask);
}

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_k_event_set_masked(struct k_event *event, uint32_t events,
			       uint32_t events_mask)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_set_masked(event, events, events_mask);
}
#include <zephyr/syscalls/k_event_set_masked_mrsh.c>
#endif /* CONFIG_USERSPACE */

uint32_t z_impl_k_event_clear(struct k_event *event, uint32_t events)
{
	return k_event_post_internal(event, 0, events);
}

#ifdef CONFIG_USERSPACE
uint32_t z_vrfy_k_event_clear(struct k_event *event, uint32_t events)
{
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_clear(event, events);
}
#include <zephyr/syscalls/k_event_clear_mrsh.c>
#endif /* CONFIG_USERSPACE */

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
	thread = k_sched_current_thread_query();

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
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait(event, events, reset, timeout);
}
#include <zephyr/syscalls/k_event_wait_mrsh.c>
#endif /* CONFIG_USERSPACE */

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
	K_OOPS(K_SYSCALL_OBJ(event, K_OBJ_EVENT));
	return z_impl_k_event_wait_all(event, events, reset, timeout);
}
#include <zephyr/syscalls/k_event_wait_all_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_OBJ_CORE_EVENT
static int init_event_obj_core_list(void)
{
	/* Initialize condvar object type */

	z_obj_type_init(&obj_type_event, K_OBJ_TYPE_EVENT_ID,
			offsetof(struct k_event, obj_core));

	/* Initialize and link statically defined condvars */

	STRUCT_SECTION_FOREACH(k_event, event) {
		k_obj_core_init_and_link(K_OBJ_CORE(event), &obj_type_event);
	}

	return 0;
}

SYS_INIT(init_event_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif /* CONFIG_OBJ_CORE_EVENT */
