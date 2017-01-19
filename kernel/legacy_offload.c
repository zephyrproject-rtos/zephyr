/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file @brief task_offload_to_fiber() function for legacy applications
 *
 * For the legacy applications that need task_offload_to_fiber() function,
 * the moduel implements it by means of using work queue
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <init.h>

struct offload_work {
	struct k_work work_item;
	int (*offload_func)();
	void *offload_args;
	struct k_thread *thread;
};

static struct k_work_q offload_work_q;

/*
 * Internal handler of the offload requests
 */

static void offload_handler(struct k_work *work)
{
	struct offload_work *offload =
		CONTAINER_OF(work, struct offload_work, work_item);
	int result = (offload->offload_func)(offload->offload_args);
	unsigned int key = irq_lock();

	offload->thread->base.swap_data = (void *)result;
	irq_unlock(key);
}

int task_offload_to_fiber(int (*func)(), void *argp)
{
	/*
	 * Create work in stack. Task is scheduled out and does not
	 * return until the work is consumed and complete, so the
	 * work item will exists until then.
	 */
	struct offload_work offload = {
		.offload_func = func,
		.offload_args = argp
	};

	__ASSERT(_is_preempt(_current), "Fiber is trying to offload work");

	k_work_init(&offload.work_item, offload_handler);

	offload.thread = _current;
	k_work_submit_to_queue(&offload_work_q, &offload.work_item);
	return (int)_current->base.swap_data;
}

static char __stack offload_work_q_stack[CONFIG_OFFLOAD_WORKQUEUE_STACK_SIZE];

static int k_offload_work_q_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_q_start(&offload_work_q,
		       offload_work_q_stack,
		       sizeof(offload_work_q_stack),
		       CONFIG_OFFLOAD_WORKQUEUE_PRIORITY);

	return 0;
}

SYS_INIT(k_offload_work_q_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
