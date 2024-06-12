/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing timer specific definitons for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

#include "timer.h"

static void timer_expiry_function(struct k_work *work)
{
	struct timer_list *timer;

	timer = (struct timer_list *)CONTAINER_OF(work, struct timer_list, work.work);

	timer->function(timer->data);
}

void init_timer(struct timer_list *timer)
{
	k_work_init_delayable(&timer->work, timer_expiry_function);
}

void mod_timer(struct timer_list *timer, int msec)
{
	k_work_schedule(&timer->work, K_MSEC(msec));
}

void del_timer_sync(struct timer_list *timer)
{
	k_work_cancel_delayable(&timer->work);
}
