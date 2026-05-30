/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H

#if defined(CONFIG_RASPBERRYPI_FIRMWARE)
#include <rpi_fw.h>
#endif

/* BCM283x DWC2 vendor quirks: power on the analog PHY island via
 * the VideoCore firmware mailbox, and declare HS capability (the
 * generic driver does not propagate GHWCFG2.HSPhyType).
 */

static inline int brcm_bcm2835_usb_pre_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_RASPBERRYPI_FIRMWARE)
	/* VC mailbox power-device IDs and state bits per
	 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
	 *   device 3 = USB HCD; state bit0=on, bit1=wait-for-stable.
	 * Bit 1 left OFF: the wait-for-stable path can take longer than
	 * rpi_fw's 100 ms sem timeout. Matches the proven pizza-branch
	 * behaviour from v0.3.
	 */
	const struct device *fw = DEVICE_DT_GET_ONE(raspberrypi_bcm283x_firmware);
	uint32_t pw[2] = {3U, 1U};

	if (!device_is_ready(fw)) {
		return -ENODEV;
	}
	return rpi_fw_transfer(fw, RPI_FW_TAG_SET_POWER_STATE, pw, sizeof(pw));
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
