/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>
#include <DA1469xAB.h>
#include <da1469x_clock.h>
#include <da1469x_sleep.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_STANDBY:
		/* We enter here with interrupts disabled by BASEPRI which prevents wfi from
		 * waking up on interrupts. Need to disable interrupts by PRIMASK instead and
		 * reset BASEPRI to 0.
		 */
		__disable_irq();
		arch_irq_unlock(0);

		da1469x_sleep();

		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_STANDBY) {
		__enable_irq();
	}
}

int renesas_da1469x_pm_init(void)
{
	static const struct da1469x_sleep_config sleep_cfg = {
		.enable_xtal_on_wakeup = DT_NODE_HAS_STATUS(DT_NODELABEL(xtal32m), okay),
	};

	da1469x_sleep_config(&sleep_cfg);

	return 0;
}
