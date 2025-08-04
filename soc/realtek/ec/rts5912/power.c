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

#ifdef RTS5912_UART_WAKE_UP
#include <reg/reg_gpio.h>
#include <soc.h>
#define REG_USR           0x7C
#define USR_BUSY          0x01
#define DT_DRV_COMPAT     realtek_rts5912_uart
#define RTS5912_RX_WAKEUP (DT_PROP(DT_INST(0, realtek_rts5912_uart), rx_wakeup_pin))
#define RTS5912_UART_RX_GPIO                                                                       \
	((volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(gpioa)) + RTS5912_RX_WAKEUP * 4))
#define RTS5912_UART_USR ((volatile uint32_t *)(DT_REG_ADDR(DT_NODELABEL(uart0)) + REG_USR))
#endif

static void realtek_WFI(void)
{
	__DSB();
	__WFI();
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

	sys_reg->SLPCTRL |= SYSTEM_SLPCTRL_SLPMDSEL_Msk;

#ifdef RTS5912_UART_WAKE_UP

	sys_reg->SLPCTRL |= SYSTEM_SLPCTRL_GPIOWKEN_Msk;

	/* Check if UART busy and wait */
	if (!(*(RTS5912_UART_USR) & (USR_BUSY))) {
		const struct pinctrl_dev_config *pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

		pinctrl_apply_state(*pcfg, PINCTRL_STATE_SLEEP);

		realtek_WFI();

		pinctrl_apply_state(*pcfg, PINCTRL_STATE_DEFAULT);

		if ((*RTS5912_UART_RX_GPIO & GPIO_GCR_INTSTS_Msk)) {
			k_timeout_t delay = K_MSEC(15000);

			pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
			k_work_reschedule(&rx_refresh_timeout_work, delay);
		}

		*RTS5912_UART_RX_GPIO |= GPIO_GCR_INTSTS_Msk;
		NVIC_ClearPendingIRQ(113);

	} else {
		k_timeout_t delay = K_MSEC(15000);

		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		k_work_reschedule(&rx_refresh_timeout_work, delay);
	}
#else
	realtek_WFI();
#endif

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
}
