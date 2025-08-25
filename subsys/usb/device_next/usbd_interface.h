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
int usbd_interface_shutdown(struct usbd_context *const uds_ctx,
			    struct usbd_config_node *const cfg_nd);

/**
 * @brief Setup all interfaces in a configuration to default alternate.
 *
 * @note Used only for configuration change.
 *
 * @param[in] uds_ctx Pointer to USB device support context
 * @param[in] speed   Configuration speed
 * @param[in] cfg_nd  Pointer to configuration node
 *
 * @return 0 on success, other values on fail.
 */
int usbd_interface_default(struct usbd_context *const uds_ctx,
			   const enum usbd_speed speed,
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
int usbd_interface_set(struct usbd_context *uds_ctx,
		       const uint8_t iface,
		       const uint8_t alternate);

/**
 * @brief Calculate the maximum memory usage among configurations
 *
 * @param[in] uds_ctx   Pointer to USB device support context
 * @param[in] rx_size   Required RX FIFO size
 * @param[in] tx_size   Required TX FIFO size
 * @param[in] rx_m_tpl  Maximum possible RX TPL
 *
 * @return 0 on success, other values on fail.
 */
int usbd_interfaces_memory_usage(struct usbd_context *const uds_ctx,
				 size_t *const rx_size,
				 size_t *const tx_size,
				 uint16_t *const rx_m_tpl);

#endif /* ZEPHYR_INCLUDE_USBD_INTERFACE_H */
