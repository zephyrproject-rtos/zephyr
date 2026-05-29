/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <r_lpc_rx_if.h>

#ifdef CONFIG_RENESAS_RX_DTC
#include <zephyr/drivers/misc/renesas_rx_dtc/renesas_rx_dtc.h>
#endif

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#ifdef CONFIG_RENESAS_RX_DTC
static const struct device *const dtc = DEVICE_DT_GET(DT_NODELABEL(dtc));
#endif

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	int err = 0;

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		err = R_LPC_LowPowerModeConfigure(LPC_LP_SLEEP);
		if (err) {
			LOG_DBG("LPC config failed %d", err);
		}
		err = R_LPC_LowPowerModeActivate(FIT_NO_FUNC);
		if (err) {
			LOG_DBG("LPC active failed %d", err);
		}
		break;
	case PM_STATE_STANDBY:
#ifdef CONFIG_RENESAS_RX_DTC
		err = dtc_renesas_rx_off(dtc);
		if (err) {
			LOG_DBG("turn off module DTC failed %d", err);
		}
#endif
		err = R_LPC_LowPowerModeConfigure(LPC_LP_DEEP_SLEEP);
		if (err) {
			LOG_DBG("LPC config failed %d", err);
		}
		err = R_LPC_LowPowerModeActivate(FIT_NO_FUNC);
		if (err) {
			LOG_DBG("LPC active failed %d", err);
		}
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_STANDBY) {
#ifdef CONFIG_RENESAS_RX_DTC
		int err = dtc_renesas_rx_on(dtc);

		if (err) {
			LOG_DBG("turn off module DTC failed %d", err);
		}
#endif
	}
}
