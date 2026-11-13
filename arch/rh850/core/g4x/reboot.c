/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 reboot interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>

/**
 * @brief Reset the system
 *
 * This is stub function to avoid build error with CONFIG_REBOOT=y
 * RH850 specification does not have a common interface for system reset.
 * Each RH850 SoC that has reset feature should implement own reset function.
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
