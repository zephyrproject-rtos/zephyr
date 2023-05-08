/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include "fsl_power.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*!< Power down all unnecessary blocks */
#define NODE_ID DT_INST(0, nxp_pdcfg_power)
#define EXCLUDE_FROM_DEEPSLEEP ((const uint32_t[]) \
					DT_PROP_OR(NODE_ID, deep_sleep_config, {}))

#define EXCLUDE_FROM_DEEP_POWERDOWN ((const uint32_t[]){0, 0, 0, 0})

static uint32_t isp_pin[3];

/* System clock frequency. */
extern uint32_t SystemCoreClock;

__ramfunc void set_deepsleep_pin_config(void)
{
	/* Backup Pin configuration. */
	isp_pin[0] = IOPCTL->PIO[1][15];
	isp_pin[1] = IOPCTL->PIO[3][28];
	isp_pin[2] = IOPCTL->PIO[3][29];

	/* Disable ISP Pin pull-ups and input buffers to avoid current leakage */
	IOPCTL->PIO[1][15] = 0;
	IOPCTL->PIO[3][28] = 0;
	IOPCTL->PIO[3][29] = 0;
}

__ramfunc void restore_deepsleep_pin_config(void)
{
	/* Restore the Pin configuration. */
	IOPCTL->PIO[1][15] = isp_pin[0];
	IOPCTL->PIO[3][28] = isp_pin[1];
	IOPCTL->PIO[3][29] = isp_pin[2];
}

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* FIXME: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue and disable interrupts using PRIMASK register as recommended
	 * by ARM.
	 */

	/* Set PRIMASK */
	__disable_irq();

	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		POWER_EnterSleep();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		set_deepsleep_pin_config();
		POWER_EnterDeepSleep(EXCLUDE_FROM_DEEPSLEEP);
		restore_deepsleep_pin_config();
		break;
	case PM_STATE_SOFT_OFF:
		set_deepsleep_pin_config();
		POWER_EnterDeepPowerDown(EXCLUDE_FROM_DEEP_POWERDOWN);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/* Clear PRIMASK */
	__enable_irq();
}

/* Initialize power system */
static int rt5xx_power_init(void)
{
	int ret = 0;

	/* This function is called to set vddcore low voltage detection
	 * falling trip voltage, this is not impacting the voltage in anyway.
	 */
	POWER_SetLdoVoltageForFreq(SystemCoreClock, 0);

#if CONFIG_REGULATOR
	/* Indicate to power library that PMIC is used. */
	POWER_UpdatePmicRecoveryTime(1);
#endif

	return ret;
}

SYS_INIT(rt5xx_power_init, PRE_KERNEL_2, 0);
