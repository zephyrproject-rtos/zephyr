/*
 * Copyright (c) 2024 tinyVision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Video Class (UVC) public header
 *
 * Header follows the Class Definitions for Video Specification
 * (UVC_1.5_Class_specification.pdf).
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_UVC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_UVC_H_

/* Video Interface Class Code */
#define UVC_CC_VIDEO				0x0E

/* Video Interface Subclass Codes */
#define UVC_SC_UNDEFINED			0x00
#define UVC_SC_VIDEOCONTROL			0x01
#define UVC_SC_VIDEOSTREAMING			0x02
#define UVC_SC_VIDEO_INTERFACE_COLLECITON	0x03

/* Video Interface Protocol Codes */
#define UVC_PC_PROTOCOL_UNDEFINED		0x00
#define UVC_PC_PROTOCOL_15			0x01

/* Video Class-Specific Descriptor Types */
#define UVC_CS_UNDEFINED			0x20
#define UVC_CS_DEVICE				0x21
#define UVC_CS_CONFIGURATION			0x22
#define UVC_CS_STRING				0x23
#define UVC_CS_INTERFACE			0x24
#define UVC_CS_ENDPOINT				0x25

/* Video Class-Specific Video Control Interface Descriptor Subtypes */
#define UVC_VC_DESCRIPTOR_UNDEFINED		0x00
#define UVC_VC_HEADER				0x01
#define UVC_VC_INPUT_TERMINAL			0x02
#define UVC_VC_OUTPUT_TERMINAL			0x03
#define UVC_VC_SELECTOR_UNIT			0x04
#define UVC_VC_PROCESSING_UNIT			0x05
#define UVC_VC_EXTENSION_UNIT			0x06
#define UVC_VC_ENCODING_UNIT			0x07

/* Video Class-Specific Video Stream Interface Descriptor Subtypes */
#define UVC_VS_UNDEFINED			0x00
#define UVC_VS_INPUT_HEADER			0x01
#define UVC_VS_OUTPUT_HEADER			0x02
#define UVC_VS_STILL_IMAGE_FRAME		0x03
#define UVC_VS_FORMAT_UNCOMPRESSED		0x04
#define UVC_VS_FRAME_UNCOMPRESSED		0x05
#define UVC_VS_FORMAT_MJPEG			0x06
#define UVC_VS_FRAME_MJPEG			0x07
#define UVC_VS_FORMAT_MPEG2TS			0x0A
#define UVC_VS_FORMAT_DV			0x0C
#define UVC_VS_COLORFORMAT			0x0D
#define UVC_VS_FORMAT_FRAME_BASED		0x10
#define UVC_VS_FRAME_FRAME_BASED		0x11
#define UVC_VS_FORMAT_STREAM_BASED		0x12
#define UVC_VS_FORMAT_H264			0x13
#define UVC_VS_FRAME_H264			0x14
#define UVC_VS_FORMAT_H264_SIMULCAST		0x15
#define UVC_VS_FORMAT_VP8			0x16
#define UVC_VS_FRAME_VP8			0x17
#define UVC_VS_FORMAT_VP8_SIMULCAST		0x18

/* Video Class-Specific Formats GUIDs */
#define UVC_GUID_UNCOMPRESSED_YUY2 GUID(0x32595559, 0x0000, 0x0010, 0x8000, 0x00AA00389B71)
#define UVC_GUID_UNCOMPRESSED_NV12 GUID(0x3231564E, 0x0000, 0x0010, 0x8000, 0x00AA00389B71)
#define UVC_GUID_UNCOMPRESSED_M420 GUID(0x3032344D, 0x0000, 0x0010, 0x8000, 0x00AA00389B71)
#define UVC_GUID_UNCOMPRESSED_I420 GUID(0x30323449, 0x0000, 0x0010, 0x8000, 0x00AA00389B71)

/* Video Class-Specific Endpoint Descriptor Subtypes */
#define UVC_EP_UNDEFINED			0x00
#define UVC_EP_GENERAL				0x01
#define UVC_EP_ENDPOINT				0x02
#define UVC_EP_INTERRUPT			0x03

/* Video Class-Specific Request Codes */
#define UVC_RC_UNDEFINED			0x00
#define UVC_SET_CUR				0x01
#define UVC_SET_CUR_ALL				0x11
#define UVC_GET_CUR				0x81
#define UVC_GET_MIN				0x82
#define UVC_GET_MAX				0x83
#define UVC_GET_RES				0x84
#define UVC_GET_LEN				0x85
#define UVC_GET_INFO				0x86
#define UVC_GET_DEF				0x87
#define UVC_GET_CUR_ALL				0x91
#define UVC_GET_MIN_ALL				0x92
#define UVC_GET_MAX_ALL				0x93
#define UVC_GET_RES_ALL				0x94
#define UVC_GET_DEF_ALL				0x97

/* VideoControl Interface Control Selectors */
#define UVC_VC_CONTROL_UNDEFINED		0x00
#define UVC_VC_VIDEO_POWER_MODE_CONTROL		0x01
#define UVC_VC_REQUEST_ERROR_CODE_CONTROL	0x02

/* Terminal Control Selectors */
#define UVC_TE_CONTROL_UNDEFINED		0x00

/* Selector Unit Control Selectors */
#define UVC_SU_CONTROL_UNDEFINED		0x00
#define UVC_SU_INPUT_SELECT_CONTROL		0x01

/* Camera Terminal Control Selectors */
#define UVC_CT_CONTROL_UNDEFINED		0x00
#define UVC_CT_SCANNING_MODE_CONTROL		0x01
#define UVC_CT_AE_MODE_CONTROL			0x02
#define UVC_CT_AE_PRIORITY_CONTROL		0x03
#define UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL	0x04
#define UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL	0x05
#define UVC_CT_FOCUS_ABSOLUTE_CONTROL		0x06
#define UVC_CT_FOCUS_RELATIVE_CONTROL		0x07
#define UVC_CT_FOCUS_AUTO_CONTROL		0x08
#define UVC_CT_IRIS_ABSOLUTE_CONTROL		0x09
#define UVC_CT_IRIS_RELATIVE_CONTROL		0x0A
#define UVC_CT_ZOOM_ABSOLUTE_CONTROL		0x0B
#define UVC_CT_ZOOM_RELATIVE_CONTROL		0x0C
#define UVC_CT_PANTILT_ABSOLUTE_CONTROL		0x0D
#define UVC_CT_PANTILT_RELATIVE_CONTROL		0x0E
#define UVC_CT_ROLL_ABSOLUTE_CONTROL		0x0F
#define UVC_CT_ROLL_RELATIVE_CONTROL		0x10
#define UVC_CT_PRIVACY_CONTROL			0x11
#define UVC_CT_FOCUS_SIMPLE_CONTROL		0x12
#define UVC_CT_WINDOW_CONTROL			0x13
#define UVC_CT_REGION_OF_INTEREST_CONTROL	0x14

/* Processing Unit Control Selectors */
#define UVC_PU_CONTROL_UNDEFINED		0x00
#define UVC_PU_BACKLIGHT_COMPENSATION_CONTROL	0x01
#define UVC_PU_BRIGHTNESS_CONTROL		0x02
#define UVC_PU_CONTRAST_CONTROL			0x03
#define UVC_PU_GAIN_CONTROL			0x04
#define UVC_PU_POWER_LINE_FREQUENCY_CONTROL	0x05
#define UVC_PU_HUE_CONTROL			0x06
#define UVC_PU_SATURATION_CONTROL		0x07
#define UVC_PU_SHARPNESS_CONTROL		0x08
#define UVC_PU_GAMMA_CONTROL			0x09
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL0x0A
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL	0x0C
#define UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0D
#define UVC_PU_DIGITAL_MULTIPLIER_CONTROL	0x0E
#define UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL	0x0F
#define UVC_PU_HUE_AUTO_CONTROL			0x10
#define UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL	0x11
#define UVC_PU_ANALOG_LOCK_STATUS_CONTROL	0x12
#define UVC_PU_CONTRAST_AUTO_CONTROL		0x13

/* Encoding Unit Control Selectors */
#define UVC_EU_CONTROL_UNDEFINED		0x00
#define UVC_EU_SELECT_LAYER_CONTROL		0x01
#define UVC_EU_PROFILE_TOOLSET_CONTROL		0x02
#define UVC_EU_VIDEO_RESOLUTION_CONTROL		0x03
#define UVC_EU_MIN_FRAME_INTERVAL_CONTROL	0x04
#define UVC_EU_SLICE_MODE_CONTROL		0x05
#define UVC_EU_RATE_CONTROL_MODE_CONTROL	0x06
#define UVC_EU_AVERAGE_BITRATE_CONTROL		0x07
#define UVC_EU_CPB_SIZE_CONTROL			0x08
#define UVC_EU_PEAK_BIT_RATE_CONTROL		0x09
#define UVC_EU_QUANTIZATION_PARAMS_CONTROL	0x0A
#define UVC_EU_SYNC_REF_FRAME_CONTROL		0x0B
#define UVC_EU_LTR_BUFFER_ CONTROL		0x0C
#define UVC_EU_LTR_PICTURE_CONTROL		0x0D
#define UVC_EU_LTR_VALIDATION_CONTROL		0x0E
#define UVC_EU_LEVEL_IDC_LIMIT_CONTROL		0x0F
#define UVC_EU_SEI_PAYLOADTYPE_CONTROL		0x10
#define UVC_EU_QP_RANGE_CONTROL			0x11
#define UVC_EU_PRIORITY_CONTROL			0x12
#define UVC_EU_START_OR_STOP_LAYER_CONTROL	0x13
#define UVC_EU_ERROR_RESILIENCY_CONTROL		0x14

/* Extension Unit Control Selectors */
#define UVC_XU_CONTROL_UNDEFINED		0x00

/* VideoStreaming Interface Control Selectors */
#define UVC_VS_CONTROL_UNDEFINED		0x00
#define UVC_VS_PROBE_CONTROL			0x01
#define UVC_VS_COMMIT_CONTROL			0x02
#define UVC_VS_STILL_PROBE_CONTROL		0x03
#define UVC_VS_STILL_COMMIT_CONTROL		0x04
#define UVC_VS_STILL_IMAGE_TRIGGER_CONTROL	0x05
#define UVC_VS_STREAM_ERROR_CODE_CONTROL	0x06
#define UVC_VS_GENERATE_KEY_FRAME_CONTROL	0x07
#define UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL	0x08
#define UVC_VS_SYNCH_DELAY_CONTROL		0x09

/* USB Terminal Types */
#define UVC_TT_VENDOR_SPECIFIC			0x0100
#define UVC_TT_STREAMING			0x0101

/* Input Terminal Types */
#define UVC_ITT_VENDOR_SPECIFIC			0x0200
#define UVC_ITT_CAMERA				0x0201
#define UVC_ITT_MEDIA_TRANSPORT_INPUT		0x0202

/* Output Terminal Types */
#define UVC_OTT_VENDOR_SPECIFIC			0x0300
#define UVC_OTT_DISPLAY				0x0301
#define UVC_OTT_MEDIA_TRANSPORT_OUTPUT		0x0302

/* External Terminal Types */
#define UVC_EXTERNAL_VENDOR_SPECIFIC		0x0400
#define UVC_COMPOSITE_CONNECTOR			0x0401
#define UVC_SVIDEO_CONNECTOR			0x0402
#define UVC_COMPONENT_CONNECTOR			0x0403

/* Video Control Interface Status Packet Format */
struct uvc_control_status_packet {
	uint8_t bStatusType;
	uint8_t bOriginator;
	uint8_t bEvent;
	uint8_t bSelector;
	uint8_t bAttribute;
	uint8_t bValue;
} __packed;

/* Video Streaming Interface Status Packet Format */
struct uvc_stream_status_packet {
	uint8_t bStatusType;
	uint8_t bOriginator;
	uint8_t bEvent;
	uint8_t bValue;
} __packed;

/** Video and Still Image Payload Headers */
struct uvc_payload_header {
	uint8_t bHeaderLength;
	uint8_t bmHeaderInfo;
#define UVC_BMHEADERINFO_FRAMEID		(1 << 0)
#define UVC_BMHEADERINFO_END_OF_FRAME		(1 << 1)
#define UVC_BMHEADERINFO_HAS_PRESENTATIONTIME	(1 << 2)
#define UVC_BMHEADERINFO_HAS_SOURCECLOCK	(1 << 3)
#define UVC_BMHEADERINFO_PAYLOAD_SPECIFIC_BIT	(1 << 4)
#define UVC_BMHEADERINFO_STILL_IMAGE		(1 << 5)
#define UVC_BMHEADERINFO_ERROR			(1 << 6)
#define UVC_BMHEADERINFO_END_OF_HEADER		(1 << 7)
	uint32_t dwPresentationTime; /* optional */
	uint32_t scrSourceClockSTC; /* optional */
	uint16_t scrSourceClockSOF; /* optional */
	//uint8_t padding[64 - 12];
} __packed;

/** Interface Header Descriptor */
struct uvc_interface_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdUVC;
	uint16_t wTotalLength;
	uint32_t dwClockFrequency;
	uint8_t bInCollection;
	uint8_t baInterfaceNr;
	/* Only a single Video Streaming interface supported for now */
} __packed;

/** Input Terminal Descriptor */
struct uvc_input_terminal_descriptor {
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

/** Output Terminal Descriptor */
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

/** Camera Terminal Descriptor */
struct uvc_camerma_terminal_descriptor {
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

/** Selector Unit Descriptor */
struct uvc_selector_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bNrInPins;
	uint8_t baSourceID_iSelector[1];
} __packed;

/** Processing Unit Descriptor */
struct uvc_processing_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t wMaxMultiplier;
	uint8_t bControlSize;
	uint8_t bmControls[3];
	uint8_t iProcessing;
	uint8_t bmVideoStandards;
} __packed;

/** Encoding Unit Descriptor */
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

/** Extension Unit Descriptor */
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
	uint8_t bmControls;
	uint8_t iExtension;
} __packed;

/** Class-specific Video Control Interrupt Endpoint Descriptor */
struct uvc_control_interrupt_ep_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint16_t wMaxTransferSize;
} __packed;

/** Class-specific Video Stream Interface Input Header Descriptor */
struct uvc_stream_input_header_descriptor {
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
	uint8_t bmaControls[1];
} __packed;

/** Class-specific Video Stream Interface Output Header Descriptor */
struct uvc_stream_output_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bNumFormats;
	uint16_t wTotalLength;
	uint8_t bEndpointAddress;
	uint8_t bTerminalLink;
	uint8_t bControlSize;
	uint8_t bmaControls[1];
} __packed;

/** Still Image Frame Descriptor */
struct uvc_still_image_frame_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bEndpointAddress;
	uint8_t bNumImageSizePatterns;
	struct {
		uint16_t wWidth;
		uint16_t wHeight;
	} n[1] __packed;
	uint8_t bNumCompression ;
	uint8_t Pattern;
	uint8_t bCompression[1];
} __packed;

/** Color Matching Descriptor */
struct uvc_color_matching_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bColorPrimaries;
	uint8_t bTransferCharacteristics;
	uint8_t bMatrixCoefficients;
} __packed;

/*
 * Header follows the Class Definitions for Video Specification
 * (USB_Video_Payload_Frame_Based.pdf).
 */

/** Header of an USB Video Format Descriptor */
struct uvc_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
} __packed;

/** Uncompressed Video Format Descriptor */
struct uvc_uncompressed_format_descriptor {
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

/** Motion-JPEG Video Format Descriptor */
struct uvc_mjpeg_format_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t bmFlags;
#define UVC_MJPEG_FLAGS_FIXEDSIZESAMPLES	(1 << 0)
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
} __packed;

/** Uncompressed and Motion-JPEG Video Frame Descriptors */
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
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	uint32_t dwMinFrameInterval;
	uint32_t dwMaxFrameInterval;
	uint32_t dwFrameIntervalStep;
} __packed;

/** Video Probe and Commit Controls */
struct uvc_vs_probe_control {
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
#define UVC_BMFRAMING_INFO_FID			(1 << 0)
#define UVC_BMFRAMING_INFO_EOF			(1 << 1)
#define UVC_BMFRAMING_INFO_EOS			(1 << 2)
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

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_UVC_H_ */
