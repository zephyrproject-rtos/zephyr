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
struct _sem_desc {
	sys_dnode_t       semg_node; /* Node in list of semaphores */
	struct k_thread  *thread;    /* Thread waiting for semaphores */
	struct k_sem     *sem;       /* Semaphore on which to wait */
};

struct _sem_thread {
	struct _thread_base dummy;
	struct _sem_desc   desc;
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
	SYS_TRACING_OBJ_INIT(k_sem, sem);
}

#ifdef CONFIG_SEMAPHORE_GROUPS
int k_sem_group_take(struct k_sem *sem_array[], struct k_sem **sem,
		     int32_t timeout)
{
	unsigned int  key;
	struct k_sem *item = *sem_array;
	int           num = 0;

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

	struct _sem_thread  wait_objects[num];
	int32_t       priority = k_thread_priority_get(_current);
	sys_dlist_t   list;

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

	/* Pend the current thread on a dummy wait queue */

	_wait_q_t     wait_q;

	sys_dlist_init(&wait_q);
	_pend_current_thread(&wait_q, timeout);

	if (_Swap(key) != 0) {
		*sem = NULL;
		return -EAGAIN;
	}

	/* The accepted semaphore is the only one left on the list */

	struct _sem_desc *desc = (struct _sem_desc *)sys_dlist_get(&list);

	*sem = desc->sem;
	return 0;
}

/**
 * @brief Cancel all but specified semaphore in list if part of a semphore group
 *
 * Interrupts are locked prior to calling this routine
 *
 * @return 0 if not part of semaphore group, 1 if it is
 */
static int handle_sem_group(struct k_sem *sem, struct k_thread *thread)
{
	struct _sem_thread *dummy = (struct _sem_thread *)thread;
	struct _sem_thread *sem_thread;
	struct _sem_desc *desc = NULL;
	sys_dlist_t  *list;
	sys_dnode_t  *node;
	sys_dnode_t  *next;

	if (!(thread->base.thread_state & _THREAD_DUMMY)) {
		/*
		 * The awakened thread is a real thread and thus was not
		 * involved in a semaphore group operation.
		 */
		return 0;
	}

	/*
	 * The awakened thread is a dummy thread and thus was involved
	 * in a semaphore group operation.
	 */

	list = (sys_dlist_t *)dummy->desc.thread->base.swap_data;
	node = sys_dlist_peek_head(list);

	__ASSERT(node != NULL, "");

	do {
		next = sys_dlist_peek_next(list, node);

		desc = (struct _sem_desc *)node;

		if (desc->sem != sem) {
			sem_thread = CONTAINER_OF(desc, struct _sem_thread,
						  desc);
			struct k_thread *dummy_thread =
				(struct k_thread *)&sem_thread->dummy;

			if (_is_thread_timeout_expired(dummy_thread)) {
				continue;
			}
			_abort_thread_timeout(dummy_thread);
			_unpend_thread(dummy_thread);

			sys_dlist_remove(node);
		}
		node = next;
	} while (node != NULL);

	/*
	 * If 'desc' is NULL, then the user-supplied 'sem_array' had only
	 * one semaphore in it. This is considered a user error as
	 * k_sem_give() should have been called instead.
	 */

	__ASSERT(desc != NULL, "");

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

	return 1;
}

#else
#define handle_sem_group(sem, thread) 0
#endif

/**
 * @brief Common semaphore give code
 *
 * @return true if _Swap() will need to be invoked; false if not
 */
static bool sem_give_common(struct k_sem *sem)
{
	struct k_thread *thread;

	thread = _unpend_first_thread(&sem->wait_q);
	if (!thread) {
		/*
		 * No thread is waiting on the semaphore.
		 * Increment the semaphore's count unless
		 * its limit has already been reached.
		 */
		sem->count += (sem->count != sem->limit);
		return false;
	}

	_abort_thread_timeout(thread);

	if (!handle_sem_group(sem, thread)) {
		/* Handle the non-group case */
		_ready_thread(thread);
		_set_thread_return_value(thread, 0);
	}

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
		/* increment semaphore's count unless limit is reached */
		sem->count += (sem->count != sem->limit);
		return;
	}

	_abort_thread_timeout(thread);

	_ready_thread(thread);
	_set_thread_return_value(thread, 0);
}

#ifdef CONFIG_SEMAPHORE_GROUPS
void k_sem_group_give(struct k_sem *sem_array[])
{
	unsigned int   key;
	bool           swap_needed = false;

	__ASSERT(sem_array[0] != K_END, "Empty semaphore list");

	key = irq_lock();

	for (int i = 0; sem_array[i] != K_END; i++) {
		swap_needed |= sem_give_common(sem_array[i]);
	}

	if (swap_needed) {
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

void k_sem_group_reset(struct k_sem *sem_array[])
{
	unsigned int  key;

	key = irq_lock();
	for (int i = 0; sem_array[i] != K_END; i++) {
		sem_array[i]->count = 0;
	}
	irq_unlock(key);
}
#endif

void k_sem_give(struct k_sem *sem)
{
	unsigned int   key;

	key = irq_lock();

	if (sem_give_common(sem)) {
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
