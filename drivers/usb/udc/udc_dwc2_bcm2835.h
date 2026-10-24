/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_BCM2835_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_BCM2835_H

static inline int bcm2835_usb_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	/* GHWCFG2.HSPHYTYPE reports a UTMI+ high-speed PHY. */
	data->caps.hs = true;

	return 0;
}

#define QUIRK_BCM2835_USB_DEFINE(n)                                                                \
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {                                 \
		.caps = bcm2835_usb_caps,                                                          \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_BCM2835_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_BCM2835_H */
