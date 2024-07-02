/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>

#include <reg/reg_system.h>
#include "device_power.h"

#define RTS5912_SCCON_REG_BASE ((SYSTEM_Type *)(DT_REG_ADDR(DT_NODELABEL(sccon))))

static void rts5912_light_sleep(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;

	rts5912_light_sleep_close_module();

	sys_reg->SLPCTRL &= ~SYSTEM_SLPCTRL_SLPMDSEL_Msk;
	k_cpu_idle();

	rts5912_light_sleep_open_module();
}

static void rts5912_heavy_sleep(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;
	int main_clk_src_record = sys_reg->SYSCLK;
	int PLL_en_record = sys_reg->PLLCTRL;

	rts5912_deep_sleep_close_module();

	if ((main_clk_src_record & SYSTEM_SYSCLK_SRC_Msk) == 0x0) {
		if ((PLL_en_record & SYSTEM_PLLCTRL_EN_Msk) == 0x0) {
			sys_reg->PLLCTRL |= SYSTEM_PLLCTRL_EN_Msk; // Force to enable PLL
			while ((sys_reg->PLLCTRL & SYSTEM_PLLCTRL_RDY_Msk) == 0x00)
				; // Wait until PLL is ready
		}
		sys_reg->SYSCLK |= SYSTEM_SYSCLK_SRC_Msk; // Switch system clock to PLL
	}

	sys_reg->SLPCTRL |= SYSTEM_SLPCTRL_SLPMDSEL_Msk; // Heavy Sleep mode
	k_cpu_idle();

	if ((main_clk_src_record & SYSTEM_SYSCLK_SRC_Msk) == 0) {
		sys_reg->SYSCLK &= ~SYSTEM_SYSCLK_SRC_Msk; // Return system clock to 25M
		if ((PLL_en_record & SYSTEM_PLLCTRL_EN_Msk) == 0x0) {
			sys_reg->PLLCTRL &= ~SYSTEM_PLLCTRL_EN_Msk; // Disable PLL
		}
	}

	rts5912_deep_sleep_open_module();
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		rts5912_light_sleep();
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		rts5912_heavy_sleep();
		break;
	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	irq_unlock(0);
}
