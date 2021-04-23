/** @file
 *  @brief Header for Bluetooth BASS.
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_
#include <zephyr/types.h>
#include <bluetooth/conn.h>

#if IS_ENABLED(CONFIG_BT_BASS)
#define BASS_MAX_METADATA_LEN CONFIG_BT_BASS_MAX_METADATA_LEN
#define BASS_MAX_SUBGROUPS    CONFIG_BT_BASS_MAX_SUBGROUPS
#else
#define BASS_MAX_METADATA_LEN 0
#define BASS_MAX_SUBGROUPS    0
#endif

#define BASS_BROADCAST_CODE_SIZE        16

#define BASS_PA_STATE_NOT_SYNCED	0x00
#define BASS_PA_STATE_INFO_REQ		0x01
#define BASS_PA_STATE_SYNCED		0x02
#define BASS_PA_STATE_FAILED		0x03
#define BASS_PA_STATE_NO_PAST		0x04

#define BASS_BIG_ENC_STATE_NO_ENC       0x00
#define BASS_BIG_ENC_STATE_BCODE_REQ    0x01
#define BASS_BIG_ENC_STATE_DEC          0x02
#define BASS_BIG_ENC_STATE_BAD_CODE     0x03

#define BASS_ERR_OPCODE_NOT_SUPPORTED   0x80
#define BASS_ERR_OPCODE_INVALID_SRC_ID  0x81

#define BASS_PA_INTERVAL_UNKNOWN        0xFFFF

#define BT_BASS_BROADCAST_ID_SIZE       3

#define BT_BASS_BIS_SYNC_NO_PREF        0xFFFFFFFF

struct bt_bass_subgroup {
	uint32_t bis_sync;
	uint32_t requested_bis_sync;
	uint8_t metadata_len;
	uint8_t metadata[BASS_MAX_METADATA_LEN];
};

/* TODO: Only expose this as an opaque type */
struct bt_bass_recv_state {
	uint8_t src_id;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t pa_sync_state;
	uint8_t encrypt_state;
	uint32_t broadcast_id; /* 24 bits */
	uint8_t bad_code[BASS_BROADCAST_CODE_SIZE];
	uint8_t num_subgroups;
	struct bt_bass_subgroup subgroups[BASS_MAX_SUBGROUPS];
};

struct bt_bass_cb_t {
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	void (*pa_synced)(struct bt_bass_recv_state *recv_state,
			  const struct bt_le_per_adv_sync_synced_info *info);
	void (*pa_term)(struct bt_bass_recv_state *recv_state,
			const struct bt_le_per_adv_sync_term_info *info);
	void (*pa_recv)(struct bt_bass_recv_state *recv_state,
			const struct bt_le_per_adv_sync_recv_info *info,
			struct net_buf_simple *buf);
	void (*biginfo)(struct bt_bass_recv_state *recv_state,
			const struct bt_iso_biginfo *biginfo);
#else
	void (*pa_sync_req)(struct bt_bass_recv_state *recv_state,
			    uint16_t pa_interval);
	void (*past_req)(struct bt_bass_recv_state *recv_state,
			 struct bt_conn *conn);
	void (*pa_sync_term_req)(struct bt_bass_recv_state *recv_state);
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */
};

/**
 * @brief Registers the callbacks used by BASS.
 */
void bt_bass_register_cb(struct bt_bass_cb_t *cb);

/**
 * @brief Set the scan state of a receive state in the server
 *
 * @param src_id         The source id used to identify the receive state.
 * @param pa_sync_state  The sync state of the PA.
 * @param bis_synced     Array of bitfields to set the BIS sync state for each subgroup.
 * @param encrypted      The BIG encryption state.
 * @return int           Error value. 0 on success, ERRNO on fail.
 */
int bt_bass_set_synced(uint8_t src_id, uint8_t pa_sync_state,
		       uint32_t bis_synced[BASS_MAX_SUBGROUPS], uint8_t encrypted);


/******************************** CLIENT API ********************************/

/** @brief Callback function for bt_bass_client_discover.
 *
 *  @param conn              The connection that was used to discover BASS.
 *  @param err               Error value. 0 on success,
 *                           GATT error or ERRNO on fail.
 *  @param recv_state_count  Number of receive states on the server.
 */
typedef void (*bt_bass_client_discover_cb_t)(struct bt_conn *conn, int err,
					     uint8_t recv_state_count);

/** @brief Callback function for bt_bass_client_discover.
 *
 *  Called when the scanner finds an advertiser that advertises the
 *  BT_UUID_BROADCAST_AUDIO UUID.
 *
 *  @param info     Advertiser information.
 */
typedef void (*bt_bass_client_scan_cb_t)(const struct bt_le_scan_recv_info *info,
					 uint32_t broadcast_id);

/** @brief Callback function for when a receive state is read or updated
 *
 *  Called whenever a receive state is read or updated.
 *
 * @param conn     The connection to the BASS server.
 * @param err      Error value. 0 on success, GATT error on fail.
 * @param state    The receive state.
 */
typedef void (*bt_bass_client_recv_state_cb_t)(
	struct bt_conn *conn, int err, const struct bt_bass_recv_state *state);


/** @brief Callback function for when a receive state is removed.
 *
 * @param conn     The connection to the BASS server.
 * @param err      Error value. 0 on success, GATT error on fail.
 * @param src_id   The receive state.
 */
typedef void (*bt_bass_client_recv_state_rem_cb_t)(
	struct bt_conn *conn, int err, uint8_t src_id);

/** @brief Callback function for writes.
 *
 *  @param conn    The connection to the peer device.
 *  @param err     Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_bass_client_write_cb_t)(
	struct bt_conn *conn, int err);

struct bt_bass_client_cb_t {
	bt_bass_client_discover_cb_t        discover;
	bt_bass_client_scan_cb_t            scan;
	bt_bass_client_recv_state_cb_t      recv_state;
	bt_bass_client_recv_state_rem_cb_t  recv_state_removed;

	bt_bass_client_write_cb_t           scan_start;
	bt_bass_client_write_cb_t           scan_stop;
	bt_bass_client_write_cb_t           add_src;
	bt_bass_client_write_cb_t           mod_src;
	bt_bass_client_write_cb_t           broadcast_code;
	bt_bass_client_write_cb_t           rem_src;
};

/**
 * @brief Discover BASS on the server.
 *
 * Warning: Only one connection can be active at any time; discovering for a
 * new connection, will delete all previous data.
 *
 * @param conn  The connection
 * @return int  Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_discover(struct bt_conn *conn);

/**
 * @brief Start remote scanning for BIS' for a server.
 *
 * Scan results, if @p start_scan is true, is sent to the bt_bass_client_scan_cb_t callback.
 *
 * @param conn          Connection to the BASS server.
 *                      Used to let the server know that we are scanning.
 * @param start_scan    Start scanning if true. If false, the application should enable scan itself.
 * @return int          Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_scan_start(struct bt_conn *conn, bool start_scan);

/**
 * @brief Stop remote scanning for BIS' for a server.
 *
 * @param conn   Connection to the server.
 * @return int   Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_scan_stop(struct bt_conn *conn);

/**
 * @brief Registers the callbacks used by BASS client.
 */
void bt_bass_client_register_cb(struct bt_bass_client_cb_t *cb);

/** Parameters for adding a source to a BASS server */
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
	 * BASS_PA_INTERVAL_UNKNOWN if unknown.
	 */
	uint16_t pa_interval;
	/** Number of subgroups */
	uint8_t num_subgroups;
	/** Pointer to array of subgroups */
	struct bt_bass_subgroup *subgroups;
};

/** @brief Add a source on the server.
 *
 *  @param conn          Connection to the server.
 *  @param param         Parameter struct.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_add_src(struct bt_conn *conn, struct bt_bass_add_src_param *param);

/** Parameters for adding a source to a BASS server */
struct bt_bass_mod_src_param {
	/** Source ID of the receive state. */
	uint8_t src_id;
	/** Whether to sync to periodic advertisements. */
	uint8_t pa_sync;
	/** Periodic advertising interval. BASS_PA_INTERVAL_UNKNOWN if unknown. */
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
int bt_bass_client_mod_src(struct bt_conn *conn, struct bt_bass_mod_src_param *param);

/** @brief Add a broadcast code to the specified receive state.
 *
 *  @param conn            Connection to the server.
 *  @param src_id          Source ID of the receive state.
 *  @param broadcast_code  The broadcast code.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_broadcast_code(struct bt_conn *conn, uint8_t src_id,
				  uint8_t broadcast_code[16]);

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
 *                 bt_bass_client_discover_cb_t)
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_read_recv_state(struct bt_conn *conn, uint8_t idx);
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_BASS_ */
