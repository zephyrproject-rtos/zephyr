/*
 * SPDX-FileCopyrightText: 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <cmsis_core.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
	case PM_STATE_SUSPEND_TO_IDLE: {
		unsigned int key = __get_PRIMASK();

		/* WFI wakes on a pending IRQ even with PRIMASK set, so masking here
		 * keeps the wake ISR from running before the PM core has finished
		 * resuming. Restored right after, since pm_state_exit_post_ops()
		 * must not unmask interrupts.
		 */
		__disable_irq();
		__WFI();
		__set_PRIMASK(key);
		break;
	}
	default:
		LOG_DBG("PM state not supported: %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}
