/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>

#include "usbh_device.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_ch9, CONFIG_USBH_LOG_LEVEL);

/*
 * For now we set it to the upper limit defined in Chapter
 * "9.2.6.4 Standard Device Requests"
 * This will need to be revised and set depending on the request.
 */
#define SETUP_REQ_TIMEOUT	5000U

K_SEM_DEFINE(ch9_req_sync, 0, 1);
static bool ctrl_req_no_status;

static int ch9_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	LOG_DBG("Request finished %p, err %d", xfer, xfer->err);
	if (xfer->err == -ECONNRESET) {
		LOG_INF("Transfer %p cancelled", (void *)xfer);
		usbh_xfer_free(udev, xfer);

		return 0;
	}

	k_sem_give(&ch9_req_sync);

	return 0;
}

void usbh_req_omit_status(const bool omit)
{
	ctrl_req_no_status = omit;
}

int usbh_req_setup(struct usb_device *const udev,
		   const uint8_t bmRequestType,
		   const uint8_t bRequest,
		   const uint16_t wValue,
		   const uint16_t wIndex,
		   const uint16_t wLength,
		   struct net_buf *const buf)
{
	struct usb_setup_packet req = {
		.bmRequestType = bmRequestType,
		.bRequest = bRequest,
		.wValue = sys_cpu_to_le16(wValue),
		.wIndex = sys_cpu_to_le16(wIndex),
		.wLength = sys_cpu_to_le16(wLength),
	};
	struct uhc_transfer *xfer;
	uint8_t ep = usb_reqtype_is_to_device(&req) ? 0x00 : 0x80;
	int ret;

	xfer = usbh_xfer_alloc(udev, ep, ch9_req_cb, NULL);
	if (!xfer) {
		return -ENOMEM;
	}

	memcpy(xfer->setup_pkt, &req, sizeof(req));

	__ASSERT(IS_ENABLED(CONFIG_ZTEST) ||
		 (buf != NULL && wLength) || (buf == NULL && !wLength),
		 "Unresolved conditions for data stage");
	if (wLength) {
		ret = usbh_xfer_buf_add(udev, xfer, buf);
		if (ret) {
			goto buf_alloc_err;
		}
	}

	if (IS_ENABLED(CONFIG_ZTEST) && ctrl_req_no_status) {
		xfer->no_status = true;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret) {
		goto buf_alloc_err;
	}

	if (k_sem_take(&ch9_req_sync, K_MSEC(SETUP_REQ_TIMEOUT)) != 0) {
		ret = usbh_xfer_dequeue(udev, xfer);
		if (ret != 0) {
			LOG_ERR("Failed to cancel transfer");
			return ret;
		}

		LOG_ERR("Timeout");
		return -ETIMEDOUT;
	}

	ret = xfer->err;

buf_alloc_err:
	usbh_xfer_free(udev, xfer);

	return ret;
}

int usbh_req_desc(struct usb_device *const udev,
		  const uint8_t type, const uint8_t index,
		  const uint16_t id,
		  const uint16_t len,
		  struct net_buf *const buf)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_HOST << 7;
	const uint8_t bRequest = USB_SREQ_GET_DESCRIPTOR;
	const uint16_t wValue = (type << 8) | index;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, id, len,
			      buf);
}

int usbh_req_desc_dev(struct usb_device *const udev,
		      const uint16_t len,
		      struct usb_device_descriptor *const desc)
{
	const uint8_t type = USB_DESC_DEVICE;
	const uint16_t wLength = MIN(len, sizeof(struct usb_device_descriptor));
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, wLength);
	if (!buf) {
		return -ENOMEM;
	}

	ret = usbh_req_desc(udev, type, 0, 0, wLength, buf);
	if (ret == 0 && buf->len == wLength) {
		memcpy(desc, buf->data, wLength);
		desc->bcdUSB = sys_le16_to_cpu(desc->bcdUSB);
		desc->idVendor = sys_le16_to_cpu(desc->idVendor);
		desc->idProduct = sys_le16_to_cpu(desc->idProduct);
		desc->bcdDevice = sys_le16_to_cpu(desc->bcdDevice);
	}

	usbh_xfer_buf_free(udev, buf);

	return ret;
}

int usbh_req_desc_cfg(struct usb_device *const udev,
		      const uint8_t index,
		      const uint16_t len,
		      struct usb_cfg_descriptor *const desc)
{
	const uint8_t type = USB_DESC_CONFIGURATION;
	const uint16_t wLength = len;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, len);
	if (!buf) {
		return -ENOMEM;
	}

	ret = usbh_req_desc(udev, type, index, 0, wLength, buf);
	if (ret == 0) {
		memcpy(desc, buf->data, len);
		desc->wTotalLength = sys_le16_to_cpu(desc->wTotalLength);
	}

	usbh_xfer_buf_free(udev, buf);

	return ret;
}

int usbh_req_set_address(struct usb_device *const udev,
			 const uint8_t addr)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_SET_ADDRESS;

	return usbh_req_setup(udev, bmRequestType, bRequest, addr, 0, 0, NULL);
}

int usbh_req_set_cfg(struct usb_device *const udev,
		     const uint8_t cfg)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_SET_CONFIGURATION;

	return usbh_req_setup(udev, bmRequestType, bRequest, cfg, 0, 0, NULL);
}

int usbh_req_get_cfg(struct usb_device *const udev,
		     uint8_t *const cfg)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_HOST << 7;
	const uint8_t bRequest = USB_SREQ_GET_CONFIGURATION;
	const uint16_t wLength = 1;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, wLength);
	if (!buf) {
		return -ENOMEM;
	}

	ret = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, buf);
	if (ret == 0 && buf->len == wLength) {
		*cfg = buf->data[0];
	}

	usbh_xfer_buf_free(udev, buf);

	return ret;
}

int usbh_req_set_alt(struct usb_device *const udev,
		     const uint8_t iface, const uint8_t alt)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_RECIPIENT_INTERFACE;
	const uint8_t bRequest = USB_SREQ_SET_INTERFACE;
	const uint16_t wValue = alt;
	const uint16_t wIndex = iface;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

int usbh_req_set_sfs_rwup(struct usb_device *const udev)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_SET_FEATURE;
	const uint16_t wValue = USB_SFS_REMOTE_WAKEUP;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, 0, 0,
			      NULL);
}

int usbh_req_clear_sfs_rwup(struct usb_device *const udev)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_CLEAR_FEATURE;
	const uint16_t wValue = USB_SFS_REMOTE_WAKEUP;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, 0, 0,
			      NULL);
}

int usbh_req_set_sfs_halt(struct usb_device *const udev, const uint8_t ep)
{
	const uint8_t bmRequestType = USB_REQTYPE_RECIPIENT_ENDPOINT;
	const uint8_t bRequest = USB_SREQ_SET_FEATURE;
	const uint16_t wValue = USB_SFS_ENDPOINT_HALT;
	const uint16_t wIndex = ep;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

int usbh_req_clear_sfs_halt(struct usb_device *const udev, const uint8_t ep)
{
	const uint8_t bmRequestType = USB_REQTYPE_RECIPIENT_ENDPOINT;
	const uint8_t bRequest = USB_SREQ_CLEAR_FEATURE;
	const uint16_t wValue = USB_SFS_ENDPOINT_HALT;
	const uint16_t wIndex = ep;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

int usbh_req_set_hcfs_ppwr(struct usb_device *const udev,
			   const uint8_t port)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER << 0;
	const uint8_t bRequest = USB_HCREQ_SET_FEATURE;
	const uint16_t wValue = USB_HCFS_PORT_POWER;
	const uint16_t wIndex = port;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

int usbh_req_set_hcfs_prst(struct usb_device *const udev,
			   const uint8_t port)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER << 0;
	const uint8_t bRequest = USB_HCREQ_SET_FEATURE;
	const uint16_t wValue = USB_HCFS_PORT_RESET;
	const uint16_t wIndex = port;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

int usbh_req_desc_bos(struct usb_device *udev, size_t len,
		      struct usb_bos_descriptor *const bos_desc)
{
	struct net_buf *netbuf;
	int ret;

	if (len < sizeof(*bos_desc)) {
		return -EINVAL;
	}

	netbuf = usbh_xfer_buf_alloc(udev, len);
	if (netbuf == NULL) {
		return -ENOMEM;
	}
	ret = usbh_req_desc(udev, USB_DESC_BOS, 0, 0, len, netbuf);
	if (ret == 0) {
		if (netbuf->len == len) {
			memcpy(bos_desc, netbuf->data, len);
			bos_desc->wTotalLength = sys_le16_to_cpu(bos_desc->wTotalLength);
			if ((bos_desc->bLength != sizeof(*bos_desc)) ||
			    (bos_desc->wTotalLength < sizeof(*bos_desc))) {
				ret = -EBADMSG;
			}
		} else {
			ret = -EBADMSG;
		}
	}
	usbh_xfer_buf_free(udev, netbuf);

	return ret;
}

int usbh_req_desc_string(struct usb_device *udev, size_t len,
			 struct usb_string_descriptor *const str_desc, uint8_t str_id,
			 uint16_t lang_code)
{
	/* NOTE: sizeof(usb_string_descriptor) cannot be used */
	const size_t string_hdr_len = 2;
	struct net_buf *netbuf;
	int ret = 0;

	if (len < string_hdr_len) {
		return -EINVAL;
	}

	netbuf = usbh_xfer_buf_alloc(udev, len);
	if (netbuf == NULL) {
		return -ENOMEM;
	}
	ret = usbh_req_desc(udev, USB_DESC_STRING, str_id, lang_code, len, netbuf);
	if (ret == 0) {
		if (netbuf->len == len) {
			memcpy(str_desc, netbuf->data, len);
			if (str_desc->bLength < string_hdr_len) {
				ret = -EBADMSG;
			}
		} else {
			ret = -EBADMSG;
		}
	}
	usbh_xfer_buf_free(udev, netbuf);

	return ret;
}
