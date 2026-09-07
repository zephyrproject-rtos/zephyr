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

/**
 * @brief Submit work to the USB host bus work queue
 *
 * The bus events of the host controllers are handled on this work queue,
 * class instances use it for work that needs synchronous requests, whose
 * completions are handled on the host stack thread.
 *
 * @param[in] work Pointer to the work item
 *
 * @return As @ref k_work_submit_to_queue
 */
int usbh_bus_work_submit(struct k_work *const work);

#endif /* ZEPHYR_INCLUDE_USBH_HOST_H */
