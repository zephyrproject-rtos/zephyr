/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT sifli_sf32lb_watchdog

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <register.h>

LOG_MODULE_REGISTER(wdt_sf32lb, CONFIG_WDT_LOG_LEVEL);

#define WDT_CMD_START 0x00000076U  /*!< WDT start */
#define WDT_CMD_STOP  0x00000034U  /*!< WDT stop */

struct wdt_sf32lb_config {
	WDT_TypeDef *reg;
	struct sf32lb_clock_dt_spec clock;
	void (*irq_cfg_func)(void);
};

struct wdt_sf32lb_data {
	wdt_callback_t callback;
	bool window_enabled;
	bool reset_enabled;
};

static void wdt_sf32lb_isr(const struct device *dev)
{
	const struct wdt_sf32lb_config *config = dev->config;
	struct wdt_sf32lb_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, 0);
	}

	config->reg->WDT_ICR = WDT_WDT_ICR_INT_CLR;
}

static int wdt_sf32lb_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_sf32lb_config *config = dev->config;
	int ret = 0;

	config->reg->WDT_CCR = WDT_CMD_START;

	return ret;
}

static int wdt_sf32lb_disable(const struct device *dev)
{
	const struct wdt_sf32lb_config *config = dev->config;
	struct wdt_sf32lb_data *data = dev->data;

	if (data->window_enabled) {
		if ((config->reg->WDT_SR & WDT_WDT_SR_INT_ASSERT) == WDT_WDT_SR_INT_ASSERT) {
			config->reg->WDT_ICR = WDT_WDT_ICR_INT_CLR;
		}
	}

	config->reg->WDT_CCR = WDT_CMD_STOP;

	return 0;
}

static int wdt_sf32lb_install_timeout(const struct device *dev,
				const struct wdt_timeout_cfg *cfg)
{
	const struct wdt_sf32lb_config *config = dev->config;
	struct wdt_sf32lb_data *data = dev->data;
	uint32_t cr = 0;

	data->reset_enabled = (cfg->flags & WDT_FLAG_RESET_SOC) != 0;
	data->window_enabled = (cfg->window.min != 0);

	if (cfg->window.max > 0x0FFFFFF) {
		return -EINVAL;
	}

	if (!data->reset_enabled) {
		return -ENOTSUP;
	}

	if (data->window_enabled) {
		cr |= WDT_WDT_CR_RESPONSE_MODE;

		if (cfg->callback) {
			data->callback = cfg->callback;
		}
#if 0
		if (cfg->window.min * 10 <= 0xFFFFFF) { /* 10K Hz */
			sys_write32(cfg->window.min * 10, config->reg->WDT_CVR0);
		} else {
			return -ENOTSUP;
		}
		if ((cfg->window.max * 10 / 2) <= 0xFFFFFF) {
			uint32_t diff = cfg->window.max - cfg->window.min;

			sys_write32(diff * 10, config->reg->WDT_CVR1);
		} else {
			return -ENOTSUP;
		}
#else
config->reg->WDT_CVR0 = cfg->window.min * 10;
config->reg->WDT_CVR1 = cfg->window.max * 10;
#endif
	} else {
		config->reg->WDT_CVR0 = cfg->window.max;
	}

	config->reg->WDT_CR = cr;

	return 0;
}

static int wdt_sf32lb_feed(const struct device *dev, int channel_id)
{
	const struct wdt_sf32lb_config *config = dev->config;

	sys_write32(WDT_CMD_START, config->reg->WDT_CCR);

	return 0;
}

static const struct wdt_driver_api wdt_sf32lb_api = {
	.setup = wdt_sf32lb_setup,
	.disable = wdt_sf32lb_disable,
	.install_timeout = wdt_sf32lb_install_timeout,
	.feed = wdt_sf32lb_feed,
};

static int wdt_sf32lb_init(const struct device *dev)
{
	const struct wdt_sf32lb_config *config = dev->config;
	int err =  0;

	if (config->clock.dev != NULL) {
		if (!sf3232lb_clock_is_ready_dt(&config->clock)) {
			LOG_ERR("Clock control device not ready");
			return -ENODEV;
		}

		err = sf32lb_clock_control_on_dt(&config->clock);
		if (err < 0) {
			LOG_ERR("Failed to enable clock");
			return err;
		}
	}

	if ((config->reg->WDT_SR & WDT_WDT_SR_WDT_ACTIVE) == WDT_WDT_SR_WDT_ACTIVE) {
		config->reg->WDT_CCR = WDT_CMD_STOP;
		while ((config->reg->WDT_SR & WDT_WDT_SR_WDT_ACTIVE) == WDT_WDT_SR_WDT_ACTIVE) {
		}
	};

	config->irq_cfg_func();

	return err;
}

#define WDT_SF32LB_INIT(index) \
	static void wdt_sf32lb_irq_cfg_##index(void) \
	{ \
		IRQ_CONNECT(DT_INST_IRQN(index), \
			    DT_INST_IRQ(index, priority), \
			    wdt_sf32lb_isr, DEVICE_DT_INST_GET(index), 0); \
		irq_enable(DT_INST_IRQN(index)); \
	} \
	static struct wdt_sf32lb_data wdt_sf32lb_data_##index; \
	static const struct wdt_sf32lb_config wdt_sf32lb_config_##index = { \
		.reg = (WDT_TypeDef *)DT_INST_REG_ADDR(index), \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET_OR(index, {}), \
		.irq_cfg_func = wdt_sf32lb_irq_cfg_##index, \
	}; \
	DEVICE_DT_INST_DEFINE(index, \
		wdt_sf32lb_init, \
		NULL, \
		&wdt_sf32lb_data_##index, \
		&wdt_sf32lb_config_##index, \
		POST_KERNEL, \
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
		&wdt_sf32lb_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_SF32LB_INIT)
