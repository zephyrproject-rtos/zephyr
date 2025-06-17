/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RX reboot interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>

/**
 * @brief Reset the system
 *
 * This is stub function to avoid build error with CONFIG_REBOOT=y
 * RX specification does not have a common interface for system reset.
 * Each RX SoC that has reset feature should implement own reset function.
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
