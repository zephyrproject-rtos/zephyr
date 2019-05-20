/*
 * Copyright (2) 2019 Vestas Wind Systems A/S
 *
 * Based on wdt_mcux_wdog.c, which is:
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/watchdog.h>
#include <drivers/clock_control.h>
#include <fsl_wdog32.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wdt_mcux_wdog32);

#define MIN_TIMEOUT 1

struct mcux_wdog32_config {
	WDOG_Type *base;
#ifdef DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY
	u32_t clock_frequency;
#else /* !DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY */
	char *clock_name;
	clock_control_subsys_t clock_subsys;
#endif /* !DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY */
	wdog32_clock_source_t clk_source;
	wdog32_clock_prescaler_t clk_divider;
	void (*irq_config_func)(struct device *dev);
};

struct mcux_wdog32_data {
	wdt_callback_t callback;
	wdog32_config_t wdog_config;
	bool timeout_valid;
};

static int mcux_wdog32_setup(struct device *dev, u8_t options)
{
	const struct mcux_wdog32_config *config = dev->config->config_info;
	struct mcux_wdog32_data *data = dev->driver_data;
	WDOG_Type *base = config->base;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	data->wdog_config.workMode.enableStop =
		(options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;

	data->wdog_config.workMode.enableDebug =
		(options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;

	WDOG32_Init(base, &data->wdog_config);
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_wdog32_disable(struct device *dev)
{
	const struct mcux_wdog32_config *config = dev->config->config_info;
	struct mcux_wdog32_data *data = dev->driver_data;
	WDOG_Type *base = config->base;

	WDOG32_Deinit(base);
	data->timeout_valid = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

#define MSEC_TO_WDOG32_TICKS(clock_freq, divider, msec) \
	((u32_t)(clock_freq * msec / 1000U / divider))

static int mcux_wdog32_install_timeout(struct device *dev,
				       const struct wdt_timeout_cfg *cfg)
{
	const struct mcux_wdog32_config *config = dev->config->config_info;
	struct mcux_wdog32_data *data = dev->driver_data;
	u32_t clock_freq;
	int div;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

#ifdef DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY
	clock_freq = config->clock_frequency;
#else /* !DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY */
	struct device *clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}
#endif /* !DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY */

	div = config->clk_divider == kWDOG32_ClockPrescalerDivide1 ? 1U : 256U;

	WDOG32_GetDefaultConfig(&data->wdog_config);

	data->wdog_config.timeoutValue =
		MSEC_TO_WDOG32_TICKS(clock_freq, div, cfg->window.max);

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
	LOG_DBG("Installed timeout (timeoutValue = %d)",
		data->wdog_config.timeoutValue);

	return 0;
}

static int mcux_wdog32_feed(struct device *dev, int channel_id)
{
	const struct mcux_wdog32_config *config = dev->config->config_info;
	WDOG_Type *base = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	WDOG32_Refresh(base);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static void mcux_wdog32_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct mcux_wdog32_config *config = dev->config->config_info;
	struct mcux_wdog32_data *data = dev->driver_data;
	WDOG_Type *base = config->base;
	u32_t flags;

	flags = WDOG32_GetStatusFlags(base);
	WDOG32_ClearStatusFlags(base, flags);

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int mcux_wdog32_init(struct device *dev)
{
	const struct mcux_wdog32_config *config = dev->config->config_info;

	config->irq_config_func(dev);

	return 0;
}

static const struct wdt_driver_api mcux_wdog32_api = {
	.setup = mcux_wdog32_setup,
	.disable = mcux_wdog32_disable,
	.install_timeout = mcux_wdog32_install_timeout,
	.feed = mcux_wdog32_feed,
};

#define TO_WDOG32_CLK_SRC(val) _DO_CONCAT(kWDOG32_ClockSource, val)
#define TO_WDOG32_CLK_DIV(val) _DO_CONCAT(kWDOG32_ClockPrescalerDivide, val)

static void mcux_wdog32_config_func_0(struct device *dev);

static const struct mcux_wdog32_config mcux_wdog32_config_0 = {
	.base = (WDOG_Type *) DT_INST_0_NXP_KINETIS_WDOG32_BASE_ADDRESS,
#ifdef DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY
	.clock_frequency = DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY,
#else /* !DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY */
	.clock_name = DT_INST_0_NXP_KINETIS_WDOG32_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)
		DT_INST_0_NXP_KINETIS_WDOG32_CLOCK_NAME,
#endif /* DT_INST_0_NXP_KINETIS_WDOG32_CLOCKS_CLOCK_FREQUENCY */
	.clk_source =
		TO_WDOG32_CLK_SRC(DT_INST_0_NXP_KINETIS_WDOG32_CLK_SOURCE),
	.clk_divider =
		TO_WDOG32_CLK_DIV(DT_INST_0_NXP_KINETIS_WDOG32_CLK_DIVIDER),
	.irq_config_func = mcux_wdog32_config_func_0,
};

static struct mcux_wdog32_data mcux_wdog32_data_0;

DEVICE_AND_API_INIT(mcux_wdog32_0, DT_INST_0_NXP_KINETIS_WDOG32_LABEL,
		    &mcux_wdog32_init, &mcux_wdog32_data_0,
		    &mcux_wdog32_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_wdog32_api);

static void mcux_wdog32_config_func_0(struct device *dev)
{
	IRQ_CONNECT(DT_INST_0_NXP_KINETIS_WDOG32_IRQ_0,
		    DT_INST_0_NXP_KINETIS_WDOG32_IRQ_0_PRIORITY,
		    mcux_wdog32_isr, DEVICE_GET(mcux_wdog32_0), 0);

	irq_enable(DT_INST_0_NXP_KINETIS_WDOG32_IRQ_0);
}
