/*
 * Copyright 2025 - 2026 NXP
 *
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

/* Common hub control request function */
static int usbh_hub_class_request_common(struct usbh_hub_instance *hub_instance,
					 uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue,
					 uint16_t wIndex, struct net_buf *buf)
{
	int ret;

	if (!hub_instance || !hub_instance->hub_udev || !buf) {
		return -EINVAL;
	}

	k_mutex_lock(&hub_instance->ctrl_lock, K_FOREVER);

	/* Execute synchronous control transfer */
	ret = usbh_req_setup(hub_instance->hub_udev, bmRequestType, bRequest, wValue, wIndex,
			     buf->size, buf);

	if (ret < 0) {
		LOG_ERR("Hub control request failed: %d", ret);
	} else {
		LOG_DBG("Hub control request completed: type=0x%02x, req=0x%02x, len=%d",
			bmRequestType, bRequest, buf->len);
	}

	k_mutex_unlock(&hub_instance->ctrl_lock);
	return ret;
}

int usbh_hub_get_descriptor(struct usbh_hub_instance *hub_instance, uint8_t *buffer,
			    uint16_t wLength)
{
	struct net_buf *buf;
	int ret;

	if (!hub_instance || !buffer || wLength == 0) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, wLength);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for hub descriptor");
		return -ENOMEM;
	}

	ret = usbh_hub_class_request_common(
		hub_instance,
		(USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
			(USB_REQTYPE_RECIPIENT_DEVICE << 0),
		USB_HUB_REQ_GET_DESCRIPTOR, (USB_HUB_DESCRIPTOR_TYPE << 8), 0, buf);

	if ((ret != 0) || (buf->len == 0)) {
		LOG_ERR("Failed to get hub descriptor");
		goto error_process;
	}

	memcpy(buffer, buf->data, MIN(wLength, buf->len));

error_process:
	usbh_xfer_buf_free(hub_instance->hub_udev, buf);
	return ret;
}

int usbh_hub_clear_hub_feature(struct usbh_hub_instance *hub_instance, uint8_t feature)
{
	struct net_buf *buf;
	int ret;

	if (!hub_instance) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, 0);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for clear hub feature");
		return -ENOMEM;
	}

	ret = usbh_hub_class_request_common(hub_instance,
					    (USB_REQTYPE_DIR_TO_DEVICE << 7) |
						    (USB_REQTYPE_TYPE_CLASS << 5) |
						    (USB_REQTYPE_RECIPIENT_DEVICE << 0),
					    USB_HUB_REQ_CLEAR_FEATURE, feature, 0, buf);

	usbh_xfer_buf_free(hub_instance->hub_udev, buf);
	return ret;
}

int usbh_hub_set_port_feature(struct usbh_hub_instance *hub_instance, uint8_t port_number,
			      uint8_t feature)
{
	struct net_buf *buf;
	int ret;

	if (!hub_instance) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, 0);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for set port feature");
		return -ENOMEM;
	}

	ret = usbh_hub_class_request_common(hub_instance,
					    (USB_REQTYPE_DIR_TO_DEVICE << 7) |
						    (USB_REQTYPE_TYPE_CLASS << 5) |
						    (USB_REQTYPE_RECIPIENT_OTHER << 0),
					    USB_HUB_REQ_SET_FEATURE, feature, port_number, buf);

	usbh_xfer_buf_free(hub_instance->hub_udev, buf);
	return ret;
}

int usbh_hub_clear_port_feature(struct usbh_hub_instance *hub_instance, uint8_t port_number,
				uint8_t feature)
{
	struct net_buf *buf;
	int ret;

	if (!hub_instance) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, 0);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for clear port feature");
		return -ENOMEM;
	}

	ret = usbh_hub_class_request_common(hub_instance,
					    (USB_REQTYPE_DIR_TO_DEVICE << 7) |
						    (USB_REQTYPE_TYPE_CLASS << 5) |
						    (USB_REQTYPE_RECIPIENT_OTHER << 0),
					    USB_HUB_REQ_CLEAR_FEATURE, feature, port_number, buf);

	usbh_xfer_buf_free(hub_instance->hub_udev, buf);
	return ret;
}

/* Common function to get status (hub or port) */
static int usbh_hub_get_status_common(struct usbh_hub_instance *hub_instance, uint8_t recipient,
				      uint16_t wIndex, uint16_t *const wStatus,
				      uint16_t *const wChange)
{
	struct net_buf *buf;
	int ret;

	if (!hub_instance || !wStatus || !wChange) {
		return -EINVAL;
	}

	buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, 4);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for status");
		return -ENOMEM;
	}

	ret = usbh_hub_class_request_common(
		hub_instance,
		(USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_CLASS << 5) | (recipient << 0),
		USB_HUB_REQ_GET_STATUS, 0, wIndex, buf);

	if ((ret != 0) || (buf->len < 4)) {
		LOG_ERR("Failed to get status or insufficient data (ret=%d, len=%d)", ret,
			buf ? buf->len : 0);
		*wStatus = 0;
		*wChange = 0;
		goto error_process;
	}

	*wStatus = net_buf_pull_le16(buf);
	*wChange = net_buf_pull_le16(buf);
	LOG_DBG("Status: wStatus=0x%04x, wChange=0x%04x", *wStatus, *wChange);

error_process:
	usbh_xfer_buf_free(hub_instance->hub_udev, buf);
	return ret;
}

int usbh_hub_get_port_status(struct usbh_hub_instance *hub_instance, uint8_t port_number,
			     uint16_t *const wPortStatus, uint16_t *const wPortChange)
{
	int ret;

	ret = usbh_hub_get_status_common(hub_instance, USB_REQTYPE_RECIPIENT_OTHER, port_number,
					 wPortStatus, wPortChange);

	LOG_DBG("Port %d status: wPortStatus=0x%04x, wPortChange=0x%04x", port_number, *wPortStatus,
		*wPortChange);

	return ret;
}

int usbh_hub_get_hub_status(struct usbh_hub_instance *hub_instance, uint16_t *const wHubStatus,
			    uint16_t *const wHubChange)
{
	int ret;

	ret = usbh_hub_get_status_common(hub_instance, USB_REQTYPE_RECIPIENT_DEVICE, 0, wHubStatus,
					 wHubChange);

	LOG_DBG("Hub status: wHubStatus=0x%04x, wHubChange=0x%04x", *wHubStatus, *wHubChange);

	return ret;
}

int usbh_hub_init_instance(struct usbh_hub_instance *hub_instance, struct usb_device *udev)
{
	if (!hub_instance || !udev) {
		return -EINVAL;
	}

	memset(hub_instance, 0, sizeof(*hub_instance));
	hub_instance->hub_udev = udev;

	k_sem_init(&hub_instance->ctrl_sem, 0, 1);
	k_mutex_init(&hub_instance->ctrl_lock);

	return 0;
}

void usbh_hub_cleanup_instance(struct usbh_hub_instance *hub_instance)
{
	if (hub_instance) {
		/* Clean up any pending operations */
		memset(hub_instance, 0, sizeof(*hub_instance));
	}
}
