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
#include <spinlock.h>
#include <errno.h>
#include <stdbool.h>

#define WORKQUEUE_THREAD_NAME	"workqueue"

#ifdef CONFIG_SYS_CLOCK_EXISTS
static struct k_spinlock lock;
#endif

extern void z_work_q_main(void *work_q_ptr, void *p2, void *p3);

void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t *stack,
		    size_t stack_size, int prio)
{
	k_queue_init(&work_q->queue);
	(void)k_thread_create(&work_q->thread, stack, stack_size, z_work_q_main,
			work_q, NULL, NULL, prio, 0, K_NO_WAIT);

	k_thread_name_set(&work_q->thread, WORKQUEUE_THREAD_NAME);
}

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
	z_init_timeout(&work->timeout);
	work->work_q = NULL;
}

static int work_cancel(struct k_delayed_work *work)
{
	__ASSERT(work->work_q != NULL, "");

	if (k_work_pending(&work->work)) {
		/* Remove from the queue if already submitted */
		if (!k_queue_remove(&work->work_q->queue, &work->work)) {
			return -EINVAL;
		}
	} else {
		(void)z_abort_timeout(&work->timeout);
	}

	/* Detach from workqueue */
	work->work_q = NULL;

	atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);

	return 0;
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
				   struct k_delayed_work *work,
				   s32_t delay)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int err = 0;

	/* Work cannot be active in multiple queues */
	if (work->work_q != NULL && work->work_q != work_q) {
		err = -EADDRINUSE;
		goto done;
	}

	/* Cancel if work has been submitted */
	if (work->work_q == work_q) {
		err = work_cancel(work);
		if (err < 0) {
			goto done;
		}
	}

	/* Attach workqueue so the timeout callback can submit it */
	work->work_q = work_q;

	/* Submit work directly if no delay.  Note that this is a
	 * blocking operation, so release the lock first.
	 */
	if (delay == 0) {
		k_spin_unlock(&lock, key);
		k_work_submit_to_queue(work_q, &work->work);
		return 0;
	}

	/* Add timeout */
	z_add_timeout(&work->timeout, work_timeout,
		     _TICK_ALIGN + z_ms_to_ticks(delay));

done:
	k_spin_unlock(&lock, key);
	return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
	if (!work->work_q) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	int ret = work_cancel(work);

	k_spin_unlock(&lock, key);
	return ret;
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */
