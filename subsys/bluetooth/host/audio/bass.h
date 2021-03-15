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
#else
#define BASS_MAX_METADATA_LEN 0
#endif

#define BASS_BROADCAST_CODE_SIZE        16

#define BASS_PA_STATE_NOT_SYNCED	0x00
#define BASS_PA_STATE_INFO_REQ		0x01
#define BASS_PA_STATE_SYNCED		0x02
#define BASS_PA_STATE_FAILED		0x03
#define BASS_PA_STATE_NO_PAST		0x04

#define BASS_ERR_OPCODE_NOT_SUPPORTED   0x80

#define BASS_RECV_STATE_EMPTY(rs) \
	((rs)->src_id == 0 && (rs)->pa_sync_state == 0 && \
	 (rs)->bis_sync_state == 0 && (rs)->big_enc == 0 && \
	 !bt_addr_le_cmp(&(rs)->addr, BT_ADDR_LE_ANY) && (rs)->adv_sid == 0 && \
	 (rs)->metadata_len == 0)

struct bass_recv_state_t {
	uint8_t src_id;
	bt_addr_le_t addr;
	uint8_t adv_sid;
	uint8_t pa_sync_state;
	uint32_t bis_sync_state;
	uint8_t big_enc;
	uint8_t metadata_len;
	uint8_t metadata[BASS_MAX_METADATA_LEN];
} __packed;

struct bt_bass_cb_t {
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	void (*pa_synced)(uint8_t src_id, uint32_t bis_sync,
			  const struct bt_le_per_adv_sync_synced_info *info);
	void (*pa_term)(uint8_t src_id,
			const struct bt_le_per_adv_sync_term_info *info);
	void (*pa_recv)(uint8_t src_id,
			const struct bt_le_per_adv_sync_recv_info *info,
			struct net_buf_simple *buf);
#else
	void (*pa_sync_req)(uint8_t src_id, uint32_t bis_sync,
			    bt_addr_le_t *addr, uint8_t adv_sid,
			    uint8_t metadata_len, uint8_t *metadata);
	void (*past_req)(struct bt_conn *conn, uint8_t src_id,
			 uint32_t bis_sync, bt_addr_le_t *addr,
			 uint8_t metadata_len, uint8_t *metadata);
	void (*pa_sync_term_req)(uint8_t src_id);
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
 * @param bis_synced     Bitfield to set the BIS sync state.
 * @param encrypted      The BIG encryption state.
 * @return int           Error value. 0 on success, ERRNO on fail.
 */
int bt_bass_set_synced(uint8_t src_id, uint8_t pa_sync_state,
		       uint32_t bis_synced, uint8_t encrypted);


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
typedef void (*bt_bass_client_scan_cb_t)(
	const struct bt_le_scan_recv_info *info);

/** @brief Callback function for when a receive state is read, updated or
 *  removed.
 *
 *  Called whenever a receive state is read (typically during discovery), or
 *  later changed by either the client or the server.
 *
 * @param conn     The connection to the BASS server.
 * @param err      Error value. 0 on success, GATT error on fail.
 * @param state    The receive state
 */
typedef void (*bt_bass_client_recv_state_cb_t)(
	struct bt_conn *conn, int err, const struct bass_recv_state_t *state);

/** @brief Callback function for writes.
 *
 *  @param conn    The connection to the peer device.
 *  @param err     Error value. 0 on success, GATT error on fail.
 */
typedef void (*bt_bass_client_write_cb_t)(
	struct bt_conn *conn, int err);

struct bt_bass_client_cb_t {
	bt_bass_client_discover_cb_t    discover;
	bt_bass_client_scan_cb_t        scan;
	bt_bass_client_recv_state_cb_t  recv_state;
	bt_bass_client_recv_state_cb_t  recv_state_removed;

	bt_bass_client_write_cb_t       scan_start;
	bt_bass_client_write_cb_t       scan_stop;
	bt_bass_client_write_cb_t       add_src;
	bt_bass_client_write_cb_t       mod_src;
	bt_bass_client_write_cb_t       broadcast_code;
	bt_bass_client_write_cb_t       rem_src;
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
 * @param conn   Connection to the server, used to determine where to
 *               send scan results.
 * @return int   Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_scan_start(struct bt_conn *conn);

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

/** @brief Add a source on the server.
 *
 *  @param conn          Connection to the server.
 *  @param addr          Address of the advertiser.
 *  @param adv_sid       SID of the advertising set.
 *  @param sync_pa       Whether to sync to periodic advertisements.
 *  @param sync_bis      Which BIS to sync to.
 *  @param metadata_len  Length of the metadata.
 *  @param metadata      The metadata, if any.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_add_src(struct bt_conn *conn, const bt_addr_le_t *addr,
			   uint8_t adv_sid, bool sync_pa, uint32_t sync_bis,
			   uint8_t metadata_len, uint8_t *metadata);

/** @brief Modify a source on the server.
 *
 *  @param conn          Connection to the server.
 *  @param src_id        Source ID of the receive state.
 *  @param sync_pa       Whether to sync to periodic advertisements.
 *  @param sync_bis      Which BIS to sync to.
 *  @param metadata_len  Length of the metadata.
 *  @param metadata      The metadata, if any.
 *
 *  @return Error value. 0 on success, GATT error or ERRNO on fail.
 */
int bt_bass_client_mod_src(struct bt_conn *conn, uint8_t src_id,
			   bool sync_pa, uint32_t sync_bis,
			   uint8_t metadata_len, uint8_t *metadata);

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
