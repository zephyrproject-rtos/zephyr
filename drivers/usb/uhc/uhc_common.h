/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for USB host controller (UHC) drivers
 */

#ifndef ZEPHYR_INCLUDE_UHC_COMMON_H
#define ZEPHYR_INCLUDE_UHC_COMMON_H

#include <zephyr/drivers/usb/uhc.h>

/**
 * @brief Get driver's private data
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return pointer to driver's private data
 */
static inline void *uhc_get_private(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return data->priv;
}

/**
 * @brief Initialize UHC common data
 *
 * UHC drivers using the common helpers must call this function during
 * driver initialization.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return 0 on success, all other values should be treated as error.
 */
int uhc_common_init(const struct device *dev);

/**
 * @brief Locking function for the drivers.
 *
 * @param[in] dev     Pointer to device struct of the driver instance
 * @param[in] timeout Timeout
 *
 * @return values provided by k_mutex_lock()
 */
static inline int uhc_lock_internal(const struct device *dev,
				    k_timeout_t timeout)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, timeout);
}

/**
 * @brief Unlocking function for the drivers.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 *
 * @return values provided by k_mutex_lock()
 */
static inline int uhc_unlock_internal(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

/**
 * @brief Helper function to return UHC transfer to a higher level.
 *
 * Function to dequeue transfer and send UHC event to a higher level.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] xfer   Pointer to UHC transfer
 * @param[in] err    Transfer error
 */
void uhc_xfer_return(const struct device *dev,
		     struct uhc_transfer *const xfer,
		     const int err);

/**
 * @brief Helper to get next transfer to process.
 *
 * This is currently a draft, and simple picks a transfer
 * from the lists.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @return pointer to the next transfer or NULL on error.
 */
struct uhc_transfer *uhc_xfer_get_next(const struct device *dev);

/**
 * @brief Helper to append a transfer to internal list.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] xfer   Pointer to UHC transfer
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENOMEM if there is no buffer in the queue
 */
int uhc_xfer_append(const struct device *dev,
		    struct uhc_transfer *const xfer);

/**
 * @brief Handle a canceled transfer during cleanup.
 *
 * Will be called for each canceled transfer.
 *
 * The callback may return the transfer with uhc_xfer_return() or leave it tracked
 * for later completion. It must not add other transfers to, or remove them from,
 * the internal dlists.
 *
 * @param[in] dev  Pointer to device struct of the driver instance
 * @param[in] xfer Canceled transfer
 * @param[in] priv Private data passed to uhc_xfer_cleanup_cancelled()
 */
typedef void (*uhc_xfer_cancel_cb_t)(const struct device *dev, struct uhc_transfer *xfer,
				     void *priv);

/**
 * @brief Process canceled transfers in the internal queues.
 *
 * A canceled transfer has its err field set to -ECONNRESET. If callback is
 * NULL, return each canceled transfer with that error. Otherwise, invoke the
 * callback for each canceled transfer without returning it automatically.
 *
 * If provided, the callback is called once for each canceled transfer.
 * If the callback does not return the transfer using uhc_xfer_return(),
 * it will be revisited on the next cleanup.
 *
 * The caller must hold the driver’s lock while calling this function.
 *
 * @param[in] dev      Pointer to device struct of the driver instance
 * @param[in] callback Optional callback responsible for handling each canceled transfer
 * @param[in] priv     Private data passed unchanged to callback. May be NULL
 */
void uhc_xfer_cleanup_cancelled(const struct device *dev, uhc_xfer_cancel_cb_t callback,
				void *priv);

/**
 * @brief Marks a queued transfer as canceled
 *
 * This function does not fully remove the transfer.
 * To fully delete the transfer, call `uhc_xfer_cleanup_cancelled()`
 * after marking a transfer as canceled.
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] xfer   Pointer to UHC transfer that should be canceled
 *
 * @return 0 on success, all other values should be treated as error.
 */
int uhc_xfer_dequeue(const struct device *dev, struct uhc_transfer *const xfer);

/**
 * @brief Helper function to send UHC event to a higher level.
 *
 * The callback would typically sends UHC even to a message queue (k_msgq).
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] type   Event type
 * @param[in] status Event status
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not initialized
 */
int uhc_submit_event(const struct device *dev,
		     const enum uhc_event_type type,
		     const int status);

#endif /* ZEPHYR_INCLUDE_UHC_COMMON_H */
