/*
 * Copyright (c) 2024 tinyVision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The macros in this file are not public, applications should not use them.
 * The macros are used to translate devicetree zephyr,uvc compatible nodes
 * into uint8_t array initializer. The output should be treated as a binary blob
 * for the USB host to use (and parse).
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#ifndef ZEPHYR_INCLUDE_USBD_UVC_MACROS_H_
#define ZEPHYR_INCLUDE_USBD_UVC_MACROS_H_

/* Video Interface Subclass Codes */
#define SC_UNDEFINED				0x00
#define SC_VIDEOCONTROL				0x01
#define SC_VIDEOSTREAMING			0x02
#define SC_VIDEO_INTERFACE_COLLECITON		0x03

/* Video Interface Protocol Codes */
#define PC_PROTOCOL_UNDEFINED			0x00
#define PC_PROTOCOL_VERSION_1_5			0x01

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

/* VideoControl Interface Selectors */
#define VC_CONTROL_UNDEFINED			0x00
#define VC_VIDEO_POWER_MODE_CONTROL		0x01
#define VC_REQUEST_ERROR_CODE_CONTROL		0x02

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

/* Descriptors Content */

/* Turn larger types into list of bytes */
#define U(value, shift) (((value) >> shift) & 0xFF)
#define U16_LE(n) U(n, 0), U(n, 8)
#define U24_LE(n) U(n, 0), U(n, 8), U(n, 16)
#define U32_LE(n) U(n, 0), U(n, 8), U(n, 16), U(n, 24)
#define U40_LE(n) U(n, 0), U(n, 8), U(n, 16), U(n, 24), U(n, 32)
#define U48_LE(n) U(n, 0), U(n, 8), U(n, 16), U(n, 24), U(n, 32), U(n, 40)
#define U56_LE(n) U(n, 0), U(n, 8), U(n, 16), U(n, 24), U(n, 32), U(n, 40), U(n, 48)
#define U64_LE(n) U(n, 0), U(n, 8), U(n, 16), U(n, 24), U(n, 32), U(n, 40), U(n, 48), U(n, 56)
#define GUID(node) DT_FOREACH_PROP_ELEM_SEP(node, guid, DT_PROP_BY_IDX, (,))

/* UVC controls for "zephyr,uvc-control-su" */
#define SU_INPUT_SELECT_CONTROL			0x01

/* UVC controls for "zephyr,uvc-control-ct" */
#define CT_SCANNING_MODE_CONTROL		0x01
#define CT_SCANNING_MODE_BIT			0
#define CT_AE_MODE_CONTROL			0x02
#define CT_AE_MODE_BIT				1
#define CT_AE_PRIORITY_CONTROL			0x03
#define CT_AE_PRIORITY_BIT			2
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL	0x04
#define CT_EXPOSURE_TIME_ABSOLUTE_BIT		3
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL	0x05
#define CT_EXPOSURE_TIME_RELATIVE_BIT		4
#define CT_FOCUS_ABSOLUTE_CONTROL		0x06
#define CT_FOCUS_ABSOLUTE_BIT			5
#define CT_FOCUS_RELATIVE_CONTROL		0x07
#define CT_FOCUS_RELATIVE_BIT			6
#define CT_FOCUS_AUTO_CONTROL			0x08
#define CT_FOCUS_AUTO_BIT			17
#define CT_IRIS_ABSOLUTE_CONTROL		0x09
#define CT_IRIS_ABSOLUTE_BIT			7
#define CT_IRIS_RELATIVE_CONTROL		0x0A
#define CT_IRIS_RELATIVE_BIT			8
#define CT_ZOOM_ABSOLUTE_CONTROL		0x0B
#define CT_ZOOM_ABSOLUTE_BIT			9
#define CT_ZOOM_RELATIVE_CONTROL		0x0C
#define CT_ZOOM_RELATIVE_BIT			10
#define CT_PANTILT_ABSOLUTE_CONTROL		0x0D
#define CT_PANTILT_ABSOLUTE_BIT			11
#define CT_PANTILT_RELATIVE_CONTROL		0x0E
#define CT_PANTILT_RELATIVE_BIT			12
#define CT_ROLL_ABSOLUTE_CONTROL		0x0F
#define CT_ROLL_ABSOLUTE_BIT			13
#define CT_ROLL_RELATIVE_CONTROL		0x10
#define CT_ROLL_RELATIVE_BIT			14
#define CT_PRIVACY_CONTROL			0x11
#define CT_PRIVACY_BIT				18
#define CT_FOCUS_SIMPLE_CONTROL			0x12
#define CT_FOCUS_SIMPLE_BIT			19
#define CT_WINDOW_CONTROL			0x13
#define CT_WINDOW_BIT				20
#define CT_REGION_OF_INTEREST_CONTROL		0x14
#define CT_REGION_OF_INTEREST_BIT		21

/* UVC controls for "zephyr,uvc-control-pu" */
#define PU_BACKLIGHT_COMPENSATION_CONTROL	0x01
#define PU_BACKLIGHT_COMPENSATION_BIT		8
#define PU_BRIGHTNESS_CONTROL			0x02
#define PU_BRIGHTNESS_BIT			0
#define PU_CONTRAST_CONTROL			0x03
#define PU_CONTRAST_BIT				1
#define PU_GAIN_CONTROL				0x04
#define PU_GAIN_BIT				9
#define PU_POWER_LINE_FREQUENCY_CONTROL		0x05
#define PU_POWER_LINE_FREQUENCY_BIT		10
#define PU_HUE_CONTROL				0x06
#define PU_HUE_BIT				2
#define PU_SATURATION_CONTROL			0x07
#define PU_SATURATION_BIT			3
#define PU_SHARPNESS_CONTROL			0x08
#define PU_SHARPNESS_BIT			4
#define PU_GAMMA_CONTROL			0x09
#define PU_GAMMA_BIT				5
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL	0x0A
#define PU_WHITE_BALANCE_TEMPERATURE_BIT	6
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0B
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_BIT	12
#define PU_WHITE_BALANCE_COMPONENT_CONTROL	0x0C
#define PU_WHITE_BALANCE_COMPONENT_BIT		7
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL	0x0D
#define PU_WHITE_BALANCE_COMPONENT_AUTO_BIT	13
#define PU_DIGITAL_MULTIPLIER_CONTROL		0x0E
#define PU_DIGITAL_MULTIPLIER_BIT		14
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL	0x0F
#define PU_DIGITAL_MULTIPLIER_LIMIT_BIT		15
#define PU_HUE_AUTO_CONTROL			0x10
#define PU_HUE_AUTO_BIT				11
#define PU_ANALOG_VIDEO_STANDARD_CONTROL	0x11
#define PU_ANALOG_VIDEO_STANDARD_BIT		16
#define PU_ANALOG_LOCK_STATUS_CONTROL		0x12
#define PU_ANALOG_LOCK_STATUS_BIT		17
#define PU_CONTRAST_AUTO_CONTROL		0x13
#define PU_CONTRAST_AUTO_BIT			18

/* UVC controls for "zephyr,uvc-control-eu" */
#define EU_SELECT_LAYER_CONTROL			0x01
#define EU_SELECT_LAYER_BIT			0
#define EU_PROFILE_TOOLSET_CONTROL		0x02
#define EU_PROFILE_TOOLSET_BIT			1
#define EU_VIDEO_RESOLUTION_CONTROL		0x03
#define EU_VIDEO_RESOLUTION_BIT			2
#define EU_MIN_FRAME_INTERVAL_CONTROL		0x04
#define EU_MIN_FRAME_INTERVAL_BIT		3
#define EU_SLICE_MODE_CONTROL			0x05
#define EU_SLICE_MODE_BIT			4
#define EU_RATE_CONTROL_MODE_CONTROL		0x06
#define EU_RATE_CONTROL_MODE_BIT		5
#define EU_AVERAGE_BITRATE_CONTROL		0x07
#define EU_AVERAGE_BITRATE_BIT			6
#define EU_CPB_SIZE_CONTROL			0x08
#define EU_CPB_SIZE_BIT				7
#define EU_PEAK_BIT_RATE_CONTROL		0x09
#define EU_PEAK_BIT_RATE_BIT			8
#define EU_QUANTIZATION_PARAMS_CONTROL		0x0A
#define EU_QUANTIZATION_PARAMS_BIT		9
#define EU_SYNC_REF_FRAME_CONTROL		0x0B
#define EU_SYNC_REF_FRAME_BIT			10
#define EU_LTR_BUFFER_CONTROL			0x0C
#define EU_LTR_BUFFER_BIT			11
#define EU_LTR_PICTURE_CONTROL			0x0D
#define EU_LTR_PICTURE_BIT			12
#define EU_LTR_VALIDATION_CONTROL		0x0E
#define EU_LTR_VALIDATION_BIT			13
#define EU_LEVEL_IDC_LIMIT_CONTROL		0x0F
#define EU_LEVEL_IDC_LIMIT_BIT			14
#define EU_SEI_PAYLOADTYPE_CONTROL		0x10
#define EU_SEI_PAYLOADTYPE_BIT			15
#define EU_QP_RANGE_CONTROL			0x11
#define EU_QP_RANGE_BIT				16
#define EU_PRIORITY_CONTROL			0x12
#define EU_PRIORITY_BIT				17
#define EU_START_OR_STOP_LAYER_CONTROL		0x13
#define EU_START_OR_STOP_LAYER_BIT		18
#define EU_ERROR_RESILIENCY_CONTROL		0x14
#define EU_ERROR_RESILIENCY_BIT			19

/* UVC controls for "zephyr,uvc-control-xu" */
#define XU_BASE_CONTROL				0x00
#define XU_BASE_BIT				0

/* UVC controls for Video Streaming Interface */
#define VS_PROBE_CONTROL			0x01
#define VS_COMMIT_CONTROL			0x02
#define VS_STILL_PROBE_CONTROL			0x03
#define VS_STILL_COMMIT_CONTROL			0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL		0x05
#define VS_STREAM_ERROR_CODE_CONTROL		0x06
#define VS_GENERATE_KEY_FRAME_CONTROL		0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL		0x08
#define VS_SYNCH_DELAY_CONTROL			0x09

/* Map DT_STRING_UPPER_TOKEN(node, compatible) of a node to a bitmap */
#define ZEPHYR_UVC_CONTROL_EU(node, t)						\
	(DT_PROP(node, control_select_layer) << EU_SELECT_LAYER_##t |		\
	 DT_PROP(node, control_profile_toolset) << EU_PROFILE_TOOLSET_##t |	\
	 DT_PROP(node, control_video_resolution) << EU_VIDEO_RESOLUTION_##t |	\
	 DT_PROP(node, control_min_frame_interval) << EU_MIN_FRAME_INTERVAL_##t |\
	 DT_PROP(node, control_slice_mode) << EU_SLICE_MODE_##t |		\
	 DT_PROP(node, control_rate_control_mode) << EU_RATE_CONTROL_MODE_##t |	\
	 DT_PROP(node, control_average_bitrate) << EU_AVERAGE_BITRATE_##t |	\
	 DT_PROP(node, control_cpb_size) << EU_CPB_SIZE_##t |			\
	 DT_PROP(node, control_peak_bit_rate) << EU_PEAK_BIT_RATE_##t |		\
	 DT_PROP(node, control_quantization_params) << EU_QUANTIZATION_PARAMS_##t |\
	 DT_PROP(node, control_sync_ref_frame) << EU_SYNC_REF_FRAME_##t |	\
	 DT_PROP(node, control_ltr_buffer) << EU_LTR_BUFFER_##t |		\
	 DT_PROP(node, control_ltr_picture) << EU_LTR_PICTURE_##t |		\
	 DT_PROP(node, control_ltr_validation) << EU_LTR_VALIDATION_##t |	\
	 DT_PROP(node, control_level_idc_limit) << EU_LEVEL_IDC_LIMIT_##t |	\
	 DT_PROP(node, control_sei_payloadtype) << EU_SEI_PAYLOADTYPE_##t |	\
	 DT_PROP(node, control_qp_range) << EU_QP_RANGE_##t |			\
	 DT_PROP(node, control_priority) << EU_PRIORITY_##t |			\
	 DT_PROP(node, control_start_or_stop_layer) << EU_START_OR_STOP_LAYER_##t |\
	 DT_PROP(node, control_error_resiliency) << EU_ERROR_RESILIENCY_##t)
#define ZEPHYR_UVC_CONTROL_PU(node, t)						\
	(DT_PROP(node, control_backlight_compensation) << PU_BACKLIGHT_COMPENSATION_##t |\
	 DT_PROP(node, control_brightness) << PU_BRIGHTNESS_##t |		\
	 DT_PROP(node, control_contrast) << PU_CONTRAST_##t |			\
	 DT_PROP(node, control_contrast) << PU_CONTRAST_AUTO_##t |		\
	 DT_PROP(node, control_gain) << PU_GAIN_##t |				\
	 DT_PROP(node, control_power_line_frequency) << PU_POWER_LINE_FREQUENCY_##t |\
	 DT_PROP(node, control_hue) << PU_HUE_##t |				\
	 DT_PROP(node, control_hue) << PU_HUE_AUTO_##t |				\
	 DT_PROP(node, control_saturation) << PU_SATURATION_##t |		\
	 DT_PROP(node, control_sharpness) << PU_SHARPNESS_##t |			\
	 DT_PROP(node, control_gamma) << PU_GAMMA_##t |				\
	 DT_PROP(node, control_white_balance_temperature) << PU_WHITE_BALANCE_TEMPERATURE_##t |\
	 DT_PROP(node, control_white_balance_temperature)			\
		<< PU_WHITE_BALANCE_TEMPERATURE_AUTO_##t |			\
	 DT_PROP(node, control_white_balance_component) << PU_WHITE_BALANCE_COMPONENT_##t |\
	 DT_PROP(node, control_white_balance_component) << PU_WHITE_BALANCE_COMPONENT_AUTO_##t |\
	 DT_PROP(node, control_digital_multiplier) << PU_DIGITAL_MULTIPLIER_##t |\
	 DT_PROP(node, control_digital_multiplier_limit) << PU_DIGITAL_MULTIPLIER_LIMIT_##t |\
	 DT_PROP(node, control_analog_video_standard) << PU_ANALOG_VIDEO_STANDARD_##t |\
	 DT_PROP(node, control_analog_lock_status) << PU_ANALOG_LOCK_STATUS_##t)
#define ZEPHYR_UVC_CONTROL_CT(node, t)						\
	(DT_PROP(node, control_scanning_mode) << CT_SCANNING_MODE_##t |		\
	 DT_PROP(node, control_ae_mode) << CT_AE_MODE_##t |			\
	 DT_PROP(node, control_ae_priority) << CT_AE_PRIORITY_##t |		\
	 DT_PROP(node, control_exposure_time_absolute) << CT_EXPOSURE_TIME_ABSOLUTE_##t |\
	 DT_PROP(node, control_exposure_time_relative) << CT_EXPOSURE_TIME_RELATIVE_##t |\
	 DT_PROP(node, control_focus_absolute) << CT_FOCUS_ABSOLUTE_##t |	\
	 DT_PROP(node, control_focus_relative) << CT_FOCUS_RELATIVE_##t |	\
	 DT_PROP(node, control_focus_auto) << CT_FOCUS_AUTO_##t |		\
	 DT_PROP(node, control_iris_absolute) << CT_IRIS_ABSOLUTE_##t |		\
	 DT_PROP(node, control_iris_relative) << CT_IRIS_RELATIVE_##t |		\
	 DT_PROP(node, control_zoom_absolute) << CT_ZOOM_ABSOLUTE_##t |		\
	 DT_PROP(node, control_zoom_relative) << CT_ZOOM_RELATIVE_##t |		\
	 DT_PROP(node, control_pantilt_absolute) << CT_PANTILT_ABSOLUTE_##t |	\
	 DT_PROP(node, control_pantilt_relative) << CT_PANTILT_RELATIVE_##t |	\
	 DT_PROP(node, control_roll_absolute) << CT_ROLL_ABSOLUTE_##t |		\
	 DT_PROP(node, control_roll_relative) << CT_ROLL_RELATIVE_##t |		\
	 DT_PROP(node, control_privacy) << CT_PRIVACY_##t |			\
	 DT_PROP(node, control_focus_simple) << CT_FOCUS_SIMPLE_##t |		\
	 DT_PROP(node, control_window) << CT_WINDOW_##t |			\
	 DT_PROP(node, control_region_of_interest) << CT_REGION_OF_INTEREST_##t)
#define ZEPHYR_UVC_CONTROL_XU(node, t)						\
	(0xffffffffffffffffull >> (64 - DT_PROP(node, control_num)))
#define ZEPHYR_UVC_CONTROL_OT(node, t) 0
#define ZEPHYR_UVC_CONTROL_IT(node, t) 0

/* Extra utilities */
#define IF_NOT_EMPTY(test, expr)						\
	COND_CODE_0(IS_EMPTY(test), expr, ())
#define IF_COMPAT(node, compat, expr)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(node, compat), expr, ())
#define NODE_IF_COMPAT(node, compat) IF_COMPAT(node, compat, (node))
#define LOOKUP_NODE(node, compat)						\
	DT_FOREACH_CHILD_VARGS(node, NODE_IF_COMPAT, compat)

/* Get the ID of the current node, or a child node given its compat */
#define NODE_ID(entity)								\
	UTIL_INC(DT_NODE_CHILD_IDX(entity))
#define LOOKUP_ID(node, compat)							\
	NODE_ID(LOOKUP_NODE(node, compat))

/* Connect the entities to their source(s) */
#define VC_PROP_N_ID(entity, prop, n)						\
	NODE_ID(DT_PHANDLE_BY_IDX(entity, prop, n))
#define VC_SOURCE_ID(entity)							\
	DT_FOREACH_PROP_ELEM_SEP(entity, source_entity, VC_PROP_N_ID, (,))
#define VC_SOURCE_NUM(entity)							\
	DT_PROP_LEN(entity, source_entity)

/* Estimate the frame buffer size out of other fields */
#define MAX_VIDEO_FRAME_BUFFER_SIZE(frame)					\
	(DT_PROP_BY_IDX(frame, size, 0) * DT_PROP_BY_IDX(frame, size, 1) *	\
	 DT_PROP(DT_PARENT(frame), bits_per_pixel) / 8 +			\
	 CONFIG_USBD_VIDEO_HEADER_SIZE)

/* 3.6 Interface Association Descriptor */
#define INTERFACE_ASSOCIATION_DESCRIPTOR(node)					\
	8,						/* bLength */		\
	USB_DESC_INTERFACE_ASSOC,			/* bDescriptorType */	\
	0x00,						/* bFirstInterface */	\
	0x02,						/* bInterfaceCount */	\
	USB_BCC_VIDEO,					/* bFunctionClass */	\
	SC_VIDEO_INTERFACE_COLLECITON,			/* bFunctionSubClass */	\
	PC_PROTOCOL_UNDEFINED,				/* bFunctionProtocol */	\
	0x00,						/* iFunction */

/* Video Control Descriptors */

/* 3.7 VideoControl Interface Descriptors */
#define VC_INTERFACE_DESCRIPTOR(node)						\
	9,						/* bLength */		\
	USB_DESC_INTERFACE,				/* bDescriptorType */	\
	0x00,						/* bInterfaceNumber */	\
	0x00,						/* bAlternateSetting */	\
	0x00,						/* bNumEndpoints */	\
	USB_BCC_VIDEO,					/* bInterfaceClass */	\
	SC_VIDEOCONTROL,				/* bInterfaceSubClass */\
	0x00,						/* bInterfaceProtocol */\
	0x00,						/* iInterface */

/* 3.7.2 Interface Header Descriptor */
#define VC_DESCRIPTORS(node) DT_FOREACH_CHILD(node, VC_DESCRIPTOR)
#define VC_TOTAL_LENGTH(node) sizeof((uint8_t []){VC_DESCRIPTORS(node)})
#define VC_INTERFACE_HEADER_DESCRIPTOR(node)					\
	12 + 1,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_HEADER,					/* bDescriptorSubtype */\
	U16_LE(0x0150),					/* bcdUVC */		\
	U16_LE(12 + 1 + VC_TOTAL_LENGTH(node)),		/* wTotalLength */	\
	U32_LE(30000000),				/* dwClockFrequency */	\
	0x01,						/* bInCollection */	\
	0x01,						/* baInterfaceNr */

/* 3.7.2.1 Input Terminal Descriptor */
#define VC_IT_DESCRIPTOR(entity)						\
	8,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_INPUT_TERMINAL,				/* bDescriptorSubtype */\
	NODE_ID(entity),				/* bTerminalID */	\
	U16_LE(ITT_VENDOR_SPECIFIC),			/* wTerminalType */	\
	0x00,						/* bAssocTerminal */	\
	0x00,						/* iTerminal */

/* 3.7.2.2 Output Terminal Descriptor */
#define VC_OT_DESCRIPTOR(entity)						\
	9,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_OUTPUT_TERMINAL,				/* bDescriptorSubtype */\
	NODE_ID(entity),				/* bTerminalID */	\
	U16_LE(TT_STREAMING),				/* wTerminalType */	\
	0x00,						/* bAssocTerminal */	\
	VC_SOURCE_ID(entity),				/* bSourceID */		\
	0x00,						/* iTerminal */

/* 3.7.2.3 Camera Terminal Descriptor */
#define VC_CT_DESCRIPTOR(entity)						\
	18,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_INPUT_TERMINAL,				/* bDescriptorSubtype */\
	NODE_ID(entity),				/* bTerminalID */	\
	U16_LE(ITT_CAMERA),				/* wTerminalType */	\
	0x00,						/* bAssocTerminal */	\
	0x00,						/* iTerminal */		\
	U16_LE(0),					/* wObjectiveFocalLengthMin */\
	U16_LE(0),					/* wObjectiveFocalLengthMax */\
	U16_LE(0),					/* wOcularFocalLength */\
	0x03,						/* bControlSize */	\
	U24_LE(ZEPHYR_UVC_CONTROL_CT(entity, BIT)),	/* bmControls */

/* 3.7.2.4 Selector Unit Descriptor */
#define VC_SU_DESCRIPTOR(entity)						\
	6 + VC_SOURCE_NUM(entity),			/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_SELECTOR_UNIT,				/* bDescriptorSubtype */\
	NODE_ID(entity),				/* bUnitID */		\
	VC_SOURCE_NUM(entity),				/* bNrInPins */		\
	VC_SOURCE_ID(entity),				/* baSourceID */	\
	0x00,						/* iSelector */

/* 3.7.2.5 Processing Unit Descriptor */
#define VC_PU_DESCRIPTOR(entity)						\
	13,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_PROCESSING_UNIT,				/* bDescriptorSubtype */\
	NODE_ID(entity),				/* bUnitID */		\
	VC_SOURCE_ID(entity),				/* bSourceID */		\
	U16_LE(0),					/* wMaxMultiplier */	\
	0x03,						/* bControlSize */	\
	U24_LE(ZEPHYR_UVC_CONTROL_PU(entity, BIT)),	/* bmControls */	\
	0x00,						/* iProcessing */	\
	0x00,						/* bmVideoStandards */

/* 3.7.2.6 Encoding Unit Descriptor */
#define VC_EU_DESCRIPTOR(entity)						\
	13,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_ENCODING_UNIT,				/* bDescriptorSubtype */\
	NODE_ID(entity),				/* bUnitID */		\
	VC_SOURCE_ID(entity),				/* bSourceID */		\
	0x00,						/* iEncoding */		\
	0x03,						/* bControlSize */	\
	U48_LE(ZEPHYR_UVC_CONTROL_EU(entity, BIT)),	/* bmControls+Runtime */

/* 3.7.2.7 Extension Unit Descriptor */
#define VC_XU_DESCRIPTOR(entity)						\
	24 + 8 + VC_SOURCE_NUM(entity),			/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VC_EXTENSION_UNIT,				/* bDescriptorSubtype */\
	NODE_ID(entity),				/* bUnitID */		\
	GUID(entity),					/* guidExtensionCode */	\
	DT_PROP(entity, control_num),			/* bNumControls */	\
	VC_SOURCE_NUM(entity),				/* bNrInPins */		\
	VC_SOURCE_ID(entity),				/* baSourceID */	\
	0x08,						/* bControlSize */	\
	U64_LE(ZEPHYR_UVC_CONTROL_XU(entity, BIT)),	/* bmControls */	\
	0x00,						/* iExtension */

/* Video Control descriptor content of a node according to its type */
#define VC_DESCRIPTOR(entity)							\
	IF_COMPAT(entity, zephyr_uvc_control_it, (VC_IT_DESCRIPTOR(entity)))	\
	IF_COMPAT(entity, zephyr_uvc_control_ct, (VC_CT_DESCRIPTOR(entity)))	\
	IF_COMPAT(entity, zephyr_uvc_control_ot, (VC_OT_DESCRIPTOR(entity)))	\
	IF_COMPAT(entity, zephyr_uvc_control_su, (VC_SU_DESCRIPTOR(entity)))	\
	IF_COMPAT(entity, zephyr_uvc_control_pu, (VC_PU_DESCRIPTOR(entity)))	\
	IF_COMPAT(entity, zephyr_uvc_control_eu, (VC_EU_DESCRIPTOR(entity)))	\
	IF_COMPAT(entity, zephyr_uvc_control_xu, (VC_XU_DESCRIPTOR(entity)))

/* Video Streaming Descriptors */

/* Automatically assign format IDs based on format order in devicetree */
#define VS_FORMAT_ID(format)							\
	(NODE_ID(format) - LOOKUP_ID(DT_PARENT(format), zephyr_uvc_control_ot))

/* 3.9 VideoStreaming Interface Descriptors */
#define VS_INTERFACE_DESCRIPTOR(node)						\
	9,						/* bLength */		\
	USB_DESC_INTERFACE,				/* bDescriptorType */	\
	0x01,						/* bInterfaceNumber */	\
	0x00,						/* bAlternateSetting */	\
	0x01,						/* bNumEndpoints */	\
	USB_BCC_VIDEO,					/* bInterfaceClass */	\
	SC_VIDEOSTREAMING,				/* bInterfaceSubClass */\
	0x00,						/* bInterfaceProtocol */\
	0x00,						/* iInterface */

/* 3.9.2.1 Input Header Descriptor */
#define VS_CONTROL(format) IF_NOT_EMPTY(VS_DESCRIPTOR(format), (0x00,))
#define VS_CONTROLS(node) DT_FOREACH_CHILD(node, VS_CONTROL)
#define VS_NUM_FORMATS(node) sizeof((uint8_t []){VS_CONTROLS(node)})
/* DT_FOREACH_CHILD() cannot be used or it would be nested within itself */
#define VS_DESCRIPTORS(node) DT_FOREACH_CHILD_SEP(node, VS_DESCRIPTOR, ())
#define VS_TOTAL_LENGTH(node) sizeof((uint8_t []){VS_DESCRIPTORS(node)})
#define VS_INPUT_HEADER_DESCRIPTOR(node)					\
	13,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VS_INPUT_HEADER,				/* bDescriptorSubtype */\
	VS_NUM_FORMATS(node),				/* bNumFormats */	\
	U16_LE(13 + VS_TOTAL_LENGTH(node)),		/* wTotalLength */	\
	0x81,						/* bEndpointAddress */	\
	0x00,						/* bmInfo */		\
	LOOKUP_ID(node, zephyr_uvc_control_ot),		/* bTerminalLink */	\
	0x00,						/* bStillCaptureMethod */\
	0x00,						/* bTriggerSupport */	\
	0x00,						/* bTriggerUsage */	\
	0x00,						/* bControlSize */	\

/* 3.9.2.6 VideoStreaming Color Matching Descriptor */
#define VS_COLOR_MATCHING_DESCRIPTOR(node)					\
	6,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VS_COLORFORMAT,					/* bDescriptorSubtype */\
	0x01,	/* BT.709, sRGB (default) */		/* bColorPrimaries */	\
	0x01,	/* BT.709 (default) */			/* bTransferCharacteristics */\
	0x04,	/* SMPTE 170M, BT.601 (default) */	/* bMatrixCoefficients */

/* 3.10 VideoStreaming Bulk FullSpeed Endpoint Descriptors */
#define VS_FULLSPEED_BULK_ENDPOINT_DESCRIPTOR(node)				\
	7,						/* bLength */		\
	USB_DESC_ENDPOINT,				/* bDescriptorType */	\
	0x81,						/* bEndpointAddress */	\
	USB_EP_TYPE_BULK,				/* bmAttributes */	\
	U16_LE(64),					/* wMaxPacketSize */	\
	0x00,						/* bInterval */

/* 3.10 VideoStreaming Bulk HighSpeed Endpoint Descriptors */
#define VS_HIGHSPEED_BULK_ENDPOINT_DESCRIPTOR(node)				\
	7,						/* bLength */		\
	USB_DESC_ENDPOINT,				/* bDescriptorType */	\
	0x81,						/* bEndpointAddress */	\
	USB_EP_TYPE_BULK,				/* bmAttributes */	\
	U16_LE(512),					/* wMaxPacketSize */	\
	0x00,						/* bInterval */

/* USB_Video_Payload_Uncompressed_1.5.pdf */

/* 3.1.1 Uncompressed Video Format Descriptor */
#define VS_UNCOMPRESSED_FORMAT_DESCRIPTOR(format)				\
	27,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VS_FORMAT_UNCOMPRESSED,				/* bDescriptorSubtype */\
	VS_FORMAT_ID(format),				/* bFormatIndex */	\
	DT_CHILD_NUM(format),				/* bNumFrameDescriptors */\
	GUID(format),					/* guidFormat */	\
	DT_PROP(format, bits_per_pixel),		/* bBitsPerPixel */	\
	0x01,						/* bDefaultFrameIndex */\
	0x00,						/* bAspectRatioX */	\
	0x00,						/* bAspectRatioY */	\
	0x00,						/* bmInterlaceFlags */	\
	0x00,						/* bCopyProtect */

/* 3.2.1 Uncompressed Video Frame Descriptors (discrete) */
#define VS_UNCOMPRESSED_FRAME_DESCRIPTOR(frame)					\
	26 + 4,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VS_FRAME_UNCOMPRESSED,				/* bDescriptorSubtype */\
	NODE_ID(frame),					/* bFrameIndex */	\
	0x00,						/* bmCapabilities */	\
	U16_LE(DT_PROP_BY_IDX(frame, size, 0)),		/* wWidth */		\
	U16_LE(DT_PROP_BY_IDX(frame, size, 1)),		/* wHeight */		\
	U32_LE(15360000),				/* dwMinBitRate */	\
	U32_LE(15360000),				/* dwMaxBitRate */	\
	U32_LE(MAX_VIDEO_FRAME_BUFFER_SIZE(frame)),	/* dwMaxVideoFrameBufferSize */\
	U32_LE(10000000 / DT_PROP(frame, max_fps)),	/* dwDefaultFrameInterval */\
	0x01,						/* bFrameIntervalType */\
	U32_LE(10000000 / DT_PROP(frame, max_fps)),	/* dwFrameInterval */

/* USB_Video_Payload_JPEG_1.5.pdf */

/* 3.1.1 Motion-JPEG Video Format Descriptor */
#define VS_MJPEG_FORMAT_DESCRIPTOR(format)					\
	11,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VS_FORMAT_MJPEG,				/* bDescriptorSubtype */\
	VS_FORMAT_ID(format),				/* bFormatIndex */	\
	DT_CHILD_NUM(format),				/* bNumFrameDescriptors */\
	BIT(0),						/* bmFlags */		\
	0x01,						/* bDefaultFrameIndex */\
	0x00,						/* bAspectRatioX */	\
	0x00,						/* bAspectRatioY */	\
	0x00,						/* bmInterlaceFlags */	\
	0x00,						/* bCopyProtect */

/* 3.2.1 Motion-JPEG Video Frame Descriptors (discrete) */
#define VS_MJPEG_FRAME_DESCRIPTOR(frame)					\
	29,						/* bLength */		\
	USB_DESC_CS_INTERFACE,				/* bDescriptorType */	\
	VS_FRAME_MJPEG,					/* bDescriptorSubtype */\
	NODE_ID(frame),					/* bFrameIndex */	\
	0x00,						/* bmCapabilities */	\
	U16_LE(DT_PROP_BY_IDX(frame, size, 0)),		/* wWidth */		\
	U16_LE(DT_PROP_BY_IDX(frame, size, 1)),		/* wHeight */		\
	U32_LE(15360000),				/* dwMinBitRate */	\
	U32_LE(15360000),				/* dwMaxBitRate */	\
	U32_LE(MAX_VIDEO_FRAME_BUFFER_SIZE(frame)),	/* dwMaxVideoFrameBufferSize */\
	U32_LE(10000000 / DT_PROP(frame, max_fps)),	/* dwDefaultFrameInterval */\
	0x01,						/* bFrameIntervalType */\
	U32_LE(10000000 / DT_PROP(frame, max_fps)),	/* dwFrameInterval */

/* Video Streaming descriptor content of a node according to its type */
#define VS_DESCRIPTOR(format)							\
	IF_COMPAT(format, zephyr_uvc_format_mjpeg, (				\
		VS_MJPEG_FORMAT_DESCRIPTOR(format)				\
		DT_FOREACH_CHILD(format, VS_MJPEG_FRAME_DESCRIPTOR)		\
	))									\
	IF_COMPAT(format, zephyr_uvc_format_uncompressed, (			\
		VS_UNCOMPRESSED_FORMAT_DESCRIPTOR(format)			\
		DT_FOREACH_CHILD(format, VS_UNCOMPRESSED_FRAME_DESCRIPTOR)	\
	))

/* Descriptor Arrays */

#define VS_MJPEG_FRAME_DESCRIPTOR_ARRAY(frame)					\
	static uint8_t frame##_desc[] = {					\
		VS_MJPEG_FRAME_DESCRIPTOR(frame)				\
	};

#define VS_UNCOMPRESSED_FRAME_DESCRIPTOR_ARRAY(frame)				\
	static uint8_t frame##_desc[] = {					\
		VS_UNCOMPRESSED_FRAME_DESCRIPTOR(frame)				\
	};

#define UVC_DESCRIPTOR_ARRAYS(node)						\
	static uint8_t node##_desc_iad[] = {					\
		INTERFACE_ASSOCIATION_DESCRIPTOR(node)				\
	};									\
	static uint8_t node##_desc_if_vc[] = {					\
		VC_INTERFACE_DESCRIPTOR(node)					\
	};									\
	static uint8_t node##_desc_if_vc_header[] = {				\
		VC_INTERFACE_HEADER_DESCRIPTOR(node)				\
	};									\
	static uint8_t node##_desc_if_vs[] = {					\
		VS_INTERFACE_DESCRIPTOR(node)					\
	};									\
	static uint8_t node##_desc_if_vs_header[] = {				\
		VS_INPUT_HEADER_DESCRIPTOR(node)				\
	};									\
	static uint8_t node##_fs_desc_ep[] = {					\
		VS_FULLSPEED_BULK_ENDPOINT_DESCRIPTOR(node)			\
	};									\
	static uint8_t node##_hs_desc_ep[] = {					\
		VS_HIGHSPEED_BULK_ENDPOINT_DESCRIPTOR(node)			\
	};									\
	DT_FOREACH_CHILD_SEP(node, INTERFACE_DESCRIPTOR_ARRAYS, ())

#define INTERFACE_DESCRIPTOR_ARRAYS(node)					\
	static uint8_t node##_desc[] = {					\
		VC_DESCRIPTOR(node)						\
		VS_DESCRIPTOR(node)						\
	};									\
	IF_COMPAT(node, zephyr_uvc_format_mjpeg, (				\
		DT_FOREACH_CHILD(node, VS_MJPEG_FRAME_DESCRIPTOR_ARRAY)		\
	))									\
	IF_COMPAT(node, zephyr_uvc_format_uncompressed, (			\
		DT_FOREACH_CHILD(node, VS_UNCOMPRESSED_FRAME_DESCRIPTOR_ARRAY)	\
	))

/* Descriptor Pointers */

#define DESCRIPTOR_PTR(node)							\
	(struct usb_desc_header *)node##_desc,

#define VC_DESCRIPTOR_PTRS(entity)						\
	IF_NOT_EMPTY(VC_DESCRIPTOR(entity), (					\
		(struct usb_desc_header *)entity##_desc,			\
	))

#define VS_DESCRIPTOR_PTRS(format)						\
	IF_NOT_EMPTY(VS_DESCRIPTOR(format), (					\
		(struct usb_desc_header *)format##_desc,			\
		DT_FOREACH_CHILD_SEP(format, DESCRIPTOR_PTR, ())		\
		(struct usb_desc_header *)color_format_desc,			\
	))

#define UVC_DESCRIPTOR_PTRS(node)						\
	(struct usb_desc_header *)node##_desc_iad,				\
	(struct usb_desc_header *)node##_desc_if_vc,				\
	(struct usb_desc_header *)node##_desc_if_vc_header,			\
	DT_FOREACH_CHILD(node, VC_DESCRIPTOR_PTRS)				\
	(struct usb_desc_header *)node##_desc_if_vs,				\
	(struct usb_desc_header *)node##_desc_if_vs_header,			\
	DT_FOREACH_CHILD(node, VS_DESCRIPTOR_PTRS)

#define UVC_FULLSPEED_DESCRIPTOR_PTRS(node)					\
	UVC_DESCRIPTOR_PTRS(node)						\
	(struct usb_desc_header *)node##_fs_desc_ep,				\
	(struct usb_desc_header *)&nil_desc,

#define UVC_HIGHSPEED_DESCRIPTOR_PTRS(node)					\
	UVC_DESCRIPTOR_PTRS(node)						\
	(struct usb_desc_header *)node##_hs_desc_ep,				\
	(struct usb_desc_header *)&nil_desc,

/* Descriptors identical for all instances */

static struct usb_desc_header nil_desc;

static uint8_t color_format_desc[] = {VS_COLOR_MATCHING_DESCRIPTOR(unused)};

#endif /* ZEPHYR_INCLUDE_USBD_UVC_MACROS_H_ */
