/** @file
 *  @brief Header for Bluetooth BAP.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BAP_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BAP_

/**
 * @brief Bluetooth Basic Audio Profile (BAP)
 * @defgroup bt_bap Bluetooth Basic Audio Profile
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>

#if defined(CONFIG_BT_BAP_SCAN_DELEGATOR)
#define BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN
#define BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS    CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS
#else
#define BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN 0
#define BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS    0
#endif

/** Periodic advertising state reported by the Scan Delegator */
enum bt_bap_pa_state {
	/** The periodic advertising has not been synchronized */
	BT_BAP_PA_STATE_NOT_SYNCED = 0x00,

	/** Waiting for SyncInfo from Broadcast Assistant */
	BT_BAP_PA_STATE_INFO_REQ = 0x01,

	/** Synchronized to periodic advertising */
	BT_BAP_PA_STATE_SYNCED = 0x02,

	/** Failed to synchronized to periodic advertising */
	BT_BAP_PA_STATE_FAILED = 0x03,

	/** No periodic advertising sync transfer receiver from Broadcast Assistant */
	BT_BAP_PA_STATE_NO_PAST = 0x04,
};

/** Broadcast Isochronous Group encryption state reported by the Scan Delegator */
enum bt_bap_big_enc_state {
	/** The Broadcast Isochronous Group not encrypted */
	BT_BAP_BIG_ENC_STATE_NO_ENC = 0x00,

	/** The Broadcast Isochronous Group broadcast code requested */
	BT_BAP_BIG_ENC_STATE_BCODE_REQ = 0x01,

	/** The Broadcast Isochronous Group decrypted */
	BT_BAP_BIG_ENC_STATE_DEC = 0x02,

	/** The Broadcast Isochronous Group bad broadcast code */
	BT_BAP_BIG_ENC_STATE_BAD_CODE = 0x03,
};

/** Broadcast Audio Scan Service (BASS) specific ATT error codes */
enum bt_bap_bass_att_err {
	/** Opcode not supported */
	BT_BAP_BASS_ERR_OPCODE_NOT_SUPPORTED = 0x80,

	/** Invalid source ID supplied */
	BT_BAP_BASS_ERR_INVALID_SRC_ID = 0x81,
};

/** Value indicating that the periodic advertising interval is unknown */
#define BT_BAP_PA_INTERVAL_UNKNOWN             0xFFFF

/** @brief Broadcast Assistant no BIS sync preference
 *
 * Value indicating that the Broadcast Assistant has no preference to which BIS
 * the Scan Delegator syncs to
 */
#define BT_BAP_BIS_SYNC_NO_PREF                0xFFFFFFFF

#if defined(CONFIG_BT_BAP_BROADCAST_SINK)
/* TODO: Since these are also used for the broadcast assistant,
 * they should not be tied to the broadcast sink
 */
#define BROADCAST_SNK_STREAM_CNT   CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT
#define BROADCAST_SNK_SUBGROUP_CNT CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT
#else /* !CONFIG_BT_BAP_BROADCAST_SINK */
#define BROADCAST_SNK_STREAM_CNT   0
#define BROADCAST_SNK_SUBGROUP_CNT 0
#endif /* CONFIG_BT_BAP_BROADCAST_SINK*/

/** Endpoint states */
enum bt_bap_ep_state {
	/** Audio Stream Endpoint Idle state */
	BT_BAP_EP_STATE_IDLE = 0x00,

	/** Audio Stream Endpoint Codec Configured state */
	BT_BAP_EP_STATE_CODEC_CONFIGURED = 0x01,

	/** Audio Stream Endpoint QoS Configured state */
	BT_BAP_EP_STATE_QOS_CONFIGURED = 0x02,

	/** Audio Stream Endpoint Enabling state */
	BT_BAP_EP_STATE_ENABLING = 0x03,

	/** Audio Stream Endpoint Streaming state */
	BT_BAP_EP_STATE_STREAMING = 0x04,

	/** Audio Stream Endpoint Disabling state */
	BT_BAP_EP_STATE_DISABLING = 0x05,

	/** Audio Stream Endpoint Streaming state */
	BT_BAP_EP_STATE_RELEASING = 0x06,
};

/**
 * @brief Response Status Code
 *
 * These are sent by the server to the client when a stream operation is
 * requested.
 */
enum bt_bap_ascs_rsp_code {
	/** Server completed operation successfully */
	BT_BAP_ASCS_RSP_CODE_SUCCESS = 0x00,
	/** Server did not support operation by client */
	BT_BAP_ASCS_RSP_CODE_NOT_SUPPORTED = 0x01,
	/** Server rejected due to invalid operation length */
	BT_BAP_ASCS_RSP_CODE_INVALID_LENGTH = 0x02,
	/** Invalid ASE ID */
	BT_BAP_ASCS_RSP_CODE_INVALID_ASE = 0x03,
	/** Invalid ASE state */
	BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE = 0x04,
	/** Invalid operation for direction */
	BT_BAP_ASCS_RSP_CODE_INVALID_DIR = 0x05,
	/** Capabilities not supported by server */
	BT_BAP_ASCS_RSP_CODE_CAP_UNSUPPORTED = 0x06,
	/** Configuration parameters not supported by server */
	BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED = 0x07,
	/** Configuration parameters rejected by server */
	BT_BAP_ASCS_RSP_CODE_CONF_REJECTED = 0x08,
	/** Invalid Configuration parameters */
	BT_BAP_ASCS_RSP_CODE_CONF_INVALID = 0x09,
	/** Unsupported metadata */
	BT_BAP_ASCS_RSP_CODE_METADATA_UNSUPPORTED = 0x0a,
	/** Metadata rejected by server */
	BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED = 0x0b,
	/** Invalid metadata */
	BT_BAP_ASCS_RSP_CODE_METADATA_INVALID = 0x0c,
	/** Server has insufficient resources */
	BT_BAP_ASCS_RSP_CODE_NO_MEM = 0x0d,
	/** Unspecified error */
	BT_BAP_ASCS_RSP_CODE_UNSPECIFIED = 0x0e,
};

/**
 * @brief Response Reasons
 *
 * These are used if the @ref bt_bap_ascs_rsp_code value is
 * @ref BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED, @ref BT_BAP_ASCS_RSP_CODE_CONF_REJECTED or
 * @ref BT_BAP_ASCS_RSP_CODE_CONF_INVALID.
 */
enum bt_bap_ascs_reason {
	/** No reason */
	BT_BAP_ASCS_REASON_NONE = 0x00,
	/** Codec ID */
	BT_BAP_ASCS_REASON_CODEC = 0x01,
	/** Codec configuration */
	BT_BAP_ASCS_REASON_CODEC_DATA = 0x02,
	/** SDU interval */
	BT_BAP_ASCS_REASON_INTERVAL = 0x03,
	/** Framing */
	BT_BAP_ASCS_REASON_FRAMING = 0x04,
	/** PHY */
	BT_BAP_ASCS_REASON_PHY = 0x05,
	/** Maximum SDU size*/
	BT_BAP_ASCS_REASON_SDU = 0x06,
	/** RTN */
	BT_BAP_ASCS_REASON_RTN = 0x07,
	/** Max transport latency */
	BT_BAP_ASCS_REASON_LATENCY = 0x08,
	/** Presendation delay */
	BT_BAP_ASCS_REASON_PD = 0x09,
	/** Invalid CIS mapping */
	BT_BAP_ASCS_REASON_CIS = 0x0a,
};

/** @brief Structure storing values of fields of ASE Control Point notification. */
struct bt_bap_ascs_rsp {
	/** @brief Value of the Response Code field. */
	enum bt_bap_ascs_rsp_code code;

	/**
	 * @brief Value of the Reason field.
	 *
	 * The meaning of this value depend on the Response Code field.
	 */
	union {
		/**
		 * @brief Response reason
		 *
		 * If the Response Code is one of the following:
		 * - @ref BT_BAP_ASCS_RSP_CODE_CONF_UNSUPPORTED
		 * - @ref BT_BAP_ASCS_RSP_CODE_CONF_REJECTED
		 * - @ref BT_BAP_ASCS_RSP_CODE_CONF_INVALID
		 * all values from @ref bt_bap_ascs_reason can be used.
		 *
		 * If the Response Code is one of the following:
		 * - @ref BT_BAP_ASCS_RSP_CODE_SUCCESS
		 * - @ref BT_BAP_ASCS_RSP_CODE_NOT_SUPPORTED
		 * - @ref BT_BAP_ASCS_RSP_CODE_INVALID_LENGTH
		 * - @ref BT_BAP_ASCS_RSP_CODE_INVALID_ASE
		 * - @ref BT_BAP_ASCS_RSP_CODE_INVALID_ASE_STATE
		 * - @ref BT_BAP_ASCS_RSP_CODE_INVALID_DIR
		 * - @ref BT_BAP_ASCS_RSP_CODE_CAP_UNSUPPORTED
		 * - @ref BT_BAP_ASCS_RSP_CODE_NO_MEM
		 * - @ref BT_BAP_ASCS_RSP_CODE_UNSPECIFIED
		 * only value @ref BT_BAP_ASCS_REASON_NONE shall be used.
		 */
		enum bt_bap_ascs_reason reason;

		/**
		 * @brief Response metadata type
		 *
		 * If the Response Code is one of the following:
		 * - @ref BT_BAP_ASCS_RSP_CODE_METADATA_UNSUPPORTED
		 * - @ref BT_BAP_ASCS_RSP_CODE_METADATA_REJECTED
		 * - @ref BT_BAP_ASCS_RSP_CODE_METADATA_INVALID
		 * the value of the Metadata Type shall be used.
		 */
		enum bt_audio_metadata_type metadata_type;
	};
};

/**
 * @brief Macro used to initialise the object storing values of ASE Control Point notification.
 *
 * @param c Response Code field
 * @param r Reason field - @ref bt_bap_ascs_reason or @ref bt_audio_metadata_type (see notes in
 *          @ref bt_bap_ascs_rsp).
 */
#define BT_BAP_ASCS_RSP(c, r) (struct bt_bap_ascs_rsp) { .code = c, .reason = r }

/** @brief Abstract Audio Broadcast Source structure. */
struct bt_bap_broadcast_source;

/** @brief Abstract Audio Broadcast Sink structure. */
struct bt_bap_broadcast_sink;

/** @brief Abstract Audio Unicast Group structure. */
struct bt_bap_unicast_group;

/** @brief Abstract Audio Endpoint structure. */
struct bt_bap_ep;

/* TODO: Replace with struct bt_bap_base_subgroup */
/** Struct to hold subgroup specific information for the receive state */
struct bt_bap_scan_delegator_subgroup {
	/** BIS synced bitfield */
	uint32_t bis_sync;

	/** Length of the metadata */
	uint8_t metadata_len;

	/** The metadata */
	uint8_t metadata[BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN];
};

/** Represents the Broadcast Audio Scan Service receive state */
struct bt_bap_scan_delegator_recv_state {
	/** The source ID  */
	uint8_t src_id;

	/** The Bluetooth address */
	bt_addr_le_t addr;

	/** The advertising set ID*/
	uint8_t adv_sid;

	/** The periodic adverting sync state */
	enum bt_bap_pa_state pa_sync_state;

	/** The broadcast isochronous group encryption state */
	enum bt_bap_big_enc_state encrypt_state;

	/** The 24-bit broadcast ID */
	uint32_t broadcast_id;

	/** @brief The bad broadcast code
	 *
	 * Only valid if encrypt_state is @ref BT_BAP_BIG_ENC_STATE_BCODE_REQ
	 */
	uint8_t bad_code[BT_AUDIO_BROADCAST_CODE_SIZE];

	/** Number of subgroups */
	uint8_t num_subgroups;

	/** Subgroup specific information */
	struct bt_bap_scan_delegator_subgroup subgroups[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS];
};

struct bt_bap_scan_delegator_cb {
	/**
	 * @brief Receive state updated
	 *
	 * @param conn       Pointer to the connection to a remote device if
	 *                   the change was caused by it, otherwise NULL.
	 * @param recv_state Pointer to the receive state that was updated.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	void (*recv_state_updated)(struct bt_conn *conn,
				   const struct bt_bap_scan_delegator_recv_state *recv_state);

	/**
	 * @brief Periodic advertising sync request
	 *
	 * Request from peer device to synchronize with the periodic advertiser
	 * denoted by the @p recv_state. To notify the Broadcast Assistant about
	 * any pending sync
	 *
	 * @param[in]  conn        Pointer to the connection requesting the
	 *                         periodic advertising sync.
	 * @param[in]  recv_state  Pointer to the receive state that is being
	 *                         requested for periodic advertising sync.
	 * @param[in]  past_avail  True if periodic advertising sync transfer is
	 *                         available.
	 * @param[in]  pa_interval The periodic advertising interval.
	 * @param[out] past_sync   Set to true if syncing via periodic
	 *                         advertising sync transfer, false otherwise.
	 *                         If @p past_avail is false, this value is
	 *                         ignored.
	 *
	 * @return 0 in case of accept, or other value to reject.
	 */
	int (*pa_sync_req)(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *recv_state,
			   bool past_avail, uint16_t pa_interval);

	/**
	 * @brief Periodic advertising sync termination request
	 *
	 * Request from peer device to terminate the periodic advertiser sync
	 * denoted by the @p recv_state.
	 *
	 * @param conn        Pointer to the connection requesting the periodic
	 *                    advertising sync termination.
	 * @param recv_state  Pointer to the receive state that is being
	 *                    requested for periodic advertising sync.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*pa_sync_term_req)(struct bt_conn *conn,
				const struct bt_bap_scan_delegator_recv_state *recv_state);

	/**
	 * @brief Broadcast code received
	 *
	 * Broadcast code received from a broadcast assistant
	 *
	 * @param conn           Pointer to the connection providing the
	 *                       broadcast code.
	 * @param recv_state     Pointer to the receive state the broadcast code
	 *                       is being provided for.
	 * @param broadcast_code The 16-octet broadcast code
	 */
	void (*broadcast_code)(struct bt_conn *conn,
			       const struct bt_bap_scan_delegator_recv_state *recv_state,
			       const uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE]);

	/**
	 * @brief Broadcast Isochronous Stream synchronize request
	 *
	 * Request from Broadcast Assistant device to modify the Broadcast
	 * Isochronous Stream states. The request shall be fulfilled with
	 * accordance to the @p bis_sync_req within reasonable time. The
	 * Broadcast Assistant may also request fewer, or none, indexes to
	 * be synchronized.
	 *
	 * @param[in]  conn          Pointer to the connection of the
	 *                           Broadcast Assistant requesting the sync.
	 * @param[in]  recv_state    Pointer to the receive state that is being
	 *                           requested for the sync.
	 * @param[in]  bis_sync_req  Array of bitfields of which BIS indexes
	 *                           that is requested to sync for each subgroup
	 *                           by the Broadcast Assistant.
	 *
	 * @return 0 in case of accept, or other value to reject.
	 */
	int (*bis_sync_req)(struct bt_conn *conn,
			    const struct bt_bap_scan_delegator_recv_state *recv_state,
			    const uint32_t bis_sync_req[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS]);
};

/** Structure holding information of audio stream endpoint */
struct bt_bap_ep_info {
	/** The ID of the endpoint */
	uint8_t id;

	/** The state of the endpoint */
	enum bt_bap_ep_state state;

	/** Capabilities type */
	enum bt_audio_dir dir;
};

/**
 * @brief Return structure holding information of audio stream endpoint
 *
 * @param ep   The audio stream endpoint object.
 * @param info The structure object to be filled with the info.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_ep_get_info(const struct bt_bap_ep *ep, struct bt_bap_ep_info *info);

/**
 * @brief Basic Audio Profile stream structure.
 *
 * Streams represents a stream configuration of a Remote Endpoint and a Local Capability.
 *
 * @note Streams are unidirectional but can be paired with other streams to use a bidirectional
 * connected isochronous stream.
 */
struct bt_bap_stream {
	/** Stream direction */
	enum bt_audio_dir dir;

	/** Connection reference */
	struct bt_conn *conn;

	/** Endpoint reference */
	struct bt_bap_ep *ep;

	/** Codec Configuration */
	struct bt_codec *codec;

	/** QoS Configuration */
	struct bt_codec_qos *qos;

	/** Audio stream operations */
	struct bt_bap_stream_ops *ops;

#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
	/**
	 * @brief Audio ISO reference
	 *
	 * This is only used for Unicast Client streams, and is handled internally.
	 */
	struct bt_bap_iso *bap_iso;
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

	/** Unicast or Broadcast group - Used internally */
	void *group;

	/** Stream user data */
	void *user_data;

	/* Internally used list node */
	sys_snode_t _node;
};

/** @brief Stream operation. */
struct bt_bap_stream_ops {
#if defined(CONFIG_BT_BAP_UNICAST)
	/**
	 * @brief Stream configured callback
	 *
	 * Configured callback is called whenever an Audio Stream has been configured.
	 *
	 * @param stream Stream object that has been configured.
	 * @param pref   Remote QoS preferences.
	 */
	void (*configured)(struct bt_bap_stream *stream, const struct bt_codec_qos_pref *pref);

	/**
	 * @brief Stream QoS set callback
	 *
	 * QoS set callback is called whenever an Audio Stream Quality of Service has been set or
	 * updated.
	 *
	 * @param stream Stream object that had its QoS updated.
	 */
	void (*qos_set)(struct bt_bap_stream *stream);

	/**
	 * @brief Stream enabled callback
	 *
	 * Enabled callback is called whenever an Audio Stream has been enabled.
	 *
	 * @param stream Stream object that has been enabled.
	 */
	void (*enabled)(struct bt_bap_stream *stream);

	/**
	 * @brief Stream metadata updated callback
	 *
	 * Metadata Updated callback is called whenever an Audio Stream's metadata has been
	 * updated.
	 *
	 *  @param stream Stream object that had its metadata updated.
	 */
	void (*metadata_updated)(struct bt_bap_stream *stream);

	/**
	 * @brief Stream disabled callback
	 *
	 * Disabled callback is called whenever an Audio Stream has been disabled.
	 *
	 *  @param stream Stream object that has been disabled.
	 */
	void (*disabled)(struct bt_bap_stream *stream);

	/**
	 * @brief Stream released callback
	 *
	 * Released callback is called whenever a Audio Stream has been released and can be
	 * deallocated.
	 *
	 * @param stream Stream object that has been released.
	 */
	void (*released)(struct bt_bap_stream *stream);
#endif /* CONFIG_BT_BAP_UNICAST */

	/**
	 * @brief Stream started callback
	 *
	 * Started callback is called whenever an Audio Stream has been started
	 * and will be usable for streaming.
	 *
	 * @param stream Stream object that has been started.
	 */
	void (*started)(struct bt_bap_stream *stream);

	/**
	 * @brief Stream stopped callback
	 *
	 * Stopped callback is called whenever an Audio Stream has been stopped.
	 *
	 * @param stream Stream object that has been stopped.
	 * @param reason BT_HCI_ERR_* reason for the disconnection.
	 */
	void (*stopped)(struct bt_bap_stream *stream, uint8_t reason);

#if defined(CONFIG_BT_AUDIO_RX)
	/**
	 * @brief Stream audio HCI receive callback.
	 *
	 * This callback is only used if the ISO data path is HCI.
	 *
	 * @param stream Stream object.
	 * @param info   Pointer to the metadata for the buffer. The lifetime of the pointer is
	 *               linked to the lifetime of the net_buf. Metadata such as sequence number and
	 *               timestamp can be provided by the bluetooth controller.
	 * @param buf    Buffer containing incoming audio data.
	 */
	void (*recv)(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
		     struct net_buf *buf);
#endif /* CONFIG_BT_AUDIO_RX */

#if defined(CONFIG_BT_AUDIO_TX)
	/**
	 * @brief Stream audio HCI sent callback
	 *
	 * If this callback is provided it will be called whenever a SDU has been completely sent,
	 * or otherwise flushed due to transmission issues.
	 *
	 * This callback is only used if the ISO data path is HCI.
	 *
	 * @param chan The channel which has sent data.
	 */
	void (*sent)(struct bt_bap_stream *stream);
#endif /* CONFIG_BT_AUDIO_TX */
};

/**
 * @brief Register Audio callbacks for a stream.
 *
 * Register Audio callbacks for a stream.
 *
 * @param stream Stream object.
 * @param ops    Stream operations structure.
 */
void bt_bap_stream_cb_register(struct bt_bap_stream *stream, struct bt_bap_stream_ops *ops);

/**
 * @brief Configure Audio Stream
 *
 * This procedure is used by a client to configure a new stream using the
 * remote endpoint, local capability and codec configuration.
 *
 * @param conn Connection object
 * @param stream Stream object being configured
 * @param ep Remote Audio Endpoint being configured
 * @param codec Codec configuration
 *
 * @return Allocated Audio Stream object or NULL in case of error.
 */
int bt_bap_stream_config(struct bt_conn *conn, struct bt_bap_stream *stream, struct bt_bap_ep *ep,
			 struct bt_codec *codec);

/**
 * @brief Reconfigure Audio Stream
 *
 * This procedure is used by a unicast client or unicast server to reconfigure
 * a stream to use a different local codec configuration.
 *
 * This can only be done for unicast streams.
 *
 * @param stream Stream object being reconfigured
 * @param codec Codec configuration
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_reconfig(struct bt_bap_stream *stream, struct bt_codec *codec);

/**
 * @brief Configure Audio Stream QoS
 *
 * This procedure is used by a client to configure the Quality of Service of streams in a unicast
 * group. All streams in the group for the specified @p conn will have the Quality of Service
 * configured. This shall only be used to configure unicast streams.
 *
 * @param conn  Connection object
 * @param group Unicast group object
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_qos(struct bt_conn *conn, struct bt_bap_unicast_group *group);

/**
 * @brief Enable Audio Stream
 *
 * This procedure is used by a client to enable a stream.
 *
 * This shall only be called for unicast streams, as broadcast streams will always be enabled once
 * created.
 *
 * @param stream Stream object
 * @param meta_count Number of metadata entries
 * @param meta Metadata entries
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_enable(struct bt_bap_stream *stream, struct bt_codec_data *meta,
			 size_t meta_count);

/**
 * @brief Change Audio Stream Metadata
 *
 * This procedure is used by a unicast client or unicast server to change the metadata of a stream.
 *
 * @param stream Stream object
 * @param meta_count Number of metadata entries
 * @param meta Metadata entries
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_metadata(struct bt_bap_stream *stream, struct bt_codec_data *meta,
			   size_t meta_count);

/**
 * @brief Disable Audio Stream
 *
 * This procedure is used by a unicast client or unicast server to disable a stream.
 *
 * This shall only be called for unicast streams, as broadcast streams will
 * always be enabled once created.
 *
 * @param stream Stream object
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_disable(struct bt_bap_stream *stream);

/**
 * @brief Start Audio Stream
 *
 * This procedure is used by a unicast client or unicast server to make a stream start streaming.
 *
 * For the unicast client, this will connect the CIS for the stream before
 * sending the start command.
 *
 * For the unicast server, this will put a @ref BT_AUDIO_DIR_SINK stream into the streaming state if
 * the CIS is connected (initialized by the unicast client). If the CIS is not connected yet, the
 * stream will go into the streaming state as soon as the CIS is connected.
 * @ref BT_AUDIO_DIR_SOURCE streams will go into the streaming state when the unicast client sends
 * the Receiver Start Ready operation, which will trigger the @ref bt_bap_unicast_server_cb.start()
 * callback.
 *
 * This shall only be called for unicast streams.
 *
 * Broadcast sinks will always be started once synchronized, and broadcast
 * source streams shall be started with bt_bap_broadcast_source_start().
 *
 * @param stream Stream object
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_start(struct bt_bap_stream *stream);

/**
 * @brief Stop Audio Stream
 *
 * This procedure is used by a client to make a stream stop streaming.
 *
 * This shall only be called for unicast streams.
 * Broadcast sinks cannot be stopped.
 * Broadcast sources shall be stopped with bt_bap_broadcast_source_stop().
 *
 * @param stream Stream object
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_stop(struct bt_bap_stream *stream);

/**
 * @brief Release Audio Stream
 *
 * This procedure is used by a unicast client or unicast server to release a unicast stream.
 *
 * Broadcast sink streams cannot be released, but can be deleted by bt_bap_broadcast_sink_delete().
 * Broadcast source streams cannot be released, but can be deleted by
 * bt_bap_broadcast_source_delete().
 *
 * @param stream Stream object
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_stream_release(struct bt_bap_stream *stream);

/**
 * @brief Send data to Audio stream
 *
 * Send data from buffer to the stream.
 *
 * @note Data will not be sent to linked streams since linking is only
 * consider for procedures affecting the state machine.
 *
 * @param stream   Stream object.
 * @param buf      Buffer containing data to be sent.
 * @param seq_num  Packet Sequence number. This value shall be incremented for each call to this
 *                 function and at least once per SDU interval for a specific channel.
 * @param ts       Timestamp of the SDU in microseconds (us). This value can be used to transmit
 *                 multiple SDUs in the same SDU interval in a CIG or BIG. Can be omitted by using
 *                 @ref BT_ISO_TIMESTAMP_NONE which will simply enqueue the ISO SDU in a FIFO
 *                 manner.
 *
 * @return Bytes sent in case of success or negative value in case of error.
 */
int bt_bap_stream_send(struct bt_bap_stream *stream, struct net_buf *buf, uint16_t seq_num,
		       uint32_t ts);

/**
 * @defgroup bt_bap_unicast_server BAP Unicast Server APIs
 * @ingroup bt_bap
 * @{
 */

/** Unicast Server callback structure */
struct bt_bap_unicast_server_cb {
	/**
	 * @brief Endpoint config request callback
	 *
	 * Config callback is called whenever an endpoint is requested to be
	 * configured
	 *
	 * @param[in]  conn    Connection object.
	 * @param[in]  ep      Local Audio Endpoint being configured.
	 * @param[in]  dir     Direction of the endpoint.
	 * @param[in]  codec   Codec configuration.
	 * @param[out] stream  Pointer to stream that will be configured for the endpoint.
	 * @param[out] pref    Pointer to a QoS preference object that shall be populated with
	 *                     values. Invalid values will reject the codec configuration request.
	 * @param[out] rsp     Object for the ASE operation response. Only used if the return
	 *                     value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*config)(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
		      const struct bt_codec *codec, struct bt_bap_stream **stream,
		      struct bt_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream reconfig request callback
	 *
	 * Reconfig callback is called whenever an Audio Stream needs to be
	 * reconfigured with different codec configuration.
	 *
	 * @param[in]  stream  Stream object being reconfigured.
	 * @param[in]  dir     Direction of the endpoint.
	 * @param[in]  codec   Codec configuration.
	 * @param[out] pref    Pointer to a QoS preference object that shall be populated with
	 *                     values. Invalid values will reject the codec configuration request.
	 * @param[out] rsp     Object for the ASE operation response. Only used if the return
	 *                     value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*reconfig)(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			const struct bt_codec *codec, struct bt_codec_qos_pref *const pref,
			struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream QoS request callback
	 *
	 * QoS callback is called whenever an Audio Stream Quality of
	 * Service needs to be configured.
	 *
	 * @param[in]  stream  Stream object being reconfigured.
	 * @param[in]  qos     Quality of Service configuration.
	 * @param[out] rsp     Object for the ASE operation response. Only used if the return
	 *                     value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*qos)(struct bt_bap_stream *stream, const struct bt_codec_qos *qos,
		   struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream Enable request callback
	 *
	 * Enable callback is called whenever an Audio Stream is requested to be enabled to stream.
	 *
	 * @param[in]  stream      Stream object being enabled.
	 * @param[in]  meta        Metadata entries
	 * @param[in]  meta_count  Number of metadata entries
	 * @param[out] rsp         Object for the ASE operation response. Only used if the return
	 *                         value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*enable)(struct bt_bap_stream *stream, const struct bt_codec_data *meta,
		      size_t meta_count, struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream Start request callback
	 *
	 * Start callback is called whenever an Audio Stream is requested to start streaming.
	 *
	 * @param[in]  stream Stream object.
	 * @param[out] rsp    Object for the ASE operation response. Only used if the return
	 *                    value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*start)(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream Metadata update request callback
	 *
	 * Metadata callback is called whenever an Audio Stream is requested to update its metadata.
	 *
	 * @param[in]  stream       Stream object.
	 * @param[in]  meta         Metadata entries
	 * @param[in]  meta_count   Number of metadata entries
	 * @param[out] rsp          Object for the ASE operation response. Only used if the return
	 *                          value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*metadata)(struct bt_bap_stream *stream, const struct bt_codec_data *meta,
			size_t meta_count, struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream Disable request callback
	 *
	 * Disable callback is called whenever an Audio Stream is requested to disable the stream.
	 *
	 * @param[in]  stream Stream object being disabled.
	 * @param[out] rsp    Object for the ASE operation response. Only used if the return
	 *                    value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*disable)(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream Stop callback
	 *
	 * Stop callback is called whenever an Audio Stream is requested to stop streaming.
	 *
	 * @param[in]  stream Stream object.
	 * @param[out] rsp    Object for the ASE operation response. Only used if the return
	 *                    value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*stop)(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp);

	/**
	 * @brief Stream release callback
	 *
	 * Release callback is called whenever a new Audio Stream needs to be released and thus
	 * deallocated.
	 *
	 * @param[in]  stream Stream object.
	 * @param[out] rsp    Object for the ASE operation response. Only used if the return
	 *                    value is non-zero.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*release)(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp);
};

/**
 * @brief Register unicast server callbacks.
 *
 * Only one callback structure can be registered, and attempting to
 * registering more than one will result in an error.
 *
 * @param cb  Unicast server callback structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_unicast_server_register_cb(const struct bt_bap_unicast_server_cb *cb);

/**
 * @brief Unregister unicast server callbacks.
 *
 * May only unregister a callback structure that has previously been
 * registered by bt_bap_unicast_server_register_cb().
 *
 * @param cb  Unicast server callback structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_unicast_server_unregister_cb(const struct bt_bap_unicast_server_cb *cb);

/**
 * @typedef bt_bap_ep_func_t
 * @brief The callback function called for each endpoint.
 *
 * @param ep The structure object with endpoint info.
 * @param user_data Data to pass to the function.
 */
typedef void (*bt_bap_ep_func_t)(struct bt_bap_ep *ep, void *user_data);

/**
 * @brief Iterate through all endpoints of the given connection.
 *
 * @param conn Connection object
 * @param func Function to call for each endpoint.
 * @param user_data Data to pass to the callback function.
 */
void bt_bap_unicast_server_foreach_ep(struct bt_conn *conn, bt_bap_ep_func_t func, void *user_data);

/**
 * @brief Initialize and configure a new ASE.
 *
 * @param conn Connection object
 * @param stream Configured stream object to be attached to the ASE
 * @param codec Codec configuration
 * @param qos_pref Audio Stream Quality of Service Preference
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_unicast_server_config_ase(struct bt_conn *conn, struct bt_bap_stream *stream,
				     struct bt_codec *codec,
				     const struct bt_codec_qos_pref *qos_pref);

/** @} */ /* End of group bt_bap_unicast_server */

/**
 * @defgroup bt_bap_unicast_client BAP Unicast Client APIs
 * @ingroup bt_bap
 * @{
 */

/** Parameter struct for each stream in the unicast group */
struct bt_bap_unicast_group_stream_param {
	/** Pointer to a stream object. */
	struct bt_bap_stream *stream;

	/** The QoS settings for the stream object. */
	struct bt_codec_qos *qos;
};

/** @brief Parameter struct for the unicast group functions
 *
 * Parameter struct for the bt_bap_unicast_group_create() and
 * bt_bap_unicast_group_add_streams() functions.
 */
struct bt_bap_unicast_group_stream_pair_param {
	/** Pointer to a receiving stream parameters. */
	struct bt_bap_unicast_group_stream_param *rx_param;

	/** Pointer to a transmiting stream parameters. */
	struct bt_bap_unicast_group_stream_param *tx_param;
};

struct bt_bap_unicast_group_param {
	/** The number of parameters in @p params */
	size_t params_count;

	/** Array of stream parameters */
	struct bt_bap_unicast_group_stream_pair_param *params;

	/** @brief Unicast Group packing mode.
	 *
	 *  @ref BT_ISO_PACKING_SEQUENTIAL or @ref BT_ISO_PACKING_INTERLEAVED.
	 *
	 *  @note This is a recommendation to the controller, which the controller may ignore.
	 */
	uint8_t packing;
};

/**
 * @brief Create audio unicast group.
 *
 * Create a new audio unicast group with one or more audio streams as a unicast client. Streams in
 * a unicast group shall share the same interval, framing and latency (see @ref bt_codec_qos).
 *
 * @param[in]  param          The unicast group create parameters.
 * @param[out] unicast_group  Pointer to the unicast group created.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_unicast_group_create(struct bt_bap_unicast_group_param *param,
				struct bt_bap_unicast_group **unicast_group);

/**
 * @brief Add streams to a unicast group as a unicast client
 *
 * This function can be used to add additional streams to a  bt_bap_unicast_group.
 *
 * This can be called at any time before any of the streams in the group has been started
 * (see bt_bap_stream_ops.started()).
 * This can also be called after the streams have been stopped (see bt_bap_stream_ops.stopped()).
 *
 * Once a stream has been added to a unicast group, it cannot be removed. To remove a stream from a
 * group, the group must be deleted with bt_bap_unicast_group_delete(), but this will require all
 * streams in the group to be released first.
 *
 * @param unicast_group  Pointer to the unicast group
 * @param params         Array of stream parameters with streams being added to the group.
 * @param num_param      Number of paramers in @p params.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_unicast_group_add_streams(struct bt_bap_unicast_group *unicast_group,
				     struct bt_bap_unicast_group_stream_pair_param params[],
				     size_t num_param);

/**
 * @brief Delete audio unicast group.
 *
 * Delete a audio unicast group as a client. All streams in the group shall
 * be in the idle or configured state.
 *
 * @param unicast_group  Pointer to the unicast group to delete
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_unicast_group_delete(struct bt_bap_unicast_group *unicast_group);

/** Unicast Client callback structure */
struct bt_bap_unicast_client_cb {
	/**
	 * @brief Remote Unicast Server Audio Locations
	 *
	 * This callback is called whenever the audio locations is read from
	 * the server or otherwise notified to the client.
	 *
	 * @param conn  Connection to the remote unicast server.
	 * @param dir   Direction of the location.
	 * @param loc   The location bitfield value.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	void (*location)(struct bt_conn *conn, enum bt_audio_dir dir, enum bt_audio_location loc);

	/**
	 * @brief Remote Unicast Server Available Contexts
	 *
	 * This callback is called whenever the available contexts are read
	 * from the server or otherwise notified to the client.
	 *
	 * @param conn     Connection to the remote unicast server.
	 * @param snk_ctx  The sink context bitfield value.
	 * @param src_ctx  The source context bitfield value.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	void (*available_contexts)(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				   enum bt_audio_context src_ctx);

	/**
	 * @brief Callback function for bt_bap_stream_config() and bt_bap_stream_reconfig().
	 *
	 * Called when the codec configure operation is completed on the server.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*config)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason);

	/**
	 * @brief Callback function for bt_bap_stream_qos().
	 *
	 * Called when the QoS configure operation is completed on the server.
	 * This will be called for each stream in the group that was being QoS
	 * configured.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*qos)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		    enum bt_bap_ascs_reason reason);

	/**
	 * @brief Callback function for bt_bap_stream_enable().
	 *
	 * Called when the enable operation is completed on the server.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*enable)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		       enum bt_bap_ascs_reason reason);

	/**
	 * @brief Callback function for bt_bap_stream_start().
	 *
	 * Called when the start operation is completed on the server. This will
	 * only be called if the stream supplied to bt_bap_stream_start() is
	 * for a @ref BT_AUDIO_DIR_SOURCE endpoint.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*start)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		      enum bt_bap_ascs_reason reason);

	/**
	 * @brief Callback function for bt_bap_stream_stop().
	 *
	 * Called when the stop operation is completed on the server. This will
	 * only be called if the stream supplied to bt_bap_stream_stop() is
	 * for a @ref BT_AUDIO_DIR_SOURCE endpoint.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*stop)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
		     enum bt_bap_ascs_reason reason);

	/**
	 * @brief Callback function for bt_bap_stream_disable().
	 *
	 * Called when the disable operation is completed on the server.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*disable)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
			enum bt_bap_ascs_reason reason);

	/**
	 * @brief Callback function for bt_bap_stream_metadata().
	 *
	 * Called when the metadata operation is completed on the server.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*metadata)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
			 enum bt_bap_ascs_reason reason);

	/**
	 * @brief Callback function for bt_bap_stream_release().
	 *
	 * Called when the release operation is completed on the server.
	 *
	 * @param stream   Stream the operation was performed on.
	 * @param rsp_code Response code.
	 * @param reason   Reason code.
	 */
	void (*release)(struct bt_bap_stream *stream, enum bt_bap_ascs_rsp_code rsp_code,
			enum bt_bap_ascs_reason reason);
};

/**
 * @brief Register unicast client callbacks.
 *
 * Only one callback structure can be registered, and attempting to
 * registering more than one will result in an error.
 *
 * @param cb  Unicast client callback structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_bap_unicast_client_register_cb(const struct bt_bap_unicast_client_cb *cb);

struct bt_bap_unicast_client_discover_params;

/**
 * @typedef bt_bap_unicast_client_discover_func_t
 * @brief Discover Audio capabilities and endpoints callback function.
 *
 * If discovery procedure has complete both cap and ep are set to NULL.
 *
 * The @p codec is only valid while in the callback, so the values must be stored by the receiver
 * if future use is wanted.
 *
 * @param conn     Connection to the remote unicast server.
 * @param codec    Remote capabilities.
 * @param ep       Remote endpoint.
 * @param params   Pointer to the discover parameters.
 *
 * If discovery procedure has complete both @p codec and @p ep are set to NULL.
 */
typedef void (*bt_bap_unicast_client_discover_func_t)(
	struct bt_conn *conn, struct bt_codec *codec, struct bt_bap_ep *ep,
	struct bt_bap_unicast_client_discover_params *params);

struct bt_bap_unicast_client_discover_params {
	/** Capabilities type */
	enum bt_audio_dir dir;

	/** Callback function */
	bt_bap_unicast_client_discover_func_t func;

	/** Number of capabilities found */
	uint8_t num_caps;

	/** Number of endpoints found */
	uint8_t num_eps;

	/** Error code. */
	uint8_t err;

	/** Read parameters used interally for discovery */
	struct bt_gatt_read_params read;

	/** Discover parameters used interally for discovery */
	struct bt_gatt_discover_params discover;
};

/**
 * @brief Discover remote capabilities and endpoints
 *
 * This procedure is used by a client to discover remote capabilities and
 * endpoints and notifies via params callback.
 *
 * @note This procedure is asynchronous therefore the parameters need to
 *       remains valid while it is active.
 *
 * @param conn   Connection object
 * @param params Discover parameters
 */
int bt_bap_unicast_client_discover(struct bt_conn *conn,
				   struct bt_bap_unicast_client_discover_params *params);

/** @} */ /* End of group bt_bap_unicast_client */
/**
 * @brief BAP Broadcast APIs
 * @defgroup bt_bap_broadcast BAP Broadcast  APIs
 * @ingroup bt_bap
 * @{
 */

struct bt_bap_base_bis_data {
	/* Unique index of the BIS */
	uint8_t index;
#if defined(CONFIG_BT_CODEC_MAX_DATA_COUNT)
	/** Codec Specific Data count.
	 *
	 *  Only valid if the data_count of struct bt_codec in the subgroup is 0
	 */
	size_t data_count;
	/** Codec Specific Data
	 *
	 *  Only valid if the data_count of struct bt_codec in the subgroup is 0
	 */
	struct bt_codec_data data[CONFIG_BT_CODEC_MAX_DATA_COUNT];
#endif /* CONFIG_BT_CODEC_MAX_DATA_COUNT */
};

struct bt_bap_base_subgroup {
	/* Number of BIS in the subgroup */
	size_t bis_count;
	/** Codec information for the subgroup
	 *
	 *  If the data_count of the codec is 0, then codec specific data may be
	 *  found for each BIS in the bis_data.
	 */
	struct bt_codec codec;
	/* Array of BIS specific data for each BIS in the subgroup */
	struct bt_bap_base_bis_data bis_data[BROADCAST_SNK_STREAM_CNT];
};

struct bt_bap_base {
	/** @brief QoS Presentation Delay in microseconds
	 *
	 *  Value range 0 to @ref BT_AUDIO_PD_MAX.
	 */
	uint32_t pd;

	/* Number of subgroups in the BASE */
	size_t subgroup_count;

	/* Array of subgroups in the BASE */
	struct bt_bap_base_subgroup subgroups[BROADCAST_SNK_SUBGROUP_CNT];
};

/** @} */ /* End of group bt_bap_broadcast */

/**
 * @brief BAP Broadcast Source APIs
 * @defgroup bt_bap_broadcast_source BAP Broadcast Source APIs
 * @ingroup bt_bap_broadcast
 * @{
 */

/** Broadcast Source stream parameters */
struct bt_bap_broadcast_source_stream_param {
	/** Audio stream */
	struct bt_bap_stream *stream;

	/**
	 * @brief The number of elements in the @p data array.
	 *
	 * The BIS specific data may be omitted and this set to 0.
	 */
	size_t data_count;

	/** BIS Codec Specific Configuration */
	struct bt_codec_data *data;
};

/** Broadcast Source subgroup parameters*/
struct bt_bap_broadcast_source_subgroup_param {
	/** The number of parameters in @p stream_params */
	size_t params_count;

	/** Array of stream parameters */
	struct bt_bap_broadcast_source_stream_param *params;

	/** Subgroup Codec configuration. */
	struct bt_codec *codec;
};

/** Broadcast Source create parameters */
struct bt_bap_broadcast_source_create_param {
	/** The number of parameters in @p subgroup_params */
	size_t params_count;

	/** Array of stream parameters */
	struct bt_bap_broadcast_source_subgroup_param *params;

	/** Quality of Service configuration. */
	struct bt_codec_qos *qos;

	/**
	 * @brief Broadcast Source packing mode.
	 *
	 * @ref BT_ISO_PACKING_SEQUENTIAL or @ref BT_ISO_PACKING_INTERLEAVED.
	 *
	 * @note This is a recommendation to the controller, which the controller may ignore.
	 */
	uint8_t packing;

	/** Whether or not to encrypt the streams. */
	bool encryption;

	/**
	 * @brief Broadcast code
	 *
	 * If the value is a string or a the value is less than 16 octets,
	 * the remaining octets shall be 0.
	 *
	 * Example:
	 *   The string "Broadcast Code" shall be
	 *   [42 72 6F 61 64 63 61 73 74 20 43 6F 64 65 00 00]
	 */
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
};

/**
 * @brief Create audio broadcast source.
 *
 * Create a new audio broadcast source with one or more audio streams.
 *
 * The broadcast source will be visible for scanners once this has been called,
 * and the device will advertise audio announcements.
 *
 * No audio data can be sent until bt_bap_broadcast_source_start() has been called and no audio
 * information (BIGInfo) will be visible to scanners (see @ref bt_le_per_adv_sync_cb).
 *
 * @param[in]  param       Pointer to parameters used to create the broadcast source.
 * @param[out] source      Pointer to the broadcast source created
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_create(struct bt_bap_broadcast_source_create_param *param,
				   struct bt_bap_broadcast_source **source);

/**
 * @brief Reconfigure audio broadcast source.
 *
 * Reconfigure an audio broadcast source with a new codec and codec quality of
 * service parameters. This can only be done when the source is stopped.
 *
 * @param source      Pointer to the broadcast source
 * @param codec       Codec configuration.
 * @param qos         Quality of Service configuration
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_reconfig(struct bt_bap_broadcast_source *source, struct bt_codec *codec,
				     struct bt_codec_qos *qos);

/**
 * @brief Modify the metadata of an audio broadcast source.
 *
 * Modify the metadata an audio broadcast source. This can only be done when the source is started.
 * To update the metadata in the stopped state, use bt_bap_broadcast_source_reconfig().
 *
 * @param source      Pointer to the broadcast source.
 * @param meta        Metadata entries.
 * @param meta_count  Number of metadata entries.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_update_metadata(struct bt_bap_broadcast_source *source,
					    const struct bt_codec_data meta[], size_t meta_count);

/**
 * @brief Start audio broadcast source.
 *
 * Start an audio broadcast source with one or more audio streams.
 * The broadcast source will start advertising BIGInfo, and audio data can be streamed.
 *
 * @param source      Pointer to the broadcast source
 * @param adv         Pointer to an extended advertising set with periodic advertising configured.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_start(struct bt_bap_broadcast_source *source,
				  struct bt_le_ext_adv *adv);

/**
 * @brief Stop audio broadcast source.
 *
 * Stop an audio broadcast source.
 * The broadcast source will stop advertising BIGInfo, and audio data can no longer be streamed.
 *
 * @param source      Pointer to the broadcast source
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_stop(struct bt_bap_broadcast_source *source);

/**
 * @brief Delete audio broadcast source.
 *
 * Delete an audio broadcast source.
 * The broadcast source will stop advertising entirely, and the source can no longer be used.
 *
 * @param source      Pointer to the broadcast source
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_delete(struct bt_bap_broadcast_source *source);

/**
 * @brief Get the broadcast ID of a broadcast source
 *
 * This will return the 3-octet broadcast ID that should be advertised in the
 * extended advertising data with @ref BT_UUID_BROADCAST_AUDIO_VAL as @ref BT_DATA_SVC_DATA16.
 *
 * See table 3.14 in the Basic Audio Profile v1.0.1 for the structure.
 *
 * @param[in]  source        Pointer to the broadcast source.
 * @param[out] broadcast_id  Pointer to the 3-octet broadcast ID.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_get_id(const struct bt_bap_broadcast_source *source,
				   uint32_t *const broadcast_id);

/**
 * @brief Get the Broadcast Audio Stream Endpoint of a broadcast source
 *
 * This will encode the BASE of a broadcast source into a buffer, that can be used for
 * advertisement. The encoded BASE will thus be encoded as little-endian. The BASE shall be put into
 * the periodic advertising data (see bt_le_per_adv_set_data()).
 *
 * See table 3.15 in the Basic Audio Profile v1.0.1 for the structure.
 *
 * @param source        Pointer to the broadcast source.
 * @param base_buf      Pointer to a buffer where the BASE will be inserted.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_source_get_base(struct bt_bap_broadcast_source *source,
				     struct net_buf_simple *base_buf);

/** @} */ /* End of bt_bap_broadcast_source */

/**
 * @brief BAP Broadcast Sink APIs
 * @defgroup bt_bap_broadcast_sink BAP Broadcast Sink APIs
 * @ingroup bt_bap_broadcast
 * @{
 */

/** Broadcast Audio Sink callback structure */
struct bt_bap_broadcast_sink_cb {
	/** @brief Scan receive callback
	 *
	 *  Scan receive callback is called whenever a broadcast source has been found.
	 *
	 *  @param info          Advertiser packet information.
	 *  @param ad            Buffer containing advertiser data.
	 *  @param broadcast_id  24-bit broadcast ID
	 *
	 *  @return true to sync to the broadcaster, else false.
	 *          Syncing to the broadcaster will stop the current scan.
	 */
	bool (*scan_recv)(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad,
			  uint32_t broadcast_id);

	/** @brief Periodic advertising sync callback
	 *
	 *  Called when synchronized to a periodic advertising. When synchronized a
	 *  bt_bap_broadcast_sink structure is allocated for future use.
	 *
	 *  @param sink          Pointer to the allocated sink structure.
	 *  @param sync          Pointer to the periodic advertising sync.
	 *  @param broadcast_id  24-bit broadcast ID previously reported by scan_recv.
	 */
	void (*pa_synced)(struct bt_bap_broadcast_sink *sink, struct bt_le_per_adv_sync *sync,
			  uint32_t broadcast_id);

	/** @brief Broadcast Audio Source Endpoint (BASE) received
	 *
	 *  Callback for when we receive a BASE from a broadcaster after
	 *  syncing to the broadcaster's periodic advertising.
	 *
	 *  @param sink          Pointer to the sink structure.
	 *  @param base          Broadcast Audio Source Endpoint (BASE).
	 */
	void (*base_recv)(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base);

	/** @brief Broadcast sink is syncable
	 *
	 *  Called whenever a broadcast sink is not synchronized to audio, but the audio is
	 *  synchronizable. This is inferred when a BIGInfo report is received.
	 *
	 *  Once this callback has been called, it is possible to call
	 *  bt_bap_broadcast_sink_sync() to synchronize to the audio stream(s).
	 *
	 *  @param sink          Pointer to the sink structure.
	 *  @param encrypted     Whether or not the broadcast is encrypted
	 */
	void (*syncable)(struct bt_bap_broadcast_sink *sink, bool encrypted);

	/** @brief Scan terminated callback
	 *
	 *  Scan terminated callback is called whenever a scan started by
	 *  bt_bap_broadcast_sink_scan_start() is terminated before
	 *  bt_bap_broadcast_sink_scan_stop().
	 *
	 *  Typical reasons for this are that the periodic advertising has synchronized
	 *  (success criteria) or the scan timed out.  It may also be called if the periodic
	 *  advertising failed to synchronize.
	 *
	 *  @param err 0 in case of success or negative value in case of error.
	 */
	void (*scan_term)(int err);

	/** @brief Periodic advertising synchronization lost callback
	 *
	 *  The periodic advertising synchronization lost callback is called if the periodic
	 *  advertising sync is lost. If this happens, the sink object is deleted. To synchronize to
	 *  the broadcaster again, bt_bap_broadcast_sink_scan_start() must be called.
	 *
	 *  @param sink          Pointer to the sink structure.
	 */
	void (*pa_sync_lost)(struct bt_bap_broadcast_sink *sink);

	/* Internally used list node */
	sys_snode_t _node;
};

/** @brief Register Broadcast sink callbacks
 * *
 *  @param cb  Broadcast sink callback structure.
 */
void bt_bap_broadcast_sink_register_cb(struct bt_bap_broadcast_sink_cb *cb);

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
int bt_bap_broadcast_sink_scan_start(const struct bt_le_scan_param *param);

/**
 * @brief Stop scan for broadcast sources.
 *
 *  Stops ongoing scanning for broadcast sources.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_sink_scan_stop(void);

/** @brief Create a Broadcast Sink from a periodic advertising sync
 *
 *  This should only be done after verifying that the periodic advertising sync
 *  is from a Broadcast Source.
 *
 *  The created Broadcast Sink will need to be supplied to
 *  bt_bap_broadcast_sink_sync() in order to synchronize to the broadcast
 *  audio.
 *
 *  bt_bap_broadcast_sink_cb.pa_synced() will be called with the Broadcast
 *  Sink object created if this is successful.
 *
 *  @param  pa_sync       Pointer to the periodic advertising sync object.
 *  @param  broadcast_id  24-bit broadcast ID.
 *
 *  @return 0 in case of success or errno value in case of error.
 */
int bt_bap_broadcast_sink_create(struct bt_le_per_adv_sync *pa_sync, uint32_t broadcast_id);

/** @brief Sync to a broadcaster's audio
 *
 *  @param sink               Pointer to the sink object from the base_recv callback.
 *  @param indexes_bitfield   Bitfield of the BIS index to sync to. To sync to e.g. BIS index 1 and
 *                            2, this should have the value of BIT(1) | BIT(2).
 *  @param streams            Stream object pointers to be used for the receiver. If multiple BIS
 *                            indexes shall be synchronized, multiple streams shall be provided.
 *  @param broadcast_code     The 16-octet broadcast code. Shall be supplied if the broadcast is
 *                            encrypted (see @ref bt_bap_broadcast_sink_cb.syncable).
 *                            If the value is a string or a the value is less
 *                            than 16 octets, the remaining octets shall be 0.
 *
 *                            Example:
 *                            The string "Broadcast Code" shall be
 *                            [42 72 6F 61 64 63 61 73 74 20 43 6F 64 65 00 00]
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_bap_broadcast_sink_sync(struct bt_bap_broadcast_sink *sink, uint32_t indexes_bitfield,
			       struct bt_bap_stream *streams[], const uint8_t broadcast_code[16]);

/** @brief Stop audio broadcast sink.
 *
 *  Stop an audio broadcast sink.
 *  The broadcast sink will stop receiving BIGInfo, and audio data can no longer be streamed.
 *
 *  @param sink      Pointer to the broadcast sink
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int bt_bap_broadcast_sink_stop(struct bt_bap_broadcast_sink *sink);

/** @brief Release a broadcast sink
 *
 *  Once a broadcast sink has been allocated after the pa_synced callback, it can be deleted using
 *  this function. If the sink has synchronized to any broadcast audio streams, these must first be
 *  stopped using bt_bap_stream_stop.
 *
 *  @param sink Pointer to the sink object to delete.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_bap_broadcast_sink_delete(struct bt_bap_broadcast_sink *sink);

/** @} */ /* End of group bt_bap_broadcast_sink */

/**
 * @brief Register the callbacks for the Basic Audio Profile Scan Delegator
 *
 * @param cb Pointer to the callback struct
 */
void bt_bap_scan_delegator_register_cb(struct bt_bap_scan_delegator_cb *cb);

/**
 * @brief Set the periodic advertising sync state to syncing
 *
 * Set the periodic advertising sync state for a receive state to syncing,
 * notifying Broadcast Assistants.
 *
 * @param src_id    The source id used to identify the receive state.
 * @param pa_state  The Periodic Advertising sync state to set.
 *                  BT_BAP_PA_STATE_NOT_SYNCED and BT_BAP_PA_STATE_SYNCED is
 *                  not necessary to provide, as they are handled internally.
 *
 * @return int    Error value. 0 on success, errno on fail.
 */
int bt_bap_scan_delegator_set_pa_state(uint8_t src_id,
				       enum bt_bap_pa_state pa_state);

/**
 * @brief Set the sync state of a receive state in the server
 *
 * @param src_id         The source id used to identify the receive state.
 * @param bis_synced     Array of bitfields to set the BIS sync state for each
 *                       subgroup.
 * @return int           Error value. 0 on success, ERRNO on fail.
 */
int bt_bap_scan_delegator_set_bis_sync_state(
	uint8_t src_id,
	uint32_t bis_synced[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS]);

struct bt_bap_scan_delegator_add_src_param {
	/** The periodic adverting sync */
	struct bt_le_per_adv_sync *pa_sync;

	/** The broadcast isochronous group encryption state */
	enum bt_bap_big_enc_state encrypt_state;

	/** The 24-bit broadcast ID */
	uint32_t broadcast_id;

	/** Number of subgroups */
	uint8_t num_subgroups;

	/** Subgroup specific information */
	struct bt_bap_scan_delegator_subgroup subgroups[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS];
};

/**
 * @brief Add a receive state source locally
 *
 * This will notify any connected clients about the new source. This allows them
 * to modify and even remove it.
 *
 * If @kconfig{CONFIG_BT_BAP_BROADCAST_SINK} is enabled, any Broadcast Sink
 * sources are autonomously added.
 *
 * @param param The parameters for adding the new source
 *
 * @return int  errno on failure, or source ID on success.
 */
int bt_bap_scan_delegator_add_src(const struct bt_bap_scan_delegator_add_src_param *param);

struct bt_bap_scan_delegator_mod_src_param {
	/** The periodic adverting sync */
	uint8_t src_id;

	/** The broadcast isochronous group encryption state */
	enum bt_bap_big_enc_state encrypt_state;

	/** The 24-bit broadcast ID */
	uint32_t broadcast_id;

	/** Number of subgroups */
	uint8_t num_subgroups;

	/**
	 * @brief Subgroup specific information
	 *
	 * If a subgroup's metadata_len is set to 0, the existing metadata
	 * for the subgroup will remain unchanged
	 */
	struct bt_bap_scan_delegator_subgroup subgroups[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS];
};

/**
 * @brief Add a receive state source locally
 *
 * This will notify any connected clients about the new source. This allows them
 * to modify and even remove it.
 *
 * If @kconfig{CONFIG_BT_BAP_BROADCAST_SINK} is enabled, any Broadcast Sink
 * sources are autonomously modifed.
 *
 * @param param The parameters for adding the new source
 *
 * @return int  errno on failure, or source ID on success.
 */
int bt_bap_scan_delegator_mod_src(const struct bt_bap_scan_delegator_mod_src_param *param);

/**
 * @brief Remove a receive state source
 *
 * This will remove the receive state. If the receive state periodic advertising
 * is synced, bt_bap_scan_delegator_cb.pa_sync_term_req() will be called.
 *
 * If @kconfig{CONFIG_BT_BAP_BROADCAST_SINK} is enabled, any Broadcast Sink
 * sources are autonomously removed.
 *
 * @param src_id The source ID to remove
 *
 * @return int   Error value. 0 on success, errno on fail.
 */
int bt_bap_scan_delegator_rem_src(uint8_t src_id);

enum bt_bap_scan_delegator_iter {
	BT_BAP_SCAN_DELEGATOR_ITER_STOP = 0,
	BT_BAP_SCAN_DELEGATOR_ITER_CONTINUE,
};

/** Callback function for Scan Delegator receive state search functions
 *
 * @param recv_state The receive state.
 * @param user_data  User data.
 *
 * @return @ref BT_BAP_SCAN_DELEGATOR_ITER_STOP to stop iterating or
 *         @ref BT_BAP_SCAN_DELEGATOR_ITER_CONTINUE to continue.
 */
typedef enum bt_bap_scan_delegator_iter (*bt_bap_scan_delegator_state_func_t)(
	const struct bt_bap_scan_delegator_recv_state *recv_state, void *user_data);

/** @brief Iterate through all existing receive states
 *
 * @param func      The callback function
 * @param user_data User specified data that sent to the callback function
 */
void bt_bap_scan_delegator_foreach_state(bt_bap_scan_delegator_state_func_t func,
					 void *user_data);

/** @brief Find and return a receive state based on a compare function
 *
 * @param func      The compare callback function
 * @param user_data User specified data that sent to the callback function
 *
 * @return The first receive state where the @p func returned true, or NULL
 */
const struct bt_bap_scan_delegator_recv_state *bt_bap_scan_delegator_find_state(
	bt_bap_scan_delegator_state_func_t func, void *user_data);

/******************************** CLIENT API ********************************/

/**
 * @brief Callback function for when a receive state is read or updated
 *
 * Called whenever a receive state is read or updated.
 *
 * @param conn     The connection to the Broadcast Audio Scan Service server.
 * @param err      Error value. 0 on success, GATT error on fail.
 * @param state    The receive state.
 */
typedef void (*bt_bap_broadcast_assistant_recv_state_cb)(
	struct bt_conn *conn, int err,
	const struct bt_bap_scan_delegator_recv_state *state);

/**
 * @brief Callback function for when a receive state is removed.
 *
 * @param conn     The connection to the Broadcast Audio Scan Service server.
 * @param err      Error value. 0 on success, GATT error on fail.
 * @param src_id   The receive state.
 */
typedef void (*bt_bap_broadcast_assistant_recv_state_rem_cb)(struct bt_conn *conn,
							     int err,
							     uint8_t src_id);

/**
 * @brief Callback function for writes.
 *
 * @param conn    The connection to the peer device.
 * @param err     Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_bap_broadcast_assistant_write_cb)(struct bt_conn *conn,
						    int err);

struct bt_bap_broadcast_assistant_cb {
	/**
	 * @brief Callback function for bt_bap_broadcast_assistant_discover.
	 *
	 * @param conn              The connection that was used to discover
	 *                          Broadcast Audio Scan Service.
	 * @param err               Error value. 0 on success,
	 *                          GATT error or ERRNO on fail.
	 * @param recv_state_count  Number of receive states on the server.
	 */
	void (*discover)(struct bt_conn *conn, int err,
			 uint8_t recv_state_count);

	/**
	 * @brief Callback function for Broadcast Audio Scan Service client scan results
	 *
	 * Called when the scanner finds an advertiser that advertises the
	 * BT_UUID_BROADCAST_AUDIO UUID.
	 *
	 * @param info          Advertiser information.
	 * @param broadcast_id  24-bit broadcast ID.
	 */
	void (*scan)(const struct bt_le_scan_recv_info *info,
		     uint32_t broadcast_id);

	/**
	 * @brief Callback function for when a receive state is read or updated
	 *
	 * Called whenever a receive state is read or updated.
	 *
	 * @param conn     The connection to the Broadcast Audio Scan Service server.
	 * @param err      Error value. 0 on success, GATT error on fail.
	 * @param state    The receive state or NULL if the receive state is empty.
	 */
	void (*recv_state)(struct bt_conn *conn, int err,
			   const struct bt_bap_scan_delegator_recv_state *state);

	/**
	 * @brief Callback function for when a receive state is removed.
	 *
	 * @param conn     The connection to the Broadcast Audio Scan Service server.
	 * @param err      Error value. 0 on success, GATT error on fail.
	 * @param src_id   The receive state.
	 */
	void (*recv_state_removed)(struct bt_conn *conn, int err,
				   uint8_t src_id);

	/**
	 * @brief Callback function for bt_bap_broadcast_assistant_scan_start().
	 *
	 * @param conn    The connection to the peer device.
	 * @param err     Error value. 0 on success, GATT error on fail.
	 */
	void (*scan_start)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback function for bt_bap_broadcast_assistant_scan_stop().
	 *
	 * @param conn    The connection to the peer device.
	 * @param err     Error value. 0 on success, GATT error on fail.
	 */
	void (*scan_stop)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback function for bt_bap_broadcast_assistant_add_src().
	 *
	 * @param conn    The connection to the peer device.
	 * @param err     Error value. 0 on success, GATT error on fail.
	 */
	void (*add_src)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback function for bt_bap_broadcast_assistant_mod_src().
	 *
	 * @param conn    The connection to the peer device.
	 * @param err     Error value. 0 on success, GATT error on fail.
	 */
	void (*mod_src)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback function for bt_bap_broadcast_assistant_broadcast_code().
	 *
	 * @param conn    The connection to the peer device.
	 * @param err     Error value. 0 on success, GATT error on fail.
	 */
	void (*broadcast_code)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback function for bt_bap_broadcast_assistant_rem_src().
	 *
	 * @param conn    The connection to the peer device.
	 * @param err     Error value. 0 on success, GATT error on fail.
	 */
	void (*rem_src)(struct bt_conn *conn, int err);
};

/**
 * @brief Discover Broadcast Audio Scan Service on the server.
 *
 * Warning: Only one connection can be active at any time; discovering for a
 * new connection, will delete all previous data.
 *
 * @param conn  The connection
 * @return int  Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_discover(struct bt_conn *conn);

/**
 * @brief Scan start for BISes for a remote server.
 *
 * This will let the Broadcast Audio Scan Service server know that this device
 * is actively scanning for broadcast sources.
 * The function can optionally also start scanning, if the caller does not want
 * to start scanning itself.
 *
 * Scan results, if @p start_scan is true, is sent to the
 * bt_bap_broadcast_assistant_scan_cb callback.
 *
 * @param conn          Connection to the Broadcast Audio Scan Service server.
 *                      Used to let the server know that we are scanning.
 * @param start_scan    Start scanning if true. If false, the application should
 *                      enable scan itself.
 * @return int          Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_scan_start(struct bt_conn *conn,
					  bool start_scan);

/**
 * @brief Stop remote scanning for BISes for a server.
 *
 * @param conn   Connection to the server.
 * @return int   Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_scan_stop(struct bt_conn *conn);

/**
 * @brief Registers the callbacks used by Broadcast Audio Scan Service client.
 */
void bt_bap_broadcast_assistant_register_cb(struct bt_bap_broadcast_assistant_cb *cb);

/** Parameters for adding a source to a Broadcast Audio Scan Service server */
struct bt_bap_broadcast_assistant_add_src_param {
	/** Address of the advertiser. */
	bt_addr_le_t addr;

	/** SID of the advertising set. */
	uint8_t adv_sid;

	/** Whether to sync to periodic advertisements. */
	uint8_t pa_sync;

	/** 24-bit broadcast ID */
	uint32_t broadcast_id;

	/**
	 * @brief Periodic advertising interval in milliseconds.
	 *
	 * BT_BAP_PA_INTERVAL_UNKNOWN if unknown.
	 */
	uint16_t pa_interval;

	/** Number of subgroups */
	uint8_t num_subgroups;

	/** Pointer to array of subgroups */
	struct bt_bap_scan_delegator_subgroup *subgroups;
};

/**
 * @brief Add a source on the server.
 *
 * @param conn          Connection to the server.
 * @param param         Parameter struct.
 *
 * @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_add_src(struct bt_conn *conn,
				       struct bt_bap_broadcast_assistant_add_src_param *param);

/** Parameters for modifying a source */
struct bt_bap_broadcast_assistant_mod_src_param {
	/** Source ID of the receive state. */
	uint8_t src_id;

	/** Whether to sync to periodic advertisements. */
	uint8_t pa_sync;

	/**
	 * @brief Periodic advertising interval.
	 *
	 * BT_BAP_PA_INTERVAL_UNKNOWN if unknown.
	 */
	uint16_t pa_interval;

	/** Number of subgroups */
	uint8_t num_subgroups;

	/** Pointer to array of subgroups */
	struct bt_bap_scan_delegator_subgroup *subgroups;
};

/** @brief Modify a source on the server.
 *
 *  @param conn          Connection to the server.
 *  @param param         Parameter struct.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_mod_src(struct bt_conn *conn,
				       struct bt_bap_broadcast_assistant_mod_src_param *param);

/** @brief Set a broadcast code to the specified receive state.
 *
 *  @param conn            Connection to the server.
 *  @param src_id          Source ID of the receive state.
 *  @param broadcast_code  The broadcast code.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_set_broadcast_code(
	struct bt_conn *conn, uint8_t src_id, uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE]);

/** @brief Remove a source from the server.
 *
 *  @param conn            Connection to the server.
 *  @param src_id          Source ID of the receive state.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_rem_src(struct bt_conn *conn, uint8_t src_id);

/** @brief Read the specified receive state from the server.
 *
 *  @param conn     Connection to the server.
 *  @param idx      The index of the receive start (0 up to the value from
 *                 bt_bap_broadcast_assistant_discover_cb)
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bap_broadcast_assistant_read_recv_state(struct bt_conn *conn,
					       uint8_t idx);

/** @} */ /* end of bt_bap */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BAP_ */
