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

/* USB device state */
enum usb_device_state {
	USB_STATE_NOTCONNECTED,
	USB_STATE_DEFAULT,
	USB_STATE_ADDRESSED,
	USB_STATE_CONFIGURED,
};

/* Host support view of a USB device */
struct usb_device {
	struct usbh_contex *ctx;
	struct usb_device_descriptor dev_desc;
	enum usb_device_state state;
	uint8_t actual_cfg;
	uint8_t addr;
};

/* Callback type to be used for e.g. synchronous requests */
typedef int (*usbh_udev_cb_t)(struct usb_device *const udev,
			      struct uhc_transfer *const xfer);

/*
 * Get a device to work on, there will only be one for the first time
 * until we implement USB device configuration/management.
 */
struct usb_device *usbh_device_get_any(struct usbh_contex *const ctx);

/* Wrappers around to avoid glue UHC calls. */
static inline struct uhc_transfer *usbh_xfer_alloc(struct usb_device *udev,
						   const uint8_t ep,
						   const uint8_t attrib,
						   const uint16_t mps,
						   const uint16_t timeout,
						   usbh_udev_cb_t *const cb)
{
	struct usbh_contex *const ctx = udev->ctx;

	return uhc_xfer_alloc(ctx->dev, udev->addr, ep, attrib, mps, timeout, udev, cb);
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

#endif /* ZEPHYR_INCLUDE_USBH_DEVICE_H */
