/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * Workqueue support functions
 */

#include <nano_private.h>
#include <wait_q.h>
#include <errno.h>

#include <misc/nano_work.h>

static void workqueue_fiber_main(int arg1, int arg2)
{
	struct nano_workqueue *wq = (struct nano_workqueue *)arg1;

	ARG_UNUSED(arg2);

	while (1) {
		struct nano_work *work;
		work_handler_t handler;

		work = nano_fiber_fifo_get(&wq->fifo, TICKS_UNLIMITED);

		handler = work->handler;

		/* Set state to idle so it can be resubmitted by handler */
		if (!atomic_test_and_set_bit(work->flags,
					     NANO_WORK_STATE_IDLE)) {
			handler(work);
		}
	}
}

void nano_fiber_workqueue_start(struct nano_workqueue *wq,
				const struct fiber_config *config)
{
	nano_fifo_init(&wq->fifo);

	fiber_fiber_start_config(config, workqueue_fiber_main,
				 (int)wq, 0, 0);
}

void nano_task_workqueue_start(struct nano_workqueue *wq,
			       const struct fiber_config *config)
{
	nano_fifo_init(&wq->fifo);

	task_fiber_start_config(config, workqueue_fiber_main,
				(int)wq, 0, 0);
}

void nano_workqueue_start(struct nano_workqueue *wq,
			  const struct fiber_config *config)
{
	nano_fifo_init(&wq->fifo);

	fiber_start_config(config, workqueue_fiber_main,
			   (int)wq, 0, 0);
}

static void work_timeout(struct _nano_timeout *t)
{
	struct nano_delayed_work *w = CONTAINER_OF(t, struct nano_delayed_work,
						   timeout);

	/* submit work to workqueue */
	nano_work_submit_to_queue(w->wq, &w->work);
}

void nano_delayed_work_init(struct nano_delayed_work *work,
			    work_handler_t handler)
{
	nano_work_init(&work->work, handler);
	_nano_timeout_init(&work->timeout, work_timeout);
	work->wq = NULL;
}

int nano_delayed_work_submit_to_queue(struct nano_workqueue *wq,
				      struct nano_delayed_work *work,
				      int ticks)
{
	int key = irq_lock();
	int err;

	/* Work cannot be active in multiple queues */
	if (work->wq && work->wq != wq) {
		err = -EADDRINUSE;
		goto done;
	}

	/* Cancel if work has been submitted */
	if (work->wq == wq) {
		err = nano_delayed_work_cancel(work);
		if (err < 0) {
			goto done;
		}
	}

	/* Attach workqueue so the timeout callback can submit it */
	work->wq = wq;

	if (!ticks) {
		/* Submit work if no ticks is 0 */
		nano_work_submit_to_queue(wq, &work->work);
	} else {
		/* Add timeout */
		_do_nano_timeout_add(NULL, &work->timeout, NULL, ticks);
	}

	err = 0;

done:
	irq_unlock(key);

	return err;
}

int nano_delayed_work_cancel(struct nano_delayed_work *work)
{
	int key = irq_lock();

	if (!atomic_test_bit(work->work.flags, NANO_WORK_STATE_IDLE)) {
		irq_unlock(key);
		return -EINPROGRESS;
	}

	if (!work->wq) {
		irq_unlock(key);
		return -EINVAL;
	}

	/* Abort timeout, if it has expired this will do nothing */
	_do_nano_timeout_abort(&work->timeout);

	/* Detach from workqueue */
	work->wq = NULL;

	irq_unlock(key);

	return 0;
}

#ifdef CONFIG_SYSTEM_WORKQUEUE

#include <init.h>

static char __stack sys_wq_stack[CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE];

static const struct fiber_config sys_wq_config = {
	.stack = sys_wq_stack,
	.stack_size = sizeof(sys_wq_stack),
	.prio = CONFIG_SYSTEM_WORKQUEUE_PRIORITY,
};

struct nano_workqueue sys_workqueue;

static int sys_workqueue_init(struct device *dev)
{
	ARG_UNUSED(dev);

	nano_workqueue_start(&sys_workqueue, &sys_wq_config);

	return 0;
}

SYS_INIT(sys_workqueue_init, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
