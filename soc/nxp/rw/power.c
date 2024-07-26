/*
 * Copyright 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/init.h>

#include "fsl_power.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Active mode */
#define POWER_MODE0		0
/* Idle mode */
#define POWER_MODE1		1
/* Standby mode */
#define POWER_MODE2		2
/* Sleep mode */
#define POWER_MODE3		3
/* Deep Sleep mode */
#define POWER_MODE4		4

#define NODE_ID DT_INST(0, nxp_pdcfg_power)

power_sleep_config_t slp_cfg;

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		POWER_SetSleepMode(POWER_MODE1);
		__WFI();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		POWER_EnterPowerMode(POWER_MODE2, &slp_cfg);
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

static int nxp_rw6xx_power_init(void)
{
	uint32_t suspend_sleepconfig[5] = DT_PROP_OR(NODE_ID, deep_sleep_config, {});

	slp_cfg.pm2MemPuCfg = suspend_sleepconfig[0];
	slp_cfg.pm2AnaPuCfg = suspend_sleepconfig[1];
	slp_cfg.clkGate = suspend_sleepconfig[2];
	slp_cfg.memPdCfg = suspend_sleepconfig[3];
	slp_cfg.pm3BuckCfg = suspend_sleepconfig[4];

	return 0;
}

SYS_INIT(nxp_rw6xx_power_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
