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

#include <zephyr/kernel.h>
#include <zephyr/init.h>

struct k_work_q k_sys_work_q;

static const struct k_work_queue_config cfg = {
	.name = "sysworkq",
	.no_yield = IS_ENABLED(CONFIG_SYSTEM_WORKQUEUE_NO_YIELD),
	.essential = true,
	.work_timeout_ms = CONFIG_SYSTEM_WORKQUEUE_WORK_TIMEOUT_MS,
};

#ifdef CONFIG_SYSTEM_WORKQUEUE_CREATE_THREAD

static K_KERNEL_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);
static struct k_thread k_sys_work_q_thread;

static int k_sys_work_q_init(void)
{
	k_work_queue_start(&k_sys_work_q, &k_sys_work_q_thread, sys_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(sys_work_q_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, &cfg);
	return 0;
}

SYS_INIT(k_sys_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#else /* CONFIG_SYSTEM_WORKQUEUE_CREATE_THREAD */

void k_sys_work_q_run(void)
{
	k_work_queue_run(&k_sys_work_q, &cfg);
}

#endif /* CONFIG_SYSTEM_WORKQUEUE_CREATE_THREAD */
