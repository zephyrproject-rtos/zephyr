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

enum usbd_speed usbd_bus_speed(const struct usbd_context *const uds_ctx)
{
	return uds_ctx->status.speed;
}

enum usbd_speed usbd_caps_speed(const struct usbd_context *const uds_ctx)
{
	struct udc_device_caps caps = udc_caps(uds_ctx->dev);

	/* For now, either high speed is supported or not. */
	if (caps.hs) {
		return USBD_SPEED_HS;
	}

	return USBD_SPEED_FS;
}

static struct usb_device_descriptor *
get_device_descriptor(struct usbd_context *const uds_ctx,
		      const enum usbd_speed speed)
{
	switch (speed) {
	case USBD_SPEED_FS:
		return uds_ctx->fs_desc;
	case USBD_SPEED_HS:
		return uds_ctx->hs_desc;
	default:
		__ASSERT(false, "Not supported speed");
		return NULL;
	}
}

int usbd_device_set_bcd_usb(struct usbd_context *const uds_ctx,
			    const enum usbd_speed speed, const uint16_t bcd)
{
	struct usb_device_descriptor *desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_bcd_exit;
	}

	desc = get_device_descriptor(uds_ctx, speed);
	desc->bcdUSB = sys_cpu_to_le16(bcd);

set_bcd_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_vid(struct usbd_context *const uds_ctx,
			 const uint16_t vid)
{
	struct usb_device_descriptor *fs_desc, *hs_desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_vid_exit;
	}

	fs_desc = get_device_descriptor(uds_ctx, USBD_SPEED_FS);
	fs_desc->idVendor = sys_cpu_to_le16(vid);

	hs_desc = get_device_descriptor(uds_ctx, USBD_SPEED_HS);
	hs_desc->idVendor = sys_cpu_to_le16(vid);

set_vid_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_pid(struct usbd_context *const uds_ctx,
			 const uint16_t pid)
{
	struct usb_device_descriptor *fs_desc, *hs_desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_pid_exit;
	}

	fs_desc = get_device_descriptor(uds_ctx, USBD_SPEED_FS);
	fs_desc->idProduct = sys_cpu_to_le16(pid);

	hs_desc = get_device_descriptor(uds_ctx, USBD_SPEED_HS);
	hs_desc->idProduct = sys_cpu_to_le16(pid);

set_pid_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_bcd_device(struct usbd_context *const uds_ctx,
			       const uint16_t bcd)
{
	struct usb_device_descriptor *fs_desc, *hs_desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_bcd_device_exit;
	}

	fs_desc = get_device_descriptor(uds_ctx, USBD_SPEED_FS);
	fs_desc->bcdDevice = sys_cpu_to_le16(bcd);

	hs_desc = get_device_descriptor(uds_ctx, USBD_SPEED_HS);
	hs_desc->bcdDevice = sys_cpu_to_le16(bcd);

set_bcd_device_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_device_set_code_triple(struct usbd_context *const uds_ctx,
				const enum usbd_speed speed,
				const uint8_t base_class,
				const uint8_t subclass, const uint8_t protocol)
{
	struct usb_device_descriptor *desc;
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_enabled(uds_ctx)) {
		ret = -EALREADY;
		goto set_code_triple_exit;
	}

	desc = get_device_descriptor(uds_ctx, speed);
	desc->bDeviceClass = base_class;
	desc->bDeviceSubClass = subclass;
	desc->bDeviceProtocol = protocol;

set_code_triple_exit:
	usbd_device_unlock(uds_ctx);
	return ret;
}

int usbd_wakeup_request(struct usbd_context *const uds_ctx)
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
		LOG_WRN("Remote wakeup feature not enabled or not suspended");
		ret = -EACCES;
		goto wakeup_request_error;
	}

	ret = udc_host_wakeup(uds_ctx->dev);

wakeup_request_error:
	usbd_device_unlock(uds_ctx);

	return ret;
}

bool usbd_is_suspended(struct usbd_context *uds_ctx)
{
	return uds_ctx->status.suspended;
}

int usbd_init(struct usbd_context *const uds_ctx)
{
	int ret;

	/*
	 * Lock the scheduler to ensure that the context is not preempted
	 * before it is fully initialized.
	 */
	k_sched_lock();
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
	k_sched_unlock();

	return ret;
}

int usbd_enable(struct usbd_context *const uds_ctx)
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

int usbd_disable(struct usbd_context *const uds_ctx)
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

int usbd_shutdown(struct usbd_context *const uds_ctx)
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

bool usbd_can_detect_vbus(struct usbd_context *const uds_ctx)
{
	const struct udc_device_caps caps = udc_caps(uds_ctx->dev);

	return caps.can_detect_vbus;
}

struct usbd_vreq_node *usbd_device_get_vreq(struct usbd_context *const uds_ctx,
					    const uint8_t code)
{
	struct usbd_vreq_node *vreq_nd;

	SYS_DLIST_FOR_EACH_CONTAINER(&uds_ctx->vreqs, vreq_nd, node) {
		if (vreq_nd->code == code) {
			return vreq_nd;
		}
	}

	return NULL;
}

int usbd_device_register_vreq(struct usbd_context *const uds_ctx,
			      struct usbd_vreq_node *const vreq_nd)
{
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (usbd_is_initialized(uds_ctx)) {
		ret = -EPERM;
		goto error;
	}

	if (vreq_nd->to_dev == NULL && vreq_nd->to_host == NULL) {
		ret = -EINVAL;
		goto error;
	}

	if (!sys_dnode_is_linked(&uds_ctx->vreqs)) {
		LOG_DBG("Initialize vendor request list");
		sys_dlist_init(&uds_ctx->vreqs);
	}

	if (sys_dnode_is_linked(&vreq_nd->node)) {
		ret = -EALREADY;
		goto error;
	}

	sys_dlist_append(&uds_ctx->vreqs, &vreq_nd->node);
	LOG_DBG("Registered vendor request 0x%02x", vreq_nd->code);

error:
	usbd_device_unlock(uds_ctx);
	return ret;
}

void usbd_device_unregister_all_vreq(struct usbd_context *const uds_ctx)
{
	struct usbd_vreq_node *tmp;
	sys_dnode_t *node;

	if (!sys_dnode_is_linked(&uds_ctx->vreqs)) {
		return;
	}

	while ((node = sys_dlist_get(&uds_ctx->vreqs))) {
		tmp = CONTAINER_OF(node, struct usbd_vreq_node, node);
		LOG_DBG("Remove vendor request 0x%02x", tmp->code);
	}
}
