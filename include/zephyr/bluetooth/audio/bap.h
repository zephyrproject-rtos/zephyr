/** @file
 *  @brief Header for Bluetooth BAP.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BAP_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BAP_
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>

#if IS_ENABLED(CONFIG_BT_BAP_SCAN_DELEGATOR)
#define BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN
#define BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS    CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS
#else
#define BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN 0
#define BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS    0
#endif

/** Size of the broadcast code */
#define BT_BAP_BROADCAST_CODE_SIZE             16

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

/** Struct to hold subgroup specific information for the receive state */
struct bt_bap_scan_delegator_subgroup {
	/** BIS synced bitfield */
	uint32_t bis_sync;

	/** Requested BIS sync bitfield */
	uint32_t requested_bis_sync;

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
	uint8_t bad_code[BT_BAP_BROADCAST_CODE_SIZE];

	/** Number of subgroups */
	uint8_t num_subgroups;

	/** Subgroup specific information */
	struct bt_bap_scan_delegator_subgroup subgroups[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS];
};

struct bt_bap_scan_delegator_cb {
	/**
	 * @brief Receive state updated
	 *
	 * @param conn       Pointer to the connection to a remove device if the
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
	 * @brief Periodic advertising sync termincation request
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
};

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
 * @param enc_state      The BIG encryption state.
 * @return int           Error value. 0 on success, ERRNO on fail.
 */
int bt_bap_scan_delegator_set_bis_sync_state(
	uint8_t src_id,
	uint32_t bis_synced[BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS],
	enum bt_bap_big_enc_state enc_state);

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
	 * @param state    The receive state.
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
	struct bt_conn *conn, uint8_t src_id,
	uint8_t broadcast_code[BT_BAP_BROADCAST_CODE_SIZE]);

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
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BAP_ */
