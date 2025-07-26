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

struct usbh_uvc_data {
	int todo;
};

static int usbh_uvc_completion_cb(struct usbh_class_data *const c_data,
				  struct uhc_transfer *const xfer)
{
	return 0;
}

static int usbh_uvc_parse_control_header(struct usbh_class_data *const c_data,
					 struct usbh_uvc_data *const uvc,
					 struct uvc_control_header_descriptor *desc)
{
	return 0;
}

static int usbh_uvc_parse_output_terminal(struct usbh_class_data *const c_data,
					 struct usbh_uvc_data *const uvc,
					 struct uvc_output_terminal_descriptor *desc)
{
	return 0;
}

static int usbh_uvc_parse_input_terminal(struct usbh_class_data *const c_data,
					struct usbh_uvc_data *const uvc,
					struct uvc_camera_terminal_descriptor *desc)
{
	return 0;
}

static int usbh_uvc_parse_selector_unit(struct usbh_class_data *const c_data,
					struct usbh_uvc_data *const uvc,
					struct uvc_selector_unit_descriptor *desc)
{
	return 0;
}

static int usbh_uvc_parse_processing_unit(struct usbh_class_data *const c_data,
					  struct usbh_uvc_data *const uvc,
					  struct uvc_processing_unit_descriptor *desc)
{
	return 0;
}

static int usbh_uvc_parse_extension_unit(struct usbh_class_data *const c_data,
					 struct usbh_uvc_data *const uvc,
					 struct uvc_extension_unit_descriptor *desc)
{
	return 0;
}

static int usbh_uvc_parse_encoding_unit(struct usbh_class_data *const c_data,
					struct usbh_uvc_data *const uvc,
					struct uvc_encoding_unit_descriptor *desc)
{
	return 0;
}

static int usbh_uvc_parse_control_desc(struct usbh_class_data *const c_data,
				       struct usbh_uvc_data *const uvc,
				       struct usb_if_descriptor *const if_control)
{
	struct uvc_if_descriptor *desc = (void *)if_control;
	int ret = 0;

	while (desc->bLength != 0) {
		/* Skip the interface descriptor or switch to the next descriptor */
		desc = (void *)((uint8_t *)desc + desc->bLength);

		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC ||
		    desc->bDescriptorType == 0) {
			break;
		} else if (desc->bDescriptorType == USB_DESC_CS_INTERFACE) {
			if (desc->bDescriptorSubtype == UVC_VC_HEADER) {
				struct uvc_control_header_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoControl interface: Header");
				ret = usbh_uvc_parse_control_header(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VC_OUTPUT_TERMINAL) {
				struct uvc_output_terminal_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoControl interface: Output Terminal");
				ret = usbh_uvc_parse_output_terminal(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VC_INPUT_TERMINAL) {
				struct uvc_camera_terminal_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoControl interface: Input/Camera Terminal");
				ret = usbh_uvc_parse_input_terminal(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VC_SELECTOR_UNIT) {
				struct uvc_selector_unit_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoControl interface: Selector Unit");
				ret = usbh_uvc_parse_selector_unit(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VC_PROCESSING_UNIT) {
				struct uvc_processing_unit_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoControl interface: Processing Unit");
				ret = usbh_uvc_parse_processing_unit(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VC_EXTENSION_UNIT) {
				struct uvc_extension_unit_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoControl interface: Extension Unit");
				ret = usbh_uvc_parse_extension_unit(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VC_ENCODING_UNIT) {
				struct uvc_encoding_unit_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoControl interface: Encoding Unit");
				ret = usbh_uvc_parse_encoding_unit(c_data, uvc, &copy);
			} else {
				LOG_WRN("VideoControl interface: unknown subtype %u, skipping",
					desc->bDescriptorSubtype);
			}
		} else {
			LOG_DBG("VideoControl descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
		}

		if (ret != 0) {
			LOG_ERR("Error while parsing descriptor type %u");
			return ret;
		}
	}

	return 0;
}

static int usbh_uvc_parse_input_header(struct usbh_class_data *const c_data,
				       struct usbh_uvc_data *const uvc,
				       struct uvc_stream_header_descriptor *const desc)
{
	return 0;
}

static int usbh_uvc_parse_output_header(struct usbh_class_data *const c_data,
					struct usbh_uvc_data *const uvc,
					struct uvc_stream_header_descriptor *const desc)
{
	return 0;
}

static int usbh_uvc_parse_format_uncomp(struct usbh_class_data *const c_data,
					struct usbh_uvc_data *const uvc,
					struct uvc_format_uncomp_descriptor *const desc)
{
	return 0;
}

static int usbh_uvc_parse_format_mjpeg(struct usbh_class_data *const c_data,
				       struct usbh_uvc_data *const uvc,
				       struct uvc_format_mjpeg_descriptor *const desc)
{
	return 0;
}

static int usbh_uvc_parse_frame(struct usbh_class_data *const c_data,
				struct usbh_uvc_data *const uvc,
				struct uvc_frame_descriptor *const desc)
{
	return 0;
}

static int usbh_uvc_parse_color(struct usbh_class_data *const c_data,
				struct usbh_uvc_data *const uvc,
				struct uvc_color_descriptor *const desc)
{
	return 0;
}

static int usbh_uvc_parse_streaming_desc(struct usbh_class_data *const c_data,
					 struct usbh_uvc_data *const uvc,
					 struct usb_if_descriptor *const if_streaming)
{
	struct uvc_if_descriptor *desc = (void *)if_streaming;
	int ret;

	while (desc->bLength != 0) {
		/* Skip the interface descriptor or switch to the next descriptor */
		desc = (void *)((uint8_t *)desc + desc->bLength);

		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC ||
		    desc->bDescriptorType == 0) {
			break;
		} else if (desc->bDescriptorType == USB_DESC_CS_INTERFACE) {
			if (desc->bDescriptorSubtype == UVC_VS_INPUT_HEADER) {
				struct uvc_stream_header_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoStreaming interface: Input header");
				ret = usbh_uvc_parse_input_header(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VS_OUTPUT_HEADER) {
				struct uvc_stream_header_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoStreaming interface: Output header");
				ret = usbh_uvc_parse_output_header(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED) {
				struct uvc_format_uncomp_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoStreaming interface: Uncompressed format");
				ret = usbh_uvc_parse_format_uncomp(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG) {
				struct uvc_format_mjpeg_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoStreaming interface: Uncompressed format");
				ret = usbh_uvc_parse_format_mjpeg(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VS_FRAME_UNCOMPRESSED ||
				   desc->bDescriptorSubtype == UVC_VS_FRAME_MJPEG) {
				struct uvc_frame_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoStreaming interface: Frame");
				ret = usbh_uvc_parse_frame(c_data, uvc, &copy);
			} else if (desc->bDescriptorSubtype == UVC_VS_COLORFORMAT) {
				struct uvc_color_descriptor copy;

				memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
				LOG_DBG("VideoStreaming interface: Color");
				ret = usbh_uvc_parse_color(c_data, uvc, &copy);
			} else {
				LOG_DBG("VideoStreaming descriptor: unknown subtype %u, skipping",
					desc->bDescriptorSubtype);
			}
		} else if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			struct usb_ep_descriptor copy;

			memcpy(&copy, desc, MIN(sizeof(copy), desc->bLength));
			LOG_DBG("VideoStreaming Endpoint", copy.bEndpointAddress);
		} else {
			LOG_DBG("VideoStreaming descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
		}
	}

	return 0;
}

static int usbh_uvc_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev, uint8_t iface)
{
	struct usbh_uvc_data *const uvc = NULL;
	struct usb_if_descriptor *if_control = NULL;
	struct usb_if_descriptor *if_streaming = NULL;
	int ret;

	/* TODO only scan through the interfaces assigned to this class by usbh_class.c */
	for (unsigned int i = 0; udev->ifaces[i].dhp != NULL; i++) {
		struct usb_if_descriptor *if_desc = (void *)udev->ifaces[i].dhp;

		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOCONTROL) {
			if_control = if_desc;
		}

		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOSTREAMING) {
			if_streaming = if_desc;
		}
	}

	if (if_streaming == NULL || if_control == NULL) {
		LOG_ERR("Video Streaming %p or Control %p interface missing",
			if_streaming, if_control);
		return -EINVAL;
	}

	ret = usbh_uvc_parse_control_desc(c_data, uvc, if_control);
	if (ret != 0) {
		return ret;
	}

	ret = usbh_uvc_parse_streaming_desc(c_data, uvc, if_streaming);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int usbh_uvc_removed(struct usbh_class_data *const c_data)
{
	return 0;
}

static int usbh_uvc_init(struct usbh_class_data *const c_data,
			 struct usbh_context *const uhs_ctx)
{
	return 0;
}

static int usbh_uvc_suspended(struct usbh_class_data *const c_data)
{
	return 0;
}

static int usbh_uvc_resumed(struct usbh_class_data *const c_data)
{
	return 0;
}

static int usbh_uvc_preinit(const struct device *dev)
{
	LOG_DBG("%s", dev->name);

	return 0;
}

static struct usbh_class_api uvc_class_api = {
	.init = usbh_uvc_init,
	.completion_cb = usbh_uvc_completion_cb,
	.probe = usbh_uvc_probe,
	.removed = usbh_uvc_removed,
	.suspended = usbh_uvc_suspended,
	.resumed = usbh_uvc_resumed,
};

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
	struct usbh_class_data uvc_class_data_##n = {				\
		.api = &uvc_class_api,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, usbh_uvc_preinit, NULL,			\
			      &uvc_class_data_##n, NULL,			\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			      &uvc_video_api);					\
										\
	VIDEO_DEVICE_DEFINE(uvc_host_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(USBH_VIDEO_DT_DEVICE_DEFINE)
