/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
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
 * Bitmask selecting transfer types for uhc_xfer_get_next().
 *
 * Combine UHC_XFER_MASK_* values with bitwise OR to select multiple transfer types.
 */
typedef uint8_t uhc_xfer_mask_t;

/** Mask for control transfers. */
#define UHC_XFER_MASK_CONTROL   BIT(USB_EP_TYPE_CONTROL)
/** Mask for isochronous transfers. */
#define UHC_XFER_MASK_ISO       BIT(USB_EP_TYPE_ISO)
/** Mask for bulk transfers. */
#define UHC_XFER_MASK_BULK      BIT(USB_EP_TYPE_BULK)
/** Mask for interrupt transfers. */
#define UHC_XFER_MASK_INTERRUPT BIT(USB_EP_TYPE_INTERRUPT)

/** Mask for periodic transfers: interrupt or isochronous. */
#define UHC_XFER_MASK_PERIODIC     (UHC_XFER_MASK_INTERRUPT | UHC_XFER_MASK_ISO)
/** Mask for non-periodic transfers: control or bulk. */
#define UHC_XFER_MASK_NON_PERIODIC (UHC_XFER_MASK_CONTROL | UHC_XFER_MASK_BULK)

/** Match any type of transfer. */
#define UHC_XFER_MASK_ALL (UHC_XFER_MASK_PERIODIC | UHC_XFER_MASK_NON_PERIODIC)

/** Transfer filtering function used in uhc_xfer_get_next(). */
typedef bool (*uhc_xfer_filter_func_t)(const struct uhc_transfer *xfer, void *priv);

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
 * @brief Reschedules the periodic UHC transfer to it's next valid frame.
 *
 * @param[inout] xfer   Pointer to UHC transfer
 * @param frame_number The current USB frame number
 */
void uhc_xfer_reschedule_periodic(const struct device *dev, struct uhc_transfer *const xfer,
				  uint32_t frame_number);

/**
 * @brief Helper to get next transfer to process.
 *
 * Return the next transfer that matches the provided mask and is accepted by
 * the optional filter function. Move the selected transfer to the active list.
 *
 * If the mask matches more than one transfer type, prioritize transfers in this order:
 *
 * -# Most-overdue eligible periodic transfer, either interrupt or isochronous.
 * -# Control transfer.
 * -# Bulk transfer.
 *
 * To use a different priority order, call this function multiple times with masks that each
 * match only one transfer type.
 *
 * @param[in] dev          Pointer to device struct of the driver instance.
 * @param[in] frame_number Current USB frame number used to determine periodic transfer eligibility.
 * @param[in] mask         Bitmask of transfer types to consider. Combine UHC_XFER_MASK_* values
 *                         with bitwise OR to select multiple transfer types.
 * @param[in] filter       Optional function for further filtering transfers selected by @p mask.
 *                         Return `true` to accept a transfer or `false` to skip it. Set to `NULL`
 *                         to accept the first transfer selected by @p mask.
 * @param[in] priv         Private data passed unchanged to @p filter. May be `NULL`.
 *
 * @return Next eligible transfer, or `NULL` if there are no pending matching non-periodic
 *         transfers, no matching periodic transfer is due, or all otherwise eligible transfers
 *         are rejected by @p filter.
 */
struct uhc_transfer *uhc_xfer_get_next(const struct device *dev, uint32_t frame_number,
				       uhc_xfer_mask_t mask, uhc_xfer_filter_func_t filter,
				       void *priv);

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
