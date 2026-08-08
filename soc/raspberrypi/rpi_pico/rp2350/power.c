/*
 * Copyright (c) 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/pm/pm.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/logging/log.h>

#include <cmsis_core.h>

#include <hardware/clocks.h>
#include <hardware/pll.h>
#include <hardware/xosc.h>
#include <hardware/powman.h>
#include <hardware/regs/powman.h>
#include <hardware/structs/powman.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

static int rp2350_s2ram_off(void)
{
	powman_power_state state = POWMAN_POWER_STATE_NONE;

	state = powman_power_state_with_domain_on(state, POWMAN_POWER_DOMAIN_XIP_CACHE);
	state = powman_power_state_with_domain_on(state, POWMAN_POWER_DOMAIN_SRAM_BANK0);
	state = powman_power_state_with_domain_on(state, POWMAN_POWER_DOMAIN_SRAM_BANK1);

	powman_set_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);

	if (powman_set_power_state(state) != PICO_OK) {
		LOG_WRN("state req rejected: current_pwrup_req=0x%02x dbg_pwrcfg=0x%02x",
			powman_hw->current_pwrup_req, powman_hw->dbg_pwrcfg);
		powman_clear_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);
		return -EBUSY;
	}

	__DSB();
	__WFI();

	/* Only reached if the power-down did not take effect. */
	powman_clear_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);
	return -EBUSY;
}

static void rp2350_restore_clocks(void)
{
	xosc_init();
	clock_configure(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC, 0,
			XOSC_HZ, XOSC_HZ);
	pll_init(pll_sys, PLL_SYS_REFDIV, PLL_SYS_VCO_FREQ_HZ, PLL_SYS_POSTDIV1,
		 PLL_SYS_POSTDIV2);
	clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
			CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, SYS_CLK_HZ,
			SYS_CLK_HZ);
	clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
			SYS_CLK_HZ, SYS_CLK_HZ);
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
	case PM_STATE_SUSPEND_TO_IDLE:
		__disable_irq();
		__WFI();
		break;
	case PM_STATE_SUSPEND_TO_RAM: {
		int ret = arch_pm_s2ram_suspend(rp2350_s2ram_off);

		if (ret < 0) {
			LOG_WRN("SUSPEND_TO_RAM not entered: %d", ret);
		}
		break;
	}
	default:
		LOG_DBG("PM state not supported: %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_SUSPEND_TO_RAM) {
		/* Cleared here, not in rp2350_s2ram_off(): a real wake resumes past
		 * arch_pm_s2ram_suspend(), never back into that function.
		 */
		powman_clear_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);

		rp2350_restore_clocks();
	}

	__enable_irq();
}
