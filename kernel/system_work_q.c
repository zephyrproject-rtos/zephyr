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

/* The section entry lives in this TU, so the system work queue is linked and
 * started only when something references k_sys_work_q (pay-per-use).
 */
Z_WORK_QUEUE_DEFINE(k_sys_work_q, "sysworkq", CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE,
		    CONFIG_SYSTEM_WORKQUEUE_PRIORITY,
		    IS_ENABLED(CONFIG_SYSTEM_WORKQUEUE_NO_YIELD),
		    CONFIG_SYSTEM_WORKQUEUE_WORK_TIMEOUT_MS);
