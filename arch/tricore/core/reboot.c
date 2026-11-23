/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore reboot interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>

/**
 * @brief Reset the system
 *
 * This is stub function to avoid build error with CONFIG_REBOOT=y
 * Each SoC should implement its own reset function.
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
