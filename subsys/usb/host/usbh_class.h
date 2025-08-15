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

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_H */
