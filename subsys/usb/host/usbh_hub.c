/*
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/sys/byteorder.h>

#include "usbh_device.h"
#include "usbh_ch9.h"
#include "usbh_hub.h"

LOG_MODULE_REGISTER(usbh_hub, CONFIG_USBH_LOG_LEVEL);

static int usbh_req_clear_feature_hub(struct usb_device *const udev, const uint8_t feature)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_DEVICE;
	const uint8_t bRequest = USB_HCREQ_CLEAR_FEATURE;
	const uint16_t wValue = feature;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, 0, 0,
			      NULL);
}

static int usbh_req_set_feature_port(struct usb_device *const udev,
				     const uint8_t port_number, const uint8_t feature)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER;
	const uint8_t bRequest = USB_HCREQ_SET_FEATURE;
	const uint16_t wValue = feature;
	const uint16_t wIndex = port_number;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

static int usbh_req_clear_feature_port(struct usb_device *const udev,
				       const uint8_t port_number, const uint8_t feature)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_DEVICE << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_OTHER;
	const uint8_t bRequest = USB_HCREQ_CLEAR_FEATURE;
	const uint16_t wValue = feature;
	const uint16_t wIndex = port_number;

	return usbh_req_setup(udev,
			      bmRequestType, bRequest, wValue, wIndex, 0,
			      NULL);
}

static int usbh_hub_get_status_common(struct usb_device *const udev,
				      const uint8_t recipient, const uint16_t wIndex,
				      uint16_t *const wStatus, uint16_t *const wChange)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_HOST << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      recipient;
	const uint8_t bRequest = USB_HCREQ_GET_STATUS;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, 4);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for status");
		return -ENOMEM;
	}

	ret = usbh_req_setup(udev,
			     bmRequestType, bRequest, 0, wIndex, 4,
			     buf);

	if ((ret != 0) || (buf->len < 4)) {
		LOG_ERR("Failed to get status or insufficient data (ret=%d, len=%d)", ret,
			buf ? buf->len : 0);
		*wStatus = 0;
		*wChange = 0;
		goto cleanup;
	}

	*wStatus = net_buf_pull_le16(buf);
	*wChange = net_buf_pull_le16(buf);
	LOG_DBG("Status: wStatus=0x%04x, wChange=0x%04x", *wStatus, *wChange);

cleanup:
	usbh_xfer_buf_free(udev, buf);
	return ret;
}

void usbh_hub_init_instance(struct usb_hub *const hub_instance,
			    struct usb_device *const udev)
{
	memset(hub_instance, 0, sizeof(*hub_instance));

	hub_instance->udev = udev;
}

void usbh_hub_cleanup_instance(struct usb_hub *hub_instance)
{
	memset(hub_instance, 0, sizeof(*hub_instance));
}

int usbh_req_desc_hub(struct usb_device *const udev,
		      uint8_t *const buffer,
		      const uint16_t len)
{
	const uint8_t bmRequestType = USB_REQTYPE_DIR_TO_HOST << 7 |
				      USB_REQTYPE_TYPE_CLASS << 5 |
				      USB_REQTYPE_RECIPIENT_DEVICE;
	const uint8_t bRequest = USB_HCREQ_GET_DESCRIPTOR;
	const uint16_t wValue = USB_HUB_DESCRIPTOR_TYPE << 8;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, len);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for hub descriptor");
		return -ENOMEM;
	}

	ret = usbh_req_setup(udev,
			     bmRequestType, bRequest, wValue, 0, len,
			     buf);
	if ((ret != 0) || (buf->len == 0)) {
		LOG_ERR("Failed to get hub descriptor");
		goto cleanup;
	}

	memcpy(buffer, buf->data, MIN(len, buf->len));

cleanup:
	usbh_xfer_buf_free(udev, buf);
	return ret;
}

/* Hub features */
int usbh_req_clear_hcfs_c_hub_local_power(struct usb_device *const udev)
{
	return usbh_req_clear_feature_hub(udev, USB_HCFS_C_HUB_LOCAL_POWER);
}

int usbh_req_clear_hcfs_c_hub_over_current(struct usb_device *const udev)
{
	return usbh_req_clear_feature_hub(udev, USB_HCFS_C_HUB_OVER_CURRENT);
}

/* Port features */
int usbh_req_set_hcfs_penable(struct usb_device *const udev,
			      const uint8_t port)
{
	return usbh_req_set_feature_port(udev, port, USB_HCFS_PORT_ENABLE);
}

int usbh_req_clear_hcfs_penable(struct usb_device *const udev,
				const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_PORT_ENABLE);
}

int usbh_req_set_hcfs_psuspend(struct usb_device *const udev,
			       const uint8_t port)
{
	return usbh_req_set_feature_port(udev, port, USB_HCFS_PORT_SUSPEND);
}

int usbh_req_clear_hcfs_psuspend(struct usb_device *const udev,
				 const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_PORT_SUSPEND);
}

int usbh_req_set_hcfs_prst(struct usb_device *const udev,
			   const uint8_t port)
{
	return usbh_req_set_feature_port(udev, port, USB_HCFS_PORT_RESET);
}

int usbh_req_clear_hcfs_prst(struct usb_device *const udev,
			     const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_PORT_RESET);
}

int usbh_req_set_hcfs_ppwr(struct usb_device *const udev,
			   const uint8_t port)
{
	return usbh_req_set_feature_port(udev, port, USB_HCFS_PORT_POWER);
}

int usbh_req_clear_hcfs_ppwr(struct usb_device *const udev,
			     const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_PORT_POWER);
}

/* Port change features */
int usbh_req_clear_hcfs_c_pconnection(struct usb_device *const udev,
				      const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_C_PORT_CONNECTION);
}

int usbh_req_clear_hcfs_c_penable(struct usb_device *const udev,
				  const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_C_PORT_ENABLE);
}

int usbh_req_clear_hcfs_c_psuspend(struct usb_device *const udev,
				   const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_C_PORT_SUSPEND);
}

int usbh_req_clear_hcfs_c_pover_current(struct usb_device *const udev,
					const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_C_PORT_OVER_CURRENT);
}

int usbh_req_clear_hcfs_c_preset(struct usb_device *const udev,
				 const uint8_t port)
{
	return usbh_req_clear_feature_port(udev, port, USB_HCFS_C_PORT_RESET);
}

int usbh_req_set_hcfs_ptest(struct usb_device *const udev,
			    const uint8_t port)
{
	return usbh_req_set_feature_port(udev, port, USB_HCFS_PORT_TEST);
}

int usbh_req_set_hcfs_pindicator(struct usb_device *const udev,
				 const uint8_t port)
{
	return usbh_req_set_feature_port(udev, port, USB_HCFS_PORT_INDICATOR);
}

/* Get port status */
int usbh_req_get_port_status(struct usb_device *const udev,
			     const uint8_t port_number, uint16_t *const wPortStatus,
			     uint16_t *const wPortChange)
{
	LOG_DBG("Querying wPortStatus and wPortChange for port %d", port_number);

	return usbh_hub_get_status_common(udev, USB_REQTYPE_RECIPIENT_OTHER, port_number,
					  wPortStatus, wPortChange);
}

/* Get hub status */
int usbh_req_get_hub_status(struct usb_device *const udev,
			    uint16_t *const wHubStatus,
			    uint16_t *const wHubChange)
{
	LOG_DBG("Querying wPortStatus and wPortChange for hub");

	return usbh_hub_get_status_common(udev, USB_REQTYPE_RECIPIENT_DEVICE, 0,
					  wHubStatus, wHubChange);
}
