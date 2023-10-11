/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

void main(void)
{
	int cnt = 0;

	while (1) {
		LOG_INF("loop: %d", cnt++);
		k_sleep(K_MSEC(500));
	}
}
