/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The macros in this file are not public, applications should not use them.
 * The macros are used to translate devicetree zephyr,uac2 compatible nodes
 * into uint8_t array initializer. The output should be treated as a binary blob
 * for the USB host to use (and parse).
 */

#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#ifndef ZEPHYR_INCLUDE_USBD_UAC2_MACROS_H_
#define ZEPHYR_INCLUDE_USBD_UAC2_MACROS_H_

#define ARRAY_BIT(idx, value) (value##ull << idx)

#define U16_LE(value) ((value) & 0xFF), (((value) & 0xFF00) >> 8)
#define U32_LE(value)								\
	((value) & 0xFF),							\
	(((value) & 0xFF00) >> 8),						\
	(((value) & 0xFF0000) >> 16),						\
	(((value) & 0xFF000000) >> 24)

#define ARRAY_ELEMENT_LESS_THAN_NEXT(node, prop, idx)				\
	COND_CODE_1(IS_EQ(idx, UTIL_DEC(DT_PROP_LEN(node, prop))),		\
		(1 /* nothing to compare the last element against */),		\
		((DT_PROP_BY_IDX(node, prop, idx) <				\
		  DT_PROP_BY_IDX(node, prop, UTIL_INC(idx)))))
#define IS_ARRAY_SORTED(node, prop)						\
	DT_FOREACH_PROP_ELEM_SEP(node, prop, ARRAY_ELEMENT_LESS_THAN_NEXT, (&&))

#define FIRST_INTERFACE_NUMBER			0x00
#define FIRST_IN_EP_ADDR			0x81
#define FIRST_OUT_EP_ADDR			0x01

/* A.1 Audio Function Class Code */
#define AUDIO_FUNCTION				AUDIO

/* A.2 Audio Function Subclass Codes */
#define FUNCTION_SUBCLASS_UNDEFINED		0x00

/* A.3 Audio Function Protocol Codes */
#define FUNCTION_PROTOCOL_UNDEFINED		0x00
#define AF_VERSION_02_00			IP_VERSION_02_00

/* A.4 Audio Interface Class Code */
#define AUDIO					0x01

/* A.5 Audio Interface Subclass Codes */
#define INTERFACE_SUBCLASS_UNDEFINED		0x00
#define AUDIOCONTROL				0x01
#define AUDIOSTREAMING				0x02
#define MIDISTREAMING				0x03

/* A.6 Audio Interface Protocol Codes */
#define INTERFACE_PROTOCOL_UNDEFINED		0x00
#define IP_VERSION_02_00			0x20

/* A.8 Audio Class-Specific Descriptor Types */
#define CS_UNDEFINED		0x20
#define CS_DEVICE		0x21
#define CS_CONFIGURATION	0x22
#define CS_STRING		0x23
#define CS_INTERFACE		0x24
#define CS_ENDPOINT		0x25

/* A.9 Audio Class-Specific AC Interface Descriptor Subtypes */
#define AC_DESCRIPTOR_UNDEFINED			0x00
#define AC_DESCRIPTOR_HEADER			0x01
#define AC_DESCRIPTOR_INPUT_TERMINAL		0x02
#define AC_DESCRIPTOR_OUTPUT_TERMINAL		0x03
#define AC_DESCRIPTOR_MIXER_UNIT		0x04
#define AC_DESCRIPTOR_SELECTOR_UNIT		0x05
#define AC_DESCRIPTOR_FEATURE_UNIT		0x06
#define AC_DESCRIPTOR_EFFECT_UNIT		0x07
#define AC_DESCRIPTOR_PROCESSING_UNIT		0x08
#define AC_DESCRIPTOR_EXTENSION_UNIT		0x09
#define AC_DESCRIPTOR_CLOCK_SOURCE		0x0A
#define AC_DESCRIPTOR_CLOCK_SELECTOR		0x0B
#define AC_DESCRIPTOR_CLOCK_MULTIPLIER		0x0C
#define AC_DESCRIPTOR_SAMPLE_RATE_CONVERTER	0x0D

/* A.10 Audio Class-Specific AS Interface Descriptor Subtypes */
#define AS_DESCRIPTOR_UNDEFINED			0x00
#define AS_DESCRIPTOR_GENERAL			0x01
#define AS_DESCRIPTOR_FORMAT_TYPE		0x02
#define AS_DESCRIPTOR_ENCODER			0x03
#define AS_DESCRIPTOR_DECODER			0x04

/* A.13 Audio Class-Specific Endpoint Descriptor Subtypes */
#define DESCRIPTOR_UNDEFINED			0x00
#define EP_GENERAL				0x01

/* Universal Serial Bus Device Class Definition for Audio Data Formats
 * Release 2.0, May 31, 2006. A.1 Format Type Codes
 * Values are in decimal to facilitate use with IS_EQ() macro.
 */
#define FORMAT_TYPE_UNDEFINED			0
#define FORMAT_TYPE_I				1
#define FORMAT_TYPE_II				2
#define FORMAT_TYPE_III				3
#define FORMAT_TYPE_IV				4
#define EXT_FORMAT_TYPE_I			129
#define EXT_FORMAT_TYPE_II			130
#define EXT_FORMAT_TYPE_III			131

/* Convert 0 to empty and everything else to itself */
#define EMPTY_ON_ZERO(value) COND_CODE_0(value, (), (value))

/* Automatically assign Entity IDs based on entities order in devicetree */
#define ENTITY_ID(e) UTIL_INC(DT_NODE_CHILD_IDX(e))

/* Name of uint8_t array holding descriptor data */
#define DESCRIPTOR_NAME(prefix, node) uac2_## prefix ## _ ## node

/* Connected Entity ID or 0 if property is not defined. Rely on devicetree
 * "required: true" to fail compilation if mandatory handle (e.g. clock source)
 * is absent.
 */
#define CONNECTED_ENTITY_ID(entity, phandle)					\
	COND_CODE_1(DT_NODE_HAS_PROP(entity, phandle),				\
		(ENTITY_ID(DT_PHANDLE_BY_IDX(entity, phandle, 0))), (0))

#define ID_IF_TERMINAL_ASSOCIATES_WITH_TARGET(entity, target_id)		\
	IF_ENABLED(UTIL_AND(IS_EQ(						\
		CONNECTED_ENTITY_ID(entity, assoc_terminal), target_id),	\
		UTIL_OR(							\
			DT_NODE_HAS_COMPAT(entity, zephyr_uac2_input_terminal),	\
			DT_NODE_HAS_COMPAT(entity, zephyr_uac2_output_terminal)	\
		)), (ENTITY_ID(entity)))

/* Find ID of terminal entity associated with given terminal entity. This macro
 * evaluates to "+ 0" if there isn't any terminal entity associated. If there
 * are terminal entities associated with given terminal, then the macro evalues
 * to "IDs + 0" where IDs are the terminal entities ID separated by spaces.
 *
 * If there is exactly one ID then compiler will compute correct value.
 * If there is more than one associated entity, then it will fail at build time
 * (as it should) because the caller expects single integer.
 */
#define FIND_ASSOCIATED_TERMINAL(entity)					\
	DT_FOREACH_CHILD_VARGS(DT_PARENT(entity),				\
		ID_IF_TERMINAL_ASSOCIATES_WITH_TARGET, ENTITY_ID(entity)) + 0

/* If entity has assoc_terminal property, return the entity ID of associated
 * terminal. Otherwise, search if any other terminal entity points to us and
 * use its ID. If search yields no results then this evaluates to "+ 0" which
 * matches the value USB Audio Class expects in bAssocTerminal if no association
 * exists.
 *
 * This is a workaround for lack of cyclic dependencies support in devicetree.
 */
#define ASSOCIATED_TERMINAL_ID(entity)						\
	COND_CODE_1(DT_NODE_HAS_PROP(entity, assoc_terminal),			\
		(CONNECTED_ENTITY_ID(entity, assoc_terminal)),			\
		(FIND_ASSOCIATED_TERMINAL(entity)))


#define CLOCK_SOURCE_ATTRIBUTES(entity)						\
	(DT_ENUM_IDX(entity, clock_type)) |					\
	(DT_PROP(entity, sof_synchronized) << 2)

/* Control properties are optional enums in devicetree that can either be
 * "read-only" or "host-programmable". If the property is missing, then it means
 * that control is not present. Convert the control property into actual values
 * used by USB Audio Class, i.e. 0b00 when control is not present, 0b01 when
 * control is present but read-only and 0b11 when control can be programmed by
 * host. Value 0b10 is not allowed by the specification.
 */
#define CONTROL_NOT_PRESENT		0x0
#define CONTROL_READ_ONLY		0x1
#define CONTROL_HOST_PROGRAMMABLE	0x3

#define CONTROL_TOKEN(entity, control_name)					\
	COND_CODE_1(DT_NODE_HAS_PROP(entity, control_name),			\
		(DT_STRING_UPPER_TOKEN(entity, control_name)),			\
		(NOT_PRESENT))

#define CONTROL_BITS(entity, control_name, bitshift)				\
	(UTIL_CAT(CONTROL_, CONTROL_TOKEN(entity, control_name)) << bitshift)

#define CONTROL_TOKEN_BY_IDX(entity, control_name, idx)				\
	COND_CODE_1(DT_PROP_HAS_IDX(entity, control_name, idx),			\
		(DT_STRING_UPPER_TOKEN_BY_IDX(entity, control_name, idx)),	\
		(NOT_PRESENT))

#define CONTROL_BITS_BY_IDX(entity, control_name, idx, bitshift)		\
	(UTIL_CAT(CONTROL_, CONTROL_TOKEN_BY_IDX(entity, control_name, idx))	\
		<< bitshift)

#define CLOCK_SOURCE_CONTROLS(entity)						\
	CONTROL_BITS(entity, frequency_control, 0)	|			\
	CONTROL_BITS(entity, validity_control, 2)

#define INPUT_TERMINAL_CONTROLS(entity)						\
	CONTROL_BITS(entity, copy_protect_control, 0)	|			\
	CONTROL_BITS(entity, connector_control, 2)	|			\
	CONTROL_BITS(entity, overload_control, 4)	|			\
	CONTROL_BITS(entity, cluster_control, 6)	|			\
	CONTROL_BITS(entity, underflow_control, 8)	|			\
	CONTROL_BITS(entity, overflow_control, 10)

#define OUTPUT_TERMINAL_CONTROLS(entity)					\
	CONTROL_BITS(entity, copy_protect_control, 0)	|			\
	CONTROL_BITS(entity, connector_control, 2)	|			\
	CONTROL_BITS(entity, overload_control, 4)	|			\
	CONTROL_BITS(entity, underflow_control, 6)	|			\
	CONTROL_BITS(entity, overflow_control, 8)

#define FEATURE_UNIT_CHANNEL_CONTROLS(entity, ch)				\
	CONTROL_BITS_BY_IDX(entity, mute_control, ch, 0)		|	\
	CONTROL_BITS_BY_IDX(entity, volume_control, ch, 2)		|	\
	CONTROL_BITS_BY_IDX(entity, bass_control, ch, 4)		|	\
	CONTROL_BITS_BY_IDX(entity, mid_control, ch, 6)			|	\
	CONTROL_BITS_BY_IDX(entity, treble_control, ch, 8)		|	\
	CONTROL_BITS_BY_IDX(entity, graphic_equalizer_control, ch, 10)	|	\
	CONTROL_BITS_BY_IDX(entity, automatic_gain_control, ch, 12)	|	\
	CONTROL_BITS_BY_IDX(entity, delay_control, ch, 14)		|	\
	CONTROL_BITS_BY_IDX(entity, bass_boost_control, ch, 16)		|	\
	CONTROL_BITS_BY_IDX(entity, loudness_control, ch, 18)		|	\
	CONTROL_BITS_BY_IDX(entity, input_gain_control, ch, 20)		|	\
	CONTROL_BITS_BY_IDX(entity, input_gain_pad_control, ch, 22)	|	\
	CONTROL_BITS_BY_IDX(entity, phase_inverter_control, ch, 24)	|	\
	CONTROL_BITS_BY_IDX(entity, underflow_control, ch, 26)		|	\
	CONTROL_BITS_BY_IDX(entity, overflow_control, ch, 28)

#define AUDIO_STREAMING_DATA_ENDPOINT_CONTROLS(node)				\
	CONTROL_BITS(node, pitch_control, 0)		|			\
	CONTROL_BITS(node, data_overrun_control, 2)	|			\
	CONTROL_BITS(node, data_underrun_control, 4)

/* 4.1 Audio Channel Cluster Descriptor */
#define SPATIAL_LOCATIONS_ARRAY(cluster)					\
	DT_PROP(cluster, front_left),						\
	DT_PROP(cluster, front_right),						\
	DT_PROP(cluster, front_center),						\
	DT_PROP(cluster, low_frequency_effects),				\
	DT_PROP(cluster, back_left),						\
	DT_PROP(cluster, back_right),						\
	DT_PROP(cluster, front_left_of_center),					\
	DT_PROP(cluster, front_right_of_center),				\
	DT_PROP(cluster, back_center),						\
	DT_PROP(cluster, side_left),						\
	DT_PROP(cluster, side_right),						\
	DT_PROP(cluster, top_center),						\
	DT_PROP(cluster, top_front_left),					\
	DT_PROP(cluster, top_front_center),					\
	DT_PROP(cluster, top_front_right),					\
	DT_PROP(cluster, top_back_left),					\
	DT_PROP(cluster, top_back_center),					\
	DT_PROP(cluster, top_back_right),					\
	DT_PROP(cluster, top_front_left_of_center),				\
	DT_PROP(cluster, top_front_right_of_center),				\
	DT_PROP(cluster, left_low_frequency_effects),				\
	DT_PROP(cluster, right_low_frequency_effects),				\
	DT_PROP(cluster, top_side_left),					\
	DT_PROP(cluster, top_side_right),					\
	DT_PROP(cluster, bottom_center),					\
	DT_PROP(cluster, back_left_of_center),					\
	DT_PROP(cluster, back_right_of_center),					\
	0, 0, 0, 0, /* D27..D30: Reserved */					\
	DT_PROP(cluster, raw_data)

#define SPATIAL_LOCATIONS_U32(entity) \
	(FOR_EACH_IDX(ARRAY_BIT, (|), SPATIAL_LOCATIONS_ARRAY(entity)))
#define NUM_SPATIAL_LOCATIONS(entity)						\
	NUM_VA_ARGS(LIST_DROP_EMPTY(						\
		FOR_EACH(EMPTY_ON_ZERO, (,), SPATIAL_LOCATIONS_ARRAY(entity))	\
	))
#define SPATIAL_LOCATIONS(entity) U32_LE(SPATIAL_LOCATIONS_U32(entity))

#define FEATURE_UNIT_NUM_CHANNELS(entity)					\
	NUM_SPATIAL_LOCATIONS(DT_PHANDLE_BY_IDX(entity, data_source, 0))

#define FEATURE_UNIT_CONTROLS_BY_IDX(i, entity)					\
	U32_LE(FEATURE_UNIT_CHANNEL_CONTROLS(entity, i))

#define FEATURE_UNIT_CONTROLS_ARRAYS(entity)					\
	LISTIFY(UTIL_INC(FEATURE_UNIT_NUM_CHANNELS(entity)),			\
		FEATURE_UNIT_CONTROLS_BY_IDX, (,), entity)

#define FEATURE_UNIT_DESCRIPTOR_LENGTH(entity)					\
	(6 + (FEATURE_UNIT_NUM_CHANNELS(entity) + 1) * 4)

/* 4.7.2.1 Clock Source Descriptor */
#define CLOCK_SOURCE_DESCRIPTOR(entity)						\
	0x08,						/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AC_DESCRIPTOR_CLOCK_SOURCE,			/* bDescriptorSubtype */\
	ENTITY_ID(entity),				/* bClockID */		\
	CLOCK_SOURCE_ATTRIBUTES(entity),		/* bmAttributes */	\
	CLOCK_SOURCE_CONTROLS(entity),			/* bmControls */	\
	CONNECTED_ENTITY_ID(entity, assoc_terminal),	/* bAssocTerminal */	\
	0x00,						/* iClockSource */

/* 4.7.2.4 Input Terminal Descriptor */
#define INPUT_TERMINAL_DESCRIPTOR(entity)					\
	0x11,						/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AC_DESCRIPTOR_INPUT_TERMINAL,			/* bDescriptorSubtype */\
	ENTITY_ID(entity),				/* bTerminalID */	\
	U16_LE(DT_PROP(entity, terminal_type)),		/* wTerminalType */	\
	ASSOCIATED_TERMINAL_ID(entity),			/* bAssocTerminal */	\
	CONNECTED_ENTITY_ID(entity, clock_source),	/* bCSourceID */	\
	NUM_SPATIAL_LOCATIONS(entity),			/* bNrChannels */	\
	SPATIAL_LOCATIONS(entity),			/* bmChannelConfig */	\
	0x00,						/* iChannelNames */	\
	U16_LE(INPUT_TERMINAL_CONTROLS(entity)),	/* bmControls */	\
	0x00,						/* iTerminal */

/* 4.7.2.5 Output Terminal Descriptor */
#define OUTPUT_TERMINAL_DESCRIPTOR(entity)					\
	0x0C,						/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AC_DESCRIPTOR_OUTPUT_TERMINAL,			/* bDescriptorSubtype */\
	ENTITY_ID(entity),				/* bTerminalID */	\
	U16_LE(DT_PROP(entity, terminal_type)),		/* wTerminalType */	\
	ASSOCIATED_TERMINAL_ID(entity),			/* bAssocTerminal */	\
	CONNECTED_ENTITY_ID(entity, data_source),	/* bSourceID */		\
	CONNECTED_ENTITY_ID(entity, clock_source),	/* bCSourceID */	\
	U16_LE(OUTPUT_TERMINAL_CONTROLS(entity)),	/* bmControls */	\
	0x00,						/* iTerminal */

/* 4.7.2.8 Feature Unit Descriptor */
#define FEATURE_UNIT_DESCRIPTOR(entity)						\
	FEATURE_UNIT_DESCRIPTOR_LENGTH(entity),		/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AC_DESCRIPTOR_FEATURE_UNIT,			/* bDescriptorSubtype */\
	ENTITY_ID(entity),				/* bUnitID */		\
	CONNECTED_ENTITY_ID(entity, data_source),	/* bSourceID */		\
	FEATURE_UNIT_CONTROLS_ARRAYS(entity),		/* bmaControls 0..ch */	\
	0x00,						/* iFeature */

#define ENTITY_HEADER(entity)							\
	IF_ENABLED(DT_NODE_HAS_COMPAT(entity, zephyr_uac2_clock_source), (	\
		CLOCK_SOURCE_DESCRIPTOR(entity)					\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(entity, zephyr_uac2_input_terminal), (	\
		INPUT_TERMINAL_DESCRIPTOR(entity)				\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(entity, zephyr_uac2_output_terminal), (	\
		OUTPUT_TERMINAL_DESCRIPTOR(entity)				\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(entity, zephyr_uac2_feature_unit), (	\
		FEATURE_UNIT_DESCRIPTOR(entity)					\
	))

#define ENTITY_HEADER_ARRAYS(entity)						\
	IF_ENABLED(UTIL_NOT(IS_EMPTY(ENTITY_HEADER(entity))), (			\
		static uint8_t DESCRIPTOR_NAME(ac_entity, entity)[] = {		\
			ENTITY_HEADER(entity)					\
		};								\
	))

#define ENTITY_HEADER_PTRS(entity)						\
	IF_ENABLED(UTIL_NOT(IS_EMPTY(ENTITY_HEADER(entity))), (			\
		(struct usb_desc_header *) &DESCRIPTOR_NAME(ac_entity, entity),	\
	))

#define ENTITY_HEADERS(node) DT_FOREACH_CHILD(node, ENTITY_HEADER)
#define ENTITY_HEADERS_ARRAYS(node) DT_FOREACH_CHILD(node, ENTITY_HEADER_ARRAYS)
#define ENTITY_HEADERS_PTRS(node) DT_FOREACH_CHILD(node, ENTITY_HEADER_PTRS)
#define ENTITY_HEADERS_LENGTH(node) sizeof((uint8_t []){ENTITY_HEADERS(node)})

#define AUDIO_STREAMING_CONTROLS(node)						\
	CONTROL_BITS(entity, active_alternate_setting_control, 0)	|	\
	CONTROL_BITS(entity, valid_alternate_settings_control, 2)

/* TODO: Support other format types. Currently the PCM format (0x00000001) is
 * hardcoded and format type is either I or IV depending on whether the
 * interface has isochronous endpoint or not.
 */
#define AUDIO_STREAMING_FORMAT_TYPE(node)					\
	COND_CODE_0(DT_PROP(node, external_interface),				\
		(FORMAT_TYPE_I), (FORMAT_TYPE_IV))
#define AUDIO_STREAMING_FORMATS(node) U32_LE(0x00000001)

#define FEATURE_UNIT_CHANNEL_CLUSTER(node)					\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_PROP(node, data_source),		\
		zephyr_uac2_input_terminal), (					\
			DT_PROP(node, data_source)				\
	))

/* Track back Output Terminal data source to entity that has channel cluster */
#define OUTPUT_TERMINAL_CHANNEL_CLUSTER(node)					\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_PROP(node, data_source),		\
		zephyr_uac2_input_terminal), (					\
			DT_PROP(node, data_source)				\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_PROP(node, data_source),		\
		zephyr_uac2_feature_unit), (					\
			FEATURE_UNIT_CHANNEL_CLUSTER(DT_PROP(node, data_source))\
	))

/* If AudioStreaming is linked to input terminal, obtain the channel cluster
 * configuration from the linked terminal. Otherwise (it has to be connected
 * to output terminal) obtain the channel cluster configuration from data source
 * entity.
 */
#define AUDIO_STREAMING_CHANNEL_CLUSTER(node)					\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_PROP(node, linked_terminal),		\
		zephyr_uac2_input_terminal), (					\
			DT_PROP(node, linked_terminal)				\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_PROP(node, linked_terminal),		\
		zephyr_uac2_output_terminal), (OUTPUT_TERMINAL_CHANNEL_CLUSTER(	\
			DT_PROP(node, linked_terminal))				\
	))

#define AUDIO_STREAMING_NUM_SPATIAL_LOCATIONS(node)				\
	NUM_SPATIAL_LOCATIONS(AUDIO_STREAMING_CHANNEL_CLUSTER(node))
#define AUDIO_STREAMING_SPATIAL_LOCATIONS(node)					\
	SPATIAL_LOCATIONS(AUDIO_STREAMING_CHANNEL_CLUSTER(node))

/* 4.9.2 Class-Specific AS Interface Descriptor */
#define AUDIO_STREAMING_GENERAL_DESCRIPTOR(node)				\
	0x10,						/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AS_DESCRIPTOR_GENERAL,				/* bDescriptorSubtype */\
	CONNECTED_ENTITY_ID(node, linked_terminal),	/* bTerminalLink */	\
	AUDIO_STREAMING_CONTROLS(node),			/* bmControls*/		\
	AUDIO_STREAMING_FORMAT_TYPE(node),		/* bFormatType */	\
	AUDIO_STREAMING_FORMATS(node),			/* bmFormats */		\
	AUDIO_STREAMING_NUM_SPATIAL_LOCATIONS(node),    /* bNrChannels */	\
	AUDIO_STREAMING_SPATIAL_LOCATIONS(node),	/* bmChannelConfig */	\
	0x00,						/* iChannelNames */

/* Universal Serial Bus Device Class Definition for Audio Data Formats
 * Release 2.0, May 31, 2006. 2.3.1.6 Type I Format Type Descriptor
 */
#define AUDIO_STREAMING_FORMAT_I_TYPE_DESCRIPTOR(node)				\
	0x06,						/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AS_DESCRIPTOR_FORMAT_TYPE,			/* bDescriptorSubtype */\
	FORMAT_TYPE_I,					/* bFormatType */	\
	DT_PROP(node, subslot_size),			/* bSubslotSize */	\
	DT_PROP(node, bit_resolution),			/* bBitResolution */

/* Universal Serial Bus Device Class Definition for Audio Data Formats
 * Release 2.0, May 31, 2006. 2.3.4.1 Type IV Format Type Descriptor
 */
#define AUDIO_STREAMING_FORMAT_IV_TYPE_DESCRIPTOR(node)				\
	0x04,						/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AS_DESCRIPTOR_FORMAT_TYPE,			/* bDescriptorSubtype */\
	FORMAT_TYPE_IV,					/* bFormatType */

/* 4.9.3 Class-Specific AS Format Type Descriptor */
#define AUDIO_STREAMING_FORMAT_TYPE_DESCRIPTOR(node)				\
	IF_ENABLED(IS_EQ(AUDIO_STREAMING_FORMAT_TYPE(node), FORMAT_TYPE_I), (	\
		AUDIO_STREAMING_FORMAT_I_TYPE_DESCRIPTOR(node)))		\
	IF_ENABLED(IS_EQ(AUDIO_STREAMING_FORMAT_TYPE(node), FORMAT_TYPE_IV), (	\
		AUDIO_STREAMING_FORMAT_IV_TYPE_DESCRIPTOR(node)))

#define AUDIO_STREAMING_INTERFACE_DESCRIPTORS_ARRAYS(node)			\
	static uint8_t DESCRIPTOR_NAME(as_general_desc, node)[] = {		\
		AUDIO_STREAMING_GENERAL_DESCRIPTOR(node)			\
	};									\
	static uint8_t DESCRIPTOR_NAME(as_format_desc, node)[] = {		\
		AUDIO_STREAMING_FORMAT_TYPE_DESCRIPTOR(node)			\
	};

/* Full and High speed share common class-specific interface descriptors */
#define AUDIO_STREAMING_INTERFACE_DESCRIPTORS_PTRS(node)			\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(as_general_desc, node),	\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(as_format_desc, node),

/* 4.7.2 Class-Specific AC Interface Descriptor */
#define AC_INTERFACE_HEADER_DESCRIPTOR(node)					\
	0x09,						/* bLength */		\
	CS_INTERFACE,					/* bDescriptorType */	\
	AC_DESCRIPTOR_HEADER,				/* bDescriptorSubtype */\
	U16_LE(0x0200),					/* bcdADC */		\
	DT_PROP(node, audio_function),			/* bCategory */		\
	U16_LE(9 + ENTITY_HEADERS_LENGTH(node)),	/* wTotalLength */	\
	0x00,						/* bmControls */	\

#define AC_INTERFACE_HEADER_DESCRIPTOR_ARRAY(node)				\
	static uint8_t DESCRIPTOR_NAME(ac_header, node)[] = {			\
		AC_INTERFACE_HEADER_DESCRIPTOR(node)				\
	};

#define AC_INTERFACE_HEADER_DESCRIPTOR_PTR(node)				\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(ac_header, node),

#define IS_AUDIOSTREAMING_INTERFACE(node)					\
	DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming)

#define UAC2_NUM_INTERFACES(node)						\
	1 /* AudioControl interface */ +					\
	DT_FOREACH_CHILD_SEP(node, IS_AUDIOSTREAMING_INTERFACE, (+))

#define UAC2_ALLOWED_AT_FULL_SPEED(node)					\
	DT_PROP(node, full_speed)

#define UAC2_ALLOWED_AT_HIGH_SPEED(node)					\
	DT_PROP(node, high_speed)

/* 4.6 Interface Association Descriptor */
#define UAC2_INTERFACE_ASSOCIATION_DESCRIPTOR(node)				\
	0x08,						/* bLength */		\
	USB_DESC_INTERFACE_ASSOC,			/* bDescriptorType */	\
	FIRST_INTERFACE_NUMBER,				/* bFirstInterface */	\
	UAC2_NUM_INTERFACES(node),			/* bInterfaceCount */	\
	AUDIO_FUNCTION,					/* bFunctionClass */	\
	FUNCTION_SUBCLASS_UNDEFINED,			/* bFunctionSubclass */ \
	AF_VERSION_02_00,				/* bFunctionProtocol */	\
	0x00,						/* iFunction */

#define UAC2_INTERFACE_ASSOCIATION_DESCRIPTOR_ARRAY(node)			\
	IF_ENABLED(UAC2_ALLOWED_AT_FULL_SPEED(node), (				\
		static uint8_t DESCRIPTOR_NAME(fs_iad, node)[] = {		\
			UAC2_INTERFACE_ASSOCIATION_DESCRIPTOR(node)		\
		};								\
	))									\
	IF_ENABLED(UAC2_ALLOWED_AT_HIGH_SPEED(node), (				\
		static uint8_t DESCRIPTOR_NAME(hs_iad, node)[] = {		\
			UAC2_INTERFACE_ASSOCIATION_DESCRIPTOR(node)		\
		};								\
	))

#define UAC2_INTERFACE_ASSOCIATION_FS_DESCRIPTOR_PTR(node)			\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(fs_iad, node),

#define UAC2_INTERFACE_ASSOCIATION_HS_DESCRIPTOR_PTR(node)			\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(hs_iad, node),

/* 4.7.1 Standard AC Interface Descriptor */
#define AC_INTERFACE_DESCRIPTOR(node)						\
	0x09,						/* bLength */		\
	USB_DESC_INTERFACE,				/* bDescriptorType */	\
	FIRST_INTERFACE_NUMBER,				/* bInterfaceNumber */	\
	0x00,						/* bAlternateSetting */ \
	DT_PROP(node, interrupt_endpoint),		/* bNumEndpoints */	\
	AUDIO,						/* bInterfaceClass */	\
	AUDIOCONTROL,					/* bInterfaceSubClass */\
	IP_VERSION_02_00,				/* bInterfaceProtocol */\
	0x00,						/* iInterface */

#define AC_INTERFACE_DESCRIPTOR_ARRAY(node)					\
	IF_ENABLED(UAC2_ALLOWED_AT_FULL_SPEED(node), (				\
		static uint8_t DESCRIPTOR_NAME(fs_ac_interface, node)[] = {	\
			AC_INTERFACE_DESCRIPTOR(node)				\
		};								\
	))									\
	IF_ENABLED(UAC2_ALLOWED_AT_HIGH_SPEED(node), (				\
		static uint8_t DESCRIPTOR_NAME(hs_ac_interface, node)[] = {	\
			AC_INTERFACE_DESCRIPTOR(node)				\
		};								\
	))

#define AC_INTERFACE_FS_DESCRIPTOR_PTR(node)					\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(fs_ac_interface, node),

#define AC_INTERFACE_HS_DESCRIPTOR_PTR(node)					\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(hs_ac_interface, node),

/* 4.8.2.1 Standard AC Interrupt Endpoint Descriptor */
#define AC_ENDPOINT_DESCRIPTOR(node)						\
	0x07,						/* bLength */		\
	USB_DESC_ENDPOINT,				/* bDescriptorType */	\
	FIRST_IN_EP_ADDR,				/* bEndpointAddress */	\
	USB_EP_TYPE_INTERRUPT,				/* bmAttributes */	\
	U16_LE(0x06),					/* wMaxPacketSize */	\
	0x01,						/* bInterval */		\

#define AC_ENDPOINT_DESCRIPTOR_ARRAY(node)					\
	static uint8_t DESCRIPTOR_NAME(ac_endpoint, node)[] = {			\
		AC_ENDPOINT_DESCRIPTOR(node)					\
	};

#define AC_ENDPOINT_DESCRIPTOR_PTR(node)					\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(ac_endpoint, node),

#define FIND_AUDIOSTREAMING(node, fn, ...)					\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		fn(node, __VA_ARGS__)))

#define FOR_EACH_AUDIOSTREAMING_INTERFACE(node, fn, ...)			\
	DT_FOREACH_CHILD_VARGS(node, FIND_AUDIOSTREAMING, fn, __VA_ARGS__)

#define COUNT_AS_INTERFACES_BEFORE_IDX(node, idx)				\
	+ 1 * (DT_NODE_CHILD_IDX(node) < idx)

#define AS_INTERFACE_NUMBER(node)						\
	FIRST_INTERFACE_NUMBER + 1 /* AudioControl interface */	+		\
	FOR_EACH_AUDIOSTREAMING_INTERFACE(DT_PARENT(node),			\
		COUNT_AS_INTERFACES_BEFORE_IDX, DT_NODE_CHILD_IDX(node))

#define AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node)					\
	UTIL_NOT(DT_PROP(node, external_interface))

#define AS_IS_USB_ISO_IN(node)							\
	UTIL_AND(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node),			\
		DT_NODE_HAS_COMPAT(DT_PROP(node, linked_terminal),		\
			zephyr_uac2_output_terminal))

#define AS_IS_USB_ISO_OUT(node)							\
	UTIL_AND(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node),			\
		DT_NODE_HAS_COMPAT(DT_PROP(node, linked_terminal),		\
			zephyr_uac2_input_terminal))

#define CLK_IS_SOF_SYNCHRONIZED(entity)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(entity, zephyr_uac2_clock_source), (	\
		DT_PROP(entity, sof_synchronized)				\
	))

/* Sampling frequencies are sorted (asserted at compile time), so just grab
 * last sampling frequency.
 */
#define CLK_MAX_FREQUENCY(entity)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(entity, zephyr_uac2_clock_source), (	\
		DT_PROP_BY_IDX(entity, sampling_frequencies,			\
			UTIL_DEC(DT_PROP_LEN(entity, sampling_frequencies)))	\
	))

#define AS_CLK_SOURCE(node)							\
	DT_PROP(DT_PROP(node, linked_terminal), clock_source)

#define AS_CLK_MAX_FREQUENCY(node)						\
	CLK_MAX_FREQUENCY(AS_CLK_SOURCE(node))

#define AS_IS_SOF_SYNCHRONIZED(node)						\
	CLK_IS_SOF_SYNCHRONIZED(AS_CLK_SOURCE(node))

#define AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node)					\
	UTIL_AND(UTIL_AND(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node),		\
			UTIL_NOT(DT_PROP(node, implicit_feedback))),		\
		UTIL_AND(UTIL_NOT(AS_IS_SOF_SYNCHRONIZED(node)),		\
			AS_IS_USB_ISO_OUT(node)))

#define AS_INTERFACE_NUM_ENDPOINTS(node)					\
	(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node) +				\
	AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node))

/* 4.9.1 Standard AS Interface Descriptor */
#define AS_INTERFACE_DESCRIPTOR(node, alternate, numendpoints)			\
	0x09,						/* bLength */		\
	USB_DESC_INTERFACE,				/* bDescriptorType */	\
	AS_INTERFACE_NUMBER(node),			/* bInterfaceNumber */	\
	alternate,					/* bAlternateSetting */	\
	numendpoints,					/* bNumEndpoints */	\
	AUDIO,						/* bInterfaceClass */	\
	AUDIOSTREAMING,					/* bInterfaceSubClass */\
	IP_VERSION_02_00,				/* bInterfaceProtocol */\
	0x00,						/* iInterface */

#define AS_INTERFACE_FS_DESCRIPTOR_ARRAY(node, alternate, numendpoints)		\
	static uint8_t DESCRIPTOR_NAME(fs_as_if_alt##alternate, node)[] = {	\
		AS_INTERFACE_DESCRIPTOR(node, alternate, numendpoints)		\
	};

#define AS_INTERFACE_HS_DESCRIPTOR_ARRAY(node, alternate, numendpoints)		\
	static uint8_t DESCRIPTOR_NAME(hs_as_if_alt##alternate, node)[] = {	\
		AS_INTERFACE_DESCRIPTOR(node, alternate, numendpoints)		\
	};

#define AS_INTERFACE_DESCRIPTOR_ARRAY(node, alternate, numendpoints)		\
	IF_ENABLED(UAC2_ALLOWED_AT_FULL_SPEED(DT_PARENT(node)), (		\
		AS_INTERFACE_FS_DESCRIPTOR_ARRAY(node, alternate, numendpoints)	\
	))									\
	IF_ENABLED(UAC2_ALLOWED_AT_HIGH_SPEED(DT_PARENT(node)), (		\
		AS_INTERFACE_HS_DESCRIPTOR_ARRAY(node, alternate, numendpoints)	\
	))

#define AS_INTERFACE_FS_DESCRIPTOR_PTR(node, altnum)				\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(fs_as_if_alt##altnum, node),

#define AS_INTERFACE_HS_DESCRIPTOR_PTR(node, altnum)				\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(hs_as_if_alt##altnum, node),

#define COUNT_AS_OUT_ENDPOINTS_BEFORE_IDX(node, idx)				\
	+ AS_IS_USB_ISO_OUT(node) * (DT_NODE_CHILD_IDX(node) < idx)

#define COUNT_AS_IN_ENDPOINTS_BEFORE_IDX(node, idx)				\
	+ (AS_IS_USB_ISO_IN(node) + AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node))	\
		* (DT_NODE_CHILD_IDX(node) < idx)

/* FIXME: Ensure that the explicit feedback endpoints assignments match
 * numbering requirements from Universal Serial Bus Specification Revision 2.0
 * 9.6.6 Endpoint. This FIXME is not limited to the macros here but also to
 * actual USB stack endpoint fixup (so the fixup does not break the numbering).
 *
 * This FIXME does not affect nRF52 and nRF53 when implicit feedback is used
 * because the endpoints after fixup will be 0x08 and 0x88 because there are
 * only two isochronous endpoints on these devices (one IN, one OUT).
 */
#define AS_NEXT_OUT_EP_ADDR(node)						\
	FIRST_OUT_EP_ADDR +							\
	FOR_EACH_AUDIOSTREAMING_INTERFACE(DT_PARENT(node),			\
		COUNT_AS_OUT_ENDPOINTS_BEFORE_IDX, DT_NODE_CHILD_IDX(node))

#define AS_NEXT_IN_EP_ADDR(node)						\
	FIRST_IN_EP_ADDR + DT_PROP(DT_PARENT(node), interrupt_endpoint) +	\
	FOR_EACH_AUDIOSTREAMING_INTERFACE(DT_PARENT(node),			\
		COUNT_AS_IN_ENDPOINTS_BEFORE_IDX, DT_NODE_CHILD_IDX(node))

#define AS_DATA_EP_ADDR(node)							\
	COND_CODE_1(AS_IS_USB_ISO_OUT(node),					\
		(AS_NEXT_OUT_EP_ADDR(node)),					\
		(AS_NEXT_IN_EP_ADDR(node)))

#define AS_BYTES_PER_SAMPLE(node)						\
	DT_PROP(node, subslot_size)

#define AS_FS_DATA_EP_BINTERVAL(node)						\
	USB_FS_ISO_EP_INTERVAL(DT_PROP_OR(node, polling_period_us, 1000))

#define AS_HS_DATA_EP_BINTERVAL(node)						\
	USB_HS_ISO_EP_INTERVAL(DT_PROP_OR(node, polling_period_us, 125))

/* Asynchronous endpoints needs space for 1 extra sample */
#define AS_SAMPLES_PER_FRAME(node)						\
	(((ROUND_UP(AS_CLK_MAX_FREQUENCY(node), 1000) / 1000)			\
	  << (AS_FS_DATA_EP_BINTERVAL(node) - 1)) +				\
	 UTIL_NOT(AS_IS_SOF_SYNCHRONIZED(node)))

#define AS_SAMPLES_PER_MICROFRAME(node)						\
	(((ROUND_UP(AS_CLK_MAX_FREQUENCY(node), 8000) / 8000)			\
	  << (AS_HS_DATA_EP_BINTERVAL(node) - 1)) +				\
	 UTIL_NOT(AS_IS_SOF_SYNCHRONIZED(node)))

#define AS_DATA_EP_SYNC_TYPE(node)						\
	COND_CODE_1(AS_IS_SOF_SYNCHRONIZED(node), (0x3 << 2), (0x1 << 2))

#define AS_DATA_EP_USAGE_TYPE(node)						\
	COND_CODE_1(UTIL_AND(DT_PROP(node, implicit_feedback),			\
		UTIL_NOT(AS_IS_USB_ISO_OUT(node))), (0x2 << 4), (0x0 << 4))

#define AS_DATA_EP_ATTR(node)							\
	USB_EP_TYPE_ISO | AS_DATA_EP_SYNC_TYPE(node) |				\
	AS_DATA_EP_USAGE_TYPE(node)

#define AS_FS_DATA_EP_MAX_PACKET_SIZE(node)					\
	AUDIO_STREAMING_NUM_SPATIAL_LOCATIONS(node) *				\
	AS_BYTES_PER_SAMPLE(node) * AS_SAMPLES_PER_FRAME(node)

#define AS_HS_DATA_EP_TPL(node)							\
	USB_TPL_ROUND_UP(AUDIO_STREAMING_NUM_SPATIAL_LOCATIONS(node) *		\
		AS_BYTES_PER_SAMPLE(node) * AS_SAMPLES_PER_MICROFRAME(node))

#define AS_HS_DATA_EP_MAX_PACKET_SIZE(node)					\
	USB_TPL_TO_MPS(AS_HS_DATA_EP_TPL(node))

/* 4.10.1.1 Standard AS Isochronous Audio Data Endpoint Descriptor */
#define STANDARD_AS_ISOCHRONOUS_DATA_ENDPOINT_FS_DESCRIPTOR(node)		\
	0x07,						/* bLength */		\
	USB_DESC_ENDPOINT,				/* bDescriptorType */	\
	AS_DATA_EP_ADDR(node),				/* bEndpointAddress */	\
	AS_DATA_EP_ATTR(node),				/* bmAttributes */	\
	U16_LE(AS_FS_DATA_EP_MAX_PACKET_SIZE(node)),	/* wMaxPacketSize */	\
	AS_FS_DATA_EP_BINTERVAL(node),			/* bInterval */

#define AS_ISOCHRONOUS_DATA_ENDPOINT_FS_DESCRIPTORS_ARRAYS(node)		\
	static uint8_t DESCRIPTOR_NAME(fs_std_data_ep, node)[] = {		\
		STANDARD_AS_ISOCHRONOUS_DATA_ENDPOINT_FS_DESCRIPTOR(node)	\
	};

#define STANDARD_AS_ISOCHRONOUS_DATA_ENDPOINT_HS_DESCRIPTOR(node)		\
	0x07,						/* bLength */		\
	USB_DESC_ENDPOINT,				/* bDescriptorType */	\
	AS_DATA_EP_ADDR(node),				/* bEndpointAddress */	\
	AS_DATA_EP_ATTR(node),				/* bmAttributes */	\
	U16_LE(AS_HS_DATA_EP_MAX_PACKET_SIZE(node)),	/* wMaxPacketSize */	\
	AS_HS_DATA_EP_BINTERVAL(node),			/* bInterval */

#define AS_ISOCHRONOUS_DATA_ENDPOINT_HS_DESCRIPTORS_ARRAYS(node)		\
	static uint8_t DESCRIPTOR_NAME(hs_std_data_ep, node)[] = {		\
		STANDARD_AS_ISOCHRONOUS_DATA_ENDPOINT_HS_DESCRIPTOR(node)	\
	};

#define LOCK_DELAY_UNITS(node)							\
	COND_CODE_1(DT_NODE_HAS_PROP(node, lock_delay_units),			\
		(1 + DT_ENUM_IDX(node, lock_delay_units)),			\
		(0 /* Undefined */))

/* 4.10.1.2 Class-Specific AS Isochronous Audio Data Endpoint Descriptor */
#define CLASS_SPECIFIC_AS_ISOCHRONOUS_DATA_ENDPOINT_DESCRIPTOR(node)		\
	0x08,						/* bLength */		\
	CS_ENDPOINT,					/* bDescriptorType */	\
	EP_GENERAL,					/* bDescriptorSubtype */\
	0x00,						/* bmAttributes */	\
	AUDIO_STREAMING_DATA_ENDPOINT_CONTROLS(node),	/* bmControls */	\
	LOCK_DELAY_UNITS(node),				/* bLockDelayUnits */	\
	U16_LE(DT_PROP_OR(node, lock_delay, 0)),	/* wLockDelay */

/* Full and High speed share common class-specific descriptor */
#define AS_ISOCHRONOUS_DATA_ENDPOINT_CS_DESCRIPTORS_ARRAYS(node)		\
	static uint8_t DESCRIPTOR_NAME(cs_data_ep, node)[] = {			\
		CLASS_SPECIFIC_AS_ISOCHRONOUS_DATA_ENDPOINT_DESCRIPTOR(node)	\
	};

#define AS_ISOCHRONOUS_DATA_ENDPOINT_FS_DESCRIPTORS_PTRS(node)			\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(fs_std_data_ep, node),	\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(cs_data_ep, node),

#define AS_ISOCHRONOUS_DATA_ENDPOINT_HS_DESCRIPTORS_PTRS(node)			\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(hs_std_data_ep, node),	\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(cs_data_ep, node),

#define AS_EXPLICIT_FEEDBACK_ENDPOINT_FS_DESCRIPTOR(node)			\
	0x07,						/* bLength */		\
	USB_DESC_ENDPOINT,				/* bDescriptorType */	\
	AS_NEXT_IN_EP_ADDR(node),			/* bEndpointAddress */	\
	0x11,						/* bmAttributes */	\
	U16_LE(0x03),					/* wMaxPacketSize */	\
	0x01, /* TODO: adjust to P 5.12.4.2 Feedback */	/* bInterval */

#define AS_EXPLICIT_FEEDBACK_FS_DESCRIPTOR_ARRAY(node)				\
	static uint8_t DESCRIPTOR_NAME(fs_feedback_ep, node)[] = {		\
		AS_EXPLICIT_FEEDBACK_ENDPOINT_FS_DESCRIPTOR(node)		\
	};

#define AS_EXPLICIT_FEEDBACK_ENDPOINT_FS_DESCRIPTOR_PTR(node)			\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(fs_feedback_ep, node),

#define AS_EXPLICIT_FEEDBACK_ENDPOINT_HS_DESCRIPTOR(node)			\
	0x07,						/* bLength */		\
	USB_DESC_ENDPOINT,				/* bDescriptorType */	\
	AS_NEXT_IN_EP_ADDR(node),			/* bEndpointAddress */	\
	0x11,						/* bmAttributes */	\
	U16_LE(0x04),					/* wMaxPacketSize */	\
	0x01, /* TODO: adjust to P 5.12.4.2 Feedback */	/* bInterval */

#define AS_EXPLICIT_FEEDBACK_HS_DESCRIPTOR_ARRAY(node)				\
	static uint8_t DESCRIPTOR_NAME(hs_feedback_ep, node)[] = {		\
		AS_EXPLICIT_FEEDBACK_ENDPOINT_HS_DESCRIPTOR(node)		\
	};

#define AS_EXPLICIT_FEEDBACK_ENDPOINT_HS_DESCRIPTOR_PTR(node)			\
	(struct usb_desc_header *) &DESCRIPTOR_NAME(hs_feedback_ep, node),

#define AS_FS_DESCRIPTORS_ARRAYS(node)						\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_ISOCHRONOUS_DATA_ENDPOINT_FS_DESCRIPTORS_ARRAYS(node)	\
		IF_ENABLED(AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node), (		\
			AS_EXPLICIT_FEEDBACK_FS_DESCRIPTOR_ARRAY(node)))	\
	))

#define AS_HS_DESCRIPTORS_ARRAYS(node)						\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_ISOCHRONOUS_DATA_ENDPOINT_HS_DESCRIPTORS_ARRAYS(node)	\
		IF_ENABLED(AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node), (		\
			AS_EXPLICIT_FEEDBACK_HS_DESCRIPTOR_ARRAY(node)))	\
	))

#define AS_DESCRIPTORS_ARRAYS(node)						\
	AS_INTERFACE_DESCRIPTOR_ARRAY(node, 0, 0)				\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_INTERFACE_DESCRIPTOR_ARRAY(node, 1,				\
			AS_INTERFACE_NUM_ENDPOINTS(node))))			\
	AUDIO_STREAMING_INTERFACE_DESCRIPTORS_ARRAYS(node)			\
	IF_ENABLED(UAC2_ALLOWED_AT_FULL_SPEED(DT_PARENT(node)), (		\
		AS_FS_DESCRIPTORS_ARRAYS(node)))				\
	IF_ENABLED(UAC2_ALLOWED_AT_HIGH_SPEED(DT_PARENT(node)), (		\
		AS_HS_DESCRIPTORS_ARRAYS(node)))				\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_ISOCHRONOUS_DATA_ENDPOINT_CS_DESCRIPTORS_ARRAYS(node)))

#define AS_FS_DESCRIPTORS_PTRS(node)						\
	AS_INTERFACE_FS_DESCRIPTOR_PTR(node, 0)					\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_INTERFACE_FS_DESCRIPTOR_PTR(node, 1)))			\
	AUDIO_STREAMING_INTERFACE_DESCRIPTORS_PTRS(node)			\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_ISOCHRONOUS_DATA_ENDPOINT_FS_DESCRIPTORS_PTRS(node)		\
		IF_ENABLED(AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node), (		\
			AS_EXPLICIT_FEEDBACK_ENDPOINT_FS_DESCRIPTOR_PTR(node)))	\
	))

#define AS_HS_DESCRIPTORS_PTRS(node)						\
	AS_INTERFACE_HS_DESCRIPTOR_PTR(node, 0)					\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_INTERFACE_HS_DESCRIPTOR_PTR(node, 1)))			\
	AUDIO_STREAMING_INTERFACE_DESCRIPTORS_PTRS(node)			\
	IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (			\
		AS_ISOCHRONOUS_DATA_ENDPOINT_HS_DESCRIPTORS_PTRS(node)		\
		IF_ENABLED(AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node), (		\
			AS_EXPLICIT_FEEDBACK_ENDPOINT_HS_DESCRIPTOR_PTR(node)))	\
	))

#define AS_DESCRIPTORS_ARRAYS_IF_AUDIOSTREAMING(node)				\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		AS_DESCRIPTORS_ARRAYS(node)))

#define AS_FS_DESCRIPTORS_PTRS_IF_AUDIOSTREAMING(node)				\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		AS_FS_DESCRIPTORS_PTRS(node)))

#define AS_HS_DESCRIPTORS_PTRS_IF_AUDIOSTREAMING(node)				\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		AS_HS_DESCRIPTORS_PTRS(node)))

#define UAC2_AUDIO_CONTROL_DESCRIPTOR_ARRAYS(node)				\
	AC_INTERFACE_DESCRIPTOR_ARRAY(node)					\
	AC_INTERFACE_HEADER_DESCRIPTOR_ARRAY(node)				\
	ENTITY_HEADERS_ARRAYS(node)						\
	IF_ENABLED(DT_PROP(node, interrupt_endpoint), (				\
		AC_ENDPOINT_DESCRIPTOR_ARRAY(node)))

#define UAC2_AUDIO_CONTROL_COMMON_DESCRIPTOR_PTRS(node)				\
	AC_INTERFACE_HEADER_DESCRIPTOR_PTR(node)				\
	ENTITY_HEADERS_PTRS(node)						\
	IF_ENABLED(DT_PROP(node, interrupt_endpoint), (				\
		AC_ENDPOINT_DESCRIPTOR_PTR(node)))

#define UAC2_AUDIO_CONTROL_FS_DESCRIPTOR_PTRS(node)				\
	AC_INTERFACE_FS_DESCRIPTOR_PTR(node)					\
	UAC2_AUDIO_CONTROL_COMMON_DESCRIPTOR_PTRS(node)

#define UAC2_AUDIO_CONTROL_HS_DESCRIPTOR_PTRS(node)				\
	AC_INTERFACE_HS_DESCRIPTOR_PTR(node)					\
	UAC2_AUDIO_CONTROL_COMMON_DESCRIPTOR_PTRS(node)

#define UAC2_DESCRIPTOR_ARRAYS(node)						\
	UAC2_INTERFACE_ASSOCIATION_DESCRIPTOR_ARRAY(node)			\
	UAC2_AUDIO_CONTROL_DESCRIPTOR_ARRAYS(node)				\
	DT_FOREACH_CHILD(node, AS_DESCRIPTORS_ARRAYS_IF_AUDIOSTREAMING)

#define UAC2_FS_DESCRIPTOR_PTRS(node)						\
	UAC2_INTERFACE_ASSOCIATION_FS_DESCRIPTOR_PTR(node)			\
	UAC2_AUDIO_CONTROL_FS_DESCRIPTOR_PTRS(node)				\
	DT_FOREACH_CHILD(node, AS_FS_DESCRIPTORS_PTRS_IF_AUDIOSTREAMING)	\
	NULL

#define UAC2_HS_DESCRIPTOR_PTRS(node)						\
	UAC2_INTERFACE_ASSOCIATION_HS_DESCRIPTOR_PTR(node)			\
	UAC2_AUDIO_CONTROL_HS_DESCRIPTOR_PTRS(node)				\
	DT_FOREACH_CHILD(node, AS_HS_DESCRIPTORS_PTRS_IF_AUDIOSTREAMING)	\
	NULL

#define UAC2_FS_DESCRIPTOR_PTRS_ARRAY(node)					\
	COND_CODE_1(UAC2_ALLOWED_AT_FULL_SPEED(node),				\
		({UAC2_FS_DESCRIPTOR_PTRS(node)}), ({NULL}))

#define UAC2_HS_DESCRIPTOR_PTRS_ARRAY(node)					\
	COND_CODE_1(UAC2_ALLOWED_AT_HIGH_SPEED(node),				\
		({UAC2_HS_DESCRIPTOR_PTRS(node)}), ({NULL}))

/* Helper macros to determine endpoint descriptor offset within descriptor set */
#define COUNT_PTRS(...) sizeof((struct usb_desc_header *[]){__VA_ARGS__})	\
	/ sizeof(struct usb_desc_header *)

#define COUNT_AS_DESCRIPTORS_UP_TO_IDX(node, idx)				\
	(COUNT_PTRS(COND_CODE_1(UAC2_ALLOWED_AT_FULL_SPEED(DT_PARENT(node)),	\
		(AS_FS_DESCRIPTORS_PTRS_IF_AUDIOSTREAMING(node)),		\
		(AS_HS_DESCRIPTORS_PTRS_IF_AUDIOSTREAMING(node))))) *		\
	(DT_NODE_CHILD_IDX(node) <= idx)

#define UAC2_DESCRIPTOR_AS_DESC_END_COUNT(node)					\
	(COUNT_PTRS(COND_CODE_1(UAC2_ALLOWED_AT_FULL_SPEED(DT_PARENT(node)), (	\
		UAC2_INTERFACE_ASSOCIATION_FS_DESCRIPTOR_PTR(DT_PARENT(node))	\
		UAC2_AUDIO_CONTROL_FS_DESCRIPTOR_PTRS(DT_PARENT(node))		\
	), (									\
		UAC2_INTERFACE_ASSOCIATION_HS_DESCRIPTOR_PTR(DT_PARENT(node))	\
		UAC2_AUDIO_CONTROL_HS_DESCRIPTOR_PTRS(DT_PARENT(node))		\
	)))) + DT_FOREACH_CHILD_SEP_VARGS(DT_PARENT(node),			\
		COUNT_AS_DESCRIPTORS_UP_TO_IDX, (+),				\
		DT_NODE_CHILD_IDX(node))

#define AS_ISOCHRONOUS_DATA_ENDPOINT_DESCRIPTORS_COUNT(node)			\
	COUNT_PTRS(COND_CODE_1(UAC2_ALLOWED_AT_FULL_SPEED(DT_PARENT(node)), (	\
		AS_ISOCHRONOUS_DATA_ENDPOINT_FS_DESCRIPTORS_PTRS(node)		\
	), (									\
		AS_ISOCHRONOUS_DATA_ENDPOINT_HS_DESCRIPTORS_PTRS(node)		\
	)))

#define AS_EXPLICIT_FEEDBACK_ENDPOINT_DESCRIPTOR_COUNT(node)			\
	COND_CODE_1(AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node), (COUNT_PTRS(	\
		COND_CODE_1(UAC2_ALLOWED_AT_FULL_SPEED(DT_PARENT(node)), (	\
			AS_EXPLICIT_FEEDBACK_ENDPOINT_FS_DESCRIPTOR_PTR(node)	\
		), (								\
			AS_EXPLICIT_FEEDBACK_ENDPOINT_HS_DESCRIPTOR_PTR(node)	\
		))								\
	)), (0))

/* Return index inside UAC2_FS_DESCRIPTOR_PTRS(DT_PARENT(node)) and/or
 * UAC2_HS_DESCRIPTOR_PTRS(DT_PARENT(node)) pointing to data endpoint
 * descriptor belonging to given AudioStreaming interface node.
 *
 * It is programmer error to call this macro with node other than AudioStreaming
 * or when AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node) is 0.
 */
#define UAC2_DESCRIPTOR_AS_DATA_EP_INDEX(node)					\
	UAC2_DESCRIPTOR_AS_DESC_END_COUNT(node)					\
	- AS_EXPLICIT_FEEDBACK_ENDPOINT_DESCRIPTOR_COUNT(node)			\
	- AS_ISOCHRONOUS_DATA_ENDPOINT_DESCRIPTORS_COUNT(node)

/* Return index inside UAC2_FS_DESCRIPTOR_PTRS(DT_PARENT(node)) and/or
 * UAC2_HS_DESCRIPTOR_PTRS(DT_PARENT(node)) pointing to feedback endpoint
 * descriptor belonging to given AudioStreaming interface node.
 *
 * It is programmer error to call this macro with node other than AudioStreaming
 * or when AS_HAS_EXPLICIT_FEEDBACK_ENDPOINT(node) is 0.
 */
#define UAC2_DESCRIPTOR_AS_FEEDBACK_EP_INDEX(node)				\
	UAC2_DESCRIPTOR_AS_DESC_END_COUNT(node)					\
	- AS_EXPLICIT_FEEDBACK_ENDPOINT_DESCRIPTOR_COUNT(node)

/* Helper macros to validate USB Audio Class 2 devicetree entries.
 * Macros above do rely on the assumptions checked below.
 */
#define VALIDATE_INPUT_TERMINAL_ASSOCIATION(entity)				\
	UTIL_OR(UTIL_NOT(DT_NODE_HAS_PROP(entity, assoc_terminal)),		\
		DT_NODE_HAS_COMPAT(DT_PROP(entity, assoc_terminal),		\
			zephyr_uac2_output_terminal))

#define VALIDATE_OUTPUT_TERMINAL_ASSOCIATION(entity)				\
	UTIL_OR(UTIL_NOT(DT_NODE_HAS_PROP(entity, assoc_terminal)),		\
		DT_NODE_HAS_COMPAT(DT_PROP(entity, assoc_terminal),		\
			zephyr_uac2_input_terminal))

#define VALIDATE_OUTPUT_TERMINAL_DATA_SOURCE(entity)				\
	UTIL_OR(DT_NODE_HAS_COMPAT(DT_PROP(entity, data_source),		\
			zephyr_uac2_input_terminal),				\
		DT_NODE_HAS_COMPAT(DT_PROP(entity, data_source),		\
			zephyr_uac2_feature_unit))

#define VALIDATE_FEATURE_UNIT_DATA_SOURCE(entity)				\
	DT_NODE_HAS_COMPAT(DT_PROP(entity, data_source),			\
		zephyr_uac2_input_terminal)

#define BUILD_ASSERT_FEATURE_UNIT_CONTROL(fu, control)				\
	BUILD_ASSERT(UTIL_OR(UTIL_NOT(DT_NODE_HAS_PROP(fu, control)),		\
		DT_PROP_LEN(fu, control) <= 1 + FEATURE_UNIT_NUM_CHANNELS(fu)),	\
		"Feature Unit " DT_NODE_PATH(fu) " has "			\
		STRINGIFY(FEATURE_UNIT_NUM_CHANNELS(fu)) " logical channel(s) "	\
		"but its property " #control " has "				\
		STRINGIFY(DT_PROP_LEN(fu, control)) " values"			\
	);

#define BUILD_ASSERT_FEATURE_UNIT_CONTROLS_LENGTH(entity)			\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, mute_control)			\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, volume_control)		\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, bass_control)			\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, mid_control)			\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, treble_control)		\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, graphic_equalizer_control)	\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, automatic_gain_control)	\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, delay_control)		\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, bass_boost_control)		\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, loudness_control)		\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, input_gain_control)		\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, input_gain_pad_control)	\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, phase_inverter_control)	\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, underflow_control)		\
	BUILD_ASSERT_FEATURE_UNIT_CONTROL(entity, overflow_control)

#define NEEDS_SUBSLOT_SIZE_AND_BIT_RESOLUTION(node) UTIL_OR(			\
	UTIL_OR(IS_EQ(AUDIO_STREAMING_FORMAT_TYPE(node), FORMAT_TYPE_I),	\
		IS_EQ(AUDIO_STREAMING_FORMAT_TYPE(node), FORMAT_TYPE_III)),	\
	UTIL_OR(IS_EQ(AUDIO_STREAMING_FORMAT_TYPE(node), EXT_FORMAT_TYPE_I),	\
		IS_EQ(AUDIO_STREAMING_FORMAT_TYPE(node), EXT_FORMAT_TYPE_III)))

#define VALIDATE_SUBSLOT_SIZE(node)						\
	(DT_PROP(node, subslot_size) >= 1 && DT_PROP(node, subslot_size) <= 4)

#define VALIDATE_BIT_RESOLUTION(node)						\
	(DT_PROP(node, bit_resolution) <= (DT_PROP(node, subslot_size) * 8))

#define VALIDATE_LINKED_TERMINAL(node)						\
	UTIL_OR(DT_NODE_HAS_COMPAT(DT_PROP(node, linked_terminal),		\
		zephyr_uac2_input_terminal),					\
		DT_NODE_HAS_COMPAT(DT_PROP(node, linked_terminal),		\
		zephyr_uac2_output_terminal))

#define VALIDATE_AS_BANDWIDTH(node)						\
	IF_ENABLED(UAC2_ALLOWED_AT_FULL_SPEED(DT_PARENT(node)),	(		\
		BUILD_ASSERT(AS_FS_DATA_EP_MAX_PACKET_SIZE(node) <= 1023,	\
			"Full-Speed bandwidth exceeded");			\
	))									\
	IF_ENABLED(UAC2_ALLOWED_AT_HIGH_SPEED(DT_PARENT(node)), (		\
		BUILD_ASSERT(USB_TPL_IS_VALID(AS_HS_DATA_EP_TPL(node)),		\
			"High-Speed bandwidth exceeded");			\
	))

#define VALIDATE_NODE(node)							\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_clock_source), (	\
		BUILD_ASSERT(DT_PROP_LEN(node, sampling_frequencies),		\
			"Sampling frequencies array must not be empty");	\
		BUILD_ASSERT(IS_ARRAY_SORTED(node, sampling_frequencies),	\
			"Sampling frequencies array must be sorted ascending");	\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_input_terminal), (	\
		BUILD_ASSERT(!((SPATIAL_LOCATIONS_U32(node) & BIT(31))) ||	\
			SPATIAL_LOCATIONS_U32(node) == BIT(31),			\
			"Raw Data set alongside other spatial locations");	\
		BUILD_ASSERT(VALIDATE_INPUT_TERMINAL_ASSOCIATION(node),		\
			"Terminals associations must be Input<->Output");	\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_output_terminal), (	\
		BUILD_ASSERT(VALIDATE_OUTPUT_TERMINAL_ASSOCIATION(node),	\
			"Terminals associations must be Input<->Output");	\
		BUILD_ASSERT(VALIDATE_OUTPUT_TERMINAL_DATA_SOURCE(node),	\
			"Unsupported Output Terminal data source");		\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_feature_unit), (	\
		BUILD_ASSERT(VALIDATE_FEATURE_UNIT_DATA_SOURCE(node),		\
			"Unsupported Feature Unit data source");		\
		BUILD_ASSERT_FEATURE_UNIT_CONTROLS_LENGTH(node);		\
	))									\
	IF_ENABLED(DT_NODE_HAS_COMPAT(node, zephyr_uac2_audio_streaming), (	\
		BUILD_ASSERT(VALIDATE_LINKED_TERMINAL(node),			\
			"Linked Terminal must be Input or Output Terminal");	\
		BUILD_ASSERT(!NEEDS_SUBSLOT_SIZE_AND_BIT_RESOLUTION(node) ||	\
			VALIDATE_SUBSLOT_SIZE(node),				\
			"Subslot Size can only be 1, 2, 3 or 4");		\
		BUILD_ASSERT(!NEEDS_SUBSLOT_SIZE_AND_BIT_RESOLUTION(node) ||	\
			VALIDATE_BIT_RESOLUTION(node),				\
			"Bit Resolution must fit inside Subslot Size");		\
		BUILD_ASSERT(!DT_PROP(node, implicit_feedback) ||		\
			!AS_IS_SOF_SYNCHRONIZED(node),				\
			"Implicit feedback on SOF synchronized clock");		\
		IF_ENABLED(AS_HAS_ISOCHRONOUS_DATA_ENDPOINT(node), (		\
			VALIDATE_AS_BANDWIDTH(node)))				\
	))

#define VALIDATE_INSTANCE(uac2)							\
	BUILD_ASSERT(DT_PROP(uac2, full_speed) || DT_PROP(uac2, high_speed),	\
		"Instance must be allowed to operate at least at one speed");	\
	DT_FOREACH_CHILD(uac2, VALIDATE_NODE)

#endif /* ZEPHYR_INCLUDE_USBD_UAC2_MACROS_H_ */
