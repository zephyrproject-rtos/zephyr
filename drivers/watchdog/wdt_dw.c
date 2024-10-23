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
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include "wdt_dw.h"
#include "wdt_dw_common.h"

LOG_MODULE_REGISTER(wdt_dw, CONFIG_WDT_LOG_LEVEL);

/* Device run time data */
struct dw_wdt_dev_data {
	/* MMIO mapping information for watchdog register base address */
	DEVICE_MMIO_RAM;
	/* Clock frequency */
	uint32_t clk_freq;
	uint32_t config;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	wdt_callback_t callback;
#endif
};

/* Device constant configuration parameters */
struct dw_wdt_dev_cfg {
	DEVICE_MMIO_ROM;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	/* Clock controller dev instance */
	const struct device *clk_dev;
	/* Identifier for timer to get clock freq from clk manager */
	clock_control_subsys_t clkid;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	void (*irq_config)(void);
#endif
	uint8_t reset_pulse_length;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	/* Reset controller device configurations */
	struct reset_dt_spec reset_spec;
#endif
};

static int dw_wdt_setup(const struct device *dev, uint8_t options)
{
	struct dw_wdt_dev_data *const dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	int ret;

	ret = dw_wdt_check_options(options);
	if (ret != 0) {
		return ret;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	/* Configure response mode */
	dw_wdt_response_mode_set((uint32_t)reg_base, !!dev_data->callback);
#endif

	return dw_wdt_configure((uint32_t)reg_base, dev_data->config);
}

static int dw_wdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *config)
{
	__maybe_unused const struct dw_wdt_dev_cfg *const dev_config = dev->config;
	struct dw_wdt_dev_data *const dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	if (config == NULL) {
		LOG_ERR("watchdog timeout configuration missing");
		return -ENODATA;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	if (config->callback && !dev_config->irq_config) {
#else
	if (config->callback) {
#endif
		LOG_ERR("Interrupt is not configured, can't set a callback.");
		return -ENOTSUP;
	}

	if (config->flags) {
		LOG_WRN("Watchdog behavior is not configurable.");
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	dev_data->callback = config->callback;
#endif

	return dw_wdt_calc_period((uint32_t)reg_base, dev_data->clk_freq, config,
				  &dev_data->config);
}

static int dw_wdt_feed(const struct device *dev, int channel_id)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	/* Only channel 0 is supported */
	if (channel_id) {
		return -EINVAL;
	}

	dw_wdt_counter_restart((uint32_t)reg_base);

	return 0;
}

int dw_wdt_disable(const struct device *dev)
{
	int ret = -ENOTSUP;
	/*
	 * Once watchdog is enabled by setting WDT_EN bit watchdog cannot be disabled by clearing
	 * WDT_EN bit and to disable/clear WDT_EN bit watchdog IP should undergo reset
	 */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;

	/*
	 * Assert and de-assert reset only if the reset prop is defined in the device
	 * tree node for this dev instance
	 */
	if (dev_config->reset_spec.dev != NULL) {
		if (!device_is_ready(dev_config->reset_spec.dev)) {
			LOG_ERR("reset controller device not ready");
			return -ENODEV;
		}

		/* Assert and de-assert reset watchdog */
		ret = reset_line_toggle(dev_config->reset_spec.dev, dev_config->reset_spec.id);
		if (ret != 0) {
			LOG_ERR("watchdog disable/reset failed");
			return ret;
		}
	}
#endif
	return ret;
}

static const struct wdt_driver_api dw_wdt_api = {
	.setup = dw_wdt_setup,
	.disable = dw_wdt_disable,
	.install_timeout = dw_wdt_install_timeout,
	.feed = dw_wdt_feed,
};

static int dw_wdt_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;
	int ret;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	/* Reset watchdog controller if reset controller is supported */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	ret = dw_wdt_disable(dev);
	if (ret != 0) {
		return ret;
	}
#endif

	/* Get clock frequency from the clock manager if supported */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	struct dw_wdt_dev_data *const dev_data = dev->data;

	if (!device_is_ready(dev_config->clk_dev)) {
		LOG_ERR("Clock controller device not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(dev_config->clk_dev, dev_config->clkid,
				&dev_data->clk_freq);
	if (ret != 0) {
		LOG_ERR("Failed to get watchdog clock rate");
		return ret;
	}
#endif
	ret = dw_wdt_probe((uint32_t)reg_base, dev_config->reset_pulse_length);
	if (ret) {
		return ret;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
	if (dev_config->irq_config) {
		dev_config->irq_config();
	}
#endif

	/*
	 * Enable watchdog if it needs to be enabled at boot.
	 * watchdog timer will be started with maximum timeout
	 * that is the default value.
	 */
	if (!IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		dw_wdt_enable((uint32_t)reg_base);
	}

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
static void dw_wdt_isr(const struct device *dev)
{
	struct dw_wdt_dev_data *const dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	if (dw_wdt_interrupt_status_register_get((uint32_t)reg_base)) {

		/*
		 * Clearing interrupt here will not assert system reset, so interrupt
		 * will not be cleared here.
		 */
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

/* Bindings to the platform */
#define DW_WDT_IRQ_FLAGS(inst) \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(inst, sense), (DT_INST_IRQ(inst, sense)), (0))

#define DW_WDT_RESET_SPEC_INIT(inst) \
	.reset_spec = RESET_DT_SPEC_INST_GET(inst),

#define IRQ_CONFIG(inst)                                                                           \
	static void dw_wdt##inst##_irq_config(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), dw_wdt_isr,           \
			DEVICE_DT_INST_GET(inst), DW_WDT_IRQ_FLAGS(inst));                         \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define DW_WDT_INIT(inst)                                                                          \
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(inst), interrupts), (IRQ_CONFIG(inst)))            \
                                                                                                   \
	static const struct dw_wdt_dev_cfg wdt_dw##inst##_config = {                               \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.reset_pulse_length = ilog2(DT_INST_PROP_OR(inst, reset_pulse_length, 2)) - 1,     \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets),                                    \
			(DW_WDT_RESET_SPEC_INIT(inst)))                                            \
		IF_ENABLED(DT_PHA_HAS_CELL(DT_DRV_INST(inst), clocks, clkid),                      \
		(                                                                                  \
			.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                       \
			.clkid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid),         \
		))                                                                                 \
		IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(inst), interrupts),                        \
			(.irq_config = dw_wdt##inst##_irq_config,)                                 \
		)                                                                                  \
	};                                                                                         \
                                                                                                   \
	static struct dw_wdt_dev_data wdt_dw##inst##_data = {                                      \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency),                          \
			(.clk_freq = DT_INST_PROP(inst, clock_frequency)),                         \
			(COND_CODE_1(DT_PHA_HAS_CELL(DT_DRV_INST(inst), clocks, clkid),            \
			(.clk_freq = 0),                                                           \
			(.clk_freq = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency))))),   \
	};											   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &dw_wdt_init, NULL, &wdt_dw##inst##_data,                      \
			      &wdt_dw##inst##_config, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dw_wdt_api);

DT_INST_FOREACH_STATUS_OKAY(DW_WDT_INIT)
