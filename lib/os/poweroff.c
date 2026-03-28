/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/poweroff.h>

void sys_poweroff(void)
{
	(void)irq_lock();

#if defined(CONFIG_ZERO_LATENCY_IRQS)
	(void)arch_zli_lock();
#endif /* CONFIG_ZERO_LATENCY_IRQS */

	z_sys_poweroff();
}
