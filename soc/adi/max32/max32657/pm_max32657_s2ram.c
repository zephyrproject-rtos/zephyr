/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <mxc_device.h>
#include <mcr_regs.h>

/* Construct retention mask from SRAM devicetree definition */
#define RET_MASK                                                                                   \
	((DT_PROP(DT_NODELABEL(sram4), adi_ram_bank_retained) << 4) |                              \
	 (DT_PROP(DT_NODELABEL(sram3), adi_ram_bank_retained) << 3) |                              \
	 (DT_PROP(DT_NODELABEL(sram2), adi_ram_bank_retained) << 2) |                              \
	 (DT_PROP(DT_NODELABEL(sram1), adi_ram_bank_retained) << 1) |                              \
	 DT_PROP(DT_NODELABEL(sram0), adi_ram_bank_retained))

uint8_t pm_get_s2ram_retention_mask(void)
{
	return RET_MASK;
}

void __attribute__((naked)) pm_s2ram_mark_set(void)
{
	__asm__ volatile(
		/* Set warm-boot register */
		"str	%[_bypass_val], [%[_byp_reg]]\n"

		"bx	lr\n"
		:
		: [_bypass_val] "r"(MXC_S_MCR_BYPASS0), [_byp_reg] "r"(&MXC_MCR->bypass0)
		: "r1", "r4", "memory");
}

bool __attribute__((naked)) pm_s2ram_mark_check_and_clear(void)
{
	__asm__ volatile(
		/* Set return value to 0 */
		"mov	r0, #0\n"

		/* Check the marker */
		"ldr	r3, [%[_byp_reg]]\n"
		"cmp	r3, %[_bypass_val]\n"
		"bne	exit\n"

		/*
		 * Reset the marker
		 */
		"str	r0, [%[_byp_reg]]\n"

		/*
		 * Set return value to 1
		 */
		"mov	r0, #1\n"

		"exit:\n"
		"bx lr\n"

		:
		: [_bypass_val] "r"(MXC_S_MCR_BYPASS0), [_byp_reg] "r"(&MXC_MCR->bypass0)
		: "r0", "r1", "r3", "r4", "memory");
}
