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
#include <kernel_internal.h>

static const struct k_work_queue_config cfg = {
	.name = "sysworkq",
	.no_yield = IS_ENABLED(CONFIG_SYSTEM_WORKQUEUE_NO_YIELD),
	.essential = true,
	.work_timeout_ms = CONFIG_SYSTEM_WORKQUEUE_WORK_TIMEOUT_MS,
};

struct k_work_q k_sys_work_q;

#ifdef CONFIG_SYSTEM_WORKQUEUE_ON_MAIN

void z_sys_work_q_run(void)
{
	/* main() has returned and the main thread is about to become the
	 * system workqueue thread. Switch it from the main thread priority to
	 * the configured system workqueue priority, matching the dedicated
	 * thread case (k_work_queue_start() above).
	 */
	k_thread_priority_set(_current, CONFIG_SYSTEM_WORKQUEUE_PRIORITY);

	k_work_queue_run(&k_sys_work_q, &cfg);
}

static void sys_work_q_init(void)
{
	k_work_queue_init(&k_sys_work_q);

	/* Arm the queue on the main thread now, before main() runs. This marks
	 * it started (so work can be submitted during main()), and records the
	 * main thread as the queue thread, so k_work_queue_thread_get() is
	 * valid and code running in main() can detect it is in the system
	 * workqueue's context. The run loop is entered later, once main()
	 * returns, via z_sys_work_q_run().
	 */
	k_work_queue_prepare(&k_sys_work_q, &cfg);
}

#else /* !CONFIG_SYSTEM_WORKQUEUE_ON_MAIN */

static K_KERNEL_STACK_DEFINE(sys_work_q_stack,
			     CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE);

static void sys_work_q_init(void)
{
	k_work_queue_start(&k_sys_work_q,
			    sys_work_q_stack,
			    K_KERNEL_STACK_SIZEOF(sys_work_q_stack),
			    CONFIG_SYSTEM_WORKQUEUE_PRIORITY, &cfg);
}

#endif /* CONFIG_SYSTEM_WORKQUEUE_ON_MAIN */

/* Registered into the kernel post-init section. The entry lives in this TU,
 * so it (and this init) is linked only when something references the system
 * work queue (e.g. k_sys_work_q), preserving pay-per-use linkage.
 */
K_KERNEL_INIT_POST(sys_work_q_init);
