/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H

#if defined(CONFIG_BCM2835_FIRMWARE)
#include <zephyr/drivers/firmware/bcm2835.h>
#endif

/* BCM283x DWC2 vendor quirks: power on the analog PHY island via
 * the VideoCore firmware mailbox, and declare HS capability (the
 * generic driver does not propagate GHWCFG2.HSPhyType).
 */

static inline int brcm_bcm2835_usb_pre_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_BCM2835_FIRMWARE)
	return bcm2835_property_set_power_state(BCM2835_POWER_DEVICE_USB_HCD, true);
#else
	return -ENODEV;
#endif
}

static inline int brcm_bcm2835_usb_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.hs = true;

	return 0;
}

#define QUIRK_BRCM_BCM2835_USB_DEFINE(n)                                                           \
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {                                 \
		.pre_enable = brcm_bcm2835_usb_pre_enable,                                         \
		.caps = brcm_bcm2835_usb_caps,                                                     \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_BRCM_BCM2835_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H */
