/*
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <hal_ch32fun.h>

#define SYSRESET BIT(31)

void sys_arch_reboot(int type)
{
	PFIC->SCTLR |= SYSRESET;
}
