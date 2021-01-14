/* wdt_xec.c - Microchip XEC watchdog driver */

#define DT_DRV_COMPAT microchip_xec_watchdog

/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wdt_mchp_xec);

#include <drivers/watchdog.h>
#include <soc.h>
#include <errno.h>

#define WDT_XEC_REG_BASE						\
	((WDT_Type *)(DT_INST_REG_ADDR(0)))

struct wdt_xec_data {
	wdt_callback_t cb;
	bool timeout_installed;
};

static int wdt_xec_setup(const struct device *dev, uint8_t options)
{
	WDT_Type *wdt_regs = WDT_XEC_REG_BASE;
	struct wdt_xec_data *data = dev->data;

	if (wdt_regs->CTRL & MCHP_WDT_CTRL_EN) {
		return -EBUSY;
	}

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_WRN("WDT_OPT_PAUSE_IN_SLEEP is not supported");
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		wdt_regs->CTRL |= MCHP_WDT_CTRL_JTAG_STALL_EN;
	} else {
		wdt_regs->CTRL &= ~MCHP_WDT_CTRL_JTAG_STALL_EN;
	}

	wdt_regs->CTRL |= MCHP_WDT_CTRL_EN;

	LOG_DBG("WDT Setup and enabled");

	return 0;
}

static int wdt_xec_disable(const struct device *dev)
{
	WDT_Type *wdt_regs = WDT_XEC_REG_BASE;
	struct wdt_xec_data *data = dev->data;

	if (!(wdt_regs->CTRL & MCHP_WDT_CTRL_EN)) {
		return -EALREADY;
	}

	wdt_regs->CTRL &= ~MCHP_WDT_CTRL_EN;
	data->timeout_installed = false;

	LOG_DBG("WDT Disabled");

	return 0;
}

static int wdt_xec_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *config)
{
	WDT_Type *wdt_regs = WDT_XEC_REG_BASE;
	struct wdt_xec_data *data = dev->data;

	if (wdt_regs->CTRL & MCHP_WDT_CTRL_EN) {
		return -EBUSY;
	}

	if (config->window.min > 0U) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	wdt_regs->LOAD = 0;

	data->cb = config->callback;
	if (data->cb) {
		wdt_regs->CTRL |= MCHP_WDT_CTRL_MODE_IRQ;
		wdt_regs->IEN |= MCHP_WDT_IEN_EVENT_IRQ_EN;

		LOG_DBG("WDT callback enabled");
	} else {
		/* Setting WDT_FLAG_RESET_SOC or not will have no effect:
		 * even after the cb, if anything is done, SoC will reset
		 */
		wdt_regs->CTRL &= ~MCHP_WDT_CTRL_MODE_IRQ;
		wdt_regs->IEN &= ~MCHP_WDT_IEN_EVENT_IRQ_EN;

		LOG_DBG("WDT Reset enabled");
	}

	/* Since it almost takes 1ms to decrement the load register
	 * (See datasheet 18.6.1.4: 33/32.768 KHz = 1.007ms)
	 * Let's use the given window directly.
	 */
	wdt_regs->LOAD = config->window.max;

	data->timeout_installed = true;

	return 0;
}

static int wdt_xec_feed(const struct device *dev, int channel_id)
{
	WDT_Type *wdt_regs = WDT_XEC_REG_BASE;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	if (!(wdt_regs->CTRL & MCHP_WDT_CTRL_EN)) {
		return -EINVAL;
	}

	LOG_DBG("WDT Kicking");

	wdt_regs->KICK = 1;

	return 0;
}

static void wdt_xec_isr(const struct device *dev)
{
	WDT_Type *wdt_regs = WDT_XEC_REG_BASE;
	struct wdt_xec_data *data = dev->data;

	LOG_DBG("WDT ISR");

	if (data->cb) {
		data->cb(dev, 0);
	}

	MCHP_GIRQ_SRC(MCHP_WDT_GIRQ) = MCHP_WDT_GIRQ_VAL;
	wdt_regs->IEN &= ~MCHP_WDT_IEN_EVENT_IRQ_EN;
}

static const struct wdt_driver_api wdt_xec_api = {
	.setup = wdt_xec_setup,
	.disable = wdt_xec_disable,
	.install_timeout = wdt_xec_install_timeout,
	.feed = wdt_xec_feed,
};

static int wdt_xec_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_xec_disable(dev);
	}

	MCHP_GIRQ_ENSET(MCHP_WDT_GIRQ) = MCHP_WDT_GIRQ_VAL;

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    wdt_xec_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static struct wdt_xec_data wdt_xec_dev_data;

DEVICE_DT_INST_DEFINE(0, wdt_xec_init, device_pm_control_nop,
		    &wdt_xec_dev_data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_xec_api);
