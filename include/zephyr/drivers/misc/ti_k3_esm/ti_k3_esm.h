/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the TI K3 Error Signaling Module (ESM).
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_TI_K3_ESM_TI_K3_ESM_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_TI_K3_ESM_TI_K3_ESM_H_

#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback invoked when an enabled ESM event is pending.
 *
 * May run in ISR context. Keep it short.
 *
 * @param dev       ESM device.
 * @param event     Pending event number (0 … N-1).
 * @param user_data Cookie from @ref ti_k3_esm_set_callback.
 */
typedef void (*ti_k3_esm_callback_t)(const struct device *dev, uint16_t event,
				     void *user_data);

/**
 * @brief Install (or clear) the event callback.
 *
 * @param dev       ESM device.
 * @param cb        Callback, or NULL to disable delivery.
 * @param user_data Opaque pointer passed to @p cb.
 *
 * @retval 0 always.
 */
int ti_k3_esm_set_callback(const struct device *dev, ti_k3_esm_callback_t cb,
			   void *user_data);

/**
 * @brief Enable CPU interrupt delivery for @p event (high priority).
 *
 * Also enables the error-pin contribution for the event.
 *
 * @param dev   ESM device.
 * @param event Event number.
 *
 * @retval 0       success
 * @retval -EINVAL event out of range
 */
int ti_k3_esm_enable_event(const struct device *dev, uint16_t event);

/**
 * @brief Disable CPU interrupt delivery for @p event.
 *
 * @param dev   ESM device.
 * @param event Event number.
 *
 * @retval 0       success
 * @retval -EINVAL event out of range
 */
int ti_k3_esm_disable_event(const struct device *dev, uint16_t event);

/**
 * @brief Clear the raw/pending status for @p event.
 *
 * @param dev   ESM device.
 * @param event Event number.
 *
 * @retval 0       success
 * @retval -EINVAL event out of range
 */
int ti_k3_esm_clear_event(const struct device *dev, uint16_t event);

/**
 * @brief Return whether @p event has a pending raw status bit.
 *
 * @param dev   ESM device.
 * @param event Event number.
 *
 * @retval 1       pending
 * @retval 0       not pending
 * @retval -EINVAL event out of range
 */
int ti_k3_esm_event_pending(const struct device *dev, uint16_t event);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_TI_K3_ESM_TI_K3_ESM_H_ */
