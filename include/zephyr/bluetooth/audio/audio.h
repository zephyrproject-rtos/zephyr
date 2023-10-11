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

/**
 * Helper to check whether metadata type is known by the stack.
 */
#define BT_AUDIO_METADATA_TYPE_IS_KNOWN(_type) \
	(IN_RANGE(_type, BT_AUDIO_METADATA_TYPE_PREF_CONTEXT, \
			 BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE) || \
	 IN_RANGE(_type, BT_AUDIO_METADATA_TYPE_EXTENDED, BT_AUDIO_METADATA_TYPE_VENDOR))

/* Unicast Announcement Type, Generic Audio */
#define BT_AUDIO_UNICAST_ANNOUNCEMENT_GENERAL    0x00
#define BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED   0x01

/**
 * @brief Helper to declare elements of bt_audio_codec_cap arrays
 *
 * This macro is mainly for creating an array of struct bt_audio_codec_cap data arrays.
 *
 * @param _type Type of advertising data field
 * @param _bytes Variable number of single-byte parameters
 */
#define BT_AUDIO_CODEC_DATA(_type, _bytes...)                                                      \
	(sizeof((uint8_t)_type) + sizeof((uint8_t[]){_bytes})), (_type), _bytes

/**
 *  @brief Helper to declare @ref bt_audio_codec_cfg
 *
 *  @param _id Codec ID
 *  @param _cid Company ID
 *  @param _vid Vendor ID
 *  @param _data Codec Specific Data in LVT format
 *  @param _meta Codec Specific Metadata in LVT format
 */
#define BT_AUDIO_CODEC_CFG(_id, _cid, _vid, _data, _meta)                                          \
	((struct bt_audio_codec_cfg){                                                              \
		/* Use HCI data path as default, can be overwritten by application */              \
		.path_id = BT_ISO_DATA_PATH_HCI,                                                   \
		.id = _id,                                                                         \
		.cid = _cid,                                                                       \
		.vid = _vid,                                                                       \
		.data_len = sizeof((uint8_t[])_data),                                              \
		.data = _data,                                                                     \
		.meta_len = sizeof((uint8_t[])_meta),                                              \
		.meta = _meta,                                                                     \
	})

/**
 *  @brief Helper to declare @ref bt_audio_codec_cap structure
 *
 *  @param _id Codec ID
 *  @param _cid Company ID
 *  @param _vid Vendor ID
 *  @param _data Codec Specific Data in LVT format
 *  @param _meta Codec Specific Metadata in LVT format
 */
#define BT_AUDIO_CODEC_CAP(_id, _cid, _vid, _data, _meta)                                          \
	((struct bt_audio_codec_cap){                                                              \
		/* Use HCI data path as default, can be overwritten by application */              \
		.path_id = BT_ISO_DATA_PATH_HCI,                                                   \
		.id = (_id),                                                                       \
		.cid = (_cid),                                                                     \
		.vid = (_vid),                                                                     \
		.data_len = sizeof((uint8_t[])_data),                                              \
		.data = _data,                                                                     \
		.meta_len = sizeof((uint8_t[])_meta),                                              \
		.meta = _meta,                                                                     \
	})

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

/** @brief Codec capability structure. */
struct bt_audio_codec_cap {
	/** Data path ID
	 *
	 * @ref BT_ISO_DATA_PATH_HCI for HCI path, or any other value for
	 * vendor specific ID.
	 */
	uint8_t path_id;
	/** Codec ID */
	uint8_t id;
	/** Codec Company ID */
	uint16_t cid;
	/** Codec Company Vendor ID */
	uint16_t vid;
#if CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0
	/** Codec Specific Capabilities Data count */
	size_t data_len;
	/** Codec Specific Capabilities Data */
	uint8_t data[CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE];
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE > 0 */
#if defined(CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE)
	/** Codec Specific Capabilities Metadata count */
	size_t meta_len;
	/** Codec Specific Capabilities Metadata */
	uint8_t meta[CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE];
#endif /* CONFIG_BT_AUDIO_CODEC_CAP_MAX_METADATA_SIZE */
};

/** @brief Codec specific configuration structure. */
struct bt_audio_codec_cfg {
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
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0
	/** Codec Specific Capabilities Data count */
	size_t data_len;
	/** Codec Specific Capabilities Data */
	uint8_t data[CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE];
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_DATA_SIZE > 0 */
#if CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0
	/** Codec Specific Capabilities Metadata count */
	size_t meta_len;
	/** Codec Specific Capabilities Metadata */
	uint8_t meta[CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE];
#endif /* CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE > 0 */
};

/**
 * @brief Helper for parsing length-type-value data.
 *
 * @param ltv       Length-type-value (LTV) encoded data.
 * @param size      Size of the @p ltv data.
 * @param func      Callback function which will be called for each element
 *                  that's found in the data. The callback should return
 *                  true to continue parsing, or false to stop parsing.
 * @param user_data User data to be passed to the callback.
 *
 * @retval 0 if all entries were parsed.
 * @retval -EINVAL if the data is incorrectly encoded
 * @retval -ECANCELED if parsing was prematurely cancelled by the callback
 */
int bt_audio_data_parse(const uint8_t ltv[], size_t size,
			bool (*func)(struct bt_data *data, void *user_data), void *user_data);

/** @brief Audio Capability type */
enum bt_audio_dir {
	BT_AUDIO_DIR_SINK = 0x01,
	BT_AUDIO_DIR_SOURCE = 0x02,
};

/**
 *  @brief Helper to declare elements of bt_audio_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _framing Framing
 *  @param _phy Target PHY
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_AUDIO_CODEC_QOS(_interval, _framing, _phy, _sdu, _rtn, _latency, _pd)                   \
	((struct bt_audio_codec_qos){                                                              \
		.interval = _interval,                                                             \
		.framing = _framing,                                                               \
		.phy = _phy,                                                                       \
		.sdu = _sdu,                                                                       \
		.rtn = _rtn,                                                                       \
		.latency = _latency,                                                               \
		.pd = _pd,                                                                         \
	})

/** @brief Codec QoS Framing */
enum bt_audio_codec_qos_framing {
	BT_AUDIO_CODEC_QOS_FRAMING_UNFRAMED = 0x00,
	BT_AUDIO_CODEC_QOS_FRAMING_FRAMED = 0x01,
};

/** @brief Codec QoS Preferred PHY */
enum {
	BT_AUDIO_CODEC_QOS_1M = BIT(0),
	BT_AUDIO_CODEC_QOS_2M = BIT(1),
	BT_AUDIO_CODEC_QOS_CODED = BIT(2),
};

/**
 *  @brief Helper to declare Input Unframed bt_audio_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_AUDIO_CODEC_QOS_UNFRAMED(_interval, _sdu, _rtn, _latency, _pd)                          \
	BT_AUDIO_CODEC_QOS(_interval, BT_AUDIO_CODEC_QOS_FRAMING_UNFRAMED, BT_AUDIO_CODEC_QOS_2M,  \
			   _sdu, _rtn, _latency, _pd)

/**
 *  @brief Helper to declare Input Framed bt_audio_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_AUDIO_CODEC_QOS_FRAMED(_interval, _sdu, _rtn, _latency, _pd)                            \
	BT_AUDIO_CODEC_QOS(_interval, BT_AUDIO_CODEC_QOS_FRAMING_FRAMED, BT_AUDIO_CODEC_QOS_2M,    \
			   _sdu, _rtn, _latency, _pd)

/** @brief Codec QoS structure. */
struct bt_audio_codec_qos {
	/** QoS PHY */
	uint8_t  phy;

	/** QoS Framing */
	enum bt_audio_codec_qos_framing framing;

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
 *  @brief Helper to declare elements of @ref bt_audio_codec_qos_pref
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
#define BT_AUDIO_CODEC_QOS_PREF(_unframed_supported, _phy, _rtn, _latency, _pd_min, _pd_max,       \
				_pref_pd_min, _pref_pd_max)                                        \
	{                                                                                          \
		.unframed_supported = _unframed_supported,                                         \
		.phy = _phy,                                                                       \
		.rtn = _rtn,                                                                       \
		.latency = _latency,                                                               \
		.pd_min = _pd_min,                                                                 \
		.pd_max = _pd_max,                                                                 \
		.pref_pd_min = _pref_pd_min,                                                       \
		.pref_pd_max = _pref_pd_max,                                                       \
	}

/** @brief Audio Stream Quality of Service Preference structure. */
struct bt_audio_codec_qos_pref {
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
 * Functions to parse codec config data when formatted as LTV wrapped into @ref bt_audio_codec_cfg.
 *
 * @{
 */

/**
 * @brief Convert assigned numbers frequency to frequency value.
 *
 * @param freq The assigned numbers frequency to convert.
 *
 * @retval -EINVAL if arguments are invalid.
 * @retval The converted frequency value in Hz.
 */
int bt_audio_codec_cfg_freq_to_freq_hz(enum bt_audio_codec_config_freq freq);

/**
 * @brief Convert frequency value to assigned numbers frequency.
 *
 * @param freq_hz The frequency value to convert.
 *
 * @retval -EINVAL if arguments are invalid.
 * @retval The assigned numbers frequency (@ref bt_audio_codec_config_freq).
 */
int bt_audio_codec_cfg_freq_hz_to_freq(uint32_t freq_hz);

/**@brief Extract the frequency from a codec configuration.
 *
 * @param codec_cfg The codec configuration to extract data from.
 *
 *  @retval A @ref bt_audio_codec_config_freq value
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cfg_get_freq(const struct bt_audio_codec_cfg *codec_cfg);

/**
 * @brief Set the frequency of a codec configuration.
 *
 * @param codec_cfg The codec configuration to set data for.
 * @param freq      The assigned numbers frequency to set.
 *
 * @retval The data_len of @p codec_cfg on success
 * @retval -EINVAL if arguments are invalid
 * @retval -ENOMEM if the new value could not set or added due to memory
 */
int bt_audio_codec_cfg_set_freq(struct bt_audio_codec_cfg *codec_cfg,
				enum bt_audio_codec_config_freq freq);

/** @brief Extract frame duration from BT codec config
 *
 *  @param codec_cfg The codec configuration to extract data from.
 *
 *  @retval Frame duration in microseconds
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cfg_get_frame_duration_us(const struct bt_audio_codec_cfg *codec_cfg);

/** @brief Extract channel allocation from BT codec config
 *
 *  The value returned is a bit field representing one or more audio locations as
 *  specified by @ref bt_audio_location
 *  Shall match one or more of the bits set in BT_PAC_SNK_LOC/BT_PAC_SRC_LOC.
 *
 *  Up to the configured @ref BT_AUDIO_CODEC_LC3_CHAN_COUNT number of channels can be present.
 *
 *  @param codec_cfg The codec configuration to extract data from.
 *  @param chan_allocation Pointer to the variable to store the extracted value in.
 *
 *  @retval 0 if value is found and stored in the pointer provided
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cfg_get_chan_allocation(const struct bt_audio_codec_cfg *codec_cfg,
					   enum bt_audio_location *chan_allocation);

/**
 * @brief Set the channel allocation of a codec configuration.
 *
 * @param codec_cfg       The codec configuration to set data for.
 * @param chan_allocation The channel allocation to set.
 *
 * @retval The data_len of @p codec_cfg on success
 * @retval -EINVAL if arguments are invalid
 * @retval -ENOMEM if the new value could not set or added due to memory
 */
int bt_audio_codec_cfg_set_chan_allocation(struct bt_audio_codec_cfg *codec_cfg,
					   enum bt_audio_location chan_allocation);

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
 *  @param codec_cfg The codec configuration to extract data from.
 *
 *  @retval Frame length in octets
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cfg_get_octets_per_frame(const struct bt_audio_codec_cfg *codec_cfg);

/**
 * @brief Set the octets per codec frame of a codec configuration.
 *
 * @param codec_cfg        The codec configuration to set data for.
 * @param octets_per_frame The octets per codec frame to set.
 *
 * @retval The data_len of @p codec_cfg on success
 * @retval -EINVAL if arguments are invalid
 * @retval -ENOMEM if the new value could not set or added due to memory
 */
int bt_audio_codec_cfg_set_octets_per_frame(struct bt_audio_codec_cfg *codec_cfg,
					    uint16_t octets_per_frame);

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
 *  @param codec_cfg The codec configuration to extract data from.
 *  @param fallback_to_default If true this function will return the default value of 1
 *         if the type is not found. In this case the function will only fail if a NULL
 *         pointer is provided.
 *
 *  @retval The count of codec frames in each SDU if value is found else of @p fallback_to_default
 *          is true then the value 1 is returned if frames per sdu is not found.
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cfg_get_frame_blocks_per_sdu(const struct bt_audio_codec_cfg *codec_cfg,
						bool fallback_to_default);

/**
 * @brief Set the frame blocks per SDU of a codec configuration.
 *
 * @param codec_cfg    The codec configuration to set data for.
 * @param frame_blocks The frame blocks per SDU to set.
 *
 * @retval The data_len of @p codec_cfg on success
 * @retval -EINVAL if arguments are invalid
 * @retval -ENOMEM if the new value could not set or added due to memory
 */
int bt_audio_codec_cfg_set_frame_blocks_per_sdu(struct bt_audio_codec_cfg *codec_cfg,
						uint8_t frame_blocks);

/** @brief Lookup a specific codec configuration value
 *
 *  @param[in] codec_cfg The codec data to search in.
 *  @param[in] type The type id to look for
 *  @param[out] data Pointer to the data-pointer to update when item is found
 *  @return Length of found @p data or 0 if not found
 */
uint8_t bt_audio_codec_cfg_get_val(const struct bt_audio_codec_cfg *codec_cfg, uint8_t type,
				   const uint8_t **data);

/**
 * @brief Set or add a specific codec configuration value
 *
 * @param codec_cfg  The codec data to set the value in.
 * @param type       The type id to set
 * @param data       Pointer to the data-pointer to set
 * @param data_len   Length of @p data
 *
 * @retval The data_len of @p codec_cfg on success
 * @retval -EINVAL if arguments are invalid
 * @retval -ENOMEM if the new value could not set or added due to memory
 */
int bt_audio_codec_cfg_set_val(struct bt_audio_codec_cfg *codec_cfg, uint8_t type,
			       const uint8_t *data, size_t data_len);

/** @brief Lookup a specific metadata value based on type
 *
 *
 *  @param[in]  codec_cfg The codec data to search in.
 *  @param[in]  type      The type id to look for
 *  @param[out] data      Pointer to the data-pointer to update when item is found
 *
 *  @retval Length of found @p data (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cfg_meta_get_val(const struct bt_audio_codec_cfg *codec_cfg, uint8_t type,
				    const uint8_t **data);

/** @brief Extract preferred contexts
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PREF_CONTEXT for more information about this value.
 *
 *  @param codec_cfg The codec data to search in.
 *
 *  @retval The preferred context type if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cfg_meta_get_pref_context(const struct bt_audio_codec_cfg *codec_cfg);

/** @brief Extract stream contexts
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT for more information about this value.
 *
 *  @param codec_cfg The codec data to search in.
 *
 *  @retval The stream context type if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cfg_meta_get_stream_context(const struct bt_audio_codec_cfg *codec_cfg);

/** @brief Extract program info
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PROGRAM_INFO for more information about this value.
 *
 *  @param[in]  codec_cfg    The codec data to search in.
 *  @param[out] program_info Pointer to the UTF-8 formatted program info.
 *
 *  @retval The length of the @p program_info (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cfg_meta_get_program_info(const struct bt_audio_codec_cfg *codec_cfg,
					     const uint8_t **program_info);

/** @brief Extract stream language
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_STREAM_LANG for more information about this value.
 *
 *  @param codec_cfg The codec data to search in.
 *
 *  @retval The stream language if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cfg_meta_get_stream_lang(const struct bt_audio_codec_cfg *codec_cfg);

/** @brief Extract CCID list
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_CCID_LIST for more information about this value.
 *
 *  @param[in]  codec_cfg The codec data to search in.
 *  @param[out] ccid_list Pointer to the array containing 8-bit CCIDs.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cfg_meta_get_ccid_list(const struct bt_audio_codec_cfg *codec_cfg,
					  const uint8_t **ccid_list);

/** @brief Extract parental rating
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PARENTAL_RATING for more information about this value.
 *
 *  @param codec_cfg The codec data to search in.
 *
 *  @retval The parental rating if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cfg_meta_get_parental_rating(const struct bt_audio_codec_cfg *codec_cfg);

/** @brief Extract program info URI
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI for more information about this value.
 *
 *  @param[in]  codec_cfg The codec data to search in.
 *  @param[out] program_info_uri Pointer to the UTF-8 formatted program info URI.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cfg_meta_get_program_info_uri(const struct bt_audio_codec_cfg *codec_cfg,
						 const uint8_t **program_info_uri);

/** @brief Extract audio active state
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_AUDIO_STATE for more information about this value.
 *
 *  @param codec_cfg The codec data to search in.
 *
 *  @retval The preferred context type if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cfg_meta_get_audio_active_state(const struct bt_audio_codec_cfg *codec_cfg);

/** @brief Extract broadcast audio immediate rendering flag
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE for more information about this value.
 *
 *  @param codec_cfg The codec data to search in.
 *
 *  @retval 0 if the flag was found
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not the flag was not found
 */
int bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(
	const struct bt_audio_codec_cfg *codec_cfg);

/** @brief Extract extended metadata
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_EXTENDED for more information about this value.
 *
 *  @param[in]  codec_cfg     The codec data to search in.
 *  @param[out] extended_meta Pointer to the extended metadata.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cfg_meta_get_extended(const struct bt_audio_codec_cfg *codec_cfg,
					 const uint8_t **extended_meta);

/** @brief Extract vendor specific metadata
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_VENDOR for more information about this value.
 *
 *  @param[in]  codec_cfg   The codec data to search in.
 *  @param[out] vendor_meta Pointer to the vendor specific metadata.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cfg_meta_get_vendor(const struct bt_audio_codec_cfg *codec_cfg,
				       const uint8_t **vendor_meta);
/** @} */ /* End of bt_audio_codec_cfg */

/**
 * @brief Audio codec capabilities APIs
 * @defgroup bt_audio_codec_cap Codec capability parsing APIs
 *
 * Functions to parse codec capability data when formatted as LTV wrapped into @ref
 * bt_audio_codec_cap.
 *
 * @{
 */

/**
 * @brief Lookup a specific value based on type
 *
 * @param[in] codec_cap The codec data to search in.
 * @param[in] type The type id to look for
 * @param[out] data Pointer to the data-pointer to update when item is found
 *
 * @return Length of found @p data or 0 if not found
 */
uint8_t bt_audio_codec_cap_get_val(const struct bt_audio_codec_cap *codec_cap, uint8_t type,
				   const uint8_t **data);

/**
 * @brief Extract the frequency from a codec capability.
 *
 * @param codec_cap The codec configuration to extract data from.
 *
 * @retval Bitfield of supported frequencies if 0 or positive
 * @retval -EINVAL if arguments are invalid
 * @retval -ENODATA if not found
 * @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cap_get_freq(const struct bt_audio_codec_cap *codec_cap);

/**
 * @brief Extract the frequency from a codec capability.
 *
 * @param codec_cap The codec configuration to extract data from.
 *
 * @retval Bitfield of supported frame durations if 0 or positive
 * @retval -EINVAL if arguments are invalid
 * @retval -ENODATA if not found
 * @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cap_get_frame_duration(const struct bt_audio_codec_cap *codec_cap);

/**
 * @brief Extract the frequency from a codec capability.
 *
 * @param codec_cap The codec configuration to extract data from.
 *
 * @retval Bitfield of supported channel counts if 0 or positive
 * @retval -EINVAL if arguments are invalid
 * @retval -ENODATA if not found
 * @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cap_get_supported_audio_chan_counts(const struct bt_audio_codec_cap *codec_cap);

/**
 * @brief Extract the frequency from a codec capability.
 *
 * @param[in]  codec_cap   The codec configuration to extract data from.
 * @param[out] codec_frame Struct to place the resulting values in
 *
 * @retval 0 on success
 * @retval -EINVAL if arguments are invalid
 * @retval -ENODATA if not found
 * @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cap_get_octets_per_frame(
	const struct bt_audio_codec_cap *codec_cap,
	struct bt_audio_codec_octets_per_codec_frame *codec_frame);

/**
 * @brief Extract the frequency from a codec capability.
 *
 * @param codec_cap The codec configuration to extract data from.
 *
 * @retval Maximum number of codec frames per SDU supported
 * @retval -EINVAL if arguments are invalid
 * @retval -ENODATA if not found
 * @retval -EBADMSG if found value has invalid size or value
 */
int bt_audio_codec_cap_get_max_codec_frames_per_sdu(const struct bt_audio_codec_cap *codec_cap);

/** @brief Lookup a specific metadata value based on type
 *
 *  @param[in]  codec_cap The codec data to search in.
 *  @param[in]  type      The type id to look for
 *  @param[out] data      Pointer to the data-pointer to update when item is found
 *
 *  @retval Length of found @p data (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cap_meta_get_val(const struct bt_audio_codec_cap *codec_cap, uint8_t type,
				    const uint8_t **data);

/** @brief Extract preferred contexts
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PREF_CONTEXT for more information about this value.
 *
 *  @param codec_cap The codec data to search in.
 *
 *  @retval The preferred context type if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cap_meta_get_pref_context(const struct bt_audio_codec_cap *codec_cap);

/** @brief Extract stream contexts
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT for more information about this value.
 *
 *  @param codec_cap The codec data to search in.
 *
 *  @retval The stream context type if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cap_meta_get_stream_context(const struct bt_audio_codec_cap *codec_cap);

/** @brief Extract program info
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PROGRAM_INFO for more information about this value.
 *
 *  @param[in]  codec_cap    The codec data to search in.
 *  @param[out] program_info Pointer to the UTF-8 formatted program info.
 *
 *  @retval The length of the @p program_info (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cap_meta_get_program_info(const struct bt_audio_codec_cap *codec_cap,
					     const uint8_t **program_info);

/** @brief Extract stream language
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_STREAM_LANG for more information about this value.
 *
 *  @param codec_cap The codec data to search in.
 *
 *  @retval The stream language if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cap_meta_get_stream_lang(const struct bt_audio_codec_cap *codec_cap);

/** @brief Extract CCID list
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_CCID_LIST for more information about this value.
 *
 *  @param[in]  codec_cap The codec data to search in.
 *  @param[out] ccid_list Pointer to the array containing 8-bit CCIDs.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cap_meta_get_ccid_list(const struct bt_audio_codec_cap *codec_cap,
					  const uint8_t **ccid_list);

/** @brief Extract parental rating
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PARENTAL_RATING for more information about this value.
 *
 *  @param codec_cap The codec data to search in.
 *
 *  @retval The parental rating if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cap_meta_get_parental_rating(const struct bt_audio_codec_cap *codec_cap);

/** @brief Extract program info URI
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI for more information about this value.
 *
 *  @param[in]  codec_cap        The codec data to search in.
 *  @param[out] program_info_uri Pointer to the UTF-8 formatted program info URI.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cap_meta_get_program_info_uri(const struct bt_audio_codec_cap *codec_cap,
						 const uint8_t **program_info_uri);

/** @brief Extract audio active state
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_AUDIO_STATE for more information about this value.
 *
 *  @param codec_cap The codec data to search in.
 *
 *  @retval The preferred context type if positive or 0
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 *  @retval -EBADMSG if found value has invalid size
 */
int bt_audio_codec_cap_meta_get_audio_active_state(const struct bt_audio_codec_cap *codec_cap);

/** @brief Extract broadcast audio immediate rendering flag
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE for more information about this value.
 *
 *  @param codec_cap The codec data to search in.
 *
 *  @retval 0 if the flag was found
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not the flag was not found
 */
int bt_audio_codec_cap_meta_get_bcast_audio_immediate_rend_flag(
	const struct bt_audio_codec_cap *codec_cap);

/** @brief Extract extended metadata
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_EXTENDED for more information about this value.
 *
 *  @param[in]  codec_cap     The codec data to search in.
 *  @param[out] extended_meta Pointer to the extended metadata.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cap_meta_get_extended(const struct bt_audio_codec_cap *codec_cap,
					 const uint8_t **extended_meta);

/** @brief Extract vendor specific metadata
 *
 *  See @ref BT_AUDIO_METADATA_TYPE_VENDOR for more information about this value.
 *
 *  @param[in]  codec_cap   The codec data to search in.
 *  @param[out] vendor_meta Pointer to the vendor specific metadata.
 *
 *  @retval The length of the @p ccid_list (may be 0)
 *  @retval -EINVAL if arguments are invalid
 *  @retval -ENODATA if not found
 */
int bt_audio_codec_cap_meta_get_vendor(const struct bt_audio_codec_cap *codec_cap,
				       const uint8_t **vendor_meta);
/** @} */ /* End of bt_audio_codec_cap */

#ifdef __cplusplus
}
#endif

/** @} */ /* end of bt_audio */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_H_ */
