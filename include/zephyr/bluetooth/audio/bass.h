/** @file
 *  @brief Header for Bluetooth BASS.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>

#if IS_ENABLED(CONFIG_BT_BASS)
#define BT_BASS_MAX_METADATA_LEN CONFIG_BT_BASS_MAX_METADATA_LEN
#define BT_BASS_MAX_SUBGROUPS    CONFIG_BT_BASS_MAX_SUBGROUPS
#else
#define BT_BASS_MAX_METADATA_LEN 0
#define BT_BASS_MAX_SUBGROUPS    0
#endif

#define BT_BASS_BROADCAST_CODE_SIZE        16

#define BT_BASS_PA_STATE_NOT_SYNCED        0x00
#define BT_BASS_PA_STATE_INFO_REQ          0x01
#define BT_BASS_PA_STATE_SYNCED            0x02
#define BT_BASS_PA_STATE_FAILED            0x03
#define BT_BASS_PA_STATE_NO_PAST           0x04

#define BT_BASS_BIG_ENC_STATE_NO_ENC       0x00
#define BT_BASS_BIG_ENC_STATE_BCODE_REQ    0x01
#define BT_BASS_BIG_ENC_STATE_DEC          0x02
#define BT_BASS_BIG_ENC_STATE_BAD_CODE     0x03

#define BT_BASS_ERR_OPCODE_NOT_SUPPORTED   0x80
#define BT_BASS_ERR_INVALID_SRC_ID         0x81

#define BT_BASS_PA_INTERVAL_UNKNOWN        0xFFFF

#define BT_BASS_BROADCAST_MAX_ID           0xFFFFFF

#define BT_BASS_BIS_SYNC_NO_PREF           0xFFFFFFFF

struct bt_bass_subgroup {
	uint32_t bis_sync;
	uint32_t requested_bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[BT_BASS_MAX_METADATA_LEN];
};

/* TODO: Only expose this as an opaque type */
struct bt_bass_recv_state {
	uint8_t src_id;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t req_pa_sync_value;
	uint8_t pa_sync_state;
	uint8_t encrypt_state;
	uint32_t broadcast_id; /* 24 bits */
	uint8_t bad_code[BT_BASS_BROADCAST_CODE_SIZE];
	uint8_t num_subgroups;
	struct bt_bass_subgroup subgroups[BT_BASS_MAX_SUBGROUPS];
};

struct bt_bass_cb {
	void (*pa_synced)(struct bt_bass_recv_state *recv_state,
			  const struct bt_le_per_adv_sync_synced_info *info);
	void (*pa_term)(struct bt_bass_recv_state *recv_state,
			const struct bt_le_per_adv_sync_term_info *info);
	void (*pa_recv)(struct bt_bass_recv_state *recv_state,
			const struct bt_le_per_adv_sync_recv_info *info,
			struct net_buf_simple *buf);
	void (*biginfo)(struct bt_bass_recv_state *recv_state,
			const struct bt_iso_biginfo *biginfo);
};

/**
 * @brief Registers the callbacks used by BASS.
 */
void bt_bass_register_cb(struct bt_bass_cb *cb);

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
int bt_bass_set_sync_state(uint8_t src_id, uint8_t pa_sync_state,
			   uint32_t bis_synced[BT_BASS_MAX_SUBGROUPS],
			   uint8_t encrypted);

/******************************** CLIENT API ********************************/

/**
 * @brief Callback function for bt_bass_client_discover.
 *
 * @param conn              The connection that was used to discover
 *                          Broadcast Audio Scan Service.
 * @param err               Error value. 0 on success,
 *                          GATT error or ERRNO on fail.
 * @param recv_state_count  Number of receive states on the server.
 */
typedef void (*bt_bass_client_discover_cb)(struct bt_conn *conn, int err,
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
typedef void (*bt_bass_client_scan_cb)(const struct bt_le_scan_recv_info *info,
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
typedef void (*bt_bass_client_recv_state_cb)(struct bt_conn *conn, int err,
					     const struct bt_bass_recv_state *state);

/**
 * @brief Callback function for when a receive state is removed.
 *
 * @param conn     The connection to the Broadcast Audio Scan Service server.
 * @param err      Error value. 0 on success, GATT error on fail.
 * @param src_id   The receive state.
 */
typedef void (*bt_bass_client_recv_state_rem_cb)(struct bt_conn *conn, int err,
						 uint8_t src_id);

/**
 * @brief Callback function for writes.
 *
 * @param conn    The connection to the peer device.
 * @param err     Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_bass_client_write_cb)(struct bt_conn *conn, int err);

struct bt_bass_client_cb {
	bt_bass_client_discover_cb        discover;
	bt_bass_client_scan_cb            scan;
	bt_bass_client_recv_state_cb      recv_state;
	bt_bass_client_recv_state_rem_cb  recv_state_removed;

	bt_bass_client_write_cb           scan_start;
	bt_bass_client_write_cb           scan_stop;
	bt_bass_client_write_cb           add_src;
	bt_bass_client_write_cb           mod_src;
	bt_bass_client_write_cb           broadcast_code;
	bt_bass_client_write_cb           rem_src;
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
int bt_bass_client_discover(struct bt_conn *conn);

/**
 * @brief Scan start for BISes for a remote server.
 *
 * This will let the Broadcast Audio Scan Service server know that this device
 * is actively scanning for broadcast sources.
 * The function can optionally also start scanning, if the caller does not want
 * to start scanning itself.
 *
 * Scan results, if @p start_scan is true, is sent to the
 * bt_bass_client_scan_cb callback.
 *
 * @param conn          Connection to the Broadcast Audio Scan Service server.
 *                      Used to let the server know that we are scanning.
 * @param start_scan    Start scanning if true. If false, the application should
 *                      enable scan itself.
 * @return int          Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_scan_start(struct bt_conn *conn, bool start_scan);

/**
 * @brief Stop remote scanning for BISes for a server.
 *
 * @param conn   Connection to the server.
 * @return int   Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_scan_stop(struct bt_conn *conn);

/**
 * @brief Registers the callbacks used by Broadcast Audio Scan Service client.
 */
void bt_bass_client_register_cb(struct bt_bass_client_cb *cb);

/** Parameters for adding a source to a Broadcast Audio Scan Service server */
struct bt_bass_add_src_param {
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
	 * BT_BASS_PA_INTERVAL_UNKNOWN if unknown.
	 */
	uint16_t pa_interval;
	/** Number of subgroups */
	uint8_t num_subgroups;
	/** Pointer to array of subgroups */
	struct bt_bass_subgroup *subgroups;
};

/**
 * @brief Add a source on the server.
 *
 * @param conn          Connection to the server.
 * @param param         Parameter struct.
 *
 * @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_add_src(struct bt_conn *conn,
			   struct bt_bass_add_src_param *param);

/** Parameters for modifying a source */
struct bt_bass_mod_src_param {
	/** Source ID of the receive state. */
	uint8_t src_id;
	/** Whether to sync to periodic advertisements. */
	uint8_t pa_sync;
	/**
	 * @brief Periodic advertising interval.
	 *
	 * BT_BASS_PA_INTERVAL_UNKNOWN if unknown.
	 */
	uint16_t pa_interval;
	/** Number of subgroups */
	uint8_t num_subgroups;
	/** Pointer to array of subgroups */
	struct bt_bass_subgroup *subgroups;
};

/** @brief Modify a source on the server.
 *
 *  @param conn          Connection to the server.
 *  @param param         Parameter struct.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_mod_src(struct bt_conn *conn,
			   struct bt_bass_mod_src_param *param);

/** @brief Set a broadcast code to the specified receive state.
 *
 *  @param conn            Connection to the server.
 *  @param src_id          Source ID of the receive state.
 *  @param broadcast_code  The broadcast code.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_set_broadcast_code(struct bt_conn *conn, uint8_t src_id,
				      uint8_t broadcast_code[BT_BASS_BROADCAST_CODE_SIZE]);

/** @brief Remove a source from the server.
 *
 *  @param conn            Connection to the server.
 *  @param src_id          Source ID of the receive state.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_rem_src(struct bt_conn *conn, uint8_t src_id);

/** @brief Read the specified receive state from the server.
 *
 *  @param conn     Connection to the server.
 *  @param idx      The index of the receive start (0 up to the value from
 *                 bt_bass_client_discover_cb)
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_read_recv_state(struct bt_conn *conn, uint8_t idx);
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_ */
