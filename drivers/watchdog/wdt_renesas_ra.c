/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include "r_wdt.h"

LOG_MODULE_REGISTER(wdt_renesas_ra, CONFIG_WDT_LOG_LEVEL);

struct wdt_renesas_ra_config {
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
};

struct wdt_renesas_ra_data {
	struct wdt_timeout_cfg timeout;
	struct st_wdt_instance_ctrl wdt_ctrl;
	struct st_wdt_cfg wdt_cfg;
	struct k_mutex inst_lock;
	atomic_t device_state;
};

#define WDT_RENESAS_RA_ATOMIC_ENABLE      (0)
#define WDT_RENESAS_RA_ATOMIC_TIMEOUT_SET (1)

/* Lookup table for WDT period raw cycle */
const float timeout_period_lut[] = {
	[WDT_TIMEOUT_128] = 128,    [WDT_TIMEOUT_512] = 512,   [WDT_TIMEOUT_1024] = 1024,
	[WDT_TIMEOUT_2048] = 2048,  [WDT_TIMEOUT_4096] = 4096, [WDT_TIMEOUT_8192] = 8192,
	[WDT_TIMEOUT_16384] = 16384};

/* Lookup table for the division value of the input clock count */
const float clock_div_lut[] = {[WDT_CLOCK_DIVISION_1] = 1,       [WDT_CLOCK_DIVISION_4] = 4,
			       [WDT_CLOCK_DIVISION_16] = 16,     [WDT_CLOCK_DIVISION_32] = 32,
			       [WDT_CLOCK_DIVISION_64] = 64,     [WDT_CLOCK_DIVISION_128] = 128,
			       [WDT_CLOCK_DIVISION_256] = 256,   [WDT_CLOCK_DIVISION_512] = 512,
			       [WDT_CLOCK_DIVISION_2048] = 2048, [WDT_CLOCK_DIVISION_8192] = 8192};

#define WDT_WINDOW_INVALID (-1)

/* Lookup table for the window start position setting */
const int window_start_lut[] = {
	[0] = WDT_WINDOW_START_100, [1] = WDT_WINDOW_START_75, [2] = WDT_WINDOW_START_50,
	[3] = WDT_WINDOW_START_25,  [4] = WDT_WINDOW_INVALID,
};

/* Lookup table for the window end position setting */
const int window_end_lut[] = {
	[0] = WDT_WINDOW_INVALID, [1] = WDT_WINDOW_END_75, [2] = WDT_WINDOW_END_50,
	[3] = WDT_WINDOW_END_25,  [4] = WDT_WINDOW_END_0,
};

static inline void wdt_renesas_ra_inst_lock(const struct device *dev)
{
	struct wdt_renesas_ra_data *data = dev->data;

	k_mutex_lock(&data->inst_lock, K_FOREVER);
}

static inline void wdt_renesas_ra_inst_unlock(const struct device *dev)
{
	struct wdt_renesas_ra_data *data = dev->data;

	k_mutex_unlock(&data->inst_lock);
}

static int wdt_renesas_ra_timeout_calculate(const struct device *dev,
					    const struct wdt_timeout_cfg *config)
{
	struct wdt_renesas_ra_data *data = dev->data;
	const struct wdt_renesas_ra_config *cfg = dev->config;
	struct st_wdt_cfg *p_cfg = &data->wdt_cfg;
	unsigned int window_start_idx;
	unsigned int window_end_idx;
	unsigned int best_divisor = WDT_CLOCK_DIVISION_1;
	unsigned int best_timeout = WDT_TIMEOUT_128;
	unsigned int best_period_ms = UINT_MAX;
	unsigned int min_delta = UINT_MAX;
	uint32_t clock_rate;
	int ret;

	if (atomic_test_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_TIMEOUT_SET)) {
		if (config->window.min != data->timeout.window.min ||
		    config->window.max != data->timeout.window.max ||
		    config->flags != data->timeout.flags) {
			LOG_ERR("wdt support only one timeout setting value");
			return -EINVAL;
		}

		data->timeout.callback = config->callback;
		return 0;
	}

	ret = clock_control_get_rate(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys,
				     &clock_rate);
	if (unlikely(ret)) {
		return ret;
	}

	for (unsigned int divisor = WDT_CLOCK_DIVISION_1; divisor < ARRAY_SIZE(clock_div_lut);
	     divisor++) {
		for (unsigned int timeout = WDT_TIMEOUT_128;
		     timeout < ARRAY_SIZE(timeout_period_lut); timeout++) {
			unsigned int period_ms =
				(unsigned int)(1000.0F * clock_div_lut[divisor] *
					       timeout_period_lut[timeout] / (float)clock_rate);
			unsigned int delta = period_ms > config->window.max
						     ? period_ms - config->window.max
						     : config->window.max - period_ms;

			if (delta < min_delta) {
				min_delta = delta;
				best_divisor = divisor;
				best_timeout = timeout;
				best_period_ms = period_ms;
			}
		}
	}

	if (min_delta == UINT_MAX) {
		LOG_ERR("wdt timeout out of range");
		return -EINVAL;
	}

	window_start_idx = (config->window.min * 4 + best_period_ms - 1) / best_period_ms;
	window_end_idx = (config->window.max * 4 + best_period_ms - 1) / best_period_ms;

	if (window_start_lut[window_start_idx] == WDT_WINDOW_INVALID ||
	    window_end_lut[window_end_idx] == WDT_WINDOW_INVALID) {
		LOG_ERR("this wdt timeout is not supported");
		return -ENOTSUP;
	}

	p_cfg->clock_division = (wdt_clock_division_t)best_divisor;
	p_cfg->timeout = (wdt_timeout_t)best_timeout;
	p_cfg->window_start = (wdt_window_start_t)window_start_lut[window_start_idx];
	p_cfg->window_end = (wdt_window_end_t)window_end_lut[window_end_idx];

	LOG_INF("actual window min = %.2f ms", window_start_idx * best_period_ms * 0.25);
	LOG_INF("actual window max = %.2f ms", window_end_idx * best_period_ms * 0.25);

	memcpy(&data->timeout, config, sizeof(struct wdt_timeout_cfg));

	return 0;
}

static int wdt_renesas_ra_setup(const struct device *dev, uint8_t options)
{
	struct wdt_renesas_ra_data *data = dev->data;
	int ret = 0;

	/*
	 * TODO: WDT must be restarted with wdt_feed call after sleep -> will be added later when PM
	 * mode is supported
	 */
	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		LOG_ERR("wdt pause in sleep mode not supported");
		return -ENOTSUP;
	}

	wdt_renesas_ra_inst_lock(dev);

	if (atomic_test_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_ENABLE)) {
		LOG_ERR("wdt has been already setup");
		ret = -EBUSY;
		goto end;
	}

	if (!atomic_test_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_TIMEOUT_SET)) {
		LOG_ERR("wdt timeout should be installed before");
		ret = -EFAULT;
		goto end;
	}

	/* Pause watchdog timer when CPU is halted by the debugger. */
	R_DEBUG->DBGSTOPCR_b.DBGSTOP_WDT = (options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0 ? 1 : 0;

	if (R_WDT_Open(&data->wdt_ctrl, &data->wdt_cfg) != FSP_SUCCESS) {
		LOG_ERR("wdt setup failed");
		ret = -EIO;
		goto end;
	}

	if (R_WDT_Refresh(&data->wdt_ctrl) != FSP_SUCCESS) {
		LOG_ERR("wdt start failed");
		ret = -EIO;
		goto end;
	}

	atomic_set_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_ENABLE);

end:
	wdt_renesas_ra_inst_unlock(dev);

	return ret;
}

static int wdt_renesas_ra_disable(const struct device *dev)
{
	struct wdt_renesas_ra_data *data = dev->data;

	if (!atomic_test_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_ENABLE)) {
		LOG_ERR("wdt has not been enabled yet");
		return -EFAULT;
	}

	LOG_ERR("wdt can not be stopped once it has started");
	return -EPERM;
}

#ifdef CONFIG_WDT_RENESAS_RA_NMI
void wdt_renesas_ra_callback_adapter(wdt_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct wdt_renesas_ra_data *data = dev->data;
	wdt_callback_t callback = data->timeout.callback;

	if (callback != NULL) {
		callback(dev, 0);
	}
}
#endif /* CONFIG_WDT_RENESAS_RA_NMI */

#define WDT_RENESAS_RA_SUPPORTED_FLAGS (WDT_FLAG_RESET_NONE | WDT_FLAG_RESET_SOC)

static int wdt_renesas_ra_install_timeout(const struct device *dev,
					  const struct wdt_timeout_cfg *config)
{
	struct wdt_renesas_ra_data *data = dev->data;
	struct st_wdt_cfg *p_config = &data->wdt_cfg;
	int ret = 0;

	if (config->window.min > config->window.max || config->window.max == 0) {
		return -EINVAL;
	}

	if ((config->flags & ~WDT_RENESAS_RA_SUPPORTED_FLAGS) != 0) {
		return -ENOTSUP;
	}

	if (config->callback == NULL && (config->flags & WDT_FLAG_RESET_MASK) == 0) {
		LOG_ERR("no timeout response was chosen");
		return -EINVAL;
	}

	if (config->callback != NULL && (config->flags & WDT_FLAG_RESET_MASK) != 0) {
		LOG_ERR("WDT_FLAG_RESET_NONE should be chosen in case of interrupt response");
		return -ENOTSUP;
	}

	wdt_renesas_ra_inst_lock(dev);

	if (atomic_test_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_ENABLE)) {
		LOG_ERR("cannot change timeout settings after wdt setup");
		ret = -EBUSY;
		goto end;
	}

#ifndef CONFIG_WDT_RENESAS_RA_NMI
	if (config->callback != NULL) {
		LOG_ERR("interrupt response only available in case CONFIG_WDT_RENESAS_RA_NMI=y");
		ret = -ENOTSUP;
		goto end;
	}
#else
	p_config->reset_control = (config->flags & WDT_FLAG_RESET_MASK) != 0
					  ? WDT_RESET_CONTROL_RESET
					  : WDT_RESET_CONTROL_NMI;
#endif

	ret = wdt_renesas_ra_timeout_calculate(dev, config);
	if (ret < 0) {
		goto end;
	}

	atomic_set_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_TIMEOUT_SET);
	LOG_INF("wdt timeout was set successfully");

end:
	wdt_renesas_ra_inst_unlock(dev);

	return ret;
}

static int wdt_renesas_ra_feed(const struct device *dev, int channel_id)
{
	struct wdt_renesas_ra_data *data = dev->data;

	if (!atomic_test_bit(&data->device_state, WDT_RENESAS_RA_ATOMIC_ENABLE) ||
	    channel_id != 0) {
		return -EINVAL;
	}

	if (R_WDT_Refresh(&data->wdt_ctrl) != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(wdt, wdt_renesas_ra_api) = {
	.setup = wdt_renesas_ra_setup,
	.disable = wdt_renesas_ra_disable,
	.install_timeout = wdt_renesas_ra_install_timeout,
	.feed = wdt_renesas_ra_feed,
};

#define WDT_RENESAS_RA_DEFINE(id)                                                                  \
	static struct wdt_renesas_ra_config wdt_renesas_ra_cfg##id = {                             \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(id)),                                    \
		.clock_subsys = {.mstp = DT_CLOCKS_CELL(id, mstp),                                 \
				 .stop_bit = DT_CLOCKS_CELL(id, stop_bit)},                        \
	};                                                                                         \
                                                                                                   \
	static struct wdt_renesas_ra_data wdt_renesas_ra_data##id = {                              \
		.device_state = ATOMIC_INIT(0),                                                    \
		.wdt_cfg = {.stop_control = WDT_STOP_CONTROL_DISABLE,                              \
			    .reset_control = WDT_RESET_CONTROL_RESET,                              \
			    .p_callback = wdt_renesas_ra_callback_adapter,                         \
			    .p_context = (void *)DEVICE_DT_GET(id)},                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(id, NULL, NULL, &wdt_renesas_ra_data##id, &wdt_renesas_ra_cfg##id,        \
			 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_renesas_ra_api);

DT_FOREACH_STATUS_OKAY(renesas_ra_wdt, WDT_RENESAS_RA_DEFINE)
