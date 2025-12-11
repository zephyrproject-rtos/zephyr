/*
 * Copyright (C) 2017 Intel Corporation
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_watchdog

#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(CONFIG_SOC_SERIES_ESP32H2)
#include <soc/lp_aon_reg.h>
#else
#include <soc/rtc_cntl_reg.h>
#endif
#include <soc/timer_group_reg.h>
#include <hal/mwdt_ll.h>
#include <hal/wdt_hal.h>

#include <string.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_esp32, CONFIG_WDT_LOG_LEVEL);

#define MWDT_TICK_PRESCALER		40000
#define MWDT_TICKS_PER_US		500

struct wdt_esp32_data {
	wdt_hal_context_t hal;
	uint32_t timeout;
	wdt_stage_action_t mode;
	wdt_callback_t callback;
};

struct wdt_esp32_config {
	wdt_inst_t wdt_inst;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	void (*connect_irq)(void);
	int irq_source;
	int irq_priority;
	int irq_flags;
};

static inline void wdt_esp32_seal(const struct device *dev)
{
	struct wdt_esp32_data *data = dev->data;

	wdt_hal_write_protect_enable(&data->hal);
}

static inline void wdt_esp32_unseal(const struct device *dev)
{
	struct wdt_esp32_data *data = dev->data;

	wdt_hal_write_protect_disable(&data->hal);
}

static void wdt_esp32_enable(const struct device *dev)
{
	struct wdt_esp32_data *data = dev->data;

	wdt_esp32_unseal(dev);
	wdt_hal_enable(&data->hal);
	wdt_esp32_seal(dev);

}

static int wdt_esp32_disable(const struct device *dev)
{
	struct wdt_esp32_data *data = dev->data;

	wdt_esp32_unseal(dev);
	wdt_hal_disable(&data->hal);
	wdt_esp32_seal(dev);

	return 0;
}

static void wdt_esp32_isr(void *arg);

static int wdt_esp32_feed(const struct device *dev, int channel_id)
{
	struct wdt_esp32_data *data = dev->data;

	wdt_esp32_unseal(dev);
	wdt_hal_feed(&data->hal);
	wdt_esp32_seal(dev);

	return 0;
}

static int wdt_esp32_set_config(const struct device *dev, uint8_t options)
{
	struct wdt_esp32_data *data = dev->data;

	wdt_esp32_unseal(dev);
	wdt_hal_config_stage(&data->hal, WDT_STAGE0, data->timeout, WDT_STAGE_ACTION_INT);
	wdt_hal_config_stage(&data->hal, WDT_STAGE1, data->timeout, data->mode);
	wdt_esp32_enable(dev);
	wdt_esp32_seal(dev);
	wdt_esp32_feed(dev, 0);

	return 0;
}

static int wdt_esp32_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	struct wdt_esp32_data *data = dev->data;

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	data->timeout = cfg->window.max;
	data->callback = cfg->callback;

	/* Set mode of watchdog and callback */
	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		data->mode = WDT_STAGE_ACTION_RESET_SYSTEM;
		LOG_DBG("Configuring reset SOC mode");
		break;

	case WDT_FLAG_RESET_CPU_CORE:
		data->mode = WDT_STAGE_ACTION_RESET_CPU;
		LOG_DBG("Configuring reset CPU mode");
		break;

	case WDT_FLAG_RESET_NONE:
		data->mode = WDT_STAGE_ACTION_OFF;
		LOG_DBG("Configuring non-reset mode");
		break;

	default:
		LOG_ERR("Unsupported watchdog config flag");
		return -EINVAL;
	}

	return 0;
}

static int wdt_esp32_init(const struct device *dev)
{
	const struct wdt_esp32_config *const config = dev->config;
	struct wdt_esp32_data *data = dev->data;
	int ret, flags;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	clock_control_on(config->clock_dev, config->clock_subsys);

	wdt_hal_init(&data->hal, config->wdt_inst, MWDT_TICK_PRESCALER, true);

	flags = ESP_PRIO_TO_FLAGS(config->irq_priority) | ESP_INT_FLAGS_CHECK(config->irq_flags);
	ret = esp_intr_alloc(config->irq_source, flags, (intr_handler_t)wdt_esp32_isr, (void *)dev,
			     NULL);

	if (ret != 0) {
		LOG_ERR("could not allocate interrupt (err %d)", ret);
		return ret;
	}

#ifndef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_esp32_enable(dev);
#endif

	return 0;
}

static DEVICE_API(wdt, wdt_api) = {
	.setup = wdt_esp32_set_config,
	.disable = wdt_esp32_disable,
	.install_timeout = wdt_esp32_install_timeout,
	.feed = wdt_esp32_feed
};

#define ESP32_WDT_INIT(idx)							   \
	static struct wdt_esp32_data wdt##idx##_data;				   \
	static struct wdt_esp32_config wdt_esp32_config##idx = {		   \
		.wdt_inst = WDT_MWDT##idx,	\
		.irq_source = DT_IRQ_BY_IDX(DT_NODELABEL(wdt##idx), 0, irq),	\
		.irq_priority = DT_IRQ_BY_IDX(DT_NODELABEL(wdt##idx), 0, priority),	\
		.irq_flags = DT_IRQ_BY_IDX(DT_NODELABEL(wdt##idx), 0, flags),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)), \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset), \
	};									   \
										   \
	DEVICE_DT_INST_DEFINE(idx,						   \
			      wdt_esp32_init,					   \
			      NULL,						   \
			      &wdt##idx##_data,					   \
			      &wdt_esp32_config##idx,				   \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	   \
			      &wdt_api)

static void wdt_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct wdt_esp32_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, 0);
	}

	wdt_hal_handle_intr(&data->hal);
}


#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wdt0))
ESP32_WDT_INIT(0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(wdt1))
ESP32_WDT_INIT(1);
#endif
