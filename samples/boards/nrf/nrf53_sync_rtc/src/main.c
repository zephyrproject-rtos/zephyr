/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <hal/nrf_ipc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

static void timeout_handler(struct k_timer *timer)
{
	nrf_ipc_task_t task = offsetof(NRF_IPC_Type, TASKS_SEND[2]);
	uint32_t now = sys_clock_tick_get_32();
	uint32_t shared_cell = 0x20070000;

	*(volatile uint32_t *)shared_cell = now;
	nrf_ipc_task_trigger(NRF_IPC, task);

	LOG_INF("IPC send at %d ticks", now);

	/* Do it only for the first second. */
	if (now > sys_clock_hw_cycles_per_sec()) {
		k_timer_stop(timer);
	}
}

K_TIMER_DEFINE(timer, timeout_handler, NULL);

void main(void)
{
	LOG_INF("Synchronization using %s driver", IS_ENABLED(CONFIG_MBOX) ? "mbox" : "ipm");
	k_timer_start(&timer, K_MSEC(50), K_MSEC(50));
}
