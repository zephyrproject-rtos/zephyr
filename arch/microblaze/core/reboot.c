/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/**
 * @brief Reset the system
 *
 * This is stub function to avoid build error with CONFIG_REBOOT=y
 * MicroBlaze, being a soft core cannot define a reboot routine.
 * Each SoC designer should implement their own reboot if needed.
 */

void __weak sys_arch_reboot(int type)
{
	printk("__weak reboot called with %d in %s\n", type, __FILE__);
	__asm__ volatile("\tbraid _start\n"
			 "\tmts rmsr, r0\n");
}
