/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

static void work_q_main(void *work_q_ptr, void *p2, void *p3)
{
	struct k_work_q *work_q = work_q_ptr;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		struct k_work *work;
		k_work_handler_t handler;

		work = k_fifo_get(&work_q->fifo, K_FOREVER);

		handler = work->handler;

		/* Set state to idle so it can be resubmitted by handler */
		if (!atomic_test_and_set_bit(work->flags, K_WORK_STATE_IDLE)) {
			handler(work);
		}

		/* Make sure we don't hog up the CPU if the FIFO never (or
		 * very rarely) gets empty.
		 */
		k_yield();
	}
}

void k_work_q_start(struct k_work_q *work_q,
		    const struct k_thread_config *config)
{
	k_fifo_init(&work_q->fifo);

	k_thread_spawn(config->stack, config->stack_size,
		       work_q_main, work_q, 0, 0,
		       config->prio, 0, 0);
}

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
	_timeout_init(&work->timeout, work_timeout);
	work->work_q = NULL;
}

int k_delayed_work_submit_to_queue(struct k_work_q *work_q,
				   struct k_delayed_work *work,
				   int32_t timeout)
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

	if (!timeout) {
		/* Submit work if no ticks is 0 */
		k_work_submit_to_queue(work_q, &work->work);
	} else {
		/* Add timeout */
		_do_timeout_add(NULL, &work->timeout, NULL,
				_ms_to_ticks(timeout));
	}

	err = 0;

done:
	irq_unlock(key);

	return err;
}

int k_delayed_work_cancel(struct k_delayed_work *work)
{
	int key = irq_lock();

	if (!atomic_test_bit(work->work.flags, K_WORK_STATE_IDLE)) {
		irq_unlock(key);
		return -EINPROGRESS;
	}

	if (!work->work_q) {
		irq_unlock(key);
		return -EINVAL;
	}

	/* Abort timeout, if it has expired this will do nothing */
	_do_timeout_abort(&work->timeout);

	/* Detach from workqueue */
	work->work_q = NULL;

	irq_unlock(key);

	return 0;
}

#ifdef CONFIG_SYSTEM_WORKQUEUE

#include <init.h>

static char __stack sys_work_q_stack[CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE];

static const struct k_thread_config sys_work_q_config = {
	.stack = sys_work_q_stack,
	.stack_size = sizeof(sys_work_q_stack),
	.prio = CONFIG_SYSTEM_WORKQUEUE_PRIORITY,
};

struct k_work_q k_sys_work_q;

static int k_sys_work_q_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_q_start(&k_sys_work_q, &sys_work_q_config);

	return 0;
}

SYS_INIT(k_sys_work_q_init, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
