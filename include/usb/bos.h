/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_BOS_H_
#define ZEPHYR_INCLUDE_USB_BOS_H_

#if defined(CONFIG_USB_DEVICE_BOS)
#define USB_DEVICE_BOS_DESC_DEFINE_HDR \
	static __in_section(usb, bos_desc_area, 0) __used
#define USB_DEVICE_BOS_DESC_DEFINE_CAP \
	static __in_section(usb, bos_desc_area, 1) __used

/* BOS descriptor type */
#define DESCRIPTOR_TYPE_BOS		0x0F

#define USB_BOS_CAPABILITY_PLATFORM	0x05

/* BOS Capability Descriptor */
struct usb_bos_platform_descriptor {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bDevCapabilityType;
	u8_t bReserved;
	u8_t PlatformCapabilityUUID[16];
} __packed;

/* BOS Descriptor */
struct usb_bos_descriptor {
	u8_t bLength;
	u8_t bDescriptorType;
	u16_t wTotalLength;
	u8_t bNumDeviceCaps;
} __packed;

/* BOS Capability webusb */
struct usb_bos_capability_webusb {
	u16_t bcdVersion;
	u8_t bVendorCode;
	u8_t iLandingPage;
} __packed;

/* BOS Capability MS OS Descriptors version 2 */
struct usb_bos_capability_msos {
	u32_t dwWindowsVersion;
	u16_t wMSOSDescriptorSetTotalLength;
	u8_t bMS_VendorCode;
	u8_t bAltEnumCode;
} __packed;

size_t usb_bos_get_length(void);
void usb_bos_fix_total_length(void);
void usb_bos_register_cap(struct usb_bos_platform_descriptor *hdr);
const void *usb_bos_get_header(void);
int usb_handle_bos(struct usb_setup_packet *setup, s32_t *len, u8_t **data);
#else
#define usb_handle_bos(x, y, z)		-ENOTSUP
#endif

#endif	/* ZEPHYR_INCLUDE_USB_BOS_H_ */
