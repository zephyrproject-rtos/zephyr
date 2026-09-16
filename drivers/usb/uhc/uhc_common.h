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
 * @brief Get USB device endpoint maximum packet size.
 *
 * The value is the raw bMaxPacketSize field of the endpoint descriptor, or
 * bMaxPacketSize0 of the device descriptor for endpoint 0.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] ep   Endpoint address
 *
 * @return Maximum packet size of the endpoint, 0 if it is not configured.
 */
static inline uint16_t uhc_get_udev_ep_mps(struct usb_device *const udev, const uint8_t ep)
{
	struct usb_host_pipe *pipe = uhc_get_udev_pipe(udev, ep);

	if (USB_EP_GET_IDX(ep) == 0U) {
		return pipe->control_mps;
	}

	return pipe->desc != NULL ? pipe->desc->wMaxPacketSize : 0U;
}

/**
 * @brief Get USB device endpoint interval.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] ep   Endpoint address
 *
 * @return Endpoint interval, 0 for a control endpoint or if it is not
 *         configured.
 */
static inline uint8_t uhc_get_udev_ep_interval(struct usb_device *const udev, const uint8_t ep)
{
	struct usb_host_pipe *pipe = uhc_get_udev_pipe(udev, ep);

	if (USB_EP_GET_IDX(ep) == 0U) {
		return 0U;
	}

	return pipe->desc != NULL ? pipe->desc->bInterval : 0U;
}

/**
 * @brief Get USB device endpoint transfer type.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] ep   Endpoint address
 *
 * @return Endpoint transfer type, USB_EP_TYPE_CONTROL for endpoint 0.
 */
static inline uint8_t uhc_get_udev_ep_type(struct usb_device *const udev, const uint8_t ep)
{
	struct usb_host_pipe *pipe = uhc_get_udev_pipe(udev, ep);

	if ((USB_EP_GET_IDX(ep) == 0U) || (pipe->desc == NULL)) {
		return USB_EP_TYPE_CONTROL;
	}

	return pipe->desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;
}

/**
 * @brief Get USB device endpoint address by pipe.
 *
 * @param[in] udev Pointer to USB device instance
 * @param[in] pipe Pointer to the pipe of the endpoint
 *
 * @return Endpoint address on success, negative errno code on error.
 * @retval -EINVAL Pipe does not belong to the device
 */
static inline int uhc_get_udev_ep(struct usb_device *const udev,
				  const struct usb_host_pipe *const pipe)
{
	if (IS_ARRAY_ELEMENT(udev->pipe_in, pipe)) {
		return (int)ARRAY_INDEX(udev->pipe_in, pipe) | USB_EP_DIR_IN;
	}

	if (IS_ARRAY_ELEMENT(udev->pipe_out, pipe)) {
		return (int)ARRAY_INDEX(udev->pipe_out, pipe) | USB_EP_DIR_OUT;
	}

	return -EINVAL;
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
