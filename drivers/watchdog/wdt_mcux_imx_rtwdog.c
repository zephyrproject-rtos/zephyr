/*
 * Copyright (C) 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_rtwdog

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys_clock.h>
#include <fsl_rtwdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_mcux_imx_rtwdog);

#define MSEC_TO_RTWDOG_TICKS(clock_freq, divider, msec)                                            \
	((uint32_t)(clock_freq * msec / 1000U / divider))

#ifndef U16_MAX
#define U16_MAX 0xFFFFU
#endif

struct mcux_rtwdog_config {
	RTWDOG_Type *base;
#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency)
	uint32_t clock_frequency;
#else  /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#endif /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	rtwdog_clock_source_t clk_source;
	rtwdog_clock_prescaler_t clk_divider;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
};

struct mcux_rtwdog_data {
	wdt_callback_t callback;
	rtwdog_config_t wdog_config;
	bool timeout_valid;
};

static int mcux_rtwdog_setup(const struct device *dev, uint8_t options)
{
	const struct mcux_rtwdog_config *config = dev->config;
	struct mcux_rtwdog_data *data = dev->data;
	RTWDOG_Type *base = config->base;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	data->wdog_config.workMode.enableStop = (options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;

	data->wdog_config.workMode.enableDebug = (options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;

	RTWDOG_Init(base, &data->wdog_config);
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_rtwdog_disable(const struct device *dev)
{
	const struct mcux_rtwdog_config *config = dev->config;
	struct mcux_rtwdog_data *data = dev->data;
	RTWDOG_Type *base = config->base;

	RTWDOG_Deinit(base);
	data->timeout_valid = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

static int mcux_rtwdog_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct mcux_rtwdog_config *config = dev->config;
	struct mcux_rtwdog_data *data = dev->data;
	uint32_t clock_freq;
	uint32_t timeoutCycle;
	uint32_t div;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	RTWDOG_GetDefaultConfig(&data->wdog_config);

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

	div = config->clk_divider == kRTWDOG_ClockPrescalerDivide1 ? 1U : 256U;

	data->wdog_config.clockSource = config->clk_source;
	data->wdog_config.prescaler = config->clk_divider;

	timeoutCycle = MSEC_TO_RTWDOG_TICKS(clock_freq, div, cfg->window.max);
	if (timeoutCycle > U16_MAX) {
		LOG_ERR("Invalid window max");
		return -EINVAL;
	}
	data->wdog_config.timeoutValue = timeoutCycle;

	if (cfg->window.min) {
		data->wdog_config.enableWindowMode = true;
		data->wdog_config.windowValue =
			MSEC_TO_RTWDOG_TICKS(clock_freq, div, cfg->window.min);
	} else {
		data->wdog_config.enableWindowMode = false;
		data->wdog_config.windowValue = 0;
	}

	data->wdog_config.enableInterrupt = cfg->callback != NULL;
	data->callback = cfg->callback;
	data->timeout_valid = true;

	return 0;
}

static int mcux_rtwdog_feed(const struct device *dev, int channel_id)
{
	const struct mcux_rtwdog_config *config = dev->config;
	RTWDOG_Type *base = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	RTWDOG_Refresh(base);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static void mcux_rtwdog_isr(const struct device *dev)
{
	const struct mcux_rtwdog_config *config = dev->config;
	struct mcux_rtwdog_data *data = dev->data;
	RTWDOG_Type *base = config->base;
	uint32_t flags;

	flags = RTWDOG_GetStatusFlags(base);
	RTWDOG_ClearStatusFlags(base, flags);

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int mcux_rtwdog_init(const struct device *dev)
{
	const struct mcux_rtwdog_config *config = dev->config;
	int ret;

	config->irq_config_func(dev);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	return 0;
}

static const struct wdt_driver_api mcux_rtwdog_api = {
	.setup = mcux_rtwdog_setup,
	.disable = mcux_rtwdog_disable,
	.install_timeout = mcux_rtwdog_install_timeout,
	.feed = mcux_rtwdog_feed,
};

static void mcux_rtwdog_config_func(const struct device *dev);

PINCTRL_DT_INST_DEFINE(0);

#define TO_RTWDOG_CLK_SRC(val) _DO_CONCAT(kRTWDOG_ClockSource, val)
#define TO_RTWDOG_CLK_DIV(val) _DO_CONCAT(kRTWDOG_ClockPrescalerDivide, val)

static const struct mcux_rtwdog_config mcux_rtwdog_config = {
	.base = (RTWDOG_Type *)DT_INST_REG_ADDR(0),
	.irq_config_func = mcux_rtwdog_config_func,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#if DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency)
	.clock_frequency = DT_INST_PROP_BY_PHANDLE(0, clocks, clock_frequency),
#else  /* !DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, name),
#endif /* DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, clocks), clock_frequency) */
	.clk_source = TO_RTWDOG_CLK_SRC(DT_INST_PROP(0, clk_source)),
	.clk_divider = TO_RTWDOG_CLK_DIV(DT_INST_PROP(0, clk_divider)),
};

static struct mcux_rtwdog_data mcux_rtwdog_data;

DEVICE_DT_INST_DEFINE(0, &mcux_rtwdog_init, NULL, &mcux_rtwdog_data, &mcux_rtwdog_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_rtwdog_api);

static void mcux_rtwdog_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mcux_rtwdog_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
