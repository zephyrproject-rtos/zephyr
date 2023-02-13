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
 * @brief Checks if the transfer is queued.
 *
 * @param[in] xfer    Pointer to UHC transfer
 *
 * @return true if transfer is queued, false otherwise
 */
static inline bool uhc_xfer_is_queued(struct uhc_transfer *xfer)
{
	return xfer->queued;
}

/**
 * @brief Helper function to set queued flag
 *
 * This function can be used by the driver to set queued flag
 *
 * @param[in] xfer    Pointer to UHC transfer
 */
static inline void uhc_xfer_queued(struct uhc_transfer *xfer)
{
	xfer->queued = true;
}

/**
 * @brief Checks if the setup flag is set.
 *
 * @param[in] xfer    Pointer to UHC transfer
 *
 * @return true if setup flagh is set, false otherwise
 */
static inline bool uhc_xfer_is_setup(struct uhc_transfer *xfer)
{
	return xfer->setup;
}

/**
 * @brief Helper function to set setup flag
 *
 * This function can be used by the driver to set setup flag
 *
 * @param[in] xfer    Pointer to UHC transfer
 */
static inline void uhc_xfer_setup(struct uhc_transfer *xfer)
{
	xfer->setup = true;
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
 * @brief Helper to move current buffer in the done-FIFO.
 *
 * Helper to move current buffer (probably completed) in the
 * designated done-FIFO.
 *
 * @param[in] xfer   Pointer to UHC transfer
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -ENOMEM if there is no buffer in the queue
 */
static inline int uhc_xfer_done(struct uhc_transfer *xfer)
{
	struct net_buf *buf;

	buf = k_fifo_get(&xfer->queue, K_NO_WAIT);
	if (buf) {
		k_fifo_put(&xfer->done, &buf->node);
	}

	return buf == NULL ? -ENOMEM : 0;
}

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
 * @brief Helper function to send UHC event to a higher level.
 *
 * The callback would typically sends UHC even to a message queue (k_msgq).
 *
 * @param[in] dev    Pointer to device struct of the driver instance
 * @param[in] type   Event type
 * @param[in] status Event status
 * @param[in] xfer   Pointer to UHC transfer
 *
 * @return 0 on success, all other values should be treated as error.
 * @retval -EPERM controller is not initialized
 */
int uhc_submit_event(const struct device *dev,
		     const enum uhc_event_type type,
		     const int status,
		     struct uhc_transfer *const xfer);

#endif /* ZEPHYR_INCLUDE_UHC_COMMON_H */
