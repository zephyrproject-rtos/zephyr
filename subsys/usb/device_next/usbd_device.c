/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"
#include "usbd_config.h"
#include "usbd_class.h"
#include "usbd_ch9.h"
#include "usbd_desc.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_dev, CONFIG_USBD_LOG_LEVEL);

/*
 * All the functions below are part of public USB device support API.
 */

int usbd_device_set_bcd(struct usbd_contex *const uds_ctx,
			const uint16_t bcd)
{
	struct usb_device_descriptor *desc = uds_ctx->desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_bcd_exit;
	}

	desc->bcdUSB = sys_cpu_to_le16(bcd);

set_bcd_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_vid(struct usbd_contex *const uds_ctx,
			 const uint16_t vid)
{
	struct usb_device_descriptor *desc = uds_ctx->desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_vid_exit;
	}

	desc->idVendor = sys_cpu_to_le16(vid);

set_vid_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_pid(struct usbd_contex *const uds_ctx,
			 const uint16_t pid)
{
	struct usb_device_descriptor *desc = uds_ctx->desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_pid_exit;
	}

	desc->idProduct = sys_cpu_to_le16(pid);

set_pid_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_class(struct usbd_contex *const uds_ctx,
			   const uint8_t value)
{
	struct usb_device_descriptor *desc = uds_ctx->desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_class_exit;
	}

	desc->bDeviceClass = value;

set_class_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_subclass(struct usbd_contex *const uds_ctx,
			     const uint8_t value)
{
	struct usb_device_descriptor *desc = uds_ctx->desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_subclass_exit;
	}

	desc->bDeviceSubClass = value;

set_subclass_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_proto(struct usbd_contex *const uds_ctx,
			  const uint8_t value)
{
	struct usb_device_descriptor *desc = uds_ctx->desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_proto_exit;
	}

	desc->bDeviceProtocol = value;

set_proto_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_wakeup_request(struct usbd_contex *const uds_ctx)
{
	struct udc_device_caps caps = udc_caps(uds_ctx->dev);
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (!caps.rwup) {
		LOG_ERR("Remote wakeup feature not supported");
		ret = -ENOTSUP;
		goto wakeup_request_error;
	}

	if (!uds_ctx->status.rwup || !usbd_is_suspended(uds_ctx)) {
		LOG_ERR("Remote wakeup feature not enabled or not suspended");
		ret = -EACCES;
		goto wakeup_request_error;
	}

	ret = udc_host_wakeup(uds_ctx->dev);

wakeup_request_error:
	usbd_device_unlock(uds_ctx);

	return ret;
}

bool usbd_is_suspended(struct usbd_contex *uds_ctx)
{
	return uds_ctx->status.suspended;
}

int usbd_init(struct usbd_contex *const uds_ctx)
{
	int ret;

	usbd_device_lock(uds_ctx);

	if (uds_ctx->dev == NULL) {
		ret = -ENODEV;
		goto init_exit;
	}

	if (usbd_is_initialized(uds_ctx)) {
		LOG_WRN("USB device support is already initialized");
		ret = -EALREADY;
		goto init_exit;
	}

	if (!device_is_ready(uds_ctx->dev)) {
		LOG_ERR("USB device controller is not ready");
		ret = -ENODEV;
		goto init_exit;
	}

	ret = usbd_device_init_core(uds_ctx);
	if (ret) {
		goto init_exit;
	}

	memset(&uds_ctx->ch9_data, 0, sizeof(struct usbd_ch9_data));
	uds_ctx->status.initialized = true;

init_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_enable(struct usbd_contex *const uds_ctx)
{
	int ret;

	usbd_device_lock(uds_ctx);

	if (!usbd_is_initialized(uds_ctx)) {
		LOG_WRN("USB device support is not initialized");
		ret = -EPERM;
		goto enable_exit;
	}

	if (usbd_is_enabled(uds_ctx)) {
		LOG_WRN("USB device support is already enabled");
		ret = -EALREADY;
		goto enable_exit;
	}

	ret = udc_enable(uds_ctx->dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable controller");
		goto enable_exit;
	}

	ret = usbd_init_control_pipe(uds_ctx);
	if (ret != 0) {
		udc_disable(uds_ctx->dev);
		goto enable_exit;
	}

	uds_ctx->status.enabled = true;

enable_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_disable(struct usbd_contex *const uds_ctx)
{
	int ret;

	if (!usbd_is_enabled(uds_ctx)) {
		LOG_WRN("USB device support is already disabled");
		return -EALREADY;
	}

	usbd_device_lock(uds_ctx);

	ret = usbd_config_set(uds_ctx, 0);
	if (ret) {
		LOG_ERR("Failed to reset configuration");
	}

	ret = udc_disable(uds_ctx->dev);
	if (ret) {
		LOG_ERR("Failed to disable USB device");
	}

	uds_ctx->status.enabled = false;

	usbd_device_unlock(uds_ctx);

	return ret;
}

int usbd_shutdown(struct usbd_contex *const uds_ctx)
{
	int ret;

	usbd_device_lock(uds_ctx);

	/* TODO: control request dequeue ? */
	ret = usbd_device_shutdown_core(uds_ctx);
	if (ret) {
		LOG_ERR("Failed to shutdown USB device");
	}

	uds_ctx->status.initialized = false;
	usbd_device_unlock(uds_ctx);

	return 0;
}
