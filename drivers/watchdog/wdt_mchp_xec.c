/* wdt_xec.c - Microchip XEC watchdog driver */

#define DT_DRV_COMPAT microchip_xec_watchdog

/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_mchp_xec);

#include <zephyr/drivers/watchdog.h>
#include <zephyr/pm/device.h>
#include <soc.h>
#include <errno.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "add exactly one wdog node to the devicetree");

struct wdt_xec_config {
	struct wdt_regs *regs;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t enc_pcr;
};

struct wdt_xec_data {
	wdt_callback_t cb;
	bool timeout_installed;
	bool pause_in_sleep;
};

static int wdt_xec_setup(const struct device *dev, uint8_t options)
{
	struct wdt_xec_config const *cfg = dev->config;
	struct wdt_xec_data *data = dev->data;
	struct wdt_regs *regs = cfg->regs;

	if (regs->CTRL & MCHP_WDT_CTRL_EN) {
		return -EBUSY;
	}

	if (!data->timeout_installed) {
		LOG_ERR("No valid WDT timeout installed");
		return -EINVAL;
	}

	data->pause_in_sleep = (options & WDT_OPT_PAUSE_IN_SLEEP) != 0;

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		regs->CTRL |= MCHP_WDT_CTRL_JTAG_STALL_EN;
	} else {
		regs->CTRL &= ~MCHP_WDT_CTRL_JTAG_STALL_EN;
	}

	regs->CTRL |= MCHP_WDT_CTRL_EN;

	LOG_DBG("WDT Setup and enabled");

	return 0;
}

static int wdt_xec_disable(const struct device *dev)
{
	struct wdt_xec_config const *cfg = dev->config;
	struct wdt_xec_data *data = dev->data;
	struct wdt_regs *regs = cfg->regs;

	if (!(regs->CTRL & MCHP_WDT_CTRL_EN)) {
		return -EALREADY;
	}

	regs->CTRL &= ~MCHP_WDT_CTRL_EN;
	data->timeout_installed = false;
	data->pause_in_sleep = false;

	LOG_DBG("WDT Disabled");

	return 0;
}

static int wdt_xec_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *config)
{
	struct wdt_xec_config const *cfg = dev->config;
	struct wdt_xec_data *data = dev->data;
	struct wdt_regs *regs = cfg->regs;

	if (regs->CTRL & MCHP_WDT_CTRL_EN) {
		return -EBUSY;
	}

	if (config->window.min > 0U) {
		data->timeout_installed = false;
		return -EINVAL;
	}

	regs->LOAD = 0;

	data->cb = config->callback;
	if (data->cb) {
		regs->CTRL |= MCHP_WDT_CTRL_MODE_IRQ;
		regs->IEN |= MCHP_WDT_IEN_EVENT_IRQ_EN;

		LOG_DBG("WDT callback enabled");
	} else {
		/* Setting WDT_FLAG_RESET_SOC or not will have no effect:
		 * even after the cb, if anything is done, SoC will reset
		 */
		regs->CTRL &= ~MCHP_WDT_CTRL_MODE_IRQ;
		regs->IEN &= ~MCHP_WDT_IEN_EVENT_IRQ_EN;

		LOG_DBG("WDT Reset enabled");
	}

	/* Since it almost takes 1ms to decrement the load register
	 * (See datasheet 18.6.1.4: 33/32.768 KHz = 1.007ms)
	 * Let's use the given window directly.
	 */
	regs->LOAD = config->window.max;

	data->timeout_installed = true;

	return 0;
}

static int wdt_xec_feed(const struct device *dev, int channel_id)
{
	struct wdt_xec_config const *cfg = dev->config;
	struct wdt_regs *regs = cfg->regs;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	if (!(regs->CTRL & MCHP_WDT_CTRL_EN)) {
		return -EINVAL;
	}

	LOG_DBG("WDT Kicking");

	regs->KICK = 1;

	return 0;
}

static void wdt_xec_isr(const struct device *dev)
{
	struct wdt_xec_config const *cfg = dev->config;
	struct wdt_xec_data *data = dev->data;
	struct wdt_regs *regs = cfg->regs;

	LOG_DBG("WDT ISR");

	if (data->cb) {
		data->cb(dev, 0);
	}

	soc_ecia_girq_status_clear(cfg->girq, cfg->girq_pos);

	regs->IEN &= ~MCHP_WDT_IEN_EVENT_IRQ_EN;
}

#ifdef CONFIG_PM_DEVICE
static int wdt_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct wdt_xec_config const *cfg = dev->config;
	struct wdt_xec_data *data = dev->data;
	struct wdt_regs *regs = cfg->regs;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (data->pause_in_sleep && (regs->CTRL & MCHP_WDT_CTRL_EN)) {
			regs->CTRL &= ~MCHP_WDT_CTRL_EN;
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (data->pause_in_sleep && data->timeout_installed) {
			regs->KICK = 1;
			regs->CTRL |= MCHP_WDT_CTRL_EN;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(wdt, wdt_xec_api) = {
	.setup = wdt_xec_setup,
	.disable = wdt_xec_disable,
	.install_timeout = wdt_xec_install_timeout,
	.feed = wdt_xec_feed,
};

static int wdt_xec_init(const struct device *dev)
{
	struct wdt_xec_config const *cfg = dev->config;

	soc_xec_pcr_sleep_en_clear(cfg->enc_pcr);

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		wdt_xec_disable(dev);
	}

	soc_ecia_girq_ctrl(cfg->girq, cfg->girq_pos, 1);

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    wdt_xec_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

#define DEV_CFG_GIRQ(inst, idx)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, idx))
#define DEV_CFG_GIRQ_POS(inst, idx) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, idx))

static const struct wdt_xec_config wdt_xec_config_0 = {
	.regs = (struct wdt_regs *)(DT_INST_REG_ADDR(0)),
	.girq = DEV_CFG_GIRQ(0, 0),
	.girq_pos = DEV_CFG_GIRQ_POS(0, 0),
	.enc_pcr = DT_INST_PROP(0, pcr_scr),
};

static struct wdt_xec_data wdt_xec_dev_data;

#ifdef CONFIG_PM_DEVICE
PM_DEVICE_DT_INST_DEFINE(0, wdt_xec_pm_action);
#endif

DEVICE_DT_INST_DEFINE(0, wdt_xec_init, PM_DEVICE_DT_INST_GET(0),
		    &wdt_xec_dev_data, &wdt_xec_config_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdt_xec_api);
