/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usb_dc_sf32lb.h"

void sf32lb_usbdc_read_packet(uintptr_t base, uint8_t ep_idx, uint8_t *buffer,
			      uint16_t len)
{
	uint32_t *buf32;
	uint8_t *buf8;
	uint32_t count32;
	uint32_t count8;
	int i;

	if (((uintptr_t)buffer & 0x03U) != 0U) {
		buf8 = buffer;
		for (i = 0; i < len; i++) {
			*buf8++ = SF32LB_USBDC_HWREGB(base + USB_FIFO_OFFSETX(ep_idx));
		}
		return;
	}

	count32 = len >> 2;
	count8 = len & 0x03;
	buf32 = (uint32_t *)buffer;

	while (count32--) {
		*buf32++ = SF32LB_USBDC_HWREG(base + USB_FIFO_OFFSETX(ep_idx));
	}

	buf8 = (uint8_t *)buf32;
	while (count8--) {
		*buf8++ = SF32LB_USBDC_HWREGB(base + USB_FIFO_OFFSETX(ep_idx));
	}
}

void sf32lb_usbdc_write_packet(uintptr_t base, uint8_t ep_idx, const uint8_t *buffer,
			       uint16_t len)
{
	const uint32_t *buf32;
	const uint8_t *buf8;
	uint32_t count32;
	uint32_t count8;
	int i;

	if (((uintptr_t)buffer & 0x03U) != 0U) {
		buf8 = buffer;
		for (i = 0; i < len; i++) {
			SF32LB_USBDC_HWREGB(base + USB_FIFO_OFFSETX(ep_idx)) = *buf8++;
		}
		return;
	}

	count32 = len >> 2;
	count8 = len & 0x03;
	buf32 = (const uint32_t *)buffer;

	while (count32--) {
		SF32LB_USBDC_HWREG(base + USB_FIFO_OFFSETX(ep_idx)) = *buf32++;
	}

	buf8 = (const uint8_t *)buf32;
	while (count8--) {
		SF32LB_USBDC_HWREGB(base + USB_FIFO_OFFSETX(ep_idx)) = *buf8++;
	}
}

void sf32lb_usbdc_phy_init(uintptr_t base, uintptr_t cfg)
{
	USBC_X_Typedef *reg = (USBC_X_Typedef *)base;

	SF32LB_USBDC_HWREG(cfg + USBCR) |=
		HPSYS_CFG_USBCR_DM_PD |
		HPSYS_CFG_USBCR_DP_EN |
		HPSYS_CFG_USBCR_USB_EN;

#ifndef SOC_SF32LB55X
	reg->usbcfg |= USB_USBCFG_AVALID | USB_USBCFG_AVALID_DR;
	reg->dpbrxdisl = 0xFE;
	reg->dpbtxdisl = 0xFE;
#endif
}

enum udc_bus_speed sf32lb_usbdc_device_speed(uintptr_t base)
{
	uint8_t power = SF32LB_USBDC_HWREGB(base + MUSB_POWER_OFFSET);
	uint8_t devctl = SF32LB_USBDC_HWREGB(base + MUSB_DEVCTL_OFFSET);

	if (power & USB_POWER_HSMODE) {
		return UDC_BUS_SPEED_HS;
	}

	if (devctl & USB_DEVCTL_FSDEV) {
		return UDC_BUS_SPEED_FS;
	}

	if (devctl & USB_DEVCTL_LSDEV) {
		return UDC_BUS_UNKNOWN;
	}

	return UDC_BUS_UNKNOWN;
}
