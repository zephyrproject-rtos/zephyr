/*
 * Copyright (c) 2025 Ambiq LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/init.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	__disable_irq();
	__set_BASEPRI(0);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE: {
		/* Put ARM core to normal sleep. */
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
		break;
	}
	case PM_STATE_SUSPEND_TO_RAM: {
		/* Put ARM core to deep sleep. */
		/* Cotex-m: power down, register value preserve.*/
		/* Cache: power down*/
		/* MRAM: power down*/
		/* ITCM + DTCM: retention, active on request*/
		/* Sram: retention, active on request*/
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
		break;
	}
	default: {
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
	}
}

/**
 * @brief PM State Exit Post Operations
 *
 * For PM_STATE_SUSPEND_TO_IDLE:
 *   Nothing is needed after soc woken up.
 *
 * For PM_STATE_SUSPEND_TO_RAM:
 *   Flash, cache, sram automatically switch
 *   to active state on wake up
 *
 * @param state PM State
 * @param substate_id Unused
 *
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	__enable_irq();
	irq_unlock(0);
}
