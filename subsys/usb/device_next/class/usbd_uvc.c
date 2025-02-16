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
#include <zephyr/usb/class/usb_uvc.h>

LOG_MODULE_REGISTER(usbd_uvc, CONFIG_USBD_VIDEO_LOG_LEVEL);

enum uvc_op {
	UVC_OP_VC_GET_ERRNO,
	UVC_OP_VC_EXT,
	UVC_OP_VC_INT,
	UVC_OP_VC_FIXED,
	UVC_OP_VS_PROBE,
	UVC_OP_VS_COMMIT,
	UVC_OP_RETURN_ERROR,
	UVC_OP_INVALID,
};

enum uvc_class_status {
	UVC_STATE_INITIALIZED,
	UVC_STATE_ENABLED,
	UVC_STATE_READY,
	UVC_STATE_PAUSED,
};

struct uvc_payload_header {
	uint8_t bHeaderLength;
	uint8_t bmHeaderInfo;
	uint32_t dwPresentationTime; /* optional */
	uint32_t scrSourceClockSTC;  /* optional */
	uint16_t scrSourceClockSOF;  /* optional */
} __packed;

/* Information specific to each VideoStreaming interface */
struct uvc_stream {
	const struct device *dev;
	atomic_t state;
	struct k_work work;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct usbd_uvc_strm_desc *desc;
	struct uvc_probe default_probe;
	struct uvc_payload_header payload_header;
	uint8_t format_id;
	uint8_t frame_id;
	const struct device *video_dev;
	struct video_format video_fmt;
	struct video_frmival video_frmival;
	size_t vbuf_offset;
};

/* Global information */
struct uvc_data {
	atomic_t state;
	struct usbd_class_data *c_data;
	struct uvc_stream *streams;
	struct usbd_uvc_desc *desc;
	struct usb_desc_header **fs_desc;
	struct usb_desc_header **hs_desc;
	/* UVC error from latest request */
	int err;
};

/* Specialized version of UDC net_buf metadata with extra fields */
struct uvc_buf_info {
	/* Regular UDC buf info so that it can be passed to USBD directly */
	struct udc_buf_info udc;
	/* Extra field at the end */
	struct video_buffer *vbuf;
	/* UVC stream this buffer belongs to */
	struct uvc_stream *stream;
} __packed;

/* Mapping between UVC controls and Video controls */
struct uvc_control_map {
	uint32_t param;
	enum uvc_op op;
	uint8_t selector;
	uint8_t size;
	uint8_t bit;
};

struct uvc_guid_quirk {
	uint32_t fourcc;
	/* GUIDs are 16-bytes long, with the first four bytes being the Four Character Code of the
	 * format and the rest constant, except for some exceptions listed in this table.
	 */
	uint8_t guid[16];
};

typedef int uvc_control_fn_t(struct uvc_stream *strm, uint8_t selector, uint8_t request,
			     struct net_buf *buf);

NET_BUF_POOL_VAR_DEFINE(uvc_pool, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 16,
			512 * DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 16,
			sizeof(struct uvc_buf_info), NULL);

/* UVC helper functions */

static const struct uvc_guid_quirk uvc_guid_quirks[] = {
	{VIDEO_PIX_FMT_YUYV,  UVC_FORMAT_GUID("YUY2")},
};

static void uvc_fourcc_to_guid(uint8_t guid[16], uint32_t fourcc)
{
	for (int i = 0; i < ARRAY_SIZE(uvc_guid_quirks); i++) {
		if (uvc_guid_quirks[i].fourcc == fourcc) {
			memcpy(guid, uvc_guid_quirks[i].guid, 16);
			return;
		}
	}
	fourcc = sys_cpu_to_le32(fourcc);
	memcpy(guid, UVC_FORMAT_GUID("...."), 16);
	memcpy(guid, &fourcc, 4);
}

static uint32_t uvc_guid_to_fourcc(uint8_t guid[16])
{
	uint32_t fourcc;

	for (int i = 0; i < ARRAY_SIZE(uvc_guid_quirks); i++) {
		if (memcmp(guid, uvc_guid_quirks[i].guid, 16) == 0) {
			return uvc_guid_quirks[i].fourcc;
		}
	}

	memcpy(&fourcc, guid, 4);

	return sys_le32_to_cpu(fourcc);
}

/* UVC control handling */

static const struct uvc_control_map uvc_control_map_ct[] = {
	{
		.size = 1,
		.bit = 1,
		.selector = UVC_CT_AE_MODE_CONTROL,
		.op = UVC_OP_VC_FIXED,
		.param = BIT(0)
	},
	{
		.size = 1,
		.bit = 2,
		.selector = UVC_CT_AE_PRIORITY_CONTROL,
		.op = UVC_OP_VC_FIXED,
		.param = 0
	},
	{
		.size = 4,
		.bit = 3,
		.selector = UVC_CT_EXPOSURE_TIME_ABS_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_EXPOSURE
	},
	{
		.size = 2,
		.bit = 9,
		.selector = UVC_CT_ZOOM_ABS_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_ZOOM_ABSOLUTE
	},
	{0},
};

static const struct uvc_control_map uvc_control_map_pu[] = {
	{
		.size = 2,
		.bit = 0,
		.selector = UVC_PU_BRIGHTNESS_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_BRIGHTNESS
	},
	{
		.size = 1,
		.bit = 1,
		.selector = UVC_PU_CONTRAST_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_CONTRAST
	},
	{
		.size = 2,
		.bit = 9,
		.selector = UVC_PU_GAIN_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_GAIN
	},
	{
		.size = 2,
		.bit = 3,
		.selector = UVC_PU_SATURATION_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_SATURATION
	},
	{
		.size = 2,
		.bit = 6,
		.selector = UVC_PU_WHITE_BALANCE_TEMP_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_WHITE_BALANCE_TEMPERATURE,
	},
	{0},
};

static const struct uvc_control_map uvc_control_map_su[] = {
	{
		.size = 1,
		.bit = 0,
		.selector = UVC_SU_INPUT_SELECT_CONTROL,
		.op = UVC_OP_VC_INT,
		.param = VIDEO_CID_TEST_PATTERN,},
	{0},
};

static const struct uvc_control_map uvc_control_map_xu[] = {
	{
		.size = 4,
		.bit = 0,
		.selector = UVC_XU_BASE_CONTROL + 0,
		.op = UVC_OP_VC_EXT,
		.param = VIDEO_CID_PRIVATE_BASE + 0,
	},
	{
		.size = 4,
		.bit = 1,
		.selector = UVC_XU_BASE_CONTROL + 1,
		.op = UVC_OP_VC_EXT,
		.param = VIDEO_CID_PRIVATE_BASE + 1,
	},
	{
		.size = 4,
		.bit = 2,
		.selector = UVC_XU_BASE_CONTROL + 2,
		.op = UVC_OP_VC_EXT,
		.param = VIDEO_CID_PRIVATE_BASE + 2,
	},
	{
		.size = 4,
		.bit = 3,
		.selector = UVC_XU_BASE_CONTROL + 3,
		.op = UVC_OP_VC_EXT,
		.param = VIDEO_CID_PRIVATE_BASE + 3,
	},
	{0},
};

/* Get the format and frame descriptors selected for the given VideoStreaming interface. */
static void uvc_get_format_frame_desc(const struct uvc_stream *strm,
				      union uvc_stream_descriptor **format_desc,
				      union uvc_stream_descriptor **frame_desc)
{
	int i;

	*format_desc = NULL;
	for (i = 0; i < strm->desc->if_vs_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->if_vs_formats[i];

		if (desc->format.bDescriptorSubtype != UVC_VS_FORMAT_UNCOMPRESSED &&
		    desc->format.bDescriptorSubtype != UVC_VS_FORMAT_MJPEG) {
			continue;
		}

		if (desc->format.bFormatIndex == strm->format_id) {
			*format_desc = &strm->desc->if_vs_formats[i];
			break;
		}
	}

	*frame_desc = NULL;
	for (i++; i < strm->desc->if_vs_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->if_vs_formats[i];

		if (desc->frame_discrete.bDescriptorSubtype != UVC_VS_FRAME_UNCOMPRESSED &&
		    desc->frame_discrete.bDescriptorSubtype != UVC_VS_FRAME_MJPEG) {
			break;
		}

		if (desc->frame_discrete.bFrameIndex == strm->frame_id) {
			*frame_desc = &strm->desc->if_vs_formats[i];
			break;
		}
	}
}

static int uvc_buf_add_int(struct net_buf *buf, uint16_t size, uint32_t value)
{
	if (buf->size != size) {
		LOG_ERR("invalid wLength %u, expected %u", buf->size, size);
		return -EINVAL;
	}

	switch (size) {
	case 4:
		net_buf_add_le32(buf, value);
		return 0;
	case 2:
		net_buf_add_le16(buf, value);
		return 0;
	case 1:
		net_buf_add_u8(buf, value);
		return 0;
	default:
		LOG_WRN("Invalid size %u", size);
		return -ENOTSUP;
	}
}

static uint8_t uvc_get_bulk_in(struct uvc_stream *strm)
{
	struct uvc_data *data = strm->dev->data;

	switch (usbd_bus_speed(usbd_class_get_ctx(data->c_data))) {
	case USBD_SPEED_FS:
		return strm->desc->if_vs_ep_fs.bEndpointAddress;
	case USBD_SPEED_HS:
		return strm->desc->if_vs_ep_hs.bEndpointAddress;
	default:
		__ASSERT_NO_MSG(false);
		return 0;
	}
}

static size_t uvc_get_bulk_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	switch (usbd_bus_speed(uds_ctx)) {
	case USBD_SPEED_FS:
		return 64;
	case USBD_SPEED_HS:
		return 512;
	default:
		__ASSERT_NO_MSG(false);
		return 0;
	}
}

static int uvc_vs_get_probe_format_index(struct uvc_stream *strm, struct uvc_probe *probe,
					 uint8_t request)
{
	uint8_t max = 0;

	for (int i = 0; i < strm->desc->if_vs_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->if_vs_formats[i];

		max += desc->format.bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED ||
		       desc->format.bDescriptorSubtype == UVC_VS_FORMAT_MJPEG;
	}

	switch (request) {
	case UVC_GET_RES:
	case UVC_GET_MIN:
		probe->bFormatIndex = 1;
		break;
	case UVC_GET_MAX:
		probe->bFormatIndex = max;
		break;
	case UVC_GET_CUR:
		probe->bFormatIndex = strm->format_id;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int uvc_vs_get_probe_frame_index(struct uvc_stream *strm, struct uvc_probe *probe,
					uint8_t request)
{
	uint8_t max = 0;
	int i;

	/* Search the current format */
	for (i = 0; i < strm->desc->if_vs_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->if_vs_formats[i];

		if (desc->format.bDescriptorSubtype != UVC_VS_FORMAT_UNCOMPRESSED &&
		    desc->format.bDescriptorSubtype != UVC_VS_FORMAT_MJPEG) {
			continue;
		}
		if (desc->format.bFormatIndex == strm->format_id) {
			break;
		}
	}

	/* Seek until the next format */
	for (i += 1; i < strm->desc->if_vs_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->if_vs_formats[i];

		if (desc->frame_discrete.bDescriptorSubtype != UVC_VS_FRAME_UNCOMPRESSED &&
		    desc->frame_discrete.bDescriptorSubtype != UVC_VS_FRAME_MJPEG) {
			break;
		}
		max++;
	}

	switch (request) {
	case UVC_GET_RES:
	case UVC_GET_MIN:
		probe->bFrameIndex = 1;
		break;
	case UVC_GET_MAX:
		probe->bFrameIndex = max;
		break;
	case UVC_GET_CUR:
		probe->bFrameIndex = strm->frame_id;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int uvc_vs_get_probe_frame_interval(struct uvc_stream *strm, struct uvc_probe *probe,
					   uint8_t request)
{
	union uvc_stream_descriptor *format_desc;
	union uvc_stream_descriptor *frame_desc;
	int max;

	uvc_get_format_frame_desc(strm, &format_desc, &frame_desc);
	if (format_desc == NULL || frame_desc == NULL) {
		LOG_DBG("Selected format ID or frame ID not found");
		return -EINVAL;
	}

	switch (request) {
	case UVC_GET_MIN:
		probe->dwFrameInterval =
			sys_cpu_to_le32(frame_desc->frame_discrete.dwFrameInterval[0]);
		break;
	case UVC_GET_MAX:
		max = frame_desc->frame_discrete.bFrameIntervalType - 1;
		probe->dwFrameInterval =
			sys_cpu_to_le32(frame_desc->frame_discrete.dwFrameInterval[max]);
		break;
	case UVC_GET_RES:
		probe->dwFrameInterval = sys_cpu_to_le32(1);
		break;
	case UVC_GET_CUR:
		probe->dwFrameInterval = sys_cpu_to_le32(strm->video_frmival.numerator);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int uvc_vs_get_probe_max_size(struct uvc_stream *strm, struct uvc_probe *probe,
				     uint8_t request)
{
	struct video_format *fmt = &strm->video_fmt;
	uint32_t max_frame_size = MAX(fmt->pitch, fmt->width) * fmt->height;
	uint32_t max_payload_size = max_frame_size + CONFIG_USBD_VIDEO_HEADER_SIZE;

	switch (request) {
	case UVC_GET_MIN:
	case UVC_GET_MAX:
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

static int uvc_vs_get_format_from_desc(struct uvc_stream *strm, struct video_format *fmt)
{
	union uvc_stream_descriptor *format_desc = NULL;
	union uvc_stream_descriptor *frame_desc = NULL;

	/* Update the format based on the probe message from the host */
	uvc_get_format_frame_desc(strm, &format_desc, &frame_desc);
	if (format_desc == NULL || frame_desc == NULL) {
		LOG_ERR("Called probe with invalid format ID (%u) and frame ID (%u)",
			strm->format_id, strm->frame_id);
		return -EINVAL;
	}

	/* Translate between UVC pixel formats and Video pixel formats */
	if (format_desc->format_mjpeg.bDescriptorSubtype == UVC_VS_FORMAT_MJPEG) {
		fmt->pixelformat = VIDEO_PIX_FMT_JPEG;
	} else {
		fmt->pixelformat = uvc_guid_to_fourcc(format_desc->format_uncomp.guidFormat);
	}

	LOG_DBG("format %u, frame %u, guid %.4s, pixfmt %u",
		format_desc->format_uncomp.bFormatIndex, frame_desc->frame_discrete.bFrameIndex,
		format_desc->format_uncomp.guidFormat, fmt->pixelformat);

	/* Fill the format according to what the host selected */
	fmt->width = frame_desc->frame_discrete.wWidth;
	fmt->height = frame_desc->frame_discrete.wHeight;
	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	return 0;
}

static int uvc_vs_get_probe_struct(struct uvc_stream *strm, struct uvc_probe *probe,
				   uint8_t request)
{
	struct video_format *fmt = &strm->video_fmt;
	int ret;

	ret = uvc_vs_get_probe_format_index(strm, probe, request);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_vs_get_probe_frame_index(strm, probe, request);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_vs_get_format_from_desc(strm, fmt);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_vs_get_probe_frame_interval(strm, probe, request);
	if (ret != 0) {
		return ret;
	}

	ret = uvc_vs_get_probe_max_size(strm, probe, request);
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

static int uvc_vs_get_probe(struct uvc_stream *strm, struct net_buf *buf, uint8_t request)
{
	size_t size = MIN(sizeof(struct uvc_probe), buf->size);
	struct uvc_probe *probe;

	switch (request) {
	case UVC_GET_INFO:
		return uvc_buf_add_int(buf, 1, UVC_INFO_SUPPORTS_GET | UVC_INFO_SUPPORTS_SET);
	case UVC_GET_LEN:
		return uvc_buf_add_int(buf, 2, sizeof(struct uvc_probe));
	case UVC_GET_DEF:
		net_buf_add_mem(buf, &strm->default_probe, size);
		return 0;
	case UVC_GET_MIN:
	case UVC_GET_RES:
	case UVC_GET_MAX:
	case UVC_GET_CUR:
		net_buf_add(buf, size);
		probe = (void *)buf->data;
		break;
	default:
		return -EINVAL;
	}

	return uvc_vs_get_probe_struct(strm, probe, request);
}

static int uvc_vs_set_probe(struct uvc_stream *strm, const struct net_buf *buf)
{
	struct uvc_probe *probe;
	struct uvc_probe max = {0};
	int ret;

	if (buf->len != sizeof(*probe)) {
		LOG_ERR("Expected probe message of %u bytes got %u", sizeof(*probe), buf->len);
		return -EINVAL;
	}

	probe = (struct uvc_probe *)buf->data;

	ret = uvc_vs_get_probe_struct(strm, &max, UVC_GET_MAX);
	if (ret != 0) {
		return ret;
	}

	if (probe->bFrameIndex > max.bFrameIndex) {
		LOG_WRN("The bFrameIndex %u requested is beyond the max %u",
			probe->bFrameIndex, max.bFrameIndex);
		return -ERANGE;
	}

	if (probe->bFormatIndex > max.bFormatIndex) {
		LOG_WRN("The bFormatIndex %u requested is beyond the max %u",
			probe->bFormatIndex, max.bFormatIndex);
		return -ERANGE;
	}

	if (probe->dwFrameInterval != 0) {
		strm->video_frmival.numerator = sys_le32_to_cpu(probe->dwFrameInterval);
	}
	if (probe->bFrameIndex != 0) {
		strm->frame_id = probe->bFrameIndex;
	}
	if (probe->bFormatIndex != 0) {
		strm->format_id = probe->bFormatIndex;
	}
	if (probe->dwFrameInterval != 0) {
		strm->video_frmival.numerator = sys_le32_to_cpu(probe->dwFrameInterval);
	}

	return 0;
}

static int uvc_vs_get_commit(struct uvc_stream *strm, struct net_buf *buf, uint8_t request)
{
	if (request != UVC_GET_CUR) {
		LOG_WRN("commit: invalid bRequest %u", request);
		return -EINVAL;
	}

	return uvc_vs_get_probe(strm, buf, UVC_GET_CUR);
}


static int uvc_vs_set_commit(struct uvc_stream *strm, const struct net_buf *buf)
{
	struct video_format *fmt = &strm->video_fmt;
	int ret;

	ret = uvc_vs_set_probe(strm, buf);
	if (ret != 0) {
		return ret;
	}

	atomic_set_bit(&strm->state, UVC_STATE_READY);
	k_work_submit(&strm->work);

	LOG_INF("Ready to transfer, setting source format to %c%c%c%c %ux%u",
		fmt->pixelformat >> 0 & 0xff, fmt->pixelformat >> 8 & 0xff,
		fmt->pixelformat >> 16 & 0xff, fmt->pixelformat >> 24 & 0xff,
		fmt->width, fmt->height);

	ret = video_set_format(strm->video_dev, VIDEO_EP_OUT, fmt);
	if (ret != 0) {
		LOG_ERR("Could not set the format of %s", strm->video_dev->name);
		return ret;
	}

	ret = video_set_frmival(strm->video_dev, VIDEO_EP_OUT, &strm->video_frmival);
	if (ret != 0) {
		LOG_WRN("Could not set the framerate of %s", strm->video_dev->name);
	}

	ret = video_stream_start(strm->video_dev);
	if (ret != 0) {
		LOG_ERR("Could not start %s", strm->video_dev->name);
		return ret;
	}

	return 0;
}

static int uvc_vc_get_default(uint8_t request, struct net_buf *buf, uint8_t size)
{
	switch (request) {
	case UVC_GET_INFO:
		return uvc_buf_add_int(buf, 1, UVC_INFO_SUPPORTS_GET | UVC_INFO_SUPPORTS_SET);
	case UVC_GET_LEN:
		return uvc_buf_add_int(buf, buf->size, size);
	case UVC_GET_RES:
		return uvc_buf_add_int(buf, size, 1);
	default:
		LOG_WRN("Unsupported request type %u", request);
		return -ENOTSUP;
	}
}

static int uvc_vc_get_fixed(struct net_buf *buf, uint8_t size, uint32_t value, uint8_t request)
{
	LOG_DBG("Control type 'fixed', size %u", size);

	switch (request) {
	case UVC_GET_DEF:
	case UVC_GET_CUR:
	case UVC_GET_MIN:
	case UVC_GET_MAX:
		return uvc_buf_add_int(buf, size, value);
	default:
		return uvc_vc_get_default(request, buf, size);
	}
}

static int uvc_vc_get_int(struct uvc_stream *strm, struct net_buf *buf, uint8_t size,
			  unsigned int cid, uint8_t request)
{
	const struct device *video_dev = strm->video_dev;
	int value;
	int ret;

	LOG_DBG("Responding to GET control of type 'integer', size %u", size);

	switch (request) {
	case UVC_GET_DEF:
	case UVC_GET_MIN:
		return uvc_buf_add_int(buf, size, 0);
	case UVC_GET_MAX:
		return uvc_buf_add_int(buf, size, INT16_MAX);
	case UVC_GET_CUR:
		ret = video_get_ctrl(video_dev, cid, &value);
		if (ret != 0) {
			LOG_INF("Failed to query '%s'", video_dev->name);
			return ret;
		}

		LOG_DBG("Value for CID 0x08%x is %u", cid, value);

		return uvc_buf_add_int(buf, size, value);
	default:
		return uvc_vc_get_default(request, buf, size);
	}
}

static int uvc_vc_set_int(struct uvc_stream *strm, const struct net_buf *buf, uint8_t size,
			  unsigned int cid)
{
	const struct device *video_dev = strm->video_dev;
	int value;
	int ret;

	if (buf->len != size) {
		LOG_ERR("Host sent an invalid size %u for this control %u", size, cid);
		return -EINVAL;
	}

	switch (size) {
	case 1:
		value = *(uint8_t *)buf->data;
		break;
	case 2:
		value = sys_le16_to_cpu(*(uint16_t *)buf->data);
		break;
	case 4:
		value = sys_le32_to_cpu(*(uint32_t *)buf->data);
		break;
	default:
		LOG_ERR("Unsupported size, cannot convert");
		return -EINVAL;
	}

	LOG_DBG("Setting CID 0x08%x to %u", cid, value);

	ret = video_set_ctrl(video_dev, cid, (void *)value);
	if (ret != 0) {
		LOG_ERR("Failed to configure target video device");
		return ret;
	}

	return 0;
}

static int uvc_vc_get_errno(const struct device *dev, struct net_buf *buf, uint8_t request)
{
	struct uvc_data *data = dev->data;

	switch (request) {
	case UVC_GET_INFO:
		return uvc_buf_add_int(buf, 1, UVC_INFO_SUPPORTS_GET);
	case UVC_GET_CUR:
		return uvc_buf_add_int(buf, 1, data->err);
	default:
		LOG_WRN("Unsupported request type %u", request);
		return -ENOTSUP;
	}
}

static void uvc_vc_set_errno(const struct device *dev, int ret)
{
	struct uvc_data *data = dev->data;

	switch (ret) {
	case 0:
		data->err = 0;
		break;
	case EBUSY:
	case EAGAIN:
	case EINPROGRESS:
	case EALREADY:
		data->err = UVC_ERR_NOT_READY;
		break;
	case EOVERFLOW:
	case ERANGE:
	case E2BIG:
		data->err = UVC_ERR_OUT_OF_RANGE;
		break;
	case EDOM:
	case EINVAL:
		data->err = UVC_ERR_INVALID_VALUE_WITHIN_RANGE;
		break;
	case ENODEV:
	case ENOTSUP:
	case ENOSYS:
		data->err = UVC_ERR_INVALID_REQUEST;
		break;
	default:
		data->err = UVC_ERR_UNKNOWN;
		break;
	}
}

static int uvc_get_control_op(const struct device *dev, const struct usb_setup_packet *setup,
			      struct uvc_stream **strmp, const void **voidpp)
{
	struct uvc_data *data = dev->data;
	uint8_t ifnum = (setup->wIndex >> 0) & 0xff;
	uint8_t unit_id = (setup->wIndex >> 8) & 0xff;
	uint8_t selector = setup->wValue >> 8;
	const struct uvc_control_map *list = NULL;
	const struct uvc_control_map *map = NULL;

	/* VideoStreaming operation */

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		if (ifnum == strm->desc->if_vs.bInterfaceNumber) {
			switch (selector) {
			case UVC_VS_PROBE_CONTROL:
				LOG_DBG("Host sent a VideoStreaming PROBE control");
				*strmp = strm;
				return UVC_OP_VS_PROBE;
			case UVC_VS_COMMIT_CONTROL:
				LOG_DBG("Host sent a VideoStreaming COMMIT control");
				*strmp = strm;
				return UVC_OP_VS_COMMIT;
			default:
				LOG_ERR("Invalid probe/commit operation for bInterfaceNumber %u",
					ifnum);
				return UVC_OP_INVALID;
			}
		}
	}

	/* VideoControl operation */

	if (ifnum != data->desc->if_vc.bInterfaceNumber) {
		LOG_WRN("Interface %u not found", ifnum);
		data->err = UVC_ERR_INVALID_UNIT;
		return UVC_OP_RETURN_ERROR;
	}

	if (unit_id == 0) {
		return UVC_OP_VC_GET_ERRNO;
	}

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		if (unit_id == strm->desc->if_vc_ct.bTerminalID) {
			*strmp = strm;
			list = uvc_control_map_ct;
			break;
		} else if (unit_id == strm->desc->if_vc_su.bUnitID) {
			*strmp = strm;
			list = uvc_control_map_su;
			break;
		} else if (unit_id == strm->desc->if_vc_pu.bUnitID) {
			*strmp = strm;
			list = uvc_control_map_pu;
			break;
		} else if (unit_id == strm->desc->if_vc_xu.bUnitID) {
			*strmp = strm;
			list = uvc_control_map_xu;
			break;
		}
	}

	for (int i = 0; list[i].size != 0; i++) {
		if (list[i].selector == selector) {
			map = &list[i];
			break;
		}
	}

	if (map == NULL) {
		LOG_WRN("Could not find a matching bUnitId %u and selector %u", unit_id, selector);
		data->err = UVC_ERR_INVALID_UNIT;
		return UVC_OP_RETURN_ERROR;
	}

	*voidpp = map;
	return map->op;
}

static int uvc_control_to_host(struct usbd_class_data *const c_data,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_stream *strm;
	const struct uvc_control_map *map;
	uint8_t request = setup->bRequest;
	const void *ptr;

	LOG_DBG("Host sent a %s request",
		request == UVC_GET_CUR ? "GET_CUR" : request == UVC_GET_MIN ? "GET_MIN" :
		request == UVC_GET_MAX ? "GET_MAX" : request == UVC_GET_RES ? "GET_RES" :
		request == UVC_GET_LEN ? "GET_LEN" : request == UVC_GET_DEF ? "GET_DEF" :
		request == UVC_GET_INFO ? "GET_INFO" : "bad");

	if (setup->wLength > buf->size) {
		LOG_ERR("wLength %u larger than %u bytes", setup->wLength, buf->size);
		errno = ENOMEM;
		goto end;
	}

	buf->size = setup->wLength;

	switch (uvc_get_control_op(dev, setup, &strm, &ptr)) {
	case UVC_OP_VS_PROBE:
		errno = -uvc_vs_get_probe(strm, buf, setup->bRequest);
		break;
	case UVC_OP_VS_COMMIT:
		errno = -uvc_vs_get_commit(strm, buf, setup->bRequest);
		break;
	case UVC_OP_VC_EXT:
		map = ptr;
		errno = -uvc_vc_get_int(strm, buf, map->size, map->param, setup->bRequest);
		break;
	case UVC_OP_VC_INT:
		map = ptr;
		errno = -uvc_vc_get_int(strm, buf, map->size, map->param, setup->bRequest);
		break;
	case UVC_OP_VC_FIXED:
		map = ptr;
		errno = -uvc_vc_get_fixed(buf, map->size, map->param, setup->bRequest);
		break;
	case UVC_OP_VC_GET_ERRNO:
		errno = -uvc_vc_get_errno(dev, buf, setup->bRequest);
		break;
	case UVC_OP_RETURN_ERROR:
		errno = EINVAL;
		return 0;
	default:
		LOG_WRN("Unhandled operation, stalling control command");
		errno = EINVAL;
	}
end:
	uvc_vc_set_errno(dev, errno);

	return 0;
}

static int uvc_control_to_dev(struct usbd_class_data *const c_data,
			      const struct usb_setup_packet *const setup,
			      const struct net_buf *const buf)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_stream *strm;
	const struct uvc_control_map *map;
	const void *ptr;

	if (setup->bRequest != UVC_SET_CUR) {
		LOG_WRN("Host issued a control write message but the bRequest is not SET_CUR");
		errno = ENOMEM;
		goto end;
	}

	LOG_DBG("Host sent a SET_CUR request");

	switch (uvc_get_control_op(dev, setup, &strm, &ptr)) {
	case UVC_OP_VS_PROBE:
		errno = -uvc_vs_set_probe(strm, buf);
		break;
	case UVC_OP_VS_COMMIT:
		errno = -uvc_vs_set_commit(strm, buf);
		break;
	case UVC_OP_VC_EXT:
		map = ptr;
		errno = -uvc_vc_set_int(strm, buf, map->size, map->param);
		break;
	case UVC_OP_VC_INT:
		map = ptr;
		errno = -uvc_vc_set_int(strm, buf, map->size, map->param);
		break;
	case UVC_OP_VC_FIXED:
		errno = ENOTSUP;
		return 0;
	case UVC_OP_RETURN_ERROR:
		errno = EINVAL;
		return 0;
	default:
		LOG_WRN("Unhandled operation, stalling control command");
		errno = EINVAL;
	}
end:
	uvc_vc_set_errno(dev, errno);

	return 0;
}

/* UVC descriptor handling */

static int uvc_init_format_desc(struct uvc_stream *strm, int *id,
			       union uvc_stream_descriptor **format_desc,
			       const struct video_format_cap *cap)
{
	union uvc_stream_descriptor *desc;
	uint32_t pixfmt = cap->pixelformat;

	if (*id >= CONFIG_USBD_VIDEO_MAX_VS_DESC) {
		LOG_ERR("Out of descriptors, consider increasing CONFIG_USBD_VIDEO_MAX_VS_DESC");
		return -ENOMEM;
	}

	desc = &strm->desc->if_vs_formats[*id];

	/* Increment the position in the list of formats/frame descriptors */
	(*id)++;

	LOG_DBG("Adding format descriptor #%u for %c%c%c%c",
		strm->desc->if_vs_header.bNumFormats + 1,
		pixfmt >> 0 & 0xff, pixfmt >> 8 & 0xff, pixfmt >> 16 & 0xff, pixfmt >> 24 & 0xff);

	__ASSERT_NO_MSG(desc != NULL);
	memset(desc, 0, sizeof(*desc));

	desc->format.bDescriptorType = USB_DESC_CS_INTERFACE;
	desc->format.bFormatIndex = ++strm->desc->if_vs_header.bNumFormats;

	switch (pixfmt) {
	case VIDEO_PIX_FMT_JPEG:
		desc->format_mjpeg.bLength = sizeof(desc->format_mjpeg);
		desc->format_mjpeg.bDescriptorSubtype = UVC_VS_FORMAT_MJPEG;
		desc->format_mjpeg.bDefaultFrameIndex = 1;
		break;
	default:
		uvc_fourcc_to_guid(desc->format_uncomp.guidFormat, pixfmt);
		desc->format_uncomp.bLength = sizeof(desc->format_uncomp);
		desc->format_uncomp.bDescriptorSubtype = UVC_VS_FORMAT_UNCOMPRESSED;
		desc->format_uncomp.bBitsPerPixel = video_bits_per_pixel(cap->pixelformat);
		desc->format_uncomp.bDefaultFrameIndex = 1;
	}

	strm->desc->if_vs_header.wTotalLength =
		sys_cpu_to_le16(sys_le16_to_cpu(strm->desc->if_vs_header.wTotalLength) +
				desc->frame_discrete.bLength);

	*format_desc = desc;
	return 0;
}

static int uvc_compare_frmival_desc(const void *a, const void *b)
{
	uint32_t ia, ib;

	/* Copy in case a and b are not 32-bit aligned */
	memcpy(&ia, a, sizeof(uint32_t));
	memcpy(&ib, b, sizeof(uint32_t));

	return ia - ib;
}

static int uvc_init_frame_desc(struct uvc_stream *strm, int *id,
			      union uvc_stream_descriptor *format_desc,
			      const struct video_format_cap *cap, bool min)
{
	uint16_t w = min ? cap->width_min : cap->width_max;
	uint16_t h = min ? cap->height_min : cap->height_max;
	uint16_t p = MAX(video_bits_per_pixel(cap->pixelformat), 8) * w / BITS_PER_BYTE;
	uint32_t min_bitrate = UINT32_MAX;
	uint32_t max_bitrate = 0;
	struct video_format fmt = {.pixelformat = cap->pixelformat,
				   .width = w, .height = h, .pitch = p};
	struct video_frmival_enum fie = {.format = &fmt};
	union uvc_stream_descriptor *desc;
	uint32_t max_size = MAX(p, w) * h;

	LOG_DBG("Adding frame descriptor #%u for %ux%u",
		format_desc->format.bNumFrameDescriptors + 1, w, h);

	if (*id >= CONFIG_USBD_VIDEO_MAX_VS_DESC) {
		LOG_ERR("Out of descriptors, consider increasing CONFIG_USBD_VIDEO_MAX_VS_DESC");
		return -ENOMEM;
	}

	desc = &strm->desc->if_vs_formats[*id];

	/* Increment the position in the list of formats/frame descriptors */
	(*id)++;

	__ASSERT_NO_MSG(desc != NULL);
	memset(desc, 0, sizeof(*desc));

	desc->frame_discrete.bLength = sizeof(desc->frame_discrete);
	desc->frame_discrete.bLength -= CONFIG_USBD_VIDEO_MAX_FRMIVAL * sizeof(uint32_t);
	desc->frame_discrete.bDescriptorType = USB_DESC_CS_INTERFACE;
	desc->frame_discrete.bFrameIndex = ++format_desc->format.bNumFrameDescriptors;
	desc->frame_discrete.wWidth = sys_cpu_to_le16(w);
	desc->frame_discrete.wHeight = sys_cpu_to_le16(h);
	desc->frame_discrete.dwMaxVideoFrameBufferSize = sys_cpu_to_le32(max_size);
	desc->frame_discrete.bDescriptorSubtype =
		(format_desc->frame_discrete.bDescriptorSubtype == UVC_VS_FORMAT_UNCOMPRESSED)
			? UVC_VS_FRAME_UNCOMPRESSED
			: UVC_VS_FRAME_MJPEG;

	/* Add the adwFrameInterval fields at the end of this descriptor */
	for (int i = 0; video_enum_frmival(strm->video_dev, VIDEO_EP_OUT, &fie) == 0; i++) {
		uint32_t bitrate;

		if (i >= CONFIG_USBD_VIDEO_MAX_FRMIVAL) {
			LOG_WRN("out of descriptor storage");
			return -ENOSPC;
		}

		if (fie.type != VIDEO_FRMIVAL_TYPE_DISCRETE) {
			LOG_WRN("Only discrete frame intervals are supported");
			return -EINVAL;
		}

		desc->frame_discrete.dwFrameInterval[i] =
			sys_cpu_to_le32(video_frmival_nsec(&fie.discrete) / 100);
		desc->frame_discrete.bFrameIntervalType++;
		desc->frame_discrete.bLength += sizeof(uint32_t);

		bitrate = MAX(fmt.pitch, fmt.width) * fmt.height * fie.discrete.denominator /
			  fie.discrete.numerator;
		min_bitrate = MIN(min_bitrate, bitrate);
		max_bitrate = MAX(max_bitrate, bitrate);
	}

	/* UVC requires the frame intervals to be sorted, but not Zephyr */
	qsort(desc->frame_discrete.dwFrameInterval, desc->frame_discrete.bFrameIntervalType,
		sizeof(*desc->frame_discrete.dwFrameInterval), uvc_compare_frmival_desc);

	if (min_bitrate > max_bitrate) {
		LOG_WRN("the minimum bitrate is above the maximum bitrate");
	}

	if (min_bitrate == 0 || max_bitrate == 0) {
		LOG_WRN("minimum bitrate or maximum bitrate is zero");
		min_bitrate = max_bitrate = 1;
	}

	desc->frame_discrete.dwDefaultFrameInterval = desc->frame_discrete.dwFrameInterval[0];
	desc->frame_discrete.dwMinBitRate = sys_cpu_to_le32(min_bitrate);
	desc->frame_discrete.dwMaxBitRate = sys_cpu_to_le32(max_bitrate);

	strm->desc->if_vs_header.wTotalLength =
		sys_cpu_to_le16(sys_le16_to_cpu(strm->desc->if_vs_header.wTotalLength) +
				desc->frame_discrete.bLength);

	return 0;
}

static uint32_t uvc_get_mask(const struct device *video_dev, const struct uvc_control_map *list)
{
	uint32_t mask = 0;
	bool ok;
	int value;

	LOG_DBG("Testing all supported controls to see which are supported");

	for (int i = 0; list[i].size != 0; i++) {
		ok = (list[i].op == UVC_OP_VC_FIXED) ||
		     (video_get_ctrl(video_dev, list[i].param, &value) == 0);
		mask |= ok << list[i].bit;
		LOG_DBG("%s supports control 0x%02x: %s",
			video_dev->name, list[i].selector, ok ? "yes" : "no");
	}

	return mask;
}

static int uvc_init_desc(const struct device *dev, struct uvc_stream *strm)
{
	struct uvc_data *data = dev->data;
	union uvc_stream_descriptor *format_desc = NULL;
	struct video_caps caps;
	uint32_t prev_pixfmt = 0;
	uint32_t mask;
	int id = 0;
	int ret;

	/* Update the length of all control interface descriptors */
	data->desc->if_vc_header.wTotalLength += strm->desc->if_vc_ct.bLength;
	data->desc->if_vc_header.wTotalLength += strm->desc->if_vc_pu.bLength;
	data->desc->if_vc_header.wTotalLength += strm->desc->if_vc_su.bLength;
	data->desc->if_vc_header.wTotalLength += strm->desc->if_vc_xu.bLength;
	data->desc->if_vc_header.wTotalLength += strm->desc->if_vc_ot.bLength;

	/* Probe each video device for their controls support within each unit */
	mask = uvc_get_mask(strm->video_dev, uvc_control_map_xu);
	strm->desc->if_vc_xu.bmControls[0] = mask >> 0 & 0xff;
	strm->desc->if_vc_xu.bmControls[1] = mask >> 8 & 0xff;
	strm->desc->if_vc_xu.bmControls[2] = mask >> 16 & 0xff;

	mask = uvc_get_mask(strm->video_dev, uvc_control_map_ct);
	strm->desc->if_vc_ct.bmControls[0] = mask >> 0 & 0xff;
	strm->desc->if_vc_ct.bmControls[1] = mask >> 8 & 0xff;
	strm->desc->if_vc_ct.bmControls[2] = mask >> 16 & 0xff;
	mask = uvc_get_mask(strm->video_dev, uvc_control_map_pu);
	strm->desc->if_vc_pu.bmControls[0] = mask >> 0 & 0xff;
	strm->desc->if_vc_pu.bmControls[1] = mask >> 8 & 0xff;
	strm->desc->if_vc_pu.bmControls[2] = mask >> 16 & 0xff;
	mask = uvc_get_mask(strm->video_dev, uvc_control_map_su);

	/* Probe the video device for their format support */
	ret = video_get_caps(strm->video_dev, VIDEO_EP_OUT, &caps);
	if (ret != 0) {
		LOG_DBG("Could not load %s video format list", strm->video_dev->name);
		return ret;
	}

	/* Generate the list of format descrptors according to the video capabilities */
	for (int i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		const struct video_format_cap *cap = &caps.format_caps[i];

		/* Convert the flat capability structure into an UVC nested structure */
		if (prev_pixfmt != cap->pixelformat) {
			ret = uvc_init_format_desc(strm, &id, &format_desc, cap);
			if (ret != 0) {
				return ret;
			}
		}

		/* Always add the minimum value */
		ret = uvc_init_frame_desc(strm, &id, format_desc, cap, true);
		if (ret != 0) {
			return ret;
		}

		/* If min and max differs, also add the max */
		if (cap->width_min != cap->width_max || cap->height_min != cap->height_max) {
			ret = uvc_init_frame_desc(strm, &id, format_desc, cap, true);
			if (ret != 0) {
				return ret;
			}
		}

		prev_pixfmt = cap->pixelformat;
	}

	return 0;
}

static void uvc_init_desc_ptrs(const struct device *dev, struct usb_desc_header **desc_ptrs)
{
	struct uvc_data *data = dev->data;
	int src = 0;
	int dst = 0;

	LOG_DBG("Initializing descriptors pointer list %p", desc_ptrs);

	/* Skipping empty gaps in the descriptor pointer list */
	for (bool is_nil_desc = false; !is_nil_desc;) {
		is_nil_desc = (desc_ptrs[src] == &data->desc->nil_desc);

		if (desc_ptrs[src]->bLength == 0 && desc_ptrs[src] != &data->desc->nil_desc) {
			LOG_DBG("Skipping %u, bDescriptorType %u",
				src, desc_ptrs[src]->bDescriptorType);
			src++;
		} else {
			LOG_DBG("Copying %u to %u, bDescriptorType %u",
				src, dst, desc_ptrs[src]->bDescriptorType);
			desc_ptrs[dst] = desc_ptrs[src];
			src++, dst++;
		}
	}
}

static int uvc_init(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;
	int ret;

	if (atomic_test_and_set_bit(&data->state, UVC_STATE_INITIALIZED)) {
		LOG_DBG("Descriptors already initialized");
		return 0;
	}

	LOG_DBG("Initializing UVC class %p (%s)", dev, dev->name);

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		ret = uvc_init_desc(dev, strm);
		if (ret != 0) {
			return ret;
		}
	}

	/* Skip the empty gaps in the list of pointers */
	uvc_init_desc_ptrs(dev, data->fs_desc);
	uvc_init_desc_ptrs(dev, data->hs_desc);

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		/* Now that descriptors are ready, store the default probe */
		ret = uvc_vs_get_probe_struct(strm, &strm->default_probe, UVC_GET_CUR);
		if (ret != 0) {
			LOG_ERR("init: failed to query the default probe");
			return ret;
		}
	}

	return 0;
}

static void uvc_update_desc(const struct device *dev, struct usbd_class_data *const c_data)
{
	struct uvc_data *data = dev->data;

	uvc_init(c_data);
	data->desc->iad.bFirstInterface = data->desc->if_vc.bInterfaceNumber;

	for (int i = 0; data->streams[i].dev != NULL; i++) {
		struct uvc_stream *strm = &data->streams[i];

		strm->desc->if_vs_header.bEndpointAddress = uvc_get_bulk_in(strm);
		data->desc->if_vc_header.baInterfaceNr[i] = strm->desc->if_vs.bInterfaceNumber;
	}
}

static void *uvc_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	/* The descriptor pointer list is generated out of the video runtime API,
	 * it must be done before they are scanned by the USB device stack
	 */
	uvc_update_desc(dev, c_data);

	switch (speed) {
	case USBD_SPEED_FS:
		return (void *)data->fs_desc;
	case USBD_SPEED_HS:
		return (void *)data->hs_desc;
	default:
		__ASSERT_NO_MSG(false);
		return NULL;
	}
}

/* UVC data handling */

static int uvc_request(struct usbd_class_data *const c_data, struct net_buf *buf, int err)
{
	struct uvc_buf_info bi = *(struct uvc_buf_info *)udc_get_buf_info(buf);
	struct uvc_stream *strm = bi.stream;

	net_buf_unref(buf);

	if (bi.udc.ep == uvc_get_bulk_in(bi.stream)) {
		LOG_DBG("Request completed for ubuf %p, vbuf %p", buf, bi.vbuf);
		if (bi.vbuf != NULL) {
			k_fifo_put(&strm->fifo_out, bi.vbuf);
		}

		/* There is now one more net_buff buffer available */
		k_work_submit(&strm->work);
	} else {
		LOG_WRN("Request on unknown endpoint 0x%02x", bi.udc.ep);
	}

	return 0;
}

/* The queue of video frame fragments (vbuf) is processed, each fragment (data)
 * is prepended by the UVC header (h). The result is cut into USB packets (pkt)
 * submitted to the USB:
 *
 * [h+data] [data::]...[data::] [data] ... [h+data] [data::]...[data::] [data] ...
 * [pkt:::] [pkt:::]...[pkt:::] [pkt:] ... [pkt:::] [pkt:::]...[pkt:::] [pkt:] ...
 * [vbuf:::::::::::::::::::::::::::::] ... [vbuf:::::::::::::::::::::::::::::] ...
 * [frame::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::] ...
 * +-------------------------------------------------------------------------> time
 *
 * This function uvc_queue_vbuf() is called once per usb packet (pkt).
 *
 * @retval 0 if vbuf was partially transferred.
 * @retval 1 if vbuf was fully transferred and can be released.
 * @return Negative error code on failure.
 */
static int uvc_queue_vbuf(struct uvc_stream *strm, struct video_buffer *vbuf)
{
	struct uvc_data *data = strm->dev->data;
	struct video_format *fmt = &strm->video_fmt;
	struct net_buf *buf;
	struct uvc_buf_info *bi;
	size_t mps = uvc_get_bulk_mps(data->c_data);
	size_t next_vbuf_offset = strm->vbuf_offset;
	size_t next_line_offset = vbuf->line_offset;
	int ret;

	/* Start-of-Transfer condition */
	if (strm->vbuf_offset == 0) {
		buf = net_buf_alloc_len(&uvc_pool, mps, K_NO_WAIT);
		if (buf == NULL) {
			LOG_DBG("Cannot allocate first USB buffer for now");
			return -ENOMEM;
		}

		if (fmt->pitch > 0) {
			next_line_offset += vbuf->bytesused / fmt->pitch;
		}

		LOG_INF("New USB transfer, bmHeaderInfo 0x%02x, buffer size %u, lines %u to %u",
			strm->payload_header.bmHeaderInfo, vbuf->bytesused,
			vbuf->line_offset, next_line_offset);

		/* Only the first 2 fields supported for now, the rest is padded with 0x00 */
		net_buf_add_mem(buf, &strm->payload_header, 2);
		net_buf_add(buf, CONFIG_USBD_VIDEO_HEADER_SIZE - buf->len);

		/* Copy the bytes, needed for the first packet only */
		next_vbuf_offset = MIN(vbuf->bytesused, net_buf_tailroom(buf));
		net_buf_add_mem(buf, vbuf->buffer, next_vbuf_offset);

		if (fmt->pitch == 0 || next_line_offset >= fmt->height) {
			LOG_INF("End of frame");

			/* Flag that this current transfer is the last */
			((struct uvc_payload_header *)buf->data)->bmHeaderInfo |=
				UVC_BMHEADERINFO_END_OF_FRAME;

			/* Toggle the Frame ID of the next vbuf */
			strm->payload_header.bmHeaderInfo ^= UVC_BMHEADERINFO_FRAMEID;

			next_line_offset = 0;
		}
	} else {
		buf = net_buf_alloc_with_data(&uvc_pool, vbuf->buffer + strm->vbuf_offset,
					      MIN(vbuf->bytesused - strm->vbuf_offset, mps),
					      K_NO_WAIT);
		if (buf == NULL) {
			LOG_DBG("Cannot allocate continuation USB buffer for now");
			return -ENOMEM;
		}
		next_vbuf_offset = strm->vbuf_offset + mps;
	}

	bi = (struct uvc_buf_info *)udc_get_buf_info(buf);
	bi->udc.ep = uvc_get_bulk_in(strm);
	bi->stream = strm;

	/* End-of-Transfer condition */
	if (next_vbuf_offset >= vbuf->bytesused) {
		bi->vbuf = vbuf;
		bi->udc.zlp = (buf->len == mps);
	}

	LOG_DBG("Video buffer %p, offset %u/%u, size %u",
		vbuf, strm->vbuf_offset, vbuf->bytesused, buf->len);

	ret = usbd_ep_enqueue(data->c_data, buf);
	if (ret != 0) {
		net_buf_unref(buf);
		return ret;
	}

	/* End-of-Transfer condition */
	if (next_vbuf_offset >= vbuf->bytesused) {
		strm->vbuf_offset = 0;
		return 1;
	}

	strm->vbuf_offset = next_vbuf_offset;
	vbuf->line_offset = next_line_offset;
	return 0;
}

static void uvc_worker(struct k_work *work)
{
	struct uvc_stream *strm = CONTAINER_OF(work, struct uvc_stream, work);
	struct video_buffer *vbuf;
	int ret;

	if (!atomic_test_bit(&strm->state, UVC_STATE_ENABLED) ||
	    !atomic_test_bit(&strm->state, UVC_STATE_READY)) {
		LOG_DBG("UVC not ready yet");
		return;
	}

	while ((vbuf = k_fifo_peek_head(&strm->fifo_in)) != NULL) {
		/* Pausing the feed at UVC level will accumulate buffers in the input queue */
		if (atomic_test_bit(&strm->state, UVC_STATE_PAUSED)) {
			break;
		}

		ret = uvc_queue_vbuf(strm, vbuf);
		if (ret != 0) {
			LOG_DBG("Could not transfer %p for now", vbuf);
			break;
		}
		if (ret == 1) {
			LOG_DBG("Video buffer %p transferred, removing from the queue", vbuf);
			k_fifo_get(&strm->fifo_in, K_NO_WAIT);
		}
	}
}

static void uvc_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		/* Catch-up with buffers that might have been delayed */
		atomic_set_bit(&strm->state, UVC_STATE_ENABLED);
		k_work_submit(&strm->work);
	}
}

static void uvc_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		atomic_clear_bit(&strm->state, UVC_STATE_ENABLED);
	}
}

static void uvc_update(struct usbd_class_data *const c_data, uint8_t iface, uint8_t alternate)
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

int uvc_get_stream(const struct device *dev, enum video_endpoint_id ep, struct uvc_stream **result)
{
	const struct uvc_data *data = dev->data;

	if (ep == VIDEO_EP_OUT) {
		return -EINVAL;
	}

	if (ep == VIDEO_EP_IN || ep == VIDEO_EP_ALL) {
		*result = &data->streams[0];
		return 0;
	}

	/* Iterate instead of dereference to prevent overflow */
	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++, ep--) {
		if (ep == 0) {
			*result = strm;
			return 0;
		}
	}

	return -ENODEV;
}

static int uvc_enqueue(const struct device *dev, enum video_endpoint_id ep,
		       struct video_buffer *vbuf)
{
	struct uvc_stream *strm;
	int ret;

	ret = uvc_get_stream(dev, ep, &strm);
	if (ret != 0) {
		return ret;
	}

	k_fifo_put(&strm->fifo_in, vbuf);
	k_work_submit(&strm->work);

	return 0;
}

static int uvc_dequeue(const struct device *dev, enum video_endpoint_id ep,
		       struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct uvc_stream *strm;
	int ret;

	ret = uvc_get_stream(dev, ep, &strm);
	if (ret != 0) {
		return ret;
	}

	*vbuf = k_fifo_get(&strm->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int uvc_get_format(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct uvc_stream *strm;
	int ret;

	ret = uvc_get_stream(dev, ep, &strm);
	if (ret != 0) {
		return ret;
	}

	if (!atomic_test_bit(&strm->state, UVC_STATE_ENABLED) ||
	    !atomic_test_bit(&strm->state, UVC_STATE_READY)) {
		return -EAGAIN;
	}

	return video_get_format(strm->video_dev, VIDEO_EP_OUT, fmt);
}

static int uvc_set_stream(const struct device *dev, bool enable)
{
	struct uvc_data *data = dev->data;

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		if (enable) {
			atomic_clear_bit(&strm->state, UVC_STATE_PAUSED);
			k_work_submit(&strm->work);
		} else {
			atomic_set_bit(&strm->state, UVC_STATE_PAUSED);
		}
	}
	return 0;
}

static int uvc_get_caps(const struct device *dev, enum video_endpoint_id ep,
			struct video_caps *caps)
{
	struct uvc_stream *strm;
	int ret;

	ret = uvc_get_stream(dev, ep, &strm);
	if (ret != 0) {
		return ret;
	}

	return video_get_caps(strm->video_dev, VIDEO_EP_OUT, caps);
}

static DEVICE_API(video, uvc_video_api) = {
	.get_format = uvc_get_format,
	.set_stream = uvc_set_stream,
	.get_caps = uvc_get_caps,
	.enqueue = uvc_enqueue,
	.dequeue = uvc_dequeue,
};

static int uvc_preinit(const struct device *dev)
{
	struct uvc_data *data = dev->data;

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		k_fifo_init(&strm->fifo_in);
		k_fifo_init(&strm->fifo_out);
		k_work_init(&strm->work, &uvc_worker);
	}

	return 0;
}

#define UVC_FOREACH_STREAM(n, fn) DT_FOREACH_CHILD_STATUS_OKAY(DT_INST_CHILD(n, port), fn)

enum uvc_unit_id {
	UVC_UNIT_ID_ZERO = 0,
#define UVC_UNIT_ID(n)								\
	UVC_UNIT_ID_CT_##n,							\
	UVC_UNIT_ID_SU_##n,							\
	UVC_UNIT_ID_PU_##n,							\
	UVC_UNIT_ID_XU_##n,							\
	UVC_UNIT_ID_OT_##n,
#define UVC_UNIT_IDS(n) UVC_FOREACH_STREAM(n, UVC_UNIT_ID)
	DT_INST_FOREACH_STATUS_OKAY(UVC_UNIT_IDS)
};

#define UVC_DEFINE_STREAM_DESCRIPTOR(n)						\
static union uvc_stream_descriptor						\
	uvc_fmt_frm_desc_##n[CONFIG_USBD_VIDEO_MAX_VS_DESC];			\
										\
struct usbd_uvc_strm_desc uvc_strm_desc_##n = {					\
	.if_vc_ct = {								\
		.bLength = sizeof(struct uvc_camera_terminal_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_INPUT_TERMINAL,			\
		.bTerminalID = UVC_UNIT_ID_CT_##n,				\
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
	.if_vc_su = {								\
		.bLength = sizeof(struct uvc_selector_unit_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_SELECTOR_UNIT,			\
		.bUnitID = UVC_UNIT_ID_SU_##n,					\
		.bNrInPins = 1,							\
		.baSourceID = {UVC_UNIT_ID_CT_##n},				\
		.iSelector = 0,							\
	},									\
										\
	.if_vc_pu = {								\
		.bLength = sizeof(struct uvc_processing_unit_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_PROCESSING_UNIT,			\
		.bUnitID = UVC_UNIT_ID_PU_##n,					\
		.bSourceID = UVC_UNIT_ID_SU_##n,				\
		.wMaxMultiplier = sys_cpu_to_le16(0),				\
		.bControlSize = 3,						\
		.bmControls = {0},						\
		.iProcessing = 0,						\
		.bmVideoStandards = 0,						\
	},									\
										\
	.if_vc_xu = {								\
		.bLength = sizeof(struct uvc_extension_unit_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_EXTENSION_UNIT,			\
		.bUnitID = UVC_UNIT_ID_XU_##n,					\
		.guidExtensionCode = {0},					\
		.bNumControls = 0,						\
		.bNrInPins = 1,							\
		.baSourceID = {UVC_UNIT_ID_PU_##n},				\
		.bControlSize = 4,						\
		.bmControls = {0},						\
		.iExtension = 0,						\
	},									\
										\
	.if_vc_ot = {								\
		.bLength = sizeof(struct uvc_output_terminal_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_OUTPUT_TERMINAL,			\
		.bTerminalID = UVC_UNIT_ID_OT_##n,				\
		.wTerminalType = sys_cpu_to_le16(UVC_TT_STREAMING),		\
		.bAssocTerminal = 0,						\
		.bSourceID = UVC_UNIT_ID_XU_##n,				\
		.iTerminal = 0,							\
	},									\
										\
	.if_vs = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 1,						\
		.bInterfaceClass = USB_BCC_VIDEO,				\
		.bInterfaceSubClass = UVC_SC_VIDEOSTREAMING,			\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if_vs_header = {							\
		.bLength = sizeof(struct uvc_stream_header_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VS_INPUT_HEADER,			\
		.bNumFormats = 0,						\
		.wTotalLength = sys_cpu_to_le16(				\
			sizeof(struct uvc_stream_header_descriptor) +		\
			sizeof(struct uvc_color_descriptor)),			\
		.bEndpointAddress = 0x81,					\
		.bmInfo = 0,							\
		.bTerminalLink = UVC_UNIT_ID_OT_##n,				\
		.bStillCaptureMethod = 0,					\
		.bTriggerSupport = 0,						\
		.bTriggerUsage = 0,						\
		.bControlSize = 0,						\
	},									\
										\
	.if_vs_formats = uvc_fmt_frm_desc_##n,					\
	.if_vs_format_num = ARRAY_SIZE(uvc_fmt_frm_desc_##n),			\
										\
	.if_vs_ep_fs = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64),				\
		.bInterval = 0,							\
	},									\
										\
	.if_vs_ep_hs = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0,							\
	},									\
										\
	.if_vs_color = {							\
		.bLength = sizeof(struct uvc_color_descriptor),			\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VS_COLORFORMAT,			\
		.bColorPrimaries = 1, /* BT.709, sRGB (default) */		\
		.bTransferCharacteristics = 1, /* BT.709 (default) */		\
		.bMatrixCoefficients = 4, /* SMPTE 170M, BT.601 (default) */	\
	},									\
};

#define UVC_DEFINE_DESCRIPTOR(n)						\
static struct usbd_uvc_desc uvc_desc_##n = {					\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 1 + DT_CHILD_NUM(DT_INST_CHILD(n, port)),	\
		.bFunctionClass = USB_BCC_VIDEO,				\
		.bFunctionSubClass = UVC_SC_VIDEO_INTERFACE_COLLECTION,		\
		.bFunctionProtocol = 0,						\
		.iFunction = 0,							\
	},									\
										\
	.if_vc = {								\
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
	.if_vc_header = {							\
		.bLength = 12 + DT_CHILD_NUM(DT_INST_CHILD(n, port)),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UVC_VC_HEADER,				\
		.bcdUVC = sys_cpu_to_le16(0x0150),				\
		.wTotalLength = sys_cpu_to_le16(				\
			12 + DT_CHILD_NUM(DT_INST_CHILD(n, port))),		\
		.dwClockFrequency = sys_cpu_to_le32(30000000),			\
		.bInCollection = DT_CHILD_NUM(DT_INST_CHILD(n, port)),		\
		.baInterfaceNr = {0},						\
	},									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
static struct usb_desc_header *uvc_fs_desc_##n[] = {				\
	(struct usb_desc_header *)&uvc_desc_##n.iad,				\
	(struct usb_desc_header *)&uvc_desc_##n.if_vc,				\
	(struct usb_desc_header *)&uvc_desc_##n.if_vc_header,			\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_CONTROL_PTRS)				\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_STREAMING_PTRS_FS)			\
	(struct usb_desc_header *)&uvc_desc_##n.nil_desc,			\
};										\
										\
static struct usb_desc_header *uvc_hs_desc_##n[] = {				\
	(struct usb_desc_header *)&uvc_desc_##n.iad,				\
	(struct usb_desc_header *)&uvc_desc_##n.if_vc,				\
	(struct usb_desc_header *)&uvc_desc_##n.if_vc_header,			\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_CONTROL_PTRS)				\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_STREAMING_PTRS_HS)			\
	(struct usb_desc_header *)&uvc_desc_##n.nil_desc,			\
};

#define UVC_VIDEO_CONTROL_PTRS(n)						\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vc_ct,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vc_su,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vc_pu,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vc_xu,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vc_ot,
#define UVC_VIDEO_STREAMING_PTRS(i, n)						\
	(struct usb_desc_header *)&uvc_fmt_frm_desc_##n[i],
#define UVC_VIDEO_STREAMING_PTRS_FS(n)						\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs_header,		\
	LISTIFY(CONFIG_USBD_VIDEO_MAX_VS_DESC, UVC_VIDEO_STREAMING_PTRS, (), n)	\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs_color,		\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs_ep_fs,
#define UVC_VIDEO_STREAMING_PTRS_HS(n)						\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs_header,		\
	LISTIFY(CONFIG_USBD_VIDEO_MAX_VS_DESC, UVC_VIDEO_STREAMING_PTRS, (), n)	\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs_color,		\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if_vs_ep_hs,

#define UVC_STREAM(n)								\
	{									\
		.dev = DEVICE_DT_GET(DT_GPARENT(n)),				\
		.desc = &uvc_strm_desc_##n,					\
		.payload_header.bHeaderLength = CONFIG_USBD_VIDEO_HEADER_SIZE,	\
		.format_id = 1,							\
		.frame_id = 1,							\
		.video_dev = DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(n)),		\
		.video_frmival.denominator = NSEC_PER_SEC / 100,		\
	},

#define USBD_VIDEO_DT_DEVICE_DEFINE(n)						\
	BUILD_ASSERT(DT_INST_CHILD_NUM(n) > 0, "Needs at least one source");	\
	UVC_FOREACH_STREAM(n, UVC_DEFINE_STREAM_DESCRIPTOR)			\
	UVC_DEFINE_DESCRIPTOR(n)						\
										\
	USBD_DEFINE_CLASS(uvc_c_data_##n, &uvc_class_api,			\
			  (void *)DEVICE_DT_INST_GET(n), NULL);			\
										\
	static struct uvc_stream uvc_streams_##n[] = {				\
		UVC_FOREACH_STREAM(n, UVC_STREAM)				\
		{0},								\
	};									\
										\
	struct uvc_data uvc_data_##n = {					\
		.c_data = &uvc_c_data_##n,					\
		.streams = uvc_streams_##n,					\
		.desc = &uvc_desc_##n,						\
		.fs_desc = uvc_fs_desc_##n,					\
		.hs_desc = uvc_hs_desc_##n,					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uvc_preinit, NULL, &uvc_data_##n, NULL,	\
		POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &uvc_video_api);

DT_INST_FOREACH_STATUS_OKAY(USBD_VIDEO_DT_DEVICE_DEFINE)
