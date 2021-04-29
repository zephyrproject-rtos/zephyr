/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V reboot interface
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <sys/util.h>

/**
 * @brief Reset the system
 *
 * This is stub function to avoid build error with CONFIG_REBOOT=y
 * RISC-V specification does not have a common interface for system reset.
 * Each RISC-V SoC that has reset feature should implement own reset function.
 *
 * @return N/A
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
