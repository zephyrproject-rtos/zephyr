/*
 * SPDX-FileCopyrightText: Copyright 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Audio Class 2.0 definitions
 *
 * This file contains USB Audio Class 2.0 definitions that are shared
 * between host and device implementations.
 */

#ifndef ZEPHYR_INCLUDE_USB_USB_UAC2_H_
#define ZEPHYR_INCLUDE_USB_USB_UAC2_H_

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup usb_uac2_definitions USB Audio Class 2.0 Definitions
 * @ingroup usb
 * @{
 */

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

/* A.7 Audio Function Category Codes */
#define FUNCTION_SUBCLASS_UNDEFINED		0x00
#define DESKTOP_SPEAKER				0x01
#define HOME_THEATER				0x02
#define MICROPHONE				0x03
#define HEADSET					0x04
#define TELEPHONE_CATEGORY			0x05
#define CONVERTER				0x06
#define VOICE_SOUND_RECORDER			0x07
#define IO_BOX					0x08
#define MUSICAL_INSTRUMENT			0x09
#define PRO_AUDIO				0x0A
#define AUDIO_VIDEO				0x0B
#define CONTROL_PANEL				0x0C
#define OTHER					0xFF

/* A.8 Audio Class-Specific Descriptor Types */
#define CS_UNDEFINED				0x20
#define CS_DEVICE				0x21
#define CS_CONFIGURATION			0x22
#define CS_STRING				0x23
#define CS_INTERFACE				0x24
#define CS_ENDPOINT				0x25

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

/* A.11 Effect Unit Effect Types */
#define EFFECT_TYPE_UNDEFINED			0x00
#define PARAM_EQ_SECTION_EFFECT			0x01
#define REVERBERATION_EFFECT			0x02
#define MOD_DELAY_EFFECT			0x03
#define DYN_RANGE_COMP_EFFECT			0x04

/* A.12 Processing Unit Process Types */
#define PROCESS_UNDEFINED			0x00
#define UP_DOWNMIX_PROCESS			0x01
#define DOLBY_PROLOGIC_PROCESS			0x02
#define STEREO_EXTENDER_PROCESS			0x03

/* A.13 Audio Class-Specific Endpoint Descriptor Subtypes */
#define DESCRIPTOR_UNDEFINED			0x00
#define EP_GENERAL				0x01

/* A.14 Audio Class-Specific Request Codes */
#define REQUEST_CODE_UNDEFINED			0x00
#define CUR					0x01
#define RANGE					0x02
#define MEM					0x03

/* A.15 Encoder Type Codes */
#define ENCODER_UNDEFINED			0x00
#define OTHER_ENCODER				0x01
#define MPEG_ENCODER				0x02
#define AC3_ENCODER				0x03
#define WMA_ENCODER				0x04
#define DTS_ENCODER				0x05

/* A.16 Decoder Type Codes */
#define DECODER_UNDEFINED			0x00
#define OTHER_DECODER				0x01
#define MPEG_DECODER				0x02
#define AC3_DECODER				0x03
#define WMA_DECODER				0x04
#define DTS_DECODER				0x05

/* A.17.1 Clock Source Control Selectors */
#define CS_CONTROL_UNDEFINED			0x00
#define CS_SAM_FREQ_CONTROL			0x01
#define CS_CLOCK_VALID_CONTROL			0x02

/* A.17.2 Clock Selector Control Selectors */
#define CX_CONTROL_UNDEFINED			0x00
#define CX_CLOCK_SELECTOR_CONTROL		0x01

/* A.17.3 Clock Multiplier Control Selectors */
#define CM_CONTROL_UNDEFINED			0x00
#define CM_NUMERATOR_CONTROL			0x01
#define CM_DENOMINATOR_CONTROL			0x02

/* A.17.4 Terminal Control Selectors */
#define TE_CONTROL_UNDEFINED			0x00
#define TE_COPY_PROTECT_CONTROL			0x01
#define TE_CONNECTOR_CONTROL			0x02
#define TE_OVERLOAD_CONTROL			0x03
#define TE_CLUSTER_CONTROL			0x04
#define TE_UNDERFLOW_CONTROL			0x05
#define TE_OVERFLOW_CONTROL			0x06
#define TE_LATENCY_CONTROL			0x07

/* A.17.5 Mixer Control Selectors */
#define MU_CONTROL_UNDEFINED			0x00
#define MU_MIXER_CONTROL			0x01
#define MU_CLUSTER_CONTROL			0x02
#define MU_UNDERFLOW_CONTROL			0x03
#define MU_OVERFLOW_CONTROL			0x04
#define MU_LATENCY_CONTROL			0x05

/* A.17.6 Selector Control Selectors */
#define SU_CONTROL_UNDEFINED			0x00
#define SU_SELECTOR_CONTROL			0x01
#define SU_LATENCY_CONTROL			0x02

/* A.17.7 Feature Unit Control Selectors */
#define FU_CONTROL_UNDEFINED			0x00
#define FU_MUTE_CONTROL				0x01
#define FU_VOLUME_CONTROL			0x02
#define FU_BASS_CONTROL				0x03
#define FU_MID_CONTROL				0x04
#define FU_TREBLE_CONTROL			0x05
#define FU_GRAPHIC_EQUALIZER_CONTROL		0x06
#define FU_AUTOMATIC_GAIN_CONTROL		0x07
#define FU_DELAY_CONTROL			0x08
#define FU_BASS_BOOST_CONTROL			0x09
#define FU_LOUDNESS_CONTROL			0x0A
#define FU_INPUT_GAIN_CONTROL			0x0B
#define FU_INPUT_GAIN_PAD_CONTROL		0x0C
#define FU_PHASE_INVERTER_CONTROL		0x0D
#define FU_UNDERFLOW_CONTROL			0x0E
#define FU_OVERFLOW_CONTROL			0x0F
#define FU_LATENCY_CONTROL			0x10

/* A.17.8.1 Parametric Equalizer Section Effect Unit Control Selectors */
#define PE_CONTROL_UNDEFINED			0x00
#define PE_ENABLE_CONTROL			0x01
#define PE_CENTERFREQ_CONTROL			0x02
#define PE_QFACTOR_CONTROL			0x03
#define PE_GAIN_CONTROL				0x04
#define PE_UNDERFLOW_CONTROL			0x05
#define PE_OVERFLOW_CONTROL			0x06
#define PE_LATENCY_CONTROL			0x07

/* A.17.8.2 Reverberation Effect Unit Control Selectors */
#define RV_CONTROL_UNDEFINED			0x00
#define RV_ENABLE_CONTROL			0x01
#define RV_TYPE_CONTROL				0x02
#define RV_LEVEL_CONTROL			0x03
#define RV_TIME_CONTROL				0x04
#define RV_FEEDBACK_CONTROL			0x05
#define RV_PREDELAY_CONTROL			0x06
#define RV_DENSITY_CONTROL			0x07
#define RV_HIFREQ_ROLLOFF_CONTROL		0x08
#define RV_UNDERFLOW_CONTROL			0x09
#define RV_OVERFLOW_CONTROL			0x0A
#define RV_LATENCY_CONTROL			0x0B

/* A.17.8.3 Modulation Delay Effect Unit Control Selectors */
#define MD_CONTROL_UNDEFINED			0x00
#define MD_ENABLE_CONTROL			0x01
#define MD_BALANCE_CONTROL			0x02
#define MD_RATE_CONTROL				0x03
#define MD_DEPTH_CONTROL			0x04
#define MD_TIME_CONTROL				0x05
#define MD_FEEDBACK_CONTROL			0x06
#define MD_UNDERFLOW_CONTROL			0x07
#define MD_OVERFLOW_CONTROL			0x08
#define MD_LATENCY_CONTROL			0x09

/* A.17.8.4 Dynamic Range Compressor Effect Unit Control Selectors */
#define DR_CONTROL_UNDEFINED			0x00
#define DR_ENABLE_CONTROL			0x01
#define DR_COMPRESSION_RATE_CONTROL		0x02
#define DR_MAXAMPL_CONTROL			0x03
#define DR_THRESHOLD_CONTROL			0x04
#define DR_ATTACK_TIME_CONTROL			0x05
#define DR_RELEASE_TIME_CONTROL			0x06
#define DR_UNDERFLOW_CONTROL			0x07
#define DR_OVERFLOW_CONTROL			0x08
#define DR_LATENCY_CONTROL			0x09

/* A.17.9.1 Up/Down-mix Processing Unit Control Selectors */
#define UD_CONTROL_UNDEFINED			0x00
#define UD_ENABLE_CONTROL			0x01
#define UD_MODE_SELECT_CONTROL			0x02
#define UD_CLUSTER_CONTROL			0x03
#define UD_UNDERFLOW_CONTROL			0x04
#define UD_OVERFLOW_CONTROL			0x05
#define UD_LATENCY_CONTROL			0x06

/* A.17.9.2 Dolby Prologic Processing Unit Control Selectors */
#define DP_CONTROL_UNDEFINED			0x00
#define DP_ENABLE_CONTROL			0x01
#define DP_MODE_SELECT_CONTROL			0x02
#define DP_CLUSTER_CONTROL			0x03
#define DP_UNDERFLOW_CONTROL			0x04
#define DP_OVERFLOW_CONTROL			0x05
#define DP_LATENCY_CONTROL			0x06

/* A.17.9.3 Stereo Extender Processing Unit Control Selectors */
#define ST_EXT_CONTROL_UNDEFINED		0x00
#define ST_EXT_ENABLE_CONTROL			0x01
#define ST_EXT_WIDTH_CONTROL			0x02
#define ST_EXT_UNDERFLOW_CONTROL		0x03
#define ST_EXT_OVERFLOW_CONTROL			0x04
#define ST_EXT_LATENCY_CONTROL			0x05

/* A.17.10 Extension Unit Control Selectors */
#define XU_CONTROL_UNDEFINED			0x00
#define XU_ENABLE_CONTROL			0x01
#define XU_CLUSTER_CONTROL			0x02
#define XU_UNDERFLOW_CONTROL			0x03
#define XU_OVERFLOW_CONTROL			0x04
#define XU_LATENCY_CONTROL			0x05

/* A.17.11 AudioStreaming Interface Control Selectors */
#define AS_CONTROL_UNDEFINED			0x00
#define AS_ACT_ALT_SETTING_CONTROL		0x01
#define AS_VAL_ALT_SETTINGS_CONTROL		0x02
#define AS_AUDIO_DATA_FORMAT_CONTROL		0x03

/* A.17.12 Encoder Control Selectors */
#define EN_CONTROL_UNDEFINED			0x00
#define EN_BIT_RATE_CONTROL			0x01
#define EN_QUALITY_CONTROL			0x02
#define EN_VBR_CONTROL				0x03
#define EN_TYPE_CONTROL				0x04
#define EN_UNDERFLOW_CONTROL			0x05
#define EN_OVERFLOW_CONTROL			0x06
#define EN_ENCODER_ERROR_CONTROL		0x07
#define EN_PARAM1_CONTROL			0x08
#define EN_PARAM2_CONTROL			0x09
#define EN_PARAM3_CONTROL			0x0A
#define EN_PARAM4_CONTROL			0x0B
#define EN_PARAM5_CONTROL			0x0C
#define EN_PARAM6_CONTROL			0x0D
#define EN_PARAM7_CONTROL			0x0E
#define EN_PARAM8_CONTROL			0x0F

/* A.17.13.1 MPEG Decoder Control Selectors */
#define MD_CONTROL_UNDEFINED			0x00
#define MD_DUAL_CHANNEL_CONTROL			0x01
#define MD_SECOND_STEREO_CONTROL		0x02
#define MD_MULTILINGUAL_CONTROL			0x03
#define MD_DYN_RANGE_CONTROL			0x04
#define MD_SCALING_CONTROL			0x05
#define MD_HILO_SCALING_CONTROL			0x06
#define MD_UNDERFLOW_CONTROL			0x07
#define MD_OVERFLOW_CONTROL			0x08
#define MD_DECODER_ERROR_CONTROL		0x09

/* A.17.13.2 AC-3 Decoder Control Selectors */
#define AD_CONTROL_UNDEFINED			0x00
#define AD_MODE_CONTROL				0x01
#define AD_DYN_RANGE_CONTROL			0x02
#define AD_SCALING_CONTROL			0x03
#define AD_HILO_SCALING_CONTROL			0x04
#define AD_UNDERFLOW_CONTROL			0x05
#define AD_OVERFLOW_CONTROL			0x06
#define AD_DECODER_ERROR_CONTROL		0x07

/* A.17.13.3 WMA Decoder Control Selectors */
#define WD_CONTROL_UNDEFINED			0x00
#define WD_UNDERFLOW_CONTROL			0x01
#define WD_OVERFLOW_CONTROL			0x02
#define WD_DECODER_ERROR_CONTROL		0x03

/* A.17.13.4 DTS Decoder Control Selectors */
#define DD_CONTROL_UNDEFINED			0x00
#define DD_UNDERFLOW_CONTROL			0x01
#define DD_OVERFLOW_CONTROL			0x02
#define DD_DECODER_ERROR_CONTROL		0x03

/* A.17.14 Endpoint Control Selectors */
#define EP_CONTROL_UNDEFINED			0x00
#define EP_PITCH_CONTROL			0x01
#define EP_DATA_OVERRUN_CONTROL			0x02
#define EP_DATA_UNDERRUN_CONTROL		0x03

/* 2.1 USB Terminal Types */
#define USB_TERMINAL_UNDEFINED			0x0100
#define USB_TERMINAL_STREAMING			0x0101
#define USB_TERMINAL_VENDOR_SPECIFIC		0x01FF

/* 2.2 Input Terminal Types */
#define INPUT_TERMINAL_UNDEFINED			0x0200
#define INPUT_TERMINAL_MICROPHONE			0x0201
#define INPUT_TERMINAL_DESKTOP_MICROPHONE		0x0202
#define INPUT_TERMINAL_PERSONAL_MICROPHONE		0x0203
#define INPUT_TERMINAL_OMNI_DIRECTIONAL_MICROPHONE	0x0204
#define INPUT_TERMINAL_MICROPHONE_ARRAY			0x0205
#define INPUT_TERMINAL_PROCESSING_MICROPHONE_ARRAY	0x0206

/* 2.3 Output Terminal Types */
#define OUTPUT_TERMINAL_UNDEFINED			0x0300
#define OUTPUT_TERMINAL_SPEAKER				0x0301
#define OUTPUT_TERMINAL_HEADPHONES			0x0302
#define OUTPUT_TERMINAL_HEAD_MOUNTED_DISPLAY_AUDIO	0x0303
#define OUTPUT_TERMINAL_DESKTOP_SPEAKER			0x0304
#define OUTPUT_TERMINAL_ROOM_SPEAKER			0x0305
#define OUTPUT_TERMINAL_COMMUNICATION_SPEAKER		0x0306
#define OUTPUT_TERMINAL_LOW_FREQUENCY_EFFECTS_SPEAKER	0x0307

/* 2.4 Bi-directional Terminal Types */
#define BIDIRECTIONAL_UNDEFINED				0x0400
#define HANDSET						0x0401
#define HEADSET_BIDIRECTIONAL				0x0402
#define SPEAKERPHONE_NO_ECHO_REDUCTION			0x0403
#define ECHO_SUPPRESSING_SPEAKERPHONE			0x0404
#define ECHO_CANCELING_SPEAKERPHONE			0x0405

/* 2.5 Telephony Terminal Types */
#define TELEPHONY_UNDEFINED				0x0500
#define PHONE_LINE					0x0501
#define TELEPHONE					0x0502
#define DOWN_LINE_PHONE					0x0503

/* 2.6 External Terminal Types */
#define EXTERNAL_UNDEFINED				0x0600
#define ANALOG_CONNECTOR				0x0601
#define DIGITAL_AUDIO_INTERFACE				0x0602
#define LINE_CONNECTOR					0x0603
#define LEGACY_AUDIO_CONNECTOR				0x0604
#define SPDIF_INTERFACE					0x0605
#define IEEE_1394_DA_STREAM				0x0606
#define IEEE_1394_DV_STREAM_SOUNDTRACK			0x0607
#define ADAT_LIGHTPIPE					0x0608
#define TDIF						0x0609
#define MADI						0x060A

/* 2.7 Embedded Function Terminal Types */
#define EMBEDDED_UNDEFINED				0x0700
#define LEVEL_CALIBRATION_NOISE_SOURCE			0x0701
#define EQUALIZATION_NOISE				0x0702
#define CD_PLAYER					0x0703
#define DAT						0x0704
#define DCC						0x0705
#define COMPRESSED_AUDIO_PLAYER				0x0706
#define ANALOG_TAPE					0x0707
#define PHONOGRAPH					0x0708
#define VCR_AUDIO					0x0709
#define VIDEO_DISC_AUDIO				0x070A
#define DVD_AUDIO					0x070B
#define TV_TUNER_AUDIO					0x070C
#define SATELLITE_RECEIVER_AUDIO			0x070D
#define CABLE_TUNER_AUDIO				0x070E
#define DSS_AUDIO					0x070F
#define RADIO_RECEIVER					0x0710
#define RADIO_TRANSMITTER				0x0711
#define MULTI_TRACK_RECORDER				0x0712
#define SYNTHESIZER					0x0713
#define PIANO						0x0714
#define GUITAR						0x0715
#define DRUMS_RHYTHM					0x0716
#define OTHER_MUSICAL_INSTRUMENT			0x0717

/* A.1 Format Type Codes */
#define FORMAT_TYPE_UNDEFINED			0
#define FORMAT_TYPE_I				1
#define FORMAT_TYPE_II				2
#define FORMAT_TYPE_III				3
#define FORMAT_TYPE_IV				4
#define EXT_FORMAT_TYPE_I			129
#define EXT_FORMAT_TYPE_II			130
#define EXT_FORMAT_TYPE_III			131

/* A.2.1 Audio Data Format Type I Bit Allocations */
#define TYPE_I_PCM				BIT(0)
#define TYPE_I_PCM8				BIT(1)
#define TYPE_I_IEEE_FLOAT			BIT(2)
#define TYPE_I_ALAW				BIT(3)
#define TYPE_I_MULAW				BIT(4)
#define TYPE_I_RAW_DATA				BIT(31)

/* A.2.2 Audio Data Format Type II Bit Allocations */
#define TYPE_II_MPEG				BIT(0)
#define TYPE_II_AC3				BIT(1)
#define TYPE_II_WMA				BIT(2)
#define TYPE_II_DTS				BIT(3)
#define TYPE_II_RAW_DATA			BIT(31)

/* A.2.3 Audio Data Format Type III Bit Allocations */
#define TYPE_III_IEC61937_AC3			BIT(0)
#define TYPE_III_IEC61937_MPEG1_L1		BIT(1)
#define TYPE_III_IEC61937_MPEG1_L23		BIT(2)	/* or IEC61937_MPEG2_NOEXT */
#define TYPE_III_IEC61937_MPEG2_NOEXT		BIT(2)	/* or IEC61937_MPEG1_L23 */
#define TYPE_III_IEC61937_MPEG2_EXT		BIT(3)
#define TYPE_III_IEC61937_MPEG2_AAC_ADTS	BIT(4)
#define TYPE_III_IEC61937_MPEG2_L1_LS		BIT(5)
#define TYPE_III_IEC61937_MPEG2_L23_LS		BIT(6)
#define TYPE_III_IEC61937_DTS_I			BIT(7)
#define TYPE_III_IEC61937_DTS_II		BIT(8)
#define TYPE_III_IEC61937_DTS_III		BIT(9)
#define TYPE_III_IEC61937_ATRAC			BIT(10)
#define TYPE_III_IEC61937_ATRAC23		BIT(11)
#define TYPE_III_WMA				BIT(12)

/* A.2.4 Audio Data Format Type IV Bit Allocations */
#define TYPE_IV_PCM				BIT(0)
#define TYPE_IV_PCM8				BIT(1)
#define TYPE_IV_IEEE_FLOAT			BIT(2)
#define TYPE_IV_ALAW				BIT(3)
#define TYPE_IV_MULAW				BIT(4)
#define TYPE_IV_MPEG				BIT(5)
#define TYPE_IV_AC3				BIT(6)
#define TYPE_IV_WMA				BIT(7)
#define TYPE_IV_IEC61937_AC3			BIT(8)
#define TYPE_IV_IEC61937_MPEG1_L1		BIT(9)
#define TYPE_IV_IEC61937_MPEG1_L23		BIT(10)	/* or IEC61937_MPEG2_NOEXT */
#define TYPE_IV_IEC61937_MPEG2_NOEXT		BIT(10)	/* or IEC61937_MPEG1_L23 */
#define TYPE_IV_IEC61937_MPEG2_EXT		BIT(11)
#define TYPE_IV_IEC61937_MPEG2_AAC_ADTS		BIT(12)
#define TYPE_IV_IEC61937_MPEG2_L1_LS		BIT(13)
#define TYPE_IV_IEC61937_MPEG2_L23_LS		BIT(14)
#define TYPE_IV_IEC61937_DTS_I			BIT(15)
#define TYPE_IV_IEC61937_DTS_II			BIT(16)
#define TYPE_IV_IEC61937_DTS_III		BIT(17)
#define TYPE_IV_IEC61937_ATRAC			BIT(18)
#define TYPE_IV_IEC61937_ATRAC23		BIT(19)
#define TYPE_IV_IEC61937_WMA			BIT(20)
#define TYPE_IV_IEC60958_PCM			BIT(21)

/* 4.1 Audio Channel Cluster Descriptor - Spatial Locations */
#define CHANNEL_FRONT_LEFT			BIT(0)
#define CHANNEL_FRONT_RIGHT			BIT(1)
#define CHANNEL_FRONT_CENTER			BIT(2)
#define CHANNEL_LOW_FREQ_EFFECTS		BIT(3)
#define CHANNEL_BACK_LEFT			BIT(4)
#define CHANNEL_BACK_RIGHT			BIT(5)
#define CHANNEL_FRONT_LEFT_CENTER		BIT(6)
#define CHANNEL_FRONT_RIGHT_CENTER		BIT(7)
#define CHANNEL_BACK_CENTER			BIT(8)
#define CHANNEL_SIDE_LEFT			BIT(9)
#define CHANNEL_SIDE_RIGHT			BIT(10)
#define CHANNEL_TOP_CENTER			BIT(11)
#define CHANNEL_TOP_FRONT_LEFT			BIT(12)
#define CHANNEL_TOP_FRONT_CENTER		BIT(13)
#define CHANNEL_TOP_FRONT_RIGHT			BIT(14)
#define CHANNEL_TOP_BACK_LEFT			BIT(15)
#define CHANNEL_TOP_BACK_CENTER			BIT(16)
#define CHANNEL_TOP_BACK_RIGHT			BIT(17)
#define CHANNEL_TOP_FRONT_LEFT_CENTER		BIT(18)
#define CHANNEL_TOP_FRONT_RIGHT_CENTER		BIT(19)
#define CHANNEL_LEFT_LOW_FREQ_EFFECTS		BIT(20)
#define CHANNEL_RIGHT_LOW_FREQ_EFFECTS		BIT(21)
#define CHANNEL_TOP_SIDE_LEFT			BIT(22)
#define CHANNEL_TOP_SIDE_RIGHT			BIT(23)
#define CHANNEL_BOTTOM_CENTER			BIT(24)
#define CHANNEL_BACK_LEFT_CENTER		BIT(25)
#define CHANNEL_BACK_RIGHT_CENTER		BIT(26)
#define CHANNEL_RAW_DATA			BIT(31)

/**
 * @brief Audio 2.0 Control Request Parameter Block Layout
 * Reference: USB Audio 2.0 spec Section 5.2.3
 */
#define UAC2_PARAM_BLOCK_LAYOUT_1		1
#define UAC2_PARAM_BLOCK_LAYOUT_2		2
#define UAC2_PARAM_BLOCK_LAYOUT_3		3

/**
 * @brief Audio Control Interface Header Descriptor (4.7.2)
 */
struct uac2_ac_header_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdADC;
	uint8_t bCategory;
	uint16_t wTotalLength;
	uint8_t bmControls;
} __packed;

/**
 * @brief Clock Source Descriptor (4.7.2.1)
 */
struct uac2_clock_source_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bClockID;
	uint8_t bmAttributes;
	uint8_t bmControls;
	uint8_t bAssocTerminal;
	uint8_t iClockSource;
} __packed;

/**
 * @brief Clock Selector Descriptor (4.7.2.2)
 */
struct uac2_clock_selector_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bClockID;
	uint8_t bNrInPins;
	/* Variable length fields follow:
	 * uint8_t baCSourceID[bNrInPins];
	 * uint8_t bmControls;
	 * uint8_t iClockSelector;
	 */
} __packed;

/**
 * @brief Clock Multiplier Descriptor (4.7.2.3)
 */
struct uac2_clock_multiplier_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bClockID;
	uint8_t bCSourceID;
	uint8_t bmControls;
	uint8_t iClockMultiplier;
} __packed;

/**
 * @brief Input Terminal Descriptor (4.7.2.4)
 */
struct uac2_input_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bCSourceID;
	uint8_t bNrChannels;
	uint32_t bmChannelConfig;
	uint8_t iChannelNames;
	uint16_t bmControls;
	uint8_t iTerminal;
} __packed;

/**
 * @brief Output Terminal Descriptor (4.7.2.5)
 */
struct uac2_output_terminal_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalID;
	uint16_t wTerminalType;
	uint8_t bAssocTerminal;
	uint8_t bSourceID;
	uint8_t bCSourceID;
	uint16_t bmControls;
	uint8_t iTerminal;
} __packed;

/**
 * @brief Mixer Unit Descriptor (4.7.2.6)
 */
struct uac2_mixer_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bNrInPins;
	/* Variable length fields follow:
	 * uint8_t baSourceID[bNrInPins];
	 * uint8_t bNrChannels;
	 * uint32_t bmChannelConfig;
	 * uint8_t iChannelNames;
	 * uint8_t bmMixerControls[(bNrInPins * bNrChannels + 7) / 8];
	 * uint8_t bmControls;
	 * uint8_t iMixer;
	 */
} __packed;

/**
 * @brief Selector Unit Descriptor (4.7.2.7)
 */
struct uac2_selector_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bNrInPins;
	/* Variable length fields follow:
	 * uint8_t baSourceID[bNrInPins];
	 * uint8_t bmControls;
	 * uint8_t iSelector;
	 */
} __packed;

/**
 * @brief Feature Unit Descriptor (4.7.2.8)
 */
struct uac2_feature_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	/* Variable length fields follow:
	 * uint32_t bmaControls[ch + 1]; where ch is number of logical channels
	 * uint8_t iFeature;
	 */
} __packed;

/**
 * @brief Effect Unit Descriptor (4.7.2.10)
 */
struct uac2_effect_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint16_t wEffectType;
	uint8_t bSourceID;
	/* Variable length fields follow:
	 * uint32_t bmaControls[ch+1];
	 * uint8_t iEffects;
	 */
} __packed;

/**
 * @brief Processing Unit Descriptor (4.7.2.11)
 */
struct uac2_processing_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint16_t wProcessType;
	uint8_t bNrInPins;
	/* Variable length fields follow:
	 * uint8_t baSourceID[bNrInPins];
	 * uint8_t bNrChannels;
	 * uint32_t bmChannelConfig;
	 * uint8_t iChannelNames;
	 * uint16_t bmControls;
	 * uint8_t iProcessing;
	 * uint8_t bmProcessSpecific[];
	 */
} __packed;

/**
 * @brief Extension Unit Descriptor (4.7.2.12)
 */
struct uac2_extension_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint16_t wExtensionCode;
	uint8_t bNrInPins;
	/* Variable length fields follow:
	 * uint8_t baSourceID[bNrInPins];
	 * uint8_t bNrChannels;
	 * uint32_t bmChannelConfig;
	 * uint8_t iChannelNames;
	 * uint8_t bmControls;
	 * uint8_t iExtension;
	 */
} __packed;

/**
 * @brief Audio Streaming Interface Descriptor (4.9.2)
 */
struct uac2_as_general_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bTerminalLink;
	uint8_t bmControls;
	uint8_t bFormatType;
	uint32_t bmFormats;
	uint8_t bNrChannels;
	uint32_t bmChannelConfig;
	uint8_t iChannelNames;
} __packed;

/**
 * @brief common Format Type Descriptor
 */
struct uac2_format_type_common_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
} __packed;

/**
 * @brief Type I Format Type Descriptor (USB Audio Data Formats 2.3.1.6)
 */
struct uac2_format_type_i_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint8_t bSubslotSize;
	uint8_t bBitResolution;
} __packed;

/**
 * @brief Type II Format Type Descriptor (USB Audio Data Formats 2.3.2.6)
 */
struct uac2_format_type_ii_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint16_t wMaxBitRate;
	uint16_t wSlotsPerFrame;
} __packed;

/**
 * @brief Type III Format Type Descriptor (USB Audio Data Formats 2.3.3.1)
 */
struct uac2_format_type_iii_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint8_t bSubslotSize;
	uint8_t bBitResolution;
} __packed;

/**
 * @brief Type IV Format Type Descriptor (USB Audio Data Formats 2.3.4.1)
 */
struct uac2_format_type_iv_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
} __packed;

/**
 * @brief Class-Specific AS Isochronous Audio Data Endpoint Descriptor (4.10.1.2)
 */
struct uac2_cs_as_iso_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmAttributes;
	uint8_t bmControls;
	uint8_t bLockDelayUnits;
	uint16_t wLockDelay;
} __packed;

/**
 * @brief Audio 2.0 Layout 1 Parameter Block (1-byte CUR)
 * Reference: USB Audio 2.0 spec Table 5-2
 */
struct uac2_layout1_cur {
	uint8_t bCUR;
} __packed;

/**
 * @brief Audio 2.0 Layout 1 RANGE Parameter Block (1-byte controls)
 * Reference: USB Audio 2.0 spec Table 5-3
 */
struct uac2_layout1_range {
	uint16_t wNumSubRanges;
	uint8_t bMIN;
	uint8_t bMAX;
	uint8_t bRES;
} __packed;

/**
 * @brief Audio 2.0 Layout 2 Parameter Block (2-byte CUR)
 * Reference: USB Audio 2.0 spec Table 5-4
 */
struct uac2_layout2_cur {
	uint16_t wCUR;
} __packed;

/**
 * @brief Audio 2.0 Layout 2 RANGE Parameter Block (2-byte controls like volume)
 * Reference: USB Audio 2.0 spec Table 5-5
 */
struct uac2_layout2_range {
	uint16_t wNumSubRanges;
	int16_t wMIN;
	int16_t wMAX;
	int16_t wRES;
} __packed;

/**
 * @brief Audio 2.0 Layout 3 Parameter Block (4-byte CUR)
 * Reference: USB Audio 2.0 spec Table 5-6
 */
struct uac2_layout3_cur {
	uint32_t dCUR;
} __packed;

/**
 * @brief Audio 2.0 Layout 3 RANGE Parameter Block (4-byte controls like sample frequency)
 * Reference: USB Audio 2.0 spec Table 5-7
 */
struct uac2_layout3_range {
	uint16_t wNumSubRanges;
	uint32_t dMIN;
	uint32_t dMAX;
	uint32_t dRES;
} __packed;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_USB_UAC2_H_ */
