/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * System workqueue.
 */

#include <kernel.h>
#include <init.h>

K_THREAD_STACK_DEFINE(sys_work_q_stack, CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);

struct k_work_q k_sys_work_q;

static int k_sys_work_q_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_q_start(&k_sys_work_q,
		       sys_work_q_stack,
		       K_THREAD_STACK_SIZEOF(sys_work_q_stack),
		       CONFIG_SYSTEM_WORKQUEUE_PRIORITY);
	k_thread_name_set(&k_sys_work_q.thread, "sysworkq");

	return 0;
}

SYS_INIT(k_sys_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
