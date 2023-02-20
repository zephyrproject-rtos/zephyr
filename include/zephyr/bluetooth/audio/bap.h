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
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>

/**
 * @brief Bluetooth Basic Audio Profile (BAP)
 * @defgroup bt_bap Bluetooth Basic Audio Profile
 * @ingroup bluetooth
 * @{
 */

#if defined(CONFIG_BT_BAP_SCAN_DELEGATOR)
#define BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN
#define BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS    CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS
#else
#define BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN 0
#define BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS    0
#endif

#define BT_BAP_PA_STATE_NOT_SYNCED             0x00
#define BT_BAP_PA_STATE_INFO_REQ               0x01
#define BT_BAP_PA_STATE_SYNCED                 0x02
#define BT_BAP_PA_STATE_FAILED                 0x03
#define BT_BAP_PA_STATE_NO_PAST                0x04

#define BT_BAP_BIG_ENC_STATE_NO_ENC            0x00
#define BT_BAP_BIG_ENC_STATE_BCODE_REQ         0x01
#define BT_BAP_BIG_ENC_STATE_DEC               0x02
#define BT_BAP_BIG_ENC_STATE_BAD_CODE          0x03

#define BT_BAP_BASS_ERR_OPCODE_NOT_SUPPORTED   0x80
#define BT_BAP_BASS_ERR_INVALID_SRC_ID         0x81

#define BT_BAP_PA_INTERVAL_UNKNOWN             0xFFFF

#define BT_BAP_BROADCAST_MAX_ID                0xFFFFFF

#define BT_BAP_BIS_SYNC_NO_PREF                0xFFFFFFFF

/** @brief Abstract Audio Broadcast Sink structure. */
struct bt_bap_broadcast_sink;

/* TODO: Replace with struct bt_audio_base_subgroup */
struct bt_bap_scan_delegator_subgroup {
	uint32_t bis_sync;
	uint32_t requested_bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN];
};

/* TODO: Only expose this as an opaque type */
struct bt_bap_scan_delegator_recv_state {
	uint8_t src_id;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t req_pa_sync_value;
	uint8_t pa_sync_state;
	uint8_t encrypt_state;
	uint32_t broadcast_id; /* 24 bits */
	uint8_t bad_code[BT_AUDIO_BROADCAST_CODE_SIZE];
	uint8_t num_subgroups;
	struct bt_bap_scan_delegator_subgroup subgroups[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS];
};

struct bt_bap_scan_delegator_cb {
	void (*pa_synced)(struct bt_bap_scan_delegator_recv_state *recv_state,
			  const struct bt_le_per_adv_sync_synced_info *info);
	void (*pa_term)(struct bt_bap_scan_delegator_recv_state *recv_state,
			const struct bt_le_per_adv_sync_term_info *info);
	void (*pa_recv)(struct bt_bap_scan_delegator_recv_state *recv_state,
			const struct bt_le_per_adv_sync_recv_info *info,
			struct net_buf_simple *buf);
	void (*biginfo)(struct bt_bap_scan_delegator_recv_state *recv_state,
			const struct bt_iso_biginfo *biginfo);
};

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
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*config)(struct bt_conn *conn, const struct bt_audio_ep *ep, enum bt_audio_dir dir,
		      const struct bt_codec *codec, struct bt_audio_stream **stream,
		      struct bt_codec_qos_pref *const pref);

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
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*reconfig)(struct bt_audio_stream *stream, enum bt_audio_dir dir,
			const struct bt_codec *codec, struct bt_codec_qos_pref *const pref);

	/**
	 * @brief Stream QoS request callback
	 *
	 * QoS callback is called whenever an Audio Stream Quality of
	 * Service needs to be configured.
	 *
	 * @param stream  Stream object being reconfigured.
	 * @param qos     Quality of Service configuration.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*qos)(struct bt_audio_stream *stream, const struct bt_codec_qos *qos);

	/**
	 * @brief Stream Enable request callback
	 *
	 * Enable callback is called whenever an Audio Stream is requested to be enabled to stream.
	 *
	 * @param stream      Stream object being enabled.
	 * @param meta        Metadata entries
	 * @param meta_count  Number of metadata entries
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*enable)(struct bt_audio_stream *stream, const struct bt_codec_data *meta,
		      size_t meta_count);

	/**
	 * @brief Stream Start request callback
	 *
	 * Start callback is called whenever an Audio Stream is requested to start streaming.
	 *
	 * @param stream Stream object.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*start)(struct bt_audio_stream *stream);

	/**
	 * @brief Stream Metadata update request callback
	 *
	 * Metadata callback is called whenever an Audio Stream is requested to update its metadata.
	 *
	 * @param stream       Stream object.
	 * @param meta         Metadata entries
	 * @param meta_count   Number of metadata entries
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*metadata)(struct bt_audio_stream *stream, const struct bt_codec_data *meta,
			size_t meta_count);

	/**
	 * @brief Stream Disable request callback
	 *
	 * Disable callback is called whenever an Audio Stream is requested to disable the stream.
	 *
	 * @param stream Stream object being disabled.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*disable)(struct bt_audio_stream *stream);

	/**
	 * @brief Stream Stop callback
	 *
	 * Stop callback is called whenever an Audio Stream is requested to stop streaming.
	 *
	 * @param stream Stream object.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*stop)(struct bt_audio_stream *stream);

	/**
	 * @brief Stream release callback
	 *
	 * Release callback is called whenever a new Audio Stream needs to be released and thus
	 * deallocated.
	 *
	 * @param stream Stream object.
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*release)(struct bt_audio_stream *stream);
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
typedef void (*bt_bap_ep_func_t)(struct bt_audio_ep *ep, void *user_data);

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
int bt_bap_unicast_server_config_ase(struct bt_conn *conn, struct bt_audio_stream *stream,
				     struct bt_codec *codec,
				     const struct bt_codec_qos_pref *qos_pref);

/** @} */ /* End of group bt_bap_unicast_server */


/**
 * @brief BAP Broadcast Sink APIs
 * @defgroup bt_bap_broadcast_sink BAP Broadcast Sink APIs
 * @ingroup bt_bap
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
	void (*base_recv)(struct bt_bap_broadcast_sink *sink, const struct bt_audio_base *base);

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
			       struct bt_audio_stream *streams[], const uint8_t broadcast_code[16]);

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
 *  stopped using bt_audio_stream_stop.
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
 * @brief Set the sync state of a receive state in the server
 *
 * @param src_id         The source id used to identify the receive state.
 * @param pa_sync_state  The sync state of the PA.
 * @param bis_synced     Array of bitfields to set the BIS sync state for each
 *                       subgroup.
 * @param encrypted      The BIG encryption state.
 * @return int           Error value. 0 on success, ERRNO on fail.
 */
int bt_bap_scan_delegator_set_sync_state(uint8_t src_id, uint8_t pa_sync_state,
					 uint32_t bis_synced[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS],
					 uint8_t encrypted);

/******************************** CLIENT API ********************************/

/**
 * @brief Callback function for bt_bap_broadcast_assistant_discover.
 *
 * @param conn              The connection that was used to discover
 *                          Broadcast Audio Scan Service.
 * @param err               Error value. 0 on success,
 *                          GATT error or ERRNO on fail.
 * @param recv_state_count  Number of receive states on the server.
 */
typedef void (*bt_bap_broadcast_assistant_discover_cb)(struct bt_conn *conn,
						       int err,
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
typedef void (*bt_bap_broadcast_assistant_scan_cb)(const struct bt_le_scan_recv_info *info,
						   uint32_t broadcast_id);

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
	bt_bap_broadcast_assistant_discover_cb        discover;
	bt_bap_broadcast_assistant_scan_cb            scan;
	bt_bap_broadcast_assistant_recv_state_cb      recv_state;
	bt_bap_broadcast_assistant_recv_state_rem_cb  recv_state_removed;

	bt_bap_broadcast_assistant_write_cb           scan_start;
	bt_bap_broadcast_assistant_write_cb           scan_stop;
	bt_bap_broadcast_assistant_write_cb           add_src;
	bt_bap_broadcast_assistant_write_cb           mod_src;
	bt_bap_broadcast_assistant_write_cb           broadcast_code;
	bt_bap_broadcast_assistant_write_cb           rem_src;
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
	struct bt_conn *conn, uint8_t src_id,
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE]);

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
