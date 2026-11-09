/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc_wdt

#include <hal/rwdt_ll.h>
#include <hal/wdt_hal.h>
#include <esp_attr.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rtc_wdt_esp32, CONFIG_WDT_LOG_LEVEL);

struct rtc_wdt_esp32_data {
	wdt_hal_context_t hal;
	uint32_t timeout_ticks;
	wdt_stage_action_t action;
	wdt_callback_t callback;
	bool installed;
	bool enabled;
};

struct rtc_wdt_esp32_config {
	const struct device *clock_dev;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

static int rtc_wdt_esp32_setup(const struct device *dev, uint8_t options)
{
	struct rtc_wdt_esp32_data *data = dev->data;

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0U) {
		return -ENOTSUP;
	}

	if (!data->installed) {
		return -EINVAL;
	}

	if (data->enabled) {
		return -EBUSY;
	}

	wdt_hal_init(&data->hal, WDT_RWDT, 0, data->callback != NULL);
	wdt_hal_write_protect_disable(&data->hal);
	rwdt_ll_set_pause_in_sleep_en(data->hal.rwdt_dev, (options & WDT_OPT_PAUSE_IN_SLEEP) != 0U);
	if (data->callback != NULL) {
		wdt_hal_config_stage(&data->hal, WDT_STAGE0, data->timeout_ticks,
				     WDT_STAGE_ACTION_INT);
		wdt_hal_config_stage(&data->hal, WDT_STAGE1, data->timeout_ticks, data->action);
	} else {
		wdt_hal_config_stage(&data->hal, WDT_STAGE0, data->timeout_ticks, data->action);
	}
	wdt_hal_enable(&data->hal);
	wdt_hal_feed(&data->hal);
	wdt_hal_write_protect_enable(&data->hal);

	data->enabled = true;

	return 0;
}

static int rtc_wdt_esp32_disable(const struct device *dev)
{
	struct rtc_wdt_esp32_data *data = dev->data;

	if (!data->enabled) {
		data->installed = false;
		return -EFAULT;
	}

	wdt_hal_write_protect_disable(&data->hal);
	wdt_hal_disable(&data->hal);
	wdt_hal_config_stage(&data->hal, WDT_STAGE0, 0, WDT_STAGE_ACTION_OFF);
	wdt_hal_config_stage(&data->hal, WDT_STAGE1, 0, WDT_STAGE_ACTION_OFF);
	wdt_hal_write_protect_enable(&data->hal);

	data->enabled = false;
	data->installed = false;

	return 0;
}

static int rtc_wdt_esp32_install_timeout(const struct device *dev,
					 const struct wdt_timeout_cfg *cfg)
{
	const struct rtc_wdt_esp32_config *config = dev->config;
	struct rtc_wdt_esp32_data *data = dev->data;
	uint32_t slow_clk_hz;
	uint64_t ticks;

	if (data->enabled) {
		return -EBUSY;
	}

	if (data->installed) {
		return -ENOMEM;
	}

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	if (clock_control_get_rate(config->clock_dev,
				   (clock_control_subsys_t)ESP32_CLOCK_CONTROL_SUBSYS_RTC_SLOW,
				   &slow_clk_hz) != 0 ||
	    slow_clk_hz == 0U) {
		return -EIO;
	}

	ticks = (uint64_t)cfg->window.max * slow_clk_hz / 1000U;
	if (ticks == 0U || ticks > UINT32_MAX) {
		return -EINVAL;
	}

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		data->action = WDT_STAGE_ACTION_RESET_SYSTEM;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		data->action = WDT_STAGE_ACTION_RESET_CPU;
		break;
	case WDT_FLAG_RESET_NONE:
		if (cfg->callback == NULL) {
			return -EINVAL;
		}
		data->action = WDT_STAGE_ACTION_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	data->timeout_ticks = (uint32_t)ticks;
	data->callback = cfg->callback;
	data->installed = true;

	return 0;
}

static int rtc_wdt_esp32_feed(const struct device *dev, int channel_id)
{
	struct rtc_wdt_esp32_data *data = dev->data;

	if (channel_id != 0) {
		return -EINVAL;
	}

	if (!data->enabled) {
		return -EINVAL;
	}

	wdt_hal_write_protect_disable(&data->hal);
	wdt_hal_feed(&data->hal);
	wdt_hal_write_protect_enable(&data->hal);

	return 0;
}

static void IRAM_ATTR rtc_wdt_esp32_isr(void *arg)
{
	const struct device *dev = arg;
	struct rtc_wdt_esp32_data *data = dev->data;

	if (!rwdt_ll_check_intr_status(data->hal.rwdt_dev)) {
		return;
	}

	rwdt_ll_clear_intr_status(data->hal.rwdt_dev);

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}

static int rtc_wdt_esp32_init(const struct device *dev)
{
	const struct rtc_wdt_esp32_config *config = dev->config;
	struct rtc_wdt_esp32_data *data = dev->data;
	int flags;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	wdt_hal_init(&data->hal, WDT_RWDT, 0, false);

	flags = ESP_PRIO_TO_FLAGS(config->irq_priority) | ESP_INT_FLAGS_CHECK(config->irq_flags) |
		ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM;
	ret = esp_intr_alloc(config->irq_source, flags, rtc_wdt_esp32_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("could not allocate interrupt (err %d)", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(wdt, rtc_wdt_esp32_api) = {
	.setup = rtc_wdt_esp32_setup,
	.disable = rtc_wdt_esp32_disable,
	.install_timeout = rtc_wdt_esp32_install_timeout,
	.feed = rtc_wdt_esp32_feed,
};

#define RTC_WDT_ESP32_INIT(inst)                                                                   \
	static struct rtc_wdt_esp32_data rtc_wdt_esp32_data_##inst;                                \
	static const struct rtc_wdt_esp32_config rtc_wdt_esp32_config_##inst = {                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.irq_source = DT_INST_IRQ_BY_IDX(inst, 0, irq),                                    \
		.irq_priority = DT_INST_IRQ_BY_IDX(inst, 0, priority),                             \
		.irq_flags = DT_INST_IRQ_BY_IDX(inst, 0, flags),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, rtc_wdt_esp32_init, NULL, &rtc_wdt_esp32_data_##inst,          \
			      &rtc_wdt_esp32_config_##inst, PRE_KERNEL_1,                          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &rtc_wdt_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_WDT_ESP32_INIT)
