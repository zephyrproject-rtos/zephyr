/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>

#include <sl_hal_wdog.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_silabs_wdog, CONFIG_WDT_LOG_LEVEL);

#define DT_DRV_COMPAT silabs_wdog

#define WINDOW_FRACTION 8
#define WARN_FRACTION   4

struct wdt_silabs_config {
	WDOG_TypeDef *base;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	void (*irq_config_func)(void);
};

struct wdt_silabs_data {
	wdt_callback_t warn_callback;
	wdt_callback_t timeout_callback;
	sl_hal_wdog_init_t settings;
	uint32_t clock_rate;
	bool config_valid;
};

static int wdt_silabs_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_silabs_config *config = dev->config;
	struct wdt_silabs_data *data = dev->data;

	if (sl_hal_wdog_is_enabled(config->base)) {
		return -EBUSY;
	}

	if (!data->config_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

#if defined(_WDOG_CFG_EM1RUN_MASK)
	data->settings.em1_run = !(options & WDT_OPT_PAUSE_IN_SLEEP);
#endif
#if defined(_WDOG_CFG_EM2RUN_MASK)
	data->settings.em2_run = !(options & WDT_OPT_PAUSE_IN_SLEEP);
#endif
#if defined(_WDOG_CFG_EM3RUN_MASK)
	data->settings.em3_run = !(options & WDT_OPT_PAUSE_IN_SLEEP);
#endif
	data->settings.debug_run = !(options & WDT_OPT_PAUSE_HALTED_BY_DBG);

	sl_hal_wdog_disable_interrupts(config->base, WDOG_IF_WARN | WDOG_IF_TOUT);
	if (data->warn_callback != NULL) {
		sl_hal_wdog_clear_interrupts(config->base, WDOG_IF_WARN);
		sl_hal_wdog_enable_interrupts(config->base, WDOG_IF_WARN);
	}
	if (data->timeout_callback != NULL) {
		sl_hal_wdog_clear_interrupts(config->base, WDOG_IF_TOUT);
		sl_hal_wdog_enable_interrupts(config->base, WDOG_IF_TOUT);
	}
	sl_hal_wdog_init(config->base, &data->settings);
	sl_hal_wdog_enable(config->base);

	return 0;
}

static int wdt_silabs_disable(const struct device *dev)
{
	const struct wdt_silabs_config *config = dev->config;
	struct wdt_silabs_data *data = dev->data;

	data->config_valid = false;

	if (!sl_hal_wdog_is_enabled(config->base)) {
		return -EFAULT;
	}

	sl_hal_wdog_disable(config->base);

	return 0;
}

static int wdt_silabs_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_silabs_config *config = dev->config;
	struct wdt_silabs_data *data = dev->data;

	uint32_t timeout_cycles, window_cycles, warn_cycles;
	uint64_t timeout, window, warn = 0;
	uint32_t flags;

	if (sl_hal_wdog_is_enabled(config->base)) {
		return -EBUSY;
	}

	if (data->config_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	data->settings = (sl_hal_wdog_init_t)SL_HAL_WDOG_INIT_DEFAULT;

	window = cfg->window.min;
	timeout = cfg->window.max;
	data->warn_callback = NULL;
	data->timeout_callback = cfg->callback;
	flags = cfg->flags;

#if defined(CONFIG_WDT_MULTISTAGE)
	if (cfg->next) {
		if (cfg->next->window.min != window) {
			LOG_ERR("Minimum window must be the same for both stages");
			return -EINVAL;
		}
		if (flags != WDT_FLAG_RESET_NONE) {
			LOG_ERR("Second stage not supported when first stage resets the device");
			return -EINVAL;
		}

		warn = timeout;
		data->warn_callback = data->timeout_callback;

		timeout = cfg->next->window.max;
		data->timeout_callback = cfg->next->callback;

		flags = cfg->next->flags;
	}
#endif

	/* Timeout must be between 2^3 + 1 and 2^18 + 1 cycles */
	timeout = timeout * data->clock_rate / MSEC_PER_SEC;
	if (!IN_RANGE(timeout, 0x9, 0x40001)) {
		LOG_ERR("Invalid timeout");
		return -EINVAL;
	}
	timeout_cycles = timeout;
	data->settings.period_select = LOG2CEIL(timeout_cycles - 1) - 3;
	timeout_cycles = BIT(data->settings.period_select + 3);
	LOG_INF("Installed timeout value: %u cycles at %u Hz", timeout_cycles + 1,
		data->clock_rate);

	window = window * data->clock_rate / MSEC_PER_SEC;
	if (window >= timeout_cycles) {
		LOG_ERR("Invalid window min, must be smaller than max");
		return -EINVAL;
	}
	window_cycles = ROUND_DOWN(window, timeout_cycles / WINDOW_FRACTION);
	data->settings.window_time_select = window_cycles * WINDOW_FRACTION / timeout_cycles;
	if (window_cycles) {
		LOG_INF("Installed window value: %u%%",
			100 * data->settings.window_time_select / WINDOW_FRACTION);
	}

	warn = warn * data->clock_rate / MSEC_PER_SEC;
	if (warn >= timeout_cycles) {
		LOG_ERR("Invalid first stage window max, must be smaller than second stage");
		return -EINVAL;
	}
	warn_cycles = ROUND_DOWN(warn, timeout_cycles / WARN_FRACTION);
	data->settings.warning_time_select = warn_cycles * WARN_FRACTION / timeout_cycles;
	if (warn_cycles) {
		LOG_INF("Installed warning value: %u%%",
			100 * data->settings.warning_time_select / WARN_FRACTION);
	}

	switch (flags) {
	case WDT_FLAG_RESET_SOC:
		if (data->timeout_callback != NULL) {
			LOG_ERR("Reset mode with callback not supported");
			return -ENOTSUP;
		}
		data->settings.reset_disable = false;
		LOG_DBG("Configuring reset SoC mode");
		break;

	case WDT_FLAG_RESET_NONE:
		data->settings.reset_disable = true;
		LOG_DBG("Configuring non-reset mode");
		break;

	case WDT_FLAG_RESET_CPU_CORE:
		LOG_ERR("CPU core only reset not supported");
		return -ENOTSUP;

	default:
		LOG_ERR("Unsupported watchdog config flag");
		return -EINVAL;
	}

	data->config_valid = true;

	return 0;
}

static int wdt_silabs_feed(const struct device *dev, int channel_id)
{
	const struct wdt_silabs_config *config = dev->config;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (!sl_hal_wdog_is_enabled(config->base)) {
		return -EINVAL;
	}

	sl_hal_wdog_feed(config->base);

	return 0;
}

static void wdt_silabs_isr(const struct device *dev)
{
	const struct wdt_silabs_config *config = dev->config;
	struct wdt_silabs_data *data = dev->data;
	uint32_t flags;

	flags = sl_hal_wdog_get_enabled_pending_interrupts(config->base);
	sl_hal_wdog_clear_interrupts(config->base, flags);

	if ((flags & WDOG_IF_WARN) && (data->warn_callback != NULL)) {
		data->warn_callback(dev, 0);
	}

	if ((flags & WDOG_IF_TOUT) && (data->timeout_callback != NULL)) {
		data->timeout_callback(dev, 0);
	}
}

static int wdt_silabs_init(const struct device *dev)
{
	const struct wdt_silabs_config *config = dev->config;
	struct wdt_silabs_data *data = dev->data;
	int err;

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	err = clock_control_get_rate(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg,
				     &data->clock_rate);
	if (err < 0) {
		return err;
	}

	config->irq_config_func();

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_silabs_disable(dev);
	}

	return 0;
}

static DEVICE_API(wdt, wdt_silabs_driver_api) = {
	.setup = wdt_silabs_setup,
	.disable = wdt_silabs_disable,
	.install_timeout = wdt_silabs_install_timeout,
	.feed = wdt_silabs_feed,
};

#define WDT_SILABS_INIT(index)                                                                     \
	static void wdt_silabs_config_func_##index(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), wdt_silabs_isr,     \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}                                                                                          \
                                                                                                   \
	static const struct wdt_silabs_config wdt_silabs_config_##index = {                        \
		.base = (WDOG_TypeDef *)DT_INST_REG_ADDR(index),                                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(index),                                      \
		.irq_config_func = wdt_silabs_config_func_##index,                                 \
	};                                                                                         \
	static struct wdt_silabs_data wdt_silabs_data_##index;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &wdt_silabs_init, NULL, &wdt_silabs_data_##index,             \
			      &wdt_silabs_config_##index, POST_KERNEL,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_silabs_driver_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_SILABS_INIT)
