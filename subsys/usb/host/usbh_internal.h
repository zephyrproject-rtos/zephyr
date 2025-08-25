/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_INTERNAL_H
#define ZEPHYR_INCLUDE_USBH_INTERNAL_H

#include <stdint.h>
#include <zephyr/usb/usbh.h>

int usbh_init_device_intl(struct usbh_context *const uhs_ctx);

#endif /* ZEPHYR_INCLUDE_USBH_INTERNAL_H */
