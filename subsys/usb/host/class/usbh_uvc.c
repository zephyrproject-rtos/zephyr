/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uvc_host

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_uvc.h>

#include "usbh_device.h"
#include "usbh_ch9.h"

#include "../../../drivers/video/video_ctrls.h"
#include "../../../drivers/video/video_device.h"

LOG_MODULE_REGISTER(usbh_uvc, CONFIG_USBH_VIDEO_LOG_LEVEL);

struct usbh_uvc_config {
	struct usbh_contex *uhs_ctx;
};

static int usbh_uvc_request(struct usbh_contex *const uhs_ctx, struct uhc_transfer *const xfer,
			    int err)
{
	LOG_INF("%p %p %d", uhs_ctx, xfer, err);

	return 0;
}

static int usbh_uvc_connected(struct usbh_contex *const uhs_ctx)
{
	struct usb_device *const udev = uhs_ctx->root;
	const size_t len = 512;
	struct net_buf *buf;
	int ret;

	LOG_INF("%p", uhs_ctx);

	buf = usbh_xfer_buf_alloc(udev, len);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate a host transfer buffer");
		return -ENOMEM;
	}

	ret = usbh_req_desc(udev, USB_DESC_DEVICE, 0, 0, len, buf);
	if (ret != 0) {
		LOG_ERR("Failed to request descriptor");
		return ret;
	}

	LOG_HEXDUMP_INF(buf->data, buf->len, "buf");

	return 0;
}

static int usbh_uvc_removed(struct usbh_contex *const uhs_ctx)
{
	LOG_INF("%p", uhs_ctx);

	return 0;
}

static int usbh_uvc_rwup(struct usbh_contex *const uhs_ctx)
{
	LOG_INF("%p", uhs_ctx);

	return 0;
}

static int usbh_uvc_suspended(struct usbh_contex *const uhs_ctx)
{
	LOG_INF("%p", uhs_ctx);

	return 0;
}

static int usbh_uvc_resumed(struct usbh_contex *const uhs_ctx)
{
	LOG_INF("%p", uhs_ctx);

	return 0;
}

static int usbh_uvc_preinit(const struct device *dev)
{
	LOG_INF("%s", dev->name);

	return 0;
}

static const struct usbh_class_data usbh_uvc_class = {
	.code = {0, 0, 0},
	.request = usbh_uvc_request,
	.connected = usbh_uvc_connected,
	.removed = usbh_uvc_removed,
	.rwup = usbh_uvc_rwup,
	.suspended = usbh_uvc_suspended,
	.resumed = usbh_uvc_resumed,
};

/* TODO this hardcodes a single class instance as only class of the system */
const struct usbh_class_data *usbh_class = &usbh_uvc_class;

int usbh_uvc_get_caps(const struct device *const dev, struct video_caps *const caps)
{
	return 0;
}

int usbh_uvc_get_format(const struct device *const dev, struct video_format *const fmt)
{
	return 0;
}

int usbh_uvc_set_stream(const struct device *const dev, bool enable, enum video_buf_type type)
{
	return 0;
}

int usbh_uvc_enqueue(const struct device *const dev, struct video_buffer *const vbuf)
{
	return 0;
}

int usbh_uvc_dequeue(const struct device *const dev, struct video_buffer **const vbuf,
		     k_timeout_t timeout)
{
	return 0;
}

static DEVICE_API(video, uvc_video_api) = {
	.get_caps = usbh_uvc_get_caps,
	.get_format = usbh_uvc_get_format,
	.set_stream = usbh_uvc_set_stream,
	.enqueue = usbh_uvc_enqueue,
	.dequeue = usbh_uvc_dequeue,
};

#define USBH_VIDEO_DT_DEVICE_DEFINE(n)						\
										\
	DEVICE_DT_INST_DEFINE(n, usbh_uvc_preinit, NULL,			\
			      NULL, &usbh_uvc_class,				\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			      &uvc_video_api);					\
										\
	VIDEO_DEVICE_DEFINE(uvc_host_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(USBH_VIDEO_DT_DEVICE_DEFINE)
