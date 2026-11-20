/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#define VIRT_CTRL_CMD_REG   0xff009004U
#define VIRT_CTRL_CMD_RESET 1U

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	sys_write32(VIRT_CTRL_CMD_RESET, VIRT_CTRL_CMD_REG);
}
