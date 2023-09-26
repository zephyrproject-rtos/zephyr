/*
 * USB audio class internal header
 *
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Audio Device Class internal header
 *
 * This header file is used to store internal configuration
 * defines.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_AUDIO_INTERNAL_H_
#define ZEPHYR_INCLUDE_USB_CLASS_AUDIO_INTERNAL_H_

#define DUMMY_API  (const void *)1

#define USB_PASSIVE_IF_DESC_SIZE sizeof(struct usb_if_descriptor)
#define USB_AC_CS_IF_DESC_SIZE sizeof(struct as_cs_interface_descriptor)
#define USB_FORMAT_TYPE_I_DESC_SIZE sizeof(struct format_type_i_descriptor)
#define USB_STD_AS_AD_EP_DESC_SIZE sizeof(struct std_as_ad_endpoint_descriptor)
#define USB_CS_AS_AD_EP_DESC_SIZE sizeof(struct cs_as_ad_ep_descriptor)
#define USB_ACTIVE_IF_DESC_SIZE (USB_PASSIVE_IF_DESC_SIZE + \
				 USB_AC_CS_IF_DESC_SIZE + \
				 USB_FORMAT_TYPE_I_DESC_SIZE + \
				 USB_STD_AS_AD_EP_DESC_SIZE + \
				 USB_CS_AS_AD_EP_DESC_SIZE)

#define INPUT_TERMINAL_DESC_SIZE sizeof(struct input_terminal_descriptor)
#define OUTPUT_TERMINAL_DESC_SIZE sizeof(struct output_terminal_descriptor)

#define BMA_CONTROLS_OFFSET 6
#define FU_FIXED_ELEMS_SIZE 7

/* Macros for maintaining features of feature unit entity */
#define FEATURE_MUTE_SIZE			0x01
#define FEATURE_VOLUME_SIZE			0x02
#define FEATURE_BASS_SIZE			0x01
#define FEATURE_MID_SIZE			0x01
#define FEATURE_TREBLE_SIZE			0x01
#define FEATURE_TONE_CONTROL_SIZE (FEATURE_BASS_SIZE +\
				   FEATURE_MID_SIZE + \
				   FEATURE_TREBLE_SIZE)
#define FEATURE_GRAPHIC_EQUALIZER_SIZE		0x01
#define FEATURE_AUTOMATIC_GAIN_CONTROL_SIZE	0x01
#define FEATURE_DELAY_SIZE			0x02
#define FEATURE_BASS_BOOST_SIZE			0x01
#define FEATURE_LOUDNESS_SIZE			0x01

#define POS_MUTE		   0
#define POS_VOLUME		   (POS_MUTE + FEATURE_MUTE_SIZE)
#define POS_BASS		   (POS_VOLUME + FEATURE_VOLUME_SIZE)
#define POS_MID			   (POS_BASS + FEATURE_BASS_SIZE)
#define POS_TREBLE		   (POS_MID + FEATURE_MID_SIZE)
#define POS_GRAPHIC_EQUALIZER	   (POS_TREBLE + FEATURE_TREBLE_SIZE)
#define POS_AUTOMATIC_GAIN_CONTROL (POS_GRAPHIC_EQUALIZER + \
					FEATURE_GRAPHIC_EQUALIZER_SIZE)
#define POS_DELAY		   (POS_AUTOMATIC_GAIN_CONTROL + \
					FEATURE_AUTOMATIC_GAIN_CONTROL_SIZE)
#define POS_BASS_BOOST		   (POS_DELAY + FEATURE_DELAY_SIZE)
#define POS_LOUDNESS		   (POS_BASS_BOOST + FEATURE_BASS_BOOST_SIZE)

#define POS(prop, ch_idx, ch_cnt) (ch_cnt * POS_##prop + \
				  (ch_idx * FEATURE_##prop##_SIZE))
#define LEN(ch_cnt, prop) (ch_cnt * FEATURE_##prop##_SIZE)

/* Names of compatibles used for configuration of the device */
#define COMPAT_HP  usb_audio_hp
#define COMPAT_MIC usb_audio_mic
#define COMPAT_HS  usb_audio_hs

#define HEADPHONES_DEVICE_COUNT  DT_NUM_INST_STATUS_OKAY(COMPAT_HP)
#define MICROPHONE_DEVICE_COUNT  DT_NUM_INST_STATUS_OKAY(COMPAT_MIC)
#define HEADSET_DEVICE_COUNT     DT_NUM_INST_STATUS_OKAY(COMPAT_HS)

#define IF_USB_AUDIO_PROP_HP(i, prop, bitmask) \
	COND_CODE_1(DT_PROP(DT_INST(i, COMPAT_HP), prop), (bitmask), (0))
#define IF_USB_AUDIO_PROP_MIC(i, prop, bitmask) \
	COND_CODE_1(DT_PROP(DT_INST(i, COMPAT_MIC), prop), (bitmask), (0))
#define IF_USB_AUDIO_PROP_HS_HP(i, prop, bitmask) \
	COND_CODE_1(DT_PROP(DT_INST(i, COMPAT_HS), hp_##prop), (bitmask), (0))
#define IF_USB_AUDIO_PROP_HS_MIC(i, prop, bitmask) \
	COND_CODE_1(DT_PROP(DT_INST(i, COMPAT_HS), mic_##prop), (bitmask), (0))
#define IF_USB_AUDIO_PROP(dev, i, prop, bitmask) \
	IF_USB_AUDIO_PROP_##dev(i, prop, bitmask)

/* Macro for getting the bitmask of configured channels for given device */
#define CH_CFG(dev, i) (0x0000			 \
| IF_USB_AUDIO_PROP(dev, i, channel_l,   BIT(0)) \
| IF_USB_AUDIO_PROP(dev, i, channel_r,   BIT(1)) \
| IF_USB_AUDIO_PROP(dev, i, channel_c,   BIT(2)) \
| IF_USB_AUDIO_PROP(dev, i, channel_lfe, BIT(3)) \
| IF_USB_AUDIO_PROP(dev, i, channel_ls,  BIT(4)) \
| IF_USB_AUDIO_PROP(dev, i, channel_rs,  BIT(5)) \
| IF_USB_AUDIO_PROP(dev, i, channel_lc,  BIT(6)) \
| IF_USB_AUDIO_PROP(dev, i, channel_rc,  BIT(7)) \
| IF_USB_AUDIO_PROP(dev, i, channel_s,   BIT(8)) \
| IF_USB_AUDIO_PROP(dev, i, channel_sl,  BIT(9)) \
| IF_USB_AUDIO_PROP(dev, i, channel_sr,  BIT(10))\
| IF_USB_AUDIO_PROP(dev, i, channel_t,   BIT(11))\
)

/* Macro for getting the number of configured channels for given device.
 * Master channel (0) excluded.
 */
#define CH_CNT(dev, i) (0		   \
+ IF_USB_AUDIO_PROP(dev, i, channel_l,   1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_r,   1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_c,   1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_lfe, 1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_ls,  1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_rs,  1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_lc,  1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_rc,  1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_s,   1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_sl,  1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_sr,  1)\
+ IF_USB_AUDIO_PROP(dev, i, channel_t,   1)\
)

/* Macro for getting bitmask of supported features for given device */
#define FEATURES(dev, i) (0x0000					    \
| IF_USB_AUDIO_PROP(dev, i, feature_mute,		    BIT(0))	    \
| IF_USB_AUDIO_PROP(dev, i, feature_volume,		    BIT(1))	    \
| IF_USB_AUDIO_PROP(dev, i, feature_tone_control,  BIT(2) | BIT(3) | BIT(4))\
| IF_USB_AUDIO_PROP(dev, i, feature_graphic_equalizer,	    BIT(5))	    \
| IF_USB_AUDIO_PROP(dev, i, feature_automatic_gain_control, BIT(6))	    \
| IF_USB_AUDIO_PROP(dev, i, feature_delay,		    BIT(7))	    \
| IF_USB_AUDIO_PROP(dev, i, feature_bass_boost,		    BIT(8))	    \
| IF_USB_AUDIO_PROP(dev, i, feature_loudness,		    BIT(9))	    \
)

/* Macro for getting required size to store values for supported features. */
#define FEATURES_SIZE(dev, i) ((CH_CNT(dev, i) + 1) * (0x0000	\
+ IF_USB_AUDIO_PROP(dev, i, feature_mute,			\
			    FEATURE_MUTE_SIZE)			\
+ IF_USB_AUDIO_PROP(dev, i, feature_volume,			\
			    FEATURE_VOLUME_SIZE)		\
+ IF_USB_AUDIO_PROP(dev, i, feature_tone_control,		\
			    FEATURE_TONE_CONTROL_SIZE)		\
+ IF_USB_AUDIO_PROP(dev, i, feature_graphic_equalizer,		\
			    FEATURE_GRAPHIC_EQUALIZER_SIZE)	\
+ IF_USB_AUDIO_PROP(dev, i, feature_automatic_gain_control,	\
			    FEATURE_AUTOMATIC_GAIN_CONTROL_SIZE)\
+ IF_USB_AUDIO_PROP(dev, i, feature_delay,			\
			    FEATURE_DELAY_SIZE)			\
+ IF_USB_AUDIO_PROP(dev, i, feature_bass_boost,			\
			    FEATURE_BASS_BOOST_SIZE)		\
+ IF_USB_AUDIO_PROP(dev, i, feature_loudness,			\
			    FEATURE_LOUDNESS_SIZE)		\
))

#define GET_RES_HP(i) DT_PROP(DT_INST(i, COMPAT_HP), resolution)
#define GET_RES_MIC(i) DT_PROP(DT_INST(i, COMPAT_MIC), resolution)
#define GET_RES_HS_HP(i) DT_PROP(DT_INST(i, COMPAT_HS), hp_resolution)
#define GET_RES_HS_MIC(i) DT_PROP(DT_INST(i, COMPAT_HS), mic_resolution)
#define GET_RES(dev, i) GET_RES_##dev(i)

#define GET_VOLUME_HS(i, prop) DT_PROP_OR(DT_INST(i, COMPAT_HS), prop, 0)
#define GET_VOLUME_HP(i, prop) DT_PROP_OR(DT_INST(i, COMPAT_HP), prop, 0)
#define GET_VOLUME_MIC(i, prop) DT_PROP_OR(DT_INST(i, COMPAT_MIC), prop, 0)
#define GET_VOLUME(dev, i, prop) GET_VOLUME_##dev(i, prop)

#define SYNC_TYPE_HP(i)     3
#define SYNC_TYPE_MIC(i)    DT_ENUM_IDX(DT_INST(i, COMPAT_MIC), sync_type)
#define SYNC_TYPE_HS_HP(i)  3
#define SYNC_TYPE_HS_MIC(i) DT_ENUM_IDX(DT_INST(i, COMPAT_HS), mic_sync_type)
#define SYNC_TYPE(dev, i) (SYNC_TYPE_##dev(i) << 2)

#define EP_SIZE(dev, i) \
	((GET_RES(dev, i)/8) * CH_CNT(dev, i) * 48)

/* *_ID() macros are used to give proper Id to each entity describing
 * the device. Entities Id must start from 1 that's why 1 is added.
 * Multiplication by 3 for HP/MIC comes from the fact that 3 entities are
 * required to describe the device, 6 in case of HS.
 */
#define HP_ID(i) ((3*i) + 1)
#define MIC_ID(i) ((3*(HEADPHONES_DEVICE_COUNT + i)) + 1)
#define HS_ID(i) ((3*(HEADPHONES_DEVICE_COUNT + \
		      MICROPHONE_DEVICE_COUNT)) + 6*i + 1)

/* *_LINK() macros are used to properly connect relevant audio streaming
 * class specific interface with valid entity. In case of Headphones this
 * will always be 1st entity describing the device (Input terminal). This
 * is where addition of 1 comes from. In case of Headphones thill will always
 * be 3rd entity (Output terminal) - addition of 3.
 */
#define HP_LINK(i) ((3*i) + 1)
#define MIC_LINK(i) ((3*(HEADPHONES_DEVICE_COUNT + i)) + 3)

/**
 * Addressable logical object inside an audio function.
 * Entity is one of: Terminal or Unit.
 * Refer to 1.4 Terms and Abbreviations from audio10.pdf
 */
struct usb_audio_entity {
	enum usb_audio_cs_ac_int_desc_subtypes subtype;
	uint8_t id;
};

/**
 * @warning Size of baInterface is 2 just to make it usable
 * for all kind of devices: headphones, microphone and headset.
 * Actual size of the struct should be checked by reading
 * .bLength.
 */
struct cs_ac_if_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdADC;
	uint16_t wTotalLength;
	uint8_t bInCollection;
	uint8_t baInterfaceNr[2];
} __packed;

struct input_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bNrChannels;
	uint16_t wChannelConfig;
	uint8_t iChannelNames;
	uint8_t iTerminal;
} __packed;

/**
 * @note Size of Feature unit descriptor is not fixed.
 * This structure is just a helper not a common type.
 */
struct feature_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t bControlSize;
	uint16_t bmaControls[1];
} __packed;

struct output_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t iTerminal;
} __packed;

struct as_cs_interface_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bDescriptorSubtype;
	uint8_t  bTerminalLink;
	uint8_t  bDelay;
	uint16_t wFormatTag;
} __packed;

struct format_type_i_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint8_t bNrChannels;
	uint8_t bSubframeSize;
	uint8_t bBitResolution;
	uint8_t bSamFreqType;
	uint8_t tSamFreq[3];
} __packed;

struct std_as_ad_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
	uint8_t bRefresh;
	uint8_t bSynchAddress;
} __packed;

struct cs_as_ad_ep_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmAttributes;
	uint8_t bLockDelayUnits;
	uint16_t wLockDelay;
} __packed;

#define DECLARE_HEADER(dev, i, ifaces)	\
struct dev##_cs_ac_if_descriptor_##i {	\
	uint8_t bLength;			\
	uint8_t bDescriptorType;		\
	uint8_t bDescriptorSubtype;	\
	uint16_t bcdADC;			\
	uint16_t wTotalLength;		\
	uint8_t bInCollection;		\
	uint8_t baInterfaceNr[ifaces];	\
} __packed

#define DECLARE_FEATURE_UNIT(dev, i)		\
struct dev##_feature_unit_descriptor_##i {	\
	uint8_t bLength;				\
	uint8_t bDescriptorType;			\
	uint8_t bDescriptorSubtype;		\
	uint8_t bUnitID;				\
	uint8_t bSourceID;				\
	uint8_t bControlSize;			\
	uint16_t bmaControls[CH_CNT(dev, i) + 1];	\
	uint8_t iFeature;				\
} __packed

#define INIT_IAD(iface_subclass, if_cnt)			\
{								\
	.bLength = sizeof(struct usb_association_descriptor),	\
	.bDescriptorType = USB_DESC_INTERFACE_ASSOC,		\
	.bFirstInterface = 0,					\
	.bInterfaceCount = if_cnt,				\
	.bFunctionClass = USB_BCC_AUDIO,			\
	.bFunctionSubClass = iface_subclass,			\
	.bFunctionProtocol = 0,					\
	.iFunction = 0,						\
}

#define USB_AUDIO_IAD_DECLARE struct usb_association_descriptor iad;
#define USB_AUDIO_IAD(if_cnt)  .iad = INIT_IAD(USB_AUDIO_AUDIOCONTROL, if_cnt),

#define DECLARE_DESCRIPTOR(dev, i, ifaces)				\
DECLARE_HEADER(dev, i, ifaces);						\
DECLARE_FEATURE_UNIT(dev, i);						\
struct dev##_descriptor_##i {						\
	USB_AUDIO_IAD_DECLARE						\
	struct usb_if_descriptor std_ac_interface;			\
	struct dev##_cs_ac_if_descriptor_##i cs_ac_interface;		\
	struct input_terminal_descriptor input_terminal;		\
	struct dev##_feature_unit_descriptor_##i feature_unit;		\
	struct output_terminal_descriptor output_terminal;		\
	struct usb_if_descriptor as_interface_alt_0;			\
	struct usb_if_descriptor as_interface_alt_1;			\
		struct as_cs_interface_descriptor as_cs_interface;	\
		struct format_type_i_descriptor format;			\
		struct std_as_ad_endpoint_descriptor std_ep;		\
		struct cs_as_ad_ep_descriptor cs_ep;			\
} __packed

#define DECLARE_DESCRIPTOR_BIDIR(dev, i, ifaces)			\
DECLARE_HEADER(dev, i, ifaces);						\
DECLARE_FEATURE_UNIT(dev##_MIC, i);					\
DECLARE_FEATURE_UNIT(dev##_HP, i);					\
struct dev##_descriptor_##i {						\
	USB_AUDIO_IAD_DECLARE						\
	struct usb_if_descriptor std_ac_interface;			\
	struct dev##_cs_ac_if_descriptor_##i cs_ac_interface;		\
	struct input_terminal_descriptor input_terminal_0;		\
	struct dev##_MIC_feature_unit_descriptor_##i feature_unit_0;	\
	struct output_terminal_descriptor output_terminal_0;		\
	struct input_terminal_descriptor input_terminal_1;		\
	struct dev##_HP_feature_unit_descriptor_##i feature_unit_1;	\
	struct output_terminal_descriptor output_terminal_1;		\
	struct usb_if_descriptor as_interface_alt_0_0;			\
	struct usb_if_descriptor as_interface_alt_0_1;			\
		struct as_cs_interface_descriptor as_cs_interface_0;	\
		struct format_type_i_descriptor format_0;		\
		struct std_as_ad_endpoint_descriptor std_ep_0;		\
		struct cs_as_ad_ep_descriptor cs_ep_0;			\
	struct usb_if_descriptor as_interface_alt_1_0;			\
	struct usb_if_descriptor as_interface_alt_1_1;			\
		struct as_cs_interface_descriptor as_cs_interface_1;	\
		struct format_type_i_descriptor format_1;		\
		struct std_as_ad_endpoint_descriptor std_ep_1;		\
		struct cs_as_ad_ep_descriptor cs_ep_1;			\
} __packed

#define INIT_STD_IF(iface_subclass, iface_num, alt_setting, eps_num)	\
{									\
	.bLength = sizeof(struct usb_if_descriptor),			\
	.bDescriptorType = USB_DESC_INTERFACE,				\
	.bInterfaceNumber = iface_num,					\
	.bAlternateSetting = alt_setting,				\
	.bNumEndpoints = eps_num,					\
	.bInterfaceClass = USB_BCC_AUDIO,				\
	.bInterfaceSubClass = iface_subclass,				\
	.bInterfaceProtocol = 0,					\
	.iInterface = 0,						\
}

#define INIT_CS_AC_IF(dev, i, ifaces)					\
{									\
	.bLength = sizeof(struct dev##_cs_ac_if_descriptor_##i),	\
	.bDescriptorType = USB_DESC_CS_INTERFACE,			\
	.bDescriptorSubtype = USB_AUDIO_HEADER,				\
	.bcdADC = sys_cpu_to_le16(0x0100),				\
	.wTotalLength = sys_cpu_to_le16(				\
		sizeof(struct dev##_cs_ac_if_descriptor_##i) +		\
		INPUT_TERMINAL_DESC_SIZE +				\
		sizeof(struct dev##_feature_unit_descriptor_##i) +	\
		OUTPUT_TERMINAL_DESC_SIZE),				\
	.bInCollection = ifaces,					\
	.baInterfaceNr = {0},						\
}

#define INIT_CS_AC_IF_BIDIR(dev, i, ifaces)				\
{									\
	.bLength = sizeof(struct dev##_cs_ac_if_descriptor_##i),	\
	.bDescriptorType = USB_DESC_CS_INTERFACE,			\
	.bDescriptorSubtype = USB_AUDIO_HEADER,				\
	.bcdADC = sys_cpu_to_le16(0x0100),				\
	.wTotalLength = sys_cpu_to_le16(				\
		sizeof(struct dev##_cs_ac_if_descriptor_##i) +		\
		2*INPUT_TERMINAL_DESC_SIZE +				\
		sizeof(struct dev##_MIC_feature_unit_descriptor_##i) +	\
		sizeof(struct dev##_HP_feature_unit_descriptor_##i) +	\
		2*OUTPUT_TERMINAL_DESC_SIZE),				\
	.bInCollection = ifaces,					\
	.baInterfaceNr = {0},						\
}

#define INIT_IN_TERMINAL(dev, i, terminal_id, type)		\
{								\
	.bLength = INPUT_TERMINAL_DESC_SIZE,			\
	.bDescriptorType = USB_DESC_CS_INTERFACE,		\
	.bDescriptorSubtype = USB_AUDIO_INPUT_TERMINAL,		\
	.bTerminalID = terminal_id,				\
	.wTerminalType = sys_cpu_to_le16(type),			\
	.bAssocTerminal = 0,					\
	.bNrChannels = MAX(1, CH_CNT(dev, i)),			\
	.wChannelConfig = sys_cpu_to_le16(CH_CFG(dev, i)),	\
	.iChannelNames = 0,					\
	.iTerminal = 0,						\
}

#define INIT_OUT_TERMINAL(terminal_id, source_id, type)	\
{							\
	.bLength = OUTPUT_TERMINAL_DESC_SIZE,		\
	.bDescriptorType = USB_DESC_CS_INTERFACE,	\
	.bDescriptorSubtype = USB_AUDIO_OUTPUT_TERMINAL,\
	.bTerminalID = terminal_id,			\
	.wTerminalType = sys_cpu_to_le16(type),		\
	.bAssocTerminal = 0,				\
	.bSourceID = source_id,				\
	.iTerminal = 0,					\
}

/** refer to Table 4-7 from audio10.pdf
 */
#define INIT_FEATURE_UNIT(dev, i, unit_id, source_id)			\
{									\
	.bLength = sizeof(struct dev##_feature_unit_descriptor_##i),	\
	.bDescriptorType = USB_DESC_CS_INTERFACE,			\
	.bDescriptorSubtype = USB_AUDIO_FEATURE_UNIT,			\
	.bUnitID = unit_id,						\
	.bSourceID = source_id,						\
	.bControlSize = sizeof(uint16_t),					\
	.bmaControls = { FEATURES(dev, i) },				\
	.iFeature = 0,							\
}

/* Class-Specific AS Interface Descriptor 4.5.2 audio10.pdf */
#define INIT_AS_GENERAL(link)				\
{							\
	.bLength = USB_AC_CS_IF_DESC_SIZE,		\
	.bDescriptorType = USB_DESC_CS_INTERFACE,	\
	.bDescriptorSubtype = USB_AUDIO_AS_GENERAL,	\
	.bTerminalLink = link,				\
	.bDelay = 1,					\
	.wFormatTag = sys_cpu_to_le16(0x0001),		\
}

/** Class-Specific AS Format Type Descriptor 4.5.3 audio10.pdf
 *  For more information refer to 2.2.5 Type I Format Type Descriptor
 *  from frmts10.pdf
 */
#define INIT_AS_FORMAT_I(ch_cnt, res)				\
{								\
	.bLength = sizeof(struct format_type_i_descriptor),	\
	.bDescriptorType = USB_DESC_CS_INTERFACE,		\
	.bDescriptorSubtype = USB_AUDIO_FORMAT_TYPE,		\
	.bFormatType = 0x01,					\
	.bNrChannels = MAX(1, ch_cnt),				\
	.bSubframeSize = res/8,					\
	.bBitResolution = res,					\
	.bSamFreqType = 1,					\
	.tSamFreq = {0x80, 0xBB, 0x00},				\
}

#define INIT_STD_AS_AD_EP(dev, i, addr)					\
{									\
	.bLength = sizeof(struct std_as_ad_endpoint_descriptor),	\
	.bDescriptorType = USB_DESC_ENDPOINT,				\
	.bEndpointAddress = addr,					\
	.bmAttributes = (USB_DC_EP_ISOCHRONOUS | SYNC_TYPE(dev, i)),	\
	.wMaxPacketSize = sys_cpu_to_le16(EP_SIZE(dev, i)),		\
	.bInterval = 0x01,						\
	.bRefresh = 0x00,						\
	.bSynchAddress = 0x00,						\
}

#define INIT_CS_AS_AD_EP					\
{								\
	.bLength = sizeof(struct cs_as_ad_ep_descriptor),	\
	.bDescriptorType = USB_DESC_CS_ENDPOINT,		\
	.bDescriptorSubtype = 0x01,				\
	.bmAttributes = 0x00,					\
	.bLockDelayUnits = 0x00,				\
	.wLockDelay = 0,					\
}

#define INIT_EP_DATA(cb, addr)	\
	{			\
		.ep_cb = cb,	\
		.ep_addr = addr,\
	}

#endif /* ZEPHYR_INCLUDE_USB_CLASS_AUDIO_INTERNAL_H_ */
