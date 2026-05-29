/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "bflb_soc.h"
#include "hbn_reg.h"

void board_early_init_hook(void)
{
	/* Enable JTAG on broken out pins */
	/* Clear HBN pin control */
	*(volatile uint32_t *)(HBN_BASE + HBN_PAD_CTRL_0_OFFSET) &= HBN_REG_EN_AON_CTRL_GPIO_UMSK;

	/* Reset Pin 0,1,2 and 7 to GPIO */
	*(volatile uint32_t *)(0x40000100) =
		(*(volatile uint32_t *)(0x40000100) & ~0x1F00U) | 0xBU << 8;
	*(volatile uint32_t *)(0x40000100) =
		(*(volatile uint32_t *)(0x40000100) & ~0x1F000000U) | 0xBU << 24;
	*(volatile uint32_t *)(0x40000104) =
		(*(volatile uint32_t *)(0x40000104) & ~0x1F00U) | 0xBU << 8;
	*(volatile uint32_t *)(0x4000010c) =
		(*(volatile uint32_t *)(0x4000010c) & ~0x1F000000U) | 0xBU << 24;

	/* Set Pins 17,18,19 and 20 to JTAG and configure
	 * GLB_BASE + GLB_GPIO_CFGCTL*_OFFSET...
	 */
	*(volatile uint32_t *)(0x40000120) =
		(*(volatile uint32_t *)(0x40000120) & ~0x1F000000U) | 0xEU << 24;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x1F00U) | 0xEU << 8;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x1F000000U) | 0xEU << 24;
	*(volatile uint32_t *)(0x40000128) =
		(*(volatile uint32_t *)(0x40000128) & ~0x1F00U) | 0xEU << 8;

	*(volatile uint32_t *)(0x40000120) =
		(*(volatile uint32_t *)(0x40000120) & ~0x100000U) | 1U << 20;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x10U) | 1U << 4;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x100000U) | 1U << 20;
	*(volatile uint32_t *)(0x40000128) =
		(*(volatile uint32_t *)(0x40000128) & ~0x10U) | 1U << 4;

	*(volatile uint32_t *)(0x40000120) =
		(*(volatile uint32_t *)(0x40000120) & ~0x20000U) | 1U << 17;
	*(volatile uint32_t *)(0x40000124) = (*(volatile uint32_t *)(0x40000124) & ~0x2U) | 1U << 1;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x20000U) | 1U << 17;
	*(volatile uint32_t *)(0x40000128) = (*(volatile uint32_t *)(0x40000128) & ~0x2U) | 1U << 1;

	*(volatile uint32_t *)(0x40000120) =
		(*(volatile uint32_t *)(0x40000120) & ~0x10000U) | 1U << 16;
	*(volatile uint32_t *)(0x40000124) = (*(volatile uint32_t *)(0x40000124) & ~0x1U) | 1U;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x10000U) | 1U << 16;
	*(volatile uint32_t *)(0x40000128) = (*(volatile uint32_t *)(0x40000128) & ~0x1U) | 1U;

	*(volatile uint32_t *)(0x40000120) =
		(*(volatile uint32_t *)(0x40000120) & ~0x80000000U) | 1U << 31;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x8000U) | 1U << 15;
	*(volatile uint32_t *)(0x40000124) =
		(*(volatile uint32_t *)(0x40000124) & ~0x80000000U) | 1U << 31;
	*(volatile uint32_t *)(0x40000128) =
		(*(volatile uint32_t *)(0x40000128) & ~0x8000U) | 1U << 15;
}
