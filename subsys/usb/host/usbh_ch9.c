/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_ch9, CONFIG_USBH_LOG_LEVEL);

#define SETUP_REQ_TIMEOUT	1000U

int usbh_req_setup(const struct device *dev,
		   const uint8_t addr,
		   const uint8_t bmRequestType,
		   const uint8_t bRequest,
		   const uint16_t wValue,
		   const uint16_t wIndex,
		   const uint16_t wLength,
		   uint8_t *const data)
{
	struct usb_setup_packet req = {
		.bmRequestType = bmRequestType,
		.bRequest = bRequest,
		.wValue = sys_cpu_to_le16(wValue),
		.wIndex = sys_cpu_to_le16(wIndex),
		.wLength = sys_cpu_to_le16(wLength),
	};
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	uint8_t ep = usb_reqtype_is_to_device(&req) ? 0x00 : 0x80;
	int ret;

	xfer = uhc_xfer_alloc(dev, addr, ep, 0, 64, SETUP_REQ_TIMEOUT, NULL);
	if (!xfer) {
		return -ENOMEM;
	}

	buf = uhc_xfer_buf_alloc(dev, xfer, sizeof(req));
	if (!buf) {
		ret = -ENOMEM;
		goto buf_alloc_err;
	}

	net_buf_add_mem(buf, &req, sizeof(req));

	if (wLength) {
		buf = uhc_xfer_buf_alloc(dev, xfer, wLength);
		if (!buf) {
			ret = -ENOMEM;
			goto buf_alloc_err;
		}

		if (usb_reqtype_is_to_device(&req) && data != NULL) {
			net_buf_add_mem(buf, data, wLength);
		}
	}

	buf = uhc_xfer_buf_alloc(dev, xfer, 0);
	if (!buf) {
		ret = -ENOMEM;
		goto buf_alloc_err;
	}

	return uhc_ep_enqueue(dev, xfer);

buf_alloc_err:
	uhc_xfer_free(dev, xfer);

	return ret;
}

int usbh_req_desc(const struct device *dev,
		  const uint8_t addr,
		  const uint8_t type, const uint8_t index,
		  const uint8_t id,
		  const uint16_t len)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_HOST << 7;
	const uint8_t bRequest = USB_SREQ_GET_DESCRIPTOR;
	const uint16_t wValue = (type << 8) | index;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, wValue, id, len,
			      NULL);
}

int usbh_req_desc_dev(const struct device *dev,
		      const uint8_t addr)
{
	const uint8_t type = USB_DESC_DEVICE;
	const uint16_t wLength = 18;

	return usbh_req_desc(dev, addr, type, 0, 0, wLength);
}

int usbh_req_desc_cfg(const struct device *dev,
		      const uint8_t addr,
		      const uint8_t index,
		      const uint16_t len)
{
	const uint8_t type = USB_DESC_CONFIGURATION;

	return usbh_req_desc(dev, addr, type, index, 0, len);
}

int usbh_req_set_address(const struct device *dev,
			 const uint8_t addr, const uint8_t new)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_SET_ADDRESS;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, new, 0, 0,
			      NULL);
}

int usbh_req_set_cfg(const struct device *dev,
		     const uint8_t addr, const uint8_t new)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_SET_CONFIGURATION;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, new, 0, 0,
			      NULL);
}

int usbh_req_set_alt(const struct device *dev,
		     const uint8_t addr, const uint8_t iface,
		     const uint8_t alt)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_RECIPIENT_INTERFACE;
	const uint8_t bRequest = USB_SREQ_SET_INTERFACE;
	const uint16_t wValue = alt;
	const uint16_t wIndex = iface;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

int usbh_req_set_sfs_rwup(const struct device *dev,
			  const uint8_t addr)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_SET_FEATURE;
	const uint16_t wValue = USB_SFS_REMOTE_WAKEUP;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, wValue, 0, 0,
			      NULL);
}

int usbh_req_clear_sfs_rwup(const struct device *dev,
			    const uint8_t addr)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7;
	const uint8_t bRequest = USB_SREQ_CLEAR_FEATURE;
	const uint16_t wValue = USB_SFS_REMOTE_WAKEUP;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, wValue, 0, 0,
			      NULL);
}

int usbh_req_set_hcfs_ppwr(const struct device *dev,
			   const uint8_t addr, const uint8_t port)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER << 0;
	const uint8_t bRequest = USB_HCREQ_SET_FEATURE;
	const uint16_t wValue = USB_HCFS_PORT_POWER;
	const uint16_t wIndex = port;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

int usbh_req_set_hcfs_prst(const struct device *dev,
			   const uint8_t addr, const uint8_t port)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER << 0;
	const uint8_t bRequest = USB_HCREQ_SET_FEATURE;
	const uint16_t wValue = USB_HCFS_PORT_RESET;
	const uint16_t wIndex = port;

	return usbh_req_setup(dev, addr,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}
