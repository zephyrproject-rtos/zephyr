/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_BRCM_BCM2835_USB_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_BRCM_BCM2835_USB_H

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define BCM2835_DMA_CHILD_BUS DT_DMA_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_PATH(soc), 0)

struct brcm_bcm2835_usb_config {
	const struct device *power_dev;
};

struct brcm_bcm2835_usb_data {
	uint32_t data;
};

static inline int brcm_bcm2835_dma_addr_translation(const struct device *dev, mem_addr_t *dma_addr)
{
	ARG_UNUSED(dev);

	*dma_addr |= BCM2835_DMA_CHILD_BUS;

	return 0;
}

static inline int brcm_bcm2835_usb_pre_enable(const struct device *dev)
{
	const struct brcm_bcm2835_usb_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	return pm_device_runtime_get(cfg->power_dev);
}

static inline int brcm_bcm2835_usb_disable(const struct device *dev)
{
	const struct brcm_bcm2835_usb_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	return pm_device_runtime_put(cfg->power_dev);
}

static inline int brcm_bcm2835_usb_shutdown(const struct device *dev)
{
	const struct brcm_bcm2835_usb_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	return pm_device_runtime_put(cfg->power_dev);
}

#define QUIRK_BRCM_BCM2835_USB_DEFINE(n)						\
											\
	static const struct brcm_bcm2835_usb_config uhc_dwc2_quirk_config_##n = {	\
		.power_dev = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), power_domains)),	\
	};										\
											\
	static struct brcm_bcm2835_usb_data uhc_dwc2_quirk_data_##n;			\
											\
	const struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {		\
		.pre_enable = brcm_bcm2835_usb_pre_enable,				\
		.disable = brcm_bcm2835_usb_disable,					\
		.shutdown = brcm_bcm2835_usb_shutdown,					\
		.dma_addr_translation = brcm_bcm2835_dma_addr_translation,		\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_BRCM_BCM2835_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_BRCM_BCM2835_USB_H */
