/*
 * Copyright (c) 2024 Texas Instruments Inc.
 * Copyright (c) 2025 Linumiz.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

/* Driverlib includes */
#include <ti/driverlib/dl_wwdt.h>

#define DT_DRV_COMPAT ti_mspm0_watchdog

LOG_MODULE_REGISTER(wdt_mspm0, CONFIG_WDT_LOG_LEVEL);

struct wwdt_mspm0_config {
	WWDT_Regs *base;
	uint8_t reset_action;
};

struct wwdt_mspm0_data {
	uint8_t period_count;
	uint8_t clock_divider;
	uint16_t window_count;
};

struct wwdt_period_lut {
	uint8_t period_count;
	uint32_t max_msec;
	uint32_t interval;
};

static int wwdt_mspm0_calculate_timeout_periods(const struct device *dev,
						const struct wdt_timeout_cfg *cfg)
{
	struct wwdt_mspm0_data *data = dev->data;
	struct wwdt_period_lut *lut_entry = NULL;
	uint32_t max_ms = cfg->window.max;
	uint32_t min_ms = cfg->window.min;
	uint32_t actual_timeout;
	uint8_t window_idx;

	struct wwdt_period_lut period_lut[] = {
		/* Timer_max_period_count,	 max_timeout(ms),   interval(ms) */
		{DL_WWDT_TIMER_PERIOD_6_BITS,	       16,		2},
		{DL_WWDT_TIMER_PERIOD_8_BITS,	       64,		8},
		{DL_WWDT_TIMER_PERIOD_10_BITS,	       256,		32},
		{DL_WWDT_TIMER_PERIOD_12_BITS,	       1000,		125},
		{DL_WWDT_TIMER_PERIOD_15_BITS,	       8000,		1000},
		{DL_WWDT_TIMER_PERIOD_18_BITS,	       64000,		8000},
		{DL_WWDT_TIMER_PERIOD_21_BITS,	       512000,		64000},
		{DL_WWDT_TIMER_PERIOD_25_BITS,	       8192000,		1024000}
	};

	uint8_t window_sixteenths[] = {0, 2, 3, 4, 8, 12, 13, 14};

	if (max_ms > period_lut[7].max_msec || min_ms >= max_ms) {
		LOG_ERR("Install timeout failed. Invalid window timing");
		return -EINVAL;
	}

	/* Find appropriate period count (PER_count) */
	for (uint8_t i = 0; i < 8; i++) {
		if (max_ms <= period_lut[i].max_msec) {
			lut_entry = &period_lut[i];
			break;
		}
	}

	data->period_count = lut_entry->period_count;

	/*
	 * Determine clock divider based on the period count. Since rounding up
	 * is the defined behavior, walking up and checking for when the value is
	 * equal or underneath will yield the rounded up value
	 */
	actual_timeout = lut_entry->interval;
	for (data->clock_divider = 0; data->clock_divider < 8; data->clock_divider++) {
		if (max_ms <= actual_timeout) {
			break;
		}
		actual_timeout += lut_entry->interval;
	}

	/* Determine closed window as per the requested lower limit of watchdog feed timeout */
	for (window_idx = 0; window_idx < 7; window_idx++) {
		if (min_ms <= (actual_timeout * window_sixteenths[window_idx] / 16)) {
			/* Use the chosen window size by window_idx */
			break;
		}
		/* If no match, the largest window available is chosen (87.5%) */
	}

	data->window_count = (window_idx << WWDT_WWDTCTL0_WINDOW0_OFS);

	return 0;
}

static int wwdt_mspm0_setup(const struct device *dev, uint8_t options)
{
	const struct wwdt_mspm0_config *config = dev->config;
	struct wwdt_mspm0_data *data = dev->data;

	DL_WWDT_SLEEP_MODE sleep_mode = DL_WWDT_RUN_IN_SLEEP;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) == WDT_OPT_PAUSE_IN_SLEEP) {
		sleep_mode = DL_WWDT_STOP_IN_SLEEP;
	}

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != WDT_OPT_PAUSE_HALTED_BY_DBG) {
		/* On reset the MSPM0 is set to halt with the core halting */
		DL_WWDT_setCoreHaltBehavior(config->base, DL_WWDT_CORE_HALT_FREE_RUN);
	}

	/* This call enables the Watchdog */
	DL_WWDT_initWatchdogMode(config->base, data->clock_divider,
				 data->period_count, sleep_mode, data->window_count,
				 data->window_count);

	return 0;
}

static int wwdt_mspm0_disable(const struct device *dev)
{
	/* Disabling a watchdog that is configured is not possible */
	ARG_UNUSED(dev);
	return -EPERM;
}

static int wwdt_mspm0_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wwdt_mspm0_config *config = dev->config;
	int status;

	/* Cannot install timeout if the WWDT is already running */
	if (DL_WWDT_isRunning(config->base)) {
		LOG_ERR("Install timeout failed. WWDT is already running");
		return -EBUSY;
	}

	if (cfg->callback) {
		LOG_ERR("Install timeout failed. Callback not supported");
		return -ENOTSUP;
	}

	if (!(cfg->flags & config->reset_action)) {
		LOG_ERR("Install timeout failed. Reset action mismatch");
		return -EINVAL;
	}

	/*
	 * To calculate the timeout period as per :
	 * TIMEOUT = (CLKDIV + 1) * PER_count / 32768 (LFCLK Frequency)
	 */
	status = wwdt_mspm0_calculate_timeout_periods(dev, cfg);

	return status;
}

static int wwdt_mspm0_feed(const struct device *dev, int channel_id)
{
	const struct wwdt_mspm0_config *config = dev->config;

	ARG_UNUSED(channel_id);
	DL_WWDT_restart(config->base);

	return 0;
}

static int wwdt_mspm0_init(const struct device *dev)
{
	const struct wwdt_mspm0_config *config = dev->config;

	DL_WWDT_reset(config->base);
	DL_WWDT_enablePower(config->base);

	return 0;
}

static DEVICE_API(wdt, wwdt_mspm0_driver_api) = {
	.setup = wwdt_mspm0_setup,
	.disable = wwdt_mspm0_disable,
	.install_timeout = wwdt_mspm0_install_timeout,
	.feed = wwdt_mspm0_feed
};

#define MSP_WDT_INIT_FN(index)								\
static const struct wwdt_mspm0_config wwdt_mspm0_cfg_##index = {			\
	.base =  (WWDT_Regs *)DT_INST_REG_ADDR(index),					\
	.reset_action = COND_CODE_1(DT_INST_PROP(index, ti_watchdog_reset_action),	\
				    WDT_FLAG_RESET_SOC, WDT_FLAG_RESET_CPU_CORE),	\
};											\
											\
static struct wwdt_mspm0_data wwdt_mspm0_data_##index;					\
											\
DEVICE_DT_INST_DEFINE(index, wwdt_mspm0_init, NULL, &wwdt_mspm0_data_##index,		\
		      &wwdt_mspm0_cfg_##index, POST_KERNEL,				\
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,				\
		      &wwdt_mspm0_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(MSP_WDT_INIT_FN)
