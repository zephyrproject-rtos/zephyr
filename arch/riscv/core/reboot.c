/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V reboot interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/reboot.h>

/**
 * @brief Reset the system
 *
 * This is stub function to avoid build error with CONFIG_REBOOT=y
 * RISC-V specification does not have a common interface for system reset.
 * Each RISC-V SoC that has reset feature should implement own reset function.
 */

void z_sys_reboot(enum sys_reboot_mode mode)
{
	ARG_UNUSED(mode);
}
