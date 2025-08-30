/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uvc_device

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usbd_uvc.h>

#include "usbd_uvc.h"
#include "../../../drivers/video/video_ctrls.h"
#include "../../../drivers/video/video_device.h"

LOG_MODULE_REGISTER(usbd_uvc, CONFIG_USBD_VIDEO_LOG_LEVEL);

#define UVC_VBUF_DONE 1
#define UVC_MAX_FS_DESC (CONFIG_USBD_VIDEO_MAX_FORMATS + 13)
#define UVC_MAX_HS_DESC (CONFIG_USBD_VIDEO_MAX_FORMATS + 13)
#define UVC_IDX_VC_UNIT 3
#define UVC_MAX_HEADER_LENGTH 0xff

enum uvc_op {
	UVC_OP_GET_ERRNO,
	UVC_OP_VC_CTRL,
	UVC_OP_VS_PROBE,
	UVC_OP_VS_COMMIT,
	UVC_OP_RETURN_ERROR,
	UVC_OP_INVALID,
};

enum uvc_class_status {
	UVC_STATE_INITIALIZED,
	UVC_STATE_ENABLED,
	UVC_STATE_STREAM_READY,
	UVC_STATE_STREAM_RESTART,
	UVC_STATE_PAUSED,
};

enum uvc_unit_id {
	UVC_UNIT_ID_CT = 1,
	UVC_UNIT_ID_SU,
	UVC_UNIT_ID_PU,
	UVC_UNIT_ID_XU,
	UVC_UNIT_ID_OT,
};

enum uvc_control_type {
	UVC_CONTROL_SIGNED,
	UVC_CONTROL_UNSIGNED,
};

union uvc_fmt_desc {
	struct usb_desc_header hdr;
	struct uvc_format_descriptor fmt;
	struct uvc_format_uncomp_descriptor fmt_uncomp;
	struct uvc_format_mjpeg_descriptor fmt_mjpeg;
	struct uvc_frame_descriptor frm;
	struct uvc_frame_continuous_descriptor frm_cont;
	struct uvc_frame_discrete_descriptor frm_disc;
};

struct uvc_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct uvc_control_header_descriptor if0_hdr;
	struct uvc_camera_terminal_descriptor if0_ct;
	struct uvc_selector_unit_descriptor if0_su;
	struct uvc_processing_unit_descriptor if0_pu;
	struct uvc_extension_unit_descriptor if0_xu;
	struct uvc_output_terminal_descriptor if0_ot;
	struct usb_if_descriptor if1;
	struct uvc_stream_header_descriptor if1_hdr;
	union uvc_fmt_desc if1_fmts[CONFIG_USBD_VIDEO_MAX_FORMATS];
	struct uvc_color_descriptor if1_color;
	struct usb_ep_descriptor if1_ep_fs;
	struct usb_ep_descriptor if1_ep_hs;
};

struct uvc_data {
	/* Input buffers to which enqueued video buffers land */
	struct k_fifo fifo_in;
	/* Output buffers from which dequeued buffers are picked */
	struct k_fifo fifo_out;
	/* Default video probe stored at boot time and sent back to the host when requested */
	struct uvc_probe default_probe;
	/* Video payload header content sent before every frame, updated between every frame */
	struct uvc_payload_header payload_header;
	/* Video device that is connected to this UVC stream */
	const struct device *video_dev;
	/* Video format cached locally for efficiency */
	struct video_format video_fmt;
	/* Current frame interval selected by the host */
	struct video_frmival video_frmival;
	/* Signal to alert video devices of buffer-related evenets */
	struct k_poll_signal *video_sig;
	/* Last pixel format that was added by uvc_add_format() */
	uint32_t last_pix_fmt;
	/* Last format descriptor that was added by uvc_add_format() */
	struct uvc_format_descriptor *last_format_desc;
	/* Makes sure flushing the stream only happens in one context at a time */
	struct k_mutex mutex;
	/* Zero Length packet used to reset a stream when restarted */
	struct net_buf zlp;
	/* Byte offset within the currently transmitted video buffer */
	size_t vbuf_offset;
	/* Let the different parts of the code know of the current state */
	atomic_t state;
	/* Index where newly generated descriptors are appened */
	unsigned int fs_desc_idx;
	unsigned int hs_desc_idx;
	unsigned int fmt_desc_idx;
	/* UVC error from latest request */
	uint8_t err;
	/* Format currently selected by the host */
	uint8_t format_id;
	/* Frame currently selected by the host */
	uint8_t frame_id;
};

struct uvc_config {
	/* Storage for the various descriptors available */
	struct uvc_desc *desc;
	/* Class context used by the USB device stack */
	struct usbd_class_data *c_data;
	/* Array of pointers to descriptors sent to the USB device stack and the host */
	struct usb_desc_header **fs_desc;
	struct usb_desc_header **hs_desc;
};

/* Specialized version of UDC net_buf metadata with extra fields */
struct uvc_buf_info {
	/* Regular UDC buf info so that it can be passed to USBD directly */
	struct udc_buf_info udc;
	/* Extra field at the end */
	struct video_buffer *vbuf;
} __packed;

/* Mapping between UVC controls and Video controls */
struct uvc_control_map {
	/* Video CID to use for this control */
	uint32_t cid;
	/* Size to write out */
	uint8_t size;
	/* Bit position in the UVC control */
	uint8_t bit;
	/* UVC selector identifying this control */
	uint8_t selector;
	/* Whether the UVC value is signed, always false for bitmaps and boolean */
	enum uvc_control_type type;
};

struct uvc_guid_quirk {
	/* A Video API format identifier, for which the UVC format GUID is not standard. */
	uint32_t fourcc;
	/* GUIDs are 16-bytes long, with the first four bytes being the Four Character Code of the
	 * format and the rest constant, except for some exceptions listed in this table.
	 */
	uint8_t guid[16];
};

#define UVC_TOTAL_BUFS (DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * CONFIG_USBD_VIDEO_NUM_BUFS)

UDC_BUF_POOL_VAR_DEFINE(uvc_buf_pool, UVC_TOTAL_BUFS, UVC_TOTAL_BUFS * USBD_MAX_BULK_MPS,
			sizeof(struct uvc_buf_info), NULL);

static void uvc_flush_queue(const struct device *dev);

/* UVC helper functions */

static const struct uvc_guid_quirk uvc_guid_quirks[] = {
	{
		.fourcc = VIDEO_PIX_FMT_YUYV,
		.guid = UVC_FORMAT_GUID("YUY2"),
	},
	{
		.fourcc = VIDEO_PIX_FMT_GREY,
		.guid = UVC_FORMAT_GUID("Y800"),
	},
};

static void uvc_fourcc_to_guid(uint8_t guid[16], const uint32_t fourcc)
{
	uint32_t fourcc_le;

	/* Lookup in the "quirk table" if the UVC format GUID is custom */
	for (int i = 0; i < ARRAY_SIZE(uvc_guid_quirks); i++) {
		if (uvc_guid_quirks[i].fourcc == fourcc) {
			memcpy(guid, uvc_guid_quirks[i].guid, 16);
			return;
		}
	}

	/* By default, UVC GUIDs are the four character code followed by a common suffix */
	fourcc_le = sys_cpu_to_le32(fourcc);
	/* Copy the common suffix with the GUID set to 'XXXX' */
	memcpy(guid, UVC_FORMAT_GUID("XXXX"), 16);
	/* Replace the 'XXXX' by the actual GUID of the format */
	memcpy(guid, &fourcc_le, 4);
}

static uint32_t uvc_guid_to_fourcc(const uint8_t guid[16])
{
	uint32_t fourcc;

	/* Lookup in the "quirk table" if the UVC format GUID is custom */
	for (int i = 0; i < ARRAY_SIZE(uvc_guid_quirks); i++) {
		if (memcmp(guid, uvc_guid_quirks[i].guid, 16) == 0) {
			return uvc_guid_quirks[i].fourcc;
		}
	}

	/* Extract the four character code out of the leading 4 bytes of the GUID */
	memcpy(&fourcc, guid, 4);
	fourcc = sys_le32_to_cpu(fourcc);

	return fourcc;
}

/* UVC control handling */

static const struct uvc_control_map uvc_control_map_ct[] = {
	{
		.size = 1,
		.bit = 1,
		.selector = UVC_CT_AE_MODE_CONTROL,
		.cid = VIDEO_CID_EXPOSURE_AUTO,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 1,
		.bit = 2,
		.selector = UVC_CT_AE_PRIORITY_CONTROL,
		.cid = VIDEO_CID_EXPOSURE_AUTO_PRIORITY,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 3,
		.selector = UVC_CT_EXPOSURE_TIME_ABS_CONTROL,
		.cid = VIDEO_CID_EXPOSURE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 5,
		.selector = UVC_CT_FOCUS_ABS_CONTROL,
		.cid = VIDEO_CID_FOCUS_ABSOLUTE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 6,
		.selector = UVC_CT_FOCUS_REL_CONTROL,
		.cid = VIDEO_CID_FOCUS_RELATIVE,
		.type = UVC_CONTROL_SIGNED,
	},
	{
		.size = 2,
		.bit = 7,
		.selector = UVC_CT_IRIS_ABS_CONTROL,
		.cid = VIDEO_CID_IRIS_ABSOLUTE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 1,
		.bit = 8,
		.selector = UVC_CT_IRIS_REL_CONTROL,
		.cid = VIDEO_CID_IRIS_RELATIVE,
		.type = UVC_CONTROL_SIGNED,
	},
	{
		.size = 2,
		.bit = 9,
		.selector = UVC_CT_ZOOM_ABS_CONTROL,
		.cid = VIDEO_CID_ZOOM_ABSOLUTE,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 3,
		.bit = 10,
		.selector = UVC_CT_ZOOM_REL_CONTROL,
		.cid = VIDEO_CID_ZOOM_RELATIVE,
		.type = UVC_CONTROL_SIGNED,
	},
};

static const struct uvc_control_map uvc_control_map_pu[] = {
	{
		.size = 2,
		.bit = 0,
		.selector = UVC_PU_BRIGHTNESS_CONTROL,
		.cid = VIDEO_CID_BRIGHTNESS,
		.type = UVC_CONTROL_SIGNED,
	},
	{
		.size = 1,
		.bit = 1,
		.selector = UVC_PU_CONTRAST_CONTROL,
		.cid = VIDEO_CID_CONTRAST,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 9,
		.selector = UVC_PU_GAIN_CONTROL,
		.cid = VIDEO_CID_GAIN,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 3,
		.selector = UVC_PU_SATURATION_CONTROL,
		.cid = VIDEO_CID_SATURATION,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 2,
		.bit = 6,
		.selector = UVC_PU_WHITE_BALANCE_TEMP_CONTROL,
		.cid = VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
		.type = UVC_CONTROL_UNSIGNED,
	},
};

static const struct uvc_control_map uvc_control_map_su[] = {
	{
		.size = 1,
		.bit = 0,
		.selector = UVC_SU_INPUT_SELECT_CONTROL,
		.cid = VIDEO_CID_TEST_PATTERN,
		.type = UVC_CONTROL_UNSIGNED,
	},
};

static const struct uvc_control_map uvc_control_map_xu[] = {
	{
		.size = 4,
		.bit = 0,
		.selector = UVC_XU_BASE_CONTROL + 0,
		.cid = VIDEO_CID_PRIVATE_BASE + 0,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 1,
		.selector = UVC_XU_BASE_CONTROL + 1,
		.cid = VIDEO_CID_PRIVATE_BASE + 1,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 2,
		.selector = UVC_XU_BASE_CONTROL + 2,
		.cid = VIDEO_CID_PRIVATE_BASE + 2,
		.type = UVC_CONTROL_UNSIGNED,
	},
	{
		.size = 4,
		.bit = 3,
		.selector = UVC_XU_BASE_CONTROL + 3,
		.cid = VIDEO_CID_PRIVATE_BASE + 3,
		.type = UVC_CONTROL_UNSIGNED,
	},
};

/* Get the format and frame descriptors selected for the given VideoStreaming interface. */
static void uvc_get_vs_fmtfrm_desc(const struct device *dev,
				   struct uvc_format_descriptor **const format_desc,
				   struct uvc_frame_discrete_descriptor **const frame_desc)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	int i;

	*format_desc = NULL;
	for (i = 0; i < ARRAY_SIZE(cfg->desc->if1_fmts); i++) {
		struct uvc_format_descriptor *desc = &cfg->desc->if1_fmts[i].fmt;

		LOG_DBG("Walking through format %u, subtype %u, index %u, ptr %p",
			i, desc->bDescriptorSubtype, desc->bFormatIndex, desc);

		if ((desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
		     desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG) &&
		     desc->bFormatIndex == data->format_id) {
			*format_desc = desc;
			break;
		}
	}

	*frame_desc = NULL;
	for (i++; i < ARRAY_SIZE(cfg->desc->if1_fmts); i++) {
		struct uvc_frame_discrete_descriptor *desc = &cfg->desc->if1_fmts[i].frm_disc;

		LOG_DBG("Walking through frame %u, subtype %u, index %u, ptr %p",
			i, desc->bDescriptorSubtype, desc->bFrameIndex, desc);

		if (desc->bDescriptorSubtype != UVC_VS_FRAME_UNCOMPRESSED &&
		    desc->bDescriptorSubtype != UVC_VS_FRAME_MJPEG) {
			break;
		}

		if (desc->bFrameIndex == data->frame_id) {
			*frame_desc = desc;
			break;
		}
	}
}

static uint8_t uvc_get_bulk_in(const struct device *dev)
{
	const struct uvc_config *cfg = dev->config;
	struct usbd_context *uds_ctx = usbd_class_get_ctx(cfg->c_data);
	struct uvc_desc *desc = cfg->desc;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if1_ep_hs.bEndpointAddress;
	}

	return desc->if1_ep_fs.bEndpointAddress;
}

static size_t uvc_get_bulk_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return 512U;
	}

	return 64U;
}

static int uvc_get_vs_probe_format_index(const struct device *dev, struct uvc_probe *const probe,
					 const uint8_t request)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	uint8_t max = 0;

	for (int i = 0; i < ARRAY_SIZE(cfg->desc->if1_fmts); i++) {
		struct uvc_format_descriptor *desc = &cfg->desc->if1_fmts[i].fmt;

		max += desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
		       desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG;
	}

	switch (request) {
	case UVC_GET_RES:
		__fallthrough;
	case UVC_GET_MIN:
		probe->bFormatIndex = 1;
		break;
	case UVC_GET_MAX:
		probe->bFormatIndex = max;
		break;
	case UVC_GET_CUR:
		probe->bFormatIndex = data->format_id;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int uvc_get_vs_probe_frame_index(const struct device *dev, struct uvc_probe *const probe,
					const uint8_t request)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	uint8_t max = 0;
	int i;

	/* Search the current format */
	for (i = 0; i < ARRAY_SIZE(cfg->desc->if1_fmts); i++) {
		struct uvc_format_descriptor *desc = &cfg->desc->if1_fmts[i].fmt;

		if ((desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
		     desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG) &&
		    desc->bFormatIndex == data->format_id) {
			break;
		}
	}

	/* Seek until the next format */
	for (i++; i < ARRAY_SIZE(cfg->desc->if1_fmts); i++) {
		struct uvc_frame_discrete_descriptor *desc = &cfg->desc->if1_fmts[i].frm_disc;

		if (desc->bDescriptorSubtype != UVC_VS_FRAME_UNCOMPRESSED &&
		    desc->bDescriptorSubtype != UVC_VS_FRAME_MJPEG) {
			break;
		}
		max++;
	}

	switch (request) {
	case UVC_GET_RES:
		__fallthrough;
	case UVC_GET_MIN:
		probe->bFrameIndex = 1;
		break;
	case UVC_GET_MAX:
		probe->bFrameIndex = max;
		break;
	case UVC_GET_CUR:
		probe->bFrameIndex = data->frame_id;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int uvc_get_vs_probe_frame_interval(const struct device *dev, struct uvc_probe *const probe,
					   const uint8_t request)
{
	struct uvc_data *data = dev->data;
	struct uvc_format_descriptor *format_desc = NULL;
	struct uvc_frame_discrete_descriptor *frame_desc = NULL;
	int max;

	uvc_get_vs_fmtfrm_desc(dev, &format_desc, &frame_desc);
	if (format_desc == NULL || frame_desc == NULL) {
		LOG_DBG("Selected format ID or frame ID not found");
		return -EINVAL;
	}

	switch (request) {
	case UVC_GET_MIN:
		probe->dwFrameInterval = sys_cpu_to_le32(frame_desc->dwFrameInterval[0]);
		break;
	case UVC_GET_MAX:
		max = frame_desc->bFrameIntervalType - 1;
		probe->dwFrameInterval = sys_cpu_to_le32(frame_desc->dwFrameInterval[max]);
		break;
	case UVC_GET_RES:
		probe->dwFrameInterval = sys_cpu_to_le32(1);
		break;
	case UVC_GET_CUR:
		probe->dwFrameInterval = sys_cpu_to_le32(data->video_frmival.numerator);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int uvc_get_vs_probe_max_size(const struct device *dev, struct uvc_probe *const probe,
				     const uint8_t request)
{
	struct uvc_data *data = dev->data;
	struct video_format *fmt = &data->video_fmt;
	uint32_t max_frame_size = MAX(fmt->pitch, fmt->width) * fmt->height;
	uint32_t max_payload_size = max_frame_size + UVC_MAX_HEADER_LENGTH;

	switch (request) {
	case UVC_GET_MIN:
		__fallthrough;
	case UVC_GET_MAX:
		__fallthrough;
	case UVC_GET_CUR:
		probe->dwMaxPayloadTransferSize = sys_cpu_to_le32(max_payload_size);
		probe->dwMaxVideoFrameSize = sys_cpu_to_le32(max_frame_size);
		break;
	case UVC_GET_RES:
		probe->dwMaxPayloadTransferSize = sys_cpu_to_le32(1);
		probe->dwMaxVideoFrameSize = sys_cpu_to_le32(1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int uvc_get_vs_format_from_desc(const struct device *dev, struct video_format *const fmt)
{
	struct uvc_data *data = dev->data;
	struct uvc_format_descriptor *format_desc = NULL;
	struct uvc_frame_discrete_descriptor *frame_desc = NULL;

	/* Update the format based on the probe message from the host */
	uvc_get_vs_fmtfrm_desc(dev, &format_desc, &frame_desc);
	if (format_desc == NULL || frame_desc == NULL) {
		LOG_ERR("Invalid format ID (%u) and/or frame ID (%u)",
			data->format_id, data->frame_id);
		return -EINVAL;
	}

	/* Translate between UVC pixel formats and Video pixel formats */
	if (format_desc->bDescriptorSubtype == UVC_VS_FORMAT_MJPEG) {
		fmt->pixelformat = VIDEO_PIX_FMT_JPEG;

		LOG_DBG("Found descriptor for format %u, frame %u, MJPEG",
			format_desc->bFormatIndex, frame_desc->bFrameIndex);
	} else {
		struct uvc_format_uncomp_descriptor *format_uncomp_desc = (void *)format_desc;

		fmt->pixelformat = uvc_guid_to_fourcc(format_uncomp_desc->guidFormat);

		LOG_DBG("Found descriptor for format %u, frame %u, GUID '%.4s', pixfmt %04x",
			format_uncomp_desc->bFormatIndex, frame_desc->bFrameIndex,
			format_uncomp_desc->guidFormat, fmt->pixelformat);
	}

	/* Fill the format according to what the host selected */
	fmt->width = frame_desc->wWidth;
	fmt->height = frame_desc->wHeight;
	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	return 0;
}

static int uvc_get_vs_probe_struct(const struct device *dev, struct uvc_probe *const probe,
				   const uint8_t request)
{
	struct uvc_data *data = dev->data;
	struct video_format *fmt = &data->video_fmt;
	int ret;

	ret = uvc_get_vs_probe_format_index(dev, probe, request);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_get_vs_probe_frame_index(dev, probe, request);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_get_vs_format_from_desc(dev, fmt);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_get_vs_probe_frame_interval(dev, probe, request);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_get_vs_probe_max_size(dev, probe, request);
	if (ret != 0) {
		return ret;
	}

	probe->dwClockFrequency = sys_cpu_to_le32(1);
	probe->bmFramingInfo = UVC_BMFRAMING_INFO_FID | UVC_BMFRAMING_INFO_EOF;
	probe->bPreferedVersion = 1;
	probe->bMinVersion = 1;
	probe->bMaxVersion = 1;
	probe->bUsage = 0;
	probe->bBitDepthLuma = 0;
	probe->bmSettings = 0;
	probe->bMaxNumberOfRefFramesPlus1 = 1;
	probe->bmRateControlModes = 0;
	probe->bmLayoutPerStream = 0;
	probe->wKeyFrameRate = sys_cpu_to_le16(0);
	probe->wPFrameRate = sys_cpu_to_le16(0);
	probe->wCompQuality = sys_cpu_to_le16(0);
	probe->wCompWindowSize = sys_cpu_to_le16(0);
	probe->wDelay = sys_cpu_to_le16(1);

	return 0;
}

static int uvc_get_vs_probe(const struct device *dev, struct net_buf *const buf,
			    const struct usb_setup_packet *const setup)
{
	struct uvc_data *data = dev->data;
	const size_t size = MIN(net_buf_tailroom(buf),
				MIN(sizeof(struct uvc_probe), setup->wLength));
	struct uvc_probe probe = {0};
	int ret;

	switch (setup->bRequest) {
	case UVC_GET_INFO:
		if (size < 1) {
			return -ENOTSUP;
		}

		net_buf_add_u8(buf, UVC_INFO_SUPPORTS_GET);
		return 0;
	case UVC_GET_LEN:
		if (size < 2) {
			return -ENOTSUP;
		}

		net_buf_add_le16(buf, sizeof(struct uvc_probe));
		return 0;
	case UVC_GET_DEF:
		net_buf_add_mem(buf, &data->default_probe, size);
		return 0;
	case UVC_GET_MIN:
		__fallthrough;
	case UVC_GET_RES:
		__fallthrough;
	case UVC_GET_MAX:
		__fallthrough;
	case UVC_GET_CUR:
		ret = uvc_get_vs_probe_struct(dev, &probe, setup->bRequest);
		if (ret != 0) {
			return ret;
		}

		net_buf_add_mem(buf, &probe, size);
		return 0;
	default:
		return -EINVAL;
	}
}

static int uvc_set_vs_probe(const struct device *dev, const struct net_buf *const buf)
{
	struct uvc_data *data = dev->data;
	const size_t size = MIN(sizeof(struct uvc_probe), buf->len);
	struct uvc_probe probe = {0};
	struct uvc_probe max = {0};
	int ret;

	memcpy(&probe, buf->data, size);

	ret = uvc_get_vs_probe_struct(dev, &max, UVC_GET_MAX);
	if (ret != 0) {
		return ret;
	}

	if (probe.bFrameIndex > max.bFrameIndex) {
		LOG_WRN("The bFrameIndex %u requested is beyond the max %u",
			probe.bFrameIndex, max.bFrameIndex);
		return -ERANGE;
	}

	if (probe.bFormatIndex > max.bFormatIndex) {
		LOG_WRN("The bFormatIndex %u requested is beyond the max %u",
			probe.bFormatIndex, max.bFormatIndex);
		return -ERANGE;
	}

	if (probe.dwFrameInterval != 0) {
		data->video_frmival.numerator = sys_le32_to_cpu(probe.dwFrameInterval);
		data->video_frmival.denominator = USEC_PER_SEC * 100;
	}

	if (probe.bFrameIndex != 0) {
		data->frame_id = probe.bFrameIndex;
	}

	if (probe.bFormatIndex != 0) {
		data->format_id = probe.bFormatIndex;
	}

	return 0;
}

static int uvc_get_vs_commit(const struct device *dev, struct net_buf *const buf,
			     const struct usb_setup_packet *const setup)
{
	if (setup->bRequest != UVC_GET_CUR) {
		LOG_WRN("Invalid commit bRequest %u", setup->bRequest);
		return -EINVAL;
	}

	return uvc_get_vs_probe(dev, buf, setup);
}

static int uvc_set_vs_commit(const struct device *dev, const struct net_buf *const buf)
{
	struct uvc_data *data = dev->data;
	struct video_format *fmt = &data->video_fmt;
	struct video_frmival *frmival = &data->video_frmival;
	int ret;

	__ASSERT_NO_MSG(data->video_dev != NULL);

	ret = uvc_set_vs_probe(dev, buf);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("Host selected format '%s' %ux%u, frame interval %u/%u",
		VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
		frmival->numerator, frmival->denominator);

	if (atomic_test_bit(&data->state, UVC_STATE_STREAM_READY)) {
		atomic_set_bit(&data->state, UVC_STATE_STREAM_RESTART);
	}

	atomic_set_bit(&data->state, UVC_STATE_STREAM_READY);
	uvc_flush_queue(dev);

	return 0;
}

static void uvc_get_vc_conversion_map(const uint32_t cid, const int **const map, int *const map_sz)
{
	static const int ct_ae_mode[] = {
		[0] = VIDEO_EXPOSURE_MANUAL,
		[1] = VIDEO_EXPOSURE_AUTO,
		[2] = VIDEO_EXPOSURE_SHUTTER_PRIORITY,
		[3] = VIDEO_EXPOSURE_APERTURE_PRIORITY,
	};

	switch (cid) {
	case VIDEO_CID_EXPOSURE_AUTO:
		*map = ct_ae_mode;
		*map_sz = ARRAY_SIZE(ct_ae_mode);
		break;
	default:
		*map = NULL;
		*map_sz = 0;
		break;
	}
}

/*
 * Convert the Zephyr Video control IDs (CID) to UVC Video Control (VC) IDs.
 */
static int uvc_convert_cid_to_vc(const uint32_t cid, int64_t *const val64)
{
	const int *map;
	int map_sz;

	uvc_get_vc_conversion_map(cid, &map, &map_sz);
	if (map == NULL) {
		/* No conversion needed */
		return 0;
	}

	for (int i = 0; i < map_sz; i++) {
		if (map[i] == *val64) {
			*val64 = BIT(i);
			return 0;
		}
	}

	return -ENOTSUP;
}

/*
 * Convert the UVC Video Control (VC) IDs to Zephyr Video control IDs (CID).
 */
static int uvc_convert_vc_to_cid(const int32_t cid, int64_t *const val64)
{
	const int *map;
	int map_sz;

	uvc_get_vc_conversion_map(cid, &map, &map_sz);
	if (map == NULL) {
		/* No conversion needed */
		return 0;
	}

	for (int i = 0; i < map_sz; i++) {
		if (BIT(i) & *val64) {
			*val64 = map[i];
			return 0;
		}
	}

	return -ENOTSUP;
}

static int uvc_get_vc_ctrl(const struct device *dev, struct net_buf *const buf,
			   const struct usb_setup_packet *const setup,
			   const struct uvc_control_map *const map)
{
	struct uvc_data *data = dev->data;
	const struct device *video_dev = data->video_dev;
	struct video_ctrl_query cq = {.id = map->cid, .dev = video_dev};
	struct video_control ctrl = {.id = map->cid};
	const size_t size = MIN(net_buf_tailroom(buf),
				MIN(sizeof(struct uvc_probe), setup->wLength));
	int64_t val64;
	int ret;

	__ASSERT_NO_MSG(video_dev != NULL);

	ret = video_query_ctrl(&cq);
	if (ret != 0) {
		LOG_ERR("Failed to query %s for control 0x%x", video_dev->name, cq.id);
		return ret;
	}

	LOG_INF("Responding to GET control '%s', size %u", cq.name, map->size);

	if (cq.type != VIDEO_CTRL_TYPE_BOOLEAN && cq.type != VIDEO_CTRL_TYPE_MENU &&
	    cq.type != VIDEO_CTRL_TYPE_INTEGER && cq.type != VIDEO_CTRL_TYPE_INTEGER64) {
		LOG_ERR("Unsupported control type %u", cq.type);
		return -ENOTSUP;
	}

	switch (setup->bRequest) {
	case UVC_GET_INFO:
		if (size < 1) {
			return -ENOTSUP;
		}
		net_buf_add_u8(buf, UVC_INFO_SUPPORTS_GET | UVC_INFO_SUPPORTS_SET);
		return 0;
	case UVC_GET_LEN:
		if (size < 2) {
			return -ENOTSUP;
		}
		net_buf_add_le16(buf, map->size);
		return 0;
	case UVC_GET_CUR:
		ret = video_get_ctrl(video_dev, &ctrl);
		if (ret != 0) {
			LOG_INF("Failed to query %s", video_dev->name);
			return ret;
		}

		val64 = (cq.type == VIDEO_CTRL_TYPE_INTEGER64) ? ctrl.val64 : ctrl.val;
		break;
	case UVC_GET_MIN:
		val64 = (cq.type == VIDEO_CTRL_TYPE_INTEGER64) ? cq.range.min64 : cq.range.min;
		break;
	case UVC_GET_MAX:
		val64 = (cq.type == VIDEO_CTRL_TYPE_INTEGER64) ? cq.range.max64 : cq.range.max;
		break;
	case UVC_GET_RES:
		val64 = (cq.type == VIDEO_CTRL_TYPE_INTEGER64) ? cq.range.step64 : cq.range.step;
		break;
	case UVC_GET_DEF:
		val64 = (cq.type == VIDEO_CTRL_TYPE_INTEGER64) ? cq.range.def64 : cq.range.def;
		break;
	default:
		LOG_WRN("Unsupported request type %u", setup->bRequest);
		return -ENOTSUP;
	}

	if (size < map->size) {
		LOG_WRN("Buffer too small (%u bytes) or unexpected size requested (%u bytes)",
			net_buf_tailroom(buf), setup->wLength);
		return -ENOTSUP;
	}

	ret = uvc_convert_cid_to_vc(cq.id, &val64);
	if (ret != 0) {
		return ret;
	}

	switch (map->type) {
	case UVC_CONTROL_SIGNED:
		if (map->size == 1) {
			net_buf_add_u8(buf, CLAMP(val64, INT8_MIN, INT8_MAX));
		} else if (map->size == 2) {
			net_buf_add_le16(buf, CLAMP(val64, INT16_MIN, INT16_MAX));
		} else if (map->size == 3) {
			net_buf_add_le24(buf, CLAMP(val64, -0x800000, 0x7fffff));
		} else if (map->size == 4) {
			net_buf_add_le32(buf, CLAMP(val64, INT32_MIN, INT32_MAX));
		} else {
			LOG_WRN("Unsupported integer size %u for UVC control value", map->size);
			return -ENOTSUP;
		}
		break;
	case UVC_CONTROL_UNSIGNED:
		if (map->size == 1) {
			net_buf_add_u8(buf, CLAMP(val64, 0, UINT8_MAX));
		} else if (map->size == 2) {
			net_buf_add_le16(buf, CLAMP(val64, 0, UINT16_MAX));
		} else if (map->size == 3) {
			net_buf_add_le24(buf, CLAMP(val64, 0, 0xffffff));
		} else if (map->size == 4) {
			net_buf_add_le32(buf, CLAMP(val64, 0, UINT32_MAX));
		} else {
			LOG_WRN("Unsupported integer size %u for UVC control value", map->size);
			return -ENOTSUP;
		}
		break;
	}

	return 0;
}

static int uvc_set_vc_ctrl(const struct device *dev, const struct net_buf *const buf_in,
			   const struct uvc_control_map *const map)
{
	struct uvc_data *data = dev->data;
	const struct device *video_dev = data->video_dev;
	struct video_ctrl_query cq = {.id = map->cid, .dev = video_dev};
	struct video_control ctrl = {.id = map->cid};
	struct net_buf buf;
	int64_t val64 = 0;
	int ret;

	__ASSERT_NO_MSG(video_dev != NULL);

	/* Local copy that can be modified, so that <net_buf.h> functions can be used */
	memcpy(&buf, buf_in, sizeof(buf));

	ret = video_query_ctrl(&cq);
	if (ret != 0) {
		LOG_ERR("Failed to query the video device for control 0x%08x", cq.id);
		return ret;
	}

	if (cq.type != VIDEO_CTRL_TYPE_BOOLEAN && cq.type != VIDEO_CTRL_TYPE_MENU &&
	    cq.type != VIDEO_CTRL_TYPE_INTEGER && cq.type != VIDEO_CTRL_TYPE_INTEGER64) {
		LOG_ERR("Unsupported control type %u", cq.type);
		return -ENOTSUP;
	}

	if (buf.len < map->size) {
		LOG_ERR("USB message size %u too short for control 0x%08x", buf.len, cq.id);
		return -ENOTSUP;
	}

	switch (map->type) {
	case UVC_CONTROL_SIGNED:
		if (map->size == 1) {
			val64 = (int8_t)net_buf_remove_u8(&buf);
		} else if (map->size == 2) {
			val64 = (int16_t)net_buf_remove_le16(&buf);
		} else if (map->size == 3) {
			val64 = (int32_t)net_buf_remove_le24(&buf);
		} else if (map->size == 4) {
			val64 = (int32_t)net_buf_remove_le32(&buf);
		} else {
			return -ENOTSUP;
		}
		break;
	case UVC_CONTROL_UNSIGNED:
		if (map->size == 1) {
			val64 = net_buf_remove_u8(&buf);
		} else if (map->size == 2) {
			val64 = net_buf_remove_le16(&buf);
		} else if (map->size == 3) {
			val64 = net_buf_remove_le24(&buf);
		} else if (map->size == 4) {
			val64 = net_buf_remove_le32(&buf);
		} else {
			return -ENOTSUP;
		}
		break;
	}

	ret = uvc_convert_vc_to_cid(cq.id, &val64);
	if (ret != 0) {
		return ret;
	}

	if (cq.type == VIDEO_CTRL_TYPE_INTEGER64) {
		ctrl.val64 = val64;
	} else {
		ctrl.val = val64;
	}

	LOG_DBG("Setting control 0x%08x to %llu", cq.id, val64);

	ret = video_set_ctrl(video_dev, &ctrl);
	if (ret != 0) {
		LOG_ERR("Failed to configure target video device");
		return ret;
	}

	return 0;
}

static int uvc_get_errno(const struct device *dev, struct net_buf *const buf,
			       const struct usb_setup_packet *const setup)
{
	struct uvc_data *data = dev->data;
	const size_t size = MIN(net_buf_tailroom(buf),
				MIN(sizeof(struct uvc_probe), setup->wLength));

	switch (setup->bRequest) {
	case UVC_GET_INFO:
		if (size < 1) {
			return -ENOTSUP;
		}
		net_buf_add_u8(buf, UVC_INFO_SUPPORTS_GET);
		break;
	case UVC_GET_CUR:
		if (size < 1) {
			return -ENOTSUP;
		}
		net_buf_add_u8(buf, data->err);
		break;
	default:
		LOG_WRN("Unsupported request type %u", setup->bRequest);
		return -ENOTSUP;
	}

	return 0;
}

static void uvc_set_errno(const struct device *dev, const int ret)
{
	struct uvc_data *data = dev->data;

	if (ret == 0) {
		data->err = 0;
	} else if (ret == EBUSY || ret == EAGAIN || ret == EINPROGRESS || ret == EALREADY) {
		data->err = UVC_ERR_NOT_READY;
	} else if (ret == EOVERFLOW || ret == ERANGE || ret == E2BIG) {
		data->err = UVC_ERR_OUT_OF_RANGE;
	} else if (ret == EDOM || ret == EINVAL) {
		data->err = UVC_ERR_INVALID_VALUE_WITHIN_RANGE;
	} else if (ret == ENODEV || ret == ENOTSUP || ret == ENOSYS) {
		data->err = UVC_ERR_INVALID_REQUEST;
	} else {
		data->err = UVC_ERR_UNKNOWN;
	}
}

static int uvc_get_control_op(const struct device *dev, const struct usb_setup_packet *const setup,
			      const struct uvc_control_map **const map)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	const struct uvc_control_map *list = NULL;
	size_t list_sz;
	uint8_t ifnum = (setup->wIndex >> 0) & 0xff;
	uint8_t unit_id = setup->wIndex >> 8;
	uint8_t selector = setup->wValue >> 8;
	uint8_t subtype = 0;

	/* VideoStreaming operation */

	if (ifnum == cfg->desc->if1.bInterfaceNumber) {
		switch (selector) {
		case UVC_VS_PROBE_CONTROL:
			LOG_INF("Host sent a VideoStreaming PROBE control");
			return UVC_OP_VS_PROBE;
		case UVC_VS_COMMIT_CONTROL:
			LOG_INF("Host sent a VideoStreaming COMMIT control");
			return UVC_OP_VS_COMMIT;
		default:
			LOG_ERR("Invalid probe/commit operation for bInterfaceNumber %u", ifnum);
			return UVC_OP_INVALID;
		}
	}

	/* VideoControl operation */

	if (ifnum != cfg->desc->if0.bInterfaceNumber) {
		LOG_WRN("Interface %u not found", ifnum);
		data->err = UVC_ERR_INVALID_UNIT;
		return UVC_OP_RETURN_ERROR;
	}

	if (unit_id == 0) {
		return UVC_OP_GET_ERRNO;
	}

	for (int i = UVC_IDX_VC_UNIT;; i++) {
		struct uvc_unit_descriptor *desc = (void *)cfg->fs_desc[i];

		if (desc->bDescriptorType != USB_DESC_CS_INTERFACE ||
		    (desc->bDescriptorSubtype != UVC_VC_INPUT_TERMINAL &&
		     desc->bDescriptorSubtype != UVC_VC_ENCODING_UNIT &&
		     desc->bDescriptorSubtype != UVC_VC_SELECTOR_UNIT &&
		     desc->bDescriptorSubtype != UVC_VC_EXTENSION_UNIT &&
		     desc->bDescriptorSubtype != UVC_VC_PROCESSING_UNIT)) {
			break;
		}

		if (unit_id == desc->bUnitID) {
			subtype = desc->bDescriptorSubtype;
			break;
		}
	}

	if (subtype == 0) {
		goto err;
	}

	switch (subtype) {
	case UVC_VC_INPUT_TERMINAL:
		list = uvc_control_map_ct;
		list_sz = ARRAY_SIZE(uvc_control_map_ct);
		break;
	case UVC_VC_SELECTOR_UNIT:
		list = uvc_control_map_su;
		list_sz = ARRAY_SIZE(uvc_control_map_su);
		break;
	case UVC_VC_PROCESSING_UNIT:
		list = uvc_control_map_pu;
		list_sz = ARRAY_SIZE(uvc_control_map_pu);
		break;
	case UVC_VC_EXTENSION_UNIT:
		list = uvc_control_map_xu;
		list_sz = ARRAY_SIZE(uvc_control_map_xu);
		break;
	default:
		CODE_UNREACHABLE;
	}

	*map = NULL;
	for (int i = 0; i < list_sz; i++) {
		if (list[i].selector == selector) {
			*map = &list[i];
			break;
		}
	}
	if (*map == NULL) {
		goto err;
	}

	return UVC_OP_VC_CTRL;
err:
	LOG_WRN("No control matches selector %u and bUnitID %u", selector, unit_id);
	data->err = UVC_ERR_INVALID_CONTROL;
	return UVC_OP_RETURN_ERROR;
}

static int uvc_control_to_host(struct usbd_class_data *const c_data,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct uvc_control_map *map = NULL;
	uint8_t request = setup->bRequest;

	LOG_INF("Host sent a %s request, wValue 0x%04x, wIndex 0x%04x, wLength %u",
		request == UVC_GET_CUR ? "GET_CUR" : request == UVC_GET_MIN ? "GET_MIN" :
		request == UVC_GET_MAX ? "GET_MAX" : request == UVC_GET_RES ? "GET_RES" :
		request == UVC_GET_LEN ? "GET_LEN" : request == UVC_GET_DEF ? "GET_DEF" :
		request == UVC_GET_INFO ? "GET_INFO" : "bad",
		setup->wValue, setup->wIndex, setup->wLength);

	switch (uvc_get_control_op(dev, setup, &map)) {
	case UVC_OP_VS_PROBE:
		errno = -uvc_get_vs_probe(dev, buf, setup);
		break;
	case UVC_OP_VS_COMMIT:
		errno = -uvc_get_vs_commit(dev, buf, setup);
		break;
	case UVC_OP_VC_CTRL:
		errno = -uvc_get_vc_ctrl(dev, buf, setup, map);
		break;
	case UVC_OP_GET_ERRNO:
		errno = -uvc_get_errno(dev, buf, setup);
		break;
	case UVC_OP_RETURN_ERROR:
		errno = EINVAL;
		return 0;
	default:
		LOG_WRN("Unhandled operation, stalling control command");
		errno = EINVAL;
	}

	uvc_set_errno(dev, errno);

	return 0;
}

static int uvc_control_to_dev(struct usbd_class_data *const c_data,
			      const struct usb_setup_packet *const setup,
			      const struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct uvc_control_map *map = NULL;

	if (setup->bRequest != UVC_SET_CUR) {
		LOG_WRN("Host issued a control write message but the bRequest is not SET_CUR");
		errno = ENOMEM;
		goto end;
	}

	LOG_INF("Host sent a SET_CUR request, wValue 0x%04x, wIndex 0x%04x, wLength %u",
		setup->wValue, setup->wIndex, setup->wLength);

	switch (uvc_get_control_op(dev, setup, &map)) {
	case UVC_OP_VS_PROBE:
		errno = -uvc_set_vs_probe(dev, buf);
		break;
	case UVC_OP_VS_COMMIT:
		errno = -uvc_set_vs_commit(dev, buf);
		break;
	case UVC_OP_VC_CTRL:
		errno = -uvc_set_vc_ctrl(dev, buf, map);
		break;
	case UVC_OP_RETURN_ERROR:
		errno = EINVAL;
		return 0;
	default:
		LOG_WRN("Unhandled operation, stalling control command");
		errno = EINVAL;
	}
end:
	uvc_set_errno(dev, errno);

	return 0;
}

/* UVC descriptor handling */

static void *uvc_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct uvc_config *const cfg = dev->config;
	struct uvc_desc *desc = cfg->desc;

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		desc->if1_hdr.bEndpointAddress = desc->if1_ep_hs.bEndpointAddress;
		return cfg->hs_desc;
	}

	desc->if1_hdr.bEndpointAddress = desc->if1_ep_fs.bEndpointAddress;
	return cfg->fs_desc;
}

static int uvc_assign_desc(const struct device *dev, void *const desc,
			   const bool add_to_fs, const bool add_to_hs)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	static const struct usb_desc_header nil_desc;

	if (add_to_fs) {
		if (data->fs_desc_idx + 1 >= UVC_MAX_FS_DESC) {
			goto err;
		}
		cfg->fs_desc[data->fs_desc_idx] = desc;
		data->fs_desc_idx++;
		cfg->fs_desc[data->fs_desc_idx] = (struct usb_desc_header *)&nil_desc;
	}

	if (USBD_SUPPORTS_HIGH_SPEED && add_to_hs) {
		if (data->hs_desc_idx + 1 >= UVC_MAX_HS_DESC) {
			goto err;
		}
		cfg->hs_desc[data->hs_desc_idx] = desc;
		data->hs_desc_idx++;
		cfg->hs_desc[data->hs_desc_idx] = (struct usb_desc_header *)&nil_desc;
	}

	return 0;
err:
	LOG_ERR("Out of descriptor pointers, raise CONFIG_USBD_VIDEO_MAX_FORMATS above %u",
		CONFIG_USBD_VIDEO_MAX_FORMATS);
	return -ENOMEM;
}

static union uvc_fmt_desc *uvc_new_fmt_desc(const struct device *dev)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	void *desc;
	int ret;

	BUILD_ASSERT(CONFIG_USBD_VIDEO_MAX_FORMATS == ARRAY_SIZE(cfg->desc->if1_fmts));

	if (data->fmt_desc_idx >= CONFIG_USBD_VIDEO_MAX_FORMATS) {
		LOG_ERR("Out of descriptor pointers, raise CONFIG_USBD_VIDEO_MAX_FORMATS above %u",
			CONFIG_USBD_VIDEO_MAX_FORMATS);
		return NULL;
	}

	desc = &cfg->desc->if1_fmts[data->fmt_desc_idx];
	data->fmt_desc_idx++;

	LOG_DBG("Allocated format/frame descriptor %u (%p)", data->fmt_desc_idx, desc);

	ret = uvc_assign_desc(dev, desc, true, true);
	if (ret != 0) {
		return NULL;
	}

	return desc;
}

static int uvc_add_vs_format_desc(const struct device *dev,
				  struct uvc_format_descriptor **const format_desc,
				  uint32_t fourcc)
{
	const struct uvc_config *cfg = dev->config;

	__ASSERT_NO_MSG(format_desc != NULL);

	if (fourcc == VIDEO_PIX_FMT_JPEG) {
		struct uvc_format_mjpeg_descriptor *desc;

		LOG_INF("Adding format descriptor #%u for MJPEG",
			cfg->desc->if1_hdr.bNumFormats + 1);

		desc = &uvc_new_fmt_desc(dev)->fmt_mjpeg;
		if (desc == NULL) {
			return -ENOMEM;
		}

		desc->bDescriptorType = USB_DESC_CS_INTERFACE;
		desc->bFormatIndex = cfg->desc->if1_hdr.bNumFormats + 1;
		desc->bLength = sizeof(*desc);
		desc->bDescriptorSubtype = UVC_VS_FORMAT_MJPEG;
		desc->bDefaultFrameIndex = 1;
		cfg->desc->if1_hdr.bNumFormats++;
		cfg->desc->if1_hdr.wTotalLength += desc->bLength;
		*format_desc = (struct uvc_format_descriptor *)desc;
	} else {
		struct uvc_format_uncomp_descriptor *desc;

		LOG_INF("Adding format descriptor #%u for '%s'",
			cfg->desc->if1_hdr.bNumFormats + 1, VIDEO_FOURCC_TO_STR(fourcc));

		desc = &uvc_new_fmt_desc(dev)->fmt_uncomp;
		if (desc == NULL) {
			return -ENOMEM;
		}

		desc->bDescriptorType = USB_DESC_CS_INTERFACE;
		desc->bFormatIndex = cfg->desc->if1_hdr.bNumFormats + 1;
		desc->bLength = sizeof(*desc);
		desc->bDescriptorSubtype = UVC_VS_FORMAT_UNCOMPRESSED;
		uvc_fourcc_to_guid(desc->guidFormat, fourcc);
		desc->bBitsPerPixel = video_bits_per_pixel(fourcc);
		desc->bDefaultFrameIndex = 1;
		cfg->desc->if1_hdr.bNumFormats++;
		cfg->desc->if1_hdr.wTotalLength += desc->bLength;
		*format_desc = (struct uvc_format_descriptor *)desc;
	}

	__ASSERT_NO_MSG(*format_desc != NULL);

	return 0;
}

static int uvc_compare_frmival_desc(const void *const a, const void *const b)
{
	uint32_t ia, ib;

	/* Copy in case a and b are not 32-bit aligned */
	memcpy(&ia, a, sizeof(uint32_t));
	memcpy(&ib, b, sizeof(uint32_t));

	return ib - ia;
}

static void uvc_set_vs_bitrate_range(struct uvc_frame_discrete_descriptor *const desc,
				     const uint64_t frmival_nsec,
				     const struct video_format *const fmt)
{
	uint32_t bitrate_min = sys_le32_to_cpu(desc->dwMinBitRate);
	uint32_t bitrate_max = sys_le32_to_cpu(desc->dwMaxBitRate);
	uint32_t bitrate;

	/* Multiplication/division in this order to avoid overflow */
	bitrate = MAX(fmt->pitch, fmt->width) * frmival_nsec / (NSEC_PER_SEC / 100) * fmt->height;

	/* Extend the min/max value to include the bitrate of this format */
	bitrate_min = MIN(bitrate_min, bitrate);
	bitrate_max = MAX(bitrate_max, bitrate);

	if (bitrate_min > bitrate_max) {
		LOG_WRN("The minimum bitrate is above the maximum bitrate");
	}

	if (bitrate_max == 0) {
		LOG_WRN("Maximum bitrate is zero");
	}

	desc->dwMinBitRate = sys_cpu_to_le32(bitrate_min);
	desc->dwMaxBitRate = sys_cpu_to_le32(bitrate_max);
}

static int uvc_add_vs_frame_interval(struct uvc_frame_discrete_descriptor *const desc,
				     const struct video_frmival *const frmival,
				     const struct video_format *const fmt)
{
	int i = desc->bFrameIntervalType;

	if (i >= CONFIG_USBD_VIDEO_MAX_FRMIVAL) {
		LOG_WRN("Out of frame interval fields");
		return -ENOSPC;
	}

	desc->dwFrameInterval[i] = sys_cpu_to_le32(video_frmival_nsec(frmival) / 100);
	desc->bFrameIntervalType++;
	desc->bLength += sizeof(uint32_t);

	uvc_set_vs_bitrate_range(desc, video_frmival_nsec(frmival), fmt);

	return 0;
}

static int uvc_add_vs_frame_desc(const struct device *dev,
				 struct uvc_format_descriptor *const format_desc,
				 const struct video_format *const fmt)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	struct uvc_frame_discrete_descriptor *desc;
	struct video_frmival_enum fie = {.format = fmt};
	uint32_t max_size = MAX(fmt->pitch, fmt->width) * fmt->height;

	__ASSERT_NO_MSG(data->video_dev != NULL);
	__ASSERT_NO_MSG(format_desc != NULL);

	LOG_INF("Adding frame descriptor #%u for %ux%u",
		format_desc->bNumFrameDescriptors + 1, fmt->width, fmt->height);

	desc = &uvc_new_fmt_desc(dev)->frm_disc;
	if (desc == NULL) {
		return -ENOMEM;
	}

	desc->bLength = sizeof(*desc) - CONFIG_USBD_VIDEO_MAX_FRMIVAL * sizeof(uint32_t);
	desc->bDescriptorType = USB_DESC_CS_INTERFACE;
	desc->bFrameIndex = format_desc->bNumFrameDescriptors + 1;
	desc->wWidth = sys_cpu_to_le16(fmt->width);
	desc->wHeight = sys_cpu_to_le16(fmt->height);
	desc->dwMaxVideoFrameBufferSize = sys_cpu_to_le32(max_size);
	desc->bDescriptorSubtype = (format_desc->bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED)
		? UVC_VS_FRAME_UNCOMPRESSED : UVC_VS_FRAME_MJPEG;
	desc->dwMinBitRate = sys_cpu_to_le32(UINT32_MAX);
	desc->dwMaxBitRate = sys_cpu_to_le32(0);

	/* Add the adwFrameInterval fields at the end of this descriptor */
	while (video_enum_frmival(data->video_dev, &fie) == 0) {
		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			LOG_DBG("Adding discrete frame interval %u", fie.index);
			uvc_add_vs_frame_interval(desc, &fie.discrete, fmt);
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			LOG_DBG("Adding stepwise frame interval %u", fie.index);
			uvc_add_vs_frame_interval(desc, &fie.stepwise.min, fmt);
			uvc_add_vs_frame_interval(desc, &fie.stepwise.max, fmt);
			break;
		default:
			CODE_UNREACHABLE;
		}
		fie.index++;
	}

	/* If no frame intrval supported, default to 30 FPS */
	if (desc->bFrameIntervalType == 0) {
		struct video_frmival frmival = {.numerator = 1, .denominator = 30};

		uvc_add_vs_frame_interval(desc, &frmival, fmt);
	}

	/* UVC requires the frame intervals to be sorted, but not Zephyr */
	qsort(desc->dwFrameInterval, desc->bFrameIntervalType,
		sizeof(*desc->dwFrameInterval), uvc_compare_frmival_desc);

	desc->dwDefaultFrameInterval = desc->dwFrameInterval[0];
	format_desc->bNumFrameDescriptors++;
	cfg->desc->if1_hdr.wTotalLength += desc->bLength;

	return 0;
}

static uint32_t uvc_get_mask(const struct device *video_dev,
			     const struct uvc_control_map *const list,
			     const size_t list_sz)
{
	uint32_t mask = 0;
	uint32_t ok;

	LOG_DBG("Querying which controls are supported:");

	for (int i = 0; i < list_sz; i++) {
		struct video_ctrl_query cq = {.id = list[i].cid, .dev = video_dev};

		ok = (video_query_ctrl(&cq) == 0);

		LOG_DBG("%s supports control 0x%02x: %s",
			video_dev->name, cq.id, ok ? "yes" : "no");

		mask |= ok << list[i].bit;
	}

	return mask;
}

static int uvc_init(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	int ret;

	__ASSERT_NO_MSG(data->video_dev != NULL);

	if (atomic_test_bit(&data->state, UVC_STATE_INITIALIZED)) {
		LOG_DBG("UVC instance '%s' is already initialized", dev->name);
		return 0;
	}

	cfg->desc->if1_hdr.wTotalLength = sys_le16_to_cpu(cfg->desc->if1_hdr.wTotalLength);
	cfg->desc->if1_hdr.wTotalLength += cfg->desc->if1_color.bLength;

	uvc_assign_desc(dev, &cfg->desc->if1_color, true, true);
	uvc_assign_desc(dev, &cfg->desc->if1_ep_fs, true, false);
	uvc_assign_desc(dev, &cfg->desc->if1_ep_hs, false, true);

	cfg->desc->if1_hdr.wTotalLength = sys_cpu_to_le16(cfg->desc->if1_hdr.wTotalLength);

	/* Generating the default probe message now that descriptors are complete */

	ret = uvc_get_vs_probe_struct(dev, &data->default_probe, UVC_GET_CUR);
	if (ret != 0) {
		LOG_ERR("init: failed to query the default probe");
		return ret;
	}

	atomic_set_bit(&data->state, UVC_STATE_INITIALIZED);

	return 0;
}

/* UVC public API */

void uvc_set_video_dev(const struct device *const dev, const struct device *const video_dev)
{
	struct uvc_data *data = dev->data;
	const struct uvc_config *cfg = dev->config;
	uint32_t mask = 0;

	data->video_dev = video_dev;

	/* Generate VideoControl descriptors (interface 0) */

	cfg->desc->if0_hdr.baInterfaceNr[0] = cfg->desc->if1.bInterfaceNumber;

	mask = uvc_get_mask(data->video_dev, uvc_control_map_ct, ARRAY_SIZE(uvc_control_map_ct));
	cfg->desc->if0_ct.bmControls[0] = mask >> 0;
	cfg->desc->if0_ct.bmControls[1] = mask >> 8;
	cfg->desc->if0_ct.bmControls[2] = mask >> 16;

	mask = uvc_get_mask(data->video_dev, uvc_control_map_pu, ARRAY_SIZE(uvc_control_map_pu));
	cfg->desc->if0_pu.bmControls[0] = mask >> 0;
	cfg->desc->if0_pu.bmControls[1] = mask >> 8;
	cfg->desc->if0_pu.bmControls[2] = mask >> 16;

	mask = uvc_get_mask(data->video_dev, uvc_control_map_xu, ARRAY_SIZE(uvc_control_map_xu));
	cfg->desc->if0_xu.bmControls[0] = mask >> 0;
	cfg->desc->if0_xu.bmControls[1] = mask >> 8;
	cfg->desc->if0_xu.bmControls[2] = mask >> 16;
	cfg->desc->if0_xu.bmControls[3] = mask >> 24;
}

int uvc_add_format(const struct device *const dev, const struct video_format *const fmt)
{
	struct uvc_data *data = dev->data;
	const struct uvc_config *cfg = dev->config;
	int ret;

	if (data->video_dev == NULL) {
		LOG_ERR("Video device not yet configured into UVC");
		return -EINVAL;
	}

	if (data->last_pix_fmt != fmt->pixelformat) {
		if (data->last_pix_fmt != 0) {
			cfg->desc->if1_hdr.wTotalLength += cfg->desc->if1_color.bLength;
			uvc_assign_desc(dev, &cfg->desc->if1_color, true, true);
		}

		ret = uvc_add_vs_format_desc(dev, &data->last_format_desc, fmt->pixelformat);
		if (ret != 0) {
			return ret;
		}
	}

	ret = uvc_add_vs_frame_desc(dev, data->last_format_desc, fmt);
	if (ret != 0) {
		return ret;
	}

	data->last_pix_fmt = fmt->pixelformat;

	return 0;
}

/* UVC data handling */

static int uvc_request(struct usbd_class_data *const c_data, struct net_buf *const buf,
		       const int err)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_buf_info bi = *(struct uvc_buf_info *)udc_get_buf_info(buf);
	struct uvc_data *data = dev->data;

	net_buf_unref(buf);

	if (bi.udc.ep == uvc_get_bulk_in(dev)) {
		LOG_DBG("Request completed for USB buffer %p, video buffer %p", buf, bi.vbuf);
		if (bi.vbuf != NULL) {
			k_fifo_put(&data->fifo_out, bi.vbuf);

			if (IS_ENABLED(CONFIG_POLL) && data->video_sig != NULL) {
				LOG_DBG("Raising VIDEO_BUF_DONE signal");
				k_poll_signal_raise(data->video_sig, VIDEO_BUF_DONE);
			}
		}

		/* There is now one more net_buf buffer available */
		uvc_flush_queue(dev);
	} else {
		LOG_WRN("Request on unknown endpoint 0x%02x", bi.udc.ep);
	}

	return 0;
}

/*
 * Handling the start of USB transfers marked by 'v' below:
 * v                                       v
 * [hdr:data:::][data::::::::::::::::::::] [hdr:data:::][data::::::::::::::::::::] ...
 *      [vbuf::::::::::::::::::::::::::::]      [vbuf::::::::::::::::::::::::::::] ...
 */
static struct net_buf *uvc_initiate_transfer(const struct device *dev,
					     struct video_buffer *const vbuf,
					     size_t *const next_line_offset,
					     size_t *const next_vbuf_offset)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	struct video_format *fmt = &data->video_fmt;
	size_t mps = uvc_get_bulk_mps(cfg->c_data);
	struct net_buf *buf;

	buf = net_buf_alloc_len(&uvc_buf_pool, mps, K_NO_WAIT);
	if (buf == NULL) {
		LOG_DBG("Cannot allocate first USB buffer for now");
		return NULL;
	}

	/* If uncompressed and line-based format, update the next position in the frame */
	if (fmt->pitch > 0) {
		*next_line_offset = vbuf->line_offset + vbuf->bytesused / fmt->pitch;
	}

	LOG_INF("Start of transfer, bytes used %u, sending lines %u to %u out of %u",
		vbuf->bytesused, vbuf->line_offset, vbuf->line_offset, fmt->height);

	/* Copy the header into the buffer */
	net_buf_add_mem(buf, &data->payload_header, data->payload_header.bHeaderLength);

	if (vbuf->bytesused <= net_buf_tailroom(buf)) {
		/* Very short video buffer fitting in the first packet */
		*next_vbuf_offset = vbuf->bytesused;
	} else {
		/* Pad the USB buffer until the next video buffer pointer is aligned for UDC */
		while (!IS_UDC_ALIGNED((uintptr_t)&vbuf->buffer[net_buf_tailroom(buf)])) {
			net_buf_add_u8(buf, 0);
			((struct uvc_payload_header *)buf->data)->bHeaderLength++;
		}

		*next_vbuf_offset = net_buf_tailroom(buf);
	}

	net_buf_add_mem(buf, vbuf->buffer, *next_vbuf_offset);

	/* If this new USB transfer will complete this frame */
	if (fmt->pitch == 0 || *next_line_offset >= fmt->height) {
		LOG_DBG("Last USB transfer for this buffer");

		/* Flag that this current transfer is the last */
		((struct uvc_payload_header *)buf->data)->bmHeaderInfo |=
			UVC_BMHEADERINFO_END_OF_FRAME;

		/* Toggle the Frame ID of the next vbuf */
		data->payload_header.bmHeaderInfo ^= UVC_BMHEADERINFO_FRAMEID;

		*next_line_offset = 0;
	}

	return buf;
}

/*
 * Handling the continuation of USB transfers marked by 'v' below:
 *              v                                       v
 * [hdr:data:::][data::::::::::::::::::::] [hdr:data:::][data::::::::::::::::::::] ...
 *      [vbuf::::::::::::::::::::::::::::]      [vbuf::::::::::::::::::::::::::::] ...
 */
static struct net_buf *uvc_continue_transfer(const struct device *dev,
					     struct video_buffer *const vbuf,
					     size_t *const next_line_offset,
					     size_t *const next_vbuf_offset)
{
	struct uvc_data *data = dev->data;
	struct video_format *fmt = &data->video_fmt;
	struct net_buf *buf;
	/* Workaround net_buf that uses uint16_t storage for lengths and offsets */
	const size_t max_len = 0xf000;
	const size_t buf_len = MIN(max_len, vbuf->bytesused - data->vbuf_offset);

	/* Directly pass the vbuf content with zero-copy */
	buf = net_buf_alloc_with_data(&uvc_buf_pool, vbuf->buffer + data->vbuf_offset,
				      buf_len, K_NO_WAIT);
	if (buf == NULL) {
		LOG_DBG("Cannot allocate continuation USB buffer for now");
		return NULL;
	}

	/* If uncompressed and line-based format, update the next line position in the frame */
	if (fmt->pitch > 0) {
		*next_line_offset = vbuf->line_offset + buf->len / fmt->pitch;
	}

	/* The entire video buffer is now submitted */
	*next_vbuf_offset = data->vbuf_offset + buf_len;

	return buf;
}

static int uvc_reset_transfer(const struct device *dev)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	struct uvc_buf_info *bi;
	struct net_buf *buf;
	int ret;

	LOG_DBG("Stream restarted, terminating the transfer after %u bytes", data->vbuf_offset);

	buf = net_buf_alloc_len(&uvc_buf_pool, 0, K_NO_WAIT);
	if (buf == NULL) {
		LOG_DBG("Cannot allocate ZLP USB buffer for now");
		return -ENOMEM;
	}

	bi = (struct uvc_buf_info *)udc_get_buf_info(buf);
	bi->udc.ep = uvc_get_bulk_in(dev);
	bi->vbuf = NULL;
	data->vbuf_offset = 0;

	ret = usbd_ep_enqueue(cfg->c_data, buf);
	if (ret != 0) {
		net_buf_unref(buf);
		return ret;
	}

	atomic_clear_bit(&data->state, UVC_STATE_STREAM_RESTART);

	return 0;
}

/*
 * The queue of video frame fragments (vbuf) is processed, each fragment (data)
 * is prepended by the UVC header (h). The result is cut into USB packets (pkt)
 * submitted to the USB. One vbuf per USB transfer
 *
 * [hdr:data:::][data::::::::::::::::::::] [hdr:data:::][data::::::::::::::::::::] ...
 *      [vbuf::::::::::::::::::::::::::::]      [vbuf::::::::::::::::::::::::::::] ...
 *
 * @retval 0 if vbuf was partially transferred.
 * @retval 1 if vbuf was fully transferred and can be released.
 * @return Negative error code on failure.
 */
static int uvc_flush_vbuf(const struct device *dev, struct video_buffer *const vbuf)
{
	const struct uvc_config *cfg = dev->config;
	struct uvc_data *data = dev->data;
	size_t next_vbuf_offset = data->vbuf_offset;
	size_t next_line_offset = vbuf->line_offset;
	struct net_buf *buf;
	struct uvc_buf_info *bi;
	int ret;

	if (atomic_test_bit(&data->state, UVC_STATE_STREAM_RESTART)) {
		return uvc_reset_transfer(dev);
	}

	if (data->vbuf_offset == 0) {
		buf = uvc_initiate_transfer(dev, vbuf, &next_line_offset, &next_vbuf_offset);
	} else {
		buf = uvc_continue_transfer(dev, vbuf, &next_line_offset, &next_vbuf_offset);
	}
	if (buf == NULL) {
		return -ENOMEM;
	}

	bi = (struct uvc_buf_info *)udc_get_buf_info(buf);
	bi->udc.ep = uvc_get_bulk_in(dev);

	LOG_DBG("Video buffer %p, offset %u/%u, size %u",
		vbuf, data->vbuf_offset, vbuf->bytesused, buf->len);

	/* End-of-Transfer condition */
	if (next_vbuf_offset == vbuf->bytesused) {
		bi->vbuf = vbuf;
		bi->udc.zlp = (buf->len % uvc_get_bulk_mps(cfg->c_data) == 0);
	}

	ret = usbd_ep_enqueue(cfg->c_data, buf);
	if (ret != 0) {
		net_buf_unref(buf);
		return ret;
	}

	data->vbuf_offset = next_vbuf_offset;
	vbuf->line_offset = next_line_offset;

	/* End-of-Transfer condition */
	if (next_vbuf_offset == vbuf->bytesused) {
		data->vbuf_offset = 0;
		return UVC_VBUF_DONE;
	}

	return 0;
}

static void uvc_flush_queue(const struct device *dev)
{
	struct uvc_data *data = dev->data;
	struct video_buffer *vbuf;
	int ret;

	__ASSERT_NO_MSG(atomic_test_bit(&data->state, UVC_STATE_INITIALIZED));
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (!atomic_test_bit(&data->state, UVC_STATE_ENABLED) ||
	    !atomic_test_bit(&data->state, UVC_STATE_STREAM_READY)) {
		LOG_DBG("UVC not ready yet");
		return;
	}

	/* Lock the access to the FIFO to make sure to only process one buffer at a time.
	 * K_FOREVER is not expected to take long, as uvc_flush_vbuf() never blocks.
	 */
	LOG_DBG("Locking the UVC stream");
	k_mutex_lock(&data->mutex, K_FOREVER);

	while ((vbuf = k_fifo_peek_head(&data->fifo_in)) != NULL) {
		/* Pausing the UVC driver will accumulate buffers in the input queue */
		if (atomic_test_bit(&data->state, UVC_STATE_PAUSED)) {
			break;
		}

		ret = uvc_flush_vbuf(dev, vbuf);
		if (ret < 0) {
			LOG_DBG("Could not transfer video buffer %p for now", vbuf);
			break;
		}
		if (ret == UVC_VBUF_DONE) {
			LOG_DBG("Video buffer %p transferred, removing from the queue", vbuf);
			k_fifo_get(&data->fifo_in, K_NO_WAIT);
		}
	}

	/* Now the other contexts calling this function can access the fifo safely. */
	LOG_DBG("Unlocking the UVC stream");
	k_mutex_unlock(&data->mutex);
}

static void uvc_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	atomic_set_bit(&data->state, UVC_STATE_ENABLED);

	/* Catch-up with buffers that might have been delayed */
	uvc_flush_queue(dev);
}

static void uvc_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	__ASSERT_NO_MSG(atomic_test_bit(&data->state, UVC_STATE_INITIALIZED));

	atomic_clear_bit(&data->state, UVC_STATE_ENABLED);
}

static void uvc_update(struct usbd_class_data *const c_data, const uint8_t iface,
		       const uint8_t alternate)
{
	LOG_DBG("Select alternate %u for interface %u", alternate, iface);
}

static const struct usbd_class_api uvc_class_api = {
	.enable = uvc_enable,
	.disable = uvc_disable,
	.request = uvc_request,
	.update = uvc_update,
	.control_to_host = uvc_control_to_host,
	.control_to_dev = uvc_control_to_dev,
	.init = uvc_init,
	.get_desc = uvc_get_desc,
};

/* UVC video API */

static int uvc_enqueue(const struct device *dev, struct video_buffer *const vbuf)
{
	struct uvc_data *data = dev->data;

	k_fifo_put(&data->fifo_in, vbuf);
	uvc_flush_queue(dev);

	return 0;
}

static int uvc_dequeue(const struct device *dev, struct video_buffer **const vbuf,
		       const k_timeout_t timeout)
{
	struct uvc_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int uvc_get_format(const struct device *dev, struct video_format *const fmt)
{
	struct uvc_data *data = dev->data;

	if (!atomic_test_bit(&data->state, UVC_STATE_ENABLED) ||
	    !atomic_test_bit(&data->state, UVC_STATE_STREAM_READY)) {
		return -EAGAIN;
	}

	*fmt = data->video_fmt;

	return 0;
}

static int uvc_get_frmival(const struct device *dev, struct video_frmival *const frmival)
{
	struct uvc_data *data = dev->data;

	if (!atomic_test_bit(&data->state, UVC_STATE_ENABLED) ||
	    !atomic_test_bit(&data->state, UVC_STATE_STREAM_READY)) {
		return -EAGAIN;
	}

	*frmival = data->video_frmival;

	return 0;
}

static int uvc_set_stream(const struct device *dev, const bool enable,
			  const enum video_buf_type type)
{
	struct uvc_data *data = dev->data;

	if (enable) {
		atomic_clear_bit(&data->state, UVC_STATE_PAUSED);
		uvc_flush_queue(dev);
	} else {
		atomic_set_bit(&data->state, UVC_STATE_PAUSED);
	}

	return 0;
}

#ifdef CONFIG_POLL
static int uvc_set_signal(const struct device *dev, struct k_poll_signal *const sig)
{
	struct uvc_data *data = dev->data;

	data->video_sig = sig;

	return 0;
}
#endif

static DEVICE_API(video, uvc_video_api) = {
	.get_format = uvc_get_format,
	.get_frmival = uvc_get_frmival,
	.set_stream = uvc_set_stream,
	.enqueue = uvc_enqueue,
	.dequeue = uvc_dequeue,
#if CONFIG_POLL
	.set_signal = uvc_set_signal,
#endif
};

static int uvc_preinit(const struct device *dev)
{
	struct uvc_data *data = dev->data;

	__ASSERT_NO_MSG(dev->config != NULL);

	data->payload_header.bHeaderLength = 2;
	data->format_id = 1;
	data->frame_id = 1;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_mutex_init(&data->mutex);

	return 0;
}

#define UVC_DEFINE_DESCRIPTOR(n)						\
static struct uvc_desc uvc_desc_##n = {						\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 2,						\
		.bFunctionClass = USB_BCC_VIDEO,				\
		.bFunctionSubClass = UVC_SC_VIDEO_INTERFACE_COLLECTION,		\
		.bFunctionProtocol = 0,						\
		.iFunction = 0,							\
	},									\
										\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 0,						\
		.bInterfaceClass = USB_BCC_VIDEO,				\
		.bInterfaceSubClass = UVC_SC_VIDEOCONTROL,			\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if0_hdr = {								\
		.bLength = sizeof(struct uvc_control_header_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_HEADER,				\
		.bcdUVC = sys_cpu_to_le16(0x0150),				\
		.wTotalLength = sys_cpu_to_le16(				\
			sizeof(struct uvc_control_header_descriptor) +		\
			sizeof(struct uvc_camera_terminal_descriptor) +		\
			sizeof(struct uvc_selector_unit_descriptor) +		\
			sizeof(struct uvc_processing_unit_descriptor) +		\
			sizeof(struct uvc_extension_unit_descriptor) +		\
			sizeof(struct uvc_output_terminal_descriptor)),		\
		.dwClockFrequency = sys_cpu_to_le32(30000000),			\
		.bInCollection = 1,						\
		.baInterfaceNr = {0},						\
	},									\
										\
	.if0_ct = {								\
		.bLength = sizeof(struct uvc_camera_terminal_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_INPUT_TERMINAL,			\
		.bTerminalID = UVC_UNIT_ID_CT,					\
		.wTerminalType = sys_cpu_to_le16(UVC_ITT_CAMERA),		\
		.bAssocTerminal = 0,						\
		.iTerminal = 0,							\
		.wObjectiveFocalLengthMin = sys_cpu_to_le16(0),			\
		.wObjectiveFocalLengthMax = sys_cpu_to_le16(0),			\
		.wOcularFocalLength = sys_cpu_to_le16(0),			\
		.bControlSize = 3,						\
		.bmControls = {0},						\
	},									\
										\
	.if0_su = {								\
		.bLength = sizeof(struct uvc_selector_unit_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_SELECTOR_UNIT,			\
		.bUnitID = UVC_UNIT_ID_SU,					\
		.bNrInPins = 1,							\
		.baSourceID = {UVC_UNIT_ID_CT},					\
		.iSelector = 0,							\
	},									\
										\
	.if0_pu = {								\
		.bLength = sizeof(struct uvc_processing_unit_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_PROCESSING_UNIT,			\
		.bUnitID = UVC_UNIT_ID_PU,					\
		.bSourceID = UVC_UNIT_ID_SU,					\
		.wMaxMultiplier = sys_cpu_to_le16(0),				\
		.bControlSize = 3,						\
		.bmControls = {0},						\
		.iProcessing = 0,						\
		.bmVideoStandards = 0,						\
	},									\
										\
	.if0_xu = {								\
		.bLength = sizeof(struct uvc_extension_unit_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_EXTENSION_UNIT,			\
		.bUnitID = UVC_UNIT_ID_XU,					\
		.guidExtensionCode = {0},					\
		.bNumControls = 0,						\
		.bNrInPins = 1,							\
		.baSourceID = {UVC_UNIT_ID_PU},					\
		.bControlSize = 4,						\
		.bmControls = {0},						\
		.iExtension = 0,						\
	},									\
										\
	.if0_ot = {								\
		.bLength = sizeof(struct uvc_output_terminal_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_OUTPUT_TERMINAL,			\
		.bTerminalID = UVC_UNIT_ID_OT,					\
		.wTerminalType = sys_cpu_to_le16(UVC_TT_STREAMING),		\
		.bAssocTerminal = UVC_UNIT_ID_CT,				\
		.bSourceID = UVC_UNIT_ID_XU,					\
		.iTerminal = 0,							\
	},									\
										\
	.if1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 1,						\
		.bInterfaceClass = USB_BCC_VIDEO,				\
		.bInterfaceSubClass = UVC_SC_VIDEOSTREAMING,			\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if1_hdr = {								\
		.bLength = sizeof(struct uvc_stream_header_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VS_INPUT_HEADER,			\
		.bNumFormats = 0,						\
		.wTotalLength = sys_cpu_to_le16(				\
			sizeof(struct uvc_stream_header_descriptor)),		\
		.bEndpointAddress = 0x81,					\
		.bmInfo = 0,							\
		.bTerminalLink = UVC_UNIT_ID_OT,				\
		.bStillCaptureMethod = 0,					\
		.bTriggerSupport = 0,						\
		.bTriggerUsage = 0,						\
		.bControlSize = 0,						\
	},									\
										\
	.if1_color = {								\
		.bLength = sizeof(struct uvc_color_descriptor),			\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VS_COLORFORMAT,			\
		.bColorPrimaries = UVC_COLOR_BT709,				\
		.bTransferCharacteristics = UVC_COLOR_BT709,			\
		.bMatrixCoefficients = UVC_COLOR_BT601,				\
	},									\
										\
	.if1_ep_fs = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64),				\
		.bInterval = 0,							\
	},									\
										\
	.if1_ep_hs = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0,							\
	},									\
};										\
										\
struct usb_desc_header *uvc_fs_desc_##n[UVC_MAX_FS_DESC] = {			\
	(struct usb_desc_header *) &uvc_desc_##n.iad,				\
	(struct usb_desc_header *) &uvc_desc_##n.if0,				\
	(struct usb_desc_header *) &uvc_desc_##n.if0_hdr,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_ct,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_su,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_pu,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_xu,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_ot,			\
	(struct usb_desc_header *) &uvc_desc_##n.if1,				\
	(struct usb_desc_header *) &uvc_desc_##n.if1_hdr,			\
	/* More pointers are generated here at runtime */			\
	(struct usb_desc_header *) &uvc_desc_##n.if1_ep_fs,			\
	(struct usb_desc_header *) NULL,					\
};										\
										\
struct usb_desc_header *uvc_hs_desc_##n[UVC_MAX_HS_DESC] = {			\
	(struct usb_desc_header *) &uvc_desc_##n.iad,				\
	(struct usb_desc_header *) &uvc_desc_##n.if0,				\
	(struct usb_desc_header *) &uvc_desc_##n.if0_hdr,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_ct,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_su,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_pu,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_xu,			\
	(struct usb_desc_header *) &uvc_desc_##n.if0_ot,			\
	(struct usb_desc_header *) &uvc_desc_##n.if1,				\
	(struct usb_desc_header *) &uvc_desc_##n.if1_hdr,			\
	/* More pointers are generated here at runtime */			\
	(struct usb_desc_header *) &uvc_desc_##n.if1_ep_hs,			\
	(struct usb_desc_header *) NULL,					\
};

#define USBD_VIDEO_DT_DEVICE_DEFINE(n)						\
	UVC_DEFINE_DESCRIPTOR(n)						\
										\
	USBD_DEFINE_CLASS(uvc_c_data_##n, &uvc_class_api,			\
			  (void *)DEVICE_DT_INST_GET(n), NULL);			\
										\
	const struct uvc_config uvc_cfg_##n = {					\
		.c_data = &uvc_c_data_##n,					\
		.desc = &uvc_desc_##n,						\
		.fs_desc = uvc_fs_desc_##n,					\
		.hs_desc = uvc_hs_desc_##n,					\
	};									\
										\
	struct uvc_data uvc_data_##n = {					\
		.fs_desc_idx = 10,						\
		.hs_desc_idx = 10,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uvc_preinit, NULL, &uvc_data_##n, &uvc_cfg_##n,\
		POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &uvc_video_api);	\
										\
	VIDEO_DEVICE_DEFINE(uvc##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(USBD_VIDEO_DT_DEVICE_DEFINE)
