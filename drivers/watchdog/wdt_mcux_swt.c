/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PERI_SWT.h"
#define DT_DRV_COMPAT nxp_swt

#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <fsl_clock.h>
#include <fsl_swt.h>

LOG_MODULE_REGISTER(wdt_mcux_swt, CONFIG_WDT_LOG_LEVEL);

#define MIN_TIMEOUT 3U

struct mcux_swt_config {
	SWT_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_swt_data {
	DEVICE_MMIO_NAMED_RAM(reg);
	wdt_callback_t callback;
	swt_config_t swt_config;
	bool timeout_valid;
};

static int mcux_swt_setup(const struct device *dev, uint8_t options)
{
	const struct mcux_swt_config *config = dev->config;
	struct mcux_swt_data *data = dev->data;
	SWT_Type *base = config->base;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	data->swt_config.enableRunInStop = (options & WDT_OPT_PAUSE_IN_SLEEP) == 0U;
	data->swt_config.enableRunInDebug = (options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0U;
	data->swt_config.interruptThenReset = (data->callback != NULL);
	data->swt_config.enableSwt = true;

	SWT_Init(base, &data->swt_config);
	LOG_DBG("SWT configured");

	return 0;
}

static int mcux_swt_disable(const struct device *dev)
{
	const struct mcux_swt_config *config = dev->config;
	struct mcux_swt_data *data = dev->data;
	SWT_Type *base = config->base;

	SWT_Deinit(base);
	data->timeout_valid = false;
	LOG_DBG("SWT disabled");

	return 0;
}

static int mcux_swt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct mcux_swt_data *data = dev->data;

	uint32_t clock_freq;
	uint64_t timeout_cycles;
	uint64_t min_cycles;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if (cfg->window.max == 0U || cfg->window.min > cfg->window.max) {
		LOG_ERR("Invalid timeout window");
		return -EINVAL;
	}

	/* Note: SWT on MCXE31x use SIRC as the only clock source */
	clock_freq = CLOCK_GetFreq(kCLOCK_SircClk);

	swt_config_t *swt_cfg = &data->swt_config;

	SWT_GetDefaultConfig(swt_cfg);

	timeout_cycles = (uint64_t)clock_freq * cfg->window.max / 1000U;
	if (timeout_cycles < MIN_TIMEOUT || timeout_cycles > UINT32_MAX) {
		LOG_ERR("Timeout out of range");
		return -EINVAL;
	}

	swt_cfg->timeoutValue = (uint32_t)timeout_cycles;

	if (cfg->window.min > 0U) {
		min_cycles = (uint64_t)clock_freq * cfg->window.min / 1000U;
		if (min_cycles >= timeout_cycles) {
			LOG_ERR("Window start exceeds timeout");
			return -EINVAL;
		}
		swt_cfg->enableWindowMode = true;
		swt_cfg->windowValue = (uint32_t)min_cycles;
	} else {
		swt_cfg->enableWindowMode = false;
		swt_cfg->windowValue = 0U;
	}

	data->callback = cfg->callback;
	data->timeout_valid = true;
	LOG_DBG("Timeout installed (timeout=%" PRIu32 ", window=%" PRIu32 ")",
		swt_cfg->timeoutValue, swt_cfg->windowValue);

	return 0;
}

static int mcux_swt_feed(const struct device *dev, int channel_id)
{
	const struct mcux_swt_config *config = dev->config;
	SWT_Type *base = config->base;

	if (channel_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	SWT_Refresh(base);
	LOG_DBG("SWT fed");

	return 0;
}

static void mcux_swt_isr(const struct device *dev)
{
	const struct mcux_swt_config *config = dev->config;
	struct mcux_swt_data *data = dev->data;
	SWT_Type *base = config->base;

	SWT_ClearTimeoutInterruptFlag(base);
	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}

static int mcux_swt_init(const struct device *dev)
{
	const struct mcux_swt_config *config = dev->config;

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, mcux_swt_api) = {
	.setup = mcux_swt_setup,
	.disable = mcux_swt_disable,
	.install_timeout = mcux_swt_install_timeout,
	.feed = mcux_swt_feed,
};

static void mcux_swt_config_func_0(const struct device *dev);

static const struct mcux_swt_config mcux_swt_config_0 = {
	.base = (SWT_Type *)DT_INST_REG_ADDR(0),
	.irq_config_func = mcux_swt_config_func_0,
};

static struct mcux_swt_data mcux_swt_data_0;

DEVICE_DT_INST_DEFINE(0, &mcux_swt_init, NULL, &mcux_swt_data_0, &mcux_swt_config_0, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_swt_api);

static void mcux_swt_config_func_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mcux_swt_isr, DEVICE_DT_INST_GET(0),
		    0);

	irq_enable(DT_INST_IRQN(0));
}
