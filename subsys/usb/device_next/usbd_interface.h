/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_INTERFACE_H
#define ZEPHYR_INCLUDE_USBD_INTERFACE_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Shutdown all interfaces in a configuration.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] cfg_nd  Pointer to configuration node
 *
 * @return 0 on success, other values on fail.
 */
int usbd_interface_shutdown(struct usbd_contex *const uds_ctx,
			    struct usbd_config_node *const cfg_nd);

/**
 * @brief Setup all interfaces in a configuration to default alternate.
 *
 * @note Used only for configuration change.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] cfg_nd  Pointer to configuration node
 *
 * @return 0 on success, other values on fail.
 */
int usbd_interface_default(struct usbd_contex *const uds_ctx,
			   struct usbd_config_node *const cfg_nd);

/**
 * @brief Set interface alternate
 *
 * @note Used only for configuration change.
 *
 * @param[in] uds_ctx   Pointer to USB device support context
 * @param[in] iface     Interface number (bInterfaceNumber)
 * @param[in] alternate Interface alternate (bAlternateSetting)
 *
 * @return 0 on success, other values on fail.
 */
int usbd_interface_set(struct usbd_contex *uds_ctx,
		       const uint8_t iface,
		       const uint8_t alternate);

#endif /* ZEPHYR_INCLUDE_USBD_INTERFACE_H */
