/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/math/ilog2.h>

#include "wdt_dw_common.h"
#include "wdt_dw.h"

LOG_MODULE_REGISTER(wdt_dw_common, CONFIG_WDT_LOG_LEVEL);

#define WDT_DW_FLAG_CONFIGURED	0x80000000

int dw_wdt_check_options(const uint8_t options)
{
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		LOG_WRN("Pausing watchdog by debugger is not configurable");
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_WRN("Pausing watchdog in sleep is not configurable");
	}

	return 0;
}

int dw_wdt_configure(const uint32_t base, const uint32_t config)
{
	uint32_t period;

	if (!(config & WDT_DW_FLAG_CONFIGURED)) {
		LOG_ERR("Timeout not installed.");
		return -ENOTSUP;
	}

	/* Configure timeout */
	period = config & ~WDT_DW_FLAG_CONFIGURED;

	if (dw_wdt_dual_timeout_period_get(base)) {
		dw_wdt_timeout_period_init_set(base, period);
	}

	dw_wdt_timeout_period_set(base, period);

	/* Enable watchdog */
	dw_wdt_enable(base);
	dw_wdt_counter_restart(base);

	return 0;
}

int dw_wdt_calc_period(const uint32_t base, const uint32_t clk_freq,
		       const struct wdt_timeout_cfg *config, uint32_t *period_out)
{
	uint64_t period64;
	uint32_t period;

	/* Window timeout is not supported by this driver */
	if (config->window.min) {
		LOG_ERR("Window timeout is not supported.");
		return -ENOTSUP;
	}

	period64 = (uint64_t)clk_freq * config->window.max;
	period64 /= 1000;
	if (!period64 || (period64 >> 31)) {
		LOG_ERR("Window max is out of range.");
		return -EINVAL;
	}

	period = period64 - 1;
	period = ilog2(period);

	if (period >= dw_wdt_cnt_width_get(base)) {
		LOG_ERR("Watchdog timeout too long.");
		return -EINVAL;
	}

	/* The minimum counter value is 64k, maximum 2G */
	*period_out = WDT_DW_FLAG_CONFIGURED | (period >= 15 ? period - 15 : 0);
	return 0;
}

int dw_wdt_probe(const uint32_t base, const uint32_t reset_pulse_length)
{
	/* Check component type */
	const uint32_t type = dw_wdt_comp_type_get(base);

	if (type != WDT_COMP_TYPE_VALUE) {
		LOG_ERR("Invalid component type %x", type);
		return -ENODEV;
	}

	dw_wdt_reset_pulse_length_set(base, reset_pulse_length);

	return 0;
}
