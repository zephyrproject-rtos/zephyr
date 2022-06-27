/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

void z_arm64_el3_plat_init(void)
{
	uint64_t reg, val;

	/* Enable access control configuration from lower EL */
	reg = read_actlr_el3();
	reg |= (ACTLR_EL3_L2ACTLR_BIT |
		ACTLR_EL3_L2ECTLR_BIT |
		ACTLR_EL3_L2CTLR_BIT |
		ACTLR_EL3_CPUACTLR_BIT |
		ACTLR_EL3_CPUECTLR_BIT);
	write_actlr_el3(reg);

	reg = read_sysreg(CORTEX_A72_L2ACTLR_EL1);
	reg |= CORTEX_A72_L2ACTLR_DISABLE_ACE_SH_OR_CHI_BIT;
	write_sysreg(reg, CORTEX_A72_L2ACTLR_EL1);

	val = ((CORTEX_A72_L2_DATA_RAM_LATENCY_MASK <<
		CORTEX_A72_L2CTLR_DATA_RAM_LATENCY_SHIFT) |
	       (CORTEX_A72_L2_TAG_RAM_LATENCY_MASK <<
		CORTEX_A72_L2CTLR_TAG_RAM_LATENCY_SHIFT) |
	       (CORTEX_A72_L2_TAG_RAM_SETUP_1_CYCLE <<
		CORTEX_A72_L2CTLR_TAG_RAM_SETUP_SHIFT) |
	       (CORTEX_A72_L2_DATA_RAM_SETUP_1_CYCLE <<
		CORTEX_A72_L2CTLR_DATA_RAM_SETUP_SHIFT));

	reg &= ~val;

	val = ((CORTEX_A72_L2_DATA_RAM_LATENCY_3_CYCLES <<
		CORTEX_A72_L2CTLR_DATA_RAM_LATENCY_SHIFT) |
	       (CORTEX_A72_L2_TAG_RAM_SETUP_1_CYCLE <<
		CORTEX_A72_L2CTLR_TAG_RAM_SETUP_SHIFT) |
	       (CORTEX_A72_L2_DATA_RAM_SETUP_1_CYCLE <<
		CORTEX_A72_L2CTLR_DATA_RAM_SETUP_SHIFT));

	reg |= val;

	write_sysreg(reg, CORTEX_A72_L2CTLR_EL1);

	dsb();
	isb();
}
