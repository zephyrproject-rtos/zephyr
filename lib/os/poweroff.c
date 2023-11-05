/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/wakeup_source.h>

void sys_poweroff(void)
{
	(void)irq_lock();

#ifdef CONFIG_WAKEUP_SOURCE
	z_sys_wakeup_sources_configure();
#endif

	z_sys_poweroff();
}
