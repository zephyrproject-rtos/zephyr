/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define PIO_NODE           DT_NODELABEL(pio)
#define PD_CFG1_OFFSET     0x204U
#define PD14_FUNC_MASK     GENMASK(27, 24)
#define PD15_FUNC_MASK     GENMASK(31, 28)
#define PD14_UART3_TX_FUNC 6U
#define PD15_UART3_RX_FUNC 6U

void board_early_init_hook(void)
{
	if (DT_NODE_HAS_STATUS(DT_NODELABEL(uart3), okay) && DT_NODE_HAS_STATUS(PIO_NODE, okay)) {
		const mem_addr_t reg = DT_REG_ADDR(PIO_NODE) + PD_CFG1_OFFSET;
		uint32_t value = sys_read32(reg);

		/* Select UART3 on PD14 and PD15, preserving the other pins. */
		value = (value & ~(PD14_FUNC_MASK | PD15_FUNC_MASK)) |
			FIELD_PREP(PD14_FUNC_MASK, PD14_UART3_TX_FUNC) |
			FIELD_PREP(PD15_FUNC_MASK, PD15_UART3_RX_FUNC);
		sys_write32(value, reg);
	}
}
