/*
 * Copyright (C) 2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_wdog

#include <drivers/watchdog.h>
#include <fsl_wdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wdt_mcux_wdog);

#define WDOG_TMOUT_SEC(x)  ((x * 2 / MSEC_PER_SEC) - 1)

struct mcux_wdog_config {
	WDOG_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_wdog_data {
	wdt_callback_t callback;
	wdog_config_t wdog_config;
	bool timeout_valid;
};

static int mcux_wdog_setup(const struct device *dev, uint8_t options)
{
	const struct mcux_wdog_config *config = dev->config;
	struct mcux_wdog_data *data = dev->data;
	WDOG_Type *base = config->base;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	data->wdog_config.workMode.enableStop =
		(options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;

	data->wdog_config.workMode.enableDebug =
		(options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;

	WDOG_Init(base, &data->wdog_config);
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_wdog_disable(const struct device *dev)
{
	const struct mcux_wdog_config *config = dev->config;
	struct mcux_wdog_data *data = dev->data;
	WDOG_Type *base = config->base;

	WDOG_Deinit(base);
	data->timeout_valid = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

static int mcux_wdog_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	struct mcux_wdog_data *data = dev->data;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	WDOG_GetDefaultConfig(&data->wdog_config);
	data->wdog_config.interruptTimeValue = 0U;

	data->wdog_config.timeoutValue =
		  WDOG_TMOUT_SEC(cfg->window.max);

	if (cfg->window.min) {
		LOG_ERR("Invalid window.min, Do not support window model");
		return -EINVAL;
	}
	if (data->wdog_config.timeoutValue > 128) {
		LOG_ERR("Invalid timeoutValue, valid (0.5s - 128.0s)");
		return -EINVAL;
	}

	data->wdog_config.enableInterrupt = cfg->callback != NULL;
	data->callback = cfg->callback;
	data->timeout_valid = true;

	return 0;
}

static int mcux_wdog_feed(const struct device *dev, int channel_id)
{
	const struct mcux_wdog_config *config = dev->config;
	WDOG_Type *base = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	WDOG_Refresh(base);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static void mcux_wdog_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct mcux_wdog_config *config = dev->config;
	struct mcux_wdog_data *data = dev->data;
	WDOG_Type *base = config->base;
	uint32_t flags;

	flags = WDOG_GetStatusFlags(base);
	WDOG_ClearInterruptStatus(base, flags);

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int mcux_wdog_init(const struct device *dev)
{
	const struct mcux_wdog_config *config = dev->config;

	config->irq_config_func(dev);

	return 0;
}

static const struct wdt_driver_api mcux_wdog_api = {
	.setup = mcux_wdog_setup,
	.disable = mcux_wdog_disable,
	.install_timeout = mcux_wdog_install_timeout,
	.feed = mcux_wdog_feed,
};

static void mcux_wdog_config_func(const struct device *dev);

static const struct mcux_wdog_config mcux_wdog_config = {
	.base = (WDOG_Type *) DT_INST_REG_ADDR(0),
	.irq_config_func = mcux_wdog_config_func,
};

static struct mcux_wdog_data mcux_wdog_data;

DEVICE_DT_INST_DEFINE(0,
		    &mcux_wdog_init,
		    device_pm_control_nop,
		    &mcux_wdog_data, &mcux_wdog_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_wdog_api);

static void mcux_wdog_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    mcux_wdog_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
