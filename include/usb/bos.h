/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_BOS_H_
#define ZEPHYR_INCLUDE_USB_BOS_H_

#if defined(CONFIG_USB_DEVICE_BOS)
#define USB_DEVICE_BOS_DESC_DEFINE_HDR \
	static __in_section(usb, bos_desc_area, 0) __aligned(1) __used
#define USB_DEVICE_BOS_DESC_DEFINE_CAP \
	static __in_section(usb, bos_desc_area, 1) __aligned(1) __used

/* BOS descriptor type */
#define DESCRIPTOR_TYPE_BOS		0x0F

#define USB_BOS_CAPABILITY_PLATFORM	0x05

/* BOS Capability Descriptor */
struct usb_bos_platform_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint8_t bReserved;
	uint8_t PlatformCapabilityUUID[16];
} __packed;

/* BOS Descriptor */
struct usb_bos_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumDeviceCaps;
} __packed;

/* BOS Capability webusb */
struct usb_bos_capability_webusb {
	uint16_t bcdVersion;
	uint8_t bVendorCode;
	uint8_t iLandingPage;
} __packed;

/* BOS Capability MS OS Descriptors version 2 */
struct usb_bos_capability_msos {
	uint32_t dwWindowsVersion;
	uint16_t wMSOSDescriptorSetTotalLength;
	uint8_t bMS_VendorCode;
	uint8_t bAltEnumCode;
} __packed;

size_t usb_bos_get_length(void);
void usb_bos_fix_total_length(void);
void usb_bos_register_cap(struct usb_bos_platform_descriptor *hdr);
const void *usb_bos_get_header(void);
int usb_handle_bos(struct usb_setup_packet *setup, int32_t *len, uint8_t **data);
#else
#define usb_handle_bos(x, y, z)		-ENOTSUP
#endif

#endif	/* ZEPHYR_INCLUDE_USB_BOS_H_ */
