/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sw_isr_table.h>

#include <em_device.h>
#include "power.h"
#include <sl_si91x_hal_soc_soft_reset.h>
#include <rsi_ipmu.h>
#include <rsi_rom_ulpss_clk.h>

void soc_early_init_hook(void)
{
	SystemInit();

	/* Remaining of SystemCoreClockUpdate() that is done in siwx91x clock manager init */

	RSI_Ipmu_Init();

	/* Configuring the ULP reference clock to 40MHz, as this frequency is required by the
	 * temperature sensor for chip supply mode configuration.
	 */
	MCU_FSM->MCU_FSM_REF_CLK_REG_b.ULPSS_REF_CLK_SEL_b = ULPSS_40MHZ_CLK;

	RSI_Configure_Ipmu_Mode();

	/* NWP clock is selected as 40MHZ clock from MCU */
	MCU_FSM->MCU_FSM_REF_CLK_REG_b.TASS_REF_CLK_SEL = ULP_MHZ_RC_CLK;

	/* Configuring MCU FSM clock for BG_PMU */
	RSI_IPMU_ClockMuxSel(2);

	/* XTAL control pointed to Software and  XTAL is Turned-Off from M4 */
	/* old RSI_ConfigXtal*/
	BATT_FF->MCU_FSM_CTRL_BYPASS_b.MCU_XTAL_EN_40MHZ_BYPASS = XTAL_DISABLE_FROM_M4 ;
	BATT_FF->MCU_FSM_CTRL_BYPASS_b.MCU_XTAL_EN_40MHZ_BYPASS_CTRL = XTAL_IS_IN_SW_CTRL_FROM_M4;

	/* Before NWP is going to power save mode ,set m4ss_ref_clk_mux_ctrl ,
	 * tass_ref_clk_mux_ctrl, AON domain power supply controls from NWP to M4
	 */
	RSI_Set_Cntrls_To_M4();

	/* End of remaining of SystemCoreClockUpdate() that is done in siwx91x clock manager init */

	if (IS_ENABLED(CONFIG_PM)) {
		siwx91x_power_init();
	}
}

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	sl_si91x_soc_nvic_reset();
}

/* SiWx917's bootloader requires IRQn 32 to hold payload's entry point address. */
extern void z_arm_reset(void);
Z_ISR_DECLARE_DIRECT(32, 0, z_arm_reset);
