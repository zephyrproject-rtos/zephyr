/** @file
 *  @brief Bluetooth Audio handling
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2020-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AUDIO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AUDIO_H_

/**
 * @brief Bluetooth Audio
 * @defgroup bt_audio Bluetooth Audio
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/lc3.h>


#ifdef __cplusplus
extern "C" {
#endif

#define BT_AUDIO_BROADCAST_ID_SIZE               3 /* octets */
/** Maximum broadcast ID value */
#define BT_AUDIO_BROADCAST_ID_MAX                0xFFFFFFU
/** Indicates that the server have no preference for the presentation delay */
#define BT_AUDIO_PD_PREF_NONE                    0x000000U
/** Maximum presentation delay in microseconds */
#define BT_AUDIO_PD_MAX                          0xFFFFFFU

#define BT_AUDIO_BROADCAST_CODE_SIZE             16

/** @brief Audio Context Type for Generic Audio
 *
 * These values are defined by the Generic Audio Assigned Numbers, bluetooth.com
 */
enum bt_audio_context {
	BT_AUDIO_CONTEXT_TYPE_PROHIBITED = 0,
	BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED = BIT(0),
	BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL = BIT(1),
	BT_AUDIO_CONTEXT_TYPE_MEDIA = BIT(2),
	BT_AUDIO_CONTEXT_TYPE_GAME = BIT(3),
	BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL = BIT(4),
	BT_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS = BIT(5),
	BT_AUDIO_CONTEXT_TYPE_LIVE = BIT(6),
	BT_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS = BIT(7),
	BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS = BIT(8),
	BT_AUDIO_CONTEXT_TYPE_RINGTONE = BIT(9),
	BT_AUDIO_CONTEXT_TYPE_ALERTS = BIT(10),
	BT_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM = BIT(11),
};

/**
 * Any known context.
 */
#define BT_AUDIO_CONTEXT_TYPE_ANY	 (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
					  BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
					  BT_AUDIO_CONTEXT_TYPE_MEDIA | \
					  BT_AUDIO_CONTEXT_TYPE_GAME | \
					  BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL | \
					  BT_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS | \
					  BT_AUDIO_CONTEXT_TYPE_LIVE | \
					  BT_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS | \
					  BT_AUDIO_CONTEXT_TYPE_NOTIFICATIONS | \
					  BT_AUDIO_CONTEXT_TYPE_RINGTONE | \
					  BT_AUDIO_CONTEXT_TYPE_ALERTS | \
					  BT_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM)

/**
 * @brief Parental rating defined by the Generic Audio assigned numbers (bluetooth.com).
 *
 * The numbering scheme is aligned with Annex F of EN 300 707 v1.2.1 which
 * defined parental rating for viewing.
 */
enum bt_audio_parental_rating {
	BT_AUDIO_PARENTAL_RATING_NO_RATING        = 0x00,
	BT_AUDIO_PARENTAL_RATING_AGE_ANY          = 0x01,
	BT_AUDIO_PARENTAL_RATING_AGE_5_OR_ABOVE   = 0x02,
	BT_AUDIO_PARENTAL_RATING_AGE_6_OR_ABOVE   = 0x03,
	BT_AUDIO_PARENTAL_RATING_AGE_7_OR_ABOVE   = 0x04,
	BT_AUDIO_PARENTAL_RATING_AGE_8_OR_ABOVE   = 0x05,
	BT_AUDIO_PARENTAL_RATING_AGE_9_OR_ABOVE   = 0x06,
	BT_AUDIO_PARENTAL_RATING_AGE_10_OR_ABOVE  = 0x07,
	BT_AUDIO_PARENTAL_RATING_AGE_11_OR_ABOVE  = 0x08,
	BT_AUDIO_PARENTAL_RATING_AGE_12_OR_ABOVE  = 0x09,
	BT_AUDIO_PARENTAL_RATING_AGE_13_OR_ABOVE  = 0x0A,
	BT_AUDIO_PARENTAL_RATING_AGE_14_OR_ABOVE  = 0x0B,
	BT_AUDIO_PARENTAL_RATING_AGE_15_OR_ABOVE  = 0x0C,
	BT_AUDIO_PARENTAL_RATING_AGE_16_OR_ABOVE  = 0x0D,
	BT_AUDIO_PARENTAL_RATING_AGE_17_OR_ABOVE  = 0x0E,
	BT_AUDIO_PARENTAL_RATING_AGE_18_OR_ABOVE  = 0x0F
};

/** @brief Audio Active State defined by the Generic Audio assigned numbers (bluetooth.com). */
enum bt_audio_active_state {
	BT_AUDIO_ACTIVE_STATE_DISABLED       = 0x00,
	BT_AUDIO_ACTIVE_STATE_ENABLED        = 0x01,
};

/**
 * @brief Codec metadata type IDs
 *
 * Metadata types defined by the Generic Audio assigned numbers (bluetooth.com).
 */
enum bt_audio_metadata_type {
	/** @brief Preferred audio context.
	 *
	 * Bitfield of preferred audio contexts.
	 *
	 * If 0, the context type is not a preferred use case for this codec
	 * configuration.
	 *
	 * See the BT_AUDIO_CONTEXT_* for valid values.
	 */
	BT_AUDIO_METADATA_TYPE_PREF_CONTEXT        = 0x01,

	/** @brief Streaming audio context.
	 *
	 * Bitfield of streaming audio contexts.
	 *
	 * If 0, the context type is not a preferred use case for this codec
	 * configuration.
	 *
	 * See the BT_AUDIO_CONTEXT_* for valid values.
	 */
	BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT      = 0x02,

	/** UTF-8 encoded title or summary of stream content */
	BT_AUDIO_METADATA_TYPE_PROGRAM_INFO        = 0x03,

	/** @brief Stream language
	 *
	 * 3 octet lower case language code defined by ISO 639-3
	 */
	BT_AUDIO_METADATA_TYPE_STREAM_LANG         = 0x04,

	/** Array of 8-bit CCID values */
	BT_AUDIO_METADATA_TYPE_CCID_LIST           = 0x05,

	/** @brief Parental rating
	 *
	 * See @ref bt_audio_parental_rating for valid values.
	 */
	BT_AUDIO_METADATA_TYPE_PARENTAL_RATING     = 0x06,

	/** UTF-8 encoded URI for additional Program information */
	BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI    = 0x07,

	/** @brief Audio active state
	 *
	 * See @ref bt_audio_active_state for valid values.
	 */
	BT_AUDIO_METADATA_TYPE_AUDIO_STATE         = 0x08,

	/** Broadcast Audio Immediate Rendering flag  */
	BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE = 0x09,

	/** Extended metadata */
	BT_AUDIO_METADATA_TYPE_EXTENDED            = 0xFE,

	/** Vendor specific metadata */
	BT_AUDIO_METADATA_TYPE_VENDOR              = 0xFF,
};

/* Unicast Announcement Type, Generic Audio */
#define BT_AUDIO_UNICAST_ANNOUNCEMENT_GENERAL    0x00
#define BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED   0x01

/** @brief Codec configuration structure */
struct bt_codec_data {
	struct bt_data data;
#if defined(CONFIG_BT_CODEC_MAX_DATA_LEN)
	uint8_t  value[CONFIG_BT_CODEC_MAX_DATA_LEN];
#endif /* CONFIG_BT_CODEC_MAX_DATA_LEN */
};

/**
 *  @brief Helper to declare elements of bt_codec_data arrays
 *
 *  This macro is mainly for creating an array of struct bt_codec_data
 *  elements inside bt_codec which is then passed to the likes of
 *  bt_bap_stream_config or bt_bap_stream_reconfig.
 *
 *  @param _type Type of advertising data field
 *  @param _bytes Variable number of single-byte parameters
 */
#define BT_CODEC_DATA(_type, _bytes...) \
	{ \
		.data = BT_DATA(_type, ((uint8_t []) { _bytes }), \
				sizeof((uint8_t []) { _bytes })) \
	}

/**
 *  @brief Helper to declare bt_codec structure
 *
 *  @param _id Codec ID
 *  @param _cid Company ID
 *  @param _vid Vendor ID
 *  @param _data Codec Specific Data in LVT format
 *  @param _meta Codec Specific Metadata in LVT format
 */
#define BT_CODEC(_id, _cid, _vid, _data, _meta) \
	{ \
		/* Use HCI data path as default, can be overwritten by application */ \
		.path_id = BT_ISO_DATA_PATH_HCI, \
		.id = _id, \
		.cid = _cid, \
		.vid = _vid, \
		.data_count = ARRAY_SIZE(((struct bt_codec_data[]) _data)), \
		.data = _data, \
		.meta_count = ARRAY_SIZE(((struct bt_codec_data[]) _meta)), \
		.meta = _meta, \
	}

/** @brief Location values for BT Audio.
 *
 * These values are defined by the Generic Audio Assigned Numbers, bluetooth.com
 */
enum bt_audio_location {
	BT_AUDIO_LOCATION_PROHIBITED = 0,
	BT_AUDIO_LOCATION_FRONT_LEFT = BIT(0),
	BT_AUDIO_LOCATION_FRONT_RIGHT = BIT(1),
	BT_AUDIO_LOCATION_FRONT_CENTER = BIT(2),
	BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_1 = BIT(3),
	BT_AUDIO_LOCATION_BACK_LEFT = BIT(4),
	BT_AUDIO_LOCATION_BACK_RIGHT = BIT(5),
	BT_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER = BIT(6),
	BT_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER = BIT(7),
	BT_AUDIO_LOCATION_BACK_CENTER = BIT(8),
	BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_2 = BIT(9),
	BT_AUDIO_LOCATION_SIDE_LEFT = BIT(10),
	BT_AUDIO_LOCATION_SIDE_RIGHT = BIT(11),
	BT_AUDIO_LOCATION_TOP_FRONT_LEFT = BIT(12),
	BT_AUDIO_LOCATION_TOP_FRONT_RIGHT = BIT(13),
	BT_AUDIO_LOCATION_TOP_FRONT_CENTER = BIT(14),
	BT_AUDIO_LOCATION_TOP_CENTER = BIT(15),
	BT_AUDIO_LOCATION_TOP_BACK_LEFT = BIT(16),
	BT_AUDIO_LOCATION_TOP_BACK_RIGHT = BIT(17),
	BT_AUDIO_LOCATION_TOP_SIDE_LEFT = BIT(18),
	BT_AUDIO_LOCATION_TOP_SIDE_RIGHT = BIT(19),
	BT_AUDIO_LOCATION_TOP_BACK_CENTER = BIT(20),
	BT_AUDIO_LOCATION_BOTTOM_FRONT_CENTER = BIT(21),
	BT_AUDIO_LOCATION_BOTTOM_FRONT_LEFT = BIT(22),
	BT_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT = BIT(23),
	BT_AUDIO_LOCATION_FRONT_LEFT_WIDE = BIT(24),
	BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE = BIT(25),
	BT_AUDIO_LOCATION_LEFT_SURROUND = BIT(26),
	BT_AUDIO_LOCATION_RIGHT_SURROUND = BIT(27),
};

/**
 * Any known location.
 */
#define BT_AUDIO_LOCATION_ANY (BT_AUDIO_LOCATION_FRONT_LEFT | \
			       BT_AUDIO_LOCATION_FRONT_RIGHT | \
			       BT_AUDIO_LOCATION_FRONT_CENTER | \
			       BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_1 | \
			       BT_AUDIO_LOCATION_BACK_LEFT | \
			       BT_AUDIO_LOCATION_BACK_RIGHT | \
			       BT_AUDIO_LOCATION_FRONT_LEFT_OF_CENTER | \
			       BT_AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER | \
			       BT_AUDIO_LOCATION_BACK_CENTER | \
			       BT_AUDIO_LOCATION_LOW_FREQ_EFFECTS_2 | \
			       BT_AUDIO_LOCATION_SIDE_LEFT | \
			       BT_AUDIO_LOCATION_SIDE_RIGHT | \
			       BT_AUDIO_LOCATION_TOP_FRONT_LEFT | \
			       BT_AUDIO_LOCATION_TOP_FRONT_RIGHT | \
			       BT_AUDIO_LOCATION_TOP_FRONT_CENTER | \
			       BT_AUDIO_LOCATION_TOP_CENTER | \
			       BT_AUDIO_LOCATION_TOP_BACK_LEFT | \
			       BT_AUDIO_LOCATION_TOP_BACK_RIGHT | \
			       BT_AUDIO_LOCATION_TOP_SIDE_LEFT | \
			       BT_AUDIO_LOCATION_TOP_SIDE_RIGHT | \
			       BT_AUDIO_LOCATION_TOP_BACK_CENTER | \
			       BT_AUDIO_LOCATION_BOTTOM_FRONT_CENTER | \
			       BT_AUDIO_LOCATION_BOTTOM_FRONT_LEFT | \
			       BT_AUDIO_LOCATION_BOTTOM_FRONT_RIGHT | \
			       BT_AUDIO_LOCATION_FRONT_LEFT_WIDE | \
			       BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE | \
			       BT_AUDIO_LOCATION_LEFT_SURROUND | \
			       BT_AUDIO_LOCATION_RIGHT_SURROUND)

/** @brief Codec structure. */
struct bt_codec {
	/** Data path ID
	 *
	 * @ref BT_ISO_DATA_PATH_HCI for HCI path, or any other value for
	 * vendor specific ID.
	 */
	uint8_t path_id;
	/** Codec ID */
	uint8_t  id;
	/** Codec Company ID */
	uint16_t cid;
	/** Codec Company Vendor ID */
	uint16_t vid;
#if defined(CONFIG_BT_CODEC_MAX_DATA_COUNT)
	/** Codec Specific Data count */
	size_t   data_count;
	/** Codec Specific Data */
	struct bt_codec_data data[CONFIG_BT_CODEC_MAX_DATA_COUNT];
#endif /* CONFIG_BT_CODEC_MAX_DATA_COUNT */
#if defined(CONFIG_BT_CODEC_MAX_METADATA_COUNT)
	/** Codec Specific Metadata count */
	size_t   meta_count;
	/** Codec Specific Metadata */
	struct bt_codec_data meta[CONFIG_BT_CODEC_MAX_METADATA_COUNT];
#endif /* CONFIG_BT_CODEC_MAX_METADATA_COUNT */
};

/** @brief Audio Capability type */
enum bt_audio_dir {
	BT_AUDIO_DIR_SINK = 0x01,
	BT_AUDIO_DIR_SOURCE = 0x02,
};

/**
 *  @brief Helper to declare elements of bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _framing Framing
 *  @param _phy Target PHY
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS(_interval, _framing, _phy, _sdu, _rtn, _latency, \
		     _pd) \
	{ \
		.interval = _interval, \
		.framing = _framing, \
		.phy = _phy, \
		.sdu = _sdu, \
		.rtn = _rtn, \
		.latency = _latency, \
		.pd = _pd, \
	}

/** @brief Codec QoS Framing */
enum {
	BT_CODEC_QOS_UNFRAMED = 0x00,
	BT_CODEC_QOS_FRAMED = 0x01,
};

/** @brief Codec QoS Preferred PHY */
enum {
	BT_CODEC_QOS_1M = BIT(0),
	BT_CODEC_QOS_2M = BIT(1),
	BT_CODEC_QOS_CODED = BIT(2),
};

/**
 *  @brief Helper to declare Input Unframed bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_UNFRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(_interval, BT_CODEC_QOS_UNFRAMED, BT_CODEC_QOS_2M, _sdu, \
		     _rtn, _latency, _pd)

/**
 *  @brief Helper to declare Input Framed bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_FRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(_interval, BT_CODEC_QOS_FRAMED, BT_CODEC_QOS_2M, _sdu, \
		     _rtn, _latency, _pd)

/** @brief Codec QoS structure. */
struct bt_codec_qos {
	/** QoS PHY */
	uint8_t  phy;

	/** QoS Framing */
	uint8_t  framing;

	/** QoS Retransmission Number */
	uint8_t rtn;

	/** QoS SDU */
	uint16_t sdu;

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE) || defined(CONFIG_BT_BAP_UNICAST)
	/**
	 * @brief QoS Transport Latency
	 *
	 * Not used for the @kconfig{CONFIG_BT_BAP_BROADCAST_SINK} role.
	 */
	uint16_t latency;
#endif /*  CONFIG_BT_BAP_BROADCAST_SOURCE || CONFIG_BT_BAP_UNICAST */

	/** QoS Frame Interval */
	uint32_t interval;

	/** @brief QoS Presentation Delay in microseconds
	 *
	 *  Value range 0 to @ref BT_AUDIO_PD_MAX.
	 */
	uint32_t pd;
};

/**
 *  @brief Helper to declare elements of @ref bt_codec_qos_pref
 *
 *  @param _unframed_supported Unframed PDUs supported
 *  @param _phy Preferred Target PHY
 *  @param _rtn Preferred Retransmission number
 *  @param _latency Preferred Maximum Transport Latency (msec)
 *  @param _pd_min Minimum Presentation Delay (usec)
 *  @param _pd_max Maximum Presentation Delay (usec)
 *  @param _pref_pd_min Preferred Minimum Presentation Delay (usec)
 *  @param _pref_pd_max Preferred Maximum Presentation Delay (usec)
 */
#define BT_CODEC_QOS_PREF(_unframed_supported, _phy, _rtn, _latency, _pd_min, \
			  _pd_max, _pref_pd_min, _pref_pd_max) \
	{ \
		.unframed_supported = _unframed_supported, \
		.phy = _phy, \
		.rtn = _rtn, \
		.latency = _latency, \
		.pd_min = _pd_min, \
		.pd_max = _pd_max, \
		.pref_pd_min = _pref_pd_min, \
		.pref_pd_max = _pref_pd_max, \
	}

/** @brief Audio Stream Quality of Service Preference structure. */
struct bt_codec_qos_pref {
	/** @brief Unframed PDUs supported
	 *
	 *  Unlike the other fields, this is not a preference but whether
	 *  the codec supports unframed ISOAL PDUs.
	 */
	bool unframed_supported;

	/** Preferred PHY */
	uint8_t phy;

	/** Preferred Retransmission Number */
	uint8_t rtn;

	/** Preferred Transport Latency */
	uint16_t latency;

	/** @brief Minimum Presentation Delay in microseconds
	 *
	 *  Unlike the other fields, this is not a preference but a minimum
	 *  requirement.
	 *
	 *  Value range 0 to @ref BT_AUDIO_PD_MAX, or @ref BT_AUDIO_PD_PREF_NONE
	 *  to indicate no preference.
	 */
	uint32_t pd_min;

	/** @brief Maximum Presentation Delay
	 *
	 *  Unlike the other fields, this is not a preference but a maximum
	 *  requirement.
	 *
	 *  Value range 0 to @ref BT_AUDIO_PD_MAX, or @ref BT_AUDIO_PD_PREF_NONE
	 *  to indicate no preference.
	 */
	uint32_t pd_max;

	/** @brief Preferred minimum Presentation Delay
	 *
	 *  Value range 0 to @ref BT_AUDIO_PD_MAX.
	 */
	uint32_t pref_pd_min;

	/** @brief Preferred maximum Presentation Delay
	 *
	 *  Value range 0 to @ref BT_AUDIO_PD_MAX.
	 */
	uint32_t pref_pd_max;
};

/**
 * @brief Audio codec Config APIs
 * @defgroup bt_audio_codec_cfg Codec config parsing APIs
 *
 * Functions to parse codec config data when formatted as LTV wrapped into @ref bt_codec.
 *
 * @{
 */

/**
 * @brief Codec parser error codes for @ref bt_audio_codec_cfg.
 */
enum bt_audio_codec_parse_err {

	/** @brief The requested type is not present in the data set. */
	BT_AUDIO_CODEC_PARSE_ERR_SUCCESS               = 0,

	/** @brief The requested type is not present in the data set. */
	BT_AUDIO_CODEC_PARSE_ERR_TYPE_NOT_FOUND        = -1,

	/** @brief The value found is invalid. */
	BT_AUDIO_CODEC_PARSE_ERR_INVALID_VALUE_FOUND   = -2,

	/** @brief The parameters specified to the function call are not valid. */
	BT_AUDIO_CODEC_PARSE_ERR_INVALID_PARAM         = -3,
};

/**@brief Extract the frequency from a codec configuration.
 *
 * @param codec The codec configuration to extract data from.
 *
 * @return The frequency in Hz if found else a negative value of type
 *         @ref bt_audio_codec_parse_err.
 */
int bt_codec_cfg_get_freq(const struct bt_codec *codec);

/** @brief Extract frame duration from BT codec config
 *
 *  @param codec The codec configuration to extract data from.
 *
 *  @return Frame duration in microseconds if value is found else a negative value
 *          of type @ref bt_audio_codec_parse_err.
 */
int bt_codec_cfg_get_frame_duration_us(const struct bt_codec *codec);

/** @brief Extract channel allocation from BT codec config
 *
 *  The value returned is a bit field representing one or more audio locations as
 *  specified by @ref bt_audio_location
 *  Shall match one or more of the bits set in BT_PAC_SNK_LOC/BT_PAC_SRC_LOC.
 *
 *  Up to the configured @ref BT_CODEC_LC3_CHAN_COUNT number of channels can be present.
 *
 *  @param codec The codec configuration to extract data from.
 *  @param chan_allocation Pointer to the variable to store the extracted value in.
 *
 *  @return BT_AUDIO_CODEC_PARSE_SUCCESS if value is found and stored in the pointer provided
 *          else a negative value of type @ref bt_audio_codec_parse_err.
 */
int bt_codec_cfg_get_chan_allocation_val(const struct bt_codec *codec, uint32_t *chan_allocation);

/** @brief Extract frame size in octets from BT codec config
 *
 * The overall SDU size will be octets_per_frame * blocks_per_sdu.
 *
 *  The Bluetooth specificationa are not clear about this value - it does not state that
 *  the codec shall use this SDU size only. A codec like LC3 supports variable bit-rate
 *  (per SDU) hence it might be allowed for an encoder to reduce the frame size below this
 *  value.
 *  Hence it is recommended to use the received SDU size and divide by
 *  blocks_per_sdu rather than relying on this octets_per_sdu value to be fixed.
 *
 *  @param codec The codec configuration to extract data from.
 *
 *  @return Frame length in octets if value is found else a negative value
 *          of type @ref bt_audio_codec_parse_err.
 */
int bt_codec_cfg_get_octets_per_frame(const struct bt_codec *codec);

/** @brief Extract number of audio frame blockss in each SDU from BT codec config
 *
 *  The overall SDU size will be octets_per_frame * frame_blocks_per_sdu * number-of-channels.
 *
 *  If this value is not present a default value of 1 shall be used.
 *
 *  A frame block is one or more frames that represents data for the same period of time but
 *  for different channels. If the stream have two audio channels and this value is two
 *  there will be four frames in the SDU.
 *
 *  @param codec The codec configuration to extract data from.
 *  @param fallback_to_default If true this function will return the default value of 1
 *         if the type is not found. In this case the function will only fail if a NULL
 *         pointer is provided.
 *
 *  @return The count of codec frames in each SDU if value is found else a negative value
 *          of type @ref bt_audio_codec_parse_err - unless when \p fallback_to_default is true
 *          then the value 1 is returned if frames per sdu is not found.
 */
int bt_codec_cfg_get_frame_blocks_per_sdu(const struct bt_codec *codec, bool fallback_to_default);

/** @brief Lookup a specific value based on type
 *
 *  Depending on context bt_codec will be either codec capabilities, codec configuration or
 *  meta data.
 *
 *  Typically types used are:
 *  @ref bt_codec_capability_type
 *  @ref bt_codec_config_type
 *  @ref bt_audio_metadata_type
 *
 *  @param codec The codec data to search in.
 *  @param type The type id to look for
 *  @param data Pointer to the data-pointer to update when item is found
 *  @return True if the type is found, false otherwise.
 */
bool bt_codec_get_val(const struct bt_codec *codec,
		      uint8_t type,
		      const struct bt_codec_data **data);

/** @} */ /* End of bt_audio_codec_cfg */

#ifdef __cplusplus
}
#endif

/** @} */ /* end of bt_audio */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_H_ */
