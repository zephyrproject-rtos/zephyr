/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Audio Class 2.0 Host API
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBH_UAC2_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBH_UAC2_H_

#include <zephyr/device.h>
#include "uac2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB Audio Class 2 host API
 * @defgroup usbh_uac2 USB Audio Class 2.0 Host API
 * @ingroup usb
 * @since 4.4
 * @version 0.1.0
 * @{
 */

/**
 * @brief Audio Control IDs (CIDs)
 *
 * Similar to Video CIDs in include/zephyr/drivers/video-controls.h
 * These IDs are user-facing and hide the internal UAC2 unit structure.
 */

/** Base value for audio control IDs */
#define AUDIO_CID_BASE					(0x00A00000)

/* Basic audio controls (most commonly used) */
/** Mute control */
#define AUDIO_CID_MUTE					(AUDIO_CID_BASE + 0)
/** Volume control */
#define AUDIO_CID_VOLUME				(AUDIO_CID_BASE + 1)
/** Bass control */
#define AUDIO_CID_BASS					(AUDIO_CID_BASE + 2)
/** Mid-range control */
#define AUDIO_CID_MID					(AUDIO_CID_BASE + 3)
/** Treble control */
#define AUDIO_CID_TREBLE				(AUDIO_CID_BASE + 4)
/** Graphic equalizer control */
#define AUDIO_CID_GRAPHIC_EQUALIZER			(AUDIO_CID_BASE + 5)
/** Automatic gain control */
#define AUDIO_CID_AUTO_GAIN				(AUDIO_CID_BASE + 6)
/** Delay control */
#define AUDIO_CID_DELAY					(AUDIO_CID_BASE + 7)
/** Bass boost control */
#define AUDIO_CID_BASS_BOOST				(AUDIO_CID_BASE + 8)
/** Loudness control */
#define AUDIO_CID_LOUDNESS				(AUDIO_CID_BASE + 9)
/** Input gain control */
#define AUDIO_CID_INPUT_GAIN				(AUDIO_CID_BASE + 10)
/** Input gain pad control */
#define AUDIO_CID_INPUT_GAIN_PAD			(AUDIO_CID_BASE + 11)
/** Phase inverter control */
#define AUDIO_CID_PHASE_INVERTER			(AUDIO_CID_BASE + 12)

/* Clock and timing controls */
/** Sample rate control */
#define AUDIO_CID_SAMPLE_RATE				(AUDIO_CID_BASE + 20)
/** Clock validity control */
#define AUDIO_CID_CLOCK_VALID				(AUDIO_CID_BASE + 21)
/** Clock selector control */
#define AUDIO_CID_CLOCK_SELECTOR			(AUDIO_CID_BASE + 22)
/** Clock multiplier numerator */
#define AUDIO_CID_CLOCK_MULTIPLIER_NUMERATOR		(AUDIO_CID_BASE + 23)
/** Clock multiplier denominator */
#define AUDIO_CID_CLOCK_MULTIPLIER_DENOMINATOR		(AUDIO_CID_BASE + 24)

/* Terminal/Connection controls */
/** Copy protection control */
#define AUDIO_CID_COPY_PROTECT				(AUDIO_CID_BASE + 30)
/** Connector control */
#define AUDIO_CID_CONNECTOR				(AUDIO_CID_BASE + 31)
/** Overload control */
#define AUDIO_CID_OVERLOAD				(AUDIO_CID_BASE + 32)
/** Cluster control */
#define AUDIO_CID_CLUSTER				(AUDIO_CID_BASE + 33)
/** Underflow control */
#define AUDIO_CID_UNDERFLOW				(AUDIO_CID_BASE + 34)
/** Overflow control */
#define AUDIO_CID_OVERFLOW				(AUDIO_CID_BASE + 35)
/** Latency control */
#define AUDIO_CID_LATENCY				(AUDIO_CID_BASE + 36)

/* Mixer controls */
/** Mixer control */
#define AUDIO_CID_MIXER					(AUDIO_CID_BASE + 40)
/** Mixer cluster control */
#define AUDIO_CID_MIXER_CLUSTER				(AUDIO_CID_BASE + 41)
/** Mixer underflow control */
#define AUDIO_CID_MIXER_UNDERFLOW			(AUDIO_CID_BASE + 42)
/** Mixer overflow control */
#define AUDIO_CID_MIXER_OVERFLOW			(AUDIO_CID_BASE + 43)
/** Mixer latency control */
#define AUDIO_CID_MIXER_LATENCY				(AUDIO_CID_BASE + 44)

/* Selector controls */
/** Selector control */
#define AUDIO_CID_SELECTOR				(AUDIO_CID_BASE + 50)
/** Selector latency control */
#define AUDIO_CID_SELECTOR_LATENCY			(AUDIO_CID_BASE + 51)

/* Parametric Equalizer controls */
/** Parametric equalizer enable */
#define AUDIO_CID_PE_ENABLE				(AUDIO_CID_BASE + 60)
/** Parametric equalizer center frequency */
#define AUDIO_CID_PE_CENTERFREQ				(AUDIO_CID_BASE + 61)
/** Parametric equalizer Q factor */
#define AUDIO_CID_PE_QFACTOR				(AUDIO_CID_BASE + 62)
/** Parametric equalizer gain */
#define AUDIO_CID_PE_GAIN				(AUDIO_CID_BASE + 63)

/* Reverb effect controls */
/** Reverb enable */
#define AUDIO_CID_REVERB_ENABLE				(AUDIO_CID_BASE + 70)
/** Reverb type */
#define AUDIO_CID_REVERB_TYPE				(AUDIO_CID_BASE + 71)
/** Reverb level */
#define AUDIO_CID_REVERB_LEVEL				(AUDIO_CID_BASE + 72)
/** Reverb time */
#define AUDIO_CID_REVERB_TIME				(AUDIO_CID_BASE + 73)
/** Reverb feedback */
#define AUDIO_CID_REVERB_FEEDBACK			(AUDIO_CID_BASE + 74)
/** Reverb pre-delay */
#define AUDIO_CID_REVERB_PREDELAY			(AUDIO_CID_BASE + 75)
/** Reverb density */
#define AUDIO_CID_REVERB_DENSITY			(AUDIO_CID_BASE + 76)
/** Reverb high-frequency rolloff */
#define AUDIO_CID_REVERB_HIFREQ_ROLLOFF			(AUDIO_CID_BASE + 77)

/* Chorus/Flanger effect controls */
/** Chorus enable */
#define AUDIO_CID_CHORUS_ENABLE				(AUDIO_CID_BASE + 80)
/** Chorus balance */
#define AUDIO_CID_CHORUS_BALANCE			(AUDIO_CID_BASE + 81)
/** Chorus rate */
#define AUDIO_CID_CHORUS_RATE				(AUDIO_CID_BASE + 82)
/** Chorus depth */
#define AUDIO_CID_CHORUS_DEPTH				(AUDIO_CID_BASE + 83)
/** Chorus time */
#define AUDIO_CID_CHORUS_TIME				(AUDIO_CID_BASE + 84)
/** Chorus feedback */
#define AUDIO_CID_CHORUS_FEEDBACK			(AUDIO_CID_BASE + 85)

/* Compressor effect controls */
/** Compressor enable */
#define AUDIO_CID_COMPRESSOR_ENABLE			(AUDIO_CID_BASE + 90)
/** Compressor ratio */
#define AUDIO_CID_COMPRESSOR_RATIO			(AUDIO_CID_BASE + 91)
/** Compressor maximum amplitude */
#define AUDIO_CID_COMPRESSOR_MAX_AMPLITUDE		(AUDIO_CID_BASE + 92)
/** Compressor threshold */
#define AUDIO_CID_COMPRESSOR_THRESHOLD			(AUDIO_CID_BASE + 93)
/** Compressor attack time */
#define AUDIO_CID_COMPRESSOR_ATTACK_TIME		(AUDIO_CID_BASE + 94)
/** Compressor release time */
#define AUDIO_CID_COMPRESSOR_RELEASE_TIME		(AUDIO_CID_BASE + 95)

/* Up/Down-mix processing controls */
/** Up/Down-mix enable */
#define AUDIO_CID_UD_ENABLE				(AUDIO_CID_BASE + 100)
/** Up/Down-mix mode select */
#define AUDIO_CID_UD_MODE_SELECT			(AUDIO_CID_BASE + 101)

/* Dolby Prologic processing controls */
/** Dolby Prologic enable */
#define AUDIO_CID_DOLBY_ENABLE				(AUDIO_CID_BASE + 110)
/** Dolby Prologic mode */
#define AUDIO_CID_DOLBY_MODE				(AUDIO_CID_BASE + 111)

/* Stereo Extender controls */
/** Stereo extender enable */
#define AUDIO_CID_STEREO_EXTENDER_ENABLE		(AUDIO_CID_BASE + 120)
/** Stereo extender width */
#define AUDIO_CID_STEREO_EXTENDER_WIDTH			(AUDIO_CID_BASE + 121)

/* Extension Unit controls */
/** Extension unit enable */
#define AUDIO_CID_XU_ENABLE				(AUDIO_CID_BASE + 130)

/* AudioStreaming Interface controls */
/** Active alternate setting */
#define AUDIO_CID_ACTIVE_ALT_SETTING			(AUDIO_CID_BASE + 140)
/** Valid alternate settings */
#define AUDIO_CID_VALID_ALT_SETTINGS			(AUDIO_CID_BASE + 141)
/** Audio data format */
#define AUDIO_CID_AUDIO_DATA_FORMAT			(AUDIO_CID_BASE + 142)

/* Encoder controls */
/** Encoder bit rate */
#define AUDIO_CID_ENCODER_BIT_RATE			(AUDIO_CID_BASE + 150)
/** Encoder quality */
#define AUDIO_CID_ENCODER_QUALITY			(AUDIO_CID_BASE + 151)
/** Encoder variable bit rate */
#define AUDIO_CID_ENCODER_VBR				(AUDIO_CID_BASE + 152)
/** Encoder type */
#define AUDIO_CID_ENCODER_TYPE				(AUDIO_CID_BASE + 153)
/** Encoder error */
#define AUDIO_CID_ENCODER_ERROR				(AUDIO_CID_BASE + 154)
/** Encoder parameter 1 */
#define AUDIO_CID_ENCODER_PARAM1			(AUDIO_CID_BASE + 155)
/** Encoder parameter 2 */
#define AUDIO_CID_ENCODER_PARAM2			(AUDIO_CID_BASE + 156)
/** Encoder parameter 3 */
#define AUDIO_CID_ENCODER_PARAM3			(AUDIO_CID_BASE + 157)
/** Encoder parameter 4 */
#define AUDIO_CID_ENCODER_PARAM4			(AUDIO_CID_BASE + 158)
/** Encoder parameter 5 */
#define AUDIO_CID_ENCODER_PARAM5			(AUDIO_CID_BASE + 159)
/** Encoder parameter 6 */
#define AUDIO_CID_ENCODER_PARAM6			(AUDIO_CID_BASE + 160)
/** Encoder parameter 7 */
#define AUDIO_CID_ENCODER_PARAM7			(AUDIO_CID_BASE + 161)
/** Encoder parameter 8 */
#define AUDIO_CID_ENCODER_PARAM8			(AUDIO_CID_BASE + 162)

/* MPEG Decoder controls */
/** MPEG dual channel */
#define AUDIO_CID_MPEG_DUAL_CHANNEL			(AUDIO_CID_BASE + 170)
/** MPEG second stereo */
#define AUDIO_CID_MPEG_SECOND_STEREO			(AUDIO_CID_BASE + 171)
/** MPEG multilingual */
#define AUDIO_CID_MPEG_MULTILINGUAL			(AUDIO_CID_BASE + 172)
/** MPEG dynamic range */
#define AUDIO_CID_MPEG_DYN_RANGE			(AUDIO_CID_BASE + 173)
/** MPEG scaling */
#define AUDIO_CID_MPEG_SCALING				(AUDIO_CID_BASE + 174)
/** MPEG high/low scaling */
#define AUDIO_CID_MPEG_HILO_SCALING			(AUDIO_CID_BASE + 175)
/** MPEG decoder error */
#define AUDIO_CID_MPEG_DECODER_ERROR			(AUDIO_CID_BASE + 176)

/* AC-3 Decoder controls */
/** AC-3 mode */
#define AUDIO_CID_AC3_MODE				(AUDIO_CID_BASE + 180)
/** AC-3 dynamic range */
#define AUDIO_CID_AC3_DYN_RANGE				(AUDIO_CID_BASE + 181)
/** AC-3 scaling */
#define AUDIO_CID_AC3_SCALING				(AUDIO_CID_BASE + 182)
/** AC-3 high/low scaling */
#define AUDIO_CID_AC3_HILO_SCALING			(AUDIO_CID_BASE + 183)
/** AC-3 decoder error */
#define AUDIO_CID_AC3_DECODER_ERROR			(AUDIO_CID_BASE + 184)

/* WMA Decoder controls */
/** WMA decoder error */
#define AUDIO_CID_WMA_DECODER_ERROR			(AUDIO_CID_BASE + 190)

/* DTS Decoder controls */
/** DTS decoder error */
#define AUDIO_CID_DTS_DECODER_ERROR			(AUDIO_CID_BASE + 200)

/* Endpoint controls */
/** Pitch control */
#define AUDIO_CID_PITCH					(AUDIO_CID_BASE + 210)
/** Data overrun */
#define AUDIO_CID_DATA_OVERRUN				(AUDIO_CID_BASE + 211)
/** Data underrun */
#define AUDIO_CID_DATA_UNDERRUN				(AUDIO_CID_BASE + 212)

/* Private/Vendor-specific controls */
/** Memory control */
#define AUDIO_CID_MEMORY				(AUDIO_CID_BASE + 0x1000)

/**
 * @brief Master channel identifier
 */
/** Master channel */
#define USBH_AUDIO_CHANNEL_MASTER			0

/** Device connected flag */
#define UAC2_DEVICE_FLAG_CONNECTED			BIT(0)
/** Device streaming IN flag */
#define UAC2_DEVICE_FLAG_STREAMING_IN			BIT(1)
/** Device streaming OUT flag */
#define UAC2_DEVICE_FLAG_STREAMING_OUT			BIT(2)

/** Encode terminal type */
#define TERMINAL_TYPE_ENCODE(capture_type, playback_type) \
	(((capture_type) << 4) | (playback_type))
/** Extract capture type */
#define TERMINAL_TYPE_CAPTURE(encoded)			(((encoded) >> 4) & 0x0F)
/** Extract playback type */
#define TERMINAL_TYPE_PLAYBACK(encoded)			((encoded) & 0x0F)
/** Check if terminal type is encoded */
#define TERMINAL_TYPE_IS_ENCODED(type)			(((type) & 0xF0) != 0)
/** Terminal not available */
#define TERMINAL_NA					0

/**
 * @brief Audio stream direction
 */
enum usbh_uac2_stream_dir {
	/** Capture (recording) direction */
	USBH_CAPTURE = 0,
	/** Playback direction */
	USBH_PLAYBACK = 1,
};

/**
 * @brief Audio control structure
 *
 * Used to get/set an audio control.
 * Similar to struct video_control in include/zephyr/drivers/video-controls.h
 *
 * Use signed variants (val8/val16/val32) for controls that can be negative
 * (e.g., volume, bass, treble). Use unsigned variants (uval8/uval16/uval32)
 * for controls that are always positive (e.g., sample rate, delay, enable).
 */
struct uac2_control {
	/** Control ID (AUDIO_CID_*) */
	uint32_t id;
	/** Control value union */
	union {
		/** Signed 8-bit value */
		int8_t   val8;
		/** Unsigned 8-bit value */
		uint8_t  uval8;
		/** Signed 16-bit value */
		int16_t  val16;
		/** Unsigned 16-bit value */
		uint16_t uval16;
		/** Signed 32-bit value */
		int32_t  val32;
		/** Unsigned 32-bit value */
		uint32_t uval32;
	};
};

/**
 * @brief Audio control subrange structure
 */
struct uac2_ctrl_subrange {
	/** Minimum value union */
	union {
		/** Signed 8-bit minimum */
		int8_t   min8;
		/** Unsigned 8-bit minimum */
		uint8_t  umin8;
		/** Signed 16-bit minimum */
		int16_t  min16;
		/** Unsigned 16-bit minimum */
		uint16_t umin16;
		/** Signed 32-bit minimum */
		int32_t  min32;
		/** Unsigned 32-bit minimum */
		uint32_t umin32;
	};
	/** Maximum value union */
	union {
		/** Signed 8-bit maximum */
		int8_t   max8;
		/** Unsigned 8-bit maximum */
		uint8_t  umax8;
		/** Signed 16-bit maximum */
		int16_t  max16;
		/** Unsigned 16-bit maximum */
		uint16_t umax16;
		/** Signed 32-bit maximum */
		int32_t  max32;
		/** Unsigned 32-bit maximum */
		uint32_t umax32;
	};
	/** Step value union */
	union {
		/** Signed 8-bit step */
		int8_t   step8;
		/** Unsigned 8-bit step */
		uint8_t  ustep8;
		/** Signed 16-bit step */
		int16_t  step16;
		/** Unsigned 16-bit step */
		uint16_t ustep16;
		/** Signed 32-bit step */
		int32_t  step32;
		/** Unsigned 32-bit step */
		uint32_t ustep32;
	};
};

/**
 * @brief Audio control range structure
 *
 * Describes range of a control. May contain multiple subranges
 * for controls with non-contiguous valid values.
 */
struct uac2_ctrl_range {
	/** Number of subranges */
	uint16_t num_subranges;
	/** Array of subranges */
	struct uac2_ctrl_subrange subranges[CONFIG_USBH_UAC2_MAX_RANGE_SUBRANGES];
};

/**
 * @brief Audio control query structure
 *
 * Used to query control capabilities and range information.
 */
struct uac2_ctrl_query {
	/** Control ID (AUDIO_CID_*) */
	uint32_t id;
	/** Control layout (1, 2, or 4 bytes) */
	uint8_t layout;
	/** Whether control values are signed */
	bool is_signed;
	/** Control range information */
	struct uac2_ctrl_range range;
};

/**
 * @brief Control mapping structure
 */
struct uac2_ctrl_mapping {
	/** Control ID (AUDIO_CID_*) */
	uint32_t cid;
	/** UAC2 control selector */
	uint8_t ctrl_selector;
	/** Control layout (1, 2, or 4 bytes) */
	uint8_t layout;
	/** Whether control values are signed */
	bool is_signed;
	/** UAC2 unit subtype */
	uint8_t unit_subtype;
};

/**
 * @brief Audio transfer callback function type
 *
 * For IN (recording): Called when audio data is received from device
 * For OUT (playback): Called to fill buffer with audio data to send
 *
 * @param data Pointer to audio data buffer
 * @param len For IN: number of bytes received; For OUT: max buffer size
 * @param user_data User data passed during stream start
 *
 * @return For IN: ignored; For OUT: number of bytes filled in buffer
 */
typedef uint16_t (*usbh_uac2_transfer_cb_t)(uint8_t *data, uint16_t len, void *user_data);

/**
 * @brief Start audio IN streaming (recording from device)
 *
 * @param dev UAC2 host device
 * @param callback Callback function to receive audio data
 * @param user_data User data to pass to callback
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_start_stream_in(const struct device *dev,
			       usbh_uac2_transfer_cb_t callback,
			       void *user_data);

/**
 * @brief Start audio OUT streaming (playback to device)
 *
 * @param dev UAC2 host device
 * @param callback Callback function to provide audio data
 * @param user_data User data to pass to callback
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_start_stream_out(const struct device *dev,
				usbh_uac2_transfer_cb_t callback,
				void *user_data);

/**
 * @brief Stop audio IN streaming
 *
 * @param dev UAC2 host device
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_stop_stream_in(const struct device *dev);

/**
 * @brief Stop audio OUT streaming
 *
 * @param dev UAC2 host device
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_stop_stream_out(const struct device *dev);

/**
 * @brief Get current audio format
 *
 * @param dev UAC2 host device
 * @param stream_dir Stream direction (USBH_CAPTURE or USBH_PLAYBACK)
 * @param sample_rate Pointer to store sample rate (can be NULL)
 * @param channels Pointer to store channels (can be NULL)
 * @param bit_resolution Pointer to store bit resolution (can be NULL)
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_get_format(const struct device *dev,
			 enum usbh_uac2_stream_dir stream_dir,
			 uint32_t *sample_rate,
			 uint8_t *channels,
			 uint8_t *bit_resolution);

/**
 * @brief Get audio control value
 *
 * @param dev UAC2 host device
 * @param dir Stream direction
 * @param channel Channel number (0 for master/clock controls)
 * @param ctrl Control structure (id as input, value as output)
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_ctrl_get(const struct device *dev,
		       enum usbh_uac2_stream_dir dir,
		       uint8_t channel,
		       struct uac2_control *ctrl);

/**
 * @brief Set audio control value
 *
 * @param dev UAC2 host device
 * @param dir Stream direction
 * @param channel Channel number (0 for master/clock controls)
 * @param ctrl Control structure (id and value as input)
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_ctrl_set(const struct device *dev,
		       enum usbh_uac2_stream_dir dir,
		       uint8_t channel,
		       struct uac2_control *ctrl);

/**
 * @brief Query audio control capabilities
 *
 * Gets control metadata (layout, signedness) and range information.
 * The query structure should have the 'id' field set on input.
 * On success, layout, is_signed, and range fields are filled.
 *
 * @param dev UAC2 host device
 * @param dir Stream direction
 * @param channel Channel number (0 for master/clock controls)
 * @param query Query structure (id as input, other fields as output)
 *
 * @return 0 on success, negative errno on error
 */
int usbh_uac2_ctrl_query(const struct device *dev,
			 enum usbh_uac2_stream_dir dir,
			 uint8_t channel,
			 struct uac2_ctrl_query *query);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_UAC2_H_ */
