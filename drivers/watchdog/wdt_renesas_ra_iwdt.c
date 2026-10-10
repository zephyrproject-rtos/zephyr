/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Independent Watchdog (IWDT) Driver for Renesas RA
 */

#define DT_DRV_COMPAT renesas_ra_iwdt

#include <zephyr/drivers/watchdog.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

#include "r_iwdt.h"

/* Config Log */
#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_iwdt_ra);

#define IWDT_WINDOW_INVALID (-1)

/* IWDTCR clock division setting and the divisor it applies to IWDTCLK (RM Table 25.1). */
struct iwdt_clk_div_map {
	wdt_clock_division_t reg_val;
	uint16_t divisor;
};

static const struct iwdt_clk_div_map iwdt_clk_div_table[] = {
	{WDT_CLOCK_DIVISION_1, 1},     {WDT_CLOCK_DIVISION_16, 16},   {WDT_CLOCK_DIVISION_32, 32},
	{WDT_CLOCK_DIVISION_64, 64},   {WDT_CLOCK_DIVISION_128, 128}, {WDT_CLOCK_DIVISION_256, 256},
};

/* IWDTTOPS timeout period setting, in IWDTCLK cycles (RM Table 25.1/25.2). */
struct iwdt_timeout_map {
	wdt_timeout_t reg_val;
	uint16_t cycles;
};

static const struct iwdt_timeout_map iwdt_timeout_table[] = {
	{WDT_TIMEOUT_128, 128},
	{WDT_TIMEOUT_512, 512},
	{WDT_TIMEOUT_1024, 1024},
	{WDT_TIMEOUT_2048, 2048},
};

/* Window start position for each quarter of the timeout period elapsed (RM Table 25.3). */
static const int32_t iwdt_window_start_lut[] = {
	[0] = WDT_WINDOW_START_100,
	[1] = WDT_WINDOW_START_75,
	[2] = WDT_WINDOW_START_50,
	[3] = WDT_WINDOW_START_25,
	[4] = IWDT_WINDOW_INVALID,
};

/* Window end position for each quarter of the timeout period elapsed (RM Table 25.3). */
static const int32_t iwdt_window_end_lut[] = {
	[0] = IWDT_WINDOW_INVALID,
	[1] = WDT_WINDOW_END_75,
	[2] = WDT_WINDOW_END_50,
	[3] = WDT_WINDOW_END_25,
	[4] = WDT_WINDOW_END_0,
};

struct iwdt_config {
	/* Devicetree-fixed timing, used only if setup() is called without install_timeout() */
	wdt_window_start_t start_window;
	wdt_window_end_t end_window;
	wdt_timeout_t timeout;
	wdt_clock_division_t clock_division;
	/* If HAL Renesas change API, we only need to update this pointer*/
	const wdt_api_t *fsp_iwdt_api;
};

struct iwdt_data {
	iwdt_instance_ctrl_t fsp_ctrl;
	wdt_callback_t callback;
	wdt_reset_control_t reset_control;
	wdt_window_start_t start_window;
	wdt_window_end_t end_window;
	wdt_clock_division_t clock_division;
	wdt_timeout_t timeout;
	bool timeout_installed;
};

static int iwdt_ra_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_ERR("Independent Watchdog cannot be stopped once started");

	return -EPERM;
}

static int iwdt_ra_feed(const struct device *dev, int channel_id)
{
	const struct iwdt_config *cfg = dev->config;
	struct iwdt_data *data = dev->data;

	ARG_UNUSED(channel_id);

	if (cfg->fsp_iwdt_api->refresh(&data->fsp_ctrl) != FSP_SUCCESS) {
		LOG_ERR("Fail to refresh watchdog!");
		return -EIO;
	}

	return 0;
}

#if defined(CONFIG_WDT_RENESAS_RA_IWDT_NMI)
static void iwdt_ra_callback_adapter(wdt_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct iwdt_data *data = dev->data;

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}
#endif /* CONFIG_WDT_RENESAS_RA_IWDT_NMI */

/*
 * Find the (clock_division, timeout) pair whose resulting period is nearest to
 * z_cfg->window.max, then convert window.min/window.max into the nearest window
 * start/end position achievable against that period (RM section 25, Table 25.1-25.3).
 */
static int iwdt_ra_timing_calculate(const struct wdt_timeout_cfg *z_cfg, struct iwdt_data *data)
{
	size_t best_div_idx = 0;
	size_t best_timeout_idx = 0;
	uint32_t best_period_ms = 0;
	uint32_t min_delta = UINT32_MAX;
	size_t window_start_idx;
	size_t window_end_idx;

	if (z_cfg->window.min > z_cfg->window.max || z_cfg->window.max == 0) {
		return -EINVAL;
	}

	/*
	 * Brute-force every (clock_division, timeout) pair: compute the resulting
	 * period in ms and keep the pair whose period has the smallest absolute
	 * distance to window.max.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(iwdt_clk_div_table); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(iwdt_timeout_table); j++) {
			uint32_t period_ms = (1000U * iwdt_clk_div_table[i].divisor *
					       iwdt_timeout_table[j].cycles) /
					      BSP_FEATURE_IWDT_CLOCK_FREQUENCY;
			uint32_t delta = period_ms > z_cfg->window.max
						  ? period_ms - z_cfg->window.max
						  : z_cfg->window.max - period_ms;

			if (delta < min_delta) {
				min_delta = delta;
				best_div_idx = i;
				best_timeout_idx = j;
				best_period_ms = period_ms;
			}
		}
	}

	if (min_delta == UINT32_MAX || best_period_ms == 0) {
		LOG_ERR("iwdt timeout out of range");
		return -EINVAL;
	}

	/*
	 * The IWDT window can only be placed at the 100/75/50/25/0% marks of the
	 * timeout period (5 boundaries, indices 0-4; RM Table 25.3). Round
	 * window.min/max up to the nearest such boundary of the chosen period.
	 */
	window_start_idx = MIN((z_cfg->window.min * 4 + best_period_ms - 1) / best_period_ms, 4U);
	window_end_idx = MIN((z_cfg->window.max * 4 + best_period_ms - 1) / best_period_ms, 4U);

	if (iwdt_window_start_lut[window_start_idx] == IWDT_WINDOW_INVALID ||
	    iwdt_window_end_lut[window_end_idx] == IWDT_WINDOW_INVALID) {
		LOG_ERR("this iwdt window is not supported");
		return -ENOTSUP;
	}

	data->clock_division = iwdt_clk_div_table[best_div_idx].reg_val;
	data->timeout = iwdt_timeout_table[best_timeout_idx].reg_val;
	data->start_window = (wdt_window_start_t)iwdt_window_start_lut[window_start_idx];
	data->end_window = (wdt_window_end_t)iwdt_window_end_lut[window_end_idx];

	LOG_INF("iwdt actual period = %u ms, window start = %zu%%, window end = %zu%%",
		best_period_ms, (4 - window_start_idx) * 25, (4 - window_end_idx) * 25);

	return 0;
}

static int iwdt_ra_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *z_cfg)
{
#define IWDT_RA_SUPPORTED_FLAGS (WDT_FLAG_RESET_NONE | WDT_FLAG_RESET_SOC)
	struct iwdt_data *data = dev->data;
	int ret;

	if (data->timeout_installed) {
		/* IWDT only supports a single timeout/channel. */
		return -ENOMEM;
	}

	if ((z_cfg->flags & ~IWDT_RA_SUPPORTED_FLAGS) != 0) {
		return -ENOTSUP;
	}

	if (z_cfg->callback == NULL && (z_cfg->flags & WDT_FLAG_RESET_MASK) == 0) {
		LOG_ERR("no timeout callback set");
		return -EINVAL;
	}

	if (z_cfg->callback != NULL && (z_cfg->flags & WDT_FLAG_RESET_MASK) != 0) {
		LOG_ERR("WDT_FLAG_RESET_NONE should be chosen in case of interrupt response");
		return -ENOTSUP;
	}

#if !defined(CONFIG_WDT_RENESAS_RA_IWDT_NMI)
	if (z_cfg->callback != NULL) {
		LOG_ERR("IWDT NMI is not enabled, IWDT callback Won't be called");
		return -ENOTSUP;
	}
#endif /* !CONFIG_WDT_RENESAS_RA_IWDT_NMI */

	ret = iwdt_ra_timing_calculate(z_cfg, data);
	if (ret != 0) {
		return ret;
	}

	data->callback = z_cfg->callback;
	data->reset_control =
		z_cfg->callback != NULL ? WDT_RESET_CONTROL_NMI : WDT_RESET_CONTROL_RESET;
	data->timeout_installed = true;

	return 0;
}

static int iwdt_ra_setup(const struct device *dev, uint8_t options)
{
	const struct iwdt_config *cfg = dev->config;
	struct iwdt_data *data = dev->data;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("Independent Watchdog pause in sleep mode is not supported");
		return -ENOTSUP;
	}

	/*
	 * If no timeout was installed, start with the devicetree-fixed window and reset
	 * on timeout, instead of refusing to start the watchdog at all.
	 */
	wdt_cfg_t fsp_cfg = {
		.window_start = cfg->start_window,
		.window_end = cfg->end_window,
		.timeout = cfg->timeout,
		.clock_division = cfg->clock_division,
		.reset_control = WDT_RESET_CONTROL_RESET,
		.p_context = (void *)dev,
	};

	if (data->timeout_installed) {
		fsp_cfg.window_start = data->start_window;
		fsp_cfg.window_end = data->end_window;
		fsp_cfg.timeout = data->timeout;
		fsp_cfg.clock_division = data->clock_division;
		fsp_cfg.reset_control = data->reset_control;
#if defined(CONFIG_WDT_RENESAS_RA_IWDT_NMI)
		fsp_cfg.p_callback = data->callback != NULL ? iwdt_ra_callback_adapter : NULL;
#endif
	} else {
		LOG_WRN("iwdt timeout was not installed; starting with devicetree window "
			"and reset on timeout");
	}

#if defined(CONFIG_WDT_RENESAS_RA_IWDT_AUTO_START_MODE)
	LOG_WRN("IWDT is configured for auto start mode; window, timeout and clock division "
		"are ignored here");
#elif !BSP_FEATURE_IWDT_SUPPORTS_REGISTER_START_MODE
#error "CONFIG_WDT_RENESAS_RA_IWDT_REGISTER_START_MODE is selected, but this SoC's " \
	     "IWDT does not implement register start mode"
#endif

	/* Pause watchdog timer when CPU is halted by the debugger. */
	R_DEBUG->DBGSTOPCR_b.DBGSTOP_IWDT = (options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0 ? 1 : 0;

	if (cfg->fsp_iwdt_api->open(&data->fsp_ctrl, &fsp_cfg) != FSP_SUCCESS) {
		LOG_ERR("Fail to setup IWDG");
		return -EIO;
	}

	if (cfg->fsp_iwdt_api->refresh(&data->fsp_ctrl) != FSP_SUCCESS) {
		LOG_ERR("Fail to kick IWDG!");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(wdt, wdt_iwdt_ra_api) = {
	.disable = iwdt_ra_disable,
	.feed = iwdt_ra_feed,
	.install_timeout = iwdt_ra_install_timeout,
	.setup = iwdt_ra_setup,
};

/* No window-start property: default to the window open for the whole period. */
#define IWDT_RA_WINDOW_START(idx)                                                                 \
	COND_CODE_1(DT_NODE_HAS_PROP(idx, window_start),                                          \
		(DT_PROP(idx, window_start) == 25   ? WDT_WINDOW_START_25                         \
		 : DT_PROP(idx, window_start) == 50 ? WDT_WINDOW_START_50                         \
		 : DT_PROP(idx, window_start) == 75 ? WDT_WINDOW_START_75                         \
						     : WDT_WINDOW_START_100),                      \
		(WDT_WINDOW_START_100))

/* No window-end property: default to the window open for the whole period. */
#define IWDT_RA_WINDOW_END(idx)                                                                   \
	COND_CODE_1(DT_NODE_HAS_PROP(idx, window_end),                                            \
		(DT_PROP(idx, window_end) == 75   ? WDT_WINDOW_END_75                             \
		 : DT_PROP(idx, window_end) == 50 ? WDT_WINDOW_END_50                             \
		 : DT_PROP(idx, window_end) == 25 ? WDT_WINDOW_END_25                             \
						   : WDT_WINDOW_END_0),                            \
		(WDT_WINDOW_END_0))

/* No timeout-period property: default to the longest available period. */
#define IWDT_RA_TIMEOUT_PERIOD(idx)                                                               \
	COND_CODE_1(DT_NODE_HAS_PROP(idx, timeout_period),                                        \
		(DT_PROP(idx, timeout_period) == 128    ? WDT_TIMEOUT_128                         \
		 : DT_PROP(idx, timeout_period) == 512  ? WDT_TIMEOUT_512                         \
		 : DT_PROP(idx, timeout_period) == 1024 ? WDT_TIMEOUT_1024                        \
							 : WDT_TIMEOUT_2048),                      \
		(WDT_TIMEOUT_2048))

/* No clock-division property: default to the longest available period. */
#define IWDT_RA_CLOCK_DIVISION(idx)                                                               \
	COND_CODE_1(DT_NODE_HAS_PROP(idx, clock_division),                                        \
		(DT_PROP(idx, clock_division) == 1     ? WDT_CLOCK_DIVISION_1                     \
		 : DT_PROP(idx, clock_division) == 16  ? WDT_CLOCK_DIVISION_16                    \
		 : DT_PROP(idx, clock_division) == 32  ? WDT_CLOCK_DIVISION_32                    \
		 : DT_PROP(idx, clock_division) == 64  ? WDT_CLOCK_DIVISION_64                    \
		 : DT_PROP(idx, clock_division) == 128 ? WDT_CLOCK_DIVISION_128                   \
							: WDT_CLOCK_DIVISION_256),                 \
		(WDT_CLOCK_DIVISION_256))

#define IWDT_RENESAS_RA_DEFINE(idx)                                                               \
	static const struct iwdt_config iwdt_ra_cfg_##idx = {                                     \
		.start_window = IWDT_RA_WINDOW_START(idx),                                        \
		.end_window = IWDT_RA_WINDOW_END(idx),                                            \
		.timeout = IWDT_RA_TIMEOUT_PERIOD(idx),                                           \
		.clock_division = IWDT_RA_CLOCK_DIVISION(idx),                                    \
		.fsp_iwdt_api = &g_wdt_on_iwdt                                            \
	};                                                                                         \
	static struct iwdt_data iwdt_ra_data_##idx;                                               \
                                                                                                   \
	DEVICE_DT_DEFINE(idx, NULL, NULL, &iwdt_ra_data_##idx, &iwdt_ra_cfg_##idx, POST_KERNEL,   \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_iwdt_ra_api);

DT_FOREACH_STATUS_OKAY(renesas_ra_iwdt, IWDT_RENESAS_RA_DEFINE)
