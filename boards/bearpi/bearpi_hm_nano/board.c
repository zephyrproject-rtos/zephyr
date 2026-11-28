/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/sys_io.h>
#include <soc.h>

void board_early_init_hook(void)
{
	sys_write32(HI3861_IOCFG_GPIO3_UART0_TX, HI3861_IOCFG_GPIO3_SELECT_REG);
	sys_write32(HI3861_IOCFG_GPIO4_UART0_RX, HI3861_IOCFG_GPIO4_SELECT_REG);
}
