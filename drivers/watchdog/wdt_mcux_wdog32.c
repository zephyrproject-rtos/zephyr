/*
 * Copyright (2) 2019 Vestas Wind Systems A/S
 * Copyright 2025 NXP
 *
 * Based on wdt_mcux_wdog.c, which is:
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_wdog32

#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/device_mmio.h>
#include <fsl_wdog32.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_mcux_wdog32);

#define MIN_TIMEOUT 1

#define DEV_CFG(_dev)  ((const struct mcux_wdog32_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_wdog32_data *)(_dev)->data)

struct mcux_wdog32_config {
	DEVICE_MMIO_NAMED_ROM(reg);
#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency)
	uint32_t clock_frequency;
#else  /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#endif /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	wdog32_clock_source_t clk_source;
	wdog32_clock_prescaler_t clk_divider;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_wdog32_data {
	DEVICE_MMIO_NAMED_RAM(reg);
	wdt_callback_t callback;
	wdog32_config_t wdog_config;
	bool timeout_valid;
};

static inline WDOG_Type *get_base_address(const struct device *dev)
{
	return (WDOG_Type *)DEVICE_MMIO_NAMED_GET(dev, reg);
}

/* When system is boot up, WDOG32 is disabled. app must wait for at least 2.5
 * periods of wdog32 clock to reconfigure wodg32. So Delay a while to wait for
 * the previous configuration taking effect.
 */
#define WDOG32_CONFIG_WAIT(clock_hz, divider)                                                      \
	do {                                                                                       \
		uint64_t _num = 5ULL * 1000ULL * (uint64_t)divider;                                \
		uint64_t _den = 2UL * (uint64_t)clock_hz;                                          \
		uint32_t ms = (uint32_t)((_num + _den - 1ULL) / _den);                             \
		k_msleep(ms ? ms : 1u);                                                            \
	} while (0)

static int mcux_wdog32_setup(const struct device *dev, uint8_t options)
{
	const struct mcux_wdog32_config *config = dev->config;
	struct mcux_wdog32_data *data = dev->data;
	WDOG_Type *base = get_base_address(dev);
	uint32_t clock_freq;
	int divider;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	data->wdog_config.workMode.enableStop = (options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;

	data->wdog_config.workMode.enableDebug = (options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;

#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency)
	clock_freq = config->clock_frequency;
#else  /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		return -EINVAL;
	}
#endif /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	divider = config->clk_divider == kWDOG32_ClockPrescalerDivide1 ? 1U : 256U;
	WDOG32_CONFIG_WAIT(clock_freq, divider);
	WDOG32_Init(base, &data->wdog_config);
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_wdog32_disable(const struct device *dev)
{
	struct mcux_wdog32_data *data = dev->data;
	WDOG_Type *base = get_base_address(dev);

	WDOG32_Deinit(base);
	data->timeout_valid = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

#define MSEC_TO_WDOG32_TICKS(clock_freq, divider, msec)                                            \
	((uint32_t)(clock_freq * msec / 1000U / divider))

static int mcux_wdog32_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct mcux_wdog32_config *config = dev->config;
	struct mcux_wdog32_data *data = dev->data;
	uint32_t clock_freq;
	int div;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency)
	clock_freq = config->clock_frequency;
#else  /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		return -EINVAL;
	}
#endif /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */

	div = config->clk_divider == kWDOG32_ClockPrescalerDivide1 ? 1U : 256U;

	WDOG32_GetDefaultConfig(&data->wdog_config);

	data->wdog_config.timeoutValue = MSEC_TO_WDOG32_TICKS(clock_freq, div, cfg->window.max);

	if (cfg->window.min) {
		data->wdog_config.enableWindowMode = true;
		data->wdog_config.windowValue =
			MSEC_TO_WDOG32_TICKS(clock_freq, div, cfg->window.min);
	} else {
		data->wdog_config.enableWindowMode = false;
		data->wdog_config.windowValue = 0;
	}

	if ((data->wdog_config.timeoutValue < MIN_TIMEOUT) ||
	    (data->wdog_config.timeoutValue < data->wdog_config.windowValue)) {
		LOG_ERR("Invalid timeout");
		return -EINVAL;
	}

	data->wdog_config.prescaler = config->clk_divider;
	data->wdog_config.clockSource = config->clk_source;
	data->wdog_config.enableInterrupt = cfg->callback != NULL;
	data->callback = cfg->callback;
	data->timeout_valid = true;
	LOG_DBG("Installed timeout (timeoutValue = %d)", data->wdog_config.timeoutValue);

	return 0;
}

static int mcux_wdog32_feed(const struct device *dev, int channel_id)
{
	WDOG_Type *base = get_base_address(dev);

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	WDOG32_Refresh(base);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static void mcux_wdog32_isr(const struct device *dev)
{
	struct mcux_wdog32_data *data = dev->data;
#ifndef CONFIG_SOC_MIMX9352
	WDOG_Type *base = get_base_address(dev);
	uint32_t flags;

	flags = WDOG32_GetStatusFlags(base);
	WDOG32_ClearStatusFlags(base, flags);
#endif

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int mcux_wdog32_init(const struct device *dev)
{
	const struct mcux_wdog32_config *config = dev->config;

	/* Map the named MMIO region */
	DEVICE_MMIO_NAMED_MAP(dev, reg, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, mcux_wdog32_api) = {
	.setup = mcux_wdog32_setup,
	.disable = mcux_wdog32_disable,
	.install_timeout = mcux_wdog32_install_timeout,
	.feed = mcux_wdog32_feed,
};

#define TO_WDOG32_CLK_SRC(val) _DO_CONCAT(kWDOG32_ClockSource, val)
#define TO_WDOG32_CLK_DIV(val) _DO_CONCAT(kWDOG32_ClockPrescalerDivide, val)

static void mcux_wdog32_config_func_0(const struct device *dev);

static const struct mcux_wdog32_config mcux_wdog32_config_0 = {
	DEVICE_MMIO_NAMED_ROM_INIT(reg, DT_DRV_INST(0)),
#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency)
	.clock_frequency = DT_INST_PROP_BY_PHANDLE(0, clocks, clock_frequency),
#else  /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, name),
#endif /* DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	.clk_source = TO_WDOG32_CLK_SRC(DT_INST_PROP(0, clk_source)),
	.clk_divider = TO_WDOG32_CLK_DIV(DT_INST_PROP(0, clk_divider)),
	.irq_config_func = mcux_wdog32_config_func_0,
};

static struct mcux_wdog32_data mcux_wdog32_data_0;

DEVICE_DT_INST_DEFINE(0, &mcux_wdog32_init, NULL, &mcux_wdog32_data_0, &mcux_wdog32_config_0,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_wdog32_api);

static void mcux_wdog32_config_func_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mcux_wdog32_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
