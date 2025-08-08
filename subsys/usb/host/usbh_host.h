/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_HOST_H
#define ZEPHYR_INCLUDE_USBH_HOST_H

#include <stdbool.h>
#include <zephyr/usb/usbh.h>

/**
 * @brief Check whether USB host is enabled
 *
 * @param[in] uhs_ctx Pointer to a host context
 *
 * @return true if USB host is in enabled, false otherwise
 */
static inline bool usbd_is_enabled(const struct usbh_context *const uhs_ctx)
{
	return uhs_ctx->status.enabled;
}

/**
 * @brief Check whether USB host is enabled
 *
 * @param[in] uhs_ctx Pointer to a host context
 *
 * @return true if USB host is in enabled, false otherwise
 */
static inline bool usbd_is_initialized(const struct usbh_context *const uhs_ctx)
{
	return uhs_ctx->status.initialized;
}

/**
 * @brief Lock the USB host stack context
 *
 * @param[in] uhs_ctx Pointer to a host context
 */
static inline void usbh_host_lock(struct usbh_context *const uhs_ctx)
{
	k_mutex_lock(&uhs_ctx->mutex, K_FOREVER);
}

/**
 * @brief Unlock the USB host stack context
 *
 * @param[in] uhs_ctx Pointer to a host context
 */
static inline void usbh_host_unlock(struct usbh_context *const uhs_ctx)
{
	k_mutex_unlock(&uhs_ctx->mutex);
}

#endif /* ZEPHYR_INCLUDE_USBH_HOST_H */
