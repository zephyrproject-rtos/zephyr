/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/poweroff.h>

void sys_poweroff(void)
{
	if (IS_ENABLED(CONFIG_POWEROFF_PREPARE)) {
		(void)z_sys_poweroff_prepare();
	}

	(void)irq_lock();

	z_sys_poweroff();
}

int sys_poweroff_ready(void)
{
	uint32_t key;

	if (IS_ENABLED(CONFIG_POWEROFF_PREPARE)) {
		int err = z_sys_poweroff_prepare();

		if (err < 0) {
			return err;
		}
	}

	key = irq_lock();

	z_sys_poweroff_ready();
	irq_unlock(key);

	if (IS_ENABLED(CONFIG_POWEROFF_PREPARE)) {
		int err = z_sys_poweroff_unprepare();

		if (err < 0) {
			return err;
		}
	}

	return 0;
}
