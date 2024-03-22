/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_MSG_H
#define ZEPHYR_INCLUDE_USBD_MSG_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Publish simple USB device message
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] type    Message type
 * @param[in] status  Status or error code
 */
void usbd_msg_pub_simple(struct usbd_contex *const ctx,
			 const enum usbd_msg_type type, const int status);

/**
 * @brief Publish USB device message with pointer to a device payload
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] type    Message type
 * @param[in] dev     Pointer to a device structure
 */
void usbd_msg_pub_device(struct usbd_contex *const ctx,
			 const enum usbd_msg_type type, const struct device *const dev);

#endif /* ZEPHYR_INCLUDE_USBD_MSG_H */
