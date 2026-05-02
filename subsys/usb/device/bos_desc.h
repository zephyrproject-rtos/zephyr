/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_DEVICE_BOS_DESC_H_
#define ZEPHYR_INCLUDE_USB_DEVICE_BOS_DESC_H_

#include <stdint.h>
#include <zephyr/usb/usb_ch9.h>

size_t usb_bos_get_length(void);

void usb_bos_fix_total_length(void);

const void *usb_bos_get_header(void);

#if defined(CONFIG_USB_DEVICE_BOS)
int usb_handle_bos(struct usb_setup_packet *setup, int32_t *len, uint8_t **data);
#else
#define usb_handle_bos(x, y, z)		-ENOTSUP
#endif

#endif	/* ZEPHYR_INCLUDE_USB_DEVICE_BOS_DESC_H_ */
