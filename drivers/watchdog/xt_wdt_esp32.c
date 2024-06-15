/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT espressif_esp32_xt_wdt

#include <soc/rtc_cntl_reg.h>
#include <hal/xt_wdt_hal.h>
#include <rom/ets_sys.h>

#include <string.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>

#ifndef CONFIG_SOC_SERIES_ESP32C3
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#endif
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(xt_wdt_esp32, CONFIG_WDT_LOG_LEVEL);

#ifdef CONFIG_SOC_SERIES_ESP32C3
#define ISR_HANDLER isr_handler_t
#else
#define ISR_HANDLER intr_handler_t
#endif

#define ESP32_XT_WDT_MAX_TIMEOUT 255

struct esp32_xt_wdt_data {
	xt_wdt_hal_context_t hal;
	wdt_callback_t callback;
	uint32_t timeout;
};

struct esp32_xt_wdt_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
};

static int esp32_xt_wdt_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(options);
	struct esp32_xt_wdt_data *data = dev->data;

	xt_wdt_hal_config_t xt_wdt_hal_config = {
		.timeout = data->timeout,
	};

	xt_wdt_hal_init(&data->hal, &xt_wdt_hal_config);
	xt_wdt_hal_enable(&data->hal, true);

	return 0;
}

static int esp32_xt_wdt_disable(const struct device *dev)
{
	struct esp32_xt_wdt_data *data = dev->data;

	xt_wdt_hal_enable(&data->hal, false);

	return 0;
}

static int esp32_xt_wdt_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	return -ENOSYS;
}

static int esp32_xt_wdt_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	struct esp32_xt_wdt_data *data = dev->data;

	if (cfg->window.min != 0U || cfg->window.max == 0U ||
	    cfg->window.max >= ESP32_XT_WDT_MAX_TIMEOUT) {
		LOG_ERR("Invalid timeout configuration");
		return -EINVAL;
	}

	data->timeout = cfg->window.max;
	data->callback = cfg->callback;

	return 0;
}

static void esp32_xt_wdt_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct esp32_xt_wdt_config *cfg = dev->config;
	struct esp32_xt_wdt_data *data = dev->data;
	struct esp32_clock_config clk_cfg = {0};
	uint32_t status = REG_READ(RTC_CNTL_INT_ST_REG);

	REG_WRITE(RTC_CNTL_INT_CLR_REG, status);

	clk_cfg.rtc.rtc_slow_clock_src = ESP32_RTC_SLOW_CLK_SRC_RC_SLOW;

	clock_control_configure(cfg->clock_dev,
				(clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW,
				&clk_cfg);

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}

static int esp32_xt_wdt_init(const struct device *dev)
{
	const struct esp32_xt_wdt_config *cfg = dev->config;
	struct esp32_xt_wdt_data *data = dev->data;
	xt_wdt_hal_config_t xt_wdt_hal_config = {
		.timeout = ESP32_XT_WDT_MAX_TIMEOUT,
	};

	xt_wdt_hal_init(&data->hal, &xt_wdt_hal_config);
	xt_wdt_hal_enable_backup_clk(&data->hal,
						ESP32_RTC_SLOW_CLK_SRC_RC_SLOW_FREQ/1000);

	int err = esp_intr_alloc(cfg->irq_source, 0, (ISR_HANDLER)esp32_xt_wdt_isr, (void *)dev,
				 NULL);
	if (err) {
		LOG_ERR("Failed to register ISR\n");
		return -EFAULT;
	}

	REG_WRITE(RTC_CNTL_INT_ENA_REG, 0);
	REG_WRITE(RTC_CNTL_INT_CLR_REG, UINT32_MAX);

	return 0;
}

static const struct wdt_driver_api esp32_xt_wdt_api = {
	.setup = esp32_xt_wdt_setup,
	.disable = esp32_xt_wdt_disable,
	.install_timeout = esp32_xt_wdt_install_timeout,
	.feed = esp32_xt_wdt_feed
};

static struct esp32_xt_wdt_data esp32_xt_wdt_data0;

static struct esp32_xt_wdt_config esp32_xt_wdt_config0 = {
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
	.irq_source = DT_INST_IRQN(0),
};

DEVICE_DT_DEFINE(DT_NODELABEL(xt_wdt),
		 &esp32_xt_wdt_init,
		 NULL,
		 &esp32_xt_wdt_data0,
		 &esp32_xt_wdt_config0,
		 POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		 &esp32_xt_wdt_api);

#if !(defined(CONFIG_SOC_SERIES_ESP32S2) ||	\
	defined(CONFIG_SOC_SERIES_ESP32S3) ||   \
	defined(CONFIG_SOC_SERIES_ESP32C3))
#error "XT WDT is not supported"
#else
BUILD_ASSERT((DT_PROP(DT_INST(0, espressif_esp32_rtc), slow_clk_src) ==
	      ESP32_RTC_SLOW_CLK_SRC_XTAL32K) ||
		     (DT_PROP(DT_INST(0, espressif_esp32_rtc), slow_clk_src) ==
		      ESP32_RTC_SLOW_CLK_32K_EXT_OSC),
	     "XT WDT is only supported with XTAL32K or 32K_EXT_OSC as slow clock source");
#endif
