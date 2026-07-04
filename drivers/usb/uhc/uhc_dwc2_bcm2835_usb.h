/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_BRCM_BCM2835_USB_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_BRCM_BCM2835_USB_H

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include "uhc_common.h"

struct brcm_bcm2835_usb_config {
	uint32_t cfg;
};

struct brcm_bcm2835_usb_data {
	uint32_t data;
};

static inline int brcm_bcm2835_dma_addr_xlate(const struct device *dev)
{
	struct uhc_dwc2_data *const priv = uhc_get_private(dev);

	if (IS_ENABLED(CONFIG_SOC_BCM2711)) {
		priv->dma_addr = BCM2711_CPU_TO_VC_ADDR(priv->dma_addr);
	}

	return 0;
}

static inline int brcm_bcm2835_usb_pre_enable(const struct device *dev)
{
	return pm_device_runtime_get(dev);
}

static inline int brcm_bcm2835_usb_shutdown(const struct device *dev)
{
	return pm_device_runtime_put(dev);
}

#define QUIRK_BRCM_BCM2835_USB_DEFINE(n)                                                           \
	static const struct brcm_bcm2835_usb_config uhc_dwc2_quirk_config_##n;                     \
                                                                                                   \
	static struct brcm_bcm2835_usb_data uhc_dwc2_quirk_data_##n;                               \
                                                                                                   \
	static const struct device *const brcm_power_dev_##n =                                     \
		DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), power_domains));                          \
                                                                                                   \
	static int brcm_bcm2835_usb_pre_enable_##n(const struct device *dev)                       \
	{                                                                                          \
		return brcm_bcm2835_usb_pre_enable(brcm_power_dev_##n);                            \
	}                                                                                          \
                                                                                                   \
	static int brcm_bcm2835_usb_shutdown_##n(const struct device *dev)                         \
	{                                                                                          \
		return brcm_bcm2835_usb_shutdown(brcm_power_dev_##n);                              \
	}                                                                                          \
                                                                                                   \
	const struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {                         \
		.pre_enable = brcm_bcm2835_usb_pre_enable_##n,                                     \
		.shutdown = brcm_bcm2835_usb_shutdown_##n,                                         \
		.dma_addr_xlate = brcm_bcm2835_dma_addr_xlate,                                     \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_BRCM_BCM2835_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_BRCM_BCM2835_USB_H */
