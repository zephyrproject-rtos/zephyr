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

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usb_uvc.h"

#include "../../../drivers/video/video_ctrls.h"
#include "../../../drivers/video/video_device.h"

LOG_MODULE_REGISTER(usbh_uvc, CONFIG_USBH_VIDEO_LOG_LEVEL);

struct usbh_uvc_data {
	int todo;
};

/*
 * Descriptor parsing utilities
 * Validate and parse the video streaming and video control descriptors.
 */

static bool usbh_uvc_desc_is_valid_vs_header(const void *const desc,
						 const void *const desc_end)
{
	const struct uvc_stream_header_descriptor *const header_desc = desc;

	return usbh_desc_is_valid(desc, desc_end, sizeof(struct uvc_stream_header_descriptor)) &&
	       header_desc->bDescriptorType == USB_DESC_CS_INTERFACE &&
	       (header_desc->bDescriptorSubtype == UVC_VS_OUTPUT_HEADER ||
	        header_desc->bDescriptorSubtype == UVC_VS_INPUT_HEADER);
}

static bool usbh_uvc_desc_is_valid_vc_header(const void *const desc,
					     const void *const desc_end)
{
	const struct uvc_control_header_descriptor *const header_desc = desc;

	return usbh_desc_is_valid(desc, desc_end, sizeof(struct uvc_control_header_descriptor)) &&
	       header_desc->bDescriptorType == USB_DESC_CS_INTERFACE &&
	       header_desc->bDescriptorSubtype == UVC_VC_HEADER;
}

const void *usbh_uvc_desc_get_vs_end(const struct usb_if_descriptor *if_desc,
				     const void *const desc_end)
{
	const struct uvc_stream_header_descriptor *const header_desc =
		usbh_desc_get_next(if_desc, desc_end);
	const void *vs_end;

	if (!usbh_uvc_desc_is_valid_vs_header(header_desc, desc_end)) {
		return NULL;
	}

	vs_end = (uint8_t *)header_desc + header_desc->wTotalLength;
	if (vs_end > desc_end) {
		return NULL;
	}

	return vs_end;
}

const void *usbh_uvc_desc_get_vc_end(const struct usb_if_descriptor *if_desc,
				          const void *const desc_end)
{
	const struct uvc_control_header_descriptor *const header_desc =
		usbh_desc_get_next(if_desc, desc_end);
	const void *vc_end;

	if (!usbh_uvc_desc_is_valid_vc_header(header_desc, desc_end)) {
		return NULL;
	}

	vc_end = (uint8_t *)header_desc + header_desc->wTotalLength;
	if (vc_end > desc_end) {
		LOG_WRN("vc_end %p > desc_end %p", vc_end, desc_end);
		return NULL;
	}

	return vc_end;
}

static int usbh_uvc_parse_vc_desc(struct usbh_class_data *const c_data,
				  const void *const desc_beg,
				  const void *const desc_end)
{
	/* Skip the interface descriptor itself */
	const struct usb_desc_header *desc = usbh_desc_get_next(desc_beg, desc_end);

	for (; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC ||
		    desc->bDescriptorType == 0) {
			break;
		} else if (desc->bDescriptorType == USB_DESC_CS_INTERFACE) {
			const struct uvc_if_descriptor *const if_desc = (void *)desc;

			switch (if_desc->bDescriptorSubtype) {
			case UVC_VC_HEADER:
				LOG_DBG("VideoControl interface: Header");
				break;
			case UVC_VC_OUTPUT_TERMINAL:
				LOG_DBG("VideoControl interface: Output Terminal");
				break;
			case UVC_VC_INPUT_TERMINAL:
				LOG_DBG("VideoControl interface: Input/Camera Terminal");
				break;
			case UVC_VC_SELECTOR_UNIT:
				LOG_DBG("VideoControl interface: Selector Unit");
				break;
			case UVC_VC_PROCESSING_UNIT:
				LOG_DBG("VideoControl interface: Processing Unit");
				break;
			case UVC_VC_EXTENSION_UNIT:
				LOG_DBG("VideoControl interface: Extension Unit");
				break;
			case UVC_VC_ENCODING_UNIT:
				LOG_DBG("VideoControl interface: Encoding Unit");
				break;
			default:
				LOG_WRN("VideoControl interface: unknown subtype %u, skipping",
					if_desc->bDescriptorSubtype);
			}
		} else {
			LOG_WRN("VideoControl descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
		}
	}

	return 0;
}

static int usbh_uvc_parse_vs_desc(struct usbh_class_data *const c_data,
				  const void *const desc_beg,
				  const void *const desc_end)
{
	/* Skip the interface descriptor itself */
	const struct usb_desc_header *desc = usbh_desc_get_next(desc_beg, desc_end);

	for (; desc != NULL; desc = usbh_desc_get_next(desc, desc_end)) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC ||
		    desc->bDescriptorType == 0) {
			break;
		} else if (desc->bDescriptorType == USB_DESC_CS_INTERFACE) {
			const struct uvc_if_descriptor *const if_desc = (void *)desc;

			switch (if_desc->bDescriptorSubtype) {
			case UVC_VS_INPUT_HEADER:
				LOG_DBG("VideoStreaming interface: Input header");
				break;
			case UVC_VS_OUTPUT_HEADER:
				LOG_DBG("VideoStreaming interface: Output header");
				break;
			case UVC_VS_FORMAT_UNCOMPRESSED:
				LOG_DBG("VideoStreaming interface: Uncompressed format");
				break;
			case UVC_VS_FORMAT_MJPEG:
				LOG_DBG("VideoStreaming interface: MJPEG format");
				break;
			case UVC_VS_FRAME_UNCOMPRESSED:
				LOG_DBG("VideoStreaming interface: Uncompressed Frame");
				break;
			case UVC_VS_FRAME_MJPEG:
				LOG_DBG("VideoStreaming interface: MJPEG Frame");
				break;
			case UVC_VS_COLORFORMAT:
				LOG_DBG("VideoStreaming interface: Color");
				break;
			default:
				LOG_DBG("VideoStreaming descriptor: unknown subtype %u, skipping",
					if_desc->bDescriptorSubtype);
			}
		} else if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *const ep_desc = (void *)desc;

			LOG_DBG("VideoStreaming Endpoint", ep_desc->bEndpointAddress);
		} else {
			LOG_WRN("VideoStreaming descriptor: unknown type %u, skipping",
				desc->bDescriptorType);
		}
	}

	return 0;
}

static int usbh_uvc_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev, uint8_t iface)
{
	const void *const desc_beg = usbh_desc_get_cfg_beg(udev);
	const void *const desc_end = usbh_desc_get_cfg_end(udev);
	const struct usb_association_descriptor *const iad_desc =
		usbh_desc_get_by_iface(desc_beg, desc_end, iface);
	const struct usb_desc_header *desc;
	bool has_vc_if = false;
	bool has_vs_if = false;
	int ret;

	if (iad_desc == NULL) {
		LOG_ERR("failed to find interface or interface association number %u", iface);
		return -ENOSYS;
	}

	if (iad_desc->bDescriptorType != USB_DESC_INTERFACE_ASSOC) {
		LOG_WRN("Interface %u is not a valid %s, skipping", iface, "interface assoc");
		return -ENOTSUP;
	}

	desc = (struct usb_desc_header *)usbh_desc_get_next(iad_desc, desc_end);
	if (desc == NULL) {
		return -EBADMSG;
	}

	for (int i = 0; i < iad_desc->bInterfaceCount; i++) {
		const struct usb_if_descriptor *if_desc;

		if_desc = (void *)usbh_desc_get_by_iface(desc, desc_end, c_data->iface + i);
		if (if_desc == NULL) {
			LOG_ERR("Not as many interfaces (%u) as announced (%u)",
				i, iad_desc->bInterfaceCount);
			return -EBADMSG;
		}

		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOCONTROL) {
			const void *vc_end;

			if (has_vc_if) {
				LOG_WRN("Skipping extra VideoControl interface");
				continue;
			}

			vc_end = usbh_uvc_desc_get_vc_end(if_desc, desc_end);
			if (vc_end == NULL) {
				LOG_ERR("Invalid VideoControl interface descriptor");
				return -EBADMSG;
			}

			ret = usbh_uvc_parse_vc_desc(c_data, if_desc, vc_end);
			if (ret != 0) {
				LOG_ERR("Failed to parse VC descriptor");
				return ret;
			}

			has_vc_if = true;
		}

		if (if_desc->bInterfaceClass == USB_BCC_VIDEO &&
		    if_desc->bInterfaceSubClass == UVC_SC_VIDEOSTREAMING) {
			const void *vs_end;

			if (has_vs_if) {
				LOG_WRN("Skipping extra VideoStreaming interface");
				continue;
			}

			vs_end = usbh_uvc_desc_get_vs_end(if_desc, desc_end);
			if (vs_end == NULL) {
				LOG_ERR("Invalid VideoStreaming interface descriptor");
				return -EBADMSG;
			}

			ret = usbh_uvc_parse_vs_desc(c_data, if_desc, vs_end);
			if (ret != 0) {
				return ret;
			}

			has_vs_if = true;
		}
	}

	if (!has_vs_if) {
		LOG_ERR("No VideoStreaming interface found");
		return -EINVAL;
	}

	if (!has_vc_if) {
		LOG_DBG("No VideoControl interface found");
		return -EINVAL;
	}

	LOG_INF("Interface %u associated with UVC class", iface);

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

static int usbh_uvc_completion_cb(struct usbh_class_data *const c_data,
				  struct uhc_transfer *const xfer)
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

static struct usbh_class_filter usbh_uvc_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CLASS | USBH_CLASS_MATCH_SUB,
		.class = USB_BCC_VIDEO,
		.sub = UVC_SC_VIDEO_INTERFACE_COLLECTION,
	},
};

#define USBH_VIDEO_DT_DEVICE_DEFINE(n)						\
	struct usbh_uvc_data usbh_uvc_data_##n = {				\
	};									\
										\
	USBH_DEFINE_CLASS(uvc_c_data_##n, &uvc_class_api,			\
			  (void *)DEVICE_DT_INST_GET(n),			\
			  usbh_uvc_filters, ARRAY_SIZE(usbh_uvc_filters));	\
										\
	DEVICE_DT_INST_DEFINE(n, usbh_uvc_preinit, NULL,			\
			      &usbh_uvc_data_##n, NULL,				\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			      &uvc_video_api);					\
										\
	VIDEO_DEVICE_DEFINE(uvc_host_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(USBH_VIDEO_DT_DEVICE_DEFINE)
