/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H

#include <rpi_fw.h>

/* BCM283x DWC2 vendor quirks: power on the analog PHY island via
 * the VideoCore firmware mailbox, and declare HS capability (the
 * generic driver does not propagate GHWCFG2.HSPhyType).
 */

/* VideoCore power-domain id for the USB HCD (mailbox SET_POWER_STATE). */
#define BRCM_BCM2835_USB_POWER_DOMAIN 3U

static inline int brcm_bcm2835_usb_pre_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct device *fw = DEVICE_DT_GET_ANY(raspberrypi_bcm283x_firmware);
	/* Payload: { power-domain id, state }; bit 0 = on. */
	uint32_t power[2] = { BRCM_BCM2835_USB_POWER_DOMAIN, 1U };

	if (fw == NULL || !device_is_ready(fw)) {
		return -ENODEV;
	}

	return rpi_fw_transfer(fw, RPI_FW_TAG_SET_POWER_STATE, power, sizeof(power));
}

static inline int brcm_bcm2835_usb_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.hs = true;

	return 0;
}

/* The dwc2 controller sits behind the VideoCore bus, so its DMA master is
 * programmed with GPU BUS addresses, not ARM physical addresses. Per the
 * BCM2710 dma-ranges (<0xc0000000 0x00000000 0x3f000000>, matching Linux
 * bcm2837.dtsi), bus = arm_phys | 0xC0000000 for SDRAM below 0x3F000000.
 * Writing the raw ARM address makes the DMA access the GPU-CACHED 0x0 alias
 * and return stale (previously-cached) buffer content -- e.g. a CONFIG read
 * transmits the prior DEVICE descriptor and the host loops. The 0xC0000000
 * alias is uncached/direct-SDRAM, coherent with the CPU's buffer writes.
 * Hardware-confirmed on Pi Zero 2 W. Applied to the IN (DIEPDMA) path; the
 * OUT path's DMA writes propagate to RAM and its SETUP-capture code reads
 * DOEPDMA back as a CPU address, so it is intentionally left untranslated.
 */
static inline uint32_t brcm_bcm2835_usb_dma_addr(const struct device *dev, void *buf)
{
	ARG_UNUSED(dev);

	return (uint32_t)((uintptr_t)buf | 0xC0000000UL);
}

#define QUIRK_BRCM_BCM2835_USB_DEFINE(n)					\
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {		\
		.pre_enable = brcm_bcm2835_usb_pre_enable,			\
		.caps = brcm_bcm2835_usb_caps,					\
		.dma_addr = brcm_bcm2835_usb_dma_addr,				\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_BRCM_BCM2835_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_BRCM_BCM2835_USB_H */
