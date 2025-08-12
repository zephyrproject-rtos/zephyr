/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include "app_conf.h"
#include "ll_intf.h"

struct k_work_q ll_work_q;

uint8_t ll_state_busy;

#define LL_TASK_STACK_SIZE (3072)
#define LL_TASK_PRIO (14)

K_THREAD_STACK_DEFINE(ll_work_area, LL_TASK_STACK_SIZE);

static int stm32wba_ll_workq_init(void)
{
	k_work_queue_init(&ll_work_q);
	k_work_queue_start(&ll_work_q, ll_work_area,
				K_THREAD_STACK_SIZEOF(ll_work_area),
				LL_TASK_PRIO, NULL);

	return 0;
}

SYS_INIT(stm32wba_ll_workq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
