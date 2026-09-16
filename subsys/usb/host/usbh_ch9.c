/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usb_hub.h>
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
	/*
	 * One global binary sem for all EP0 control completions. A late callback
	 * after timeout/dequeue, or any double-completion bug, can leave count=1.
	 * The next wait would return immediately while the new xfer is still
	 * in flight (stale xfer->err, early usbh_xfer_free) — typical symptom:
	 * logs stop right after a successful control transfer.
	 */
	while (k_sem_take(&ch9_req_sync, K_NO_WAIT) == 0) {
		LOG_WRN("ch9: drained stray control-transfer sync token before req0x%02x",
			bRequest);
	}

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

	LOG_DBG("usbh: ch9: waiting xfer (req=0x%02x wLength=%u)", bRequest, wLength);

	if (k_sem_take(&ch9_req_sync, K_MSEC(SETUP_REQ_TIMEOUT)) != 0) {
		/*
		 * ep_enqueue blocks until the TD completes; this timeout means the
		 * usbh_thread callback did not run. Try Stop EP cancel if still active.
		 */
		(void)usbh_xfer_dequeue(udev, xfer);
		LOG_ERR("ch9: timeout waiting for completion callback (req=0x%02x)", bRequest);
		ret = -ETIMEDOUT;
		goto buf_alloc_err;
	}

	ret = xfer->err;
	LOG_DBG("usbh: ch9: xfer done (req=0x%02x err=%d)", bRequest, ret);

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
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
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

#if IS_ENABLED(CONFIG_USBH_HUB)

static uint8_t hub_req_bm(const uint8_t recipient, const bool to_host)
{
	return (to_host ? USB_REQTYPE_DIR_TO_HOST : USB_REQTYPE_DIR_TO_DEVICE) << 7 |
	       USB_REQTYPE_TYPE_CLASS << 5 | recipient;
}

int usbh_req_get_hcfs_port_status(struct usb_device *const hub, const uint8_t port,
				  uint16_t *const portstatus, uint16_t *const portchange)
{
	struct usb_port_status ps;
	struct net_buf *buf;
	int ret;

	if (hub == NULL || portstatus == NULL || portchange == NULL) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub, sizeof(ps));
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_setup(hub, hub_req_bm(USB_REQTYPE_RECIPIENT_OTHER, true),
			     USB_HCREQ_GET_STATUS, 0, port, sizeof(ps), buf);
	if (ret == 0 && buf->len >= sizeof(ps)) {
		memcpy(&ps, buf->data, sizeof(ps));
		*portstatus = sys_le16_to_cpu(ps.wPortStatus);
		*portchange = sys_le16_to_cpu(ps.wPortChange);
	}

	usbh_xfer_buf_free(hub, buf);

	return ret;
}

int usbh_req_get_hcfs_hub_status(struct usb_device *const hub, uint16_t *const hubstatus,
				 uint16_t *const hubchange)
{
	struct usb_hub_status hs;
	struct net_buf *buf;
	int ret;

	if (hub == NULL || hubstatus == NULL || hubchange == NULL) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub, sizeof(hs));
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_setup(hub, hub_req_bm(USB_REQTYPE_RECIPIENT_DEVICE, true),
			     USB_HCREQ_GET_STATUS, 0, 0, sizeof(hs), buf);
	if (ret == 0 && buf->len >= sizeof(hs)) {
		memcpy(&hs, buf->data, sizeof(hs));
		*hubstatus = sys_le16_to_cpu(hs.wHubStatus);
		*hubchange = sys_le16_to_cpu(hs.wHubChange);
	}

	usbh_xfer_buf_free(hub, buf);

	return ret;
}

int usbh_req_set_hcfs_port_feature(struct usb_device *const hub, const uint8_t port,
				   const uint8_t feature)
{
	return usbh_req_setup(hub, hub_req_bm(USB_REQTYPE_RECIPIENT_OTHER, false),
			      USB_HCREQ_SET_FEATURE, feature, port, 0, NULL);
}

int usbh_req_clear_hcfs_port_feature(struct usb_device *const hub, const uint8_t port,
				     const uint8_t feature)
{
	return usbh_req_setup(hub, hub_req_bm(USB_REQTYPE_RECIPIENT_OTHER, false),
			      USB_HCREQ_CLEAR_FEATURE, feature, port, 0, NULL);
}

int usbh_req_clear_hcfs_hub_feature(struct usb_device *const hub, const uint8_t feature)
{
	return usbh_req_setup(hub, hub_req_bm(USB_REQTYPE_RECIPIENT_DEVICE, false),
			      USB_HCREQ_CLEAR_FEATURE, feature, 0, 0, NULL);
}

int usbh_req_desc_hub(struct usb_device *const hub, const uint16_t len, void *const desc)
{
	const uint8_t bmRequestType = hub_req_bm(USB_REQTYPE_RECIPIENT_DEVICE, true);
	const uint8_t bRequest = USB_HCREQ_GET_DESCRIPTOR;
	const uint16_t wValue = (USB_DT_HUB << 8);
	struct net_buf *buf;
	int ret;

	if (hub == NULL || desc == NULL || len == 0U) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub, len);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_setup(hub, bmRequestType, bRequest, wValue, 0, len, buf);
	if (ret == 0 && buf->len > 0U) {
		memcpy(desc, buf->data, MIN((size_t)buf->len, (size_t)len));
	}

	usbh_xfer_buf_free(hub, buf);

	return ret;
}

int usbh_hub_port_reset(struct usb_device *const hub, const uint8_t port, const uint32_t reset_ms)
{
	uint16_t portstatus;
	uint16_t portchange;
	int ret;
	unsigned int tries;

	if (hub == NULL || port == 0U) {
		return -EINVAL;
	}

	ret = usbh_req_set_hcfs_port_feature(hub, port, USB_HCFS_PORT_RESET);
	if (ret != 0) {
		return ret;
	}

	if (reset_ms > 0U) {
		k_msleep(reset_ms);
	}

	for (tries = 0U; tries < 20U; tries++) {
		ret = usbh_req_get_hcfs_port_status(hub, port, &portstatus, &portchange);
		if (ret != 0) {
			return ret;
		}

		if ((portchange & USB_PORT_STAT_C_RESET) != 0U) {
			break;
		}

		k_msleep(10);
	}

	return usbh_req_clear_hcfs_port_feature(hub, port, USB_HCFS_C_PORT_RESET);
}

#endif /* CONFIG_USBH_HUB */

int usbh_req_set_hcfs_ppwr(struct usb_device *const udev,
			   const uint8_t port)
{
#if IS_ENABLED(CONFIG_USBH_HUB)
	return usbh_req_set_hcfs_port_feature(udev, port, USB_HCFS_PORT_POWER);
#else
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 | USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER << 0;
	const uint8_t bRequest = USB_HCREQ_SET_FEATURE;
	const uint16_t wValue = USB_HCFS_PORT_POWER;
	const uint16_t wIndex = port;

	return usbh_req_setup(udev, bmRequestType, bRequest, wValue, wIndex, 0, NULL);
#endif
}

int usbh_req_set_hcfs_prst(struct usb_device *const udev,
			   const uint8_t port)
{
#if IS_ENABLED(CONFIG_USBH_HUB)
	return usbh_req_set_hcfs_port_feature(udev, port, USB_HCFS_PORT_RESET);
#else
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 | USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER << 0;
	const uint8_t bRequest = USB_HCREQ_SET_FEATURE;
	const uint16_t wValue = USB_HCFS_PORT_RESET;
	const uint16_t wIndex = port;

	return usbh_req_setup(udev, bmRequestType, bRequest, wValue, wIndex, 0, NULL);
#endif
}
