/*
 * Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat1_watchdog

#include "cyhal_wdt.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_infineon_cat1, CONFIG_WDT_LOG_LEVEL);

#define IFX_CAT1_WDT_IS_IRQ_EN DT_NODE_HAS_PROP(DT_DRV_INST(0), interrupts)

struct ifx_cat1_wdt_data {
	cyhal_wdt_t obj;
#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	wdt_callback_t callback;
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */
	uint32_t timeout;
	bool timeout_installed;
};

struct ifx_cat1_wdt_data wdt_data;

#ifdef IFX_CAT1_WDT_IS_IRQ_EN
static void ifx_cat1_wdt_isr_handler(const struct device *dev)
{
	struct ifx_cat1_wdt_data *dev_data = dev->data;

	if (dev_data->callback) {
		dev_data->callback(dev, 0);
	}
	Cy_WDT_MaskInterrupt();
}
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

static int ifx_cat1_wdt_setup(const struct device *dev, uint8_t options)
{
	cy_rslt_t result;
	struct ifx_cat1_wdt_data *dev_data = dev->data;

	/* Initialize the WDT */
	result = cyhal_wdt_init(&dev_data->obj, dev_data->timeout);
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("Initialization failure : 0x%x", result);
		return -ENOMSG;
	}

#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	if (dev_data->callback) {
		Cy_WDT_UnmaskInterrupt();
		irq_enable(DT_INST_IRQN(0));
	}
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

	return 0;
}

static int ifx_cat1_wdt_disable(const struct device *dev)
{
	struct ifx_cat1_wdt_data *dev_data = dev->data;

#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	Cy_WDT_MaskInterrupt();
	irq_disable(DT_INST_IRQN(0));
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

	cyhal_wdt_free(&dev_data->obj);

	return 0;
}

static int ifx_cat1_wdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct ifx_cat1_wdt_data *dev_data = dev->data;

	if (dev_data->timeout_installed) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if (cfg->flags) {
		LOG_WRN("Watchdog behavior is not configurable.");
	}

	if (cfg->callback) {
#ifndef IFX_CAT1_WDT_IS_IRQ_EN
		LOG_WRN("Interrupt is not configured, can't set a callback.");
#else
		dev_data->callback = cfg->callback;
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */
	}

	/* window watchdog not supported */
	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	dev_data->timeout = cfg->window.max;

	return 0;
}

static int ifx_cat1_wdt_feed(const struct device *dev, int channel_id)
{
	struct ifx_cat1_wdt_data *data = dev->data;

	/* Only channel 0 is supported */
	if (channel_id) {
		return -EINVAL;
	}

	cyhal_wdt_kick(&data->obj);

	return 0;
}

static int ifx_cat1_wdt_init(const struct device *dev)
{
#ifdef IFX_CAT1_WDT_IS_IRQ_EN
	/* Connect WDT interrupt to ISR */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), ifx_cat1_wdt_isr_handler,
		    DEVICE_DT_INST_GET(0), 0);
#endif /* IFX_CAT1_WDT_IS_IRQ_EN */

	return 0;
}

static DEVICE_API(wdt, ifx_cat1_wdt_api) = {
	.setup = ifx_cat1_wdt_setup,
	.disable = ifx_cat1_wdt_disable,
	.install_timeout = ifx_cat1_wdt_install_timeout,
	.feed = ifx_cat1_wdt_feed,
};

DEVICE_DT_INST_DEFINE(0, ifx_cat1_wdt_init, NULL, &wdt_data, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ifx_cat1_wdt_api);
