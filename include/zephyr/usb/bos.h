/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Binary Device Object Store support
 * @ingroup usb_bos
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
	/** Size of this descriptor in bytes (5). */
	uint8_t bLength;
	/** Descriptor type. Must be set to @ref USB_DESC_BOS. */
	uint8_t bDescriptorType;
	/**
	 * Total length of this descriptor and all associated device
	 * capability descriptors.
	 */
	uint16_t wTotalLength;
	/** Number of device capability descriptors that follow. */
	uint8_t bNumDeviceCaps;
} __packed;

/** Device capability type codes */
enum usb_bos_capability_types {
	/** USB 2.0 Extension capability. */
	USB_BOS_CAPABILITY_EXTENSION = 0x02,
	/** Platform-specific capability (e.g., WebUSB, MS OS). */
	USB_BOS_CAPABILITY_PLATFORM = 0x05,
};

/**
 * BOS USB 2.0 extension capability descriptor
 *
 * Used to indicate support for USB 2.0 Link Power Management (LPM) and associated best effort
 * service latency (BESL) parameters.
 */
struct usb_bos_capability_lpm {
	/** Size of this descriptor in bytes. */
	uint8_t bLength;

	/** Descriptor type. Must be set to @ref USB_DESC_DEVICE_CAPABILITY. */
	uint8_t bDescriptorType;

	/** Device capability type. Must be @ref USB_BOS_CAPABILITY_EXTENSION. */
	uint8_t bDevCapabilityType;

	/**
	 * Bitmap of supported attributes.
	 */
	uint32_t bmAttributes;
} __packed;

/**
 * BOS platform capability descriptor
 *
 * Used to describe platform-specific capabilities, identified by a UUID.
 */
struct usb_bos_platform_descriptor {
	/** Size of this descriptor in bytes (20). */
	uint8_t bLength;
	/** Descriptor type. Must be set to @c USB_DESC_DEVICE_CAPABILITY. */
	uint8_t bDescriptorType;
	/** Device capability type. Must be @c USB_BOS_CAPABILITY_PLATFORM. */
	uint8_t bDevCapabilityType;
	/** Reserved (must be zero). */
	uint8_t bReserved;
	/** Platform capability UUID (16 bytes, little-endian). */
	uint8_t PlatformCapabilityUUID[16];
} __packed;

/**
 * WebUSB specific part of platform capability descriptor
 *
 * Defines the WebUSB-specific fields that extend the generic platform capability descriptor.
 */
struct usb_bos_capability_webusb {
	/** WebUSB specification version in BCD format (e.g., 0x0100). */
	uint16_t bcdVersion;
	/**
	 * Vendor-specific request code used by the host to retrieve WebUSB descriptors.
	 */
	uint8_t bVendorCode;

	/**
	 * Index of the landing page string descriptor.
	 * Zero means no landing page is defined.
	 */
	uint8_t iLandingPage;
} __packed;

/**
 * Microsoft OS 2.0 descriptor specific part of platform capability descriptor
 *
 * Defines the Microsoft OS 2.0 descriptor set, used to describe device capabilities to Windows
 * hosts.
 */
struct usb_bos_capability_msos {
	/** Windows version supported (e.g., 0x0A000000UL for Windows 10). */
	uint32_t dwWindowsVersion;
	/** Total length of the MS OS 2.0 descriptor set. */
	uint16_t wMSOSDescriptorSetTotalLength;
	/**
	 * Vendor-specific request code used to retrieve the MS OS 2.0 descriptor set.
	 */
	uint8_t bMS_VendorCode;
	/** Alternate enumeration code (or 0 if not used). */
	uint8_t bAltEnumCode;
} __packed;

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_USB_BOS_H_ */
