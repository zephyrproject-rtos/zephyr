/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_CLASS_H
#define ZEPHYR_INCLUDE_USBD_CLASS_H

#include <zephyr/usb/usbh.h>

/**
 * @brief Information about a device, which is relevant for matching a particular class.
 */
struct usbh_class_filter {
	/** Vendor ID */
	uint16_t vid;
	/** Product ID */
	uint16_t pid;
	/** Device Class Code */
	uint8_t dclass;
	/** Class Subclass Code */
	uint8_t sub;
	/** Class Protocol Code */
	uint8_t proto;
	/** Flags that tell which field to match */
	uint8_t flags;
};

/** Match a device's vendor ID */
#define USBH_CLASS_MATCH_VID BIT(1)

/** Match a device's product ID */
#define USBH_CLASS_MATCH_PID BIT(2)

/** Match a class code */
#define USBH_CLASS_MATCH_DCLASS BIT(3)

/** Match a subclass code */
#define USBH_CLASS_MATCH_SUB BIT(4)

/** Match a protocol code */
#define USBH_CLASS_MATCH_PROTO BIT(5)

/** Match a code triple */
#define USBH_CLASS_MATCH_CODE_TRIPLE \
	(USBH_CLASS_MATCH_DCLASS | USBH_CLASS_MATCH_SUB | USBH_CLASS_MATCH_PROTO)

/**
 * @brief Match an USB host class (a driver) against a device descriptor.
 *
 * @param[in] filters Array of filter rules to match
 * @param[in] n_filters Number of rules in the array.
 * @param[in] desc USB Device descriptor to match against each rule
 * @retval true if the USB Device descriptor matches at least one rule.
 */
bool usbh_class_is_matching(struct usbh_class_filter *const filters, size_t n_filters,
			    struct usb_device_descriptor *const desc);

/**
 * @brief Initialize every class instantiated on the system
 *
 * @param[in] uhs_ctx USB Host context to pass to the class.
 * @retval 0 on success or negative error code on failure
 */
int usbh_class_init_all(struct usbh_context *const uhs_ctx);

/**
 * @brief Probe all classes to against a newly connected USB device.
 *
 * @param[in] udev USB device to probe.
 * @retval 0 on success or negative error code on failure
 */
int usbh_class_probe_all(struct usb_device *const udev);

/**
 * @brief Call the device removal handler for every class configured with it
 *
 * @param[in] udev USB device that got removed.
 * @retval 0 on success or negative error code on failure
 */
int usbh_class_remove_all(const struct usb_device *const udev);

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_H */
