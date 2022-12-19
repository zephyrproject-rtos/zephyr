/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#define DT_DRV_COMPAT snps_designware_watchdog

#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/math/ilog2.h>

#include "wdt_dw.h"

LOG_MODULE_REGISTER(wdt_dw, CONFIG_WDT_LOG_LEVEL);

#define WDT_DW_FLAG_CONFIGURED		0x80000000

#define WDT_IS_INST_IRQ_EN(inst)	DT_NODE_HAS_PROP(DT_DRV_INST(inst), interrupts)
#define WDT_CHECK_INTERRUPT_USED(inst)	WDT_IS_INST_IRQ_EN(inst) ||
#define WDT_DW_INTERRUPT_SUPPORT	DT_INST_FOREACH_STATUS_OKAY(WDT_CHECK_INTERRUPT_USED) 0

/* Device run time data */
struct dw_wdt_dev_data {
	uint32_t config;
#if WDT_DW_INTERRUPT_SUPPORT
	wdt_callback_t callback;
#endif
};

/* Device constant configuration parameters */
struct dw_wdt_dev_cfg {
	uint32_t base;
	uint32_t clk_freq;
#if WDT_DW_INTERRUPT_SUPPORT
	void (*irq_config)(void);
#endif
	uint8_t reset_pulse_length;
};

static int dw_wdt_setup(const struct device *dev, uint8_t options)
{
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;
	struct dw_wdt_dev_data *const dev_data = dev->data;
	uint32_t period;

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		LOG_ERR("Pausing watchdog by debugger is not supported.");
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("Pausing watchdog in sleep is not supported.");
		return -ENOTSUP;
	}

	if (!(dev_data->config & WDT_DW_FLAG_CONFIGURED)) {
		LOG_ERR("Timeout not installed.");
		return -ENOTSUP;
	}

	/* Configure timeout */
	period = dev_data->config & ~WDT_DW_FLAG_CONFIGURED;

	if (dw_wdt_dual_timeout_period_get(dev_config->base)) {
		dw_wdt_timeout_period_init_set(dev_config->base, period);
	}

	dw_wdt_timeout_period_set(dev_config->base, period);

#if WDT_DW_INTERRUPT_SUPPORT
	/* Configure response mode */
	dw_wdt_response_mode_set(dev_config->base, !!dev_data->callback);
#endif

	/* Enable watchdog */
	dw_wdt_enable(dev_config->base);
	dw_wdt_counter_restart(dev_config->base);

	return 0;
}

static int dw_wdt_disable(const struct device *dev)
{
	return -ENOTSUP;
}

static int dw_wdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *config)
{
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;
	struct dw_wdt_dev_data *const dev_data = dev->data;
	uint64_t period64;
	uint32_t period;

#if WDT_DW_INTERRUPT_SUPPORT
	if (config->callback && !dev_config->irq_config) {
#else
	if (config->callback) {
#endif
		LOG_ERR("Interrupt is not configured, can't set a callback.");
		return -ENOTSUP;
	}

	/* Window timeout is not supported by this driver */
	if (config->window.min) {
		LOG_ERR("Window timeout is not supported.");
		return -ENOTSUP;
	}

	if (config->flags) {
		LOG_ERR("Watchdog behavior is not configurable.");
		return -ENOTSUP;
	}

	period64 = (uint64_t)dev_config->clk_freq * config->window.max;
	period64 /= 1000;
	if (!period64 || (period64 >> 31)) {
		LOG_ERR("Window max is out of range.");
		return -EINVAL;
	}

	period = period64 - 1;
	period = ilog2(period);

	if (period >= dw_wdt_cnt_width_get(dev_config->base)) {
		LOG_ERR("Watchdog timeout too long.");
		return -EINVAL;
	}

	/* The minimum counter value is 64k, maximum 2G */
	dev_data->config = WDT_DW_FLAG_CONFIGURED | (period >= 15 ? period - 15 : 0);
	dev_data->callback = config->callback;
	return 0;
}

static int dw_wdt_feed(const struct device *dev, int channel_id)
{
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;

	/* Only channel 0 is supported */
	if (channel_id) {
		return -EINVAL;
	}

	dw_wdt_counter_restart(dev_config->base);

	return 0;
}

static const struct wdt_driver_api dw_wdt_api = {
	.setup = dw_wdt_setup,
	.disable = dw_wdt_disable,
	.install_timeout = dw_wdt_install_timeout,
	.feed = dw_wdt_feed,
};

static int dw_wdt_init(const struct device *dev)
{
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;

	/* Check component type */
	if (dw_wdt_comp_type_get(dev_config->base) != WDT_COMP_TYPE_VALUE) {
		LOG_ERR("Invalid component type %x", dw_wdt_comp_type_get(dev_config->base));
		return -ENODEV;
	}

	dw_wdt_reset_pulse_length_set(dev_config->base, dev_config->reset_pulse_length);

#if WDT_DW_INTERRUPT_SUPPORT
	if (dev_config->irq_config) {
		dev_config->irq_config();
	}
#endif

	return 0;
}

#if WDT_DW_INTERRUPT_SUPPORT
static void dw_wdt_isr(const struct device *dev)
{
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;
	struct dw_wdt_dev_data *const dev_data = dev->data;

	if (dw_wdt_interrupt_status_register_get(dev_config->base)) {
		dw_wdt_clear_interrupt(dev_config->base);

		if (dev_data->callback) {
			dev_data->callback(dev, 0);
		}
	}
}
#endif

#define CHECK_CLOCK(inst)                                                                          \
	!(DT_INST_NODE_HAS_PROP(inst, clock_frequency) || DT_INST_NODE_HAS_PROP(inst, clocks)) ||

#if DT_INST_FOREACH_STATUS_OKAY(CHECK_CLOCK) 0
#error Clock frequency not configured!
#endif

#define IRQ_CONFIG(inst)                                                                           \
	static void dw_wdt##inst##_irq_config(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), dw_wdt_isr,           \
			DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, sense));                       \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define DW_WDT_INIT(inst)                                                                          \
	IF_ENABLED(IS_INST_IRQ_EN(inst), (IRQ_CONFIG(inst)))                                       \
                                                                                                   \
	static const struct dw_wdt_dev_cfg wdt_dw##inst##_config = {                               \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency),                          \
			(.clk_freq = DT_INST_PROP(inst, clock_frequency)),                         \
			(.clk_freq = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency))       \
		),                                                                                 \
		.reset_pulse_length = ilog2(DT_INST_PROP_OR(inst, reset_pulse_length, 2)) - 1,     \
		IF_ENABLED(IS_INST_IRQ_EN(inst),                                                   \
			(.irq_config = dw_wdt##inst##_irq_config,)                                 \
		)                                                                                  \
	};                                                                                         \
                                                                                                   \
	static struct dw_wdt_dev_data wdt_dw##inst##_data;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &dw_wdt_init, NULL, &wdt_dw##inst##_data,                      \
			      &wdt_dw##inst##_config, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dw_wdt_api);

DT_INST_FOREACH_STATUS_OKAY(DW_WDT_INIT)
