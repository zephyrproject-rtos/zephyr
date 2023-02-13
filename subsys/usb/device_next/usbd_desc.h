/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_DESC_H
#define ZEPHYR_INCLUDE_USBD_DESC_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Get common USB descriptor
 *
 * Get descriptor from internal descrptor list.
 *
 * @param[in] ctx    Pointer to USB device support context
 * @param[in] type   Descriptor type (bDescriptorType)
 * @param[in] idx    Descriptor index
 *
 * @return pointer to descriptor or NULL if not found.
 */
void *usbd_get_descriptor(struct usbd_contex *uds_ctx,
			  const uint8_t type, const uint8_t idx);

#endif /* ZEPHYR_INCLUDE_USBD_DESC_H */
