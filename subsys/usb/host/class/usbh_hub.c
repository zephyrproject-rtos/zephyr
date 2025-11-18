/*
 * Copyright 2025 NXP
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

/**
 * @brief Common hub control request function
 */
static int usbh_hub_class_request_common(struct usbh_hub_instance *hub_instance,
					 uint8_t request_type, uint8_t request,
					 uint16_t wvalue, uint16_t windex,
					 uint8_t *buffer, uint16_t buffer_length,
					 usbh_hub_callback_t callback_fn,
					 void *callback_param)
{
	struct net_buf *buf;
	int ret;

	if (!hub_instance || !hub_instance->hub_udev) {
		return -EINVAL;
	}

	k_mutex_lock(&hub_instance->ctrl_lock, K_FOREVER);

	/* Allocate buffer for transfer */
	if (buffer_length > 0) {
		buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, buffer_length);
		if (!buf) {
			LOG_ERR("Failed to allocate buffer for hub control request");
			k_mutex_unlock(&hub_instance->ctrl_lock);
			return -ENOMEM;
		}

		/* For OUT transfers, copy data to buffer */
		if (!(request_type & 0x80) && buffer) {
			memcpy(buf->data, buffer, buffer_length);
			net_buf_add(buf, buffer_length);
		}
	} else {
		buf = usbh_xfer_buf_alloc(hub_instance->hub_udev, 0);
		if (!buf) {
			LOG_ERR("Failed to allocate zero-length buffer");
			k_mutex_unlock(&hub_instance->ctrl_lock);
			return -ENOMEM;
		}
	}

	/* Execute synchronous control transfer - 参考CDC ECM的方式 */
	ret = usbh_req_setup(hub_instance->hub_udev,
			     request_type,
			     request,
			     wvalue,
			     windex,
			     buffer_length,
			     buf);

	if (ret < 0) {
		LOG_ERR("Hub control request failed: %d", ret);
		goto cleanup;
	}

	/* For IN transfers, copy data back to user buffer */
	if ((request_type & 0x80) && 
			buffer_length > 0 && buffer && buf->len > 0) {
		memcpy(buffer, buf->data, MIN(buffer_length, buf->len));
	}

	/* Call callback if provided */
	if (callback_fn) {
		callback_fn(callback_param, buffer, 
			   (ret == 0) ? buf->len : 0, ret);
	}

	LOG_DBG("Hub control request completed: type=0x%02x, req=0x%02x, status=%d",
		request_type, request, ret);

cleanup:
	usbh_xfer_buf_free(hub_instance->hub_udev, buf);
	k_mutex_unlock(&hub_instance->ctrl_lock);
	return ret;
}

int usbh_hub_init_instance(struct usbh_hub_instance *hub_instance,
			   struct usb_device *udev)
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

int usbh_hub_get_descriptor(struct usbh_hub_instance *hub_instance,
			    uint8_t *buffer, uint16_t buffer_length,
			    usbh_hub_callback_t callback_fn, void *callback_param)
{
	return usbh_hub_class_request_common(hub_instance,
		(USB_REQTYPE_DIR_TO_HOST << 7) |
		(USB_REQTYPE_TYPE_CLASS << 5) |
		(USB_REQTYPE_RECIPIENT_DEVICE << 0),
		USB_HUB_REQ_GET_DESCRIPTOR, 
		(USB_HUB_DESCRIPTOR_TYPE << 8), 
		0,
		buffer, buffer_length, callback_fn, callback_param);
}

int usbh_hub_set_port_feature(struct usbh_hub_instance *hub_instance,
			      uint8_t port_number, uint8_t feature,
			      usbh_hub_callback_t callback_fn, void *callback_param)
{
	return usbh_hub_class_request_common(hub_instance,
		(USB_REQTYPE_DIR_TO_DEVICE << 7) |
		(USB_REQTYPE_TYPE_CLASS << 5) |
		(USB_REQTYPE_RECIPIENT_OTHER << 0),
		USB_HUB_REQ_SET_FEATURE, 
		feature, 
		port_number,
		NULL, 0, callback_fn, callback_param);
}

int usbh_hub_clear_port_feature(struct usbh_hub_instance *hub_instance,
				uint8_t port_number, uint8_t feature,
				usbh_hub_callback_t callback_fn, void *callback_param)
{
	return usbh_hub_class_request_common(hub_instance,
		(USB_REQTYPE_DIR_TO_DEVICE << 7) |
		(USB_REQTYPE_TYPE_CLASS << 5) |
		(USB_REQTYPE_RECIPIENT_OTHER << 0),
		USB_HUB_REQ_CLEAR_FEATURE, 
		feature, 
		port_number,
		NULL, 0, callback_fn, callback_param);
}

int usbh_hub_get_port_status(struct usbh_hub_instance *hub_instance,
			     uint8_t port_number, uint8_t *buffer, uint16_t buffer_length,
			     usbh_hub_callback_t callback_fn, void *callback_param)
{
	return usbh_hub_class_request_common(hub_instance,
		(USB_REQTYPE_DIR_TO_HOST << 7) |
		(USB_REQTYPE_TYPE_CLASS << 5) |
		(USB_REQTYPE_RECIPIENT_OTHER << 0),
		USB_HUB_REQ_GET_STATUS, 
		0, 
		port_number,
		buffer, buffer_length, callback_fn, callback_param);
}

int usbh_hub_get_hub_status(struct usbh_hub_instance *hub_instance,
			    uint8_t *buffer, uint16_t buffer_length,
			    usbh_hub_callback_t callback_fn, void *callback_param)
{
	return usbh_hub_class_request_common(hub_instance,
		(USB_REQTYPE_DIR_TO_HOST << 7) |
		(USB_REQTYPE_TYPE_CLASS << 5) |
		(USB_REQTYPE_RECIPIENT_DEVICE << 0),
		USB_HUB_REQ_GET_STATUS, 
		0, 
		0,
		buffer, buffer_length, callback_fn, callback_param);
}

void usbh_hub_cleanup_instance(struct usbh_hub_instance *hub_instance)
{
	if (hub_instance) {
		/* Clean up any pending operations */
		memset(hub_instance, 0, sizeof(*hub_instance));
	}
}
