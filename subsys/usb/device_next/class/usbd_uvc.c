/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uvc

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include "usbd_uvc_macros.h"

LOG_MODULE_REGISTER(usbd_uvc, CONFIG_USBD_UVC_LOG_LEVEL);

#define UVC_CLASS_ENABLED			0
#define UVC_CLASS_READY				1

/* Video Class-Specific Request Codes */
#define RC_UNDEFINED				0x00
#define SET_CUR					0x01
#define GET_CUR					0x81
#define GET_MIN					0x82
#define GET_MAX					0x83
#define GET_RES					0x84
#define GET_LEN					0x85
#define GET_INFO				0x86
#define GET_DEF					0x87

/* 4.2.1.2 Request Error Code Control */
#define ERR_NOT_READY				0x01
#define ERR_WRONG_STATE				0x02
#define ERR_OUT_OF_RANGE			0x04
#define ERR_INVALID_UNIT			0x05
#define ERR_INVALID_CONTROL			0x06
#define ERR_INVALID_REQUEST			0x07
#define ERR_INVALID_VALUE_WITHIN_RANGE		0x08
#define ERR_UNKNOWN				0xff

/* Flags announcing which controls are supported */
#define INFO_SUPPORTS_GET			BIT(0)
#define INFO_SUPPORTS_SET			BIT(1)

/* Video and Still Image Payload Headers */
#define UVC_BMHEADERINFO_FRAMEID		BIT(0)
#define UVC_BMHEADERINFO_END_OF_FRAME		BIT(1)
#define UVC_BMHEADERINFO_HAS_PRESENTATIONTIME	BIT(2)
#define UVC_BMHEADERINFO_HAS_SOURCECLOCK	BIT(3)
#define UVC_BMHEADERINFO_PAYLOAD_SPECIFIC_BIT	BIT(4)
#define UVC_BMHEADERINFO_STILL_IMAGE		BIT(5)
#define UVC_BMHEADERINFO_ERROR			BIT(6)
#define UVC_BMHEADERINFO_END_OF_HEADER		BIT(7)

/* Video Probe and Commit Controls */
struct uvc_probe {
	uint16_t bmHint;
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint32_t dwFrameInterval;
	uint16_t wKeyFrameRate;
	uint16_t wPFrameRate;
	uint16_t wCompQuality;
	uint16_t wCompWindowSize;
	uint16_t wDelay;
	uint32_t dwMaxVideoFrameSize;
	uint32_t dwMaxPayloadTransferSize;
	uint32_t dwClockFrequency;
	uint8_t bmFramingInfo;
#define UVC_BMFRAMING_INFO_FID			BIT(0)
#define UVC_BMFRAMING_INFO_EOF			BIT(1)
#define UVC_BMFRAMING_INFO_EOS			BIT(2)
	uint8_t bPreferedVersion;
	uint8_t bMinVersion;
	uint8_t bMaxVersion;
	uint8_t bUsage;
	uint8_t bBitDepthLuma;
	uint8_t bmSettings;
	uint8_t bMaxNumberOfRefFramesPlus1;
	uint16_t bmRateControlModes;
	uint64_t bmLayoutPerStream;
} __packed;

/* Video and Still Image Payload Headers */
struct uvc_payload_header {
	uint8_t bHeaderLength;
	uint8_t bmHeaderInfo;
	uint32_t dwPresentationTime;	/* optional */
	uint32_t scrSourceClockSTC;	/* optional */
	uint16_t scrSourceClockSOF;	/* optional */
} __packed;

/* Lookup table between the format and frame index, and the video caps */
struct uvc_format {
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	uint8_t bits_per_pixel;
	uint32_t frame_interval;	/* see #72254 */
};

/* Lookup table between the interface ID and the control function */
struct uvc_control {
	uint8_t entity_id;
	int (*fn)(const struct usb_setup_packet *setup, struct net_buf *buf,
		  const struct device *dev);
	const struct device *target_dev;
	uint64_t mask;
	uint32_t *defaults;
};

struct uvc_conf {
	/* USBD class structure */
	struct usbd_class_data *c_data;
	const struct video_format_cap *caps;
	/* UVC format lookup tables */
	const struct uvc_format *formats;
	/* UVC control lookup table */
	const struct uvc_control *controls;
	/* UVC Descriptors */
	struct usb_desc_header *const *fs_desc;
	struct usb_desc_header *const *hs_desc;
	/* UVC Fields that need to be accessed */
	uint8_t *desc_iad_ifnum;
	uint8_t *desc_if_vc_ifnum;
	uint8_t *desc_if_vc_header_ifnum;
	uint8_t *desc_if_vs_ifnum;
	uint8_t *desc_if_vs_header_epaddr;
	uint8_t *fs_desc_ep_epaddr;
	uint8_t *hs_desc_ep_epaddr;
	uint8_t *ss_desc_ep_epaddr;
	/* Video device controlled by the host via UVC */
	const struct device *output_dev;
};

struct uvc_data {
	/* UVC device reference for getting the context from workers */
	const struct device *dev;
	/* USBD class state */
	atomic_t state;
	/* UVC worker to process the queue */
	struct k_work work;
	/* UVC format currently selected */
	int format_id;
	/* UVC error from latest request */
	int err;
	/* UVC probe-commit control default values */
	struct uvc_probe default_probe;
	/* UVC payload header, passed just before the image data */
	struct uvc_payload_header payload_header;
	/* Video FIFOs for submission (in) and completion (out) queue */
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	/* Offset in bytes within the fragment including the header */
	uint32_t xfer_offset;
};

struct uvc_buf_info {
	/* Regular UDC buf info so that it can be passed to USBD directly */
	struct udc_buf_info udc;
	/* Extra field at the end */
	struct video_buffer *vbuf;
} __packed;

NET_BUF_POOL_FIXED_DEFINE(uvc_pool, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 100, 0,
			  sizeof(struct uvc_buf_info), NULL);

static int uvc_get_format(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt);

static int uvc_get_errno(int err)
{
	switch (err) {
	case EBUSY:		/* Busy and not ready */
	case EAGAIN:		/* Try again when the device becomes ready */
	case EINPROGRESS:	/* Will be ready after this ongoing operation */
	case EALREADY:		/* Already enqueued, will be ready when done */
		return ERR_NOT_READY;
	case EOVERFLOW:		/* Values overflowed the range */
	case ERANGE:		/* Value not in range */
	case E2BIG:		/* Value too big for the range */
		return ERR_OUT_OF_RANGE;
	case EDOM:		/* Invalid but still in the range */
	case EINVAL:		/* Invalid argument but not ERANGE */
		return ERR_INVALID_VALUE_WITHIN_RANGE;
	case ENODEV:		/* No device supporting this request */
	case ENOTSUP:		/* Request not supported */
	case ENOSYS:		/* Request not implemented */
		return ERR_INVALID_REQUEST;
	default:
		return ERR_UNKNOWN;
	}
}

static uint32_t uvc_get_video_cid(const struct usb_setup_packet *setup, uint32_t cid)
{
	switch (setup->bRequest) {
	case GET_DEF:
		return VIDEO_CTRL_GET_DEF | cid;
	case GET_CUR:
		return VIDEO_CTRL_GET_CUR | cid;
	case GET_MIN:
		return VIDEO_CTRL_GET_MIN | cid;
	case GET_MAX:
		return VIDEO_CTRL_GET_MAX | cid;
	default:
		__ASSERT_NO_MSG(false);
		return 0;
	}
}

static int uvc_buf_add(struct net_buf *buf, uint16_t size, uint32_t value)
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
		LOG_WRN("control: invalid size %u", size);
		return -ENOTSUP;
	}
}

static int uvc_buf_remove(struct net_buf *buf, uint16_t size, uint32_t *value)
{
	switch (size) {
	case 4:
		*value = net_buf_remove_le32(buf);
		return 0;
	case 2:
		*value = net_buf_remove_le16(buf);
		return 0;
	case 1:
		*value = net_buf_remove_u8(buf);
		return 0;
	default:
		LOG_WRN("control: invalid size %u", size);
		return -ENOTSUP;
	}
}

static uint8_t uvc_get_bulk_in(const struct device *const dev)
{
	const struct uvc_conf *conf = dev->config;

	switch (usbd_bus_speed(usbd_class_get_ctx(conf->c_data))) {
	case USBD_SPEED_FS:
		return *conf->fs_desc_ep_epaddr;
	case USBD_SPEED_HS:
		return *conf->hs_desc_ep_epaddr;
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
		return 64u;
	case USBD_SPEED_HS:
                return 512U;
	case USBD_SPEED_SS:
		return 1024U;
	default:
		__ASSERT_NO_MSG(false);
		return 0;
	}
}


static uint32_t uvc_get_max_frame_size(const struct device *dev, int id)
{
	const struct uvc_conf *conf = dev->config;
	const struct video_format_cap *cap = &conf->caps[id];
	const struct uvc_format *ufmt = &conf->formats[id];

	return cap->width_max * cap->height_max * ufmt->bits_per_pixel / 8;
}

static int uvc_control_probe_format_index(const struct device *dev, uint8_t request,
					  struct uvc_probe *probe)
{
	const struct uvc_conf *conf = dev->config;
	struct uvc_data *data = dev->data;

	switch (request) {
	case GET_MIN:
		probe->bFormatIndex = UINT8_MAX;
		for (const struct uvc_format *fmt = conf->formats; fmt->bFormatIndex; fmt++) {
			probe->bFormatIndex = MIN(fmt->bFormatIndex, probe->bFormatIndex);
		}
		break;
	case GET_MAX:
		probe->bFormatIndex = 0;
		for (const struct uvc_format *fmt = conf->formats; fmt->bFormatIndex; fmt++) {
			probe->bFormatIndex = MAX(fmt->bFormatIndex, probe->bFormatIndex);
		}
		break;
	case GET_RES:
		probe->bFormatIndex = 1;
		break;
	case GET_CUR:
		probe->bFormatIndex = conf->formats[data->format_id].bFormatIndex;
		break;
	case SET_CUR:
		if (probe->bFormatIndex == 0) {
			return 0;
		}
		for (size_t i = 0; conf->formats[i].bFormatIndex; i++) {
			if (conf->formats[i].bFormatIndex == probe->bFormatIndex) {
				data->format_id = i;
				return 0;
			}
		}
		LOG_WRN("probe: format index %u not found", probe->bFormatIndex);
		return -ENOTSUP;
	}
	return 0;
}

static int uvc_control_probe_frame_index(const struct device *dev, uint8_t request,
					 struct uvc_probe *probe)
{
	const struct uvc_conf *conf = dev->config;
	struct uvc_data *data = dev->data;
	const struct uvc_format *cur = &conf->formats[data->format_id];

	switch (request) {
	case GET_MIN:
		probe->bFrameIndex = UINT8_MAX;
		for (const struct uvc_format *fmt = conf->formats; fmt->bFormatIndex; fmt++) {
			if (fmt->bFormatIndex == cur->bFormatIndex) {
				probe->bFrameIndex = MIN(fmt->bFrameIndex, probe->bFrameIndex);
			}
		}
		break;
	case GET_MAX:
		probe->bFrameIndex = 0;
		for (const struct uvc_format *fmt = conf->formats; fmt->bFormatIndex; fmt++) {
			if (fmt->bFormatIndex == cur->bFormatIndex) {
				probe->bFrameIndex = MAX(fmt->bFrameIndex, probe->bFrameIndex);
			}
		}
		break;
	case GET_RES:
		probe->bFrameIndex = 1;
		break;
	case GET_CUR:
		probe->bFrameIndex = cur->bFrameIndex;
		break;
	case SET_CUR:
		if (probe->bFrameIndex == 0) {
			return 0;
		}
		for (size_t i = 0; conf->formats[i].bFormatIndex; i++) {
			const struct uvc_format *fmt = &conf->formats[i];

			LOG_DBG("bFormatIndex %u, cur %u", fmt->bFormatIndex, cur->bFormatIndex);
			LOG_DBG("bFrameIndex %u, want %u", fmt->bFrameIndex, probe->bFrameIndex);
			if (fmt->bFormatIndex == cur->bFormatIndex &&
			    fmt->bFrameIndex == probe->bFrameIndex) {
				data->format_id = i;
				return 0;
			}
		}
		LOG_WRN("probe: frame index %u not found", probe->bFrameIndex);
		return -ENOTSUP;
	}
	return 0;
}

static int uvc_control_probe_frame_interval(const struct device *dev, uint8_t request,
					    struct uvc_probe *probe)
{
	const struct uvc_conf *conf = dev->config;
	struct uvc_data *data = dev->data;
	uint32_t frmival = conf->formats[data->format_id].frame_interval;

	switch (request) {
	case GET_MIN:
	case GET_MAX:
		/* TODO call the frame interval API on the video source once supported */
		probe->dwFrameInterval = sys_cpu_to_le32(frmival);
		break;
	case GET_RES:
		probe->dwFrameInterval = sys_cpu_to_le32(1);
		break;
	case SET_CUR:
		/* TODO call the frame interval API on the video source once supported */
		break;
	}
	return 0;
}

static int uvc_control_probe_max_video_frame_size(const struct device *dev, uint8_t request,
						  struct uvc_probe *probe)
{
	struct uvc_data *data = dev->data;
	uint32_t max_frame_size = uvc_get_max_frame_size(dev, data->format_id);

	switch (request) {
	case GET_MIN:
	case GET_MAX:
	case GET_CUR:
		probe->dwMaxVideoFrameSize = sys_cpu_to_le32(max_frame_size);
		break;
	case GET_RES:
		probe->dwMaxVideoFrameSize = sys_cpu_to_le32(1);
		break;
	case SET_CUR:
		if (sys_le32_to_cpu(probe->dwMaxVideoFrameSize) > 0 &&
		    sys_le32_to_cpu(probe->dwMaxVideoFrameSize) != max_frame_size) {
			LOG_WRN("probe: dwMaxVideoFrameSize is read-only");
		}
		break;
	}
	return 0;
}

static int uvc_control_probe_max_payload_size(const struct device *dev, uint8_t request,
					      struct uvc_probe *probe)
{
	struct uvc_data *data = dev->data;
	uint32_t max_payload_size =
		uvc_get_max_frame_size(dev, data->format_id) + CONFIG_USBD_VIDEO_HEADER_SIZE;

	switch (request) {
	case GET_MIN:
	case GET_MAX:
	case GET_CUR:
		probe->dwMaxPayloadTransferSize = sys_cpu_to_le32(max_payload_size);
		break;
	case GET_RES:
		probe->dwMaxPayloadTransferSize = sys_cpu_to_le32(1);
		break;
	case SET_CUR:
		if (sys_le32_to_cpu(probe->dwMaxPayloadTransferSize) > 0 &&
		    sys_le32_to_cpu(probe->dwMaxPayloadTransferSize) != max_payload_size) {
			LOG_WRN("probe: dwPayloadTransferSize is read-only");
		}
		break;
	}
	return 0;
}

#if CONFIG_USBD_UVC_LOG_LEVEL >= LOG_LEVEL_DBG
static void uvc_log_probe(const char *name, uint32_t probe)
{
	if (probe > 0) {
		LOG_DBG(" %s %u", name, probe);
	}
}
#endif

static char const *uvc_get_request_str(const struct usb_setup_packet *setup)
{
	switch (setup->bRequest) {
	case SET_CUR:
		return "SET_CUR";
	case GET_CUR:
		return "GET_CUR";
	case GET_MIN:
		return "GET_MIN";
	case GET_MAX:
		return "GET_MAX";
	case GET_RES:
		return "GET_RES";
	case GET_LEN:
		return "GET_LEN";
	case GET_DEF:
		return "GET_DEF";
	case GET_INFO:
		return "GET_INFO";
	default:
		return "(unknown)";
	}
}

static int uvc_control_probe(const struct device *dev, uint8_t request, struct uvc_probe *probe)
{
	static int (*control_fn[])(const struct device *, uint8_t, struct uvc_probe *) = {
		uvc_control_probe_format_index,
		uvc_control_probe_frame_index,
		uvc_control_probe_frame_interval,
		uvc_control_probe_max_video_frame_size,
		uvc_control_probe_max_payload_size,
	};
	int err;

	switch (request) {
	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	case GET_CUR:
	case SET_CUR:
		break;
	default:
		LOG_WRN("control: invalid bRequest %u", request);
		return -EINVAL;
	}

	/* Dynamic fields */
	for (size_t i = 0; i < ARRAY_SIZE(control_fn); i++) {
		err = control_fn[i](dev, request, probe);
		if (err) {
			return err;
		}
	}

#if CONFIG_USBD_UVC_LOG_LEVEL >= LOG_LEVEL_DBG
	uvc_log_probe("bmHint", sys_le16_to_cpu(probe->bmHint));
	uvc_log_probe("bFormatIndex", probe->bFormatIndex);
	uvc_log_probe("bFrameIndex", probe->bFrameIndex);
	uvc_log_probe("dwFrameInterval (us)", sys_le32_to_cpu(probe->dwFrameInterval) / 10);
	uvc_log_probe("wKeyFrameRate", sys_le16_to_cpu(probe->wKeyFrameRate));
	uvc_log_probe("wPFrameRate", sys_le16_to_cpu(probe->wPFrameRate));
	uvc_log_probe("wCompQuality", sys_le16_to_cpu(probe->wCompQuality));
	uvc_log_probe("wCompWindowSize", sys_le16_to_cpu(probe->wCompWindowSize));
	uvc_log_probe("wDelay (ms)", sys_le16_to_cpu(probe->wDelay));
	uvc_log_probe("dwMaxVideoFrameSize", sys_le32_to_cpu(probe->dwMaxVideoFrameSize));
	uvc_log_probe("dwMaxPayloadTransferSize", sys_le32_to_cpu(probe->dwMaxPayloadTransferSize));
	uvc_log_probe("dwClockFrequency (Hz)", sys_le32_to_cpu(probe->dwClockFrequency));
	uvc_log_probe("bmFramingInfo", probe->bmFramingInfo);
	uvc_log_probe("bPreferedVersion", probe->bPreferedVersion);
	uvc_log_probe("bMinVersion", probe->bMinVersion);
	uvc_log_probe("bMaxVersion", probe->bMaxVersion);
	uvc_log_probe("bUsage", probe->bUsage);
	uvc_log_probe("bBitDepthLuma", probe->bBitDepthLuma + 8);
	uvc_log_probe("bmSettings", probe->bmSettings);
	uvc_log_probe("bMaxNumberOfRefFramesPlus1", probe->bMaxNumberOfRefFramesPlus1);
	uvc_log_probe("bmRateControlModes", probe->bmRateControlModes);
	uvc_log_probe("bmLayoutPerStream", probe->bmLayoutPerStream);
#endif

	/* Static or unimplemented fields */
	probe->dwClockFrequency = sys_cpu_to_le32(1);
	/* Include Frame ID and EOF fields in the payload header */
	probe->bmFramingInfo = BIT(0) | BIT(1);
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
	/* TODO devicetree */
	probe->wDelay = sys_cpu_to_le16(1);

	return 0;
}

static int uvc_control_commit(const struct device *dev, uint8_t request, struct uvc_probe *probe)
{
	const struct uvc_conf *conf = dev->config;
	struct uvc_data *data = dev->data;
	struct video_format fmt = {0};
	int err;

	switch (request) {
	case GET_CUR:
		return uvc_control_probe(dev, request, probe);
	case SET_CUR:
		err = uvc_control_probe(dev, request, probe);
		if (err) {
			return err;
		}

		atomic_set_bit(&data->state, UVC_CLASS_READY);
		k_work_submit(&data->work);

		err = uvc_get_format(dev, VIDEO_EP_IN, &fmt);
		if (err) {
			LOG_ERR("Failed to inquire the current UVC format");
			return err;
		}

		LOG_INF("control: ready to transfer %ux%u frames of %u bytes",
			fmt.width, fmt.height, fmt.height * fmt.pitch);

		if (conf->output_dev != NULL) {
			LOG_DBG("control: setting source format to %ux%u", fmt.width, fmt.height);

			err = video_set_format(conf->output_dev, VIDEO_EP_OUT, &fmt);
			if (err) {
				LOG_ERR("Could not set the format of the video source");
				return err;
			}

			err = video_stream_start(conf->output_dev);
			if (err) {
				LOG_ERR("Could not start the video source");
				return err;
			}
		}

		break;
	default:
		LOG_WRN("commit: invalid bRequest %u", request);
		return -EINVAL;
	}
	return 0;
}

static int uvc_control_streaming(const struct device *dev, const struct usb_setup_packet *setup,
				 struct net_buf *buf)
{
	struct uvc_data *data = dev->data;
	uint8_t control_selector = setup->wValue >> 8;
	struct uvc_probe *probe = (void *)buf->data;

	switch (setup->bRequest) {
	case GET_INFO:
		return uvc_buf_add(buf, 1, INFO_SUPPORTS_GET | INFO_SUPPORTS_SET);
	case GET_LEN:
		return uvc_buf_add(buf, 2, sizeof(probe));
	case GET_DEF:
		if (setup->wLength != sizeof(*probe) || buf->size < sizeof(*probe)) {
			LOG_ERR("control: bad wLength %u or bufsize %u", setup->wLength, buf->size);
			return -EINVAL;
		}
		net_buf_add_mem(buf, &data->default_probe, sizeof(*probe));
		return 0;
	case GET_MIN:
	case GET_MAX:
	case GET_RES:
	case GET_CUR:
		if (setup->wLength != sizeof(*probe) || buf->size < sizeof(*probe)) {
			LOG_ERR("control: bad wLength %u or bufsize %u", setup->wLength, buf->size);
			return -EINVAL;
		}
		net_buf_add(buf, sizeof(*probe));
		break;
	case SET_CUR:
		if (setup->wLength != sizeof(*probe) || buf->len < sizeof(*probe)) {
			LOG_ERR("control: bad wLength %u or buflen %u", setup->wLength, buf->len);
			return -EINVAL;
		}
		break;
	}

	switch (control_selector) {
	case VS_PROBE_CONTROL:
		LOG_DBG("VS_PROBE_CONTROL");
		return uvc_control_probe(dev, setup->bRequest, probe);
	case VS_COMMIT_CONTROL:
		LOG_DBG("VS_COMMIT_CONTROL");
		return uvc_control_commit(dev, setup->bRequest, probe);
	default:
		LOG_WRN("control: unknown selector %u for streaming interface", control_selector);
		return -ENOTSUP;
	}
}

static int uvc_control_default(const struct usb_setup_packet *setup, struct net_buf *buf,
			       uint8_t size)
{
	switch (setup->bRequest) {
	case GET_INFO:
		return uvc_buf_add(buf, 1, INFO_SUPPORTS_GET | INFO_SUPPORTS_SET);
	case GET_LEN:
		return uvc_buf_add(buf, setup->wLength, size);
	case GET_RES:
		return uvc_buf_add(buf, size, 1);
	default:
		LOG_WRN("control: unsupported request type %u", setup->bRequest);
		return -ENOTSUP;
	}
}

static int uvc_control_fix(const struct usb_setup_packet *setup, struct net_buf *buf, uint8_t size,
			   uint32_t value)
{
	LOG_DBG("control: fixed type control, size %u", size);

	switch (setup->bRequest) {
	case GET_DEF:
	case GET_CUR:
	case GET_MIN:
	case GET_MAX:
		return uvc_buf_add(buf, size, value);
	case SET_CUR:
		return 0;
	default:
		return uvc_control_default(setup, buf, size);
	}
}

static int uvc_control_uint(const struct usb_setup_packet *setup, struct net_buf *buf, uint8_t size,
			   const struct device *dev, uint32_t cid)
{
	uint32_t value;
	int err;

	LOG_DBG("control: integer type control, size %u", size);

	switch (setup->bRequest) {
	case GET_DEF:
	case GET_CUR:
	case GET_MIN:
	case GET_MAX:
		err = video_get_ctrl(dev, uvc_get_video_cid(setup, cid), &value);
		if (err) {
			LOG_ERR("control: failed to query target video device");
			return err;
		}
		LOG_DBG("control: value for CID 0x08%x is %u", cid, value);
		return uvc_buf_add(buf, size, value);
	case SET_CUR:
		err = uvc_buf_remove(buf, size, &value);
		if (err) {
			return err;
		}
		LOG_DBG("control: setting CID 0x08%x to %u", cid, value);
		err = video_set_ctrl(dev, cid, (void *)value);
		if (err) {
			LOG_ERR("control: failed to configure target video device");
			return err;
		}
		return 0;
	default:
		return uvc_control_default(setup, buf, size);
	}
}

__unused
static int zephyr_uvc_control_ct(const struct usb_setup_packet *setup, struct net_buf *buf,
				     const struct device *dev)
{
	uint8_t control_selector = setup->wValue >> 8;

	/* See also zephyr,uvc-control-ct.yaml */
	switch (control_selector) {
	case CT_AE_MODE_CONTROL:
		LOG_DBG("CT_AE_MODE_CONTROL -> (none)");
		return uvc_control_fix(setup, buf, 1, BIT(0));
	case CT_AE_PRIORITY_CONTROL:
		LOG_DBG("CT_AE_PRIORITY_CONTROL -> (none)");
		return uvc_control_fix(setup, buf, 1, 0);
	case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
		LOG_DBG("CT_EXPOSURE_TIME_ABSOLUTE_CONTROL -> VIDEO_CID_CAMERA_EXPOSURE");
		return uvc_control_uint(setup, buf, 4, dev, VIDEO_CID_CAMERA_EXPOSURE);
	case CT_ZOOM_ABSOLUTE_CONTROL:
		LOG_DBG("CT_ZOOM_ABSOLUTE_CONTROL -> VIDEO_CID_CAMERA_ZOOM");
		return uvc_control_uint(setup, buf, 2, dev, VIDEO_CID_CAMERA_ZOOM);
	default:
		LOG_WRN("control: unsupported selector %u for camera terminal ", control_selector);
		return -ENOTSUP;
	}
}

__unused
static int zephyr_uvc_control_pu(const struct usb_setup_packet *setup, struct net_buf *buf,
					 const struct device *dev)
{
	uint8_t control_selector = setup->wValue >> 8;

	/* See also zephyr,uvc-control-pu.yaml */
	switch (control_selector) {
	case PU_BRIGHTNESS_CONTROL:
		LOG_DBG("PU_BRIGHTNESS_CONTROL -> VIDEO_CID_CAMERA_BRIGHTNESS");
		return uvc_control_uint(setup, buf, 2, dev, VIDEO_CID_CAMERA_BRIGHTNESS);
	case PU_CONTRAST_CONTROL:
		LOG_DBG("PU_CONTRAST_CONTROL -> VIDEO_CID_CAMERA_CONTRAST");
		return uvc_control_uint(setup, buf, 1, dev, VIDEO_CID_CAMERA_CONTRAST);
	case PU_GAIN_CONTROL:
		LOG_DBG("PU_GAIN_CONTROL -> VIDEO_CID_CAMERA_GAIN");
		return uvc_control_uint(setup, buf, 2, dev, VIDEO_CID_CAMERA_GAIN);
	case PU_SATURATION_CONTROL:
		LOG_DBG("PU_SATURATION_CONTROL -> VIDEO_CID_CAMERA_SATURATION");
		return uvc_control_uint(setup, buf, 2, dev, VIDEO_CID_CAMERA_SATURATION);
	case PU_WHITE_BALANCE_TEMPERATURE_CONTROL:
		LOG_DBG("PU_WHITE_BALANCE_TEMPERATURE_CONTROL -> VIDEO_CID_CAMERA_WHITE_BAL");
		return uvc_control_uint(setup, buf, 2, dev, VIDEO_CID_CAMERA_WHITE_BAL);
	default:
		LOG_WRN("control: unsupported selector %u for processing unit", control_selector);
		return -ENOTSUP;
	}
}

__unused
static int zephyr_uvc_control_xu(const struct usb_setup_packet *setup, struct net_buf *buf,
				     const struct device *dev)
{
	LOG_WRN("control: nothing supported for extension unit");
	return -ENOTSUP;
};

__unused
static int zephyr_uvc_control_it(const struct usb_setup_packet *setup, struct net_buf *buf,
				     const struct device *dev)
{
	LOG_WRN("control: nothing supported for input terminal");
	return -ENOTSUP;
};

static int zephyr_uvc_control_ot(const struct usb_setup_packet *setup, struct net_buf *buf,
				     const struct device *dev)
{
	LOG_WRN("control: nothing supported for output terminal");
	return -ENOTSUP;
};

static int uvc_control_errno(const struct usb_setup_packet *setup, struct net_buf *buf, int err)
{
	switch (setup->bRequest) {
	case GET_INFO:
		return uvc_buf_add(buf, 1, INFO_SUPPORTS_GET);
	case GET_CUR:
		return uvc_buf_add(buf, 1, err);
	default:
		LOG_WRN("control: unsupported request type %u", setup->bRequest);
		return -ENOTSUP;
	}
}

static int uvc_control(const struct device *dev, const struct usb_setup_packet *const setup,
		       struct net_buf *buf)
{
	const struct uvc_conf *conf = dev->config;
	struct uvc_data *data = dev->data;
	const struct uvc_control *controls = conf->controls;
	uint8_t interface = (setup->wIndex >> 0) & 0xff;
	uint8_t entity_id = (setup->wIndex >> 8) & 0xff;
	uint8_t control_selector = (setup->wValue) >> 8;
	int err;

	LOG_DBG("%s", uvc_get_request_str(setup));

	if (interface == *conf->desc_if_vs_ifnum) {
		return uvc_control_streaming(dev, setup, buf);
	}

	if (interface == *conf->desc_if_vc_ifnum) {
		if (entity_id == 0) {
			return uvc_control_errno(setup, buf, data->err);
		}

		for (const struct uvc_control *ctrl = controls; ctrl->entity_id != 0; ctrl++) {
			if (ctrl->entity_id == entity_id) {
				if ((ctrl->mask & BIT(control_selector)) == 0) {
					LOG_WRN("control selector %u not enabled for bEntityID %u",
						control_selector, entity_id);
					data->err = ERR_INVALID_CONTROL;
					return -ENOTSUP;
				}

				/* Control set as supported by the devicetree, call the handler */
				err = ctrl->fn(setup, buf, ctrl->target_dev);
				data->err = uvc_get_errno(-err);
				return err;
			}
		}
		LOG_WRN("control: no unit %u found", entity_id);
		data->err = ERR_INVALID_UNIT;
		return -ENOTSUP;
	}

	LOG_WRN("control: interface %u not found", interface);
	data->err = ERR_INVALID_UNIT;
	return -ENOTSUP;
}

static int uvc_control_to_host(struct usbd_class_data *const c_data,
			       const struct usb_setup_packet *const setup,
			       struct net_buf *const buf)
{
	errno = uvc_control(usbd_class_get_private(c_data), setup, buf);
	return 0;
}

static int uvc_control_to_dev(struct usbd_class_data *const c_data,
			      const struct usb_setup_packet *const setup,
			      const struct net_buf *const buf)
{
	errno = uvc_control(usbd_class_get_private(c_data), setup, (struct net_buf *)buf);
	return 0;
}

static int uvc_request(struct usbd_class_data *const c_data, struct net_buf *buf, int err)
{
	const struct device *dev = usbd_class_get_private(c_data);

	struct uvc_data *data = dev->data;
	struct uvc_buf_info bi = *(struct uvc_buf_info *)udc_get_buf_info(buf);

	net_buf_unref(buf);

	if (bi.udc.ep == uvc_get_bulk_in(dev)) {
		if (bi.vbuf != NULL) {
			/* Upon completion, move the buffer from submission to completion queue */
			LOG_DBG("Request completed, vbuf %p transferred", bi.vbuf);
			k_fifo_put(&data->fifo_out, bi.vbuf);
		}
	} else {
		LOG_WRN("Request on unknown endpoint 0x%02x", bi.udc.ep);
	}

	return 0;
}

static void uvc_update(struct usbd_class_data *const c_data, uint8_t iface, uint8_t alternate)
{
	LOG_DBG("update");
}

static int uvc_init(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;
	int err;

	/* Get the default probe by querying the current probe at startup */
	err = uvc_control_probe(dev, GET_CUR, &data->default_probe);
	if (err) {
		LOG_ERR("init: failed to query the default probe");
		return err;
	}

	return 0;
}

static void uvc_update_desc(const struct device *dev)
{
	const struct uvc_conf *conf = dev->config;

	*conf->desc_iad_ifnum = *conf->desc_if_vc_ifnum;
	*conf->desc_if_vc_header_ifnum = *conf->desc_if_vs_ifnum;
	*conf->desc_if_vs_header_epaddr = uvc_get_bulk_in(dev);
}

static void *uvc_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const struct uvc_conf *conf = dev->config;

	uvc_update_desc(dev);

	switch (speed) {
	case USBD_SPEED_FS:
		return (void *)conf->fs_desc;
	case USBD_SPEED_HS:
		return (void *)conf->hs_desc;
	default:
		__ASSERT_NO_MSG(false);
		return NULL;
	}
}

static int uvc_enqueue_usb(const struct device *dev, struct net_buf *buf, struct video_buffer *vbuf)
{
	const struct uvc_conf *conf = dev->config;
	size_t mps = uvc_get_bulk_mps(conf->c_data);
	struct uvc_buf_info *bi = (void *)udc_get_buf_info(buf);
	size_t len = buf->len;
	int ret;

	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->udc.zlp = (vbuf->flags & VIDEO_BUF_EOF) && buf->len == mps;
	bi->udc.ep = uvc_get_bulk_in(dev);
	/* If this is the last buffer, attach vbuf so we can free it from uvc_request() */
	bi->vbuf = (buf->len <= mps) ? vbuf : NULL;

	/* Apply USB limit */
	buf->len = len = MIN(buf->len, mps);

	LOG_DBG("Queue USB buffer %p, data %p, size %u, len %u",
		buf, buf->data, buf->size, buf->len);

	ret = usbd_ep_enqueue(conf->c_data, buf);
	if (ret < 0) {
		return ret;
	}

	return len;
}

/* Here, the queue of video frame fragments is processed, each
 * fragment is prepended by the UVC header, and the result is cut into
 * USB packets submitted to the hardware:
 * 
 *	[frame: [vbuf: [header+payload, payload, payload, payload...],
 *	         vbuf: [header+payload, payload, payload, payload...], ...],
 *	[frame: [vbuf: [header+payload, payload, payload, payload...],
 *	         vbuf: [header+payload, payload, payload, payload...], ...],
 *	[frame: [vbuf: [header+payload, payload, payload, payload...],
 *	         vbuf: [header+payload, payload, payload, payload...], ...],
 *	...
 */
static int uvc_queue_vbuf(const struct device *dev, struct video_buffer *vbuf)
{
	struct uvc_data *data = dev->data;
	size_t xfer_len = CONFIG_USBD_VIDEO_HEADER_SIZE + vbuf->bytesused;
	struct net_buf *buf;
	int ret;

	if (data->xfer_offset >= xfer_len) {
		data->xfer_offset = 0;
		return 1;
	}

	LOG_DBG("Queue vbuf %p, offset %u/%u, flags 0x%02x",
		vbuf, data->xfer_offset, xfer_len, vbuf->flags);

	/* Add another video frame an USB packet */
	buf = net_buf_alloc_with_data(&uvc_pool, vbuf->header + data->xfer_offset,
				      xfer_len - data->xfer_offset, K_NO_WAIT);
	if (buf == NULL) {
		LOG_ERR("Queue failed: cannot allocate USB buffer");
		return -ENOMEM;
	}

	if (data->xfer_offset == 0) {
		LOG_INF("Queue start of frame, bmHeaderInfo 0x%02x",
			data->payload_header.bmHeaderInfo);

		/* Only the 2 first 8-bit fields supported for now, the rest is padded with 0x00 */
		memcpy(vbuf->header, &data->payload_header, 2);
		memset(vbuf->header + 2, 0, CONFIG_USBD_VIDEO_HEADER_SIZE - 2);

		if (vbuf->flags & VIDEO_BUF_EOF) {
			((struct uvc_payload_header *)vbuf->header)->bmHeaderInfo |=
				UVC_BMHEADERINFO_END_OF_FRAME;
		}

		if (vbuf->flags & VIDEO_BUF_EOF) {
			/* Toggle the Frame ID bit every new frame */
			data->payload_header.bmHeaderInfo ^= UVC_BMHEADERINFO_FRAMEID;
		}
	}

	ret = uvc_enqueue_usb(dev, buf, vbuf);
	if (ret < 0) {
		LOG_ERR("Queue to USB failed");
		data->xfer_offset = 0;
		net_buf_unref(buf);
		return ret;
	}

	data->xfer_offset += ret;
	return 0;
}

static void uvc_worker(struct k_work *work)
{
	struct uvc_data *data = CONTAINER_OF(work, struct uvc_data, work);
	const struct device *dev = data->dev;
	struct video_buffer *vbuf;
	int ret;

	if (!atomic_test_bit(&data->state, UVC_CLASS_ENABLED) ||
	    !atomic_test_bit(&data->state, UVC_CLASS_READY)) {
		LOG_DBG("Queue not ready");
		return;
	}

	/* Only remove the buffer from the queue after it is submitted to USB */
	vbuf = k_fifo_peek_head(&data->fifo_in);
	if (vbuf == NULL) {
		return;
	}

	if (vbuf->buffer - vbuf->header != CONFIG_USBD_VIDEO_HEADER_SIZE) {
		LOG_ERR("Queue expecting header of size %u", CONFIG_USBD_VIDEO_HEADER_SIZE);
		/* TODO: Submit a k_poll event mentioning the error */
		return;
	}

	ret = uvc_queue_vbuf(dev, vbuf);
	if (ret < 0) {
		LOG_ERR("Queue vbuf %p failed", vbuf);
		/* TODO: Submit a k_poll event mentioning the error */
		return;
	}
	if (ret == 1) {
		/* Remove the buffer from the queue now that USB driver received it */
		k_fifo_get(&data->fifo_in, K_NO_WAIT);
	}

	/* Work on the next buffer */
	k_work_submit(&data->work);
}

static void uvc_enable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	/* Catch-up with buffers that might have been delayed */
	atomic_set_bit(&data->state, UVC_CLASS_ENABLED);
	k_work_submit(&data->work);
}

static void uvc_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	atomic_clear_bit(&data->state, UVC_CLASS_ENABLED);
}

struct usbd_class_api uvc_class_api = {
	.enable = uvc_enable,
	.disable = uvc_disable,
	.request = uvc_request,
	.update = uvc_update,
	.control_to_host = uvc_control_to_host,
	.control_to_dev = uvc_control_to_dev,
	.init = uvc_init,
	.get_desc = uvc_get_desc,
};

static int uvc_enqueue(const struct device *dev, enum video_endpoint_id ep,
		       struct video_buffer *vbuf)
{
	struct uvc_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	k_fifo_put(&data->fifo_in, vbuf);
	k_work_submit(&data->work);
	return 0;
}

static int uvc_dequeue(const struct device *dev, enum video_endpoint_id ep,
		       struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct uvc_data *data = dev->data;

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int uvc_get_format(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	const struct uvc_conf *conf = dev->config;
	struct uvc_data *data = dev->data;
	const struct video_format_cap *cap = &conf->caps[data->format_id];
	const struct uvc_format *ufmt = &conf->formats[data->format_id];

	if (ep != VIDEO_EP_IN && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&data->state, UVC_CLASS_ENABLED) ||
	    !atomic_test_bit(&data->state, UVC_CLASS_READY)) {
		return -EAGAIN;
	}

	memset(fmt, 0, sizeof(*fmt));
	fmt->pixelformat = cap->pixelformat;
	fmt->width = cap->width_max;
	fmt->height = cap->height_max;
	fmt->pitch = fmt->width * ufmt->bits_per_pixel / 8;
	return 0;
}

static int uvc_stream_start(const struct device *dev)
{
	/* TODO: resume the stream after it was interrupted if needed */
	return 0;
}

static int uvc_stream_stop(const struct device *dev)
{
	/* TODO: cancel the ongoing USB request and stop the stream */
	return 0;
}

static int uvc_get_caps(const struct device *dev, enum video_endpoint_id ep,
			struct video_caps *caps)
{
	const struct uvc_conf *conf = dev->config;

	caps->format_caps = conf->caps;
	return 0;
}

struct video_driver_api uvc_video_api = {
	.get_format = uvc_get_format,
	.stream_start = uvc_stream_start,
	.stream_stop = uvc_stream_stop,
	.get_caps = uvc_get_caps,
	.enqueue = uvc_enqueue,
	.dequeue = uvc_dequeue,
};

static int uvc_preinit(const struct device *dev)
{
	struct uvc_data *data = dev->data;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_work_init(&data->work, &uvc_worker);
	return 0;
}

#define TARGET_DEV(node) DEVICE_DT_GET_OR_NULL(DT_PHANDLE(node, control_target))
#define OUTPUT_DEV(node) TARGET_DEV(LOOKUP_NODE(node, zephyr_uvc_control_ot))

#define UVC_CAP(frame, format)							\
	{									\
		.pixelformat = video_fourcc(DT_PROP(format, fourcc)[0],		\
					    DT_PROP(format, fourcc)[1],		\
					    DT_PROP(format, fourcc)[2],		\
					    DT_PROP(format, fourcc)[3]),	\
		.width_min = DT_PROP_BY_IDX(frame, size, 0),			\
		.width_max = DT_PROP_BY_IDX(frame, size, 0),			\
		.width_step = 0,						\
		.height_min = DT_PROP_BY_IDX(frame, size, 1),			\
		.height_max = DT_PROP_BY_IDX(frame, size, 1),			\
		.height_step = 0						\
	},

#define UVC_CAPS(format)							\
	IF_NOT_EMPTY(VS_DESCRIPTOR(format), (					\
		DT_FOREACH_CHILD_SEP_VARGS(format, UVC_CAP, (), format)		\
	))

#define UVC_FORMAT(frame, format)						\
	{									\
		.bFormatIndex = VS_FORMAT_ID(format),				\
		.bFrameIndex = NODE_ID(frame),					\
		.frame_interval = 0,						\
		.bits_per_pixel = DT_PROP(format, bits_per_pixel),		\
	},

#define UVC_FORMATS(format)							\
	IF_NOT_EMPTY(VS_DESCRIPTOR(format), (					\
		DT_FOREACH_CHILD_SEP_VARGS(format, UVC_FORMAT, (), format)	\
	))

#define UVC_CONTROL_MASK(node)							\
	DT_STRING_UPPER_TOKEN_BY_IDX(node, compatible, 0)(node, CONTROL)

#define UVC_CONTROL(node)							\
	IF_NOT_EMPTY(VC_DESCRIPTOR(node), ({					\
		.entity_id = NODE_ID(node),					\
		.fn = &DT_STRING_TOKEN_BY_IDX(node, compatible, 0),		\
		.target_dev = TARGET_DEV(node),					\
		.mask = UVC_CONTROL_MASK(node),					\
	},))

#define UVC_DEVICE_DEFINE(node)							\
										\
	UVC_DESCRIPTOR_ARRAYS(node)						\
										\
	static struct usb_desc_header *node##_fs_desc[] = {			\
		UVC_FULLSPEED_DESCRIPTOR_PTRS(node)				\
	};									\
										\
	static struct usb_desc_header *node##_hs_desc[] = {			\
		UVC_HIGHSPEED_DESCRIPTOR_PTRS(node)				\
	};									\
										\
	static const struct video_format_cap node##_caps[] = {			\
		DT_FOREACH_CHILD(node, UVC_CAPS)				\
		{0}								\
	};									\
										\
	static const struct uvc_control node##_controls[] = {			\
		DT_FOREACH_CHILD(node, UVC_CONTROL)				\
		{0}								\
	};									\
										\
	static const struct uvc_format node##_formats[] = {			\
		DT_FOREACH_CHILD(node, UVC_FORMATS)				\
		{0}								\
	};									\
										\
	USBD_DEFINE_CLASS(node##_c_data, &uvc_class_api,			\
			  (void *)DEVICE_DT_GET(node), NULL);			\
										\
	static struct uvc_conf node##_conf = {					\
		.c_data = &node##_c_data,					\
		.caps = node##_caps,						\
		.formats = node##_formats,					\
		.controls = node##_controls,					\
		.fs_desc = node##_fs_desc,					\
		.hs_desc = node##_hs_desc,					\
		.desc_iad_ifnum = node##_desc_iad + 2,				\
		.desc_if_vc_ifnum = node##_desc_if_vc + 2,			\
		.desc_if_vc_header_ifnum = node##_desc_if_vc_header + 12,	\
		.desc_if_vs_ifnum = node##_desc_if_vs + 2,			\
		.desc_if_vs_header_epaddr = node##_desc_if_vs_header + 6,	\
		.fs_desc_ep_epaddr = node##_fs_desc_ep + 2,			\
		.hs_desc_ep_epaddr = node##_hs_desc_ep + 2,			\
		.output_dev = OUTPUT_DEV(node),					\
	};									\
										\
	static struct uvc_data node##_data = {					\
		.dev = DEVICE_DT_GET(node),					\
		.format_id = 0,							\
		.payload_header.bHeaderLength = CONFIG_USBD_VIDEO_HEADER_SIZE,	\
	};									\
										\
	BUILD_ASSERT(DT_ON_BUS(node, usb),					\
		     "node " DT_NODE_PATH(node) " is not"			\
		     " assigned to a USB device controller");			\
										\
	DEVICE_DT_DEFINE(node, uvc_preinit, NULL, &node##_data, &node##_conf,	\
			 POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,		\
			 &uvc_video_api);

DT_FOREACH_STATUS_OKAY(DT_DRV_COMPAT, UVC_DEVICE_DEFINE)
