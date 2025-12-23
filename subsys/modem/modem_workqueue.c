/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include "modem_workqueue.h"

static struct k_work_q modem_work_q;
static K_THREAD_STACK_DEFINE(modem_stack_area, CONFIG_MODEM_DEDICATED_WORKQUEUE_STACK_SIZE);

int modem_work_submit(struct k_work *work)
{
	return k_work_submit_to_queue(&modem_work_q, work);
}

int modem_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_schedule_for_queue(&modem_work_q, dwork, delay);
}

int modem_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_reschedule_for_queue(&modem_work_q, dwork, delay);
}

static int modem_work_q_init(void)
{
	/* Boot the dedicated workqueue */
	k_work_queue_init(&modem_work_q);
	k_work_queue_start(&modem_work_q, modem_stack_area, K_THREAD_STACK_SIZEOF(modem_stack_area),
			   CONFIG_MODEM_DEDICATED_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(k_work_queue_thread_get(&modem_work_q), "modem_workq");
	return 0;
}

SYS_INIT(modem_work_q_init, POST_KERNEL, 0);
