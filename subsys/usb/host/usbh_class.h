/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_H
#define ZEPHYR_INCLUDE_USBH_CLASS_H

#include <zephyr/usb/usbh.h>

/** Match both the device's vendor ID and product ID */
#define USBH_CLASS_MATCH_VID_PID BIT(1)

/** Match a class/subclass/protocol code triple */
#define USBH_CLASS_MATCH_CODE_TRIPLE BIT(2)

/**
 * @brief Match an USB host class (a driver) against a device descriptor.
 *
 * An empty filter set matches everything.
 * This can be used to only rely on @c class_api->probe() return value.
 *
 * @param[in] filters Array of filter rules to match
 * @param[in] device_info Device information filled by this function
 *
 * @retval true if the USB Device descriptor matches at least one rule.
 */
bool usbh_class_is_matching(struct usbh_class_filter *const filters,
			    struct usbh_class_filter *const device_info);

/**
 * @brief Initialize every class instantiated on the system
 *
 * @param[in] uhs_ctx USB Host context to pass to the class.
 */
void usbh_class_init_all(struct usbh_context *const uhs_ctx);

/**
 * @brief Probe an USB device function against all available classes.
 *
 * Try to match a class from the global list of all system classes using their filter rules
 * and return status to update the state of each matched class.
 *
 * The first matched class
 *
 * @param[in] udev USB device to probe.
 */
void usbh_class_probe_device(struct usb_device *const udev);

/**
 * @brief Call the device removal handler for every class configured with it
 *
 * @param[in] udev USB device that got removed.
 */
void usbh_class_remove_all(struct usb_device *const udev);

#endif /* ZEPHYR_INCLUDE_USBH_CLASS_H */
