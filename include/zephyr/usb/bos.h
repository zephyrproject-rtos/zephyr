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
 * @since 1.13
 * @version 1.0.0
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
	USB_BOS_CAPABILITY_SUPERSPEED_USB = 0x03,
	USB_BOS_CAPABILITY_PLATFORM = 0x05,
	USB_BOS_CAPABILITY_SUPERSPEED_PLUS = 0x0a,
	USB_BOS_CAPABILITY_PRECISION_TIME_MEASUREMENT = 0x0b,
};

/** BOS USB 2.0 extension capability descriptor */
struct usb_bos_capability_lpm {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint32_t bmAttributes;
} __packed;

/** Fields for @ref usb_bos_capability_lpm bmAttributes */
enum usb_bos_attributes {
	USB_BOS_ATTRIBUTES_LPM = BIT(1),
	USB_BOS_ATTRIBUTES_BESL = BIT(2),
};

/** BOS platform capability descriptor */
struct usb_bos_platform_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint8_t bReserved;
	uint8_t PlatformCapabilityUUID[16];
} __packed;

/** BOS SuperSpeed device capability descriptor */
struct usb_bos_capability_superspeed_usb {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint8_t bmAttributes;
	uint16_t wSpeedsSupported;
	uint8_t bFunctionnalSupport;
	uint8_t bU1DevExitLat;
	uint16_t wU2DevExitLat;
} __packed;

/** Fields for @ref usb_bos_capability_superspeed_usb bmAttributes */
enum usb_bos_attributes_superspeed_usb {
	USB_BOS_ATTRIBUTES_SUPERSPEED_LTM = BIT(1),
};

/** BOS SuperSpeedPlus device capability descriptor */
struct usb_bos_capability_superspeedplus {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint8_t bReserved;
	uint16_t bmAttributes;
	uint16_t bFunctionnalSupport;
	/* Variable size depending on the SSAC value in bmAttributes */
	uint32_t bmSublinkSpeedAttr[1];
} __packed;

/** BOS Precision Time Measurement device capability descriptor */
struct usb_bos_capability_precision_time_measurement {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
};

/** BOS description of different speeds supported by the device */
enum usb_bos_speed {
	USB_BOS_SPEED_LOWSPEED = BIT(0),
	USB_BOS_SPEED_FULLSPEED = BIT(1),
	USB_BOS_SPEED_HIGHSPEED = BIT(2),
	USB_BOS_SPEED_SUPERSPEED_GEN1 = BIT(3),
	USB_BOS_SPEED_SUPERSPEED_GEN2 = BIT(4),
};

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
