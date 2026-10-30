/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	(void)arch_irq_lock();
	for (;;) {
		__asm__ volatile("halt");
	}
}
