/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Realtek Semiconductor Corporation, SIBG-SD7, Dylan Hsieh
 */

#define DT_DRV_COMPAT realtek_rts5912_watchdog

#include <soc.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_rts5912, CONFIG_WDT_LOG_LEVEL);

#include "reg/reg_wdt.h"

#define WDT_MAX_CNT   256
#define WDT_CLK_CYCLE 32768UL

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "multiple instances not supported");

struct wdt_rts5912_config {
	uint32_t base;
	uint32_t div;
	const struct device *clk_dev;
	uint32_t clk_grp;
	uint32_t clk_idx;
};

struct wdt_rts5912_data {
	wdt_callback_t callback;
	bool timeout_installed;
	uint32_t timeout;
};

static void wdt_rts5912_isr(const struct device *dev)
{
	const struct wdt_rts5912_config *const cfg = dev->config;
	struct wdt_rts5912_data *data = dev->data;
	WDT_Type *wdt_reg = (WDT_Type *)cfg->base;

	LOG_DBG("WDT ISR");

	wdt_reg->CTRL |= WDT_CTRL_CLRRSTFLAG;

	if (data->callback) {
		data->callback(dev, 0);
	}
}

static int wdt_rts5912_setup(const struct device *dev, uint8_t options)
{
	const struct wdt_rts5912_config *const config = dev->config;
	struct wdt_rts5912_data *data = dev->data;
	WDT_Type *wdt_reg = (WDT_Type *)config->base;

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	if (wdt_reg->CTRL & WDT_CTRL_EN) {
		LOG_ERR("WDT is already running");
		return -EBUSY;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		LOG_ERR("Pause when halted by debugger not supported");
		return -ENOTSUP;
	}

	irq_enable(DT_INST_IRQN(0));

	wdt_reg->INTEN = WDT_INTEN_WDTINTEN;
	wdt_reg->CTRL |= (WDT_CTRL_CLRRSTFLAG | WDT_CTRL_RELOAD);
	wdt_reg->CTRL |= WDT_CTRL_EN;

	LOG_DBG("WDT setup and enabled");

	return 0;
}

static int wdt_rts5912_disable(const struct device *dev)
{
	const struct wdt_rts5912_config *const config = dev->config;
	struct wdt_rts5912_data *data = dev->data;
	WDT_Type *wdt_reg = (WDT_Type *)config->base;

	if (!(wdt_reg->CTRL & WDT_CTRL_EN)) {
		return -EALREADY;
	}

	wdt_reg->INTEN = 0ul;
	wdt_reg->CTRL |= WDT_CTRL_CLRRSTFLAG;
	wdt_reg->CTRL &= ~WDT_CTRL_EN;

	data->timeout_installed = false;

	LOG_DBG("WDT disabled");

	return 0;
}

static int wdt_rts5912_install_timeout(const struct device *dev,
				       const struct wdt_timeout_cfg *config)
{
	const struct wdt_rts5912_config *const cfg = dev->config;
	struct wdt_rts5912_data *data = dev->data;
	WDT_Type *wdt_reg = (WDT_Type *)cfg->base;

	uint32_t timeout;
	uint32_t max, min;

	LOG_DBG("WDT intstall timeout");

	if (wdt_reg->CTRL & WDT_CTRL_EN) {
		LOG_ERR("WDT is already running");
		return -EBUSY;
	}

	if (config->window.min > 0U) {
		LOG_ERR("Lower limit of watchdog is not supported, keep it zero");
		data->timeout_installed = false;
		return -EINVAL;
	}

	switch (config->flags) {
	case WDT_FLAG_RESET_SOC:
		wdt_reg->CTRL |= WDT_CTRL_RSTEN;
		break;
	case WDT_FLAG_RESET_NONE:
		wdt_reg->CTRL &= ~WDT_CTRL_RSTEN;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		LOG_ERR("WDT_FLAG_RESET_CPU_CORE is not supported\n");
		break;
	default:
		LOG_ERR("Unsupported watchdog config Flag\n");
		return -ENOTSUP;
	}

	timeout = config->window.max * 1000U;
	min = (cfg->div * USEC_PER_SEC) / WDT_CLK_CYCLE;
	max = min * WDT_MAX_CNT;

	if ((timeout < min) || (timeout > max)) {
		LOG_ERR("Invalid timeout value allowed range:"
			"%d ms to %d ms",
			min / MSEC_PER_SEC, max / MSEC_PER_SEC);
		return -EINVAL;
	}

	wdt_reg->CNT = timeout / min;

	data->callback = config->callback;
	data->timeout_installed = true;

	LOG_DBG("DIV: 0x%08x, CNT: 0x%08x", wdt_reg->DIV, wdt_reg->CNT);

	return 0;
}

static int wdt_rts5912_feed(const struct device *dev, int channel_id)
{
	const struct wdt_rts5912_config *const cfg = dev->config;
	WDT_Type *wdt_reg = (WDT_Type *)cfg->base;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	if (!(wdt_reg->CTRL & WDT_CTRL_EN)) {
		return -EINVAL;
	}

	wdt_reg->CTRL |= WDT_CTRL_RELOAD;

	LOG_DBG("WDT feed");

	return 0;
}

static DEVICE_API(wdt, wdt_rts5912_api) = {
	.setup = wdt_rts5912_setup,
	.disable = wdt_rts5912_disable,
	.install_timeout = wdt_rts5912_install_timeout,
	.feed = wdt_rts5912_feed,
};

static int wdt_rts5912_init(const struct device *dev)
{
	int rc;

	const struct wdt_rts5912_config *const cfg = dev->config;
	struct rts5912_sccon_subsys sccon;
	WDT_Type *wdt_reg = (WDT_Type *)cfg->base;

	LOG_DBG("WDT init");

	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	sccon.clk_grp = cfg->clk_grp;
	sccon.clk_idx = cfg->clk_idx;
	rc = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		return rc;
	}

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_rts5912_disable(dev);
	}

	wdt_reg->CTRL = 0ul;
	wdt_reg->DIV = cfg->div;

	wdt_reg->CTRL |= WDT_CTRL_CLRRSTFLAG;
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));

	IRQ_CONNECT(DT_INST_IRQN(0), 0, wdt_rts5912_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static const struct wdt_rts5912_config wdt_rts5912_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.div = DT_INST_PROP(0, clk_divider),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(wdog), watchdog, clk_grp),
	.clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(wdog), watchdog, clk_idx),
};

static struct wdt_rts5912_data wdt_rts5912_dev_data;

DEVICE_DT_INST_DEFINE(0, wdt_rts5912_init, NULL, &wdt_rts5912_dev_data, &wdt_rts5912_cfg,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &wdt_rts5912_api);
