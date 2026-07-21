/*
 * Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Independent Watchdog (IWDT) Driver for Renesas RX
 */

#define DT_DRV_COMPAT renesas_rx_iwdt

#include <zephyr/drivers/watchdog.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/arch/rx/sw_nmi_table.h>

#include "r_iwdt_rx_if.h"

#include <platform.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_iwdt_rx);

#define IWDT_NODELABEL  DT_NODELABEL(iwdt)
#define IWDT_NMI_VECTOR 2

#ifdef CONFIG_WDT_RENESAS_RX_IWDT_REGISTER_START_MODE
#define IWDT_WINDOW_START DT_PROP(IWDT_NODELABEL, window_start)
#define IWDT_WINDOW_END   DT_PROP(IWDT_NODELABEL, window_end)
#endif /* CONFIG_WDT_RENESAS_RX_IWDT_REGISTER_START_MODE */

#define WDT_RENESAS_RX_SUPPORTED_FLAGS (WDT_FLAG_RESET_NONE | WDT_FLAG_RESET_SOC)
struct wdt_iwdt_rx_config {
	struct st_iwdt *const regs;
	const struct device *clock_dev;
};

struct wdt_iwdt_rx_data {
	wdt_callback_t callback;
	iwdt_config_t iwdt_config;
	bool timeout_installed;
};

#if CONFIG_WDT_RENESAS_RX_IWDT_USE_NMI
static void wdt_iwdt_isr(const struct device *dev)
{
	struct wdt_iwdt_rx_data *data = dev->data;

	wdt_callback_t callback = data->callback;

	if (callback) {
		callback(dev, 0);
	}
}
#endif /* CONFIG_WDT_RENESAS_RX_IWDT_USE_NMI */

static int wdt_iwdt_rx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_ERR("Independent Watchdog cannot be stopped once started");

	return -EPERM;
}

static int wdt_iwdt_rx_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	iwdt_err_t err;

	/* Reset counter */
	err = R_IWDT_Control(IWDT_CMD_REFRESH_COUNTING, NULL);
	if (err != IWDT_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int wdt_iwdt_rx_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_iwdt_rx_data *data = dev->data;

	if (cfg->window.min > cfg->window.max || cfg->window.max == 0) {
		return -EINVAL;
	}

	if ((cfg->flags & ~WDT_RENESAS_RX_SUPPORTED_FLAGS) != 0) {
		return -ENOTSUP;
	}

	if (cfg->callback != NULL && (cfg->flags & WDT_FLAG_RESET_MASK) != 0) {
		LOG_ERR("WDT_FLAG_RESET_NONE should be chosen in case of interrupt response");
		return -ENOTSUP;
	}

#ifdef CONFIG_WDT_RENESAS_RX_IWDT_AUTO_START_MODE
	/* In auto-start mode, IWDT is started by OFS setting when the chip is reset.
	 * So timeout and other parameters must be configured via Kconfig.
	 * Runtime configuration is ignored.
	 */
	if (CONFIG_WDT_RENESAS_RX_OFS0_IRQ_SEL_IWDTRSTIRQS == 0) {
		if ((cfg->flags & WDT_FLAG_RESET_MASK) != WDT_FLAG_RESET_NONE) {
			LOG_ERR("Reset flag is not consistent with OFS setting");
			return -EINVAL;
		}
	} else if (CONFIG_WDT_RENESAS_RX_OFS0_IRQ_SEL_IWDTRSTIRQS == 1) {
		if ((cfg->flags & WDT_FLAG_RESET_MASK) == WDT_FLAG_RESET_NONE) {
			LOG_ERR("Reset flag is not consistent with OFS setting");
			return -EINVAL;
		}
	} else {
		LOG_ERR("Invalid OFS setting");
		return -EINVAL;
	}

	LOG_WRN("IWDT is auto-started by OFS. Timeout and other parameters must be configured via "
		"Kconfig, runtime configuration is ignored.");
#ifdef CONFIG_WDT_RENESAS_RX_IWDT_USE_NMI
	data->callback = cfg->callback;
#endif
	data->timeout_installed = true;
	return 0;
#else /* CONFIG_WDT_RENESAS_RX_IWDT_AUTO_START_MODE */
	const struct wdt_iwdt_rx_config *iwdt_cfg = dev->config;
	uint32_t clock_rate;
	int ret;

	/* Get the iwdt clock rate in hz */
	ret = clock_control_get_rate(iwdt_cfg->clock_dev, NULL, &clock_rate);
	if (ret != 0) {
		return ret;
	}

	uint32_t clock_rate_khz = clock_rate / 1000;

	const uint16_t timeout_period[4] = {128, 512, 1024, 2048};
	const uint16_t clock_divide[6][2] = {{IWDT_CLOCK_DIV_1, 1},     {IWDT_CLOCK_DIV_16, 16},
					     {IWDT_CLOCK_DIV_32, 32},   {IWDT_CLOCK_DIV_64, 64},
					     {IWDT_CLOCK_DIV_128, 128}, {IWDT_CLOCK_DIV_256, 256}};
	int16_t last_error = INT16_MAX;
	int32_t error;
	uint16_t iwdt_tops = 0;
	uint16_t iwdt_clock_div = 0;
	uint16_t iwdt_timeout = ((uint32_t)timeout_period[0] * clock_divide[0][1]) / clock_rate_khz;

	for (int idx_p = 0; idx_p < 4; idx_p++) {
		for (int idx_d = 0; idx_d < 6; idx_d++) {
			iwdt_timeout = ((uint32_t)timeout_period[idx_p] * clock_divide[idx_d][1]) /
				       clock_rate_khz;
			error = cfg->window.max - iwdt_timeout;

			if (error < 0) {
				break;
			}

			if (error < last_error) {
				last_error = error;
				iwdt_tops = idx_p;
				iwdt_clock_div = clock_divide[idx_d][0];
			}
		}
	}

	data->iwdt_config.timeout = iwdt_tops;
	data->iwdt_config.iwdtclk_div = iwdt_clock_div;
	data->iwdt_config.window_start = IWDT_WINDOW_START;
	data->iwdt_config.window_end = IWDT_WINDOW_END;
	data->iwdt_config.timeout_control =
		(cfg->flags & WDT_FLAG_RESET_MASK) != 0 ? IWDT_TIMEOUT_RESET : IWDT_TIMEOUT_NMI;
	data->iwdt_config.count_stop_enable = IWDT_COUNT_STOP_DISABLE;

	data->timeout_installed = true;

#if CONFIG_WDT_RENESAS_RX_IWDT_USE_NMI
	data->callback = cfg->callback;
#endif
	return 0;

#endif /* CONFIG_WDT_RENESAS_RX_IWDT_AUTO_START_MODE */
}

static int wdt_iwdt_rx_setup(const struct device *dev, uint8_t options)
{
	struct wdt_iwdt_rx_data *data = dev->data;

	if (!data->timeout_installed) {
		return -EINVAL;
	}

#ifdef CONFIG_WDT_RENESAS_RX_IWDT_USE_NMI
	nmi_enable(IWDT_NMI_VECTOR, (nmi_callback_t)wdt_iwdt_isr, (void *)dev);
#endif /* CONFIG_WDT_RENESAS_RX_IWDT_USE_NMI */

#if CONFIG_WDT_RENESAS_RX_IWDT_AUTO_START_MODE
	LOG_WRN("WDT is configured in auto-start mode, setup via API is not supported");
	return 0;
#else  /* CONFIG_WDT_RENESAS_RX_IWDT_AUTO_START_MODE */

	iwdt_err_t err;
	int ret;

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		data->iwdt_config.count_stop_enable = IWDT_COUNT_STOP_ENABLE;
	} else {
		data->iwdt_config.count_stop_enable = IWDT_COUNT_STOP_DISABLE;
	}

	err = R_IWDT_Open(&data->iwdt_config);
	if (err != IWDT_SUCCESS) {
		return -EINVAL;
	}

	ret = wdt_iwdt_rx_feed(dev, 0);
	if (ret != 0) {
		return ret;
	}

	return 0;
#endif /* CONFIG_WDT_RENESAS_RX_IWDT_AUTO_START_MODE */
}

static DEVICE_API(wdt, wdt_iwdt_rx_api) = {
	.disable = wdt_iwdt_rx_disable,
	.feed = wdt_iwdt_rx_feed,
	.install_timeout = wdt_iwdt_rx_install_timeout,
	.setup = wdt_iwdt_rx_setup,
};

#define IWDT_RENESAS_RX_DEFINE(idx)                                                                \
	static struct wdt_iwdt_rx_config iwdt_rx_cfg##idx = {                                      \
		.regs = (struct st_iwdt *)DT_REG_ADDR(IWDT_NODELABEL),                             \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(idx)),                                   \
	};                                                                                         \
	static struct wdt_iwdt_rx_data iwdt_rx_data##idx = {                                       \
		.timeout_installed = false,                                                        \
		.iwdt_config = {.count_stop_enable = IWDT_COUNT_STOP_DISABLE,                      \
				.timeout_control = IWDT_TIMEOUT_RESET},                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(idx, NULL, NULL, &iwdt_rx_data##idx, &iwdt_rx_cfg##idx, POST_KERNEL,      \
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_iwdt_rx_api);

DT_FOREACH_STATUS_OKAY(renesas_rx_iwdt, IWDT_RENESAS_RX_DEFINE);
