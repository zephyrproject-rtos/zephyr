/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include "fsl_power.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{
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

	switch (info.state) {
	case PM_STATE_RUNTIME_IDLE:
		POWER_EnterSleep();
		break;
	default:
		LOG_DBG("Unsupported power state %u", info.state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	ARG_UNUSED(info);

	/* Clear PRIMASK */
	__enable_irq();
}
