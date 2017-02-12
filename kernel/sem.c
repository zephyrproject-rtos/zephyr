/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Kernel semaphore object.
 *
 * The semaphores are of the 'counting' type, i.e. each 'give' operation will
 * increment the internal count by 1, if no fiber is pending on it. The 'init'
 * call initializes the count to 0. Following multiple 'give' operations, the
 * same number of 'take' operations can be performed without the calling fiber
 * having to pend on the semaphore, or the calling task having to poll.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <wait_q.h>
#include <misc/dlist.h>
#include <ksched.h>
#include <init.h>

#ifdef CONFIG_SEMAPHORE_GROUPS
struct sem_desc {

	/* node in list of semaphores */
	sys_dnode_t semg_node;

	/* thread waiting for semaphores */
	struct k_thread *thread;

	/* semaphore on which to wait */
	struct k_sem *sem;
};

struct sem_thread {

	/* dummy thread, only the thread base */
	struct _thread_base dummy;

	/* descriptor containing real thread , sem, and group info */
	struct sem_desc desc;
};
#endif

extern struct k_sem _k_sem_list_start[];
extern struct k_sem _k_sem_list_end[];

struct k_sem *_trace_list_k_sem;

#ifdef CONFIG_OBJECT_TRACING

/*
 * Complete initialization of statically defined semaphores.
 */
static int init_sem_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_sem *sem;

	for (sem = _k_sem_list_start; sem < _k_sem_list_end; sem++) {
		SYS_TRACING_OBJ_INIT(k_sem, sem);
	}
	return 0;
}

SYS_INIT(init_sem_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void k_sem_init(struct k_sem *sem, unsigned int initial_count,
		unsigned int limit)
{
	__ASSERT(limit != 0, "limit cannot be zero");

	sem->count = initial_count;
	sem->limit = limit;
	sys_dlist_init(&sem->wait_q);

	_INIT_OBJ_POLL_EVENT(sem);

	SYS_TRACING_OBJ_INIT(k_sem, sem);
}

#ifdef CONFIG_SEMAPHORE_GROUPS
int k_sem_group_take(struct k_sem *sem_array[], struct k_sem **sem,
		     int32_t timeout)
{
	unsigned int key;
	struct k_sem *item = *sem_array;
	int num = 0;

	__ASSERT(sem_array[0] != K_END, "Empty semaphore list");

	key = irq_lock();

	do {
		if (item->count > 0) {
			item->count--;       /* Available semaphore found */
			irq_unlock(key);
			*sem = item;
			return 0;
		}
		num++;
		item = sem_array[num];
	} while (item != K_END);

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		*sem = NULL;
		return -EBUSY;
	}

	struct sem_thread wait_objects[num];
	int32_t priority = k_thread_priority_get(_current);
	sys_dlist_t list;

	sys_dlist_init(&list);
	_current->base.swap_data = &list;

	for (int i = 0; i < num; i++) {

		_init_thread_base(&wait_objects[i].dummy, priority,
				  _THREAD_DUMMY, 0);

		sys_dlist_append(&list, &wait_objects[i].desc.semg_node);
		wait_objects[i].desc.thread = _current;
		wait_objects[i].desc.sem = sem_array[i];

		_pend_thread((struct k_thread *)&wait_objects[i].dummy,
			     &sem_array[i]->wait_q, timeout);
	}

	/*
	 * Pend the current thread on a dummy wait queue, adding it _after_ all
	 * the dummy threads on the _timeout_q, but expiring on the same tick,
	 * which will cause it to be _prepended_ to the dummy threads. See
	 * description of _add_timeout() for details.
	 */

	_wait_q_t wait_q;

	sys_dlist_init(&wait_q);
	_pend_current_thread(&wait_q, timeout);

	if (_Swap(key) != 0) {
		*sem = NULL;
		return -EAGAIN;
	}

	/* The accepted semaphore is the only one left on the list */

	struct sem_desc *desc = (struct sem_desc *)sys_dlist_get(&list);

	*sem = desc->sem;
	return 0;
}

/* cancel all but specified semaphore in list if part of a semphore group */
static void handle_sem_group(struct k_sem *sem, struct sem_thread *sem_thread)
{
	struct sem_desc *desc = NULL;
	sys_dlist_t *list;
	sys_dnode_t *node;
	sys_dnode_t *next;

	list = (sys_dlist_t *)sem_thread->desc.thread->base.swap_data;
	node = sys_dlist_peek_head(list);

	__ASSERT(node != NULL, "");

	do {
		next = sys_dlist_peek_next(list, node);
		desc = (struct sem_desc *)node;

		sem_thread = CONTAINER_OF(desc, struct sem_thread, desc);
		struct k_thread *dummy = (struct k_thread *)&sem_thread->dummy;

		/*
		 * This is tricky: due to the fact that the timeouts expiring
		 * at the same time are queued in reverse order, we know that,
		 * since the caller of this function has already verified that
		 * the timeout of the real thread has not expired and since it
		 * was queued after the dummy threads, causing it to be the
		 * first to be unpended, that the timeouts of the dummy threads
		 * have not expired. Thus, we do not have to handle the case
		 * where the timeout of the dummy thread might have expired.
		 */
		_abort_thread_timeout(dummy);
		_unpend_thread(dummy);

		if (desc->sem != sem) {
			sys_dlist_remove(node);
		}

		node = next;
	} while (node != NULL);

	/* if node was not NULL, desc is not NULL: no need to check */

	/*
	 * As this code may be executed several times by a semaphore group give
	 * operation, it is important to ensure that the attempt to ready the
	 * master thread is done only once.
	 */

	if (!_is_thread_ready(desc->thread)) {
		_abort_thread_timeout(desc->thread);
		_mark_thread_as_not_pending(desc->thread);
		if (_is_thread_ready(desc->thread)) {
			_add_thread_to_ready_q(desc->thread);
		}
	}
	_set_thread_return_value(desc->thread, 0);
}

#else
#define handle_sem_group(sem, thread) 0
#endif

/* returns 1 if a reschedule must take place, 0 otherwise */
static inline int handle_poll_event(struct k_sem *sem)
{
#ifdef CONFIG_POLL
	uint32_t state = K_POLL_STATE_SEM_AVAILABLE;

	return sem->poll_event ?
	       _handle_obj_poll_event(&sem->poll_event, state) : 0;
#else
	return 0;
#endif
}

static inline void increment_count_up_to_limit(struct k_sem *sem)
{
	sem->count += (sem->count != sem->limit);
}

/* returns 1 if _Swap() will need to be invoked, 0 otherwise */
static int do_sem_give(struct k_sem *sem)
{
#ifdef CONFIG_SEMAPHORE_GROUPS
	struct k_thread *thread = NULL;

again:
	thread = _find_first_thread_to_unpend(&sem->wait_q, thread);
	if (!thread) {
		increment_count_up_to_limit(sem);
		return handle_poll_event(sem);
	}

	if (unlikely(_is_thread_dummy(thread))) {
		/*
		 * The awakened thread is a dummy struct sem_thread and thus
		 * was involved in a semaphore group operation.
		 */
		struct sem_thread *sem_thread = (struct sem_thread *)thread;
		struct k_thread *real_thread = sem_thread->desc.thread;

		/*
		 * This is an extremely tricky way of handling the fact that
		 * the current sem_give might have happened from an ISR while
		 * the timeout handling code is running, going through the list
		 * of expired timeouts.
		 *
		 * We have to be able to handle all timeouts on a
		 * k_sem_group_take operation as one. We do that by checking if
		 * the timeout of the real thread has expired or not. We can do
		 * this, because of the way the timeouts are queued in the
		 * kernel's timeout_q: timeouts expiring on the same tick are
		 * queued in the _reverse_ order that they arrive. It is done
		 * this way to save time with interrupts locked. By knowing
		 * this, and by adding the real thread _last_ to the timeout_q,
		 * we know that it is queued _before_ all the dummy threads
		 * from the k_sem_group_take operation. This allows us to check
		 * that, if the real thread's timeout has not expired, then all
		 * dummy threads' timeouts have not expired either. If the real
		 * thread's timeout has expired, then the dummy threads'
		 * timeouts will expire or have expired already during the
		 * current handling of timeouts, and the timeout code will take
		 * care of signalling the waiter that its operation has
		 * timedout. In that case, we look for the next thread not part
		 * of the same k_sem_group_take operation to give it the
		 * semaphore.
		 */
		if (_is_thread_timeout_expired(real_thread)) {
			goto again;
		}

		/*
		 * Do not dequeue the dummy thread: that will be done when
		 * looping through the list of dummy waiters in
		 * handle_sem_group().
		 */
		handle_sem_group(sem, sem_thread);
	} else {
		_unpend_thread(thread);
		(void)_abort_thread_timeout(thread);
		_ready_thread(thread);
		_set_thread_return_value(thread, 0);
	}
#else
	struct k_thread *thread = _unpend_first_thread(&sem->wait_q);

	if (!thread) {
		increment_count_up_to_limit(sem);
		return handle_poll_event(sem);
	}
	(void)_abort_thread_timeout(thread);
	_ready_thread(thread);
	_set_thread_return_value(thread, 0);
#endif

	return !_is_in_isr() && _must_switch_threads();
}

/*
 * This function is meant to be called only by
 * _sys_event_logger_put_non_preemptible(), which itself is really meant to be
 * called only by _sys_k_event_logger_context_switch(), used within a context
 * switch to log the event.
 *
 * WARNING:
 * It must be called with interrupts already locked.
 * It cannot be called for a sempahore part of a group.
 */
void _sem_give_non_preemptible(struct k_sem *sem)
{
	struct k_thread *thread;

	thread = _unpend_first_thread(&sem->wait_q);
	if (!thread) {
		increment_count_up_to_limit(sem);
		return;
	}

	_abort_thread_timeout(thread);

	_ready_thread(thread);
	_set_thread_return_value(thread, 0);
}

#ifdef CONFIG_SEMAPHORE_GROUPS
void k_sem_group_give(struct k_sem *sem_array[])
{
	int swap_needed = 0;
	unsigned int key;

	__ASSERT(sem_array[0] != K_END, "Empty semaphore list");

	key = irq_lock();

	for (int i = 0; sem_array[i] != K_END; i++) {
		swap_needed |= do_sem_give(sem_array[i]);
	}

	if (swap_needed) {
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

void k_sem_group_reset(struct k_sem *sem_array[])
{
	unsigned int key;

	key = irq_lock();
	for (int i = 0; sem_array[i] != K_END; i++) {
		sem_array[i]->count = 0;
	}
	irq_unlock(key);
}
#endif

void k_sem_give(struct k_sem *sem)
{
	unsigned int key;

	key = irq_lock();

	if (do_sem_give(sem)) {
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

int k_sem_take(struct k_sem *sem, int32_t timeout)
{
	__ASSERT(!_is_in_isr() || timeout == K_NO_WAIT, "");

	unsigned int key = irq_lock();

	if (likely(sem->count > 0)) {
		sem->count--;
		irq_unlock(key);
		return 0;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return -EBUSY;
	}

	_pend_current_thread(&sem->wait_q, timeout);

	return _Swap(key);
}
