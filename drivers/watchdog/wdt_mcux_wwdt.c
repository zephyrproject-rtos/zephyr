/*
 * Copyright (c) 2020, Jiri Kerestes
 *
 * Based on wdt_mcux_wdog32.c, which is:
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_wwdt

#include <drivers/watchdog.h>
#include <fsl_wwdt.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wdt_mcux_wwdt);

#define MIN_TIMEOUT 0xFF

struct mcux_wwdt_config {
	WWDT_Type *base;
	uint8_t clk_divider;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_wwdt_data {
	wdt_callback_t callback;
	wwdt_config_t wwdt_config;
	bool timeout_valid;
};

static int mcux_wwdt_setup(const struct device *dev, uint8_t options)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	WWDT_Type *base = config->base;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	WWDT_Init(base, &data->wwdt_config);
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_wwdt_disable(const struct device *dev)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	WWDT_Type *base = config->base;

	WWDT_Deinit(base);
	data->timeout_valid = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

/*
 * LPC55xxx WWDT has a fixed divide-by-4 clock prescaler.
 * This prescaler is different from the clock divider specified in Device Tree.
 */
#define MSEC_TO_WWDT_TICKS(clock_freq, msec) \
	((uint32_t)(clock_freq * msec / MSEC_PER_SEC / 4))

static int mcux_wwdt_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	uint32_t clock_freq;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	CLOCK_SetClkDiv(kCLOCK_DivWdtClk, config->clk_divider, true);
	clock_freq = CLOCK_GetWdtClkFreq();

	WWDT_GetDefaultConfig(&data->wwdt_config);

	data->wwdt_config.clockFreq_Hz = clock_freq;

	data->wwdt_config.timeoutValue =
		MSEC_TO_WWDT_TICKS(clock_freq, cfg->window.max);

	if (cfg->window.min) {
		data->wwdt_config.windowValue =
			MSEC_TO_WWDT_TICKS(clock_freq, cfg->window.min);
	}

	if ((data->wwdt_config.timeoutValue < MIN_TIMEOUT) ||
	    (data->wwdt_config.timeoutValue > data->wwdt_config.windowValue)) {
		LOG_ERR("Invalid timeout");
		return -EINVAL;
	}

	if (cfg->flags & WDT_FLAG_RESET_SOC) {
		data->wwdt_config.enableWatchdogReset = true;
		LOG_DBG("Enabling SoC reset");
	}

	data->callback = cfg->callback;
	data->timeout_valid = true;
	LOG_DBG("Installed timeout (timeoutValue = %d)",
		data->wwdt_config.timeoutValue);

	return 0;
}

static int mcux_wwdt_feed(const struct device *dev, int channel_id)
{
	const struct mcux_wwdt_config *config = dev->config;
	WWDT_Type *base = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	WWDT_Refresh(base);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static void mcux_wwdt_isr(const struct device *dev)
{
	const struct mcux_wwdt_config *config = dev->config;
	struct mcux_wwdt_data *data = dev->data;
	WWDT_Type *base = config->base;
	uint32_t flags;

	flags = WWDT_GetStatusFlags(base);
	WWDT_ClearStatusFlags(base, flags);

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int mcux_wwdt_init(const struct device *dev)
{
	const struct mcux_wwdt_config *config = dev->config;

	config->irq_config_func(dev);

	return 0;
}

static const struct wdt_driver_api mcux_wwdt_api = {
	.setup = mcux_wwdt_setup,
	.disable = mcux_wwdt_disable,
	.install_timeout = mcux_wwdt_install_timeout,
	.feed = mcux_wwdt_feed,
};

static void mcux_wwdt_config_func_0(const struct device *dev);

static const struct mcux_wwdt_config mcux_wwdt_config_0 = {
	.base = (WWDT_Type *) DT_INST_REG_ADDR(0),
	.clk_divider =
		DT_INST_PROP(0, clk_divider),
	.irq_config_func = mcux_wwdt_config_func_0,
};

static struct mcux_wwdt_data mcux_wwdt_data_0;

DEVICE_DT_INST_DEFINE(0, &mcux_wwdt_init,
		    device_pm_control_nop, &mcux_wwdt_data_0,
		    &mcux_wwdt_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_wwdt_api);

static void mcux_wwdt_config_func_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    mcux_wwdt_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
