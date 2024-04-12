/*  Bluetooth Scan Delegator */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>

#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/buf.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bap_scan_delegator, CONFIG_BT_BAP_SCAN_DELEGATOR_LOG_LEVEL);

#include "common/bt_str.h"

#include "audio_internal.h"
#include "bap_internal.h"
#include "../host/conn_internal.h"
#include "../host/hci_core.h"

#define PAST_TIMEOUT              K_SECONDS(10)

#define SCAN_DELEGATOR_BUF_SEM_TIMEOUT K_MSEC(CONFIG_BT_BAP_SCAN_DELEGATOR_BUF_TIMEOUT)
static K_SEM_DEFINE(read_buf_sem, 1, 1);
NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

enum bass_recv_state_internal_flag {
	/* TODO: Replace this flag with a k_work_delayable */
	BASS_RECV_STATE_INTERNAL_FLAG_NOTIFY_PEND,

	BASS_RECV_STATE_INTERNAL_FLAG_NUM,
};

struct broadcast_assistant {
	struct bt_conn *conn;
	uint8_t scanning;
};

/* TODO: Merge bass_recv_state_internal_t and bt_bap_scan_delegator_recv_state */
struct bass_recv_state_internal {
	const struct bt_gatt_attr *attr;

	bool active;
	uint8_t index;
	struct bt_bap_scan_delegator_recv_state state;
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];
	struct bt_le_per_adv_sync *pa_sync;
	/** Requested BIS sync bitfield for each subgroup */
	uint32_t requested_bis_sync[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];

	ATOMIC_DEFINE(flags, BASS_RECV_STATE_INTERNAL_FLAG_NUM);
};

struct bt_bap_scan_delegator_inst {
	uint8_t next_src_id;
	struct broadcast_assistant assistant_configs[CONFIG_BT_MAX_CONN];
	struct bass_recv_state_internal recv_states
		[CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT];
};

static bool conn_cb_registered;
static struct bt_bap_scan_delegator_inst scan_delegator;
static struct bt_bap_scan_delegator_cb *scan_delegator_cbs;

/**
 * @brief Returns whether a value's bits is a subset of another value's bits
 *
 * @param a The subset to check
 * @param b The bits to check against
 *
 * @return True if @p a is a bitwise subset of @p b
 */
static bool bits_subset_of(uint32_t a, uint32_t b)
{
	return (((a) & (~(b))) == 0);
}

static bool valid_bis_syncs(uint32_t bis_sync)
{
	if (bis_sync == BT_BAP_BIS_SYNC_NO_PREF) {
		return true;
	}

	if (bis_sync > BIT_MASK(31)) { /* Max BIS index */
		return false;
	}

	return true;
}

static bool bis_syncs_unique_or_no_pref(uint32_t requested_bis_syncs,
					uint32_t aggregated_bis_syncs)
{
	if (requested_bis_syncs == 0U || aggregated_bis_syncs == 0U) {
		return true;
	}

	if (requested_bis_syncs == BT_BAP_BIS_SYNC_NO_PREF &&
	    aggregated_bis_syncs == BT_BAP_BIS_SYNC_NO_PREF) {
		return true;
	}

	return (requested_bis_syncs & aggregated_bis_syncs) != 0U;
}

static void bt_debug_dump_recv_state(const struct bass_recv_state_internal *recv_state)
{
	if (recv_state->active) {
		const struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;
		const bool is_bad_code = state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE;

		LOG_DBG("Receive State[%d]: src ID %u, addr %s, adv_sid %u, broadcast_id 0x%06X, "
			"pa_sync_state %u, encrypt state %u%s%s, num_subgroups %u",
			recv_state->index, state->src_id, bt_addr_le_str(&state->addr),
			state->adv_sid, state->broadcast_id, state->pa_sync_state,
			state->encrypt_state, is_bad_code ? ", bad code" : "",
			is_bad_code ? bt_hex(state->bad_code, sizeof(state->bad_code)) : "",
			state->num_subgroups);

		for (uint8_t i = 0U; i < state->num_subgroups; i++) {
			const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

			LOG_DBG("\tSubgroup[%u]: BIS sync %u (requested %u), metadata_len %zu, "
				"metadata: %s",
				i, subgroup->bis_sync, recv_state->requested_bis_sync[i],
				subgroup->metadata_len,
				bt_hex(subgroup->metadata, subgroup->metadata_len));
		}
	} else {
		LOG_DBG("Inactive receive state");
	}
}

static void bass_notify_receive_state(struct bt_conn *conn,
				      const struct bass_recv_state_internal *internal_state)
{
	const uint8_t att_ntf_header_size = 3; /* opcode (1) + handle (2) */
	uint16_t max_ntf_size;
	uint16_t ntf_size;
	int err;

	if (conn != NULL) {
		max_ntf_size = bt_gatt_get_mtu(conn) - att_ntf_header_size;
	} else {
		max_ntf_size = MIN(BT_L2CAP_RX_MTU, BT_L2CAP_TX_MTU) - att_ntf_header_size;
	}

	ntf_size = MIN(max_ntf_size, read_buf.len);
	if (ntf_size < read_buf.len) {
		LOG_DBG("Sending truncated notification (%u/%u)", ntf_size, read_buf.len);
	}

	LOG_DBG("Sending bytes %d", ntf_size);
	err = bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
				  internal_state->attr, read_buf.data,
				  ntf_size);

	if (err != 0 && err != -ENOTCONN) {
		LOG_DBG("Could not notify receive state: %d", err);
	}
}

static void net_buf_put_recv_state(const struct bass_recv_state_internal *recv_state)
{
	const struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;

	net_buf_simple_reset(&read_buf);

	__ASSERT(recv_state, "NULL receive state");

	if (!recv_state->active) {
		/* Notify empty */

		return;
	}

	(void)net_buf_simple_add_u8(&read_buf, state->src_id);
	(void)net_buf_simple_add_u8(&read_buf, state->addr.type);
	(void)net_buf_simple_add_mem(&read_buf, &state->addr.a,
				     sizeof(state->addr.a));
	(void)net_buf_simple_add_u8(&read_buf, state->adv_sid);
	(void)net_buf_simple_add_le24(&read_buf, state->broadcast_id);
	(void)net_buf_simple_add_u8(&read_buf, state->pa_sync_state);
	(void)net_buf_simple_add_u8(&read_buf, state->encrypt_state);
	if (state->encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		(void)net_buf_simple_add_mem(&read_buf, &state->bad_code,
					     sizeof(state->bad_code));
	}
	(void)net_buf_simple_add_u8(&read_buf, state->num_subgroups);
	for (int i = 0; i < state->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

		(void)net_buf_simple_add_le32(&read_buf, subgroup->bis_sync >> 1);
		(void)net_buf_simple_add_u8(&read_buf, subgroup->metadata_len);
		(void)net_buf_simple_add_mem(&read_buf, subgroup->metadata,
					     subgroup->metadata_len);
	}
}

static void receive_state_updated(struct bt_conn *conn,
				  const struct bass_recv_state_internal *internal_state)
{
	int err;

	/* If something is holding the NOTIFY_PEND flag we should not notify now */
	if (atomic_test_bit(internal_state->flags,
			    BASS_RECV_STATE_INTERNAL_FLAG_NOTIFY_PEND)) {
		return;
	}

	err = k_sem_take(&read_buf_sem, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("Failed to take read_buf_sem: %d", err);

		return;
	}

	bt_debug_dump_recv_state(internal_state);
	net_buf_put_recv_state(internal_state);
	bass_notify_receive_state(conn, internal_state);
	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->recv_state_updated != NULL) {
		scan_delegator_cbs->recv_state_updated(conn,
						       &internal_state->state);
	}

	k_sem_give(&read_buf_sem);
}

static void bis_sync_request_updated(struct bt_conn *conn,
				     const struct bass_recv_state_internal *internal_state)
{
	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->bis_sync_req != NULL) {
		scan_delegator_cbs->bis_sync_req(conn, &internal_state->state,
						 internal_state->requested_bis_sync);
	}
}

static void scan_delegator_disconnected(struct bt_conn *conn, uint8_t reason)
{
	int i;
	struct broadcast_assistant *assistant = NULL;

	for (i = 0; i < ARRAY_SIZE(scan_delegator.assistant_configs); i++) {
		if (scan_delegator.assistant_configs[i].conn == conn) {
			assistant = &scan_delegator.assistant_configs[i];
			break;
		}
	}

	if (assistant != NULL) {
		LOG_DBG("Instance %u with addr %s disconnected",
		       i, bt_addr_le_str(bt_conn_get_dst(conn)));
		(void)memset(assistant, 0, sizeof(*assistant));
	}
}

static void scan_delegator_security_changed(struct bt_conn *conn,
					    bt_security_t level,
					    enum bt_security_err err)
{
	if (err != 0 || conn->encrypt == 0) {
		return;
	}

	if (bt_addr_le_is_bonded(conn->id, &conn->le.dst) == 0) {
		return;
	}

	/* Notify all receive states after a bonded device reconnects */
	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		struct bass_recv_state_internal *internal_state = &scan_delegator.recv_states[i];
		int gatt_err;

		if (!internal_state->active) {
			continue;
		}

		err = k_sem_take(&read_buf_sem, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
		if (err != 0) {
			LOG_DBG("Failed to take read_buf_sem: %d", err);

			return;
		}

		net_buf_put_recv_state(internal_state);

		gatt_err = bt_gatt_notify_uuid(conn, BT_UUID_BASS_RECV_STATE,
					       internal_state->attr, read_buf.data,
					       read_buf.len);

		k_sem_give(&read_buf_sem);

		if (gatt_err != 0) {
			LOG_WRN("Could not notify receive state[%d] to reconnecting assistant: %d",
				i, gatt_err);
		}
	}
}

static struct bt_conn_cb conn_cb = {
	.disconnected = scan_delegator_disconnected,
	.security_changed = scan_delegator_security_changed,
};

static struct broadcast_assistant *get_bap_broadcast_assistant(struct bt_conn *conn)
{
	struct broadcast_assistant *new = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.assistant_configs); i++) {
		if (scan_delegator.assistant_configs[i].conn == conn) {
			return &scan_delegator.assistant_configs[i];
		} else if (new == NULL &&
			   scan_delegator.assistant_configs[i].conn == NULL) {
			new = &scan_delegator.assistant_configs[i];
			new->conn = conn;
		}
	}

	if (!conn_cb_registered) {
		bt_conn_cb_register(&conn_cb);
		conn_cb_registered = true;
	}

	return new;
}

static uint8_t next_src_id(void)
{
	uint8_t next_src_id;
	bool unique = false;

	while (!unique) {
		next_src_id = scan_delegator.next_src_id++;
		unique = true;
		for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
			if (scan_delegator.recv_states[i].active &&
			    scan_delegator.recv_states[i].state.src_id == next_src_id) {
				unique = false;
				break;
			}
		}
	}

	return next_src_id;
}

static struct bass_recv_state_internal *bass_lookup_src_id(uint8_t src_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (scan_delegator.recv_states[i].active &&
		    scan_delegator.recv_states[i].state.src_id == src_id) {
			return &scan_delegator.recv_states[i];
		}
	}

	return NULL;
}

static struct bass_recv_state_internal *bass_lookup_pa_sync(struct bt_le_per_adv_sync *sync)
{
	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (scan_delegator.recv_states[i].pa_sync == sync) {
			return &scan_delegator.recv_states[i];
		}
	}

	return NULL;
}

static struct bass_recv_state_internal *bass_lookup_addr(const bt_addr_le_t *addr)
{
	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (bt_addr_le_eq(&scan_delegator.recv_states[i].state.addr,
				  addr)) {
			return &scan_delegator.recv_states[i];
		}
	}

	return NULL;
}

static struct bass_recv_state_internal *get_free_recv_state(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		struct bass_recv_state_internal *free_internal_state =
			&scan_delegator.recv_states[i];

		if (!free_internal_state->active) {
			return free_internal_state;
		}
	}

	return NULL;
}

static void pa_synced(struct bt_le_per_adv_sync *sync,
		      struct bt_le_per_adv_sync_synced_info *info)
{
	struct bass_recv_state_internal *internal_state;

	LOG_DBG("Synced%s", info->conn ? " via PAST" : "");

	internal_state = bass_lookup_addr(info->addr);
	if (internal_state == NULL) {
		LOG_DBG("BASS receive state not found");
		return;
	}

	internal_state->pa_sync = sync;

	if (internal_state->state.pa_sync_state != BT_BAP_PA_STATE_SYNCED) {
		internal_state->state.pa_sync_state = BT_BAP_PA_STATE_SYNCED;
		receive_state_updated(info->conn, internal_state);
	}
}

static void pa_terminated(struct bt_le_per_adv_sync *sync,
			  const struct bt_le_per_adv_sync_term_info *info)
{
	struct bass_recv_state_internal *internal_state = bass_lookup_pa_sync(sync);

	LOG_DBG("Terminated");
	if (internal_state == NULL) {
		LOG_DBG("BASS receive state not found");
		return;
	}

	internal_state->pa_sync = NULL;

	if (internal_state->state.pa_sync_state != BT_BAP_PA_STATE_NOT_SYNCED) {
		internal_state->state.pa_sync_state = BT_BAP_PA_STATE_NOT_SYNCED;
		receive_state_updated(NULL, internal_state);
	}
}

static struct bt_le_per_adv_sync_cb pa_sync_cb =  {
	.synced = pa_synced,
	.term = pa_terminated,
};

static bool supports_past(struct bt_conn *conn, uint8_t pa_sync_val)
{
	LOG_DBG("%p remote %s PAST, local %s PAST (req %u)", (void *)conn,
		BT_FEAT_LE_PAST_SEND(conn->le.features) ? "supports" : "does not support",
		BT_FEAT_LE_PAST_RECV(bt_dev.le.features) ? "supports" : "does not support",
		pa_sync_val);

	return pa_sync_val == BT_BAP_BASS_PA_REQ_SYNC_PAST &&
	       BT_FEAT_LE_PAST_SEND(conn->le.features) &&
	       BT_FEAT_LE_PAST_RECV(bt_dev.le.features);
}

static int pa_sync_request(struct bt_conn *conn,
			   const struct bt_bap_scan_delegator_recv_state *state,
			   uint8_t pa_sync_val, uint16_t pa_interval)
{
	int err = -EACCES;

	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->pa_sync_req != NULL) {
		const bool past_supported = supports_past(conn, pa_sync_val);

		err = scan_delegator_cbs->pa_sync_req(conn, state,
						      past_supported,
						      pa_interval);
	}

	return err;
}

static int pa_sync_term_request(struct bt_conn *conn,
				const struct bt_bap_scan_delegator_recv_state *state)
{
	int err = -EACCES;

	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->pa_sync_req != NULL) {
		err = scan_delegator_cbs->pa_sync_term_req(conn, state);
	}

	return err;
}

/* BAP 6.5.4 states that the Broadcast Assistant shall not initiate the Add Source operation
 * if the operation would result in duplicate values for the combined Source_Address_Type,
 * Source_Adv_SID, and Broadcast_ID fields of any Broadcast Receive State characteristic exposed
 * by the Scan Delegator.
 */
static bool bass_source_is_duplicate(uint32_t broadcast_id, uint8_t adv_sid, uint8_t addr_type)
{
	struct bass_recv_state_internal *state;

	for (size_t i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		state = &scan_delegator.recv_states[i];

		if (state != NULL && state->state.broadcast_id == broadcast_id &&
			state->state.adv_sid == adv_sid && state->state.addr.type == addr_type) {
			LOG_DBG("recv_state already exists at src_id=0x%02X", state->state.src_id);

			return true;
		}
	}

	return false;
}

static int scan_delegator_add_source(struct bt_conn *conn,
				     struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state = NULL;
	struct bt_bap_scan_delegator_recv_state *state;
	bt_addr_t *addr;
	uint8_t pa_sync;
	uint16_t pa_interval;
	uint32_t aggregated_bis_syncs = 0;
	uint32_t broadcast_id;
	bool bis_sync_requested;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bt_bap_bass_cp_add_src) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	internal_state = get_free_recv_state();
	if (internal_state == NULL) {
		LOG_DBG("Could not get free receive state");

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	state = &internal_state->state;

	state->src_id = next_src_id();
	state->addr.type = net_buf_simple_pull_u8(buf);
	if (state->addr.type > BT_ADDR_LE_RANDOM) {
		LOG_DBG("Invalid address type %u", state->addr.type);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	addr = net_buf_simple_pull_mem(buf, sizeof(*addr));
	bt_addr_copy(&state->addr.a, addr);

	state->adv_sid = net_buf_simple_pull_u8(buf);
	if (state->adv_sid > BT_GAP_SID_MAX) {
		LOG_DBG("Invalid adv SID %u", state->adv_sid);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	broadcast_id = net_buf_simple_pull_le24(buf);

	if (bass_source_is_duplicate(broadcast_id, state->adv_sid, state->addr.type)) {
		LOG_DBG("Adding broadcast_id=0x%06X, adv_sid=0x%02X, and addr.type=0x%02X would "
			"result in duplication", state->broadcast_id, state->adv_sid,
			state->addr.type);

		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	state->broadcast_id = broadcast_id;

	pa_sync = net_buf_simple_pull_u8(buf);
	if (pa_sync > BT_BAP_BASS_PA_REQ_SYNC) {
		LOG_DBG("Invalid PA sync value %u", pa_sync);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	pa_interval = net_buf_simple_pull_le16(buf);

	state->num_subgroups = net_buf_simple_pull_u8(buf);
	if (state->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u", state->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	bis_sync_requested = false;
	for (int i = 0; i < state->num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];
		uint8_t *metadata;

		if (buf->len < (sizeof(subgroup->bis_sync) + sizeof(subgroup->metadata_len))) {
			LOG_DBG("Invalid length %u", buf->size);
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		internal_state->requested_bis_sync[i] = net_buf_simple_pull_le32(buf);
		if (internal_state->requested_bis_sync[i] != BT_BAP_BIS_SYNC_NO_PREF) {
			/* Received BIS Index bitfield uses BIT(0) for BIS Index 1 */
			internal_state->requested_bis_sync[i] <<= 1;
		}

		if (internal_state->requested_bis_sync[i] &&
		    pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC) {
			LOG_DBG("Cannot sync to BIS without PA");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		if (internal_state->requested_bis_sync[i] != 0U) {
			bis_sync_requested = true;
		}

		/* Verify that the request BIS sync indexes are unique or no preference */
		if (!bis_syncs_unique_or_no_pref(internal_state->requested_bis_sync[i],
						 aggregated_bis_syncs)) {
			LOG_DBG("Duplicate BIS index [%d]%x (aggregated %x)",
				i, internal_state->requested_bis_sync[i],
				aggregated_bis_syncs);

			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		if (!valid_bis_syncs(internal_state->requested_bis_sync[i])) {
			LOG_DBG("Invalid BIS sync[%d]: 0x%08X", i,
				internal_state->requested_bis_sync[i]);

			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		aggregated_bis_syncs |= internal_state->requested_bis_sync[i];

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (buf->len < subgroup->metadata_len) {
			LOG_DBG("Invalid length %u", buf->size);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}


		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_WRN("Metadata too long %u/%u", subgroup->metadata_len,
				CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE);

			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);
		(void)memcpy(subgroup->metadata, metadata,
			     subgroup->metadata_len);
	}

	if (buf->len != 0) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	/* The active flag shall be set before any application callbacks, so that any calls for the
	 * receive state can be processed
	 */
	internal_state->active = true;

	/* Set NOTIFY_PEND flag to ensure that we only send 1 notification in case that the upper
	 * layer calls another function that changes the state in the pa_sync_request callback
	 */
	atomic_set_bit(internal_state->flags, BASS_RECV_STATE_INTERNAL_FLAG_NOTIFY_PEND);

	if (pa_sync != BT_BAP_BASS_PA_REQ_NO_SYNC) {
		int err;

		err = pa_sync_request(conn, state, pa_sync, pa_interval);

		if (err != 0) {
			(void)memset(state, 0, sizeof(*state));
			internal_state->active = false;

			LOG_DBG("PA sync %u from %p was rejected with reason %d", pa_sync, conn,
				err);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
	}

	LOG_DBG("Index %u: New source added: ID 0x%02x",
		internal_state->index, state->src_id);

	atomic_clear_bit(internal_state->flags,
			 BASS_RECV_STATE_INTERNAL_FLAG_NOTIFY_PEND);

	receive_state_updated(conn, internal_state);

	if (bis_sync_requested) {
		bis_sync_request_updated(conn, internal_state);
	}

	return 0;
}

static int scan_delegator_mod_src(struct bt_conn *conn,
				  struct net_buf_simple *buf)
{
	struct bt_bap_scan_delegator_recv_state backup_state;
	struct bass_recv_state_internal *internal_state;
	struct bt_bap_scan_delegator_recv_state *state;
	uint8_t src_id;
	bool state_changed = false;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bap_bass_subgroup
		subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS] = { 0 };
	uint8_t pa_sync;
	uint32_t aggregated_bis_syncs = 0;
	bool bis_sync_change_requested;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bt_bap_bass_cp_mod_src) - 1) {
		LOG_DBG("Invalid length %u", buf->len);

		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	LOG_DBG("src_id %u", src_id);

	if (internal_state == NULL) {
		LOG_DBG("Could not find state by src id %u", src_id);

		return BT_GATT_ERR(BT_BAP_BASS_ERR_INVALID_SRC_ID);
	}

	pa_sync = net_buf_simple_pull_u8(buf);
	if (pa_sync > BT_BAP_BASS_PA_REQ_SYNC) {
		LOG_DBG("Invalid PA sync value %u", pa_sync);

		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	pa_interval = net_buf_simple_pull_le16(buf);

	num_subgroups = net_buf_simple_pull_u8(buf);
	if (num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u", num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	bis_sync_change_requested = false;
	for (int i = 0; i < num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup = &subgroups[i];
		uint32_t old_bis_sync_req;
		uint8_t *metadata;

		if (buf->len < (sizeof(subgroup->bis_sync) + sizeof(subgroup->metadata_len))) {
			LOG_DBG("Invalid length %u", buf->len);
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		old_bis_sync_req = internal_state->requested_bis_sync[i];

		internal_state->requested_bis_sync[i] = net_buf_simple_pull_le32(buf);
		if (internal_state->requested_bis_sync[i] != BT_BAP_BIS_SYNC_NO_PREF) {
			/* Received BIS Index bitfield uses BIT(0) for BIS Index 1 */
			internal_state->requested_bis_sync[i] <<= 1;
		}

		if (internal_state->requested_bis_sync[i] &&
		    pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC) {
			LOG_DBG("Cannot sync to BIS without PA");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		if (old_bis_sync_req != internal_state->requested_bis_sync[i]) {
			bis_sync_change_requested = true;
		}

		/* Verify that the request BIS sync indexes are unique or no preference */
		if (!bis_syncs_unique_or_no_pref(internal_state->requested_bis_sync[i],
						 aggregated_bis_syncs)) {
			LOG_DBG("Duplicate BIS index [%d]%x (aggregated %x)",
				i, internal_state->requested_bis_sync[i],
				aggregated_bis_syncs);

			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		if (!valid_bis_syncs(internal_state->requested_bis_sync[i])) {
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
		aggregated_bis_syncs |= internal_state->requested_bis_sync[i];

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (buf->len < subgroup->metadata_len) {
			LOG_DBG("Invalid length %u", buf->len);
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_WRN("Metadata too long %u/%u", subgroup->metadata_len,
				CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE);
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);

		(void)memcpy(subgroup->metadata, metadata,
			     subgroup->metadata_len);
	}

	if (buf->len != 0) {
		LOG_DBG("Invalid length %u", buf->size);

		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	/* All input has been validated; update receive state and check for changes */
	state = &internal_state->state;

	/* Store backup in case upper layers rejects */
	(void)memcpy(&backup_state, state, sizeof(backup_state));

	if (state->num_subgroups != num_subgroups) {
		state->num_subgroups = num_subgroups;
		state_changed = true;
	}

	for (int i = 0; i < num_subgroups; i++) {
		/* If the metadata len is 0, we shall not overwrite the existing metadata */
		if (subgroups[i].metadata_len == 0) {
			continue;
		}

		if (subgroups[i].metadata_len != state->subgroups[i].metadata_len) {
			state->subgroups[i].metadata_len = subgroups[i].metadata_len;
			state_changed = true;
		}

		if (memcmp(subgroups[i].metadata, state->subgroups[i].metadata,
			   sizeof(subgroups[i].metadata)) != 0) {
			(void)memcpy(state->subgroups[i].metadata,
				     subgroups[i].metadata,
				     state->subgroups[i].metadata_len);
			state->subgroups[i].metadata_len = subgroups[i].metadata_len;
			state_changed = true;
		}
	}

	/* Set NOTIFY_PEND flag to ensure that we only send 1 notification in case that the upper
	 * layer calls another function that changes the state in the pa_sync_request callback
	 */
	atomic_set_bit(internal_state->flags, BASS_RECV_STATE_INTERNAL_FLAG_NOTIFY_PEND);

	/* Only send the sync request to upper layers if it is requested, and
	 * we are not already synced to the device
	 */
	if (pa_sync != BT_BAP_BASS_PA_REQ_NO_SYNC &&
	    state->pa_sync_state != BT_BAP_PA_STATE_SYNCED) {
		const int err = pa_sync_request(conn, state, pa_sync,
						pa_interval);

		if (err != 0) {
			/* Restore backup */
			(void)memcpy(state, &backup_state,
				     sizeof(backup_state));

			LOG_DBG("PA sync %u from %p was rejected with reason %d", pa_sync, conn,
				err);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
	} else if (pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC &&
		   (state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ ||
		    state->pa_sync_state == BT_BAP_PA_STATE_SYNCED)) {
		/* Terminate PA sync */
		const int err = pa_sync_term_request(conn, &internal_state->state);

		if (err != 0) {
			LOG_DBG("PA sync term from %p was rejected with reason %d", conn, err);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		state_changed = true;
	}

	atomic_clear_bit(internal_state->flags, BASS_RECV_STATE_INTERNAL_FLAG_NOTIFY_PEND);

	/* Notify if changed */
	if (state_changed) {
		LOG_DBG("Index %u: Source modified: ID 0x%02x",
			internal_state->index, state->src_id);

		receive_state_updated(conn, internal_state);
	}

	if (bis_sync_change_requested) {
		bis_sync_request_updated(conn, internal_state);
	}

	return 0;
}

static int scan_delegator_broadcast_code(struct bt_conn *conn,
					 struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state;
	uint8_t src_id;
	const uint8_t *broadcast_code;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bt_bap_bass_cp_broadcase_code) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (internal_state == NULL) {
		LOG_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BT_BAP_BASS_ERR_INVALID_SRC_ID);
	}

	broadcast_code = net_buf_simple_pull_mem(buf, sizeof(internal_state->broadcast_code));

	(void)memcpy(internal_state->broadcast_code, broadcast_code,
		     sizeof(internal_state->broadcast_code));

	LOG_DBG("Index %u: broadcast code added: %s", internal_state->index,
		bt_hex(internal_state->broadcast_code, sizeof(internal_state->broadcast_code)));

	if (scan_delegator_cbs != NULL &&
	    scan_delegator_cbs->broadcast_code != NULL) {
		scan_delegator_cbs->broadcast_code(conn, &internal_state->state,
						   broadcast_code);
	}

	return 0;
}

static int scan_delegator_rem_src(struct bt_conn *conn,
				  struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state;
	struct bt_bap_scan_delegator_recv_state *state;
	bool bis_sync_was_requested;
	uint8_t src_id;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bt_bap_bass_cp_rem_src) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (internal_state == NULL) {
		LOG_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BT_BAP_BASS_ERR_INVALID_SRC_ID);
	}

	state = &internal_state->state;

	/* If conn == NULL then it's a local operation and we do not need to ask the application */
	if (conn != NULL && (state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ ||
			     state->pa_sync_state == BT_BAP_PA_STATE_SYNCED)) {
		int err;

		/* Terminate PA sync */
		err = pa_sync_term_request(conn, &internal_state->state);
		if (err != 0) {
			LOG_DBG("PA sync term from %p was rejected with reason %d", conn, err);

			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
	}

	bis_sync_was_requested = false;
	for (uint8_t i = 0U; i < state->num_subgroups; i++) {
		if (internal_state->requested_bis_sync[i] != 0U) {
			bis_sync_was_requested = true;
			break;
		}
	}

	LOG_DBG("Index %u: Removed source with ID 0x%02x",
		internal_state->index, src_id);

	internal_state->active = false;
	internal_state->pa_sync = NULL;
	(void)memset(&internal_state->state, 0, sizeof(internal_state->state));
	(void)memset(internal_state->broadcast_code, 0,
		     sizeof(internal_state->broadcast_code));
	(void)memset(internal_state->requested_bis_sync, 0,
		     sizeof(internal_state->requested_bis_sync));

	if (bis_sync_was_requested) {
		bis_sync_request_updated(conn, internal_state);
	}
	receive_state_updated(conn, internal_state);

	return 0;
}

static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	struct broadcast_assistant *bap_broadcast_assistant;
	struct net_buf_simple buf;
	uint8_t opcode;
	int err;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (len == 0) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	opcode = net_buf_simple_pull_u8(&buf);

	if (!BT_BAP_BASS_VALID_OPCODE(opcode)) {
		return BT_GATT_ERR(BT_BAP_BASS_ERR_OPCODE_NOT_SUPPORTED);
	}

	bap_broadcast_assistant = get_bap_broadcast_assistant(conn);

	if (bap_broadcast_assistant == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	LOG_HEXDUMP_DBG(data, len, "Data");

	switch (opcode) {
	case BT_BAP_BASS_OP_SCAN_STOP:
		LOG_DBG("Assistant stopping scanning");

		if (buf.len != 0) {
			LOG_DBG("Invalid length %u", buf.size);
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}

		bap_broadcast_assistant->scanning = false;
		break;
	case BT_BAP_BASS_OP_SCAN_START:
		LOG_DBG("Assistant starting scanning");

		if (buf.len != 0) {
			LOG_DBG("Invalid length %u", buf.size);
			return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
		}
		bap_broadcast_assistant->scanning = true;
		break;
	case BT_BAP_BASS_OP_ADD_SRC:
		LOG_DBG("Assistant adding source");

		err = scan_delegator_add_source(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not add source %d", err);
			return err;
		}

		break;
	case BT_BAP_BASS_OP_MOD_SRC:
		LOG_DBG("Assistant modifying source");

		err = scan_delegator_mod_src(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not modify source %d", err);
			return err;
		}

		break;
	case BT_BAP_BASS_OP_BROADCAST_CODE:
		LOG_DBG("Assistant setting broadcast code");

		err = scan_delegator_broadcast_code(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not set broadcast code");
			return err;
		}

		break;
	case BT_BAP_BASS_OP_REM_SRC:
		LOG_DBG("Assistant removing source");

		err = scan_delegator_rem_src(conn, &buf);
		if (err != 0) {
			LOG_DBG("Could not remove source %d", err);
			return err;
		}

		break;
	default:
		break;
	}

	return len;
}

static void recv_state_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_recv_state(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint8_t idx = POINTER_TO_UINT(BT_AUDIO_CHRC_USER_DATA(attr));
	struct bass_recv_state_internal *recv_state = &scan_delegator.recv_states[idx];
	struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;

	if (recv_state->active) {
		ssize_t ret_val;
		int err;

		LOG_DBG("Index %u: Source ID 0x%02x", idx, state->src_id);

		err = k_sem_take(&read_buf_sem, SCAN_DELEGATOR_BUF_SEM_TIMEOUT);
		if (err != 0) {
			LOG_DBG("Failed to take read_buf_sem: %d", err);

			return err;
		}

		bt_debug_dump_recv_state(recv_state);
		net_buf_put_recv_state(recv_state);

		ret_val = bt_gatt_attr_read(conn, attr, buf, len, offset,
					    read_buf.data, read_buf.len);

		k_sem_give(&read_buf_sem);

		return ret_val;
	}
	LOG_DBG("Index %u: Not active", idx);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);

}

#define RECEIVE_STATE_CHARACTERISTIC(idx) \
	BT_AUDIO_CHRC(BT_UUID_BASS_RECV_STATE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,\
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_recv_state, NULL, UINT_TO_POINTER(idx)), \
	BT_AUDIO_CCC(recv_state_cfg_changed)

BT_GATT_SERVICE_DEFINE(bass_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BASS),
	BT_AUDIO_CHRC(BT_UUID_BASS_CONTROL_POINT,
		      BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
		      BT_GATT_PERM_WRITE_ENCRYPT,
		      NULL, write_control_point, NULL),
	RECEIVE_STATE_CHARACTERISTIC(0),
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1
	RECEIVE_STATE_CHARACTERISTIC(1),
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2
	RECEIVE_STATE_CHARACTERISTIC(2)
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2 */
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1 */
);

static int bt_bap_scan_delegator_init(void)
{
	/* Store the pointer to the first characteristic in each receive state */
	scan_delegator.recv_states[0].attr = &bass_svc.attrs[3];
	scan_delegator.recv_states[0].index = 0;
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1
	scan_delegator.recv_states[1].attr = &bass_svc.attrs[6];
	scan_delegator.recv_states[1].index = 1;
#if CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2
	scan_delegator.recv_states[2].attr = &bass_svc.attrs[9];
	scan_delegator.recv_states[2].index = 2;
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 2 */
#endif /* CONFIG_BT_BAP_SCAN_DELEGATOR_RECV_STATE_COUNT > 1 */

	bt_le_per_adv_sync_cb_register(&pa_sync_cb);

	return 0;
}

SYS_INIT(bt_bap_scan_delegator_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/****************************** PUBLIC API ******************************/
void bt_bap_scan_delegator_register_cb(struct bt_bap_scan_delegator_cb *cb)
{
	scan_delegator_cbs = cb;
}

int bt_bap_scan_delegator_set_pa_state(uint8_t src_id,
				       enum bt_bap_pa_state pa_state)
{
	struct bass_recv_state_internal *internal_state = bass_lookup_src_id(src_id);
	struct bt_bap_scan_delegator_recv_state *recv_state;

	if (internal_state == NULL) {
		LOG_DBG("Could not find recv_state by src_id %u", src_id);
		return -EINVAL;
	}

	recv_state = &internal_state->state;

	if (recv_state->pa_sync_state != pa_state) {
		recv_state->pa_sync_state = pa_state;
		receive_state_updated(NULL, internal_state);
	}

	return 0;
}

int bt_bap_scan_delegator_set_bis_sync_state(
	uint8_t src_id,
	uint32_t bis_synced[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS])
{
	struct bass_recv_state_internal *internal_state = bass_lookup_src_id(src_id);
	bool notify = false;

	if (internal_state == NULL) {
		LOG_DBG("Could not find recv_state by src_id %u", src_id);
		return -EINVAL;
	}

	if (internal_state->state.pa_sync_state != BT_BAP_PA_STATE_SYNCED) {
		LOG_DBG("PA for src_id %u isn't synced, cannot be BIG synced",
			src_id);
		return -EINVAL;
	}

	/* Verify state for all subgroups before assigning any data */
	for (uint8_t i = 0U; i < internal_state->state.num_subgroups; i++) {
		if (i >= CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
			break;
		}

		if (bis_synced[i] == BT_BAP_BIS_SYNC_NO_PREF ||
		    !bits_subset_of(bis_synced[i],
				    internal_state->requested_bis_sync[i])) {
			LOG_DBG("Subgroup[%u] invalid bis_sync value %x for %x",
				i, bis_synced[i], internal_state->requested_bis_sync[i]);

			return -EINVAL;
		}
	}

	for (uint8_t i = 0U; i < internal_state->state.num_subgroups; i++) {
		struct bt_bap_bass_subgroup *subgroup =
			&internal_state->state.subgroups[i];

		if (i >= CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
			break;
		}

		if (bis_synced[i] != subgroup->bis_sync) {
			notify = true;
			subgroup->bis_sync = bis_synced[i];
		}
	}

	LOG_DBG("Index %u: Source ID 0x%02x synced",
		internal_state->index, src_id);

	if (internal_state->state.encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		(void)memcpy(internal_state->state.bad_code,
			     internal_state->broadcast_code,
			     sizeof(internal_state->state.bad_code));
	}

	if (notify) {
		receive_state_updated(NULL, internal_state);
	}

	return 0;
}

static bool valid_bt_bap_scan_delegator_add_src_param(
	const struct bt_bap_scan_delegator_add_src_param *param)
{
	uint32_t aggregated_bis_syncs = 0U;

	if (param->broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("Invalid broadcast_id: %u", param->broadcast_id);

		return false;
	}

	if (param->pa_sync == NULL) {
		LOG_DBG("NULL pa_sync");

		return false;
	}

	if (param->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u",
			param->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return false;
	}

	for (uint8_t i = 0U; i < param->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &param->subgroups[i];

		if (!bis_syncs_unique_or_no_pref(subgroup->bis_sync,
						 aggregated_bis_syncs)) {
			LOG_DBG("Invalid BIS sync: %u", subgroup->bis_sync);

			return false;
		}

		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_DBG("subgroup[%u]: Invalid metadata_len: %u",
				i, subgroup->metadata_len);

			return false;
		}
	}

	return true;
}

int bt_bap_scan_delegator_add_src(const struct bt_bap_scan_delegator_add_src_param *param)
{
	struct bass_recv_state_internal *internal_state = NULL;
	struct bt_bap_scan_delegator_recv_state *state;
	struct bt_le_per_adv_sync_info sync_info;
	int err;

	CHECKIF(!valid_bt_bap_scan_delegator_add_src_param(param)) {
		return -EINVAL;
	}

	internal_state = bass_lookup_pa_sync(param->pa_sync);
	if (internal_state != NULL) {
		LOG_DBG("PA Sync already in a receive state with src_id %u",
			internal_state->state.src_id);

		return -EALREADY;
	}

	internal_state = get_free_recv_state();
	if (internal_state == NULL) {
		LOG_DBG("Could not get free receive state");

		return -ENOMEM;
	}

	err = bt_le_per_adv_sync_get_info(param->pa_sync, &sync_info);
	if (err != 0) {
		LOG_DBG("Failed to get sync info: %d", err);

		return err;
	}

	state = &internal_state->state;

	state->src_id = next_src_id();
	bt_addr_le_copy(&state->addr, &sync_info.addr);
	state->adv_sid = sync_info.sid;
	state->broadcast_id = param->broadcast_id;
	state->pa_sync_state = BT_BAP_PA_STATE_SYNCED;
	state->num_subgroups = param->num_subgroups;
	if (state->num_subgroups > 0U) {
		(void)memcpy(state->subgroups, param->subgroups,
			     sizeof(state->subgroups));
	} else {
		(void)memset(state->subgroups, 0, sizeof(state->subgroups));
	}

	internal_state->active = true;
	internal_state->pa_sync = param->pa_sync;

	/* Set all requested_bis_sync to BT_BAP_BIS_SYNC_NO_PREF, as no
	 * Broadcast Assistant has set any requests yet
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(internal_state->requested_bis_sync); i++) {
		internal_state->requested_bis_sync[i] = BT_BAP_BIS_SYNC_NO_PREF;
	}

	LOG_DBG("Index %u: New source added: ID 0x%02x",
		internal_state->index, state->src_id);

	receive_state_updated(NULL, internal_state);

	return state->src_id;
}

static bool valid_bt_bap_scan_delegator_mod_src_param(
	const struct bt_bap_scan_delegator_mod_src_param *param)
{
	uint32_t aggregated_bis_syncs = 0U;

	if (param->broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		LOG_DBG("Invalid broadcast_id: %u", param->broadcast_id);

		return false;
	}

	if (param->num_subgroups > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u",
			param->num_subgroups,
			CONFIG_BT_BAP_BASS_MAX_SUBGROUPS);

		return false;
	}

	for (uint8_t i = 0U; i < param->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *subgroup = &param->subgroups[i];

		if (subgroup->bis_sync == BT_BAP_BIS_SYNC_NO_PREF ||
		    !bis_syncs_unique_or_no_pref(subgroup->bis_sync,
						 aggregated_bis_syncs)) {
			LOG_DBG("Invalid BIS sync: %u", subgroup->bis_sync);

			return false;
		}

		if (subgroup->metadata_len > CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE) {
			LOG_DBG("subgroup[%u]: Invalid metadata_len: %u",
				i, subgroup->metadata_len);

			return false;
		}
	}

	return true;
}

int bt_bap_scan_delegator_mod_src(const struct bt_bap_scan_delegator_mod_src_param *param)
{
	struct bass_recv_state_internal *internal_state = NULL;
	struct bt_bap_scan_delegator_recv_state *state;
	bool state_changed = false;

	CHECKIF(!valid_bt_bap_scan_delegator_mod_src_param(param)) {
		return -EINVAL;
	}

	internal_state = bass_lookup_src_id(param->src_id);
	if (internal_state == NULL) {
		LOG_DBG("Could not find receive state with src_id %u", param->src_id);

		return -ENOENT;
	}

	state = &internal_state->state;

	if (state->broadcast_id != param->broadcast_id) {
		state->broadcast_id = param->broadcast_id;
		state_changed = true;
	}

	if (state->num_subgroups != param->num_subgroups) {
		state->num_subgroups = param->num_subgroups;
		state_changed = true;
	}

	if (state->encrypt_state != param->encrypt_state) {
		state->encrypt_state = param->encrypt_state;
		state_changed = true;
	}

	/* Verify that the BIS sync values is acceptable for the receive state */
	for (uint8_t i = 0U; i < state->num_subgroups; i++) {
		const uint32_t bis_sync = param->subgroups[i].bis_sync;
		const uint32_t bis_sync_requested = internal_state->requested_bis_sync[i];

		if (!bits_subset_of(bis_sync, bis_sync_requested)) {
			LOG_DBG("Subgroup[%d] invalid bis_sync value %x for %x",
				i, bis_sync, bis_sync_requested);

			return -EINVAL;
		}
	}

	for (uint8_t i = 0U; i < state->num_subgroups; i++) {
		const struct bt_bap_bass_subgroup *param_subgroup = &param->subgroups[i];
		struct bt_bap_bass_subgroup *subgroup = &state->subgroups[i];

		if (subgroup->bis_sync != param_subgroup->bis_sync) {
			subgroup->bis_sync = param_subgroup->bis_sync;
			state_changed = true;
		}

		/* If the metadata len is 0, we shall not overwrite the existing metadata */
		if (param_subgroup->metadata_len == 0U) {
			continue;
		}

		if (subgroup->metadata_len != param_subgroup->metadata_len) {
			subgroup->metadata_len = param_subgroup->metadata_len;
			state_changed = true;
		}

		if (subgroup->metadata_len != param_subgroup->metadata_len ||
		    memcmp(subgroup->metadata, param_subgroup->metadata,
			   param_subgroup->metadata_len) != 0) {
			(void)memcpy(subgroup->metadata,
				     param_subgroup->metadata,
				     param_subgroup->metadata_len);
			subgroup->metadata_len = param_subgroup->metadata_len;
			state_changed = true;
		}
	}

	if (state_changed) {
		LOG_DBG("Index %u: Source modified: ID 0x%02x",
			internal_state->index, state->src_id);

		receive_state_updated(NULL, internal_state);
	}

	return 0;
}

int bt_bap_scan_delegator_rem_src(uint8_t src_id)
{
	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, (void *)&src_id, sizeof(src_id));

	if (scan_delegator_rem_src(NULL, &buf) == 0) {
		return 0;
	} else {
		return -EINVAL;
	}
}

void bt_bap_scan_delegator_foreach_state(bt_bap_scan_delegator_state_func_t func, void *user_data)
{
	for (size_t i = 0U; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (scan_delegator.recv_states[i].active) {
			bool stop;

			stop = func(&scan_delegator.recv_states[i].state, user_data);
			if (stop) {
				return;
			}
		}
	}
}

struct scan_delegator_state_find_state_param {
	const struct bt_bap_scan_delegator_recv_state *recv_state;
	bt_bap_scan_delegator_state_func_t func;
	void *user_data;
};

static bool
scan_delegator_state_find_state_cb(const struct bt_bap_scan_delegator_recv_state *recv_state,
				   void *user_data)
{
	struct scan_delegator_state_find_state_param *param = user_data;
	bool found;

	found = param->func(recv_state, param->user_data);
	if (found) {
		param->recv_state = recv_state;

		return true;
	}

	return false;
}

const struct bt_bap_scan_delegator_recv_state *
bt_bap_scan_delegator_find_state(bt_bap_scan_delegator_state_func_t func, void *user_data)
{
	struct scan_delegator_state_find_state_param param = {
		.recv_state = NULL,
		.func = func,
		.user_data = user_data,
	};

	bt_bap_scan_delegator_foreach_state(scan_delegator_state_find_state_cb, &param);

	return param.recv_state;
}

const struct bt_bap_scan_delegator_recv_state *bt_bap_scan_delegator_lookup_src_id(uint8_t src_id)
{
	const struct bass_recv_state_internal *internal_state = bass_lookup_src_id(src_id);

	if (internal_state != NULL) {
		return &internal_state->state;
	}

	return NULL;
}
