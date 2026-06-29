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
 * @brief Compare microframe numbers of xfers to determine which one should be sent first.
 *
 * @return Negative value if a < b, positive if a > b, 0 if a == b
 */
static inline int32_t xfer_seq_cmp(uint32_t a, uint32_t b)
{
	/*
	 * Use serial-number arithmetic for the unsigned 32-bit microframe counter. The
	 * wrapped subtraction is interpreted as a signed delta, so values just
	 * after rollover compare newer than values just before it. Comparisons
	 * are meaningful only within half the counter range.
	 */
	return (int32_t)(a - b);
}

/**
 * @brief Convert from bInterval value from USB descriptors to number of microframe ticks
 *
 * The microframe ticks are in 125 us units.
 *
 * @return The value in microframes ticks.
 */
uint32_t uhc_xfer_bInterval_to_microframe(const struct usb_device *const udev,
					  uint16_t bInterval, uint8_t ep_type);

/**
 * @brief Encode a number of microframe ticks into a bInterval as used in USB descriptors
 *
 * The microframe ticks are in 125 us units.
 *
 * @return The bInterval value.
 */
uint32_t uhc_xfer_microframe_to_bInterval(const struct usb_device *const udev,
					  uint32_t microframe_interval, uint8_t ep_type);

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
