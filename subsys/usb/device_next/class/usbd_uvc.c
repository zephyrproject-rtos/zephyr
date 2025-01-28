/*
 * Copyright (c) 2024 tinyVision.ai Inc.
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

LOG_MODULE_REGISTER(usbd_uvc, CONFIG_USBD_VIDEO_LOG_LEVEL);

/* Video Class-Specific Request Codes */
#define SET_CUR					0x01
#define GET_CUR					0x81
#define GET_MIN					0x82
#define GET_MAX					0x83
#define GET_RES					0x84
#define GET_LEN					0x85
#define GET_INFO				0x86
#define GET_DEF					0x87

/* Flags announcing which controls are supported */
#define INFO_SUPPORTS_GET			BIT(0)
#define INFO_SUPPORTS_SET			BIT(1)

/* 4.2.1.2 Request Error Code Control */
#define ERR_NOT_READY				0x01
#define ERR_WRONG_STATE				0x02
#define ERR_OUT_OF_RANGE			0x04
#define ERR_INVALID_UNIT			0x05
#define ERR_INVALID_CONTROL			0x06
#define ERR_INVALID_REQUEST			0x07
#define ERR_INVALID_VALUE_WITHIN_RANGE		0x08
#define ERR_UNKNOWN				0xff

/* Video and Still Image Payload Headers */
#define UVC_BMHEADERINFO_FRAMEID		BIT(0)
#define UVC_BMHEADERINFO_END_OF_FRAME		BIT(1)
#define UVC_BMHEADERINFO_HAS_PRESENTATIONTIME	BIT(2)
#define UVC_BMHEADERINFO_HAS_SOURCECLOCK	BIT(3)
#define UVC_BMHEADERINFO_PAYLOAD_SPECIFIC_BIT	BIT(4)
#define UVC_BMHEADERINFO_STILL_IMAGE		BIT(5)
#define UVC_BMHEADERINFO_ERROR			BIT(6)
#define UVC_BMHEADERINFO_END_OF_HEADER		BIT(7)

/* Video Interface Subclass Codes */
#define SC_VIDEOCONTROL				0x01
#define SC_VIDEOSTREAMING			0x02
#define SC_VIDEO_INTERFACE_COLLECTION		0x03

/* Video Class-Specific Video Control Interface Descriptor Subtypes */
#define VC_DESCRIPTOR_UNDEFINED			0x00
#define VC_HEADER				0x01
#define VC_INPUT_TERMINAL			0x02
#define VC_OUTPUT_TERMINAL			0x03
#define VC_SELECTOR_UNIT			0x04
#define VC_PROCESSING_UNIT			0x05
#define VC_EXTENSION_UNIT			0x06
#define VC_ENCODING_UNIT			0x07

/* Video Class-Specific Video Stream Interface Descriptor Subtypes */
#define VS_UNDEFINED				0x00
#define VS_INPUT_HEADER				0x01
#define VS_OUTPUT_HEADER			0x02
#define VS_STILL_IMAGE_FRAME			0x03
#define VS_FORMAT_UNCOMPRESSED			0x04
#define VS_FRAME_UNCOMPRESSED			0x05
#define VS_FORMAT_MJPEG				0x06
#define VS_FRAME_MJPEG				0x07
#define VS_FORMAT_MPEG2TS			0x0A
#define VS_FORMAT_DV				0x0C
#define VS_COLORFORMAT				0x0D
#define VS_FORMAT_FRAME_BASED			0x10
#define VS_FRAME_FRAME_BASED			0x11
#define VS_FORMAT_STREAM_BASED			0x12
#define VS_FORMAT_H264				0x13
#define VS_FRAME_H264				0x14
#define VS_FORMAT_H264_SIMULCAST		0x15
#define VS_FORMAT_VP8				0x16
#define VS_FRAME_VP8				0x17
#define VS_FORMAT_VP8_SIMULCAST			0x18

/* Video Class-Specific Endpoint Descriptor Subtypes */
#define EP_UNDEFINED				0x00
#define EP_GENERAL				0x01
#define EP_ENDPOINT				0x02
#define EP_INTERRUPT				0x03

/* USB Terminal Types */
#define TT_VENDOR_SPECIFIC			0x0100
#define TT_STREAMING				0x0101

/* Input Terminal Types */
#define ITT_VENDOR_SPECIFIC			0x0200
#define ITT_CAMERA				0x0201
#define ITT_MEDIA_TRANSPORT_INPUT		0x0202

/* Output Terminal Types */
#define OTT_VENDOR_SPECIFIC			0x0300
#define OTT_DISPLAY				0x0301
#define OTT_MEDIA_TRANSPORT_OUTPUT		0x0302

/* External Terminal Types */
#define EXT_EXTERNAL_VENDOR_SPECIFIC		0x0400
#define EXT_COMPOSITE_CONNECTOR			0x0401
#define EXT_SVIDEO_CONNECTOR			0x0402
#define EXT_COMPONENT_CONNECTOR			0x0403

/* VideoStreaming Interface Controls */
#define VS_PROBE_CONTROL			0x01
#define VS_COMMIT_CONTROL			0x02
#define VS_STILL_PROBE_CONTROL			0x03
#define VS_STILL_COMMIT_CONTROL			0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL		0x05
#define VS_STREAM_ERROR_CODE_CONTROL		0x06
#define VS_GENERATE_KEY_FRAME_CONTROL		0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL		0x08
#define VS_SYNCH_DELAY_CONTROL			0x09

/* VideoControl Interface Controls */
#define VC_CONTROL_UNDEFINED			0x00
#define VC_VIDEO_POWER_MODE_CONTROL		0x01
#define VC_REQUEST_ERROR_CODE_CONTROL		0x02

/* Selector Unit Controls */
#define SU_INPUT_SELECT_CONTROL			0x01

/* Camera Terminal Controls */
#define CT_SCANNING_MODE_CONTROL		0x01
#define CT_AE_MODE_CONTROL			0x02
#define CT_AE_PRIORITY_CONTROL			0x03
#define CT_EXPOSURE_TIME_ABS_CONTROL		0x04
#define CT_EXPOSURE_TIME_REL_CONTROL		0x05
#define CT_FOCUS_ABS_CONTROL			0x06
#define CT_FOCUS_REL_CONTROL			0x07
#define CT_FOCUS_AUTO_CONTROL			0x08
#define CT_IRIS_ABS_CONTROL			0x09
#define CT_IRIS_REL_CONTROL			0x0A
#define CT_ZOOM_ABS_CONTROL			0x0B
#define CT_ZOOM_REL_CONTROL			0x0C
#define CT_PANTILT_ABS_CONTROL			0x0D
#define CT_PANTILT_REL_CONTROL			0x0E
#define CT_ROLL_ABS_CONTROL			0x0F
#define CT_ROLL_REL_CONTROL			0x10
#define CT_PRIVACY_CONTROL			0x11
#define CT_FOCUS_SIMPLE_CONTROL			0x12
#define CT_WINDOW_CONTROL			0x13
#define CT_REGION_OF_INTEREST_CONTROL		0x14

/* Processing Unit Controls */
#define PU_BACKLIGHT_COMPENSATION_CONTROL	0x01
#define PU_BRIGHTNESS_CONTROL			0x02
#define PU_CONTRAST_CONTROL			0x03
#define PU_GAIN_CONTROL				0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL		0x05
#define PU_HUE_CONTROL				0x06
#define PU_SATURATION_CONTROL			0x07
#define PU_SHARPNESS_CONTROL			0x08
#define PU_GAMMA_CONTROL			0x09
#define PU_WHITE_BALANCE_TEMP_CONTROL		0x0A
#define PU_WHITE_BALANCE_TEMP_AUTO_CONTROL	0x0B
#define PU_WHITE_BALANCE_COMPONENT_CONTROL	0x0C
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL	0x0D
#define PU_DIGITAL_MULTIPLIER_CONTROL		0x0E
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL	0x0F
#define PU_HUE_AUTO_CONTROL			0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL	0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL		0x12
#define PU_CONTRAST_AUTO_CONTROL		0x13

/* Encoding Unit Controls */
#define EU_SELECT_LAYER_CONTROL			0x01
#define EU_PROFILE_TOOLSET_CONTROL		0x02
#define EU_VIDEO_RESOLUTION_CONTROL		0x03
#define EU_MIN_FRAME_INTERVAL_CONTROL		0x04
#define EU_SLICE_MODE_CONTROL			0x05
#define EU_RATE_CONTROL_MODE_CONTROL		0x06
#define EU_AVERAGE_BITRATE_CONTROL		0x07
#define EU_CPB_SIZE_CONTROL			0x08
#define EU_PEAK_BIT_RATE_CONTROL		0x09
#define EU_QUANTIZATION_PARAMS_CONTROL		0x0A
#define EU_SYNC_REF_FRAME_CONTROL		0x0B
#define EU_LTR_BUFFER_CONTROL			0x0C
#define EU_LTR_PICTURE_CONTROL			0x0D
#define EU_LTR_VALIDATION_CONTROL		0x0E
#define EU_LEVEL_IDC_LIMIT_CONTROL		0x0F
#define EU_SEI_PAYLOADTYPE_CONTROL		0x10
#define EU_QP_RANGE_CONTROL			0x11
#define EU_PRIORITY_CONTROL			0x12
#define EU_START_OR_STOP_LAYER_CONTROL		0x13
#define EU_ERROR_RESILIENCY_CONTROL		0x14

/* Extension Unit Controls */
#define XU_BASE_CONTROL				0x00

/* Base GUID string present at the end of most GUID formats, preceded by the FourCC code */
#define UVC_GUID_TAIL "\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71"

enum uvc_class_status {
	CLASS_INITIALIZED,
	CLASS_ENABLED,
	CLASS_READY,
};

struct uvc_control_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdUVC;
	uint16_t wTotalLength;
	uint32_t dwClockFrequency;
	uint8_t bInCollection;
	uint8_t baInterfaceNr[CONFIG_USBD_VIDEO_MAX_STREAMS];
} __packed;

struct uvc_output_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t iTerminal;
} __packed;

struct uvc_camera_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t iTerminal;
	uint16_t wObjectiveFocalLengthMin;
	uint16_t wObjectiveFocalLengthMax;
	uint16_t wOcularFocalLength;
	uint8_t bControlSize;
	uint8_t bmControls[3];
} __packed;

struct uvc_selector_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bNrInPins;
	uint8_t baSourceID[1];
	uint8_t iSelector;
} __packed;

struct uvc_processing_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint16_t wMaxMultiplier;
	uint8_t bControlSize;
	uint8_t bmControls[3];
	uint8_t iProcessing;
	uint8_t bmVideoStandards;
} __packed;

struct uvc_encoding_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t iEncoding;
	uint8_t bControlSize;
	uint8_t bmControls[3];
	uint8_t bmControlsRuntime[3];
} __packed;

struct uvc_extension_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t guidExtensionCode[16];
	uint8_t bNumControls;
	uint8_t bNrInPins;
	uint8_t baSourceID[1];
	uint8_t bControlSize;
	uint8_t bmControls[4];
	uint8_t iExtension;
} __packed;

struct uvc_stream_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bNumFormats;
	uint16_t wTotalLength;
	uint8_t bEndpointAddress;
	uint8_t bmInfo;
	uint8_t bTerminalLink;
	uint8_t bStillCaptureMethod;
	uint8_t bTriggerSupport;
	uint8_t bTriggerUsage;
	uint8_t bControlSize;
} __packed;

struct uvc_frame_still_image_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bEndpointAddress;
	uint8_t bNumImageSizePatterns;
	struct {
		uint16_t wWidth;
		uint16_t wHeight;
	} n[1] __packed;
	uint8_t bNumCompressionPattern;
	uint8_t bCompression[1];
} __packed;

struct uvc_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
} __packed;

struct uvc_format_uncomp_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t guidFormat[16];
	uint8_t bBitsPerPixel;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
} __packed;

struct uvc_format_mjpeg_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t bmFlags;
#define UVC_MJPEG_FLAGS_FIXEDSIZESAMPLES (1 << 0)
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
} __packed;

struct uvc_frame_continuous_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwMinFrameInterval;
	uint32_t dwMaxFrameInterval;
	uint32_t dwFrameIntervalStep;
} __packed;

struct uvc_frame_discrete_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwFrameInterval[CONFIG_USBD_VIDEO_MAX_FRMIVAL];
} __packed;

union uvc_stream_descriptor {
	struct uvc_format_descriptor format;
	struct uvc_format_uncomp_descriptor format_uncomp;
	struct uvc_format_mjpeg_descriptor format_mjpeg;
	struct uvc_frame_discrete_descriptor frame_discrete;
	struct uvc_frame_continuous_descriptor frame_continuous;
} __packed;

struct uvc_color_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bColorPrimaries;
	uint8_t bTransferCharacteristics;
	uint8_t bMatrixCoefficients;
} __packed;

struct usbd_uvc_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct uvc_control_header_descriptor if0_header;
	struct usb_desc_header nil_desc;
};

struct usbd_uvc_strm_desc {
	struct uvc_camera_terminal_descriptor if0_ct;
	struct uvc_selector_unit_descriptor if0_su;
	struct uvc_processing_unit_descriptor if0_pu;
	struct uvc_extension_unit_descriptor if0_xu;
	struct uvc_output_terminal_descriptor if0_ot;

	struct usb_if_descriptor ifN;
	struct uvc_stream_header_descriptor ifN_header;
	union uvc_stream_descriptor *ifN_formats;
	size_t ifN_format_num;
	struct usb_ep_descriptor ifN_ep_fs;
	struct usb_ep_descriptor ifN_ep_hs;
	struct uvc_color_descriptor ifN_color;
};

struct uvc_probe_msg {
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
#define UVC_BMFRAMING_INFO_FID BIT(0)
#define UVC_BMFRAMING_INFO_EOF BIT(1)
#define UVC_BMFRAMING_INFO_EOS BIT(2)
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
	struct uvc_probe_msg default_probe;
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
	uint8_t selector;
	uint8_t size;
	uint8_t bit;
	int (*fn)(uint8_t request, struct net_buf *buf, uint8_t size,
		  const struct device *video_dev, unsigned int cid);
	uint32_t param;
};

struct uvc_guid_quirk {
	uint32_t fourcc;
	uint8_t guid[16];
};

static const struct uvc_guid_quirk uvc_guid_quirks[] = {
	{VIDEO_PIX_FMT_YUYV,  "Y" "U" "Y" "2" UVC_GUID_TAIL},
};

typedef int uvc_control_fn_t(struct uvc_stream *strm, uint8_t selector, uint8_t request,
			     struct net_buf *buf);

NET_BUF_POOL_VAR_DEFINE(uvc_pool, DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 16,
			512 * DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 16,
			sizeof(struct uvc_buf_info), NULL);

static void uvc_fourcc_to_guid(uint8_t guid[16], uint32_t fourcc)
{
	for (int i = 0; i < ARRAY_SIZE(uvc_guid_quirks); i++) {
		if (uvc_guid_quirks[i].fourcc == fourcc) {
			memcpy(guid, uvc_guid_quirks[i].guid, 16);
			return;
		}
	}
	fourcc = sys_cpu_to_le32(fourcc);
	memcpy(guid, &fourcc, 4);
	memcpy(guid + 4, UVC_GUID_TAIL, 16 - 4);
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

/**
 * @brief Get the currently selected format and frame descriptors.
 *
 * @param strm The VideoStreaming interface from which search the descriptors
 * @param format_desc Pointer to be set to a format descriptor pointer.
 * @param frame_desc Pointer to be set to a frame descriptor pointer.
 */
static void uvc_get_format_frame_desc(const struct uvc_stream *strm,
				      union uvc_stream_descriptor **format_desc,
				      union uvc_stream_descriptor **frame_desc)
{
	int i;

	*format_desc = NULL;
	for (i = 0; i < strm->desc->ifN_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->ifN_formats[i];

		if (desc->format.bDescriptorSubtype != VS_FORMAT_UNCOMPRESSED &&
		    desc->format.bDescriptorSubtype != VS_FORMAT_MJPEG) {
			continue;
		}

		if (desc->format.bFormatIndex == strm->format_id) {
			*format_desc = &strm->desc->ifN_formats[i];
			break;
		}
	}

	*frame_desc = NULL;
	for (i++; i < strm->desc->ifN_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->ifN_formats[i];

		if (desc->frame_discrete.bDescriptorSubtype != VS_FRAME_UNCOMPRESSED &&
		    desc->frame_discrete.bDescriptorSubtype != VS_FRAME_MJPEG) {
			break;
		}

		if (desc->frame_discrete.bFrameIndex == strm->frame_id) {
			*frame_desc = &strm->desc->ifN_formats[i];
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

static int uvc_buf_remove_int(struct net_buf *buf, uint16_t size, uint32_t *value)
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
		LOG_WRN("Invalid size %u", size);
		return -ENOTSUP;
	}
}

static uint8_t uvc_get_bulk_in(struct uvc_stream *strm)
{
	struct uvc_data *data = strm->dev->data;

	switch (usbd_bus_speed(usbd_class_get_ctx(data->c_data))) {
	case USBD_SPEED_FS:
		return strm->desc->ifN_ep_fs.bEndpointAddress;
	case USBD_SPEED_HS:
		return strm->desc->ifN_ep_hs.bEndpointAddress;
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

static int uvc_control_probe_format_index(struct uvc_stream *strm, uint8_t request,
					  struct uvc_probe_msg *probe)
{
	uint8_t max = 0;

	for (int i = 0; i < strm->desc->ifN_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->ifN_formats[i];

		max += desc->format.bDescriptorSubtype == VS_FORMAT_UNCOMPRESSED ||
		       desc->format.bDescriptorSubtype == VS_FORMAT_MJPEG;
	}

	switch (request) {
	case GET_RES:
	case GET_MIN:
		probe->bFormatIndex = 1;
		break;
	case GET_MAX:
		probe->bFormatIndex = max;
		break;
	case GET_CUR:
		probe->bFormatIndex = strm->format_id;
		break;
	case SET_CUR:
		if (probe->bFormatIndex == 0) {
			return 0;
		}
		if (probe->bFormatIndex > max) {
			LOG_WRN("probe: format index %u not found", probe->bFormatIndex);
			return -EINVAL;
		}
		strm->format_id = probe->bFormatIndex;
		break;
	}
	return 0;
}

static int uvc_control_probe_frame_index(struct uvc_stream *strm, uint8_t request,
					 struct uvc_probe_msg *probe)
{
	uint8_t max = 0;
	int i;

	/* Search the current format */
	for (i = 0; i < strm->desc->ifN_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->ifN_formats[i];

		if (desc->format.bDescriptorSubtype != VS_FORMAT_UNCOMPRESSED &&
		    desc->format.bDescriptorSubtype != VS_FORMAT_MJPEG) {
			continue;
		}
		if (desc->format.bFormatIndex == strm->format_id) {
			break;
		}
	}

	/* Seek until the next format */
	for (i += 1; i < strm->desc->ifN_format_num; i++) {
		union uvc_stream_descriptor *desc = &strm->desc->ifN_formats[i];

		if (desc->frame_discrete.bDescriptorSubtype != VS_FRAME_UNCOMPRESSED &&
		    desc->frame_discrete.bDescriptorSubtype != VS_FRAME_MJPEG) {
			break;
		}
		max++;
	}

	switch (request) {
	case GET_RES:
	case GET_MIN:
		probe->bFrameIndex = 1;
		break;
	case GET_MAX:
		probe->bFrameIndex = max;
		break;
	case GET_CUR:
		probe->bFrameIndex = strm->frame_id;
		break;
	case SET_CUR:
		if (probe->bFrameIndex == 0) {
			return 0;
		}
		if (probe->bFrameIndex > max) {
			LOG_WRN("probe: frame index %u not found", probe->bFrameIndex);
			return -EINVAL;
		}
		strm->frame_id = probe->bFrameIndex;
		break;
	}
	return 0;
}

static int uvc_control_probe_frame_interval(struct uvc_stream *strm, uint8_t request,
					    struct uvc_probe_msg *probe)
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
	case GET_MIN:
		probe->dwFrameInterval =
			sys_cpu_to_le32(frame_desc->frame_discrete.dwFrameInterval[0]);
		break;
	case GET_MAX:
		max = frame_desc->frame_discrete.bFrameIntervalType - 1;
		probe->dwFrameInterval =
			sys_cpu_to_le32(frame_desc->frame_discrete.dwFrameInterval[max]);
		break;
	case GET_RES:
		probe->dwFrameInterval = sys_cpu_to_le32(1);
		break;
	case GET_CUR:
		probe->dwFrameInterval = sys_cpu_to_le32(strm->video_frmival.numerator);
		break;
	case SET_CUR:
		strm->video_frmival.numerator = sys_le32_to_cpu(probe->dwFrameInterval);
		break;
	}
	return 0;
}

static int uvc_control_probe_max_size(struct uvc_stream *strm, uint8_t request,
					      struct uvc_probe_msg *probe)
{
	struct video_format *fmt = &strm->video_fmt;
	uint32_t max_size = MAX(fmt->pitch, fmt->width) * fmt->height;

	switch (request) {
	case GET_MIN:
	case GET_MAX:
	case GET_CUR:
		probe->dwMaxPayloadTransferSize = sys_cpu_to_le32(max_size);
		probe->dwMaxVideoFrameSize = sys_cpu_to_le32(max_size);
		break;
	case GET_RES:
		probe->dwMaxPayloadTransferSize = sys_cpu_to_le32(1);
		probe->dwMaxVideoFrameSize = sys_cpu_to_le32(1);
		break;
	case SET_CUR:
		if (sys_le32_to_cpu(probe->dwMaxPayloadTransferSize) > 0 &&
		    sys_le32_to_cpu(probe->dwMaxPayloadTransferSize) != max_size) {
			LOG_WRN("probe: dwPayloadTransferSize is read-only");
		}
		if (sys_le32_to_cpu(probe->dwMaxVideoFrameSize) > 0 &&
		    sys_le32_to_cpu(probe->dwMaxVideoFrameSize) != max_size) {
			LOG_WRN("probe: dwMaxVideoFrameSize is read-only");
		}
		break;
	}
	return 0;
}

static int uvc_control_probe(struct uvc_stream *strm, uint8_t request, struct uvc_probe_msg *probe)
{
	union uvc_stream_descriptor *format_desc = NULL;
	union uvc_stream_descriptor *frame_desc = NULL;
	struct video_format *fmt = &strm->video_fmt;
	int ret;

	if (request != GET_MIN && request != GET_MAX && request != GET_RES && request != GET_CUR &&
	    request != SET_CUR) {
		LOG_WRN("Invalid bRequest %u", request);
		return -EINVAL;
	}

	ret = uvc_control_probe_format_index(strm, request, probe);
	if (ret < 0) {
		return ret;
	}

	ret = uvc_control_probe_frame_index(strm, request, probe);
	if (ret < 0) {
		return ret;
	}

	/* Update the format based on the probe message from the host */
	uvc_get_format_frame_desc(strm, &format_desc, &frame_desc);
	if (format_desc == NULL || frame_desc == NULL) {
		LOG_ERR("Called probe with invalid format ID (%u) and frame ID (%u)",
			strm->format_id, strm->frame_id);
		return -EINVAL;
	}

	/* Translate between UVC pixel formats and Video pixel formats */
	if (format_desc->format_mjpeg.bDescriptorSubtype == VS_FORMAT_MJPEG) {
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
	fmt->pitch = fmt->width * video_pix_fmt_bpp(fmt->pixelformat);

	ret = uvc_control_probe_frame_interval(strm, request, probe);
	if (ret < 0) {
		return ret;
	}

	ret = uvc_control_probe_max_size(strm, request, probe);
	if (ret < 0) {
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
	/* TODO configure from devicetree */
	probe->wDelay = sys_cpu_to_le16(1);

	/* Show extended debugging as hosts typically do not show these messages,
	 * so it helps that it gets printed there to understand the host behavior.
	 */
	LOG_DBG("- bmHint %u", sys_le16_to_cpu(probe->bmHint));
	LOG_DBG("- bFormatIndex %u", probe->bFormatIndex);
	LOG_DBG("- bFrameIndex %u", probe->bFrameIndex);
	LOG_DBG("- dwFrameInterval %u (us)", sys_le32_to_cpu(probe->dwFrameInterval) / 10);
	LOG_DBG("- wKeyFrameRate %u", sys_le16_to_cpu(probe->wKeyFrameRate));
	LOG_DBG("- wPFrameRate %u", sys_le16_to_cpu(probe->wPFrameRate));
	LOG_DBG("- wCompQuality %u", sys_le16_to_cpu(probe->wCompQuality));
	LOG_DBG("- wCompWindowSize %u", sys_le16_to_cpu(probe->wCompWindowSize));
	LOG_DBG("- wDelay %u (ms)", sys_le16_to_cpu(probe->wDelay));
	LOG_DBG("- dwMaxVideoFrameSize %u", sys_le32_to_cpu(probe->dwMaxVideoFrameSize));
	LOG_DBG("- dwMaxPayloadTransferSize %u", sys_le32_to_cpu(probe->dwMaxPayloadTransferSize));
	LOG_DBG("- dwClockFrequency %u (Hz)", sys_le32_to_cpu(probe->dwClockFrequency));
	LOG_DBG("- bmFramingInfo %u", probe->bmFramingInfo);
	LOG_DBG("- bPreferedVersion %u", probe->bPreferedVersion);
	LOG_DBG("- bMinVersion %u", probe->bMinVersion);
	LOG_DBG("- bMaxVersion %u", probe->bMaxVersion);
	LOG_DBG("- bUsage %u", probe->bUsage);
	LOG_DBG("- bBitDepthLuma %u", probe->bBitDepthLuma + 8);
	LOG_DBG("- bmSettings %u", probe->bmSettings);
	LOG_DBG("- bMaxNumberOfRefFramesPlus1 %u", probe->bMaxNumberOfRefFramesPlus1);
	LOG_DBG("- bmRateControlModes %u", probe->bmRateControlModes);
	LOG_DBG("- bmLayoutPerStream %llu", probe->bmLayoutPerStream);

	return 0;
}

static int uvc_control_commit(struct uvc_stream *strm, uint8_t request, struct uvc_probe_msg *probe)
{
	struct video_format *fmt = &strm->video_fmt;
	int ret;

	switch (request) {
	case GET_CUR:
		return uvc_control_probe(strm, request, probe);
	case SET_CUR:
		ret = uvc_control_probe(strm, request, probe);
		if (ret < 0) {
			return ret;
		}

		atomic_set_bit(&strm->state, CLASS_READY);
		k_work_submit(&strm->work);

		LOG_INF("Ready to transfer, setting source format to %c%c%c%c %ux%u",
			fmt->pixelformat >> 0 & 0xff, fmt->pixelformat >> 8 & 0xff,
			fmt->pixelformat >> 16 & 0xff, fmt->pixelformat >> 24 & 0xff,
			fmt->width, fmt->height);

		ret = video_set_format(strm->video_dev, VIDEO_EP_OUT, fmt);
		if (ret < 0) {
			LOG_ERR("Could not set the format of %s", strm->video_dev->name);
			return ret;
		}

		ret = video_set_frmival(strm->video_dev, VIDEO_EP_OUT, &strm->video_frmival);
		if (ret < 0) {
			LOG_WRN("Could not set the framerate of %s", strm->video_dev->name);
		}

		ret = video_stream_start(strm->video_dev);
		if (ret < 0) {
			LOG_ERR("Could not start %s", strm->video_dev->name);
			return ret;
		}

		break;
	default:
		LOG_WRN("commit: invalid bRequest %u", request);
		return -EINVAL;
	}
	return 0;
}

static int uvc_control_vs(struct uvc_stream *strm, const struct usb_setup_packet *setup,
			  struct net_buf *buf)
{
	struct uvc_probe_msg *probe = (void *)buf->data;
	uint8_t selector = setup->wValue >> 8;
	uint8_t request = setup->bRequest;
	size_t size = MIN(sizeof(*probe), buf->size);

	switch (request) {
	case GET_INFO:
		return uvc_buf_add_int(buf, 1, INFO_SUPPORTS_GET | INFO_SUPPORTS_SET);
	case GET_LEN:
		return uvc_buf_add_int(buf, 2, sizeof(probe));
	case GET_DEF:
		net_buf_add_mem(buf, &strm->default_probe, size);
		return 0;
	case GET_MIN:
	case GET_MAX:
	case GET_CUR:
		net_buf_add(buf, size);
	}

	switch (selector) {
	case VS_PROBE_CONTROL:
		LOG_DBG("VS_PROBE_CONTROL");
		return uvc_control_probe(strm, setup->bRequest, probe);
	case VS_COMMIT_CONTROL:
		LOG_DBG("VS_COMMIT_CONTROL");
		return uvc_control_commit(strm, setup->bRequest, probe);
	default:
		LOG_WRN("Unknown selector %u for streaming interface", selector);
		return -ENOTSUP;
	}
}

static int uvc_control_default(uint8_t request, struct net_buf *buf, uint8_t size)
{
	switch (request) {
	case GET_INFO:
		return uvc_buf_add_int(buf, 1, INFO_SUPPORTS_GET | INFO_SUPPORTS_SET);
	case GET_LEN:
		return uvc_buf_add_int(buf, buf->size, size);
	case GET_RES:
		return uvc_buf_add_int(buf, size, 1);
	default:
		LOG_WRN("Unsupported request type %u", request);
		return -ENOTSUP;
	}
}

static int uvc_control_fixed(uint8_t request, struct net_buf *buf, uint8_t size,
			     const struct device *dev, uint32_t value)
{
	LOG_DBG("Control type 'fixed', size %u", size);

	switch (request) {
	case GET_DEF:
	case GET_CUR:
	case GET_MIN:
	case GET_MAX:
		return uvc_buf_add_int(buf, size, value);
	case SET_CUR:
		return 0;
	default:
		return uvc_control_default(request, buf, size);
	}
}

static int uvc_control_int(uint8_t request, struct net_buf *buf, uint8_t size,
			   const struct device *video_dev, unsigned int cid)
{
	int value;
	int ret;

	LOG_DBG("type 'integer', size %u", size);

	switch (request) {
	case GET_DEF:
	case GET_MIN:
		return uvc_buf_add_int(buf, size, 0);
	case GET_MAX:
		return uvc_buf_add_int(buf, size, INT16_MAX);
	case GET_CUR:
		ret = video_get_ctrl(video_dev, cid, &value);
		if (ret < 0) {
			LOG_INF("Failed to query '%s'", video_dev->name);
			return ret;
		}

		LOG_DBG("Value for CID 0x08%x is %u", cid, value);

		return uvc_buf_add_int(buf, size, value);
	case SET_CUR:
		ret = uvc_buf_remove_int(buf, size, &value);
		if (ret < 0) {
			return ret;
		}

		LOG_DBG("Setting CID 0x08%x to %u", cid, value);

		ret = video_set_ctrl(video_dev, cid, (void *)value);
		if (ret < 0) {
			LOG_ERR("Failed to configure target video device");
			return ret;
		}

		return 0;
	default:
		return uvc_control_default(request, buf, size);
	}
}

static const struct uvc_control_map uvc_control_map_ct[] = {
	{CT_AE_MODE_CONTROL,		1, 1, uvc_control_fixed, BIT(0)},
	{CT_AE_PRIORITY_CONTROL,	1, 2, uvc_control_fixed, 0},
	{CT_EXPOSURE_TIME_ABS_CONTROL,	4, 3, uvc_control_int, VIDEO_CID_EXPOSURE},
	{CT_ZOOM_ABS_CONTROL,		2, 9, uvc_control_int, VIDEO_CID_ZOOM_ABSOLUTE},
	{0},
};

static const struct uvc_control_map uvc_control_map_pu[] = {
	{PU_BRIGHTNESS_CONTROL,		2, 0, uvc_control_int, VIDEO_CID_BRIGHTNESS},
	{PU_CONTRAST_CONTROL,		1, 1, uvc_control_int, VIDEO_CID_CONTRAST},
	{PU_GAIN_CONTROL,		2, 9, uvc_control_int, VIDEO_CID_GAIN},
	{PU_SATURATION_CONTROL,		2, 3, uvc_control_int, VIDEO_CID_SATURATION},
	{PU_WHITE_BALANCE_TEMP_CONTROL,	2, 6, uvc_control_int, VIDEO_CID_WHITE_BALANCE_TEMPERATURE},
	{0},
};

static const struct uvc_control_map uvc_control_map_su[] = {
	{SU_INPUT_SELECT_CONTROL,	1, 0, uvc_control_int, VIDEO_CID_TEST_PATTERN},
	{0},
};

static const struct uvc_control_map uvc_control_map_xu[] = {
	{XU_BASE_CONTROL + 0,		4, 0, uvc_control_int, VIDEO_CID_PRIVATE_BASE + 0},
	{XU_BASE_CONTROL + 1,		4, 1, uvc_control_int, VIDEO_CID_PRIVATE_BASE + 1},
	{XU_BASE_CONTROL + 2,		4, 2, uvc_control_int, VIDEO_CID_PRIVATE_BASE + 2},
	{XU_BASE_CONTROL + 3,		4, 3, uvc_control_int, VIDEO_CID_PRIVATE_BASE + 3},
	{0},
};

static int uvc_control_call(const struct device *dev, struct uvc_stream *strm,
			    const struct usb_setup_packet *const setup, struct net_buf *buf,
			    const struct uvc_control_map *map)
{
	struct uvc_data *data = dev->data;
	uint8_t selector = setup->wValue >> 8;
	uint8_t request = setup->bRequest;
	int ret = -ENOTSUP;

	for (int i = 0; map[i].size != 0; i++) {
		if (map[i].selector == selector) {
			ret = map[i].fn(request, buf, map[i].size, strm->video_dev, map[i].param);
			break;
		}
	}

	switch (ret) {
	case 0:
		data->err = 0;
		break;
	case -EBUSY:
	case -EAGAIN:
	case -EINPROGRESS:
	case -EALREADY:
		data->err = ERR_NOT_READY;
		break;
	case -EOVERFLOW:
	case -ERANGE:
	case -E2BIG:
		data->err = ERR_OUT_OF_RANGE;
		break;
	case -EDOM:
	case -EINVAL:
		data->err = ERR_INVALID_VALUE_WITHIN_RANGE;
		break;
	case -ENODEV:
	case -ENOTSUP:
	case -ENOSYS:
		data->err = ERR_INVALID_REQUEST;
		break;
	default:
		data->err = ERR_UNKNOWN;
		break;
	}
	return ret;
}

static int uvc_control_errno(uint8_t request, struct net_buf *buf, int err)
{
	switch (request) {
	case GET_INFO:
		return uvc_buf_add_int(buf, 1, INFO_SUPPORTS_GET);
	case GET_CUR:
		return uvc_buf_add_int(buf, 1, err);
	default:
		LOG_WRN("Unsupported request type %u", request);
		return -ENOTSUP;
	}
}

static int uvc_control(const struct device *dev, const struct usb_setup_packet *const setup,
		       struct net_buf *buf)
{
	struct uvc_data *data = dev->data;
	uint8_t ifnum = (setup->wIndex >> 0) & 0xff;
	uint8_t unit_id = (setup->wIndex >> 8) & 0xff;
	uint8_t request = setup->bRequest;

	LOG_DBG("Host sent a %s request",
		request == SET_CUR ? "SET_CUR" : request == GET_CUR ? "GET_CUR" :
		request == GET_MIN ? "GET_MIN" : request == GET_MAX ? "GET_MAX" :
		request == GET_RES ? "GET_RES" : request == GET_LEN ? "GET_LEN" :
		request == GET_DEF ? "GET_DEF" : request == GET_INFO ? "GET_INFO" : "bad");

	if (setup->wLength > buf->size) {
		LOG_ERR("wLength %u larger than %u bytes", setup->wLength, buf->size);
		return -ENOMEM;
	}
	buf->size = setup->wLength;

	/* VideoStreaming requests */

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		if (ifnum == strm->desc->ifN.bInterfaceNumber) {
			return uvc_control_vs(strm, setup, buf);
		}
	}

	/* VideoControl requests */

	if (ifnum != data->desc->if0.bInterfaceNumber) {
		LOG_WRN("Interface %u not found", ifnum);
		data->err = ERR_INVALID_UNIT;
		return -ENOTSUP;
	}

	if (unit_id == 0) {
		return uvc_control_errno(request, buf, data->err);
	}

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		if (unit_id == strm->desc->if0_ct.bTerminalID) {
			return uvc_control_call(dev, strm, setup, buf, uvc_control_map_ct);
		} else if (unit_id == strm->desc->if0_su.bUnitID) {
			return uvc_control_call(dev, strm, setup, buf, uvc_control_map_su);
		} else if (unit_id == strm->desc->if0_pu.bUnitID) {
			return uvc_control_call(dev, strm, setup, buf, uvc_control_map_pu);
		} else if (unit_id == strm->desc->if0_xu.bUnitID) {
			return uvc_control_call(dev, strm, setup, buf, uvc_control_map_xu);
		}
	}

	LOG_WRN("Could not handle command for bUnitID %u", unit_id);

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

static void uvc_update(struct usbd_class_data *const c_data, uint8_t iface, uint8_t alternate)
{
	LOG_DBG("update");
}

static int uvc_init_format_desc(struct uvc_stream *strm, int *id,
			       union uvc_stream_descriptor **format_desc,
			       const struct video_format_cap *cap)
{
	union uvc_stream_descriptor *desc = &strm->desc->ifN_formats[*id];
	uint32_t pixfmt = cap->pixelformat;

	if (*id >= CONFIG_USBD_VIDEO_MAX_VS_DESC) {
		LOG_ERR("Out of descriptors, consider increasing CONFIG_USBD_VIDEO_MAX_VS_DESC");
		return -ENOMEM;
	}

	/* Increment the position in the list of formats/frame descriptors */
	(*id)++;

	LOG_DBG("Adding format descriptor #%u for %c%c%c%c",
		strm->desc->ifN_header.bNumFormats + 1,
		pixfmt >> 0 & 0xff, pixfmt >> 8 & 0xff, pixfmt >> 16 & 0xff, pixfmt >> 24 & 0xff);

	memset(desc, 0, sizeof(*desc));

	desc->format.bDescriptorType = USB_DESC_CS_INTERFACE;
	desc->format.bFormatIndex = ++strm->desc->ifN_header.bNumFormats;

	switch (pixfmt) {
	case VIDEO_PIX_FMT_JPEG:
		desc->format_mjpeg.bLength = sizeof(desc->format_mjpeg);
		desc->format_mjpeg.bDescriptorSubtype = VS_FORMAT_MJPEG;
		desc->format_mjpeg.bDefaultFrameIndex = 1;
		break;
	default:
		uvc_fourcc_to_guid(desc->format_uncomp.guidFormat, pixfmt);
		desc->format_uncomp.bLength = sizeof(desc->format_uncomp);
		desc->format_uncomp.bDescriptorSubtype = VS_FORMAT_UNCOMPRESSED;
		desc->format_uncomp.bBitsPerPixel = video_pix_fmt_bpp(cap->pixelformat) * 8;
		desc->format_uncomp.bDefaultFrameIndex = 1;
	}

	strm->desc->ifN_header.wTotalLength =
		sys_cpu_to_le16(sys_le16_to_cpu(strm->desc->ifN_header.wTotalLength) +
				desc->frame_discrete.bLength);

	*format_desc = desc;
	return 0;
}

static int uvc_compare_frmival_desc(const void *a, const void *b)
{
	return (*(uint32_t *)a) - (*(uint32_t *)b);
}

static int uvc_init_frame_desc(struct uvc_stream *strm, int *id,
			      union uvc_stream_descriptor *format_desc,
			      const struct video_format_cap *cap, bool min)
{
	uint16_t w = min ? cap->width_min : cap->width_max;
	uint16_t h = min ? cap->height_min : cap->height_max;
	uint16_t p = MAX(video_pix_fmt_bpp(cap->pixelformat), 1) * w;
	uint32_t min_bitrate = UINT32_MAX;
	uint32_t max_bitrate = 0;
	struct video_format fmt = {.pixelformat = cap->pixelformat,
				   .width = w, .height = h, .pitch = p};
	struct video_frmival_enum fie = {.format = &fmt};
	union uvc_stream_descriptor *desc = &strm->desc->ifN_formats[*id];
	uint32_t max_size = MAX(p, w) * h;

	LOG_DBG("Adding frame descriptor #%u for %ux%u",
		format_desc->format.bNumFrameDescriptors + 1, (unsigned int)w, (unsigned int)h);

	if (*id >= CONFIG_USBD_VIDEO_MAX_VS_DESC) {
		LOG_ERR("Out of descriptors, consider increasing CONFIG_USBD_VIDEO_MAX_VS_DESC");
		return -ENOMEM;
	}

	/* Increment the position in the list of formats/frame descriptors */
	(*id)++;

	memset(desc, 0, sizeof(*desc));

	desc->frame_discrete.bLength = sizeof(desc->frame_discrete);
	desc->frame_discrete.bLength -= CONFIG_USBD_VIDEO_MAX_FRMIVAL * sizeof(uint32_t);
	desc->frame_discrete.bDescriptorType = USB_DESC_CS_INTERFACE;
	desc->frame_discrete.bFrameIndex = ++format_desc->format.bNumFrameDescriptors;
	desc->frame_discrete.wWidth = sys_cpu_to_le16(w);
	desc->frame_discrete.wHeight = sys_cpu_to_le16(h);
	desc->frame_discrete.dwMaxVideoFrameBufferSize = sys_cpu_to_le32(max_size);
	desc->frame_discrete.bDescriptorSubtype =
		(format_desc->frame_discrete.bDescriptorSubtype == VS_FORMAT_UNCOMPRESSED)
			? VS_FRAME_UNCOMPRESSED
			: VS_FRAME_MJPEG;

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

	strm->desc->ifN_header.wTotalLength =
		sys_cpu_to_le16(sys_le16_to_cpu(strm->desc->ifN_header.wTotalLength) +
				desc->frame_discrete.bLength);

	return 0;
}

static uint32_t uvc_get_mask(const struct device *video_dev, const struct uvc_control_map *map)
{
	uint32_t mask = 0;
	bool ok;
	int value;

	LOG_DBG("Testing all supported controls to see which are supported");
	for (int i = 0; map[i].size != 0; i++) {
		ok = (map[i].fn == uvc_control_fixed) ||
		     (video_get_ctrl(video_dev, map[i].param, &value) == 0);
		mask |= ok << map[i].bit;
		LOG_DBG("video device %s support for control 0x%02x: %s",
			video_dev->name, map[i].selector, ok ? "yes" : "no");
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
	data->desc->if0_header.wTotalLength += strm->desc->if0_ct.bLength;
	data->desc->if0_header.wTotalLength += strm->desc->if0_pu.bLength;
	data->desc->if0_header.wTotalLength += strm->desc->if0_su.bLength;
	data->desc->if0_header.wTotalLength += strm->desc->if0_xu.bLength;
	data->desc->if0_header.wTotalLength += strm->desc->if0_ot.bLength;

	/* Probe each video device for their controls support within each unit */
	mask = uvc_get_mask(strm->video_dev, uvc_control_map_xu);
	strm->desc->if0_xu.bmControls[0] = mask >> 0 & 0xff;
	strm->desc->if0_xu.bmControls[1] = mask >> 8 & 0xff;
	strm->desc->if0_xu.bmControls[2] = mask >> 16 & 0xff;
	mask = uvc_get_mask(strm->video_dev, uvc_control_map_ct);
	strm->desc->if0_ct.bmControls[0] = mask >> 0 & 0xff;
	strm->desc->if0_ct.bmControls[1] = mask >> 8 & 0xff;
	strm->desc->if0_ct.bmControls[2] = mask >> 16 & 0xff;
	mask = uvc_get_mask(strm->video_dev, uvc_control_map_pu);
	strm->desc->if0_pu.bmControls[0] = mask >> 0 & 0xff;
	strm->desc->if0_pu.bmControls[1] = mask >> 8 & 0xff;
	strm->desc->if0_pu.bmControls[2] = mask >> 16 & 0xff;
	mask = uvc_get_mask(strm->video_dev, uvc_control_map_su);

	/* Probe the video device for their format support */
	ret = video_get_caps(strm->video_dev, VIDEO_EP_OUT, &caps);
	if (ret < 0) {
		LOG_DBG("Could not load %s video format list", strm->video_dev->name);
		return ret;
	}

	/* Generate the list of format descrptors according to the video capabilities */
	for (int i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		const struct video_format_cap *cap = &caps.format_caps[i];

		/* Convert the flat capability structure into an UVC nested structure */
		if (prev_pixfmt != cap->pixelformat) {
			ret = uvc_init_format_desc(strm, &id, &format_desc, cap);
			if (ret < 0) {
				return ret;
			}
		}

		/* Always add the minimum value */
		ret = uvc_init_frame_desc(strm, &id, format_desc, cap, true);
		if (ret < 0) {
			return ret;
		}

		/* If min and max differs, also add the max */
		if (cap->width_min != cap->width_max || cap->height_min != cap->height_max) {
			ret = uvc_init_frame_desc(strm, &id, format_desc, cap, true);
			if (ret < 0) {
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
			LOG_DBG("Skipping %u (%p), bDescriptorType %u",
				src, desc_ptrs[src], desc_ptrs[src]->bDescriptorType);
			src++;
		} else {
			LOG_DBG("Copying %u to %u (%p), bDescriptorType %u",
				src, dst, desc_ptrs[dst], desc_ptrs[src]->bDescriptorType);
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

	if (atomic_test_and_set_bit(&data->state, CLASS_INITIALIZED)) {
		LOG_DBG("Descriptors already initialized");
		return 0;
	}

	LOG_DBG("Initializing UVC class %p (%s)", dev, dev->name);

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		ret = uvc_init_desc(dev, strm);
		if (ret < 0) {
			return ret;
		}
	}

	/* Skip the empty gaps in the list of pointers */
	uvc_init_desc_ptrs(dev, data->fs_desc);
	uvc_init_desc_ptrs(dev, data->hs_desc);

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		/* Now that descriptors are ready, store the default probe */
		ret = uvc_control_probe(strm, GET_CUR, &strm->default_probe);
		if (ret < 0) {
			LOG_ERR("init: failed to query the default probe");
			return ret;
		}
	}

	return 0;
}

static int uvc_init(struct usbd_class_data *const c_data);

static void uvc_update_desc(const struct device *dev, struct usbd_class_data *const c_data)
{
	struct uvc_data *data = dev->data;

	uvc_init(c_data);
	data->desc->iad.bFirstInterface = data->desc->if0.bInterfaceNumber;

	for (int i = 0; data->streams[i].dev != NULL; i++) {
		struct uvc_stream *strm = &data->streams[i];

		strm->desc->ifN_header.bEndpointAddress = uvc_get_bulk_in(strm);
		data->desc->if0_header.baInterfaceNr[i] = strm->desc->ifN.bInterfaceNumber;
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
	if (ret < 0) {
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

	if (!atomic_test_bit(&strm->state, CLASS_ENABLED) ||
	    !atomic_test_bit(&strm->state, CLASS_READY)) {
		LOG_DBG("UVC not ready yet");
		return;
	}

	while ((vbuf = k_fifo_peek_head(&strm->fifo_in)) != NULL) {
		ret = uvc_queue_vbuf(strm, vbuf);
		if (ret < 0) {
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
		atomic_set_bit(&strm->state, CLASS_ENABLED);
		k_work_submit(&strm->work);
	}
}

static void uvc_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct uvc_data *data = dev->data;

	for (struct uvc_stream *strm = data->streams; strm->dev != NULL; strm++) {
		atomic_clear_bit(&strm->state, CLASS_ENABLED);
	}
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

static int uvc_enqueue(const struct device *dev, enum video_endpoint_id ep,
		       struct video_buffer *vbuf)
{
	struct uvc_stream *strm;
	int ret;

	ret = uvc_get_stream(dev, ep, &strm);
	if (ret < 0) {
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
	if (ret < 0) {
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
	if (ret < 0) {
		return ret;
	}

	if (!atomic_test_bit(&strm->state, CLASS_ENABLED) ||
	    !atomic_test_bit(&strm->state, CLASS_READY)) {
		return -EAGAIN;
	}

	return video_get_format(strm->video_dev, VIDEO_EP_OUT, fmt);
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
	struct uvc_stream *strm;
	int ret;

	ret = uvc_get_stream(dev, ep, &strm);
	if (ret < 0) {
		return ret;
	}

	return video_get_caps(strm->video_dev, VIDEO_EP_OUT, caps);
}

static DEVICE_API(video, uvc_video_api) = {
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
	.if0_ct = {								\
		.bLength = sizeof(struct uvc_camera_terminal_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = VC_INPUT_TERMINAL,			\
		.bTerminalID = UVC_UNIT_ID_CT_##n,				\
		.wTerminalType = sys_cpu_to_le16(ITT_CAMERA),			\
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
		.bDescriptorSubtype = VC_SELECTOR_UNIT,				\
		.bUnitID = UVC_UNIT_ID_SU_##n,					\
		.bNrInPins = 1,							\
		.baSourceID = {UVC_UNIT_ID_CT_##n},				\
		.iSelector = 0,							\
	},									\
										\
	.if0_pu = {								\
		.bLength = sizeof(struct uvc_processing_unit_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = VC_PROCESSING_UNIT,			\
		.bUnitID = UVC_UNIT_ID_PU_##n,					\
		.bSourceID = UVC_UNIT_ID_SU_##n,				\
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
		.bDescriptorSubtype = VC_EXTENSION_UNIT,			\
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
	.if0_ot = {								\
		.bLength = sizeof(struct uvc_output_terminal_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = VC_OUTPUT_TERMINAL,			\
		.bTerminalID = UVC_UNIT_ID_OT_##n,				\
		.wTerminalType = sys_cpu_to_le16(TT_STREAMING),			\
		.bAssocTerminal = 0,						\
		.bSourceID = UVC_UNIT_ID_XU_##n,				\
		.iTerminal = 0,							\
	},									\
										\
	.ifN = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 1,						\
		.bInterfaceClass = USB_BCC_VIDEO,				\
		.bInterfaceSubClass = SC_VIDEOSTREAMING,			\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.ifN_header = {								\
		.bLength = sizeof(struct uvc_stream_header_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = VS_INPUT_HEADER,				\
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
	.ifN_formats = uvc_fmt_frm_desc_##n,					\
	.ifN_format_num = ARRAY_SIZE(uvc_fmt_frm_desc_##n),			\
										\
	.ifN_ep_fs = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64),				\
		.bInterval = 0,							\
	},									\
										\
	.ifN_ep_hs = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0,							\
	},									\
										\
	.ifN_color = {								\
		.bLength = sizeof(struct uvc_color_descriptor),			\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = VS_COLORFORMAT,				\
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
		.bFunctionSubClass = SC_VIDEO_INTERFACE_COLLECTION,		\
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
		.bInterfaceSubClass = SC_VIDEOCONTROL,				\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if0_header = {								\
		.bLength = 12 + DT_CHILD_NUM(DT_INST_CHILD(n, port)),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = VC_HEADER,				\
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
	(struct usb_desc_header *)&uvc_desc_##n.if0,				\
	(struct usb_desc_header *)&uvc_desc_##n.if0_header,			\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_CONTROL_PTRS)				\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_STREAMING_PTRS_FS)			\
	(struct usb_desc_header *)&uvc_desc_##n.nil_desc,			\
};										\
										\
static struct usb_desc_header *uvc_hs_desc_##n[] = {				\
	(struct usb_desc_header *)&uvc_desc_##n.iad,				\
	(struct usb_desc_header *)&uvc_desc_##n.if0,				\
	(struct usb_desc_header *)&uvc_desc_##n.if0_header,			\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_CONTROL_PTRS)				\
	UVC_FOREACH_STREAM(n, UVC_VIDEO_STREAMING_PTRS_HS)			\
	(struct usb_desc_header *)&uvc_desc_##n.nil_desc,			\
};

#define UVC_VIDEO_CONTROL_PTRS(n)						\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if0_ct,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if0_su,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if0_pu,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if0_xu,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.if0_ot,
#define UVC_VIDEO_STREAMING_PTRS(i, n)						\
	(struct usb_desc_header *)&uvc_fmt_frm_desc_##n[i],
#define UVC_VIDEO_STREAMING_PTRS_FS(n)						\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN_header,		\
	LISTIFY(CONFIG_USBD_VIDEO_MAX_VS_DESC, UVC_VIDEO_STREAMING_PTRS, (), n)	\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN_color,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN_ep_fs,
#define UVC_VIDEO_STREAMING_PTRS_HS(n)						\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN_header,		\
	LISTIFY(CONFIG_USBD_VIDEO_MAX_VS_DESC, UVC_VIDEO_STREAMING_PTRS, (), n)	\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN_color,			\
	(struct usb_desc_header *)&uvc_strm_desc_##n.ifN_ep_hs,

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

USBD_VIDEO_DT_DEVICE_DEFINE(0)
