/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_BOS_H_
#define ZEPHYR_INCLUDE_USB_BOS_H_

#include <stdint.h>

/**
 * @brief USB Binary Device Object Store support
 * @defgroup usb_bos USB BOS support
 * @ingroup usb
 * @{
 */

/** Root BOS Descriptor */
struct usb_bos_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumDeviceCaps;
} __packed;

/** Device capability type codes */
enum usb_bos_capability_types {
	USB_BOS_CAPABILITY_EXTENSION = 0x02,
	USB_BOS_CAPABILITY_PLATFORM = 0x05,
};

/** BOS USB 2.0 extension capability descriptor */
struct usb_bos_capability_lpm {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint32_t bmAttributes;
} __packed;

/** BOS platform capability descriptor */
struct usb_bos_platform_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint8_t bReserved;
	uint8_t PlatformCapabilityUUID[16];
} __packed;

/** WebUSB specific part of platform capability descriptor */
struct usb_bos_capability_webusb {
	uint16_t bcdVersion;
	uint8_t bVendorCode;
	uint8_t iLandingPage;
} __packed;

/** Microsoft OS 2.0 descriptor specific part of platform capability descriptor */
struct usb_bos_capability_msos {
	uint32_t dwWindowsVersion;
	uint16_t wMSOSDescriptorSetTotalLength;
	uint8_t bMS_VendorCode;
	uint8_t bAltEnumCode;
} __packed;

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_USB_BOS_H_ */
