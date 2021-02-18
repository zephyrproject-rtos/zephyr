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

/** @brief Abstract Audio endpoint structure. */
struct bt_audio_endpoint;

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
#define BT_CODEC_META_BASE		0x01

#define BT_CODEC_META_PREFER_CONTEXT	(BT_CODEC_META_BASE)
#define BT_CODEC_META_CONTEXT		(BT_CODEC_META_BASE + 1)

/* @def BT_CODEC_META_CONTEXT_NONE
 *
 * Unspecified. Matches any audio content.
 */
#define BT_CODEC_META_CONTEXT_NONE	BIT(0)

/* @def BT_CODEC_META_CONTEXT_VOICE
 *
 * Conversation between humans as, for example, in telephony or video calls.
 */
#define BT_CODEC_META_CONTEXT_VOICE	BIT(1)

/* @def BT_CODEC_META_CONTEXT_MEDIA
 *
 * Media as, for example, in music, public radio, podcast or video soundtrack.
 * Conversation between humans as, for example, in telephony or video calls.
 */
#define BT_CODEC_META_CONTEXT_MEDIA	BIT(2)

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
#define BT_CODEC_META_CONTEXT_ATTENTION	BIT(4)

/* @def BT_CODEC_META_CONTEXT_ALERT
 *
 * Immediate alerts as, for example, in a low battery alarm, timer expiry or
 * alarm clock.
 */
#define BT_CODEC_META_CONTEXT_ALERT	BIT(5)

/* @def BT_CODEC_META_CONTEXT_MAN_MACHINE
 *
 * Man machine communication as, for example, with voice recognition or
 * virtual assistant.
 */
#define BT_CODEC_META_CONTEXT_MAN_MACHINE	BIT(6)

/* @def BT_CODEC_META_CONTEXT_EMERGENCY
 *
 * Emergency alerts as, for example, with fire alarms or other urgent alerts.
 */
#define BT_CODEC_META_CONTEXT_EMERGENCY	BIT(7)

/* @def BT_CODEC_META_CONTEXT_RINGTONE
 *
 * Ringtone as in a call alert.
 */
#define BT_CODEC_META_CONTEXT_RINGTONE	BIT(8)

/* @def BT_CODEC_META_CONTEXT_TV
 *
 * Audio associated with a television program and/or with metadata conforming
 * to the Bluetooth Broadcast TV profile.
 */
#define BT_CODEC_META_CONTEXT_TV	BIT(9)

/* @def BT_CODEC_META_CONTEXT_ANY
 *
 * Any known context.
 */
#define BT_CODEC_META_CONTEXT_ANY	(BT_CODEC_META_CONTEXT_NONE | \
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
	/** QoS direction */
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
	sys_slist_t links;
	sys_snode_t node;
	uint8_t  state;
};

/** @brief Capability operations structure. */
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
	struct bt_audio_chan	*(*config)(struct bt_conn *conn,
					   struct bt_audio_ep *ep,
					   struct bt_audio_capability *cap,
					   struct bt_codec *codec);

	/** @brief Capability reconfig callback
	 *
	 *  Reconfig callback is called whenever an Audio Channel needs to be
	 *  reconfigured with different codec configuration.
	 *
	 *  @param channel Channel object being reconfigured
	 *  @param cap Local Audio Capability being reconfigured
	 *  @param codec Codec configuration
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*reconfig)(struct bt_audio_chan *chan,
					    struct bt_audio_capability *cap,
					    struct bt_codec *codec);

	/** @brief Capability QoS callback
	 *
	 *  QoS callback is called whenever an Audio Channel Quality of
	 *  Service needs to be configured.
	 *
	 *  @param channel Channel object being reconfigured
	 *  @param QoS Quality of Service configuration
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*qos)(struct bt_audio_chan *chan,
				       struct bt_codec_qos *qos);

	/** @brief Capability Enable callback
	 *
	 *  Enable callback is called whenever an Audio Channel is about to be
	 *  enabled to stream.
	 *
	 *  @param channel Channel object being enabled
	 *  @param meta_count Number of metadata entries
	 *  @param meta Metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*enable)(struct bt_audio_chan *chan,
					  uint8_t meta_count,
					  struct bt_codec_data *meta);

	/** @brief Capability Start callback
	 *
	 *  Start callback is called whenever an Audio Channel is about to be
	 *  start streaming.
	 *
	 *  @param channel Channel object
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*start)(struct bt_audio_chan *chan);

	/** @brief Capability Metadata callback
	 *
	 *  Metadata callback is called whenever an Audio Channel is needs to
	 *  update its metadata.
	 *
	 *  @param channel Channel object
	 *  @param meta_count Number of metadata entries
	 *  @param meta Metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*metadata)(struct bt_audio_chan *chan,
					    uint8_t meta_count,
					    struct bt_codec_data *meta);

	/** @brief Capability Disable callback
	 *
	 *  Disable callback is called whenever an Audio Channel is about to be
	 *  disabled to stream.
	 *
	 *  @param channel Channel object being disabled.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*disable)(struct bt_audio_chan *chan);

	/** @brief Capability Stop callback
	 *
	 *  Stop callback is called whenever an Audio Channel is about to be
	 *  stop streaming.
	 *
	 *  @param channel Channel object
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*stop)(struct bt_audio_chan *chan);

	/** @brief Capability release callback
	 *
	 *  Release callback is called whenever a new Audio Channel needs to be
	 *  deallocated.
	 *
	 *  @param channel Channel object
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int			(*release)(struct bt_audio_chan *chan);
};

/** @brief Audio Capability type */
enum bt_audio_pac_type {
	BT_AUDIO_SINK = 0x01,
	BT_AUDIO_SOURCE = 0x02,
} __packed;

#define BT_AUDIO_CONTENT_UNSPECIFIED	BIT(0)
#define BT_AUDIO_CONTENT_CONVERSATIONAL	BIT(1)
#define BT_AUDIO_CONTENT_MEDIA		BIT(2)
#define BT_AUDIO_CONTENT_INSTRUMENTAL	BIT(3)
#define BT_AUDIO_CONTENT_ATTENTION	BIT(4)
#define BT_AUDIO_CONTENT_ALERT		BIT(5)
#define BT_AUDIO_CONTENT_MAN_MACHINE	BIT(6)
#define BT_AUDIO_CONTENT_EMERGENCY	BIT(7)

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
 *  a channel.
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
 *  @param chan Channel object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_disable(struct bt_audio_chan *chan);

/** @brief Start Audio Channel
 *
 *  This procedure is used by a client to make a channel start streaming.
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
 *  @param chan Channel object
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_chan_stop(struct bt_audio_chan *chan);

/** @brief Release Audio Channel
 *
 *  This procedure is used by a client release a channel..
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

/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_H_ */
