/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_DEVICE_H
#define ZEPHYR_INCLUDE_USBD_DEVICE_H

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

/**
 * @brief Get device descriptor bNumConfigurations value
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] speed   Speed for which the bNumConfigurations should be returned
 *
 * @return bNumConfigurations value
 */
static inline uint8_t usbd_get_num_configs(const struct usbd_context *const uds_ctx,
					   const enum usbd_speed speed)
{
	struct usb_device_descriptor *desc;

	if (speed == USBD_SPEED_FS) {
		desc = uds_ctx->fs_desc;
	} else if (speed == USBD_SPEED_HS) {
		desc = uds_ctx->hs_desc;
	} else {
		return 0;
	}

	return desc->bNumConfigurations;
}


/**
 * @brief Set device descriptor bNumConfigurations value
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] speed   Speed for which the bNumConfigurations should be set
 * @param[in] value   new bNumConfigurations value
 */
static inline void usbd_set_num_configs(struct usbd_context *const uds_ctx,
					const enum usbd_speed speed,
					const uint8_t value)
{
	struct usb_device_descriptor *desc;

	if (speed == USBD_SPEED_FS) {
		desc = uds_ctx->fs_desc;
	} else if (speed == USBD_SPEED_HS) {
		desc = uds_ctx->hs_desc;
	} else {
		return;
	}

	desc->bNumConfigurations = value;
}

/**
 * @brief Check whether USB device is enabled
 *
 * @param[in] node Pointer to a device context
 *
 * @return true if USB device is in enabled, false otherwise
 */
static inline bool usbd_is_enabled(const struct usbd_context *const uds_ctx)
{
	return uds_ctx->status.enabled;
}

/**
 * @brief Check whether USB device is enabled
 *
 * @param[in] node Pointer to a device context
 *
 * @return true if USB device is in enabled, false otherwise
 */
static inline bool usbd_is_initialized(const struct usbd_context *const uds_ctx)
{
	return uds_ctx->status.initialized;
}

/**
 * @brief Set device suspended status
 *
 * @param[in] uds_ctx Pointer to a device context
 * @param[in] value   new suspended value
 */
static inline void usbd_status_suspended(struct usbd_context *const uds_ctx,
					 const bool value)
{
	uds_ctx->status.suspended = value;
}

/**
 * @brief Lock USB device stack context
 *
 * @param[in] node Pointer to a device context
 */
static inline void usbd_device_lock(struct usbd_context *const uds_ctx)
{
	k_mutex_lock(&uds_ctx->mutex, K_FOREVER);
}

/**
 * @brief Lock USB device stack context
 *
 * @param[in] node Pointer to a device context
 */
static inline void usbd_device_unlock(struct usbd_context *const uds_ctx)
{
	k_mutex_unlock(&uds_ctx->mutex);
}

/**
 * @brief Init USB device stack core
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_device_init_core(struct usbd_context *uds_ctx);

/**
 * @brief Shutdown USB device stack core
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_device_shutdown_core(struct usbd_context *const uds_ctx);

#endif /* ZEPHYR_INCLUDE_USBD_DEVICE_H */
