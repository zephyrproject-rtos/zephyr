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
