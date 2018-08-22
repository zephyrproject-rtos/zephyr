/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Workqueue support functions
 */

#include <kernel_structs.h>
#include <wait_q.h>
#include <errno.h>
#include <syscall_handler.h>

static inline bool z_is_work_q_user(struct k_work_q *work_q)
{
	return work_q->thread.base.user_options & K_USER;
}

void *_impl_z_work_q_get(struct k_work_q *work_q)
{
	return k_queue_get(&work_q->queue, K_FOREVER);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(z_work_q_get, work_q)
{
	Z_OOPS(Z_SYSCALL_OBJ(work_q, K_OBJ_WORK_Q));

	return (u32_t)_impl_z_work_q_get((struct k_work_q *)work_q);
}
#endif

int _impl_z_work_q_put(struct k_work_q *work_q, void *work)
{
	k_queue_append(&work_q->queue, work);

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(z_work_q_put, work_q_ptr, work)
{
	struct k_work_q *work_q = (struct k_work_q *)work_q_ptr;

	Z_OOPS(Z_SYSCALL_OBJ(work_q, K_OBJ_WORK_Q));

	/* User threads may only put stuff in a work queue that is also
	 * running in user mode, extra paranoia if somehow a user thread
	 * got permissions on a work_q that runs in supervisor mode.
	 */
	Z_OOPS(!z_is_work_q_user(work_q));

	/* Using alloc_append since supervisor mode must never dereference
	 * the work pointer itself; it gets wrapped in a container first.
	 */
	return k_queue_alloc_append(&work_q->queue, (void *)work);
}
#endif

#ifdef CONFIG_MULTITHREADING
void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t *stack,
		    size_t stack_size, int prio)
{
	k_queue_init(&work_q->queue);
	k_thread_create(&work_q->thread, stack, stack_size, z_work_q_main,
			work_q, 0, 0, prio, 0, 0);
	_k_object_init(work_q);
}

void _impl_k_work_q_user_start(struct k_work_q *work_q, k_thread_stack_t *stack,
			       size_t stack_size, int prio)
{
	k_queue_init(&work_q->queue);

	/* Created worker thread will inherit object permissions and memory
	 * domain configuration of the caller
	 */
	k_thread_create(&work_q->thread, stack, stack_size, z_work_q_main,
			work_q, 0, 0, prio, K_USER | K_INHERIT_PERMS, 0);
	k_object_access_grant(work_q, &work_q->thread);

	_k_object_init(work_q);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_work_q_user_start, work_q, stack, stack_size, prio)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(work_q, K_OBJ_WORK_Q));
	Z_OOPS(z_thread_stack_validate((k_thread_stack_t *)stack, stack_size));

	/* Check validity of prio argument; must be the same or worse priority
	 * than the caller
	 */
	Z_OOPS(Z_SYSCALL_VERIFY(_is_valid_prio(prio, NULL)));
	Z_OOPS(Z_SYSCALL_VERIFY(_is_prio_lower_or_equal(prio,
							_current->base.prio)));

	_impl_k_work_q_user_start((struct k_work_q *)work_q,
				  (k_thread_stack_t *)stack, stack_size, prio);
	return 0;
}
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_MULTITHREADING */

#ifdef CONFIG_SYS_CLOCK_EXISTS
static void work_timeout(struct _timeout *t)
{
	struct k_delayed_work *w = CONTAINER_OF(t, struct k_delayed_work,
						   timeout);

	/* submit work to workqueue */
	k_work_submit_to_queue(w->work_q, &w->work);
}

void k_delayed_work_init(struct k_delayed_work *work, k_work_handler_t handler)
{
	k_work_init(&work->work, handler);
	_init_timeout(&work->timeout, work_timeout);
	work->work_q = NULL;

	_k_object_init(work);
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
				   struct k_delayed_work *work,
				   s32_t delay)
{
	unsigned int key = irq_lock();
	int err;

	/* Work cannot be active in multiple queues */
	if (work->work_q && work->work_q != work_q) {
		err = -EADDRINUSE;
		goto done;
	}

	/* Cancel if work has been submitted */
	if (work->work_q == work_q) {
		err = k_delayed_work_cancel(work);
		if (err < 0) {
			goto done;
		}
	}

	/* Attach workqueue so the timeout callback can submit it */
	work->work_q = work_q;

	if (!delay) {
		/* Submit work if no ticks is 0 */
		k_work_submit_to_queue(work_q, &work->work);
	} else {
		/* Add timeout */
		_add_timeout(NULL, &work->timeout, NULL,
				_TICK_ALIGN + _ms_to_ticks(delay));
	}

	err = 0;

done:
	irq_unlock(key);

	return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
	unsigned int key = irq_lock();

	if (!work->work_q) {
		irq_unlock(key);
		return -EINVAL;
	}

	if (k_work_pending(&work->work)) {
		/* Remove from the queue if already submitted */
		if (!k_queue_remove(&work->work_q->queue, &work->work)) {
			irq_unlock(key);
			return -EINVAL;
		}
	} else {
		_abort_timeout(&work->timeout);
	}

	/* Detach from workqueue */
	work->work_q = NULL;

	atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);
	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */
