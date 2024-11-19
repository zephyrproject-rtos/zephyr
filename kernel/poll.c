/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
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

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <wait_q.h>
#include <ksched.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <stdbool.h>

/* Single subsystem lock.  Locking per-event would be better on highly
 * contended SMP systems, but the original locking scheme here is
 * subtle (it relies on releasing/reacquiring the lock in areas for
 * latency control and it's sometimes hard to see exactly what data is
 * "inside" a given critical section).  Do the synchronization port
 * later as an optimization.
 */
static struct k_spinlock lock;

enum POLL_MODE { MODE_NONE, MODE_POLL, MODE_TRIGGERED };

static int signal_poller(struct k_poll_event *event, uint32_t state);
static int signal_triggered_work(struct k_poll_event *event, uint32_t status);

void k_poll_event_init(struct k_poll_event *event, uint32_t type,
		       int mode, void *obj)
{
	__ASSERT(mode == K_POLL_MODE_NOTIFY_ONLY,
		 "only NOTIFY_ONLY mode is supported\n");
	__ASSERT(type < (BIT(_POLL_NUM_TYPES)), "invalid type\n");
	__ASSERT(obj != NULL, "must provide an object\n");

	event->poller = NULL;
	/* event->tag is left uninitialized: the user will set it if needed */
	event->type = type;
	event->state = K_POLL_STATE_NOT_READY;
	event->mode = mode;
	event->unused = 0U;
	event->obj = obj;

	SYS_PORT_TRACING_FUNC(k_poll_api, event_init, event);
}

/* must be called with interrupts locked */
static inline bool is_condition_met(struct k_poll_event *event, uint32_t *state)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		if (k_sem_count_get(event->sem) > 0U) {
			*state = K_POLL_STATE_SEM_AVAILABLE;
			return true;
		}
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		if (!k_queue_is_empty(event->queue)) {
			*state = K_POLL_STATE_FIFO_DATA_AVAILABLE;
			return true;
		}
		break;
	case K_POLL_TYPE_SIGNAL:
		if (event->signal->signaled != 0U) {
			*state = K_POLL_STATE_SIGNALED;
			return true;
		}
		break;
	case K_POLL_TYPE_MSGQ_DATA_AVAILABLE:
		if (event->msgq->used_msgs > 0) {
			*state = K_POLL_STATE_MSGQ_DATA_AVAILABLE;
			return true;
		}
		break;
#ifdef CONFIG_PIPES
	case K_POLL_TYPE_PIPE_DATA_AVAILABLE:
		if (k_pipe_read_avail(event->pipe)) {
			*state = K_POLL_STATE_PIPE_DATA_AVAILABLE;
			return true;
		}
#endif /* CONFIG_PIPES */
	case K_POLL_TYPE_IGNORE:
		break;
	default:
		__ASSERT(false, "invalid event type (0x%x)\n", event->type);
		break;
	}

	return false;
}

static struct k_thread *poller_thread(struct z_poller *p)
{
	return p ? CONTAINER_OF(p, struct k_thread, poller) : NULL;
}

static inline void add_event(sys_dlist_t *events, struct k_poll_event *event,
			     struct z_poller *poller)
{
	struct k_poll_event *pending;

	pending = (struct k_poll_event *)sys_dlist_peek_tail(events);
	if ((pending == NULL) ||
		(z_sched_prio_cmp(poller_thread(pending->poller),
							   poller_thread(poller)) > 0)) {
		sys_dlist_append(events, &event->_node);
		return;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(events, pending, _node) {
		if (z_sched_prio_cmp(poller_thread(poller),
					poller_thread(pending->poller)) > 0) {
			sys_dlist_insert(&pending->_node, &event->_node);
			return;
		}
	}

	sys_dlist_append(events, &event->_node);
}

/* must be called with interrupts locked */
static inline void register_event(struct k_poll_event *event,
				 struct z_poller *poller)
{
	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		__ASSERT(event->sem != NULL, "invalid semaphore\n");
		add_event(&event->sem->poll_events, event, poller);
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT(event->queue != NULL, "invalid queue\n");
		add_event(&event->queue->poll_events, event, poller);
		break;
	case K_POLL_TYPE_SIGNAL:
		__ASSERT(event->signal != NULL, "invalid poll signal\n");
		add_event(&event->signal->poll_events, event, poller);
		break;
	case K_POLL_TYPE_MSGQ_DATA_AVAILABLE:
		__ASSERT(event->msgq != NULL, "invalid message queue\n");
		add_event(&event->msgq->poll_events, event, poller);
		break;
#ifdef CONFIG_PIPES
	case K_POLL_TYPE_PIPE_DATA_AVAILABLE:
		__ASSERT(event->pipe != NULL, "invalid pipe\n");
		add_event(&event->pipe->poll_events, event, poller);
		break;
#endif /* CONFIG_PIPES */
	case K_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT(false, "invalid event type\n");
		break;
	}

	event->poller = poller;
}

/* must be called with interrupts locked */
static inline void clear_event_registration(struct k_poll_event *event)
{
	bool remove_event = false;

	event->poller = NULL;

	switch (event->type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		__ASSERT(event->sem != NULL, "invalid semaphore\n");
		remove_event = true;
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		__ASSERT(event->queue != NULL, "invalid queue\n");
		remove_event = true;
		break;
	case K_POLL_TYPE_SIGNAL:
		__ASSERT(event->signal != NULL, "invalid poll signal\n");
		remove_event = true;
		break;
	case K_POLL_TYPE_MSGQ_DATA_AVAILABLE:
		__ASSERT(event->msgq != NULL, "invalid message queue\n");
		remove_event = true;
		break;
#ifdef CONFIG_PIPES
	case K_POLL_TYPE_PIPE_DATA_AVAILABLE:
		__ASSERT(event->pipe != NULL, "invalid pipe\n");
		remove_event = true;
		break;
#endif /* CONFIG_PIPES */
	case K_POLL_TYPE_IGNORE:
		/* nothing to do */
		break;
	default:
		__ASSERT(false, "invalid event type\n");
		break;
	}
	if (remove_event && sys_dnode_is_linked(&event->_node)) {
		sys_dlist_remove(&event->_node);
	}
}

/* must be called with interrupts locked */
static inline void clear_event_registrations(struct k_poll_event *events,
					      int num_events,
					      k_spinlock_key_t key)
{
	while (num_events--) {
		clear_event_registration(&events[num_events]);
		k_spin_unlock(&lock, key);
		key = k_spin_lock(&lock);
	}
}

static inline void set_event_ready(struct k_poll_event *event, uint32_t state)
{
	event->poller = NULL;
	event->state |= state;
}

static inline int register_events(struct k_poll_event *events,
				  int num_events,
				  struct z_poller *poller,
				  bool just_check)
{
	int events_registered = 0;

	for (int ii = 0; ii < num_events; ii++) {
		k_spinlock_key_t key;
		uint32_t state;

		key = k_spin_lock(&lock);
		if (is_condition_met(&events[ii], &state)) {
			set_event_ready(&events[ii], state);
			poller->is_polling = false;
		} else if (!just_check && poller->is_polling) {
			register_event(&events[ii], poller);
			events_registered += 1;
		} else {
			/* Event is not one of those identified in is_condition_met()
			 * catching non-polling events, or is marked for just check,
			 * or not marked for polling. No action needed.
			 */
			;
		}
		k_spin_unlock(&lock, key);
	}

	return events_registered;
}

static int signal_poller(struct k_poll_event *event, uint32_t state)
{
	struct k_thread *thread = poller_thread(event->poller);

	__ASSERT(thread != NULL, "poller should have a thread\n");

	if (!z_is_thread_pending(thread)) {
		return 0;
	}

	z_unpend_thread(thread);
	arch_thread_return_value_set(thread,
		state == K_POLL_STATE_CANCELLED ? -EINTR : 0);

	if (!z_is_thread_ready(thread)) {
		return 0;
	}

	z_ready_thread(thread);

	return 0;
}

int z_impl_k_poll(struct k_poll_event *events, int num_events,
		  k_timeout_t timeout)
{
	int events_registered;
	k_spinlock_key_t key;
	struct z_poller *poller = &arch_current_thread()->poller;

	poller->is_polling = true;
	poller->mode = MODE_POLL;

	__ASSERT(!arch_is_in_isr(), "");
	__ASSERT(events != NULL, "NULL events\n");
	__ASSERT(num_events >= 0, "<0 events\n");

	SYS_PORT_TRACING_FUNC_ENTER(k_poll_api, poll, events);

	events_registered = register_events(events, num_events, poller,
					    K_TIMEOUT_EQ(timeout, K_NO_WAIT));

	key = k_spin_lock(&lock);

	/*
	 * If we're not polling anymore, it means that at least one event
	 * condition is met, either when looping through the events here or
	 * because one of the events registered has had its state changed.
	 */
	if (!poller->is_polling) {
		clear_event_registrations(events, events_registered, key);
		k_spin_unlock(&lock, key);

		SYS_PORT_TRACING_FUNC_EXIT(k_poll_api, poll, events, 0);

		return 0;
	}

	poller->is_polling = false;

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&lock, key);

		SYS_PORT_TRACING_FUNC_EXIT(k_poll_api, poll, events, -EAGAIN);

		return -EAGAIN;
	}

	static _wait_q_t wait_q = Z_WAIT_Q_INIT(&wait_q);

	int swap_rc = z_pend_curr(&lock, key, &wait_q, timeout);

	/*
	 * Clear all event registrations. If events happen while we're in this
	 * loop, and we already had one that triggered, that's OK: they will
	 * end up in the list of events that are ready; if we timed out, and
	 * events happen while we're in this loop, that is OK as well since
	 * we've already know the return code (-EAGAIN), and even if they are
	 * added to the list of events that occurred, the user has to check the
	 * return code first, which invalidates the whole list of event states.
	 */
	key = k_spin_lock(&lock);
	clear_event_registrations(events, events_registered, key);
	k_spin_unlock(&lock, key);

	SYS_PORT_TRACING_FUNC_EXIT(k_poll_api, poll, events, swap_rc);

	return swap_rc;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_poll(struct k_poll_event *events,
				int num_events, k_timeout_t timeout)
{
	int ret;
	k_spinlock_key_t key;
	struct k_poll_event *events_copy = NULL;
	uint32_t bounds;

	/* Validate the events buffer and make a copy of it in an
	 * allocated kernel-side buffer.
	 */
	if (K_SYSCALL_VERIFY(num_events >= 0)) {
		ret = -EINVAL;
		goto out;
	}
	if (K_SYSCALL_VERIFY_MSG(!u32_mul_overflow(num_events,
						   sizeof(struct k_poll_event),
						   &bounds),
				 "num_events too large")) {
		ret = -EINVAL;
		goto out;
	}
	events_copy = z_thread_malloc(bounds);
	if (!events_copy) {
		ret = -ENOMEM;
		goto out;
	}

	key = k_spin_lock(&lock);
	if (K_SYSCALL_MEMORY_WRITE(events, bounds)) {
		k_spin_unlock(&lock, key);
		goto oops_free;
	}
	(void)memcpy(events_copy, events, bounds);
	k_spin_unlock(&lock, key);

	/* Validate what's inside events_copy */
	for (int i = 0; i < num_events; i++) {
		struct k_poll_event *e = &events_copy[i];

		if (K_SYSCALL_VERIFY(e->mode == K_POLL_MODE_NOTIFY_ONLY)) {
			ret = -EINVAL;
			goto out_free;
		}

		switch (e->type) {
		case K_POLL_TYPE_IGNORE:
			break;
		case K_POLL_TYPE_SIGNAL:
			K_OOPS(K_SYSCALL_OBJ(e->signal, K_OBJ_POLL_SIGNAL));
			break;
		case K_POLL_TYPE_SEM_AVAILABLE:
			K_OOPS(K_SYSCALL_OBJ(e->sem, K_OBJ_SEM));
			break;
		case K_POLL_TYPE_DATA_AVAILABLE:
			K_OOPS(K_SYSCALL_OBJ(e->queue, K_OBJ_QUEUE));
			break;
		case K_POLL_TYPE_MSGQ_DATA_AVAILABLE:
			K_OOPS(K_SYSCALL_OBJ(e->msgq, K_OBJ_MSGQ));
			break;
#ifdef CONFIG_PIPES
		case K_POLL_TYPE_PIPE_DATA_AVAILABLE:
			K_OOPS(K_SYSCALL_OBJ(e->pipe, K_OBJ_PIPE));
			break;
#endif /* CONFIG_PIPES */
		default:
			ret = -EINVAL;
			goto out_free;
		}
	}

	ret = k_poll(events_copy, num_events, timeout);
	(void)memcpy((void *)events, events_copy, bounds);
out_free:
	k_free(events_copy);
out:
	return ret;
oops_free:
	k_free(events_copy);
	K_OOPS(1);
}
#include <zephyr/syscalls/k_poll_mrsh.c>
#endif /* CONFIG_USERSPACE */

/* must be called with interrupts locked */
static int signal_poll_event(struct k_poll_event *event, uint32_t state)
{
	struct z_poller *poller = event->poller;
	int retcode = 0;

	if (poller != NULL) {
		if (poller->mode == MODE_POLL) {
			retcode = signal_poller(event, state);
		} else if (poller->mode == MODE_TRIGGERED) {
			retcode = signal_triggered_work(event, state);
		} else {
			/* Poller is not poll or triggered mode. No action needed.*/
			;
		}

		poller->is_polling = false;

		if (retcode < 0) {
			return retcode;
		}
	}

	set_event_ready(event, state);
	return retcode;
}

void z_handle_obj_poll_events(sys_dlist_t *events, uint32_t state)
{
	struct k_poll_event *poll_event;
	k_spinlock_key_t key = k_spin_lock(&lock);

	poll_event = (struct k_poll_event *)sys_dlist_get(events);
	if (poll_event != NULL) {
		(void) signal_poll_event(poll_event, state);
	}

	k_spin_unlock(&lock, key);
}

void z_impl_k_poll_signal_init(struct k_poll_signal *sig)
{
	sys_dlist_init(&sig->poll_events);
	sig->signaled = 0U;
	/* signal->result is left uninitialized */
	k_object_init(sig);

	SYS_PORT_TRACING_FUNC(k_poll_api, signal_init, sig);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_poll_signal_init(struct k_poll_signal *sig)
{
	K_OOPS(K_SYSCALL_OBJ_INIT(sig, K_OBJ_POLL_SIGNAL));
	z_impl_k_poll_signal_init(sig);
}
#include <zephyr/syscalls/k_poll_signal_init_mrsh.c>
#endif /* CONFIG_USERSPACE */

void z_impl_k_poll_signal_reset(struct k_poll_signal *sig)
{
	sig->signaled = 0U;

	SYS_PORT_TRACING_FUNC(k_poll_api, signal_reset, sig);
}

void z_impl_k_poll_signal_check(struct k_poll_signal *sig,
			       unsigned int *signaled, int *result)
{
	*signaled = sig->signaled;
	*result = sig->result;

	SYS_PORT_TRACING_FUNC(k_poll_api, signal_check, sig);
}

#ifdef CONFIG_USERSPACE
void z_vrfy_k_poll_signal_check(struct k_poll_signal *sig,
			       unsigned int *signaled, int *result)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(signaled, sizeof(unsigned int)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(result, sizeof(int)));
	z_impl_k_poll_signal_check(sig, signaled, result);
}
#include <zephyr/syscalls/k_poll_signal_check_mrsh.c>
#endif /* CONFIG_USERSPACE */

int z_impl_k_poll_signal_raise(struct k_poll_signal *sig, int result)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	struct k_poll_event *poll_event;

	sig->result = result;
	sig->signaled = 1U;

	poll_event = (struct k_poll_event *)sys_dlist_get(&sig->poll_events);
	if (poll_event == NULL) {
		k_spin_unlock(&lock, key);

		SYS_PORT_TRACING_FUNC(k_poll_api, signal_raise, sig, 0);

		return 0;
	}

	int rc = signal_poll_event(poll_event, K_POLL_STATE_SIGNALED);

	SYS_PORT_TRACING_FUNC(k_poll_api, signal_raise, sig, rc);

	z_reschedule(&lock, key);
	return rc;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_poll_signal_raise(struct k_poll_signal *sig,
					     int result)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	return z_impl_k_poll_signal_raise(sig, result);
}
#include <zephyr/syscalls/k_poll_signal_raise_mrsh.c>

static inline void z_vrfy_k_poll_signal_reset(struct k_poll_signal *sig)
{
	K_OOPS(K_SYSCALL_OBJ(sig, K_OBJ_POLL_SIGNAL));
	z_impl_k_poll_signal_reset(sig);
}
#include <zephyr/syscalls/k_poll_signal_reset_mrsh.c>

#endif /* CONFIG_USERSPACE */

static void triggered_work_handler(struct k_work *work)
{
	struct k_work_poll *twork =
			CONTAINER_OF(work, struct k_work_poll, work);

	/*
	 * If callback is not set, the k_work_poll_submit_to_queue()
	 * already cleared event registrations.
	 */
	if (twork->poller.mode != MODE_NONE) {
		k_spinlock_key_t key;

		key = k_spin_lock(&lock);
		clear_event_registrations(twork->events,
					  twork->num_events, key);
		k_spin_unlock(&lock, key);
	}

	/* Drop work ownership and execute real handler. */
	twork->workq = NULL;
	twork->real_handler(work);
}

static void triggered_work_expiration_handler(struct _timeout *timeout)
{
	struct k_work_poll *twork =
		CONTAINER_OF(timeout, struct k_work_poll, timeout);

	twork->poller.is_polling = false;
	twork->poll_result = -EAGAIN;
	k_work_submit_to_queue(twork->workq, &twork->work);
}

extern int z_work_submit_to_queue(struct k_work_q *queue,
			 struct k_work *work);

static int signal_triggered_work(struct k_poll_event *event, uint32_t status)
{
	struct z_poller *poller = event->poller;
	struct k_work_poll *twork =
		CONTAINER_OF(poller, struct k_work_poll, poller);

	if (poller->is_polling && twork->workq != NULL) {
		struct k_work_q *work_q = twork->workq;

		z_abort_timeout(&twork->timeout);
		twork->poll_result = 0;
		z_work_submit_to_queue(work_q, &twork->work);
	}

	return 0;
}

static int triggered_work_cancel(struct k_work_poll *work,
				 k_spinlock_key_t key)
{
	/* Check if the work waits for event. */
	if (work->poller.is_polling && work->poller.mode != MODE_NONE) {
		/* Remove timeout associated with the work. */
		z_abort_timeout(&work->timeout);

		/*
		 * Prevent work execution if event arrives while we will be
		 * clearing registrations.
		 */
		work->poller.mode = MODE_NONE;

		/* Clear registrations and work ownership. */
		clear_event_registrations(work->events, work->num_events, key);
		work->workq = NULL;
		return 0;
	}

	/*
	 * If we reached here, the work is either being registered in
	 * the k_work_poll_submit_to_queue(), executed or is pending.
	 * Only in the last case we have a chance to cancel it, but
	 * unfortunately there is no public API performing this task.
	 */

	return -EINVAL;
}

void k_work_poll_init(struct k_work_poll *work,
		      k_work_handler_t handler)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_work_poll, init, work);

	*work = (struct k_work_poll) {};
	k_work_init(&work->work, triggered_work_handler);
	work->real_handler = handler;
	z_init_timeout(&work->timeout);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_work_poll, init, work);
}

int k_work_poll_submit_to_queue(struct k_work_q *work_q,
				struct k_work_poll *work,
				struct k_poll_event *events,
				int num_events,
				k_timeout_t timeout)
{
	int events_registered;
	k_spinlock_key_t key;

	__ASSERT(work_q != NULL, "NULL work_q\n");
	__ASSERT(work != NULL, "NULL work\n");
	__ASSERT(events != NULL, "NULL events\n");
	__ASSERT(num_events > 0, "zero events\n");

	SYS_PORT_TRACING_FUNC_ENTER(k_work_poll, submit_to_queue, work_q, work, timeout);

	/* Take ownership of the work if it is possible. */
	key = k_spin_lock(&lock);
	if (work->workq != NULL) {
		if (work->workq == work_q) {
			int retval;

			retval = triggered_work_cancel(work, key);
			if (retval < 0) {
				k_spin_unlock(&lock, key);

				SYS_PORT_TRACING_FUNC_EXIT(k_work_poll, submit_to_queue, work_q,
					work, timeout, retval);

				return retval;
			}
		} else {
			k_spin_unlock(&lock, key);

			SYS_PORT_TRACING_FUNC_EXIT(k_work_poll, submit_to_queue, work_q,
				work, timeout, -EADDRINUSE);

			return -EADDRINUSE;
		}
	}


	work->poller.is_polling = true;
	work->workq = work_q;
	work->poller.mode = MODE_NONE;
	k_spin_unlock(&lock, key);

	/* Save list of events. */
	work->events = events;
	work->num_events = num_events;

	/* Clear result */
	work->poll_result = -EINPROGRESS;

	/* Register events */
	events_registered = register_events(events, num_events,
					    &work->poller, false);

	key = k_spin_lock(&lock);
	if (work->poller.is_polling && !K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/*
		 * Poller is still polling.
		 * No event is ready and all are watched.
		 */
		__ASSERT(num_events == events_registered,
			 "Some events were not registered!\n");

		/* Setup timeout if such action is requested */
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			z_add_timeout(&work->timeout,
				      triggered_work_expiration_handler,
				      timeout);
		}

		/* From now, any event will result in submitted work. */
		work->poller.mode = MODE_TRIGGERED;
		k_spin_unlock(&lock, key);

		SYS_PORT_TRACING_FUNC_EXIT(k_work_poll, submit_to_queue, work_q, work, timeout, 0);

		return 0;
	}

	/*
	 * The K_NO_WAIT timeout was specified or at least one event
	 * was ready at registration time or changed state since
	 * registration. Hopefully, the poller mode was not set, so
	 * work was not submitted to workqueue.
	 */

	/*
	 * If poller is still polling, no watched event occurred. This means
	 * we reached here due to K_NO_WAIT timeout "expiration".
	 */
	if (work->poller.is_polling) {
		work->poller.is_polling = false;
		work->poll_result = -EAGAIN;
	} else {
		work->poll_result = 0;
	}

	/* Clear registrations. */
	clear_event_registrations(events, events_registered, key);
	k_spin_unlock(&lock, key);

	/* Submit work. */
	k_work_submit_to_queue(work_q, &work->work);

	SYS_PORT_TRACING_FUNC_EXIT(k_work_poll, submit_to_queue, work_q, work, timeout, 0);

	return 0;
}

int k_work_poll_submit(struct k_work_poll *work,
				     struct k_poll_event *events,
				     int num_events,
				     k_timeout_t timeout)
{
	SYS_PORT_TRACING_FUNC_ENTER(k_work_poll, submit, work, timeout);

	int ret = k_work_poll_submit_to_queue(&k_sys_work_q, work,
								events, num_events, timeout);

	SYS_PORT_TRACING_FUNC_EXIT(k_work_poll, submit, work, timeout, ret);

	return ret;
}

int k_work_poll_cancel(struct k_work_poll *work)
{
	k_spinlock_key_t key;
	int retval;

	SYS_PORT_TRACING_FUNC_ENTER(k_work_poll, cancel, work);

	/* Check if the work was submitted. */
	if (work == NULL || work->workq == NULL) {
		SYS_PORT_TRACING_FUNC_EXIT(k_work_poll, cancel, work, -EINVAL);

		return -EINVAL;
	}

	key = k_spin_lock(&lock);
	retval = triggered_work_cancel(work, key);
	k_spin_unlock(&lock, key);

	SYS_PORT_TRACING_FUNC_EXIT(k_work_poll, cancel, work, retval);

	return retval;
}
