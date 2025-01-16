/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_DEVICE_H
#define ZEPHYR_INCLUDE_USBH_DEVICE_H

#include <stdint.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/drivers/usb/uhc.h>

/* Callback type to be used for e.g. synchronous requests */
typedef int (*usbh_udev_cb_t)(struct usb_device *const udev,
			      struct uhc_transfer *const xfer);

/*
 * This will return the first available USB device; for a single-point
 * connection without hub support, this is the device connected directly to the
 * host controller.
 */
struct usb_device *usbh_device_get_any(struct usbh_contex *const ctx);

/* Allocate/free USB device */
struct usb_device *usbh_device_alloc(struct usbh_contex *const uhs_ctx);
void usbh_device_free(struct usb_device *const udev);

/* Reset and configure new USB device */
int usbh_device_init(struct usb_device *const udev);

/* Set USB device interface alternate */
int usbh_device_interface_set(struct usb_device *const udev,
			      const uint8_t iface, const uint8_t alt,
			      const bool dry);

/* Wrappers around to avoid glue UHC calls. */
static inline struct uhc_transfer *usbh_xfer_alloc(struct usb_device *udev,
						   const uint8_t ep,
						   usbh_udev_cb_t cb,
						   void *const cb_priv)
{
	struct usbh_contex *const ctx = udev->ctx;

	return uhc_xfer_alloc(ctx->dev, ep, udev, cb, cb_priv);
}

static inline int usbh_xfer_buf_add(const struct usb_device *udev,
				    struct uhc_transfer *const xfer,
				    struct net_buf *buf)
{
	struct usbh_contex *const ctx = udev->ctx;

	return uhc_xfer_buf_add(ctx->dev, xfer, buf);
}

static inline struct net_buf *usbh_xfer_buf_alloc(struct usb_device *udev,
						  const size_t size)
{
	struct usbh_contex *const ctx = udev->ctx;

	return uhc_xfer_buf_alloc(ctx->dev, size);
}

static inline int usbh_xfer_free(const struct usb_device *udev,
				 struct uhc_transfer *const xfer)
{
	struct usbh_contex *const ctx = udev->ctx;

	return uhc_xfer_free(ctx->dev, xfer);
}

static inline void usbh_xfer_buf_free(const struct usb_device *udev,
				      struct net_buf *const buf)
{
	struct usbh_contex *const ctx = udev->ctx;

	uhc_xfer_buf_free(ctx->dev, buf);
}

static inline int usbh_xfer_enqueue(const struct usb_device *udev,
				    struct uhc_transfer *const xfer)
{
	struct usbh_contex *const ctx = udev->ctx;

	return uhc_ep_enqueue(ctx->dev, xfer);
}

static inline int usbh_xfer_dequeue(const struct usb_device *udev,
				    struct uhc_transfer *const xfer)
{
	struct usbh_contex *const ctx = udev->ctx;

	return uhc_ep_dequeue(ctx->dev, xfer);
}

#endif /* ZEPHYR_INCLUDE_USBH_DEVICE_H */
