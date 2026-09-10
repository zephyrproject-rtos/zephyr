/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OpenRISC reboot interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>

/**
 * @brief Reset the system
 *
 * This is stub function to avoid build error with CONFIG_REBOOT=y
 * OpenRISC specification does not have a common interface for system reset.
 * Each OpenRISC SoC that has reset feature should implement own reset function.
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	__asm__("l.nop 13");
}
