/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H

#include <zephyr/pm/device_runtime.h>

struct brcm_bcm2835_usb_power {
	const struct device *dev;
	bool active;
};

static inline int brcm_bcm2835_usb_pre_enable(struct brcm_bcm2835_usb_power *power)
{
	int ret;

	if (!device_is_ready(power->dev)) {
		return -ENODEV;
	}

	if (power->active) {
		return 0;
	}

	ret = pm_device_runtime_get(power->dev);
	if (ret == 0) {
		power->active = true;
	}

	return ret;
}

static inline int brcm_bcm2835_usb_disable(struct brcm_bcm2835_usb_power *power)
{
	int ret;

	/* Shutdown can follow disable or a failed controller initialization. */
	if (!power->active) {
		return 0;
	}

	ret = pm_device_runtime_put(power->dev);
	if (ret == 0) {
		power->active = false;
	}

	return ret;
}

static inline int brcm_bcm2835_usb_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	/* GHWCFG2.HSPHYTYPE reports a UTMI+ high-speed PHY. */
	data->caps.hs = true;

	return 0;
}

#define BRCM_BCM2835_USB_DEFINE(n)						\
	static struct brcm_bcm2835_usb_power brcm_bcm2835_usb_power_##n = {	\
		.dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, power_domains)),		\
	};									\
										\
	static int brcm_bcm2835_usb_pre_enable_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);							\
		return brcm_bcm2835_usb_pre_enable(&brcm_bcm2835_usb_power_##n);	\
	}									\
										\
	static int brcm_bcm2835_usb_disable_##n(const struct device *dev)	\
	{									\
		ARG_UNUSED(dev);							\
		return brcm_bcm2835_usb_disable(&brcm_bcm2835_usb_power_##n);	\
	}									\
										\
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {		\
		.caps = brcm_bcm2835_usb_caps,					\
		.pre_enable = brcm_bcm2835_usb_pre_enable_##n,			\
		.disable = brcm_bcm2835_usb_disable_##n,				\
		.shutdown = brcm_bcm2835_usb_disable_##n,				\
	};

DT_INST_FOREACH_STATUS_OKAY(BRCM_BCM2835_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H */
