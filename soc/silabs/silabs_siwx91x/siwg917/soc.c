/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sw_isr_table.h>

#include "em_device.h"
#include "power.h"
#include "sl_si91x_hal_soc_soft_reset.h"


void soc_early_init_hook(void)
{
	SystemInit();

	if (IS_ENABLED(CONFIG_PM)) {
		siwx91x_power_init();
	}
}

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	sl_si91x_soc_nvic_reset();
}

/* SiWx917's bootloader requires IRQn 32 to hold payload's entry point address. */
extern void z_arm_reset(void);
Z_ISR_DECLARE_DIRECT(32, 0, z_arm_reset);
