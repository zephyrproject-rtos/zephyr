/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <ti/driverlib/driverlib.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define MSPM0_HAS_HSCLK DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hsclk))

#if MSPM0_HAS_HSCLK
static DL_SYSCTL_HSCLK_SOURCE saved_hsclk_source;
static bool restore_hsclk_on_wake;
#endif

static void prepare_run_sleep_clock(void)
{
#if MSPM0_HAS_HSCLK
	restore_hsclk_on_wake = DL_SYSCTL_getMCLKSource() == DL_SYSCTL_MCLK_SOURCE_HSCLK;

	if (restore_hsclk_on_wake) {
		saved_hsclk_source = DL_SYSCTL_getHSCLKSource();
		DL_SYSCTL_switchMCLKfromHSCLKtoSYSOSC();
	}
#endif
}

static void restore_run_clock(void)
{
#if MSPM0_HAS_HSCLK
	if (restore_hsclk_on_wake) {
		DL_SYSCTL_switchMCLKfromSYSOSCtoHSCLK(saved_hsclk_source);
		restore_hsclk_on_wake = false;
	}
#endif
}

static void set_mode_run(uint8_t state)
{
	switch (state) {
	case DL_SYSCTL_POWER_POLICY_RUN_SLEEP0:
		DL_SYSCTL_setPowerPolicyRUN0SLEEP0();
		break;
	case DL_SYSCTL_POWER_POLICY_RUN_SLEEP1:
		prepare_run_sleep_clock();
		DL_SYSCTL_setPowerPolicyRUN1SLEEP1();
		break;
	case DL_SYSCTL_POWER_POLICY_RUN_SLEEP2:
		prepare_run_sleep_clock();
		DL_SYSCTL_setPowerPolicyRUN2SLEEP2();
		break;
	}
}

static void set_mode_stop(uint8_t state)
{
	switch (state) {
	case DL_SYSCTL_POWER_POLICY_STOP0:
		DL_SYSCTL_setPowerPolicySTOP0();
		break;
	case DL_SYSCTL_POWER_POLICY_STOP1:
		DL_SYSCTL_setPowerPolicySTOP1();
		break;
	case DL_SYSCTL_POWER_POLICY_STOP2:
		DL_SYSCTL_setPowerPolicySTOP2();
		break;
	}
}

static void set_mode_standby(uint8_t state)
{
	switch (state) {
	case DL_SYSCTL_POWER_POLICY_STANDBY0:
		DL_SYSCTL_setPowerPolicySTANDBY0();
		break;
	case DL_SYSCTL_POWER_POLICY_STANDBY1:
		DL_SYSCTL_setPowerPolicySTANDBY1();
		break;
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
	DL_SYSCTL_setPowerPolicyRUN0SLEEP0();
	restore_run_clock();

	irq_unlock(0);
}
