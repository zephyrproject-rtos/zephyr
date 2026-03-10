/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

#include "uvc.h"
#include "../../../drivers/video/video_ctrls.h"
#include "../../../drivers/video/video_device.h"

LOG_MODULE_REGISTER(usbh_uvc, CONFIG_USBH_UVC_LOG_LEVEL);

struct usbh_uvc_data {
	const char *name;
};

/*
 * Descriptor parsing utilities
 * Validate and parse the video streaming and video control descriptors.
 */

static int usbh_uvc_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev, const uint8_t iface)
{
	return 0;
}

static int usbh_uvc_removed(struct usbh_class_data *const c_data)
{
	return 0;
}

static int usbh_uvc_init(struct usbh_class_data *const c_data)
{
	return 0;
}

static int usbh_uvc_completion_cb(struct usbh_class_data *const c_data,
				  struct uhc_transfer *const xfer)
{
	return 0;
}

static int usbh_uvc_preinit(const struct device *dev)
{
	return 0;
}

static struct usbh_class_api uvc_class_api = {
	.init = usbh_uvc_init,
	.completion_cb = usbh_uvc_completion_cb,
	.probe = usbh_uvc_probe,
	.removed = usbh_uvc_removed,
};

static int usbh_uvc_get_caps(const struct device *const dev, struct video_caps *const caps)
{
	return 0;
}

static int usbh_uvc_get_format(const struct device *const dev, struct video_format *const fmt)
{
	return 0;
}

static int usbh_uvc_set_stream(const struct device *const dev, const bool enable,
			       const enum video_buf_type type)
{
	return 0;
}

static int usbh_uvc_enqueue(const struct device *const dev, struct video_buffer *const vbuf)
{
	return 0;
}

static int usbh_uvc_dequeue(const struct device *const dev, struct video_buffer **const vbuf,
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

static struct usbh_class_filter usbh_uvc_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_VIDEO,
		.sub = UVC_SC_VIDEO_INTERFACE_COLLECTION,
		.proto = 0,
	},
	{0},
};


#define USBH_VIDEO_DEVICE_DEFINE(n, _)						\
										\
	static struct usbh_uvc_data uvc_data_##n = {				\
		.name = "uvc_"#n,						\
	};									\
										\
	DEVICE_DEFINE(usbh_uvc_##n, "usbh_uvc_"#n,				\
		      usbh_uvc_preinit,	NULL,					\
		      &uvc_data_##n, NULL,					\
		      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,			\
		      &uvc_video_api);						\
										\
	USBH_DEFINE_CLASS(uvc_c_data_##n, &uvc_class_api,			\
			  (void *)DEVICE_GET(usbh_uvc_##n),			\
			  usbh_uvc_filters);					\
										\
	VIDEO_DEVICE_DEFINE(uvc_host_##n, DEVICE_GET(usbh_uvc_##n), NULL);

LISTIFY(CONFIG_USBH_UVC_INSTANCES_COUNT, USBH_VIDEO_DEVICE_DEFINE, ())
