/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_H
#define ZEPHYR_INCLUDE_USBH_CLASS_H

#include <zephyr/usb/usbh.h>

/** Match a device's vendor ID */
#define USBH_CLASS_MATCH_VID BIT(1)

/** Match a device's product ID */
#define USBH_CLASS_MATCH_PID BIT(2)

/** Match a class code */
#define USBH_CLASS_MATCH_CLASS BIT(3)

/** Match a subclass code */
#define USBH_CLASS_MATCH_SUB BIT(4)

/** Match a protocol code */
#define USBH_CLASS_MATCH_PROTO BIT(5)

/** Match a code triple */
#define USBH_CLASS_MATCH_CODE_TRIPLE \
	(USBH_CLASS_MATCH_CLASS | USBH_CLASS_MATCH_SUB | USBH_CLASS_MATCH_PROTO)

/**
 * @brief Match an USB host class (a driver) against a device descriptor.
 *
 * An empty filter set matches everything.
 * This can be used to only rely on @c class_api->probe() return value.
 *
 * @param[in] filters Array of filter rules to match
 * @param[in] num_filters Number of rules in the array.
 * @param[in] device_info Device information filled by this function
 *
 * @retval true if the USB Device descriptor matches at least one rule.
 */
bool usbh_class_is_matching(struct usbh_class_filter *const filters, size_t num_filters,
			    struct usbh_class_filter *const device_info);

/**
 * @brief Initialize every class instantiated on the system
 *
 * @param[in] uhs_ctx USB Host context to pass to the class.
 *
 * @retval 0 on success or negative error code on failure
 */
void usbh_class_init_all(struct usbh_context *const uhs_ctx);

/**
 * @brief Probe an USB device function against all available classes of the system.
 *
 * Try to match a class from the global list of all system classes, using their filter rules
 * and return status to tell if a class matches or not.
 *
 * The first matched class will stop the loop, and the status will be updated so that classes
 * are only matched for a single USB function at a time.
 *
 * USB functions will only have one class matching, and calling @ref usbh_class_probe_function()
 * multiple times consequently has no effect.
 *
 * @param[in] udev USB device to probe.
 * @param[in] info Information about the device to match.
 * @param[in] iface Interface number that  the device uses.
 *
 * @retval 0 on success or negative error code on failure
 */
void usbh_class_probe_function(struct usb_device *const udev,
			       struct usbh_class_filter *const info, uint8_t iface);

/**
 * @brief Probe an USB device function against all available classes.
 *
 * Try to match a class from the global list of all system classes using their filter rules
 * and return status to update the state of each matched class.
 *
 * The first matched class
 *
 * @param[in] udev USB device to probe.
 *
 * @retval 0 on success or negative error code on failure
 */
void usbh_class_probe_device(struct usb_device *const udev);

/**
 * @brief Call the device removal handler for every class configured with it
 *
 * @param[in] udev USB device that got removed.
 *
 * @retval 0 on success or negative error code on failure
 */
void usbh_class_remove_all(struct usb_device *const udev);

#endif /* ZEPHYR_INCLUDE_USBH_CLASS_H */
