/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
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
 * Reserve class instances for all functions of a given configuration.
 *
 * Match device-level and interface-level functions against registered
 * class drivers, marking matched class nodes as RESERVED for
 * subsequent usbh_class_probe_device() to bind.
 *
 * @param[in] udev USB device to match against
 * @param[in] cfg_desc Configuration descriptor pointer
 *
 * @retval true  At least one class instance was reserved
 * @retval false No match found
 */
bool usbh_class_match_device(struct usb_device *const udev, const void *cfg_desc);

/**
 * @brief Probe all matched USB classes of current USB device.
 *
 * Try to probe all class instances that matched, which are identified by usbh_class_match_device().
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
