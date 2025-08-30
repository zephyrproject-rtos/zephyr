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
 * @brief Auto-register all compile-time defined class drivers
 */
int usbh_register_all_classes(struct usbh_context *uhs_ctx);

/**
 * @brief Initialize registered class drivers
 */
int usbh_init_registered_classes(struct usbh_context *uhs_ctx);

/**
 * @brief Check if a device's descriptor information matches a host class instance
 *
 * @param cdata Pointer to class driver data
 * @param device_info Information from the device descriptors
 *
 * @return true if matched, false otherwise
 */
bool usbh_class_is_matching(struct usbh_class_data *cdata,
			    struct usbh_class_filter *device_info);

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_H */
