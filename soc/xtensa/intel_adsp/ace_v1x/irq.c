/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/interrupt_controller/dw_ace_v1x.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ace_v1x_soc, CONFIG_SOC_LOG_LEVEL);

void z_soc_irq_enable(uint32_t irq)
{
	const struct device *dev;
	const struct dw_ace_v1_ictl_driver_api *api;

	dev = device_get_binding(DT_LABEL(DT_NODELABEL(ace_intc)));

	if (!dev) {
		LOG_DBG("board: ACE V1X device binding failed");
		return;
	}

	api = (const struct dw_ace_v1_ictl_driver_api *)dev->api;
	api->intr_enable(dev, irq);
}

void z_soc_irq_disable(uint32_t irq)
{
	const struct device *dev;
	const struct dw_ace_v1_ictl_driver_api *api;

	dev = device_get_binding(DT_LABEL(DT_NODELABEL(ace_intc)));

	if (!dev) {
		LOG_DBG("board: ACE V1X device binding failed");
		return;
	}

	api = (const struct dw_ace_v1_ictl_driver_api *)dev->api;
	api->intr_disable(dev, irq);
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	const struct device *dev;
	const struct dw_ace_v1_ictl_driver_api *api;

	dev = device_get_binding(DT_LABEL(DT_NODELABEL(ace_intc)));

	if (!dev) {
		LOG_DBG("board: ACE V1X device binding failed");
		return -ENODEV;
	}

	api = (const struct dw_ace_v1_ictl_driver_api *)dev->api;
	return api->intr_is_enabled(dev, irq);
}
