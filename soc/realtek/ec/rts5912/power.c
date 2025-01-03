/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>

#include <reg/reg_system.h>
#include "device_power.h"

#define RTS5912_SCCON_REG_BASE ((SYSTEM_Type *)(DT_REG_ADDR(DT_NODELABEL(sccon))))

static void realtek_WFI(void)
{
	__DSB();
	__WFI();
}

static void rts5912_light_sleep(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;

	before_rts5912_sleep();

	sys_reg->SLPCTRL &= ~SYSTEM_SLPCTRL_SLPMDSEL_Msk;

	realtek_WFI();

	after_rts5912_sleep();
}

static void rts5912_heavy_sleep(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;
	int main_clk_src_record = sys_reg->SYSCLK;
	int PLL_en_record = sys_reg->PLLCTRL;

	before_rts5912_sleep();

	if ((main_clk_src_record & SYSTEM_SYSCLK_SRC_Msk) == 0x0) {
		if ((PLL_en_record & SYSTEM_PLLCTRL_EN_Msk) == 0x0) {
			sys_reg->PLLCTRL |= SYSTEM_PLLCTRL_EN_Msk; /* Force to enable PLL */
			while ((sys_reg->PLLCTRL & SYSTEM_PLLCTRL_RDY_Msk) == 0x00) {
				; /* Wait until PLL is ready */
			}
		}
		sys_reg->SYSCLK |= SYSTEM_SYSCLK_SRC_Msk; /* Switch system clock to PLL */
	}

	sys_reg->SLPCTRL |= SYSTEM_SLPCTRL_SLPMDSEL_Msk; /* Heavy Sleep mode */

	realtek_WFI();

	if ((main_clk_src_record & SYSTEM_SYSCLK_SRC_Msk) == 0) {
		sys_reg->SYSCLK &= ~SYSTEM_SYSCLK_SRC_Msk; /* Return system clock to 25M */
		if ((PLL_en_record & SYSTEM_PLLCTRL_EN_Msk) == 0x0) {
			sys_reg->PLLCTRL &= ~SYSTEM_PLLCTRL_EN_Msk; /* Disable PLL */
		}
	}

	after_rts5912_sleep();
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
