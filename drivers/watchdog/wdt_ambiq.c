/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_watchdog

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>

#include <errno.h>
#include <am_mcu_apollo.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_ambiq, CONFIG_WDT_LOG_LEVEL);

typedef void (*ambiq_wdt_cfg_func_t)(void);

struct wdt_ambiq_config {
	uint32_t base;
	uint32_t irq_num;
	uint8_t clk_freq;
	ambiq_wdt_cfg_func_t cfg_func;
};

struct wdt_ambiq_data {
	wdt_callback_t callback;
	uint32_t timeout;
	bool reset;
};

static void wdt_ambiq_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct wdt_ambiq_data *data = dev->data;

	uint32_t status;

	am_hal_wdt_interrupt_status_get(AM_HAL_WDT_MCU, &status, false);
	am_hal_wdt_interrupt_clear(AM_HAL_WDT_MCU, status);

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int wdt_ambiq_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_ambiq_config *dev_cfg = dev->config;
	struct wdt_ambiq_data *data = dev->data;
	am_hal_wdt_config_t cfg;

	if (dev_cfg->clk_freq == 128) {
		cfg.eClockSource = AM_HAL_WDT_128HZ;
	} else if (dev_cfg->clk_freq == 16) {
		cfg.eClockSource = AM_HAL_WDT_16HZ;
	} else if (dev_cfg->clk_freq == 1) {
		cfg.eClockSource = AM_HAL_WDT_1HZ;
	}

	cfg.bInterruptEnable = true;
	cfg.ui32InterruptValue = data->timeout;
	cfg.bResetEnable = data->reset;
	cfg.ui32ResetValue = data->timeout;
	cfg.bAlertOnDSPReset = false;

	am_hal_wdt_config(AM_HAL_WDT_MCU, &cfg);
	am_hal_wdt_interrupt_enable(AM_HAL_WDT_MCU, AM_HAL_WDT_INTERRUPT_MCU);
	am_hal_wdt_start(AM_HAL_WDT_MCU, false);

	return 0;
}

static int wdt_ambiq_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	am_hal_wdt_stop(AM_HAL_WDT_MCU);

	return 0;
}

static int wdt_ambiq_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_ambiq_config *dev_cfg = dev->config;
	struct wdt_ambiq_data *data = dev->data;

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	data->timeout = cfg->window.max / 1000 * dev_cfg->clk_freq;
	data->callback = cfg->callback;

	switch (cfg->flags) {
	case WDT_FLAG_RESET_CPU_CORE:
	case WDT_FLAG_RESET_SOC:
		data->reset = true;
		break;
	case WDT_FLAG_RESET_NONE:
		data->reset = false;
		break;
	default:
		LOG_ERR("Unsupported watchdog config flag");
		return -EINVAL;
	}

	return 0;
}

static int wdt_ambiq_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	am_hal_wdt_restart(AM_HAL_WDT_MCU);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static int wdt_ambiq_init(const struct device *dev)
{
	const struct wdt_ambiq_config *dev_cfg = dev->config;

	if (dev_cfg->clk_freq != 128 && dev_cfg->clk_freq != 16 && dev_cfg->clk_freq != 1) {
		return -ENOTSUP;
	}

	NVIC_ClearPendingIRQ(dev_cfg->irq_num);

	dev_cfg->cfg_func();

	irq_enable(dev_cfg->irq_num);

	return 0;
}

static const struct wdt_driver_api wdt_ambiq_driver_api = {
	.setup = wdt_ambiq_setup,
	.disable = wdt_ambiq_disable,
	.install_timeout = wdt_ambiq_install_timeout,
	.feed = wdt_ambiq_feed,
};

#define AMBIQ_WDT_INIT(n)                                                                          \
	static struct wdt_ambiq_data wdt_ambiq_data##n;                                            \
	static void ambiq_wdt_cfg_func_##n(void)                                                   \
	{                                                                                          \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), wdt_ambiq_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	};                                                                                         \
	static const struct wdt_ambiq_config wdt_ambiq_config##n = {                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clk_freq = DT_INST_PROP(n, clock_frequency),                                      \
		.irq_num = DT_INST_IRQN(n),                                                        \
		.cfg_func = ambiq_wdt_cfg_func_##n};                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, wdt_ambiq_init, NULL, &wdt_ambiq_data##n, &wdt_ambiq_config##n,   \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                    \
			      &wdt_ambiq_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMBIQ_WDT_INIT)
