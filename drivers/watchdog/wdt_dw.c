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
#include "wdt_dw_common.h"

/* Check if reset property is defined */
#define WDT_DW_IS_INST_RESET_EN(inst)	DT_NODE_HAS_PROP(DT_DRV_INST(inst), resets)
#define WDT_DW_CHECK_RESET(inst)	WDT_DW_IS_INST_RESET_EN(inst) ||
#define WDT_DW_RESET_SUPPORT		DT_INST_FOREACH_STATUS_OKAY(WDT_DW_CHECK_RESET) 0

#if WDT_DW_RESET_SUPPORT
#include <zephyr/drivers/reset.h>
#endif

/* Check if clock manager is supported */
#define WDT_DW_IS_INST_CLK_MGR_EN(inst)	DT_PHA_HAS_CELL(DT_DRV_INST(inst), clocks, clkid)
#define WDT_DW_CHECK_CLK_MGR(inst)	WDT_DW_IS_INST_CLK_MGR_EN(inst) ||
#define WDT_DW_CLK_MANAGER_SUPPORT	DT_INST_FOREACH_STATUS_OKAY(WDT_DW_CHECK_CLK_MGR) 0

#if WDT_DW_CLK_MANAGER_SUPPORT
#include <zephyr/drivers/clock_control.h>
#endif

LOG_MODULE_REGISTER(wdt_dw, CONFIG_WDT_LOG_LEVEL);

#define WDT_IS_INST_IRQ_EN(inst)	DT_NODE_HAS_PROP(DT_DRV_INST(inst), interrupts)
#define WDT_CHECK_INTERRUPT_USED(inst)	WDT_IS_INST_IRQ_EN(inst) ||
#define WDT_DW_INTERRUPT_SUPPORT	DT_INST_FOREACH_STATUS_OKAY(WDT_CHECK_INTERRUPT_USED) 0

/* Device run time data */
struct dw_wdt_dev_data {
	/* MMIO mapping information for watchdog register base address */
	DEVICE_MMIO_RAM;
#if WDT_DW_CLK_MANAGER_SUPPORT
	/* Clock frequency */
	uint32_t clk_freq;
#endif
	uint32_t config;
#if WDT_DW_INTERRUPT_SUPPORT
	wdt_callback_t callback;
#endif
};

/* Device constant configuration parameters */
struct dw_wdt_dev_cfg {
	DEVICE_MMIO_ROM;

#if WDT_DW_CLK_MANAGER_SUPPORT
	/* Clock controller dev instance */
	const struct device *clk_dev;
	/* Identifier for timer to get clock freq from clk manager */
	clock_control_subsys_t clkid;
#endif

	uint32_t clk_freq;
#if WDT_DW_INTERRUPT_SUPPORT
	void (*irq_config)(void);
#endif
	uint8_t reset_pulse_length;
#if WDT_DW_RESET_SUPPORT
	/* Reset controller device configurations */
	struct reset_dt_spec reset_spec;
#endif
};

static int dw_wdt_setup(const struct device *dev, uint8_t options)
{
	int ret = 0;
	struct dw_wdt_dev_data *const dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	ret = dw_wdt_check_options(options);
	if (ret != 0) {
		return ret;
	}

#if WDT_DW_INTERRUPT_SUPPORT
	/* Configure response mode */
	dw_wdt_response_mode_set((uint32_t)reg_base, !!dev_data->callback);
#endif

	return dw_wdt_configure((uint32_t)reg_base, dev_data->config);
}

static int dw_wdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *config)
{
#if WDT_DW_INTERRUPT_SUPPORT || !WDT_DW_CLK_MANAGER_SUPPORT
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;
#endif
	struct dw_wdt_dev_data *const dev_data = dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	if (config == NULL) {
		LOG_ERR("watchdog timeout configuration missing");
		return -ENODATA;
	}

#if WDT_DW_INTERRUPT_SUPPORT
	if (config->callback && !dev_config->irq_config) {
#else
	if (config->callback) {
#endif
		LOG_ERR("Interrupt is not configured, can't set a callback.");
		return -ENOTSUP;
	}

	/*
	 * CONFIG_WDT_DW_RESET_MODE defines which of the reset(cpu reset, soc reset or no reset)
	 * is supported when watchdog expires
	 */
#if CONFIG_WDT_DW_RESET_MODE == 0
	if (config->flags) {
		LOG_ERR("Watchdog behavior is not configurable.");
		return -ENOTSUP;
	}
#elif CONFIG_WDT_DW_RESET_MODE == 1
	if (config->flags != WDT_FLAG_RESET_CPU_CORE) {
		LOG_ERR("only cpu core reset supported");
		return -ENOTSUP;
	}
#elif CONFIG_WDT_DW_RESET_MODE == 2
	if (config->flags != WDT_FLAG_RESET_SOC) {
		LOG_ERR("only SoC reset supported");
		return -ENOTSUP;
	}
#endif

#if WDT_DW_INTERRUPT_SUPPORT
	dev_data->callback = config->callback;
#endif

#if WDT_DW_CLK_MANAGER_SUPPORT
	return dw_wdt_calc_period((uint32_t)reg_base, dev_data->clk_freq, config,
				  &dev_data->config);
#else
	return dw_wdt_calc_period((uint32_t)reg_base, dev_config->clk_freq, config,
				  &dev_data->config);
#endif
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

int dw_wdt_disable_wrapper(const struct device *dev)
{
	int ret = 0;
	/*
	 * Once watchdog is enabled by setting WDT_EN bit watchdog cannot be disabled by clearing
	 * WDT_EN bit and to disable/clear WDT_EN bit watchdog IP should be resetted
	 */
#if WDT_DW_RESET_SUPPORT
	const struct dw_wdt_dev_cfg *const dev_config = dev->config;

	if (!device_is_ready(dev_config->reset_spec.dev)) {
		LOG_ERR("reset controller device not ready");
		return -ENODEV;
	}

	/* Assert and deassert reset watchdog */
	ret = reset_line_toggle(dev_config->reset_spec.dev, dev_config->reset_spec.id);
	if (ret != 0) {
		LOG_ERR("watchdog disable/reset failed");
		return ret;
	}
#else
	ret = dw_wdt_disable(dev);
#endif
	return ret;
}

static const struct wdt_driver_api dw_wdt_api = {
	.setup = dw_wdt_setup,
	.disable = dw_wdt_disable_wrapper,
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
#if WDT_DW_RESET_SUPPORT
	ret = dw_wdt_disable_wrapper(dev);
	if (ret != 0) {
		return ret;
	}
#endif

	/* Get clock frequency from the clock manager if supported */
#if WDT_DW_CLK_MANAGER_SUPPORT
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
	if (ret)
		return ret;

#if WDT_DW_INTERRUPT_SUPPORT
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

#if WDT_DW_INTERRUPT_SUPPORT
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
	IF_ENABLED(WDT_IS_INST_IRQ_EN(inst), (IRQ_CONFIG(inst)))                                   \
                                                                                                   \
	static const struct dw_wdt_dev_cfg wdt_dw##inst##_config = {                               \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency),                          \
			(.clk_freq = DT_INST_PROP(inst, clock_frequency)),                         \
			(COND_CODE_1(DT_PHA_HAS_CELL(DT_DRV_INST(inst), clocks, clkid),            \
			(                                                                          \
				.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),               \
				.clkid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid), \
				.clk_freq = 0                                                      \
			),                                                                         \
			(.clk_freq = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency))       \
			)                                                                          \
			)                                                                          \
		),                                                                                 \
		.reset_pulse_length = ilog2(DT_INST_PROP_OR(inst, reset_pulse_length, 2)) - 1,     \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets),                                    \
			(DW_WDT_RESET_SPEC_INIT(inst)))                                            \
		IF_ENABLED(WDT_IS_INST_IRQ_EN(inst),                                               \
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
