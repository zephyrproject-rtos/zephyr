/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_CH9_H
#define ZEPHYR_INCLUDE_USBD_CH9_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Check whether USB device is in default state.
 *
 * @param[in] node Pointer to a device context
 *
 * @return true if USB device is in default state, false otherwise
 */
static inline bool usbd_state_is_default(const struct usbd_contex *const uds_ctx)
{
	return (uds_ctx->ch9_data.state == USBD_STATE_DEFAULT) ?  true : false;
}

/**
 * @brief Check whether USB device is in address state.
 *
 * @param[in] node Pointer to a device context
 *
 * @return true if USB device is in address state, false otherwise
 */
static inline bool usbd_state_is_address(const struct usbd_contex *const uds_ctx)
{
	return (uds_ctx->ch9_data.state == USBD_STATE_ADDRESS) ?  true : false;
}

/**
 * @brief Check whether USB device is in configured state.
 *
 * @param[in] node Pointer to a device context
 *
 * @return true if USB device is in configured state, false otherwise
 */
static inline bool usbd_state_is_configured(const struct usbd_contex *const uds_ctx)
{
	return (uds_ctx->ch9_data.state == USBD_STATE_CONFIGURED) ?  true : false;
}

/**
 * @brief Get current configuration value
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return current configuration value
 */
static inline uint8_t usbd_get_config_value(const struct usbd_contex *const uds_ctx)
{
	return uds_ctx->ch9_data.configuration;
}

/**
 * @brief Set current configuration value
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] value   New configuration value
 */
static inline void usbd_set_config_value(struct usbd_contex *const uds_ctx,
					  const uint8_t value)
{
	uds_ctx->ch9_data.configuration = value;
}

/**
 * @brief Get interface alternate value
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] iface   Interface number
 * @param[out] alt    Alternate value
 *
 * @return 0 on success, other values on fail.
 */
static inline uint8_t usbd_get_alt_value(const struct usbd_contex *const uds_ctx,
					 const uint8_t iface,
					 uint8_t *const alt)
{
	if (iface >= USBD_NUMOF_INTERFACES_MAX) {
		return -ENOENT;
	}

	*alt = uds_ctx->ch9_data.alternate[iface];

	return 0;
}

/**
 * @brief Set interface alternate value
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] iface   Interface number
 * @param[out] alt    Alternate value
 *
 * @return 0 on success, other values on fail.
 */
static inline uint8_t usbd_set_alt_value(struct usbd_contex *const uds_ctx,
					 const uint8_t iface,
					 const uint8_t alt)
{
	if (iface >= USBD_NUMOF_INTERFACES_MAX) {
		return -ENOENT;
	}

	uds_ctx->ch9_data.alternate[iface] = alt;

	return 0;
}

/**
 * @brief Get pointer to last received setup packet
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return Pointer to last received setup packet
 */
static inline struct usb_setup_packet *
usbd_get_setup_pkt(struct usbd_contex *const uds_ctx)
{
	return &uds_ctx->ch9_data.setup;
}

/**
 * @brief Handle control endpoint transfer result
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] buf     Pointer to UDC request buffer
 * @param[in] err     Trasnfer status
 *
 * @return 0 on success, other values on fail.
 */
int usbd_handle_ctrl_xfer(struct usbd_contex *uds_ctx,
			  struct net_buf *buf, int err);

/**
 * @brief Initialize control pipe to default values
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_init_control_pipe(struct usbd_contex *uds_ctx);


#endif /* ZEPHYR_INCLUDE_USBD_CH9_H */
