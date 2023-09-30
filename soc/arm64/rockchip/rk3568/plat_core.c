/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/sys_io.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/interrupt_controller/gic.h>

/* For get GICv3 address */
#include "../../../../drivers/interrupt_controller/intc_gicv3_priv.h"

void z_arm64_el3_plat_init(void)
{
	uint64_t reg = 0;

	reg = (ICC_SRE_ELx_DFB_BIT | ICC_SRE_ELx_DIB_BIT |
	       ICC_SRE_ELx_SRE_BIT | ICC_SRE_EL3_EN_BIT);

	write_sysreg(reg, ICC_SRE_EL3);

	/* Init GICv3 ctrl register for NS group1 route enable*/
#ifdef CONFIG_ARMV8_A_NS
	/* set DS bit to enable NS mode */
	sys_write32(BIT(GICD_CTRL_NS), GICD_CTLR);

	/* Direct write to GICD_CTRL_ARE_NS may have some unpredictable value */
	sys_write32(BIT(GICD_CTRL_NS)|BIT(GICD_CTRL_ARE_NS), GICD_CTLR);
#endif
}
