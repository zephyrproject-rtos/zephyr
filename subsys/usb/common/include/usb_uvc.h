/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Video Class private header
 *
 * Header follows below documentation:
 * - USB Device Class Definition for Video Devices (Revision 1.5)
 *
 * Additional documentation considered a part of UVC 1.5:
 * - USB Device Class Definition for Video Devices: Uncompressed Payload (Revision 1.5)
 * - USB Device Class Definition for Video Devices: Motion-JPEG Payload (Revision 1.5)
 */

#ifndef ZEPHYR_INCLUDE_USBD_CLASS_UVC_H_
#define ZEPHYR_INCLUDE_USBD_CLASS_UVC_H_

#include <zephyr/usb/usb_ch9.h>

/* Video Class-Specific Request Codes */
#define UVC_SET_CUR					0x01
#define UVC_GET_CUR					0x81
#define UVC_GET_MIN					0x82
#define UVC_GET_MAX					0x83
#define UVC_GET_RES					0x84
#define UVC_GET_LEN					0x85
#define UVC_GET_INFO					0x86
#define UVC_GET_DEF					0x87

/* Flags announcing which controls are supported */
#define UVC_INFO_SUPPORTS_GET				BIT(0)
#define UVC_INFO_SUPPORTS_SET				BIT(1)

/* Request Error Code Control */
#define UVC_ERR_NOT_READY				0x01
#define UVC_ERR_WRONG_STATE				0x02
#define UVC_ERR_OUT_OF_RANGE				0x04
#define UVC_ERR_INVALID_UNIT				0x05
#define UVC_ERR_INVALID_CONTROL				0x06
#define UVC_ERR_INVALID_REQUEST				0x07
#define UVC_ERR_INVALID_VALUE_WITHIN_RANGE		0x08
#define UVC_ERR_UNKNOWN					0xff

/* Video and Still Image Payload Headers */
#define UVC_BMHEADERINFO_FRAMEID			BIT(0)
#define UVC_BMHEADERINFO_END_OF_FRAME			BIT(1)
#define UVC_BMHEADERINFO_HAS_PRESENTATIONTIME		BIT(2)
#define UVC_BMHEADERINFO_HAS_SOURCECLOCK		BIT(3)
#define UVC_BMHEADERINFO_PAYLOAD_SPECIFIC_BIT		BIT(4)
#define UVC_BMHEADERINFO_STILL_IMAGE			BIT(5)
#define UVC_BMHEADERINFO_ERROR				BIT(6)
#define UVC_BMHEADERINFO_END_OF_HEADER			BIT(7)

/* Video Interface Subclass Codes */
#define UVC_SC_VIDEOCONTROL				0x01
#define UVC_SC_VIDEOSTREAMING				0x02
#define UVC_SC_VIDEO_INTERFACE_COLLECTION		0x03

/* Video Class-Specific Video Control Interface Descriptor Subtypes */
#define UVC_VC_DESCRIPTOR_UNDEFINED			0x00
#define UVC_VC_HEADER					0x01
#define UVC_VC_INPUT_TERMINAL				0x02
#define UVC_VC_OUTPUT_TERMINAL				0x03
#define UVC_VC_SELECTOR_UNIT				0x04
#define UVC_VC_PROCESSING_UNIT				0x05
#define UVC_VC_EXTENSION_UNIT				0x06
#define UVC_VC_ENCODING_UNIT				0x07

/* Video Class-Specific Video Stream Interface Descriptor Subtypes */
#define UVC_VS_UNDEFINED				0x00
#define UVC_VS_INPUT_HEADER				0x01
#define UVC_VS_OUTPUT_HEADER				0x02
#define UVC_VS_STILL_IMAGE_FRAME			0x03
#define UVC_VS_FORMAT_UNCOMPRESSED			0x04
#define UVC_VS_FRAME_UNCOMPRESSED			0x05
#define UVC_VS_FORMAT_MJPEG				0x06
#define UVC_VS_FRAME_MJPEG				0x07
#define UVC_VS_FORMAT_MPEG2TS				0x0A
#define UVC_VS_FORMAT_DV				0x0C
#define UVC_VS_COLORFORMAT				0x0D
#define UVC_VS_FORMAT_FRAME_BASED			0x10
#define UVC_VS_FRAME_FRAME_BASED			0x11
#define UVC_VS_FORMAT_STREAM_BASED			0x12
#define UVC_VS_FORMAT_H264				0x13
#define UVC_VS_FRAME_H264				0x14
#define UVC_VS_FORMAT_H264_SIMULCAST			0x15
#define UVC_VS_FORMAT_VP8				0x16
#define UVC_VS_FRAME_VP8				0x17
#define UVC_VS_FORMAT_VP8_SIMULCAST			0x18

/* Video Class-Specific Endpoint Descriptor Subtypes */
#define UVC_EP_UNDEFINED				0x00
#define UVC_EP_GENERAL					0x01
#define UVC_EP_ENDPOINT					0x02
#define UVC_EP_INTERRUPT				0x03

/* USB Terminal Types */
#define UVC_TT_VENDOR_SPECIFIC				0x0100
#define UVC_TT_STREAMING				0x0101

/* Input Terminal Types */
#define UVC_ITT_VENDOR_SPECIFIC				0x0200
#define UVC_ITT_CAMERA					0x0201
#define UVC_ITT_MEDIA_TRANSPORT_INPUT			0x0202

/* Output Terminal Types */
#define UVC_OTT_VENDOR_SPECIFIC				0x0300
#define UVC_OTT_DISPLAY					0x0301
#define UVC_OTT_MEDIA_TRANSPORT_OUTPUT			0x0302

/* External Terminal Types */
#define UVC_EXT_EXTERNAL_VENDOR_SPECIFIC		0x0400
#define UVC_EXT_COMPOSITE_CONNECTOR			0x0401
#define UVC_EXT_SVIDEO_CONNECTOR			0x0402
#define UVC_EXT_COMPONENT_CONNECTOR			0x0403

/* VideoStreaming Interface Controls */
#define UVC_VS_PROBE_CONTROL				0x01
#define UVC_VS_COMMIT_CONTROL				0x02
#define UVC_VS_STILL_PROBE_CONTROL			0x03
#define UVC_VS_STILL_COMMIT_CONTROL			0x04
#define UVC_VS_STILL_IMAGE_TRIGGER_CONTROL		0x05
#define UVC_VS_STREAM_ERROR_CODE_CONTROL		0x06
#define UVC_VS_GENERATE_KEY_FRAME_CONTROL		0x07
#define UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL		0x08
#define UVC_VS_SYNCH_DELAY_CONTROL			0x09

/* VideoControl Interface Controls */
#define UVC_VC_CONTROL_UNDEFINED			0x00
#define UVC_VC_VIDEO_POWER_MODE_CONTROL			0x01
#define UVC_VC_REQUEST_ERROR_CODE_CONTROL		0x02

/* Selector Unit Controls */
#define UVC_SU_INPUT_SELECT_CONTROL			0x01

/* Camera Terminal Controls */
#define UVC_CT_SCANNING_MODE_CONTROL			0x01
#define UVC_CT_AE_MODE_CONTROL				0x02
#define UVC_CT_AE_PRIORITY_CONTROL			0x03
#define UVC_CT_EXPOSURE_TIME_ABS_CONTROL		0x04
#define UVC_CT_EXPOSURE_TIME_REL_CONTROL		0x05
#define UVC_CT_FOCUS_ABS_CONTROL			0x06
#define UVC_CT_FOCUS_REL_CONTROL			0x07
#define UVC_CT_FOCUS_AUTO_CONTROL			0x08
#define UVC_CT_IRIS_ABS_CONTROL				0x09
#define UVC_CT_IRIS_REL_CONTROL				0x0A
#define UVC_CT_ZOOM_ABS_CONTROL				0x0B
#define UVC_CT_ZOOM_REL_CONTROL				0x0C
#define UVC_CT_PANTILT_ABS_CONTROL			0x0D
#define UVC_CT_PANTILT_REL_CONTROL			0x0E
#define UVC_CT_ROLL_ABS_CONTROL				0x0F
#define UVC_CT_ROLL_REL_CONTROL				0x10
#define UVC_CT_PRIVACY_CONTROL				0x11
#define UVC_CT_FOCUS_SIMPLE_CONTROL			0x12
#define UVC_CT_WINDOW_CONTROL				0x13
#define UVC_CT_REGION_OF_INTEREST_CONTROL		0x14

/* Processing Unit Controls */
#define UVC_PU_BACKLIGHT_COMPENSATION_CONTROL		0x01
#define UVC_PU_BRIGHTNESS_CONTROL			0x02
#define UVC_PU_CONTRAST_CONTROL				0x03
#define UVC_PU_GAIN_CONTROL				0x04
#define UVC_PU_POWER_LINE_FREQUENCY_CONTROL		0x05
#define UVC_PU_HUE_CONTROL				0x06
#define UVC_PU_SATURATION_CONTROL			0x07
#define UVC_PU_SHARPNESS_CONTROL			0x08
#define UVC_PU_GAMMA_CONTROL				0x09
#define UVC_PU_WHITE_BALANCE_TEMP_CONTROL		0x0A
#define UVC_PU_WHITE_BALANCE_TEMP_AUTO_CONTROL		0x0B
#define UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL		0x0C
#define UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL	0x0D
#define UVC_PU_DIGITAL_MULTIPLIER_CONTROL		0x0E
#define UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL		0x0F
#define UVC_PU_HUE_AUTO_CONTROL				0x10
#define UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL		0x11
#define UVC_PU_ANALOG_LOCK_STATUS_CONTROL		0x12
#define UVC_PU_CONTRAST_AUTO_CONTROL			0x13

/* Encoding Unit Controls */
#define UVC_EU_SELECT_LAYER_CONTROL			0x01
#define UVC_EU_PROFILE_TOOLSET_CONTROL			0x02
#define UVC_EU_VIDEO_RESOLUTION_CONTROL			0x03
#define UVC_EU_MIN_FRAME_INTERVAL_CONTROL		0x04
#define UVC_EU_SLICE_MODE_CONTROL			0x05
#define UVC_EU_RATE_CONTROL_MODE_CONTROL		0x06
#define UVC_EU_AVERAGE_BITRATE_CONTROL			0x07
#define UVC_EU_CPB_SIZE_CONTROL				0x08
#define UVC_EU_PEAK_BIT_RATE_CONTROL			0x09
#define UVC_EU_QUANTIZATION_PARAMS_CONTROL		0x0A
#define UVC_EU_SYNC_REF_FRAME_CONTROL			0x0B
#define UVC_EU_LTR_BUFFER_CONTROL			0x0C
#define UVC_EU_LTR_PICTURE_CONTROL			0x0D
#define UVC_EU_LTR_VALIDATION_CONTROL			0x0E
#define UVC_EU_LEVEL_IDC_LIMIT_CONTROL			0x0F
#define UVC_EU_SEI_PAYLOADTYPE_CONTROL			0x10
#define UVC_EU_QP_RANGE_CONTROL				0x11
#define UVC_EU_PRIORITY_CONTROL				0x12
#define UVC_EU_START_OR_STOP_LAYER_CONTROL		0x13
#define UVC_EU_ERROR_RESILIENCY_CONTROL			0x14

/* Extension Unit Controls */
#define UVC_XU_BASE_CONTROL				0x00

/* Base GUID string present at the end of most GUID formats, preceded by the FourCC code */
#define UVC_FORMAT_GUID(fourcc) fourcc "\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71"

struct uvc_if_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
} __packed;

struct uvc_control_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdUVC;
	uint16_t wTotalLength;
	uint32_t dwClockFrequency;
	uint8_t bInCollection;
	uint8_t baInterfaceNr[1];
} __packed;

struct uvc_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
};

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
	/* Other fields depending on bDescriptorSubtype value */
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

struct uvc_format_frame_based_descriptor {
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
	uint8_t bVariableSize;
} __packed;

struct uvc_frame_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	/* Other fields depending on bDescriptorSubtype value */
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

struct uvc_frame_based_continuous_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwMinFrameInterval;
	uint32_t dwMaxFrameInterval;
	uint32_t dwFrameIntervalStep;
} __packed;

struct uvc_frame_based_discrete_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwFrameInterval[CONFIG_USBD_VIDEO_MAX_FRMIVAL];
} __packed;

struct uvc_color_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bColorPrimaries;
	uint8_t bTransferCharacteristics;
	uint8_t bMatrixCoefficients;
#define UVC_COLOR_BT709		1
#define UVC_COLOR_BT470M	2
#define UVC_COLOR_BT470BG	3
#define UVC_COLOR_BT601		4
#define UVC_COLOR_SMPTE170M	4
#define UVC_COLOR_SMPTE240M	5
#define UVC_COLOR_LINEAR	6
#define UVC_COLOR_SRGB		7
} __packed;

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

/* This is a particular variant of this struct that is used by the Zephyr implementation. Other
 * organization of the fields are allowed by the standard.
 */
struct uvc_payload_header {
	uint8_t bHeaderLength;
	uint8_t bmHeaderInfo;
	uint32_t dwPresentationTime; /* optional */
	uint32_t scrSourceClockSTC;  /* optional */
	uint16_t scrSourceClockSOF;  /* optional */
} __packed;

#endif /* ZEPHYR_INCLUDE_USBD_CLASS_UVC_H_ */
