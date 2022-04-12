/** @file
 *  @brief Bluetooth Audio handling
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AUDIO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_AUDIO_H_

#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/lc3.h>

/**
 * @brief Bluetooth Audio
 * @defgroup bt_audio Bluetooth Audio
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define BT_AUDIO_BROADCAST_ID_SIZE               3 /* octets */

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

/** @def BT_AUDIO_CONTEXT_TYPE_ANY
 *
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


/* Unicast Announcement Type, Generic Audio */
#define BT_AUDIO_UNICAST_ANNOUNCEMENT_GENERAL    0x00
#define BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED   0x01

#if defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
#define BROADCAST_SNK_STREAM_CNT CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT
#define BROADCAST_SNK_SUBGROUP_CNT CONFIG_BT_AUDIO_BROADCAST_SNK_SUBGROUP_COUNT
#else /* !CONFIG_BT_AUDIO_BROADCAST_SINK */
#define BROADCAST_SNK_STREAM_CNT 0
#define BROADCAST_SNK_SUBGROUP_CNT 0
#endif /* CONFIG_BT_AUDIO_BROADCAST_SINK*/

/** @brief Abstract Audio Unicast Group structure. */
struct bt_audio_unicast_group;

/** @brief Abstract Audio Broadcast Sink structure. */
struct bt_audio_broadcast_sink;

/** @brief Abstract Audio Broadcast Source structure. */
struct bt_audio_broadcast_source;

/** @brief Codec configuration structure */
struct bt_codec_data {
	struct bt_data data;
	uint8_t  value[CONFIG_BT_CODEC_MAX_DATA_LEN];
};

/** @def BT_CODEC_DATA
 *  @brief Helper to declare elements of bt_codec_data arrays
 *
 *  This macro is mainly for creating an array of struct bt_codec_data
 *  elements inside bt_codec which is then passed to the likes of
 *  bt_audio_stream_config or bt_audio_stream_reconfig.
 *
 *  @param _type Type of advertising data field
 *  @param _bytes Variable number of single-byte parameters
 */
#define BT_CODEC_DATA(_type, _bytes...) \
	{ \
		.data = BT_DATA(_type, ((uint8_t []) { _bytes }), \
				sizeof((uint8_t []) { _bytes })) \
	}

/** @def BT_CODEC
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
		.id = _id, \
		.cid = _cid, \
		.vid = _vid, \
		.data_count = ARRAY_SIZE(((struct bt_codec_data[]) _data)), \
		.data = _data, \
		.meta_count = ARRAY_SIZE(((struct bt_codec_data[]) _meta)), \
		.meta = _meta, \
	}


/** @brief Meta data type ids used for LTV encoded metadata.
 *
 * These values are defined by the Generic Audio Assigned Numbers, bluetooth.com
 */
enum bt_audio_meta_type {
	BT_CODEC_META_PREFER_CONTEXT     = 0x01,
	BT_CODEC_META_CONTEXT            = 0x02,
	BT_CODEC_META_PROGRAM_INFO       = 0x03,
	BT_CODEC_META_LANGUAGE           = 0x04,
	BT_CODEC_META_CCID_LIST          = 0x05,
	BT_CODEC_META_PARENTAL_RATING    = 0x06,
	BT_CODEC_META_PROGRAM_INFO_URI   = 0x07,
	BT_CODEC_META_EXTENDED_METADATA  = 0xFE,
	BT_CODEC_META_VENDOR_SPECIFIC    = 0xFF,
};

/** @brief Location values for BT Audio.
 *
 * These values are defined by the Generic Audio Assigned Numbers, bluetooth.com
 */
enum bt_audio_location {
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
	BT_AUDIO_LOCATION_TOP_BECK_RIGHT = BIT(17),
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

/** @brief Codec structure. */
struct bt_codec {
	/** Codec ID */
	uint8_t  id;
	/** Codec Company ID */
	uint16_t cid;
	/** Codec Company Vendor ID */
	uint16_t vid;
	/** Codec Specific Data count */
	size_t   data_count;
	/** Codec Specific Data */
	struct bt_codec_data data[CONFIG_BT_CODEC_MAX_DATA_COUNT];
	/** Codec Specific Metadata count */
	size_t   meta_count;
	/** Codec Specific Metadata */
	struct bt_codec_data meta[CONFIG_BT_CODEC_MAX_METADATA_COUNT];
};

struct bt_audio_base_bis_data {
	/* Unique index of the BIS */
	uint8_t index;
	/** Codec Specific Data count.
	 *
	 *  Only valid if the data_count of struct bt_codec in the subgroup is 0
	 */
	size_t   data_count;
	/** Codec Specific Data
	 *
	 *  Only valid if the data_count of struct bt_codec in the subgroup is 0
	 */
	struct bt_codec_data data[CONFIG_BT_CODEC_MAX_DATA_COUNT];
};

struct bt_audio_base_subgroup {
	/* Number of BIS in the subgroup */
	size_t bis_count;
	/** Codec information for the subgroup
	 *
	 *  If the data_count of the codec is 0, then codec specific data may be
	 *  found for each BIS in the bis_data.
	 */
	struct bt_codec	codec;
	/* Array of BIS specific data for each BIS in the subgroup */
	struct bt_audio_base_bis_data bis_data[BROADCAST_SNK_STREAM_CNT];
};

struct bt_audio_base {
	/* Number of subgroups in the BASE */
	size_t subgroup_count;
	/* Array of subgroups in the BASE */
	struct bt_audio_base_subgroup subgroups[BROADCAST_SNK_SUBGROUP_CNT];
};

/** @brief Audio Capability type */
enum bt_audio_dir {
	BT_AUDIO_DIR_SINK = 0x01,
	BT_AUDIO_DIR_SOURCE = 0x02,
};

/** @def BT_CODEC_QOS
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

/** @def BT_CODEC_QOS_UNFRAMED
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

/** @def BT_CODEC_QOS_FRAMED
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

	/** QoS Transport Latency */
	uint16_t latency;

	/** QoS Frame Interval */
	uint32_t interval;
	/** QoS Presentation Delay */
	uint32_t pd;
};

/** @def BT_CODEC_QOS_PREF
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

	/** @brief Minimum Presentation Delay
	 *
	 *  Unlike the other fields, this is not a preference but a minimum
	 *  requirement.
	 */
	uint32_t pd_min;

	/** @brief Maximum Presentation Delay
	 *
	 *  Unlike the other fields, this is not a preference but a maximum
	 *  requirement.
	 */
	uint32_t pd_max;

	/** @brief Preferred minimum Presentation Delay */
	uint32_t pref_pd_min;

	/** @brief Preferred maximum Presentation Delay	*/
	uint32_t pref_pd_max;
};

/** Struct to hold a BAP defined LC3 preset */
struct bt_audio_lc3_preset {
	/** The LC3 Codec */
	struct bt_codec codec;
	/** The BAP spec defined QoS values */
	struct bt_codec_qos qos;
};

/** Helper to declare an LC3 preset structure */
#define BT_AUDIO_LC3_PRESET(_codec, _qos) \
	{ \
		.codec = _codec, \
		.qos = _qos, \
	}

/* LC3 Unicast presets defined by table 5.2 in the BAP v1.0 specification */
#define BT_AUDIO_LC3_UNICAST_PRESET_8_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(26u, 2u, 8u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_8_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(30u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_16_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(30u, 2u, 8u, 40000u) \
	)

/** Mandatory to support as both unicast client and server */
#define BT_AUDIO_LC3_UNICAST_PRESET_16_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(40u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_24_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(45u, 2u, 8u, 40000u) \
	)

/** Mandatory to support as unicast server */
#define BT_AUDIO_LC3_UNICAST_PRESET_24_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(60u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_32_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(60u, 2u, 8u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_32_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(80u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_441_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_1, \
		BT_CODEC_QOS(8163u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 97u, 5u, 24u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_441_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_2, \
		BT_CODEC_QOS(10884u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 130u, 5u, 31u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(75u, 5u, 15u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(100u, 5u, 20u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_3_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_3, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(90u, 5u, 15u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_4_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_4, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(120u, 5u, 20u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_5_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_5, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(117u, 5u, 15u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_6_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_6, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(155u, 5u, 20u, 40000u) \
	)

/* Following presets are for unicast high reliability audio data */
#define BT_AUDIO_LC3_UNICAST_PRESET_8_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(26u, 13u, 75u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_8_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(30u, 13u, 95u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_16_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(30u, 13u, 75u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_16_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(40u, 13u, 95u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_24_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(45u, 13u, 75u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_24_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(60u, 13u, 95u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_32_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(60u, 13u, 75u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_32_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(80u, 13u, 95u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_441_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_1, \
		BT_CODEC_QOS(8163u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 97u, 13u, 80u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_441_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_2, \
		BT_CODEC_QOS(10884u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 130u, 13u, 85u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(75u, 13u, 75u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(100u, 13u, 95u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_3_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_3, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(90u, 13u, 75u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_4_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_4, \
	BT_CODEC_LC3_QOS_10_UNFRAMED(120u, 13u, 100u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_5_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_5, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(117u, 13u, 75u, 40000u) \
	)

#define BT_AUDIO_LC3_UNICAST_PRESET_48_6_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_6, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(155u, 13u, 100u, 40000u) \
	)

/* LC3 Broadcast presets defined by table 6.4 in the BAP v1.0 specification */
#define BT_AUDIO_LC3_BROADCAST_PRESET_8_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(26u, 2u, 8u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_8_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(30u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_16_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(30u, 2u, 8u, 40000u) \
	)

/** Mandatory to support as both broadcast source and sink */
#define BT_AUDIO_LC3_BROADCAST_PRESET_16_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(40u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_24_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(45u, 2u, 8u, 40000u) \
	)

/** Mandatory to support as broadcast sink */
#define BT_AUDIO_LC3_BROADCAST_PRESET_24_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(60u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_32_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(60u, 2u, 8u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_32_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(80u, 2u, 10u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_441_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_1, \
		BT_CODEC_QOS(8163u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 97u, 4u, 24u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_441_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_2, \
		BT_CODEC_QOS(10884u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 130u, 4u, 31u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_1_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(75u, 4u, 15u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_2_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(100u, 4u, 20u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_3_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_3, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(90u, 4u, 15u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_4_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_4, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(120u, 4u, 20u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_5_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_5, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(117u, 4u, 15u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_6_1 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_6, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(155u, 4u, 20u, 40000u) \
	)

/* Following presets are for broadcast high reliability audio data */
#define BT_AUDIO_LC3_BROADCAST_PRESET_8_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(26u, 4u, 45u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_8_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_8_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(30u, 4u, 60u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_16_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(30u, 4u, 45u, 40000u) \
	)

/** Mandatory to support as both broadcast source and sink */
#define BT_AUDIO_LC3_BROADCAST_PRESET_16_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_16_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(40u, 4u, 60u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_24_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(45u, 4u, 45u, 40000u) \
	)

/** Mandatory to support as broadcast sink */
#define BT_AUDIO_LC3_BROADCAST_PRESET_24_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_24_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(60u, 4u, 60u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_32_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(60u, 4u, 45u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_32_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_32_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(80u, 4u, 60u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_441_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_1, \
		BT_CODEC_QOS(8163u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 97u, 4u, 54u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_441_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_441_2, \
		BT_CODEC_QOS(10884u, BT_CODEC_QOS_FRAMED, \
			     BT_CODEC_QOS_2M, 130u, 4u, 60u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_1_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_1, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(75u, 4u, 50u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_2_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_2, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(100u, 4u, 65u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_3_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_3, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(90u, 4u, 50u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_4_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_4, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(120u, 4u, 65u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_5_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_5, \
		BT_CODEC_LC3_QOS_7_5_UNFRAMED(117u, 4u, 50u, 40000u) \
	)

#define BT_AUDIO_LC3_BROADCAST_PRESET_48_6_2 \
	BT_AUDIO_LC3_PRESET( \
		BT_CODEC_LC3_CONFIG_48_6, \
		BT_CODEC_LC3_QOS_10_UNFRAMED(155u, 4u, 65u, 40000u) \
	)

/** @brief Audio stream structure.
 *
 *  Audio Streams represents a stream configuration of a Remote Endpoint and
 *  a Local Capability.
 *
 *  @note Audio streams are unidirectional although its QoS can be configured
 *  to be bidirectional if stream are linked, in which case the QoS must be
 *  symmetric in both directions.
 */
struct bt_audio_stream {
	/** Connection reference */
	struct bt_conn *conn;
	/** Endpoint reference */
	struct bt_audio_ep *ep;
	/** Codec Configuration */
	struct bt_codec *codec;
	/** QoS Configuration */
	struct bt_codec_qos *qos;
	/** ISO channel reference */
	struct bt_iso_chan *iso;
	/** Audio stream operations */
	struct bt_audio_stream_ops *ops;
	sys_snode_t node;

	union {
		void *group;
		struct bt_audio_unicast_group *unicast_group;
		struct bt_audio_broadcast_source *broadcast_source;
		struct bt_audio_broadcast_sink *broadcast_sink;
	};

	/** Stream user data */
	void *user_data;
};

/** Unicast Server callback structure */
struct bt_audio_unicast_server_cb {
	/** @brief Endpoint config request callback
	 *
	 *  Config callback is called whenever an endpoint is requested to be
	 *  configured
	 *
	 *  @param[in]  conn    Connection object.
	 *  @param[in]  ep      Local Audio Endpoint being configured.
	 *  @param[in]  dir     Direction of the endpoint.
	 *  @param[in]  codec   Codec configuration.
	 *  @param[out] stream  Pointer to stream that will be configured for
	 *                      the endpoint.
	 *  @param[out] pref    Pointer to a QoS preference object that shall
	 *                      be populated with values. Invalid values will
	 *                      reject the codec configuration request.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*config)(struct bt_conn *conn,
		      const struct bt_audio_ep *ep,
		      enum bt_audio_dir dir,
		      const struct bt_codec *codec,
		      struct bt_audio_stream **stream,
		      struct bt_codec_qos_pref *const pref);

	/** @brief Stream reconfig request callback
	 *
	 *  Reconfig callback is called whenever an Audio Stream needs to be
	 *  reconfigured with different codec configuration.
	 *
	 *  @param[in]  stream  Stream object being reconfigured.
	 *  @param[in]  dir     Direction of the endpoint.
	 *  @param[in]  codec   Codec configuration.
	 *  @param[out] pref    Pointer to a QoS preference object that shall
	 *                      be populated with values. Invalid values will
	 *                      reject the codec configuration request.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*reconfig)(struct bt_audio_stream *stream,
			enum bt_audio_dir dir,
			const struct bt_codec *codec,
			struct bt_codec_qos_pref *const pref);

	/** @brief Stream QoS request callback
	 *
	 *  QoS callback is called whenever an Audio Stream Quality of
	 *  Service needs to be configured.
	 *
	 *  @param stream  Stream object being reconfigured.
	 *  @param qos     Quality of Service configuration.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*qos)(struct bt_audio_stream *stream,
		   const struct bt_codec_qos *qos);

	/** @brief Stream Enable request callback
	 *
	 *  Enable callback is called whenever an Audio Stream is requested to
	 *  be enabled to stream.
	 *
	 *  @param stream      Stream object being enabled.
	 *  @param meta        Metadata entries
	 *  @param meta_count  Number of metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*enable)(struct bt_audio_stream *stream,
		      const struct bt_codec_data *meta,
		      size_t meta_count);

	/** @brief Stream Start request callback
	 *
	 *  Start callback is called whenever an Audio Stream is requested to
	 *  start streaming.
	 *
	 *  @param stream Stream object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*start)(struct bt_audio_stream *stream);

	/** @brief Stream Metadata update request callback
	 *
	 *  Metadata callback is called whenever an Audio Stream is requested to
	 *  update its metadata.
	 *
	 *  @param stream       Stream object.
	 *  @param meta         Metadata entries
	 *  @param meta_count   Number of metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*metadata)(struct bt_audio_stream *stream,
			const struct bt_codec_data *meta,
			size_t meta_count);

	/** @brief Stream Disable request callback
	 *
	 *  Disable callback is called whenever an Audio Stream is requested to
	 *  disable the stream.
	 *
	 *  @param stream Stream object being disabled.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*disable)(struct bt_audio_stream *stream);

	/** @brief Stream Stop callback
	 *
	 *  Stop callback is called whenever an Audio Stream is requested to
	 *  stop streaming.
	 *
	 *  @param stream Stream object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*stop)(struct bt_audio_stream *stream);

	/** @brief Stream release callback
	 *
	 *  Release callback is called whenever a new Audio Stream needs to be
	 *  released and thus deallocated.
	 *
	 *  @param stream Stream object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*release)(struct bt_audio_stream *stream);
};

/**  @brief Callback structure for the Public Audio Capabilities Service (PACS)
 *
 * This is used for the Unicast Server
 * (@kconfig{CONFIG_BT_AUDIO_UNICAST_SERVER}) and Broadcast Sink
 * (@kconfig{CONFIG_BT_AUDIO_BROADCAST_SINK}) roles.
 */
struct bt_audio_pacs_cb {
	/** @brief Get available audio contexts callback
	 *
	 *  Get available audio contexts callback is called whenever a remote client
	 *  requests to read the value of Published Audio Capabilities (PAC) Available
	 *  Audio Contexts, or if the value needs to be notified.
	 *
	 *  @param[in]  conn     The connection that requests the available audio
	 *                       contexts. Will be NULL if requested for sending
	 *                       a notification, as a result of calling
	 *                       bt_pacs_available_contexts_changed().
	 *  @param[in]  dir      Direction of the endpoint.
	 *  @param[out] context  Pointer to the contexts that needs to be set.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*get_available_contexts)(struct bt_conn *conn, enum bt_audio_dir dir,
				      enum bt_audio_context *context);

	/** @brief Publish Capability callback
	 *
	 *  Publish Capability callback is called whenever a remote client
	 *  requests to read the Published Audio Capabilities (PAC) records.
	 *  The callback will be called iteratively until it returns an error,
	 *  increasing the @p index each time. Once an error value (non-zero)
	 *  is returned, the previously returned @p codec values (if any) will
	 *  be sent to the client that requested the value.
	 *
	 *  @param conn   The connection that requests the capabilities.
	 *                Will be NULL if the capabilities is requested for
	 *                sending a notification, as a result of calling
	 *                bt_audio_capability_register() or
	 *                bt_audio_capability_unregister().
	 *  @param type   Type of the endpoint.
	 *  @param index  Index of the codec object requested. Multiple objects
	 *                may be returned, and this value keep tracks of how
	 *                many have previously been returned.
	 *  @param codec  Codec object that shall be populated if returning
	 *                success (0). Ignored if returning non-zero.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*publish_capability)(struct bt_conn *conn, uint8_t type,
				  uint8_t index, struct bt_codec *const codec);

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
	/** @brief Publish location callback
	 *
	 *  Publish location callback is called whenever a remote client
	 *  requests to read the Published Audio Capabilities (PAC) location,
	 *  or if the location needs to be notified.
	 *
	 *  @param[in]  conn      The connection that requests the location.
	 *                        Will be NULL if the location is requested
	 *                        for sending a notification, as a result of
	 *                        calling bt_audio_pacs_location_changed().
	 *  @param[in]  dir       Direction of the endpoint.
	 *  @param[out] location  Pointer to the location that needs to be set.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*publish_location)(struct bt_conn *conn,
				enum bt_audio_dir dir,
				enum bt_audio_location *location);

#if defined(CONFIG_BT_PAC_SNK_LOC_WRITEABLE) || defined(CONFIG_BT_PAC_SRC_LOC_WRITEABLE)
	/** @brief Write location callback
	 *
	 *  Write location callback is called whenever a remote client
	 *  requests to write the Published Audio Capabilities (PAC) location.
	 *
	 *  @param conn      The connection that requests the write.
	 *  @param dir       Direction of the endpoint.
	 *  @param location  The location being written.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*write_location)(struct bt_conn *conn, enum bt_audio_dir dir,
			      enum bt_audio_location location);
#endif /* CONFIG_BT_PAC_SNK_LOC_WRITEABLE || CONFIG_BT_PAC_SRC_LOC_WRITEABLE */
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */
};

/** Broadcast Audio Sink callback structure */
struct bt_audio_broadcast_sink_cb {
	/** @brief Scan receive callback
	 *
	 *  Scan receive callback is called whenever a broadcast source has been
	 *  found.
	 *
	 *  @param info          Advertiser packet information.
	 *  @param broadcast_id  24-bit broadcast ID
	 *
	 *  @return true to sync to the broadcaster, else false.
	 *          Syncing to the broadcaster will stop the current scan.
	 */
	bool (*scan_recv)(const struct bt_le_scan_recv_info *info,
			  uint32_t broadcast_id);

	/** @brief Periodic advertising sync callback
	 *
	 *  Called when synchronized to a periodic advertising. When
	 *  synchronized a bt_audio_broadcast_sink structure is allocated for
	 *  future use.
	 *
	 *  @param sink          Pointer to the allocated sink structure.
	 *  @param sync          Pointer to the periodic advertising sync.
	 *  @param broadcast_id  24-bit broadcast ID previously reported by
	 *                       scan_recv.
	 */
	void (*pa_synced)(struct bt_audio_broadcast_sink *sink,
			  struct bt_le_per_adv_sync *sync,
			  uint32_t broadcast_id);

	/** @brief Broadcast Audio Source Endpoint (BASE) received
	 *
	 *  Callback for when we receive a BASE from a broadcaster after
	 *  syncing to the broadcaster's periodic advertising.
	 *
	 *  @param sink          Pointer to the sink structure.
	 *  @param base          Broadcast Audio Source Endpoint (BASE).
	 */
	void (*base_recv)(struct bt_audio_broadcast_sink *sink,
			  const struct bt_audio_base *base);

	/** @brief Broadcast sink is syncable
	 *
	 *  Called whenever a broadcast sink is not synchronized to audio, but
	 *  the audio is synchronizable. This is inferred when a BIGInfo report
	 *  is received.
	 *
	 *  Once this callback has been called, it is possible to call
	 *  bt_audio_broadcast_sink_sync() to synchronize to the audio stream(s).
	 *
	 *  @param sink          Pointer to the sink structure.
	 *  @param encrypted     Whether or not the broadcast is encrypted
	 */
	void (*syncable)(struct bt_audio_broadcast_sink *sink, bool encrypted);

	/** @brief Scan terminated callback
	 *
	 *  Scan terminated callback is called whenever a scan started by
	 *  bt_audio_broadcast_sink_scan_start() is terminated before
	 *  bt_audio_broadcast_sink_scan_stop().
	 *
	 *  Typical reasons for this are that the periodic advertising has
	 *  synchronized (success criteria) or the scan timed out.
	 *  It may also be called if the periodic advertising failed to
	 *  synchronize.
	 *
	 *  @param err 0 in case of success or negative value in case of error.
	 */
	void (*scan_term)(int err);

	/** @brief Periodic advertising synchronization lost callback
	 *
	 *  The periodic advertising synchronization lost callback is called if
	 *  the periodic advertising sync is lost. If this happens, the sink
	 *  object is deleted. To synchronize to the broadcaster again,
	 *  bt_audio_broadcast_sink_scan_start() must be called.
	 *
	 *  @param sink          Pointer to the sink structure.
	 */
	void (*pa_sync_lost)(struct bt_audio_broadcast_sink *sink);

	/* Internally used list node */
	sys_snode_t node;
};

/** @brief Stream operation. */
struct bt_audio_stream_ops {
#if defined(CONFIG_BT_AUDIO_UNICAST)
	/** @brief Stream configured callback
	 *
	 *  Configured callback is called whenever an Audio Stream has been
	 *  configured.
	 *
	 *  @param stream Stream object that has been configured.
	 *  @param pref   Remote QoS preferences.
	 */
	void (*configured)(struct bt_audio_stream *stream,
			   const struct bt_codec_qos_pref *pref);

	/** @brief Stream QoS set callback
	 *
	 *  QoS set callback is called whenever an Audio Stream Quality of
	 *  Service has been set or updated.
	 *
	 *  @param stream Stream object that had its QoS updated.
	 */
	void (*qos_set)(struct bt_audio_stream *stream);

	/** @brief Stream enabled callback
	 *
	 *  Enabled callback is called whenever an Audio Stream has been
	 *  enabled.
	 *
	 *  @param stream Stream object that has been enabled.
	 */
	void (*enabled)(struct bt_audio_stream *stream);

	/** @brief Stream metadata updated callback
	 *
	 *  Metadata Updated callback is called whenever an Audio Stream's
	 *  metadata has been updated.
	 *
	 *  @param stream Stream object that had its metadata updated.
	 */
	void (*metadata_updated)(struct bt_audio_stream *stream);

	/** @brief Stream disabled callback
	 *
	 *  Disabled callback is called whenever an Audio Stream has been
	 *  disabled.
	 *
	 *  @param stream Stream object that has been disabled.
	 */
	void (*disabled)(struct bt_audio_stream *stream);

	/** @brief Stream released callback
	 *
	 *  Released callback is called whenever a Audio Stream has been
	 *  released and can be deallocated.
	 *
	 *  @param stream Stream object that has been released.
	 */
	void (*released)(struct bt_audio_stream *stream);
#endif /* CONFIG_BT_AUDIO_UNICAST */

	/** @brief Stream started callback
	 *
	 *  Started callback is called whenever an Audio Stream has been started
	 *  and will be usable for streaming.
	 *
	 *  @param stream Stream object that has been started.
	 */
	void (*started)(struct bt_audio_stream *stream);

	/** @brief Stream stopped callback
	 *
	 *  Stopped callback is called whenever an Audio Stream has been
	 *  stopped.
	 *
	 *  @param stream Stream object that has been stopped.
	 */
	void (*stopped)(struct bt_audio_stream *stream);

#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SINK)
	/** @brief Stream audio HCI receive callback.
	 *
	 *  This callback is only used if the ISO data path is HCI.
	 *
	 *  @param stream Stream object.
	 *  @param info   Pointer to the metadata for the buffer. The lifetime
	 *                of the pointer is linked to the lifetime of the
	 *                net_buf. Metadata such as sequence number and
	 *                timestamp can be provided by the bluetooth controller.
	 *  @param buf    Buffer containing incoming audio data.
	 */
	void (*recv)(struct bt_audio_stream *stream,
		     const struct bt_iso_recv_info *info,
		     struct net_buf *buf);
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SINK */

#if defined(CONFIG_BT_AUDIO_UNICAST) || defined(CONFIG_BT_AUDIO_BROADCAST_SOURCE)
	/** @brief Stream audio HCI sent callback
	 *
	 *  If this callback is provided it will be called whenever a SDU has
	 *  been completely sent, or otherwise flushed due to transmission
	 *  issues.
	 *  This callback is only used if the ISO data path is HCI.
	 *
	 *  @param chan The channel which has sent data.
	 */
	void (*sent)(struct bt_audio_stream *stream);
#endif /* CONFIG_BT_AUDIO_UNICAST || CONFIG_BT_AUDIO_BROADCAST_SOURCE */

};

/** @brief Register Audio callbacks for a stream.
 *
 *  Register Audio callbacks for a stream.
 *
 *  @param stream Stream object.
 *  @param ops    Stream operations structure.
 */
void bt_audio_stream_cb_register(struct bt_audio_stream *stream,
				 struct bt_audio_stream_ops *ops);
/**
 * @defgroup bt_audio_server Audio Server APIs
 * @ingroup bt_audio
 * @{
 */

/** @brief Register Published Audio Capabilities Service callbacks.
 *
 *  Only one callback structure can be registered, and attempting to
 *  registering more than one will result in an error.
 *
 *  This can only be done for the Unicast Server
 *  (@kconfig{CONFIG_BT_AUDIO_UNICAST_SERVER}) and Broadcast Sink
 *  (@kconfig{CONFIG_BT_AUDIO_BROADCAST_SINK}) roles.
 *
 *  Calling bt_audio_capability_register() will implicitly register the
 *  callbacks.
 *
 *  @param cb  Unicast server callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_pacs_register_cb(const struct bt_audio_pacs_cb *cb);

/** @brief Notify that the location has changed
 *
 * @param dir Direction of the location changed.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_audio_pacs_location_changed(enum bt_audio_dir dir);

/** @brief Notify available audio contexts changed
 *
 * Notify connected clients that the available audio contexts has changed
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_pacs_available_contexts_changed(void);

/** @brief Register unicast server callbacks.
 *
 *  Only one callback structure can be registered, and attempting to
 *  registering more than one will result in an error.
 *
 *  @param cb  Unicast server callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_unicast_server_register_cb(const struct bt_audio_unicast_server_cb *cb);

/** @brief Unregister unicast server callbacks.
 *
 *  May only unregister a callback structure that has previously been
 *  registered by bt_audio_unicast_server_register_cb().
 *
 *  @param cb  Unicast server callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_unicast_server_unregister_cb(const struct bt_audio_unicast_server_cb *cb);

/** @} */ /* End of group bt_audio_server */

/**
 * @defgroup bt_audio_client Audio Client APIs
 * @ingroup bt_audio
 * @{
 */

struct bt_audio_discover_params;

/** @typedef bt_audio_discover_func_t
 *  @brief Discover Audio capabilities and endpoints callback function.
 *
 *  If discovery procedure has complete both cap and ep are set to NULL.
 */
typedef void (*bt_audio_discover_func_t)(struct bt_conn *conn,
					 struct bt_codec *codec,
					 struct bt_audio_ep *ep,
					 struct bt_audio_discover_params *params);

struct bt_audio_discover_params {
	/** Capabilities type */
	enum bt_audio_dir dir;
	/** Callback function */
	bt_audio_discover_func_t func;
	/** Number of capabilities found */
	uint8_t  num_caps;
	/** Number of endpoints found */
	uint8_t  num_eps;
	/** Error code. */
	uint8_t  err;
	struct bt_gatt_read_params read;
	struct bt_gatt_discover_params discover;
};

/** @brief Discover remote capabilities and endpoints
 *
 *  This procedure is used by a client to discover remote capabilities and
 *  endpoints and notifies via params callback.
 *
 *  @note This procedure is asynchronous therefore the parameters need to
 *        remains valid while it is active.
 *
 *  @param conn Connection object
 *  @param params Discover parameters
 */
int bt_audio_discover(struct bt_conn *conn,
		      struct bt_audio_discover_params *params);

/** @brief Configure Audio Stream
 *
 *  This procedure is used by a client to configure a new stream using the
 *  remote endpoint, local capability and codec configuration.
 *
 *  @param conn Connection object
 *  @param stream Stream object being configured
 *  @param ep Remote Audio Endpoint being configured
 *  @param codec Codec configuration
 *
 *  @return Allocated Audio Stream object or NULL in case of error.
 */
int bt_audio_stream_config(struct bt_conn *conn,
			   struct bt_audio_stream *stream,
			   struct bt_audio_ep *ep,
			   struct bt_codec *codec);

/** @brief Reconfigure Audio Stream
 *
 *  This procedure is used by a client to reconfigure a stream using the
 *  a different local capability and/or codec configuration.
 *
 *  This can only be done for unicast streams.
 *
 *  @param stream Stream object being reconfigured
 *  @param codec Codec configuration
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_reconfig(struct bt_audio_stream *stream,
			     struct bt_codec *codec);

/** @brief Configure Audio Stream QoS
 *
 *  This procedure is used by a client to configure the Quality of Service of
 *  streams in a unicast group. All streams in the group for the specified
 *  @p conn will have the Quality of Service configured.
 *  This shall only be used to configure unicast streams.
 *
 *  @param conn  Connection object
 *  @param group Unicast group object
 *  @param qos   Quality of Service configuration
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_qos(struct bt_conn *conn,
			struct bt_audio_unicast_group *group,
			struct bt_codec_qos *qos);

/** @brief Enable Audio Stream
 *
 *  This procedure is used by a client to enable a stream.
 *
 *  This shall only be called for unicast streams, as broadcast streams will
 *  always be enabled once created.
 *
 *  @param stream Stream object
 *  @param meta_count Number of metadata entries
 *  @param meta Metadata entries
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_enable(struct bt_audio_stream *stream,
			   struct bt_codec_data *meta,
			   size_t meta_count);

/** @brief Change Audio Stream Metadata
 *
 *  This procedure is used by a client to change the metadata of a stream.
 *
 *  @param stream Stream object
 *  @param meta_count Number of metadata entries
 *  @param meta Metadata entries
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_metadata(struct bt_audio_stream *stream,
			     struct bt_codec_data *meta,
			     size_t meta_count);

/** @brief Disable Audio Stream
 *
 *  This procedure is used by a client to disable a stream.
 *
 *  This shall only be called for unicast streams, as broadcast streams will
 *  always be enabled once created.
 *
 *  @param stream Stream object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_disable(struct bt_audio_stream *stream);

/** @brief Start Audio Stream
 *
 *  This procedure is used by a client to make a stream start streaming.
 *
 *  This shall only be called for unicast streams.
 *  Broadcast sinks will always be started once synchronized, and broadcast
 *  source streams shall be started with bt_audio_broadcast_source_start().
 *
 *  @param stream Stream object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_start(struct bt_audio_stream *stream);

/** @brief Stop Audio Stream
 *
 *  This procedure is used by a client to make a stream stop streaming.
 *
 *  This shall only be called for unicast streams.
 *  Broadcast sinks cannot be stopped.
 *  Broadcast sources shall be stopped with bt_audio_broadcast_source_stop().
 *
 *  @param stream Stream object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_stop(struct bt_audio_stream *stream);

/** @brief Release Audio Stream
 *
 *  This procedure is used by a client to release a unicast or broadcast
 *  source stream.
 *
 *  Broadcast sink streams cannot be released, but can be deleted by
 *  bt_audio_broadcast_sink_delete().
 *  Broadcast source streams cannot be released, but can be deleted by
 *  bt_audio_broadcast_source_delete().
 *
 *  @param stream Stream object
 *  @param cache True to cache the codec configuration or false to forget it
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_stream_release(struct bt_audio_stream *stream, bool cache);

/** @brief Send data to Audio stream
 *
 *  Send data from buffer to the stream.
 *
 *  @note Data will not be sent to linked streams since linking is only
 *  consider for procedures affecting the state machine.
 *
 *  @param stream Stream object.
 *  @param buf Buffer containing data to be sent.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_audio_stream_send(struct bt_audio_stream *stream, struct net_buf *buf);

/** @brief Create audio unicast group.
 *
 *  Create a new audio unicast group with one or more audio streams as a
 *  unicast client. Streams in a unicast group shall share the same interval,
 *  framing and latency (see @ref bt_codec_qos).
 *
 *  @param[in]  streams        Array of stream object pointers being used for
 *                             the group.
 *  @param[in]  num_stream     Number of streams in @p streams.
 *  @param[out] unicast_group  Pointer to the unicast group created
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_unicast_group_create(struct bt_audio_stream *streams[],
				  size_t num_stream,
				  struct bt_audio_unicast_group **unicast_group);

/** @brief Add streams to a unicast group as a unicast client
 *
 *  This function can be used to add additional streams to a
 *  bt_audio_unicast_group.
 *
 *  This can be called at any time before any of the streams in the
 *  group has been started (see bt_audio_stream_ops.started()).
 *  This can also be called after the streams have been stopped
 *  (see bt_audio_stream_ops.stopped()).
 *
 *  @param unicast_group  Pointer to the unicast group
 *  @param streams        Array of stream object pointers being added to the
 *                        group.
 *  @param num_stream     Number of streams in @p streams.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_unicast_group_add_streams(struct bt_audio_unicast_group *unicast_group,
				       struct bt_audio_stream *streams[],
				       size_t num_stream);

/** @brief Remove streams from a unicast group as a unicast client
 *
 *  This function can be used to remove streams from a bt_audio_unicast_group.
 *
 *  This can be called at any time before any of the streams in the
 *  group has been QoS configured (see bt_audio_stream_ops.qos_set()).
 *
 *  @param unicast_group  Pointer to the unicast group
 *  @param streams        Array of stream object pointers removed from the
 *                        group.
 *  @param num_stream     Number of streams in @p streams.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_unicast_group_remove_streams(struct bt_audio_unicast_group *unicast_group,
					  struct bt_audio_stream *streams[],
					  size_t num_stream);

/** @brief Delete audio unicast group.
 *
 *  Delete a audio unicast group as a client. All streams in the group shall
 *  be in the idle or configured state.
 *
 *  @param unicast_group  Pointer to the unicast group to delete
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_unicast_group_delete(struct bt_audio_unicast_group *unicast_group);

/** @} */ /* End of group bt_audio_client */


/**
 * @brief Audio Broadcast APIs
 * @defgroup bt_audio_broadcast Audio Broadcast APIs
 * @{
 */

/** @brief Create audio broadcast source.
 *
 *  Create a new audio broadcast source with one or more audio streams.
 *
 *  The broadcast source will be visible for scanners once this has been called,
 *  and the device will advertise audio announcements.
 *
 *  No audio data can be sent until bt_audio_broadcast_source_start() has been
 *  called and no audio information (BIGInfo) will be visible to scanners
 *  (see bt_le_per_adv_sync_cb).
 *
 *  @param[in]  streams     Array of stream object pointers being used for the
 *                          broadcaster. This array shall remain valid for the
 *                          duration of the broadcast source.
 *  @param[in]  num_stream  Number of streams in @p streams.
 *  @param[in]  codec       Codec configuration.
 *  @param[in]  qos         Quality of Service configuration
 *  @param[out] source      Pointer to the broadcast source created
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_source_create(struct bt_audio_stream *streams[],
				     size_t num_stream,
				     struct bt_codec *codec,
				     struct bt_codec_qos *qos,
				     struct bt_audio_broadcast_source **source);

/** @brief Reconfigure audio broadcast source.
 *
 *  Reconfigure an audio broadcast source with a new codec and codec quality of
 *  service parameters.
 *
 *  @param source      Pointer to the broadcast source
 *  @param codec       Codec configuration.
 *  @param qos         Quality of Service configuration
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_source_reconfig(struct bt_audio_broadcast_source *source,
				       struct bt_codec *codec,
				       struct bt_codec_qos *qos);

/** @brief Start audio broadcast source.
 *
 *  Start an audio broadcast source with one or more audio streams.
 *  The broadcast source will start advertising BIGInfo, and audio data can
 *  be streamed.
 *
 *  @param source      Pointer to the broadcast source
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_source_start(struct bt_audio_broadcast_source *source);

/** @brief Stop audio broadcast source.
 *
 *  Stop an audio broadcast source.
 *  The broadcast source will stop advertising BIGInfo, and audio data can no
 *  longer be streamed.
 *
 *  @param source      Pointer to the broadcast source
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_source_stop(struct bt_audio_broadcast_source *source);

/** @brief Delete audio broadcast source.
 *
 *  Delete an audio broadcast source.
 *  The broadcast source will stop advertising entirely, and the source can
 *  no longer be used.
 *
 *  @param source      Pointer to the broadcast source
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_source_delete(struct bt_audio_broadcast_source *source);

/** @brief Register Broadcast sink callbacks
 * *
 *  @param cb  Broadcast sink callback structure.
 */
void bt_audio_broadcast_sink_register_cb(struct bt_audio_broadcast_sink_cb *cb);

/** @brief Start scan for broadcast sources.
 *
 *  Starts a scan for broadcast sources. Scan results will be received by
 *  the scan_recv callback.
 *  Only reports from devices advertising broadcast audio support will be sent.
 *  Note that a broadcast source may advertise broadcast audio capabilities,
 *  but may not be streaming.
 *
 *  @param param Scan parameters.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_sink_scan_start(const struct bt_le_scan_param *param);

/**
 * @brief Stop scan for broadcast sources.
 *
 *  Stops ongoing scanning for broadcast sources.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_sink_scan_stop(void);

/** @brief Sync to a broadcaster's audio
 *
 *  @param sink               Pointer to the sink object from the base_recv
 *                            callback.
 *  @param indexes_bitfield   Bitfield of the BIS index to sync to. To sync to
 *                            e.g. BIS index 1 and 2, this should have the value
 *                            of BIT(1) | BIT(2).
 *  @param streams            Stream object pointerss to be used for the
 *                            receiver. If multiple BIS indexes shall be
 *                            synchronized, multiple streams shall be provided.
 *  @param codec              Codec configuration.
 *  @param broadcast_code     The 16-octet broadcast code. Shall be supplied if
 *                            the broadcast is encrypted (see the syncable
 *                            callback).
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_broadcast_sink_sync(struct bt_audio_broadcast_sink *sink,
				 uint32_t indexes_bitfield,
				 struct bt_audio_stream *streams[],
				 struct bt_codec *codec,
				 const uint8_t broadcast_code[16]);

/** @brief Stop audio broadcast sink.
 *
 *  Stop an audio broadcast sink.
 *  The broadcast sink will stop receiving BIGInfo, and audio data can no
 *  longer be streamed.
 *
 *  @param sink      Pointer to the broadcast sink
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_sink_stop(struct bt_audio_broadcast_sink *sink);

/** @brief Release a broadcast sink
 *
 *  Once a broadcast sink has been allocated after the pa_synced callback,
 *  it can be deleted using this function. If the sink has synchronized to any
 *  broadcast audio streams, these must first be stopped using
 *  bt_audio_stream_stop.
 *
 *  @param sink Pointer to the sink object to delete.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_broadcast_sink_delete(struct bt_audio_broadcast_sink *sink);

/** @} */ /* End of bt_audio_broadcast */


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
 *  @ref bt_audio_meta_type
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
