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

#include <misc/nano_work.h>

static void workqueue_fiber_main(int arg1, int arg2)
{
	struct nano_workqueue *wq = (struct nano_workqueue *)arg1;

	ARG_UNUSED(arg2);

	while (1) {
		struct nano_work *work;

		work = nano_fiber_fifo_get(&wq->fifo, TICKS_UNLIMITED);

		work->handler(work);
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
