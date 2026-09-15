/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/ppc/ppc.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <stdint.h>

/* SPCTRL register offsets */
#define SPCTRL_NSCCFG       0x014U
#define SPCTRL_APBNSPPCEXP1 0x084U

#define SPCTRL_NODE DT_INST(0, arm_sse_200_spctrl)

static int arm_sse200_spctrl_init(void)
{
	uintptr_t base = (uintptr_t)DT_REG_ADDR(SPCTRL_NODE);
	uint32_t nsccfg = DT_PROP(SPCTRL_NODE, nsccfg);

	if (nsccfg != 0U) {
		*(volatile uint32_t *)(base + SPCTRL_NSCCFG) |= nsccfg;
	}

	return 0;
}

SYS_INIT(arm_sse200_spctrl_init, PRE_KERNEL_1, 0);

void ppc_configure_ns_all(void)
{
	uintptr_t base = (uintptr_t)DT_REG_ADDR(SPCTRL_NODE);
	uint32_t mask = DT_PROP(SPCTRL_NODE, apbnsppcexp1_ns_mask);

	if (mask != 0U) {
		*(volatile uint32_t *)(base + SPCTRL_APBNSPPCEXP1) = mask;
	}
}
