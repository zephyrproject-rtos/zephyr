/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Kernel asynchronous event polling interface.
 *
 * This polling mechanism allows waiting on multiple events concurrently,
 * either events triggered directly, or from kernel objects or other kernel
 * constructs.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <wait_q.h>
#include <ksched.h>
#include <misc/slist.h>
#include <misc/dlist.h>
#include <misc/__assert.h>

void k_poll_event_init(struct k_poll_event *event, u32_t type,
		       int mode, void *obj)
{
	__ASSERT(mode == K_POLL_MODE_NOTIFY_ONLY,
		 "only NOTIFY_ONLY mode is supported\n");
	__ASSERT(type < (1 << _POLL_NUM_TYPES), "invalid type\n");
	__ASSERT(obj, "must provide an object\n");

	event->poller = NULL;
	/* event->tag is left uninitialized: the user will set it if needed */
	event->type = type;
	event->state = K_POLL_STATE_NOT_READY;
	event->mode = mode;
	event->unused = 0;
	event->obj = obj;
}

/* must be called with interrupts locked */
static inline void set_polling_state(struct k_thread *thread)
{
	_mark_thread_as_polling(thread);
}

/* must be called with interrupts locked */
static inline void clear_polling_state(struct k_thread *thread)
{
	_mark_thread_as_not_polling(thread);
}

/* must be called with interrupts locked */
static inline int is_polling(void)
{
	return _is_thread_polling(_current);
}

/* must be called with interrupts locked */
static inline int is_condition_met(struct k_poll_event *event, u32_t *state)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		if (k_sem_count_get(event->sem) > 0) {
			*state = K_POLL_STATE_SEM_AVAILABLE;
			return 1;
		}
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		if (!k_queue_is_empty(event->queue)) {
			*state = K_POLL_STATE_FIFO_DATA_AVAILABLE;
			return 1;
		}
		break;
	case K_POLL_TYPE_SIGNAL:
		if (event->signal->signaled) {
			*state = K_POLL_STATE_SIGNALED;
			return 1;
		}
		break;
	case K_POLL_TYPE_IGNORE:
		return 0;
	default:
		__ASSERT(0, "invalid event type (0x%x)\n", event->type);
		break;
	}

	return 0;
}

static inline void add_event(sys_dlist_t *events, struct k_poll_event *event,
			     struct _poller *poller)
{
	struct k_poll_event *pending;

	pending = (struct k_poll_event *)sys_dlist_peek_tail(events);
	if (!pending || _is_t1_higher_prio_than_t2(pending->poller->thread,
						   poller->thread)) {
		sys_dlist_append(events, &event->_node);
		return;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(events, pending, _node) {
		if (_is_t1_higher_prio_than_t2(poller->thread,
					       pending->poller->thread)) {
			sys_dlist_insert_before(events, &pending->_node,
						&event->_node);
			return;
		}
	}

	sys_dlist_append(events, &event->_node);
}

/* must be called with interrupts locked */
static inline int register_event(struct k_poll_event *event,
				 struct _poller *poller)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		__ASSERT(event->sem, "invalid semaphore\n");
		add_event(&event->sem->poll_events, event, poller);
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT(event->queue, "invalid queue\n");
		add_event(&event->queue->poll_events, event, poller);
		break;
	case K_POLL_TYPE_SIGNAL:
		__ASSERT(event->signal, "invalid poll signal\n");
		add_event(&event->signal->poll_events, event, poller);
		break;
	case K_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT(0, "invalid event type\n");
		break;
	}

	event->poller = poller;

	return 0;
}

/* must be called with interrupts locked */
static inline void clear_event_registration(struct k_poll_event *event)
{
	event->poller = NULL;

	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		__ASSERT(event->sem, "invalid semaphore\n");
		sys_dlist_remove(&event->_node);
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT(event->queue, "invalid queue\n");
		sys_dlist_remove(&event->_node);
		break;
	case K_POLL_TYPE_SIGNAL:
		__ASSERT(event->signal, "invalid poll signal\n");
		sys_dlist_remove(&event->_node);
		break;
	case K_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT(0, "invalid event type\n");
		break;
	}
}

/* must be called with interrupts locked */
static inline void clear_event_registrations(struct k_poll_event *events,
					      int last_registered,
					      unsigned int key)
{
	for (; last_registered >= 0; last_registered--) {
		clear_event_registration(&events[last_registered]);
		irq_unlock(key);
		key = irq_lock();
	}
}

static inline void set_event_ready(struct k_poll_event *event, u32_t state)
{
	event->poller = NULL;
	event->state |= state;
}

int k_poll(struct k_poll_event *events, int num_events, s32_t timeout)
{
	__ASSERT(!_is_in_isr(), "");
	__ASSERT(events, "NULL events\n");
	__ASSERT(num_events > 0, "zero events\n");

	int last_registered = -1, rc;
	unsigned int key;

	key = irq_lock();
	set_polling_state(_current);
	irq_unlock(key);

	struct _poller poller = { .thread = _current };

	/* find events whose condition is already fulfilled */
	for (int ii = 0; ii < num_events; ii++) {
		u32_t state;

		key = irq_lock();
		if (is_condition_met(&events[ii], &state)) {
			set_event_ready(&events[ii], state);
			clear_polling_state(_current);
		} else if (timeout != K_NO_WAIT && is_polling()) {
			rc = register_event(&events[ii], &poller);
			if (rc == 0) {
				++last_registered;
			} else {
				__ASSERT(0, "unexpected return code\n");
			}
		}
		irq_unlock(key);
	}

	key = irq_lock();

	/*
	 * If we're not polling anymore, it means that at least one event
	 * condition is met, either when looping through the events here or
	 * because one of the events registered has had its state changed.
	 */
	if (!is_polling()) {
		clear_event_registrations(events, last_registered, key);
		irq_unlock(key);
		return 0;
	}

	clear_polling_state(_current);

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return -EAGAIN;
	}

	_wait_q_t wait_q = _WAIT_Q_INIT(&wait_q);

	_pend_current_thread(&wait_q, timeout);

	int swap_rc = _Swap(key);

	/*
	 * Clear all event registrations. If events happen while we're in this
	 * loop, and we already had one that triggered, that's OK: they will
	 * end up in the list of events that are ready; if we timed out, and
	 * events happen while we're in this loop, that is OK as well since
	 * we've already know the return code (-EAGAIN), and even if they are
	 * added to the list of events that occurred, the user has to check the
	 * return code first, which invalidates the whole list of event states.
	 */
	key = irq_lock();
	clear_event_registrations(events, last_registered, key);
	irq_unlock(key);

	return swap_rc;
}

/* must be called with interrupts locked */
static int _signal_poll_event(struct k_poll_event *event, u32_t state,
			      int *must_reschedule)
{
	*must_reschedule = 0;

	if (!event->poller) {
		goto ready_event;
	}

	struct k_thread *thread = event->poller->thread;

	__ASSERT(event->poller->thread, "poller should have a thread\n");

	clear_polling_state(thread);

	if (!_is_thread_pending(thread)) {
		goto ready_event;
	}

	if (_is_thread_timeout_expired(thread)) {
		return -EAGAIN;
	}

	_unpend_thread(thread);
	_abort_thread_timeout(thread);
	_set_thread_return_value(thread,
				 state == K_POLL_STATE_NOT_READY ? -EINTR : 0);

	if (!_is_thread_ready(thread)) {
		goto ready_event;
	}

	_add_thread_to_ready_q(thread);
	*must_reschedule = !_is_in_isr() && _must_switch_threads();

ready_event:
	set_event_ready(event, state);
	return 0;
}

/* returns 1 if a reschedule must take place, 0 otherwise */
int _handle_obj_poll_events(sys_dlist_t *events, u32_t state)
{
	struct k_poll_event *poll_event;
	int must_reschedule;

	poll_event = (struct k_poll_event *)sys_dlist_get(events);
	if (!poll_event) {
		return 0;
	}

	(void)_signal_poll_event(poll_event, state, &must_reschedule);
	return must_reschedule;
}

void k_poll_signal_init(struct k_poll_signal *signal)
{
	sys_dlist_init(&signal->poll_events);
	signal->signaled = 0;
	/* signal->result is left unitialized */
}

int k_poll_signal(struct k_poll_signal *signal, int result)
{
	unsigned int key = irq_lock();
	struct k_poll_event *poll_event;
	int must_reschedule;

	signal->result = result;
	signal->signaled = 1;

	poll_event = (struct k_poll_event *)sys_dlist_get(&signal->poll_events);
	if (!poll_event) {
		irq_unlock(key);
		return 0;
	}

	int rc = _signal_poll_event(poll_event, K_POLL_STATE_SIGNALED,
				    &must_reschedule);

	if (must_reschedule) {
		(void)_Swap(key);
	} else {
		irq_unlock(key);
	}

	return rc;
}
