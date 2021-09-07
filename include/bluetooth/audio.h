/** @file
 *  @brief Bluetooth Audio handling
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_H_

#include <sys/atomic.h>
#include <bluetooth/buf.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <bluetooth/iso.h>
#include <bluetooth/lc3.h>
#include <bluetooth/gatt.h>

/**
 * @brief Bluetooth Audio
 * @defgroup bt_audio Bluetooth Audio
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct bt_audio_chan;
struct bt_audio_cap;

#define BT_AUDIO_ERR(_err)  (-(_err))

#define BT_BROADCAST_ID_SIZE             3 /* octets */

#if defined(CONFIG_BT_BAP) && defined(CONFIG_BT_AUDIO_BROADCAST)
#define BROADCAST_SNK_STREAM_CNT CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT
#define BROADCAST_SUBGROUP_CNT CONFIG_BT_BAP_BROADCAST_SUBGROUP_COUNT
#else /* !(CONFIG_BT_BAP && CONFIG_BT_AUDIO_BROADCAST) */
#define BROADCAST_SNK_STREAM_CNT 0
#define BROADCAST_SUBGROUP_CNT 0
#endif /* CONFIG_BT_BAP && CONFIG_BT_AUDIO_BROADCAST*/

/** @brief Abstract Audio endpoint structure. */
struct bt_audio_endpoint;

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
 *  bt_audio_chan_config or bt_audio_chan_reconfig.
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

/* TODO: Remove base once LTV types are defined, are these specific to LC3? */
#define BT_CODEC_META_BASE               0x01

#define BT_CODEC_META_PREFER_CONTEXT     (BT_CODEC_META_BASE)
#define BT_CODEC_META_CONTEXT            (BT_CODEC_META_BASE + 1)

/* @def BT_CODEC_META_CONTEXT_NONE
 *
 * Unspecified. Matches any audio content.
 */
#define BT_CODEC_META_CONTEXT_NONE       BIT(0)

/* @def BT_CODEC_META_CONTEXT_VOICE
 *
 * Conversation between humans as, for example, in telephony or video calls.
 */
#define BT_CODEC_META_CONTEXT_VOICE      BIT(1)

/* @def BT_CODEC_META_CONTEXT_MEDIA
 *
 * Media as, for example, in music, public radio, podcast or video soundtrack.
 * Conversation between humans as, for example, in telephony or video calls.
 */
#define BT_CODEC_META_CONTEXT_MEDIA      BIT(2)

/* @def BT_CODEC_META_CONTEXT_INSTRUCTION
 *
 * Instructional audio as, for example, in navigation, traffic announcements or
 * user guidance.
 */
#define BT_CODEC_META_CONTEXT_INSTRUCTION BIT(3)

/* @def BT_CODEC_META_CONTEXT_ATTENTION
 *
 * Attention seeking audio as, for example, in beeps signalling arrival of a
 * message or keyboard clicks.
 */
#define BT_CODEC_META_CONTEXT_ATTENTION  BIT(4)

/* @def BT_CODEC_META_CONTEXT_ALERT
 *
 * Immediate alerts as, for example, in a low battery alarm, timer expiry or
 * alarm clock.
 */
#define BT_CODEC_META_CONTEXT_ALERT      BIT(5)

/* @def BT_CODEC_META_CONTEXT_MAN_MACHINE
 *
 * Man machine communication as, for example, with voice recognition or
 * virtual assistant.
 */
#define BT_CODEC_META_CONTEXT_MAN_MACHINE BIT(6)

/* @def BT_CODEC_META_CONTEXT_EMERGENCY
 *
 * Emergency alerts as, for example, with fire alarms or other urgent alerts.
 */
#define BT_CODEC_META_CONTEXT_EMERGENCY  BIT(7)

/* @def BT_CODEC_META_CONTEXT_RINGTONE
 *
 * Ringtone as in a call alert.
 */
#define BT_CODEC_META_CONTEXT_RINGTONE   BIT(8)

/* @def BT_CODEC_META_CONTEXT_TV
 *
 * Audio associated with a television program and/or with metadata conforming
 * to the Bluetooth Broadcast TV profile.
 */
#define BT_CODEC_META_CONTEXT_TV         BIT(9)

/* @def BT_CODEC_META_CONTEXT_ANY
 *
 * Any known context.
 */
#define BT_CODEC_META_CONTEXT_ANY	 (BT_CODEC_META_CONTEXT_NONE | \
					  BT_CODEC_META_CONTEXT_VOICE | \
					  BT_CODEC_META_CONTEXT_MEDIA | \
					  BT_CODEC_META_CONTEXT_INSTRUCTION | \
					  BT_CODEC_META_CONTEXT_ATTENTION | \
					  BT_CODEC_META_CONTEXT_ALERT | \
					  BT_CODEC_META_CONTEXT_MAN_MACHINE | \
					  BT_CODEC_META_CONTEXT_EMERGENCY | \
					  BT_CODEC_META_CONTEXT_RINGTONE | \
					  BT_CODEC_META_CONTEXT_TV)

/** @brief Codec structure. */
struct bt_codec {
	/** Codec ID */
	uint8_t  id;
	/** Codec Company ID */
	uint16_t cid;
	/** Codec Company Vendor ID */
	uint16_t vid;
	/** Codec Specific Data count */
	uint8_t  data_count;
	/** Codec Specific Data */
	struct bt_codec_data data[CONFIG_BT_CODEC_MAX_DATA_COUNT];
	/** Codec Specific Metadata count */
	uint8_t  meta_count;
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
	uint8_t  data_count;
	/** Codec Specific Data
	 *
	 *  Only valid if the data_count of struct bt_codec in the subgroup is 0
	 */
	struct bt_codec_data data[CONFIG_BT_CODEC_MAX_DATA_COUNT];
};

struct bt_audio_base_subgroup {
	/* Number of BIS in the subgroup */
	uint8_t bis_count;
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
	uint8_t subgroup_count;
	/* Array of subgroups in the BASE */
	struct bt_audio_base_subgroup subgroups[BROADCAST_SUBGROUP_CNT];
};

/** @def BT_CODEC_QOS
 *  @brief Helper to declare elements of bt_codec_qos
 *
 *  @param _dir direction
 *  @param _interval SDU interval (usec)
 *  @param _framing Framing
 *  @param _phy Target PHY
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS(_dir, _interval, _framing, _phy, _sdu, _rtn, _latency, \
		     _pd) \
	{ \
		.dir = _dir, \
		.interval = _interval, \
		.framing = _framing, \
		.phy = _phy, \
		.sdu = _sdu, \
		.rtn = _rtn, \
		.latency = _latency, \
		.pd = _pd, \
	}

/** @brief Audio QoS direction */
enum {
	BT_CODEC_QOS_IN,
	BT_CODEC_QOS_OUT,
	BT_CODEC_QOS_INOUT
};

/** @brief Codec QoS direction */
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

/** @def BT_CODEC_QOS_IN_UNFRAMED
 *  @brief Helper to declare Input Unframed bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_IN_UNFRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(BT_CODEC_QOS_IN, _interval, BT_CODEC_QOS_UNFRAMED, \
		     BT_CODEC_QOS_2M, _sdu, _rtn, _latency, _pd)

/** @def BT_CODEC_QOS_OUT_UNFRAMED
 *  @brief Helper to declare Output Unframed bt_code *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_OUT_UNFRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(BT_CODEC_QOS_OUT, _interval, BT_CODEC_QOS_UNFRAMED, \
		     BT_CODEC_QOS_2M, _sdu, _rtn, _latency, _pd)

/** @def BT_CODEC_QOS_OUT_UNFRAMED
 *  @brief Helper to declare Input/Output Unframed bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_INOUT_UNFRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(BT_CODEC_QOS_INOUT, _interval, BT_CODEC_QOS_UNFRAMED, \
		     BT_CODEC_QOS_2M, _sdu, _rtn, _latency, _pd)

/** @def BT_CODEC_QOS_IN_FRAMED
 *  @brief Helper to declare Input Framed bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_IN_FRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(BT_CODEC_QOS_IN, _interval, BT_CODEC_QOS_FRAMED, \
		     BT_CODEC_QOS_2M, _sdu, _rtn, _latency, _pd)

/** @def BT_CODEC_QOS_OUT_FRAMED
 *  @brief Helper to declare Output Framed bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_OUT_FRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(BT_CODEC_QOS_OUT, _interval, BT_CODEC_QOS_FRAMED, \
		     BT_CODEC_QOS_2M, _sdu, _rtn, _latency, _pd)

/** @def BT_CODEC_QOS_OUT_FRAMED
 *  @brief Helper to declare Output Framed bt_codec_qos
 *
 *  @param _interval SDU interval (usec)
 *  @param _sdu Maximum SDU Size
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd Presentation Delay (usec)
 */
#define BT_CODEC_QOS_INOUT_FRAMED(_interval, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(BT_CODEC_QOS_INOUT, _interval, BT_CODEC_QOS_FRAMED, \
		     BT_CODEC_QOS_2M, _sdu, _rtn, _latency, _pd)

/** @brief Codec QoS structure. */
struct bt_codec_qos {
	/** QoS direction
	 *
	 *  This shall be set to BT_CODEC_QOS_OUT for broadcast sources, and
	 *  shall be set to BT_CODEC_QOS_IN for broadcast sinks.
	 */
	uint8_t  dir;
	/** QoS Frame Interval */
	uint32_t interval;
	/** QoS Framing */
	uint8_t  framing;
	/** QoS PHY */
	uint8_t  phy;
	/** QoS SDU */
	uint16_t sdu;
	/** QoS Retransmission Number */
	uint8_t rtn;
	/** QoS Transport Latency */
	uint16_t latency;
	/** QoS Presentation Delay */
	uint32_t pd;
};

/** @brief Audio channel structure.
 *
 *  Audio Channels represents a stream configuration of a Remote Endpoint and
 *  a Local Capability.
 *
 *  @note Audio channels are unidirectional although its QoS can be configured
 *  to be bidirectional if channel are linked, in which case the QoS must be
 *  symmetric in both directions.
 */
struct bt_audio_chan {
	/** Connection reference */
	struct bt_conn *conn;
	/** Endpoint reference */
	struct bt_audio_ep *ep;
	/** Capability reference */
	struct bt_audio_capability *cap;
	/** Codec Configuration */
	struct bt_codec *codec;
	/** QoS Configuration */
	struct bt_codec_qos *qos;
	/** ISO channel reference */
	struct bt_iso_chan *iso;
	/** Audio channel operations */
	struct bt_audio_chan_ops *ops;
	sys_slist_t links;
	sys_snode_t node;
	uint8_t  state;
};

/** @brief Capability operations structure.
 *
 *  These are only used for unicast channels and broadcast sink channels.
 */
struct bt_audio_capability_ops {
	/** @brief Capability config callback
	 *
	 *  Config callback is called whenever a new Audio Channel needs to be
	 *  allocated.
	 *
	 *  @param conn Connection object
	 *  @param ep Remote Audio Endpoint being configured
	 *  @param cap Local Audio Capability being configured
	 *  @param codec Codec configuration
	 *
	 *  @return Allocated Audio Channel object or NULL in case of error.
	 */
	struct bt_audio_chan *(*config)(struct bt_conn *conn,
					struct bt_audio_ep *ep,
					struct bt_audio_capability *cap,
					struct bt_codec *codec);

	/** @brief Capability reconfig callback
	 *
	 *  Reconfig callback is called whenever an Audio Channel needs to be
	 *  reconfigured with different codec configuration.
	 *
	 *  @param chan Channel object being reconfigured.
	 *  @param cap Local Audio Capability being reconfigured
	 *  @param codec Codec configuration
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*reconfig)(struct bt_audio_chan *chan,
			struct bt_audio_capability *cap,
			struct bt_codec *codec);

	/** @brief Capability QoS callback
	 *
	 *  QoS callback is called whenever an Audio Channel Quality of
	 *  Service needs to be configured.
	 *
	 *  @param chan Channel object being reconfigured.
	 *  @param QoS Quality of Service configuration
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*qos)(struct bt_audio_chan *chan, struct bt_codec_qos *qos);

	/** @brief Capability Enable callback
	 *
	 *  Enable callback is called whenever an Audio Channel is about to be
	 *  enabled to stream.
	 *
	 *  @param chan Channel object being enabled.
	 *  @param meta_count Number of metadata entries
	 *  @param meta Metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*enable)(struct bt_audio_chan *chan, uint8_t meta_count,
		      struct bt_codec_data *meta);

	/** @brief Capability Start callback
	 *
	 *  Start callback is called whenever an Audio Channel is about to be
	 *  start streaming.
	 *
	 *  @param chan Channel object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*start)(struct bt_audio_chan *chan);

	/** @brief Capability Metadata callback
	 *
	 *  Metadata callback is called whenever an Audio Channel is needs to
	 *  update its metadata.
	 *
	 *  @param chan Channel object.
	 *  @param meta_count Number of metadata entries
	 *  @param meta Metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*metadata)(struct bt_audio_chan *chan, uint8_t meta_count,
			struct bt_codec_data *meta);

	/** @brief Capability Disable callback
	 *
	 *  Disable callback is called whenever an Audio Channel is about to be
	 *  disabled to stream.
	 *
	 *  @param chan Channel object being disabled.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*disable)(struct bt_audio_chan *chan);

	/** @brief Capability Stop callback
	 *
	 *  Stop callback is called whenever an Audio Channel is about to be
	 *  stop streaming.
	 *
	 *  @param chan Channel object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*stop)(struct bt_audio_chan *chan);

	/** @brief Capability release callback
	 *
	 *  Release callback is called whenever a new Audio Channel needs to be
	 *  deallocated.
	 *
	 *  @param chan Channel object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*release)(struct bt_audio_chan *chan);

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
};

/** @brief Channel operation. */
struct bt_audio_chan_ops {
	/** @brief Channel connected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param chan The channel that has been connected
	 */
	void (*connected)(struct bt_audio_chan *chan);

	/** @brief Channel disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  channel is disconnected, including when a connection gets
	 *  rejected.
	 *
	 *  @param chan   The channel that has been Disconnected
	 *  @param reason HCI reason for the disconnection.
	 */
	void (*disconnected)(struct bt_audio_chan *chan, uint8_t reason);

	/** @brief Channel audio HCI receive callback.
	 *
	 *  This callback is only used if the ISO data path is HCI.
	 *
	 *  @param chan Channel object.
	 *  @param buf  Buffer containing incoming audio data.
	 */
	void (*recv)(struct bt_audio_chan *chan, struct net_buf *buf);
};

/** @brief Audio Capability type */
enum bt_audio_pac_type {
	BT_AUDIO_SINK = 0x01,
	BT_AUDIO_SOURCE = 0x02,
} __packed;

#define BT_AUDIO_CONTENT_UNSPECIFIED     BIT(0)
#define BT_AUDIO_CONTENT_CONVERSATIONAL  BIT(1)
#define BT_AUDIO_CONTENT_MEDIA           BIT(2)
#define BT_AUDIO_CONTENT_INSTRUMENTAL    BIT(3)
#define BT_AUDIO_CONTENT_ATTENTION       BIT(4)
#define BT_AUDIO_CONTENT_ALERT           BIT(5)
#define BT_AUDIO_CONTENT_MAN_MACHINE     BIT(6)
#define BT_AUDIO_CONTENT_EMERGENCY       BIT(7)

/** @def BT_AUDIO_QOS
 *  @brief Helper to declare elements of bt_audio_qos
 *
 *  @param _framing Framing
 *  @param _phy Target PHY
 *  @param _rtn Retransmission number
 *  @param _latency Maximum Transport Latency (msec)
 *  @param _pd_min Minimum Presentation Delay (usec)
 *  @param _pd_max Maximum Presentation Delay (usec)
 */
#define BT_AUDIO_QOS(_framing, _phy, _rtn, _latency, _pd_min, _pd_max) \
	{ \
		.framing = _framing, \
		.phy = _phy, \
		.rtn = _rtn, \
		.latency = _latency, \
		.pd_min = _pd_min, \
		.pd_max = _pd_max, \
	}

/** @brief Audio Capability QoS structure. */
struct bt_audio_qos {
	/** QoS Framing */
	uint8_t  framing;
	/** QoS PHY */
	uint8_t  phy;
	/** QoS Retransmission Number */
	uint8_t  rtn;
	/** QoS Transport Latency */
	uint16_t latency;
	/** QoS Minimun Presentation Delay */
	uint32_t pd_min;
	/** QoS Maximun Presentation Delay */
	uint32_t pd_max;
};

/** @brief Audio Capability structure.
 *
 *  Audio Capability represents a Local Codec including its preferrable
 *  Quality of service.
 *
 */
struct bt_audio_capability {
	/** Capability type */
	uint8_t  type;
	/** Supported Audio Contexts */
	uint16_t context;
	/** Capability codec reference */
	struct bt_codec *codec;
	/** Capability QoS */
	struct bt_audio_qos qos;
	/** Capability operations reference */
	struct bt_audio_capability_ops *ops;
	sys_snode_t node;
};

/** @brief Register Audio Capability.
 *
 *  Register Audio Local Capability.
 *
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_capability_register(struct bt_audio_capability *cap);

/** @brief Unregister Audio Capability.
 *
 *  Unregister Audio Local Capability.
 *
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_capability_unregister(struct bt_audio_capability *cap);

/** @brief Register Audio callbacks for a channel.
 *
 *  Register Audio callbacks for a channel.
 *
 *  @param chan Channel object.
 *  @param ops  Channel operations structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
void bt_audio_chan_cb_register(struct bt_audio_chan *chan, struct bt_audio_chan_ops *ops);

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
typedef void
(*bt_audio_discover_func_t)(struct bt_conn *conn,
			    struct bt_audio_capability *cap,
			    struct bt_audio_ep *ep,
			    struct bt_audio_discover_params *params);

struct bt_audio_discover_params {
	/** Capabilities type */
	uint8_t  type;
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

/** @brief Configure Audio Channel
 *
 *  This procedure is used by a client to configure a new channel using the
 *  remote endpoint, local capability and codec configuration.
 *
 *  @param conn Connection object
 *  @param ep Remote Audio Endpoint being configured
 *  @param cap Local Audio Capability being configured
 *  @param codec Codec configuration
 *
 *  @return Allocated Audio Channel object or NULL in case of error.
 */
struct bt_audio_chan *bt_audio_chan_config(struct bt_conn *conn,
					   struct bt_audio_ep *ep,
					   struct bt_audio_capability *cap,
					   struct bt_codec *codec);

/** @brief Reconfigure Audio Channel
 *
 *  This procedure is used by a client to reconfigure a channel using the
 *  a different local capability and/or codec configuration.
 *
 *  This can only be done for unicast channels.
 *
 *  @param chan Channel object being reconfigured
 *  @param cap Local Audio Capability being reconfigured
 *  @param codec Codec configuration
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_reconfig(struct bt_audio_chan *chan,
			   struct bt_audio_capability *cap,
			   struct bt_codec *codec);

/** @brief Configure Audio Channel QoS
 *
 *  This procedure is used by a client to configure the Quality of Service of
 *  a channel. This shall only be used to configure a unicast channel.
 *
 *  @param chan Channel object
 *  @param qos Quality of Service configuration
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_qos(struct bt_audio_chan *chan, struct bt_codec_qos *qos);

/** @brief Enable Audio Channel
 *
 *  This procedure is used by a client to enable a channel.
 *
 *  This shall only be called for unicast channels, as broadcast channels will
 *  always be enabled once created.
 *
 *  @param chan Channel object
 *  @param meta_count Number of metadata entries
 *  @param meta Metadata entries
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_enable(struct bt_audio_chan *chan,
			 uint8_t meta_count, struct bt_codec_data *meta);

/** @brief Change Audio Channel Metadata
 *
 *  This procedure is used by a client to change the metadata of a channel.
 *
 *  @param chan Channel object
 *  @param meta_count Number of metadata entries
 *  @param meta Metadata entries
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_metadata(struct bt_audio_chan *chan,
			   uint8_t meta_count, struct bt_codec_data *meta);

/** @brief Disable Audio Channel
 *
 *  This procedure is used by a client to disable a channel.
 *
 *  This shall only be called for unicast channels, as broadcast channels will
 *  always be enabled once created.
 *
 *  @param chan Channel object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_disable(struct bt_audio_chan *chan);

/** @brief Start Audio Channel
 *
 *  This procedure is used by a client to make a channel start streaming.
 *
 *  This shall only be called for unicast channels.
 *  Broadcast sinks will always be started once synchronized, and broadcast
 *  source channels shall be started with bt_audio_broadcast_source_start().
 *
 *  @param chan Channel object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_start(struct bt_audio_chan *chan);

/** @brief Stop Audio Channel
 *
 *  This procedure is used by a client to make a channel stop streaming.
 *
 *  This shall only be called for unicast channels.
 *  Broadcast sinks cannot be stopped.
 *  Broadcast sources shall be stopped with bt_audio_broadcast_source_stop().
 *
 *  @param chan Channel object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_stop(struct bt_audio_chan *chan);

/** @brief Release Audio Channel
 *
 *  This procedure is used by a client to release a unicast or broadcast
 *  source channel.
 *
 *  Broadcast sink channels cannot be released, but can be deleted by
 *  bt_audio_broadcast_sink_delete().
 *  Broadcast source channels cannot be released, but can be deleted by
 *  bt_audio_broadcast_source_delete().
 *
 *  @param chan Channel object
 *  @param cache True to cache the codec configuration or false to forget it
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_release(struct bt_audio_chan *chan, bool cache);

/** @brief Link Audio Channels
 *
 *  This procedure links channels so that any procedure on either of the
 *  channel is replicated on the other.
 *
 *  @param chan1 First Channel object
 *  @param chan2 Second Channel object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_link(struct bt_audio_chan *chan1,
		       struct bt_audio_chan *chan2);

/** @brief Unlink Audio Channels
 *
 *  This procedure links channels so that any procedure on either of the
 *  channel is replicated on the other.
 *
 *  @param chan1 First Channel object
 *  @param chan2 Second Channel object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_unlink(struct bt_audio_chan *chan1,
			 struct bt_audio_chan *chan2);

/** @brief Send data to Audio channel
 *
 *  Send data from buffer to the channel.
 *
 *  @note Data will not be sent to linked channels since linking is only
 *  consider for procedures affecting the state machine.
 *
 *  @param chan Channel object.
 *  @param buf Buffer containing data to be sent.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_audio_chan_send(struct bt_audio_chan *chan, struct net_buf *buf);

/** @brief Create audio broadcast source.
 *
 *  Create a new audio broadcast source with one or more audio channels.
 *  To create a broadcast source with multiple channels, the channels must be
 *  linked with bt_audio_chan_link().
 *
 *  The broadcast source will be visible for scanners once this has been called,
 *  and the device will advertise audio announcements.
 *
 *  No audio data can be sent until bt_audio_broadcast_source_start() has been
 *  called and no audio information (BIGInfo) will be visible to scanners
 *  (see bt_le_per_adv_sync_cb).
 *
 *  @param[in]  chans       Array of channel objects being used for the
 *                          broadcaster. This array shall remain valid for the
 *                          duration of the broadcast source.
 *  @param[in]  num_chan    Number of channels in @p chans.
 *  @param[in]  codec       Codec configuration.
 *  @param[in]  qos         Quality of Service configuration
 *  @param[out] source      Pointer to the broadcast source created
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_audio_broadcast_source_create(struct bt_audio_chan *chans,
				     uint8_t num_chan,
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
 *  Start an audio broadcast source with one or more audio channels.
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

/** @brief Start scan for broadcast sources.
 *
 *  Starts a scan for broadcast sources. Scan results will be received by
 *  the scan_recv callback of the struct bt_audio_capability_ops.
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
 *  @param chan               Channel object to be used for the receiver. If
 *                            multiple BIS indexes shall be synchronized,
 *                            multiple channels shall be provided. To provide
 *                            multiple channels use bt_audio_chan_link to link
 *                            them.
 *  @param broadcast_code     The 16-octet broadcast code. Shall be supplied if
 *                            the broadcast is encrypted (see the syncable
 *                            callback).
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_broadcast_sink_sync(struct bt_audio_broadcast_sink *sink,
				 uint32_t indexes_bitfield,
				 struct bt_audio_chan *chan,
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
 *  broadcast audio channels, these must first be stopped using
 *  bt_audio_chan_stop.
 *
 *  @param sink Pointer to the sink object to delete.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_broadcast_sink_delete(struct bt_audio_broadcast_sink *sink);

/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_H_ */
