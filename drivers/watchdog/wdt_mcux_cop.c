/*
 * Copyright (c) 2025, Swiss Timing Ltd.
 *
 * Based on wdt_mcux_rtwdog.c, which is:
 * Copyright 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_cop

#include <zephyr/drivers/watchdog.h>
#include <fsl_cop.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_mcux_cop);

struct mcux_cop_config {
	SIM_Type *base;
	cop_clock_source_t clk_source;
	uint64_t timeout_cycles;
	bool windowed_mode;
};

struct mcux_cop_data {
	cop_config_t cop_config;
	bool timeout_valid;
	bool enabled;
};

static int mcux_cop_setup(const struct device *dev, uint8_t options)
{
	const struct mcux_cop_config *config = dev->config;
	struct mcux_cop_data *data = dev->data;
	SIM_Type *base = config->base;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	if (data->enabled) {
		LOG_ERR("This watchdog has been enabled");
		return -EBUSY;
	}

#if defined(FSL_FEATURE_COP_HAS_STOP_ENABLE) && FSL_FEATURE_COP_HAS_STOP_ENABLE
	data->cop_config.enableStop = (options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;
#else
	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0U) {
		LOG_ERR("Not support WDT_OPT_PAUSE_IN_SLEEP");
		return -ENOTSUP;
	}
#endif

#if defined(FSL_FEATURE_COP_HAS_DEBUG_ENABLE) && FSL_FEATURE_COP_HAS_DEBUG_ENABLE
	data->cop_config.enableDebug = (options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;
#else
	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) != 0U) {
		LOG_ERR("Not support WDT_OPT_PAUSE_HALTED_BY_DBG");
		return -ENOTSUP;
	}
#endif

	COP_Init(base, &data->cop_config);
	data->enabled = true;
	LOG_DBG("Setup the watchdog");

	return 0;
}

static int mcux_cop_disable(const struct device *dev)
{
	const struct mcux_cop_config *config = dev->config;
	struct mcux_cop_data *data = dev->data;

	data->timeout_valid = false;

	if (!data->enabled) {
		LOG_ERR("Disabled when watchdog is not enabled");
		return -EFAULT;
	}

	COP_Disable(config->base);
	data->enabled = false;
	LOG_DBG("Disabled the watchdog");

	return 0;
}

static int mcux_cop_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct mcux_cop_config *config = dev->config;
	struct mcux_cop_data *data = dev->data;

	if (cfg != NULL) {
		LOG_ERR("Watchdog only configurable via Device Tree, cfg parameter must be NULL");
		return -ENOTSUP;
	}

	if (data->enabled) {
		LOG_ERR("Timeout can not be installed while watchdog has already been setup");
		return -EBUSY;
	}

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	COP_GetDefaultConfig(&data->cop_config);

	switch (config->timeout_cycles) {
	case 32:
		data->cop_config.timeoutCycles = kCOP_2Power5CyclesOr2Power13Cycles;
		break;
	case 256:
		data->cop_config.timeoutCycles = kCOP_2Power8CyclesOr2Power16Cycles;
		break;
	case 1024:
		data->cop_config.timeoutCycles = kCOP_2Power10CyclesOr2Power18Cycles;
		break;
	case 8192:
		data->cop_config.timeoutCycles = kCOP_2Power5CyclesOr2Power13Cycles;
		break;
	case 65526:
		data->cop_config.timeoutCycles = kCOP_2Power8CyclesOr2Power16Cycles;
		break;
	case 262144:
		data->cop_config.timeoutCycles = kCOP_2Power10CyclesOr2Power18Cycles;
		break;
	}

#if defined(FSL_FEATURE_COP_HAS_LONGTIME_MODE) && FSL_FEATURE_COP_HAS_LONGTIME_MODE
	data->cop_config.timeoutMode =
		(config->timeout_cycles < 8192) ? kCOP_ShortTimeoutMode : kCOP_LongTimeoutMode;

	if (config->windowed_mode) {
		if (data->cop_config.timeoutMode == kCOP_LongTimeoutMode) {
			data->cop_config.enableWindowMode = true;
		} else {
			LOG_ERR("Windowed mode not supported in short timeout mode");
			return -ENOTSUP;
		}
	}
#endif

	data->cop_config.clockSource = config->clk_source;
	data->timeout_valid = true;

	return 0;
}

static int mcux_cop_feed(const struct device *dev, int channel_id)
{
	const struct mcux_cop_config *config = dev->config;
	struct mcux_cop_data *data = dev->data;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (!data->enabled) {
		LOG_ERR("Feed disabled watchdog");
		return -EINVAL;
	}

	COP_Refresh(config->base);
	LOG_DBG("Fed the watchdog");

	return 0;
}

static DEVICE_API(wdt, mcux_cop_api) = {
	.setup = mcux_cop_setup,
	.disable = mcux_cop_disable,
	.install_timeout = mcux_cop_install_timeout,
	.feed = mcux_cop_feed,
};

static const struct mcux_cop_config mcux_cop_config = {
	.base = (SIM_Type *)DT_INST_REG_ADDR(0),
	.clk_source = DT_INST_PROP(0, clk_source),
	.timeout_cycles = DT_INST_PROP(0, timeout_cycles),
	.windowed_mode = DT_INST_PROP(0, windowed_mode),
};

static struct mcux_cop_data mcux_cop_data;

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &mcux_cop_data, &mcux_cop_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_cop_api);
