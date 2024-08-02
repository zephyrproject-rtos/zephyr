/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT litex_soc_controller

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/reboot.h>
#include <soc.h>

#define LITEX_CTRL_RESET DT_INST_REG_ADDR_BY_NAME(0, reset)

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	/* SoC Reset on BIT(0)*/
	litex_write8(BIT(0), LITEX_CTRL_RESET);
}
