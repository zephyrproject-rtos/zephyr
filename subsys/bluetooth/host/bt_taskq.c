/* bt_taskq.c - Workqueue for quick non-blocking Bluetooth tasks */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/autoconf.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

static K_THREAD_STACK_DEFINE(bt_taskq_stack, CONFIG_BT_TASKQ_STACK_SIZE);
static struct k_work_q bt_taskq;

__maybe_unused static int bt_taskq_init(void)
{
	struct k_work_queue_config cfg = {};

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		cfg.name = "bt_taskq";
	}

	k_work_queue_start(&bt_taskq, bt_taskq_stack, K_THREAD_STACK_SIZEOF(bt_taskq_stack),
			   CONFIG_BT_TASKQ_THREAD_PRIO, &cfg);

	return 0;
}

#if defined(CONFIG_BT_TASKQ_DEDICATED)
/* The init priority is set to POST_KERNEL 999, the last level
 * before APPLICATION.
 */
SYS_INIT(bt_taskq_init, POST_KERNEL, 999);
#endif /* CONFIG_BT_TASKQ_DEDICATED */

/* Exports */
struct k_work_q *const bt_taskq_chosen =
	COND_CODE_1(CONFIG_BT_TASKQ_DEDICATED, (&bt_taskq), (&k_sys_work_q));
