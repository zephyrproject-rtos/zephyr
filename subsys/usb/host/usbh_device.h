/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host device management helpers
 */

#ifndef ZEPHYR_INCLUDE_USBH_DEVICE_H
#define ZEPHYR_INCLUDE_USBH_DEVICE_H

#include <stdint.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/drivers/usb/uhc.h>

/**
 * @brief USB device transfer completion callback type.
 *
 * Used for synchronous control and bulk helpers in the host stack.
 *
 * @param udev USB device associated with the transfer
 * @param xfer Completed transfer object
 *
 * @retval 0 Continue default completion handling
 * @retval negative errno Propagate as transfer error
 */
typedef int (*usbh_udev_cb_t)(struct usb_device *const udev, struct uhc_transfer *const xfer);

/**
 * @brief Return the first connected USB device in @a ctx.
 *
 * For a single-point connection without hub support, this is the device
 * attached directly to the host controller.
 *
 * @param ctx USB host context
 *
 * @return Pointer to a connected device, or NULL if none
 */
struct usb_device *usbh_device_get_any(struct usbh_context *const ctx);

/**
 * @brief Look up a USB device by address.
 *
 * @param uhs_ctx USB host context
 * @param addr Device address assigned by the host (1–127)
 *
 * @return Matching device, or NULL if not found
 */
struct usb_device *usbh_device_get(struct usbh_context *const uhs_ctx, const uint8_t addr);

/**
 * @brief Allocate a USB device object from the host context pool.
 *
 * @param uhs_ctx USB host context
 *
 * @return New device object, or NULL when the pool is exhausted
 */
struct usb_device *usbh_device_alloc(struct usbh_context *const uhs_ctx);

/**
 * @brief Release a USB device object back to the host context pool.
 *
 * @param udev Device to free
 */
void usbh_device_free(struct usb_device *const udev);

/**
 * @brief Enumerate and configure a newly connected USB device.
 *
 * Runs bus reset, descriptor reads, SET_ADDRESS, SET_CONFIGURATION, and invokes
 * the optional HCD @ref uhc_add_endpoints hook. Holds @c udev->mutex for the
 * duration of enumeration so partially enumerated state is not visible to
 * concurrent callers. Settle and retry intervals use @c k_msleep() while the
 * mutex remains held (control transfers still complete on the host EP thread).
 *
 * @param udev Device object allocated by @ref usbh_device_alloc
 *
 * @retval 0 Device reached @ref USB_STATE_CONFIGURED
 * @retval negative errno on enumeration or configuration failure
 */
int usbh_device_init(struct usb_device *const udev);

/**
 * @brief Set USB device address (shell / debug helper).
 *
 * @param udev USB device
 * @param new_addr New address (0 returns device to default state)
 *
 * @retval 0 Address assigned (@c udev->addr holds the wire address)
 * @retval negative errno from address assignment
 */
int usbh_device_set_address(struct usb_device *const udev, const uint8_t new_addr);

/**
 * @brief Optional hook for class drivers to prefer a configuration descriptor.
 *
 * Weak default returns @c -ENOENT (no preference). Class drivers may override
 * to return 0 when @a cfg_desc is suitable (e.g. MSC bulk pair present).
 *
 * @param udev USB device (device descriptor available; not yet configured)
 * @param cfg_desc Full configuration descriptor blob
 * @param len Length of @a cfg_desc in bytes
 *
 * @retval 0 Prefer this configuration
 * @retval -ENOENT Not preferred; enumeration continues scanning
 * @retval negative errno Malformed descriptor
 */
int usbh_device_configuration_prefers(const struct usb_device *udev, const void *cfg_desc,
				      uint16_t len);

/**
 * @brief Optional weak hook: bus thread calls this when enumeration reaches @ref
 * USB_STATE_CONFIGURED
 *
 * Invoked from @c usbh_device_init() on success, after the device mutex is fully
 * unlocked (initial SET_CONFIGURATION + descriptor parse + HCD configure complete).
 * Override in the application to e.g. give a semaphore so preemptible @c main()
 * need not poll @c udev->mutex (cooperative @c usbh_bus_thread can starve it).
 *
 * @param udev Newly configured USB device
 */
void usbh_device_configured_notify(struct usb_device *udev);

/**
 * @brief Weak hook invoked when a USB device is disconnected.
 *
 * Called from the USB host bus thread after disconnect is detected and before
 * class drivers are torn down or the device object is freed. Override in the
 * application to unmount file systems while disk volumes are still registered,
 * or clear stale pointers to @a udev.
 *
 * @param udev Device being removed (valid only for the duration of this call)
 */
void usbh_device_removed_notify(struct usb_device *udev);

/**
 * @brief Set an interface alternate setting.
 *
 * Issues SET_INTERFACE and updates @c udev->ifaces when @a dry is false.
 *
 * @param udev USB device
 * @param iface Interface number
 * @param alt Alternate setting number
 * @param dry When true, validate only without sending the request
 *
 * @retval 0 Alternate setting selected
 * @retval negative errno from control transfer or descriptor validation
 */
int usbh_device_interface_set(struct usb_device *const udev, const uint8_t iface, const uint8_t alt,
			      const bool dry);

/**
 * @brief Allocate a UHC transfer bound to @a udev.
 *
 * @param udev USB device
 * @param ep Endpoint address including direction bit
 * @param cb Optional completion callback
 * @param cb_priv Opaque callback context
 *
 * @return Transfer object, or NULL on allocation failure
 */
static inline struct uhc_transfer *usbh_xfer_alloc(struct usb_device *udev, const uint8_t ep,
						   usbh_udev_cb_t cb, void *const cb_priv)
{
	struct usbh_context *const ctx = udev->ctx;

	return uhc_xfer_alloc(ctx->dev, ep, udev, cb, cb_priv);
}

/**
 * @brief Attach a net_buf payload to a transfer.
 *
 * @param udev USB device
 * @param xfer Transfer object
 * @param buf Payload buffer
 *
 * @retval 0 Buffer attached
 * @retval negative errno on failure
 */
static inline int usbh_xfer_buf_add(const struct usb_device *udev, struct uhc_transfer *const xfer,
				    struct net_buf *buf)
{
	struct usbh_context *const ctx = udev->ctx;

	return uhc_xfer_buf_add(ctx->dev, xfer, buf);
}

/**
 * @brief Allocate a net_buf from the UHC buffer pool.
 *
 * @param udev USB device
 * @param size Requested buffer size in bytes
 *
 * @return Allocated buffer, or NULL on failure
 */
static inline struct net_buf *usbh_xfer_buf_alloc(struct usb_device *udev, const size_t size)
{
	struct usbh_context *const ctx = udev->ctx;

	return uhc_xfer_buf_alloc(ctx->dev, size);
}

/**
 * @brief Free a transfer object.
 *
 * @param udev USB device
 * @param xfer Transfer to release
 *
 * @retval 0 Transfer freed
 * @retval negative errno on failure
 */
static inline int usbh_xfer_free(const struct usb_device *udev, struct uhc_transfer *const xfer)
{
	struct usbh_context *const ctx = udev->ctx;

	return uhc_xfer_free(ctx->dev, xfer);
}

/**
 * @brief Free a net_buf back to the UHC buffer pool.
 *
 * @param udev USB device
 * @param buf Buffer to release
 */
static inline void usbh_xfer_buf_free(const struct usb_device *udev, struct net_buf *const buf)
{
	struct usbh_context *const ctx = udev->ctx;

	uhc_xfer_buf_free(ctx->dev, buf);
}

/**
 * @brief Queue a transfer on the host controller.
 *
 * @param udev USB device
 * @param xfer Transfer to enqueue
 *
 * @retval 0 Transfer queued
 * @retval negative errno on enqueue failure
 */
static inline int usbh_xfer_enqueue(const struct usb_device *udev, struct uhc_transfer *const xfer)
{
	struct usbh_context *const ctx = udev->ctx;

	return uhc_ep_enqueue(ctx->dev, xfer);
}

/**
 * @brief Cancel a queued or in-flight transfer.
 *
 * @param udev USB device
 * @param xfer Transfer to dequeue
 *
 * @retval 0 Transfer dequeued or not active
 * @retval negative errno on failure
 */
static inline int usbh_xfer_dequeue(const struct usb_device *udev, struct uhc_transfer *const xfer)
{
	struct usbh_context *const ctx = udev->ctx;

	return uhc_ep_dequeue(ctx->dev, xfer);
}

#endif /* ZEPHYR_INCLUDE_USBH_DEVICE_H */
