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

static void work_q_main(void *work_q_ptr, void *p2, void *p3)
{
	struct k_work_q *work_q = work_q_ptr;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct k_work *work;
		k_work_handler_t handler;

		work = k_queue_get(&work_q->queue, K_FOREVER);
		if (!work) {
			continue;
		}

		handler = work->handler;

		/* Reset pending state so it can be resubmitted by handler */
		if (atomic_test_and_clear_bit(work->flags,
					      K_WORK_STATE_PENDING)) {
			handler(work);
		}

		/* Make sure we don't hog up the CPU if the FIFO never (or
		 * very rarely) gets empty.
		 */
		k_yield();
	}
}

void k_work_q_start(struct k_work_q *work_q, k_thread_stack_t *stack,
		    size_t stack_size, int prio)
{
	k_queue_init(&work_q->queue);
	k_thread_create(&work_q->thread, stack, stack_size, work_q_main,
			work_q, 0, 0, prio, 0, 0);
	_k_object_init(work_q);
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
	_init_timeout(&work->timeout, work_timeout);
	work->work_q = NULL;

	_k_object_init(work);
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
				   struct k_delayed_work *work,
				   s32_t delay)
{
	int key = irq_lock();
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
	int key = irq_lock();

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

	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_SYS_CLOCK_EXISTS */
