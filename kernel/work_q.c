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
#include <sys/check.h>

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
	if (k_work_pending(&work->work)) {
		/* Remove from the queue if already submitted */
		if (!k_queue_remove(&work->work_q->queue, &work->work)) {
			return -EINVAL;
		}
	} else {
		int err = z_abort_timeout(&work->timeout);

		if (err) {
			return -EALREADY;
		}
	}

	/* Detach from workqueue */
	work->work_q = NULL;

	atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);

	return 0;
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
				   struct k_delayed_work *work,
				   k_timeout_t delay)
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
		/* -EALREADY may indicate the work has already completed so
		 * this is likely a recurring work.  It may also indicate that
		 * the work handler is still executing.  But it's neither
		 * delayed nor pending, so it can be rescheduled.
		 */
		if (err == -EALREADY) {
			err = 0;
		} else if (err < 0) {
			goto done;
		}
	}

	/* Attach workqueue so the timeout callback can submit it */
	work->work_q = work_q;

	/* Submit work directly if no delay.  Note that this is a
	 * blocking operation, so release the lock first.
	 */
	if (K_TIMEOUT_EQ(delay, K_NO_WAIT)) {
		k_spin_unlock(&lock, key);
		k_work_submit_to_queue(work_q, &work->work);
		return 0;
	}

#ifdef CONFIG_LEGACY_TIMEOUT_API
	delay = _TICK_ALIGN + k_ms_to_ticks_ceil32(delay);
#endif

	/* Add timeout */
	z_add_timeout(&work->timeout, work_timeout, delay);

done:
	k_spin_unlock(&lock, key);
	return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int ret = -EINVAL;

	if (work->work_q != NULL) {
		ret = work_cancel(work);
	}

	k_spin_unlock(&lock, key);
	return ret;
}

bool k_delayed_work_pending(struct k_delayed_work *work)
{
	return !z_is_inactive_timeout(&work->timeout) ||
	       k_work_pending(&work->work);
}

static void work_delayable_timeout(struct _timeout *t)
{
	struct k_work_delayable *w = CONTAINER_OF(t, struct k_work_delayable,
						   timeout);
	k_spinlock_key_t key = k_spin_lock(&lock);
	struct k_work_q *wq = w->work_q;

	w->work_q = NULL;
	k_spin_unlock(&lock, key);

	/* submit work to workqueue */
	k_work_submit_to_queue(wq, &w->work);
}

void k_work_delayable_init(struct k_work_delayable *work, k_work_handler_t handler)
{
	k_work_init(&work->work, handler);
	z_init_timeout(&work->timeout);
	work->work_q = NULL;
}

int k_work_schedule_for_queue(struct k_work_q *work_q,
			      struct k_work_delayable *work,
			      k_timeout_t delay)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int err = 0;

	/* Verify work queue validity and cancel any incomplete
	 * timeout.
	 */
	if (work->work_q != NULL) {
		/* Work cannot be active in multiple queues */
		if (work->work_q != work_q) {
			err = -EBUSY;
			goto unlock;
		}

		/* Cancel any incomplete timeout. */
		z_abort_timeout(&work->timeout);
		work->work_q = NULL;
	}

	/* If the timeout is no-wait, just submit it.  If it's already
	 * pending its position will be unchanged.
	 */
	if (K_TIMEOUT_EQ(delay, K_NO_WAIT)) {
		k_spin_unlock(&lock, key);
		k_work_submit_to_queue(work_q, &work->work);
		goto out;
	}

	/* Attach workqueue so the timeout callback can submit it */
	work->work_q = work_q;

#ifdef CONFIG_LEGACY_TIMEOUT_API
	delay = _TICK_ALIGN + k_ms_to_ticks_ceil32(delay);
#endif

	/* Add timeout */
	z_add_timeout(&work->timeout, work_delayable_timeout, delay);

unlock:
	k_spin_unlock(&lock, key);
out:
	return err;
}

int k_work_cancel(struct k_work_delayable *work)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	int ret = z_abort_timeout(&work->timeout);

	if (ret != 0) {
		ret = k_work_pending(&work->work) ? -EINPROGRESS : -EINVAL;
	}

	k_spin_unlock(&lock, key);
	return ret;
}

#endif /* CONFIG_SYS_CLOCK_EXISTS */
