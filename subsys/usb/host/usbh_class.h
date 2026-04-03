/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_H
#define ZEPHYR_INCLUDE_USBH_CLASS_H

#include <zephyr/usb/usbh.h>

/** Match both the device's vendor ID and product ID */
#define USBH_CLASS_MATCH_VID_PID BIT(1)

/** Match a class/subclass/protocol code triple */
#define USBH_CLASS_MATCH_CODE_TRIPLE BIT(2)

/** Interface number referring to the entire device instead of a particular interface */
#define USBH_CLASS_IFNUM_DEVICE 0xff

/**
 * @brief Match an USB host class (a driver) against a device descriptor.
 *
 * A filter set to NULL always matches.
 * This can be used to only rely on @c class_api->probe() (typically happening next) return value
 * for the matching.
 *
 * @param[in] filter_rules Array of filter rules to match, or NULL
 * @param[in] filtered_data Data against which match the filter
 *
 * @retval true if the USB Device descriptor matches at least one rule.
 * @retval false if all the rules failed to match.
 */
bool usbh_class_is_matching(const struct usbh_class_filter *const filter_rules,
			    const struct usbh_class_filter *const filtered_data);

/**
 * @brief Initialize all available host class instances.
 */
void usbh_class_init_all(void);

/**
 * @brief Probe an USB device function against all available host class instances.
 *
 * Try to match a class from the global list of all system classes using their filter rules
 * and return status to update the state of each matched class.
 *
 * The first matching host class driver is going to stop the scanning, and become the one in use.
 *
 * @param[in] udev USB device to probe.
 */
void usbh_class_probe_device(struct usb_device *const udev);

/**
 * @brief Call the device removal handler for every class configured with this device.
 *
 * @param[in] udev USB device that got removed.
 */
void usbh_class_remove_all(struct usb_device *const udev);

#endif /* ZEPHYR_INCLUDE_USBH_CLASS_H */
