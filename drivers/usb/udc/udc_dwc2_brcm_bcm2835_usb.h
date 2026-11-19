/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H

#include <zephyr/device.h>
#include <zephyr/pm/device_runtime.h>

static inline int brcm_bcm2835_usb_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.hs = true;

	return 0;
}

#define QUIRK_BRCM_BCM2835_USB_INST(n)							\
	static int brcm_bcm2835_usb_pre_enable_##n(const struct device *dev)		\
	{										\
		ARG_UNUSED(dev);							\
		return pm_device_runtime_get(						\
			DEVICE_DT_GET(DT_INST_PHANDLE(n, power_domains)));		\
	}										\
											\
	static int brcm_bcm2835_usb_disable_##n(const struct device *dev)		\
	{										\
		ARG_UNUSED(dev);							\
		return pm_device_runtime_put(						\
			DEVICE_DT_GET(DT_INST_PHANDLE(n, power_domains)));		\
	}										\
											\
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {			\
		.pre_enable = brcm_bcm2835_usb_pre_enable_##n,				\
		.disable = brcm_bcm2835_usb_disable_##n,				\
		.caps = brcm_bcm2835_usb_caps,						\
	};

#define QUIRK_BRCM_BCM2835_USB_DEFINE(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_COMPAT(n, brcm_bcm2835_usb),			\
		    (QUIRK_BRCM_BCM2835_USB_INST(n)), ())

DT_INST_FOREACH_STATUS_OKAY(QUIRK_BRCM_BCM2835_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H */
