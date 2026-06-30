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
 * @brief Get USB device pipe context by endpoint address.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] ep   Endpoint address
 *
 * @return Pointer to IN or OUT pipe context.
 */
static inline struct usb_host_pipe *uhc_get_udev_pipe(struct usb_device *udev, uint8_t ep)
{
	uint8_t idx = USB_EP_GET_IDX(ep) & 0xFU;

	/* Control endpoints only need one `struct usb_host_pipe`.
	 * If both directions are needed for some vendors controllers, this needs to be improved.
	 */
	if (USB_EP_DIR_IS_IN(ep) || (idx == 0)) {
		return &udev->pipe_in[idx];
	} else {
		return &udev->pipe_out[idx];
	}
}

/**
 * @brief Get USB device endpoint address by endpoint context.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] pipe Pointer to pipe
 *
 * @return Endpoint address on success, negative errno code on error.
 * @retval -EINVAL if endpoint context is not found
 */
static inline int uhc_get_udev_ep(struct usb_device *udev, struct usb_host_pipe *pipe)
{
	for (size_t i = 0; i < 16; i++) {
		if (pipe == &udev->pipe_in[i]) {
			return i | USB_EP_DIR_IN;
		}

		if (pipe == &udev->pipe_out[i]) {
			return i | USB_EP_DIR_OUT;
		}
	}

	return -EINVAL;
}

/**
 * @brief Get USB device endpoint maximum packet size.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] ep   Endpoint address
 *
 * @return Maximum packet size of the endpoint, 0 if endpoint index is invalid.
 */
static inline uint16_t uhc_get_udev_ep_mps(struct usb_device *udev, uint8_t ep)
{
	uint8_t idx = USB_EP_GET_IDX(ep) & 0xFU;

	if (idx == 0) {
		return udev->pipe_in[0].control_mps;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		return udev->pipe_in[idx].desc != NULL ?
			udev->pipe_in[idx].desc->wMaxPacketSize : 0;
	} else {
		return udev->pipe_out[idx].desc != NULL ?
			udev->pipe_out[idx].desc->wMaxPacketSize : 0;
	}
}

/**
 * @brief Get USB device endpoint interval.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] ep   Endpoint address
 *
 * @return Endpoint interval value, 0 if endpoint index is invalid or control endpoint.
 */
static inline uint8_t uhc_get_udev_ep_interval(struct usb_device *udev, uint8_t ep)
{
	uint8_t idx = USB_EP_GET_IDX(ep) & 0xFU;

	if (idx == 0) {
		return 0;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		return udev->pipe_in[idx].desc != NULL ? udev->pipe_in[idx].desc->bInterval : 0;
	} else {
		return udev->pipe_out[idx].desc != NULL ? udev->pipe_out[idx].desc->bInterval : 0;
	}
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
