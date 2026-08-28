/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmsis_core.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/logging/log.h>

#include <soc_sysctl.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

static const struct device *const ckm_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
static const struct device *const sysctl_dev = DEVICE_DT_GET(DT_NODELABEL(sysctl));

static int mclk_set_source(enum mspm0_clock_source source)
{
	struct mspm0_sys_clock mclk = {.clk = MSPM0_CLOCK_MCLK};

	return clock_control_configure(ckm_dev, (clock_control_subsys_t)&mclk, &source);
}

static int sysosc_set_enabled(bool enable)
{
	struct mspm0_sys_clock sysosc = {.clk = MSPM0_CLOCK_SYSOSC};

	if (enable) {
		return clock_control_on(ckm_dev, (clock_control_subsys_t)&sysosc);
	}

	return clock_control_off(ckm_dev, (clock_control_subsys_t)&sysosc);
}

static void set_mode_run(uint8_t state)
{
	switch (state) {
	case 1: /* RUN0/SLEEP0: MCLK from SYSOSC */
		sysosc_set_enabled(true);
		mclk_set_source(MSPM0_CLOCK_SRC_SYSOSC);
		break;
	case 2: /* RUN1/SLEEP1: MCLK from LFCLK, SYSOSC stays on */
		sysosc_set_enabled(true);
		mclk_set_source(MSPM0_CLOCK_SRC_LFCLK);
		break;
	case 3: /* RUN2/SLEEP2: MCLK from LFCLK, SYSOSC off */
		mclk_set_source(MSPM0_CLOCK_SRC_LFCLK);
		sysosc_set_enabled(false);
		break;
	default:
		return;
	}

	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
}

static void set_mode_stop(uint8_t state)
{
	syscon_write_reg(sysctl_dev, SYSCTL_PMODECFG_OFFSET, SYSCTL_PMODECFG_DSLEEP_VAL_STOP);
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	switch (state) {
	case 1: /* STOP0: SYSOSC keeps running */
		syscon_update_bits(sysctl_dev, SYSCTL_SYSOSCCFG_OFFSET,
				   SYSCTL_SYSOSCCFG_USE4MHZSTOP | SYSCTL_SYSOSCCFG_DISABLESTOP, 0);
		break;
	case 2: /* STOP1: SYSOSC gear-shifted to 4 MHz */
		syscon_update_bits(sysctl_dev, SYSCTL_SYSOSCCFG_OFFSET,
				   SYSCTL_SYSOSCCFG_USE4MHZSTOP | SYSCTL_SYSOSCCFG_DISABLESTOP,
				   SYSCTL_SYSOSCCFG_USE4MHZSTOP);
		break;
	case 3: /* STOP2: SYSOSC disabled */
		syscon_update_bits(sysctl_dev, SYSCTL_SYSOSCCFG_OFFSET,
				   SYSCTL_SYSOSCCFG_USE4MHZSTOP | SYSCTL_SYSOSCCFG_DISABLESTOP,
				   SYSCTL_SYSOSCCFG_DISABLESTOP);
		break;
	default:
		return;
	}
}

static void set_mode_standby(uint8_t state)
{
	syscon_write_reg(sysctl_dev, SYSCTL_PMODECFG_OFFSET, SYSCTL_PMODECFG_DSLEEP_VAL_STANDBY);
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	switch (state) {
	case 1: /* STANDBY0: ULPCLK/LFCLK stays available to all PD0 peripherals */
		syscon_update_bits(sysctl_dev, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_STOPCLKSTBY,
				   0);
		break;
	case 2: /* STANDBY1: ULPCLK/LFCLK gated to PD0, only TIMG0/1 and RTC clocked */
		syscon_update_bits(sysctl_dev, SYSCTL_MCLKCFG_OFFSET, SYSCTL_MCLKCFG_STOPCLKSTBY,
				   SYSCTL_MCLKCFG_STOPCLKSTBY);
		break;
	default:
		return;
	}
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		set_mode_run(substate_id);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		set_mode_stop(substate_id);
		break;
	case PM_STATE_STANDBY:
		set_mode_standby(substate_id);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}

	__WFI();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	set_mode_run(1);
	irq_unlock(0);
}
