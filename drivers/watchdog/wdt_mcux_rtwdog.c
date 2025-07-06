/*
 * Copyright 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtwdog

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys_clock.h>
#include <fsl_rtwdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_mcux_rtwdog);

#define MSEC_TO_RTWDOG_TICKS(clock_freq, divider, msec)                                            \
	((uint32_t)(clock_freq * msec / 1000U / divider))

#define RTWDOG_MIN_TIMEOUT 1U

struct mcux_rtwdog_config {
	RTWDOG_Type *base;
	uint32_t clock_frequency;
	rtwdog_clock_source_t clk_source;
	rtwdog_clock_prescaler_t clk_divider;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_rtwdog_data {
	wdt_callback_t callback;
	rtwdog_config_t wdog_config;
	bool timeout_valid;
	bool enabled;
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

	if (data->enabled) {
		LOG_ERR("This watchdog has been enabled");
		return -EBUSY;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0U) {
		LOG_ERR("Not support WDT_OPT_PAUSE_IN_SLEEP");
		return -ENOTSUP;
	}

	data->wdog_config.workMode.enableDebug = ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U);

	RTWDOG_Init(base, &data->wdog_config);
	data->enabled = true;
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_rtwdog_disable(const struct device *dev)
{
	const struct mcux_rtwdog_config *config = dev->config;
	struct mcux_rtwdog_data *data = dev->data;
	RTWDOG_Type *base = config->base;

	data->timeout_valid = false;

	if (!data->enabled) {
		LOG_ERR("Disabled when watchdog is not enabled");
		return -EFAULT;
	}

	RTWDOG_Deinit(base);
	data->enabled = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

static int mcux_rtwdog_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct mcux_rtwdog_config *config = dev->config;
	struct mcux_rtwdog_data *data = dev->data;
	uint32_t clock_freq;
	uint32_t clk_divider;

	if (data->enabled) {
		LOG_ERR("Timeout can not be installed while watchdog has already been setup");
		return -EBUSY;
	}

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if (cfg->flags == WDT_FLAG_RESET_NONE) {
		LOG_ERR("Not support WDT_FLAG_RESET_NONE");
		return -ENOTSUP;
	}

	RTWDOG_GetDefaultConfig(&data->wdog_config);

	clock_freq = config->clock_frequency;
	clk_divider = config->clk_divider == kRTWDOG_ClockPrescalerDivide1 ? 1U : 256U;

	data->wdog_config.clockSource = config->clk_source;
	data->wdog_config.prescaler = config->clk_divider;
	data->wdog_config.timeoutValue =
		MSEC_TO_RTWDOG_TICKS(clock_freq, clk_divider, cfg->window.max);

	if (data->wdog_config.timeoutValue > UINT16_MAX) {
		LOG_ERR("Invalid window max");
		return -EINVAL;
	}

	if (cfg->window.min != 0U) {
		data->wdog_config.enableWindowMode = true;
		data->wdog_config.windowValue =
			MSEC_TO_RTWDOG_TICKS(clock_freq, clk_divider, cfg->window.min);
	} else {
		data->wdog_config.enableWindowMode = false;
		data->wdog_config.windowValue = 0;
	}

	if ((data->wdog_config.timeoutValue < RTWDOG_MIN_TIMEOUT) ||
	    (data->wdog_config.timeoutValue <= data->wdog_config.windowValue)) {
		LOG_ERR("Invalid timeout");
		return -EINVAL;
	}

	data->wdog_config.enableInterrupt = cfg->callback != NULL;
	data->callback = cfg->callback;
	data->timeout_valid = true;

	return 0;
}

static int mcux_rtwdog_feed(const struct device *dev, int channel_id)
{
	const struct mcux_rtwdog_config *config = dev->config;
	struct mcux_rtwdog_data *data = dev->data;
	RTWDOG_Type *base = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (!data->enabled) {
		LOG_ERR("Feed disabled watchdog");
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

	RTWDOG_ClearStatusFlags(base, kRTWDOG_InterruptFlag);

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int mcux_rtwdog_init(const struct device *dev)
{
	const struct mcux_rtwdog_config *config = dev->config;

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, mcux_rtwdog_api) = {
	.setup = mcux_rtwdog_setup,
	.disable = mcux_rtwdog_disable,
	.install_timeout = mcux_rtwdog_install_timeout,
	.feed = mcux_rtwdog_feed,
};

#define TO_RTWDOG_CLK_SRC(val) _DO_CONCAT(kRTWDOG_ClockSource, val)
#define TO_RTWDOG_CLK_DIV(val) _DO_CONCAT(kRTWDOG_ClockPrescalerDivide, val)

#define MCUX_RTWDOG_INIT(n)                                                                        \
                                                                                                   \
	static struct mcux_rtwdog_data mcux_rtwdog_data_##n;                                       \
	static void mcux_rtwdog_config_func_##n(const struct device *dev);                         \
                                                                                                   \
	static const struct mcux_rtwdog_config mcux_rtwdog_config_##n = {                          \
		.base = (RTWDOG_Type *)DT_INST_REG_ADDR(n),                                        \
		.irq_config_func = mcux_rtwdog_config_func_##n,                                    \
		.clock_frequency = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),            \
		.clk_source = TO_RTWDOG_CLK_SRC(DT_INST_PROP(n, clk_source)),                      \
		.clk_divider = TO_RTWDOG_CLK_DIV(DT_INST_PROP(n, clk_divider)),                    \
	};                                                                                         \
	static void mcux_rtwdog_config_func_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_rtwdog_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &mcux_rtwdog_init, NULL, &mcux_rtwdog_data_##n,                   \
			      &mcux_rtwdog_config_##n, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_rtwdog_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_RTWDOG_INIT)
