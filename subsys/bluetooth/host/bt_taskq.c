/* bt_taskq.c - Workqueue for quick non-blocking Bluetooth tasks */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/autoconf.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/kernel/thread_stack.h>

static K_THREAD_STACK_DEFINE(bt_taskq_stack, CONFIG_BT_TASKQ_STACK_SIZE);
struct k_work_q bt_taskq;

static int bt_taskq_init(void)
{
	const struct k_work_queue_config cfg = {
		.name = "bt_taskq",
		/* Enable CONFIG_WORKQUEUE_WORK_TIMEOUT to detect tasks that take too long. */
		.work_timeout_ms = 10,
	};

	k_work_queue_start(&bt_taskq, bt_taskq_stack, K_THREAD_STACK_SIZEOF(bt_taskq_stack),
			   CONFIG_BT_TASKQ_THREAD_PRIO, &cfg);

	return 0;
}

/* The init priority is set to POST_KERNEL 999, the last level
 * before APPLICATION.
 */
SYS_INIT(bt_taskq_init, POST_KERNEL, 999);
