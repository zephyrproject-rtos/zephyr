/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(ll_sys_if_adapt);

#include "ll_sys.h"

extern struct k_mutex ble_ctlr_stack_mutex;
extern struct k_work_q ll_work_q;
struct k_work ll_sys_work;

void ll_sys_schedule_bg_process(void)
{
	k_work_submit_to_queue(&ll_work_q, &ll_sys_work);
}

void ll_sys_schedule_bg_process_isr(void)
{
	k_work_submit_to_queue(&ll_work_q, &ll_sys_work);
}

static void ll_sys_bg_process_handler(struct k_work *work)
{
	k_mutex_lock(&ble_ctlr_stack_mutex, K_FOREVER);
	ll_sys_bg_process();
	k_mutex_unlock(&ble_ctlr_stack_mutex);
}

void ll_sys_bg_process_init(void)
{
	k_work_init(&ll_sys_work, &ll_sys_bg_process_handler);
}
