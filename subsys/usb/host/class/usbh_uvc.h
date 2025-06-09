/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Video Class host driver header
 *
 */

#ifndef ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_
#define ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_

#include <zephyr/usb/usb_ch9.h>

/* Video Interface Class Codes */
#define UVC_SC_VIDEOCLASS						   0x0E

/* Video Interface Subclass Codes */
#define UVC_SC_VIDEOCONTROL						 0x01
#define UVC_SC_VIDEOSTREAMING					   0x02
#define UVC_SC_VIDEO_INTERFACE_COLLECTION		   0x03

#define UVC_CS_INTERFACE							0x24
#define UVC_CS_ENDPOINT							 0x25

/* Video Control Interface Descriptor Subtypes */
#define UVC_VC_DESCRIPTOR_UNDEFINED				 0x00
#define UVC_VC_HEADER							   0x01
#define UVC_VC_INPUT_TERMINAL					   0x02
#define UVC_VC_OUTPUT_TERMINAL					  0x03
#define UVC_VC_SELECTOR_UNIT						0x04
#define UVC_VC_PROCESSING_UNIT					  0x05
#define UVC_VC_EXTENSION_UNIT					   0x06
#define UVC_VC_ENCODING_UNIT						0x07

/* Video Streaming Interface Descriptor Subtypes */
#define UVC_VS_UNDEFINED							0x00
#define UVC_VS_INPUT_HEADER						 0x01
#define UVC_VS_OUTPUT_HEADER						0x02
#define UVC_VS_STILL_IMAGE_FRAME					0x03
#define UVC_VS_FORMAT_UNCOMPRESSED				  0x04
#define UVC_VS_FRAME_UNCOMPRESSED				   0x05
#define UVC_VS_FORMAT_MJPEG						 0x06
#define UVC_VS_FRAME_MJPEG						  0x07
#define UVC_VS_FORMAT_MPEG2TS					   0x0A
#define UVC_VS_FORMAT_DV							0x0C
#define UVC_VS_COLORFORMAT						  0x0D
#define UVC_VS_FORMAT_FRAME_BASED				   0x10
#define UVC_VS_FRAME_FRAME_BASED					0x11
#define UVC_VS_FORMAT_STREAM_BASED				  0x12
#define UVC_VS_FORMAT_H264						  0x13
#define UVC_VS_FRAME_H264						   0x14
#define UVC_VS_FORMAT_H264_SIMULCAST				0x15
#define UVC_VS_FORMAT_VP8						   0x16
#define UVC_VS_FRAME_VP8							0x17
#define UVC_VS_FORMAT_VP8_SIMULCAST				 0x18

/* Video Class-Specific Endpoint Descriptor Subtypes */
#define UVC_EP_UNDEFINED							0x00
#define UVC_EP_GENERAL							  0x01
#define UVC_EP_ENDPOINT							 0x02
#define UVC_EP_INTERRUPT							0x03

/* USB Terminal Types */
#define UVC_TT_VENDOR_SPECIFIC					  0x0100
#define UVC_TT_STREAMING							0x0101

/* Input Terminal Types */
#define UVC_ITT_VENDOR_SPECIFIC					 0x0200
#define UVC_ITT_CAMERA							  0x0201
#define UVC_ITT_MEDIA_TRANSPORT_INPUT			   0x0202

/* Output Terminal Types */
#define UVC_OTT_VENDOR_SPECIFIC					 0x0300
#define UVC_OTT_DISPLAY							 0x0301
#define UVC_OTT_MEDIA_TRANSPORT_OUTPUT			  0x0302

/* External Terminal Types */
#define UVC_EXT_EXTERNAL_VENDOR_SPECIFIC			0x0400
#define UVC_EXT_COMPOSITE_CONNECTOR				 0x0401
#define UVC_EXT_SVIDEO_CONNECTOR					0x0402
#define UVC_EXT_COMPONENT_CONNECTOR				 0x0403

/* VideoStreaming Interface Controls */
#define UVC_VS_CONTROL_UNDEFINED					0x00
#define UVC_VS_PROBE_CONTROL						0x01
#define UVC_VS_COMMIT_CONTROL					   0x02
#define UVC_VS_STILL_PROBE_CONTROL				  0x03
#define UVC_VS_STILL_COMMIT_CONTROL				 0x04
#define UVC_VS_STILL_IMAGE_TRIGGER_CONTROL		  0x05
#define UVC_VS_STREAM_ERROR_CODE_CONTROL			0x06
#define UVC_VS_GENERATE_KEY_FRAME_CONTROL		   0x07
#define UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL		 0x08
#define UVC_VS_SYNCH_DELAY_CONTROL				  0x09

/* VideoControl Interface Controls */
#define UVC_VC_CONTROL_UNDEFINED					0x00
#define UVC_VC_VIDEO_POWER_MODE_CONTROL			 0x01
#define UVC_VC_REQUEST_ERROR_CODE_CONTROL		   0x02

/* Selector Unit Controls */
#define UVC_SU_INPUT_SELECT_CONTROL				 0x01

/* Camera Terminal Controls */
#define UVC_CT_CONTROL_UNDEFINED					0x00
#define UVC_CT_SCANNING_MODE_CONTROL				0x01
#define UVC_CT_AE_MODE_CONTROL					  0x02
#define UVC_CT_AE_PRIORITY_CONTROL				  0x03
#define UVC_CT_EXPOSURE_TIME_ABS_CONTROL			0x04
#define UVC_CT_EXPOSURE_TIME_REL_CONTROL			0x05
#define UVC_CT_FOCUS_ABS_CONTROL					0x06
#define UVC_CT_FOCUS_REL_CONTROL					0x07
#define UVC_CT_FOCUS_AUTO_CONTROL				   0x08
#define UVC_CT_IRIS_ABS_CONTROL					 0x09
#define UVC_CT_IRIS_REL_CONTROL					 0x0A
#define UVC_CT_ZOOM_ABS_CONTROL					 0x0B
#define UVC_CT_ZOOM_REL_CONTROL					 0x0C
#define UVC_CT_PANTILT_ABS_CONTROL				  0x0D
#define UVC_CT_PANTILT_REL_CONTROL				  0x0E
#define UVC_CT_ROLL_ABS_CONTROL					 0x0F
#define UVC_CT_ROLL_REL_CONTROL					 0x10
#define UVC_CT_PRIVACY_CONTROL					  0x11
#define UVC_CT_FOCUS_SIMPLE_CONTROL				 0x12
#define UVC_CT_WINDOW_CONTROL					   0x13
#define UVC_CT_REGION_OF_INTEREST_CONTROL		   0x14

/* Processing Unit Controls */
#define UVC_PU_BACKLIGHT_COMPENSATION_CONTROL	   0x01
#define UVC_PU_BRIGHTNESS_CONTROL				   0x02
#define UVC_PU_CONTRAST_CONTROL					 0x03
#define UVC_PU_GAIN_CONTROL						 0x04
#define UVC_PU_POWER_LINE_FREQUENCY_CONTROL		 0x05
#define UVC_PU_HUE_CONTROL						  0x06
#define UVC_PU_SATURATION_CONTROL				   0x07
#define UVC_PU_SHARPNESS_CONTROL					0x08
#define UVC_PU_GAMMA_CONTROL						0x09
#define UVC_PU_WHITE_BALANCE_TEMP_CONTROL		   0x0A
#define UVC_PU_WHITE_BALANCE_TEMP_AUTO_CONTROL	  0x0B
#define UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL	  0x0C
#define UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0D
#define UVC_PU_DIGITAL_MULTIPLIER_CONTROL		   0x0E
#define UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL	 0x0F
#define UVC_PU_HUE_AUTO_CONTROL					 0x10
#define UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL		0x11
#define UVC_PU_ANALOG_LOCK_STATUS_CONTROL		   0x12
#define UVC_PU_CONTRAST_AUTO_CONTROL				0x13

/* Encoding Unit Controls */
#define UVC_EU_SELECT_LAYER_CONTROL				 0x01
#define UVC_EU_PROFILE_TOOLSET_CONTROL			  0x02
#define UVC_EU_VIDEO_RESOLUTION_CONTROL			 0x03
#define UVC_EU_MIN_FRAME_INTERVAL_CONTROL		   0x04
#define UVC_EU_SLICE_MODE_CONTROL				   0x05
#define UVC_EU_RATE_CONTROL_MODE_CONTROL			0x06
#define UVC_EU_AVERAGE_BITRATE_CONTROL			  0x07
#define UVC_EU_CPB_SIZE_CONTROL					 0x08
#define UVC_EU_PEAK_BIT_RATE_CONTROL				0x09
#define UVC_EU_QUANTIZATION_PARAMS_CONTROL		  0x0A
#define UVC_EU_SYNC_REF_FRAME_CONTROL			   0x0B
#define UVC_EU_LTR_BUFFER_CONTROL				   0x0C
#define UVC_EU_LTR_PICTURE_CONTROL				  0x0D
#define UVC_EU_LTR_VALIDATION_CONTROL			   0x0E
#define UVC_EU_LEVEL_IDC_LIMIT_CONTROL			  0x0F
#define UVC_EU_SEI_PAYLOADTYPE_CONTROL			  0x10
#define UVC_EU_QP_RANGE_CONTROL					 0x11
#define UVC_EU_PRIORITY_CONTROL					 0x12
#define UVC_EU_START_OR_STOP_LAYER_CONTROL		  0x13
#define UVC_EU_ERROR_RESILIENCY_CONTROL			 0x14

/* Video Class-Specific Request Code */
#define UVC_RC_UNDEFINED				0x00
#define UVC_SET_CUR					 0x01
#define UVC_SET_CUR_ALL				 0x11
#define UVC_GET_CUR					 0x81
#define UVC_GET_MIN					 0x82
#define UVC_GET_MAX					 0x83
#define UVC_GET_RES					 0x84
#define UVC_GET_LEN					 0x85
#define UVC_GET_INFO					0x86
#define UVC_GET_DEF					 0x87
#define UVC_GET_CUR_ALL				 0x91
#define UVC_GET_MIN_ALL				 0x92
#define UVC_GET_MAX_ALL				 0x93
#define UVC_GET_RES_ALL				 0x94
#define UVC_GET_DEF_ALL				 0x97

/* Processing Unit Control Bit Positions (for bmControls bitmap) */
#define UVC_PU_BMCONTROL_BRIGHTNESS					  BIT(0)
#define UVC_PU_BMCONTROL_CONTRAST						BIT(1)
#define UVC_PU_BMCONTROL_HUE							 BIT(2)
#define UVC_PU_BMCONTROL_SATURATION					  BIT(3)
#define UVC_PU_BMCONTROL_SHARPNESS					   BIT(4)
#define UVC_PU_BMCONTROL_GAMMA						   BIT(5)
#define UVC_PU_BMCONTROL_WHITE_BALANCE_TEMPERATURE	   BIT(6)
#define UVC_PU_BMCONTROL_WHITE_BALANCE_COMPONENT		 BIT(7)
#define UVC_PU_BMCONTROL_BACKLIGHT_COMPENSATION		  BIT(8)
#define UVC_PU_BMCONTROL_GAIN							BIT(9)
#define UVC_PU_BMCONTROL_POWER_LINE_FREQUENCY			BIT(10)
#define UVC_PU_BMCONTROL_HUE_AUTO						BIT(11)
#define UVC_PU_BMCONTROL_WHITE_BALANCE_TEMPERATURE_AUTO  BIT(12)
#define UVC_PU_BMCONTROL_WHITE_BALANCE_COMPONENT_AUTO	BIT(13)
#define UVC_PU_BMCONTROL_DIGITAL_MULTIPLIER			  BIT(14)
#define UVC_PU_BMCONTROL_DIGITAL_MULTIPLIER_LIMIT		BIT(15)
#define UVC_PU_BMCONTROL_ANALOG_VIDEO_STANDARD		   BIT(16)
#define UVC_PU_BMCONTROL_ANALOG_LOCK_STATUS			  BIT(17)
#define UVC_PU_BMCONTROL_CONTRAST_AUTO				   BIT(18)
/* Bits 19-23 are reserved for future use */

/* Camera Terminal Control Bit Positions (for bmControls bitmap) */
#define UVC_CT_BMCONTROL_SCANNING_MODE					  BIT(0)
#define UVC_CT_BMCONTROL_AE_MODE							BIT(1)
#define UVC_CT_BMCONTROL_AE_PRIORITY						BIT(2)
#define UVC_CT_BMCONTROL_EXPOSURE_TIME_ABSOLUTE			 BIT(3)
#define UVC_CT_BMCONTROL_EXPOSURE_TIME_RELATIVE			 BIT(4)
#define UVC_CT_BMCONTROL_FOCUS_ABSOLUTE					 BIT(5)
#define UVC_CT_BMCONTROL_FOCUS_RELATIVE					 BIT(6)
#define UVC_CT_BMCONTROL_IRIS_ABSOLUTE					  BIT(7)
#define UVC_CT_BMCONTROL_IRIS_RELATIVE					  BIT(8)
#define UVC_CT_BMCONTROL_ZOOM_ABSOLUTE					  BIT(9)
#define UVC_CT_BMCONTROL_ZOOM_RELATIVE					  BIT(10)
#define UVC_CT_BMCONTROL_PAN_TILT_ABSOLUTE				  BIT(11)
#define UVC_CT_BMCONTROL_PAN_TILT_RELATIVE				  BIT(12)
#define UVC_CT_BMCONTROL_ROLL_ABSOLUTE					  BIT(13)
#define UVC_CT_BMCONTROL_ROLL_RELATIVE					  BIT(14)
/* Bits 15-16 are reserved */
#define UVC_CT_BMCONTROL_FOCUS_AUTO						 BIT(17)
#define UVC_CT_BMCONTROL_PRIVACY							BIT(18)
#define UVC_CT_BMCONTROL_FOCUS_SIMPLE					   BIT(19)
#define UVC_CT_BMCONTROL_WINDOW							 BIT(20)
#define UVC_CT_BMCONTROL_REGION_OF_INTEREST				 BIT(21)
/* Bits 22-23 are reserved for future use */

/* Video and Still Image Payload Headers */
#define UVC_BMHEADERINFO_FRAMEID					BIT(0)
#define UVC_BMHEADERINFO_END_OF_FRAME			   BIT(1)
#define UVC_BMHEADERINFO_HAS_PRESENTATIONTIME	   BIT(2)
#define UVC_BMHEADERINFO_HAS_SOURCECLOCK			BIT(3)
#define UVC_BMHEADERINFO_PAYLOAD_SPECIFIC_BIT	   BIT(4)
#define UVC_BMHEADERINFO_STILL_IMAGE				BIT(5)
#define UVC_BMHEADERINFO_ERROR					  BIT(6)
#define UVC_BMHEADERINFO_END_OF_HEADER			  BIT(7)

/* Maximum number of MJPEG formats */
#ifndef UVC_MAX_MJPEG_FORMAT
#define UVC_MAX_MJPEG_FORMAT						8
#endif

/* Maximum number of uncompressed formats */
#ifndef UVC_MAX_UNCOMPRESSED_FORMAT
#define UVC_MAX_UNCOMPRESSED_FORMAT				 8
#endif

/* Maximum number of stream interfaces */
#define UVC_STREAM_INTERFACES_MAX_ALT				   32

/* MJPEG alias definition for code consistency */
#ifndef VIDEO_PIX_FMT_MJPEG
#define VIDEO_PIX_FMT_MJPEG						 VIDEO_PIX_FMT_JPEG
#endif

/* Maximum frame intervals configuration */
#ifndef CONFIG_USBH_UVC_MAX_FRAME_INTERVALS
#define CONFIG_USBH_UVC_MAX_FRAME_INTERVALS		 8
#endif

/* Standard Video Interface Collection IAD */
struct usb_interface_association_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* INTERFACE_ASSOCIATION (0x0B) */
	uint8_t  bFirstInterface;
	uint8_t  bInterfaceCount;
	uint8_t  bFunctionClass;		/* UVC_SC_VIDEOCLASS (0x0E) */
	uint8_t  bFunctionSubClass;		/* UVC_SC_VIDEO_INTERFACE_COLLECTION (0x03) */
	uint8_t  bFunctionProtocol;
	uint8_t  iFunction;
} __packed;

/* UVC VideoStreaming Input Header Descriptor */
struct uvc_vs_input_header_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	/* VS_INPUT_HEADER (0x01) */
	uint8_t  bNumFormats;
	uint16_t wTotalLength;
	uint8_t  bEndpointAddress;
	uint8_t  bmInfo;
	uint8_t  bTerminalLink;
	uint8_t  bStillCaptureMethod;
	uint8_t  bTriggerSupport;
	uint8_t  bTriggerUsage;
	uint8_t  bControlSize;
	uint8_t  bmControls[];			/* Variable length control bitmap */
} __packed;

/* UVC VideoStreaming Output Header Descriptor */
struct uvc_vs_output_header_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	/* VS_OUTPUT_HEADER (0x02) */
	uint8_t  bNumFormats;
	uint16_t wTotalLength;
	uint8_t  bEndpointAddress;
	uint8_t  bTerminalLink;
	uint8_t  bControlSize;
	uint8_t  bmControls[];			/* Variable length control bitmap */
} __packed;

/* UVC VideoStreaming Uncompressed Format Descriptor */
struct uvc_vs_format_uncompressed {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
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

/* UVC VideoStreaming MJPEG Format Descriptor */
struct uvc_vs_format_mjpeg {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
	uint8_t bmFlags;
	uint8_t bDefaultFrameIndex;
	uint8_t bAspectRatioX;
	uint8_t bAspectRatioY;
	uint8_t bmInterlaceFlags;
	uint8_t bCopyProtect;
} __packed;

/* UVC VideoStreaming Uncompressed Frame Descriptor */
struct uvc_vs_frame_uncompressed {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	/* Followed by frame interval data */
} __packed;

/* UVC VideoStreaming MJPEG Frame Descriptor */
struct uvc_vs_frame_mjpeg {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
	uint32_t dwMinBitRate;
	uint32_t dwMaxBitRate;
	uint32_t dwMaxVideoFrameBufferSize;
	uint32_t dwDefaultFrameInterval;
	uint8_t bFrameIntervalType;
	/* Followed by frame interval data */
} __packed;

/* UVC Format Descriptor Common Header */
struct uvc_format_header {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFormatIndex;
	uint8_t bNumFrameDescriptors;
} __packed;

/* UVC Frame Descriptor Common Header */
struct uvc_frame_header {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType;
	uint8_t bFrameIndex;
	uint8_t bmCapabilities;
	uint16_t wWidth;
	uint16_t wHeight;
} __packed;

/** Header of an USB CS descriptor */
struct usb_cs_desc_header {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubType; 
} __packed;

/* UVC VideoControl Interface Header Descriptor */
struct uvc_vc_header_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubType;
	uint16_t bcdUVC;
	uint16_t wTotalLength;
	uint32_t dwClockFrequency;
	uint8_t  bInCollection;
	uint8_t  baInterfaceNr[];
} __packed;

/* UVC VideoControl Input Terminal Descriptor */
struct uvc_vc_input_terminal_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	 /* VC_INPUT_TERMINAL (0x02) */
	uint8_t  bTerminalID;
	uint16_t wTerminalType;
	uint8_t  bAssocTerminal;
	uint8_t  iTerminal;
} __packed;

/* UVC VideoControl Output Terminal Descriptor */
struct uvc_vc_output_terminal_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	 /* VC_OUTPUT_TERMINAL (0x03) */
	uint8_t  bTerminalID;
	uint16_t wTerminalType;
	uint8_t  bAssocTerminal;
	uint8_t  bSourceID;
	uint8_t  iTerminal;
} __packed;

/* UVC VideoControl Camera Terminal Descriptor */
struct uvc_vc_camera_terminal_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	 /* VC_INPUT_TERMINAL (0x02) */
	uint8_t  bTerminalID;
	uint16_t wTerminalType;		  /* Should be 0x0201 for Camera Terminal */
	uint8_t  bAssocTerminal;
	uint8_t  iTerminal;
	uint16_t wObjectiveFocalLengthMin;
	uint16_t wObjectiveFocalLengthMax;
	uint16_t wOcularFocalLength;
	uint8_t  bControlSize;
	uint8_t  bmControls[];		   /* Variable length control bitmap */
} __packed;

/* UVC VideoControl Selector Unit Descriptor */
struct uvc_vc_selector_unit_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	 /* VC_SELECTOR_UNIT (0x04) */
	uint8_t  bUnitID;
	uint8_t  bNrInPins;
	uint8_t  baSourceID[];		   /* Variable length array of source IDs */
	/* Note: iSelector field follows baSourceID but is not included here 
	 * due to variable length baSourceID array */
} __packed;

/* UVC VideoControl Processing Unit Descriptor */
struct uvc_vc_processing_unit_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	 /* VC_PROCESSING_UNIT (0x05) */
	uint8_t  bUnitID;
	uint8_t  bSourceID;
	uint16_t wMaxMultiplier;
	uint8_t  bControlSize;
	uint8_t  bmControls[];		   /* Variable length control bitmap */
	/* Note: iProcessing field follows bmControls but is not included here 
	 * due to variable length bmControls array */
} __packed;

/* UVC VideoControl Encoding Unit Descriptor */
struct uvc_vc_encoding_unit_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	 /* VC_ENCODING_UNIT (0x07) */
	uint8_t  bUnitID;
	uint8_t  bSourceID;
	uint8_t  iEncoding;
	uint8_t  bControlSize;
	uint8_t  bmControls[];		   /* Variable length control bitmap */
	/* Note: bmControlsRuntime field may follow bmControls depending on version */
} __packed;

/* UVC VideoControl Extension Unit Descriptor */
struct uvc_vc_extension_unit_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;		/* CS_INTERFACE (0x24) */
	uint8_t  bDescriptorSubType;	 /* VC_EXTENSION_UNIT (0x06) */
	uint8_t  bUnitID;
	uint8_t  guidExtensionCode[16];  /* GUID identifying the Extension Unit */
	uint8_t  bNumControls;
	uint8_t  bNrInPins;
	uint8_t  baSourceID[];		   /* Variable length array of source IDs */
	/* Note: bControlSize, bmControls[], and iExtension fields follow baSourceID 
	 * but are not included here due to variable length baSourceID array */
} __packed;

/* UVC Payload Header */
struct uvc_payload_header {
    uint8_t bHeaderLength;
    uint8_t bmHeaderInfo;
    uint32_t dwPresentationTime;
    uint8_t scrSourceClock[6];
} __packed;

/* Video Probe and Commit Controls */
struct uvc_probe_commit {
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
	uint8_t bPreferedVersion;
	uint8_t bMinVersion;
	uint8_t bMaxVersion;
} __packed;

/* Video Still Probe Control and Still Commit Control */
struct uvc_still_probe_commit {
	uint8_t bFormatIndex;							   /* Still image format index */
	uint8_t bFrameIndex;								/* Still image frame index */
	uint8_t bCompressionIndex;						  /* Compression level index */
	uint32_t dwMaxVideoFrameSize;					   /* Maximum image size */
	uint32_t dwMaxPayloadTransferSize;				  /* Maximum transfer payload */
} __packed;


/* UVC frame interval types */
enum uvc_frame_interval_type {
	UVC_FRAME_INTERVAL_DISCRETE,
	UVC_FRAME_INTERVAL_CONTINUOUS,
	UVC_FRAME_INTERVAL_STEPWISE,
};

/* UVC frame information structure */
struct uvc_frame_info {
	uint8_t frame_index;
	uint16_t width;
	uint16_t height;
	uint32_t min_bit_rate;
	uint32_t max_bit_rate;
	uint32_t max_frame_buffer_size;
	uint32_t default_frame_interval;
	
	enum uvc_frame_interval_type interval_type;
	uint8_t num_intervals;
	union {
		uint32_t discrete[CONFIG_USBH_UVC_MAX_FRAME_INTERVALS];
		struct {
			uint32_t min;
			uint32_t max;
			uint32_t step;
		} stepwise;
	} intervals;
};

struct uvc_stream_iface_info {
	struct usb_if_descriptor *current_stream_iface;	 /* Stream interface */
	struct usb_ep_descriptor *current_stream_ep;		/* Stream endpoint */
	uint32_t cur_ep_mps_mult;						  /* Endpoint max packet size including multiple transactions */
};

/* UVC format structure */
struct uvc_vs_format {
	uint32_t pixelformat;
	uint16_t width;
	uint16_t height;
	uint32_t fps;
	uint32_t frame_interval;
	uint32_t pitch;
	uint8_t format_index;
	uint8_t frame_index;
	struct uvc_format_header *format_ptr;
	struct uvc_frame_header *frame_ptr;
};

struct uvc_vs_format_mjpeg_info {
	struct uvc_vs_format_mjpeg *vs_mjpeg_format[UVC_MAX_MJPEG_FORMAT];
	uint8_t num_mjpeg_formats;
};

struct uvc_vs_format_uncompressed_info {
	struct uvc_vs_format_uncompressed *uncompressed_format[UVC_MAX_UNCOMPRESSED_FORMAT];
	uint8_t num_uncompressed_formats;
};

/* Video stream format information structure */
struct uvc_vs_format_info {
	struct uvc_vs_format_mjpeg_info format_mjpeg;
	struct uvc_vs_format_uncompressed_info format_uncompressed;
};

#endif /* ZEPHYR_INCLUDE_USBH_CLASS_UVC_H_ */
