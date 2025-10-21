/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "battery_backup.h"

#define VCC_DROP_DETECTION_STABILIZATION_WAIT_TIME_US 20
#define VBTBPSR_VBPORF_IS_SET                         BIT(0)
#define VBTBPCR2_VDETLVL_SETTING_NOT_USED             0x6

static uint8_t vbtbpsr_state_at_boot;

bool is_backup_domain_reset_happen(void)
{
	return (vbtbpsr_state_at_boot & VBTBPSR_VBPORF_IS_SET);
}

void battery_backup_init(void)
{
#if DT_NODE_HAS_PROP(DT_NODELABEL(battery_backup), switch_threshold)
	/*  Check VBPORM bit. If VBPORM flag is 0, wait until it changes to 1 */
	while (R_SYSTEM->VBTBPSR_b.VBPORM == 0) {
	}
	vbtbpsr_state_at_boot = R_SYSTEM->VBTBPSR;
	if (R_SYSTEM->VBTBPSR_b.VBPORF == 1) {
		R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);
		R_SYSTEM->VBTBPSR_b.VBPORF = 0;
		R_SYSTEM->VBTBPCR2_b.VDETLVL =
			DT_ENUM_IDX(DT_NODELABEL(battery_backup), switch_threshold);
		R_BSP_SoftwareDelay(VCC_DROP_DETECTION_STABILIZATION_WAIT_TIME_US,
				    BSP_DELAY_UNITS_MICROSECONDS);
		R_SYSTEM->VBTBPCR2_b.VDETE = 1;
		R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);
	}
#else
	/*  Set the BPWSWSTP bit to 1. The power supply switch is stopped */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);
	R_SYSTEM->VBTBPCR1_b.BPWSWSTP = 1;

	/* Check VBPORM flag. If VBPORM flag is 0, wait until it changes to 1 */
	while (R_SYSTEM->VBTBPSR_b.VBPORM == 0) {
	}
	vbtbpsr_state_at_boot = R_SYSTEM->VBTBPSR;
	R_SYSTEM->VBTBPSR_b.VBPORF = 0;
	R_SYSTEM->VBTBPCR2_b.VDETE = 0;
	R_SYSTEM->VBTBPCR2_b.VDETLVL = DT_ENUM_IDX_OR(
		DT_NODELABEL(battery_backup), switch_threshold, VBTBPCR2_VDETLVL_SETTING_NOT_USED);
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);

	/* Set the SOSTP bit to 1 regardless of its value. Stop Sub-Clock Oscillator */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
	R_SYSTEM->SOSCCR_b.SOSTP = 1;
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
#endif /* BATTERY_BACKUP_CONFIGURATION_NOT_USED */
}
