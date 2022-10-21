/*  Bluetooth Scan Delegator */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/buf.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bap_scan_delegator, CONFIG_BT_BAP_SCAN_DELEGATOR_LOG_LEVEL);

#include "common/bt_str.h"

#include "audio_internal.h"
#include "bap_internal.h"
#include "../host/conn_internal.h"
#include "../host/hci_core.h"

#define PA_SYNC_SKIP              5
#define SYNC_RETRY_COUNT          6 /* similar to retries for connections */
#define PAST_TIMEOUT              K_SECONDS(10)

NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

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
	uint16_t pa_interval;
	bool broadcast_code_received;
	struct bt_le_per_adv_sync *pa_sync;
	bool pa_sync_pending;
	struct k_work_delayable pa_timer;
	uint8_t biginfo_num_bis;
	bool biginfo_received;
	bool big_encrypted;
	uint16_t iso_interval;
	struct bt_iso_big *big;
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

static int bis_sync(struct bass_recv_state_internal *state);

/* TODO: Integrate the following with the bt_audio (audio.h) API */
static void iso_recv(struct bt_iso_chan *chan,
		     const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	printk("Incoming data channel %p len %u\n", chan, buf->len);
}

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected with reason 0x%02x\n",
	       chan, reason);
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		= iso_recv,
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_qos bis_iso_qos;
static struct bt_iso_chan bis_iso_chan = {
	.ops = &iso_ops,
	.qos = &bis_iso_qos,
};

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
	return (requested_bis_syncs & aggregated_bis_syncs) != 0 &&
	       requested_bis_syncs != BT_BAP_BIS_SYNC_NO_PREF &&
	       aggregated_bis_syncs != BT_BAP_BIS_SYNC_NO_PREF;
}

static uint32_t aggregated_bis_syncs_get(const struct bass_recv_state_internal *recv_state)
{
	uint32_t aggregated_bis_syncs = 0;

	for (int i = 0; i < recv_state->state.num_subgroups; i++) {
		aggregated_bis_syncs |= recv_state->state.subgroups[i].requested_bis_sync;
	}

	return aggregated_bis_syncs;
}

static void bt_debug_dump_recv_state(const struct bass_recv_state_internal *recv_state)
{
	const struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;
	const bool is_bad_code = state->encrypt_state ==
					BT_BAP_BIG_ENC_STATE_BAD_CODE;

	LOG_DBG("Receive State[%d]: src ID %u, addr %s, adv_sid %u, "
		"broadcast_id %u, pa_sync_state %u, "
		"encrypt state %u%s%s, num_subgroups %u",
		recv_state->index, state->src_id, bt_addr_le_str(&state->addr), state->adv_sid,
		state->broadcast_id, state->pa_sync_state, state->encrypt_state,
		is_bad_code ? ", bad code" : "",
		is_bad_code ? bt_hex(state->bad_code, sizeof(state->bad_code)) : "",
		state->num_subgroups);

	for (int i = 0; i < state->num_subgroups; i++) {
		const struct bt_bap_scan_delegator_subgroup *subgroup = &state->subgroups[i];

		LOG_DBG("\tSubgroup[%d]: BIS sync %u (requested %u), metadata_len %u, metadata: %s",
			i, subgroup->bis_sync, subgroup->requested_bis_sync, subgroup->metadata_len,
			bt_hex(subgroup->metadata, subgroup->metadata_len));
	}
}

static void bass_notify_receive_state(const struct bass_recv_state_internal *state)
{
	int err = bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
				      state->attr, read_buf.data, read_buf.len);

	if (err != 0) {
		LOG_DBG("Could not notify receive state: %d", err);
	}
}

static void net_buf_put_recv_state(const struct bass_recv_state_internal *recv_state)
{
	const struct bt_bap_scan_delegator_recv_state *state = &recv_state->state;

	net_buf_simple_reset(&read_buf);

	__ASSERT(recv_state, "NULL receive state");

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
		const struct bt_bap_scan_delegator_subgroup *subgroup = &state->subgroups[i];

		(void)net_buf_simple_add_le32(&read_buf, subgroup->bis_sync);
		(void)net_buf_simple_add_u8(&read_buf, subgroup->metadata_len);
		(void)net_buf_simple_add_mem(&read_buf, subgroup,
					     subgroup->metadata_len);
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
	for (int i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		struct bass_recv_state_internal *state = &scan_delegator.recv_states[i];
		int err;

		if (!state->active) {
			continue;
		}

		net_buf_put_recv_state(state);

		err = bt_gatt_notify_uuid(conn, BT_UUID_BASS_RECV_STATE,
					  state->attr, read_buf.data,
					  read_buf.len);
		if (err != 0) {
			LOG_WRN("Could not notify receive state[%d] to reconnecting assistant: %d",
				i, err);
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

	for (int i = 0; i < ARRAY_SIZE(scan_delegator.assistant_configs); i++) {
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
		for (int i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
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
	for (int i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (scan_delegator.recv_states[i].active &&
		    scan_delegator.recv_states[i].state.src_id == src_id) {
			return &scan_delegator.recv_states[i];
		}
	}

	return NULL;
}

static struct bass_recv_state_internal *bass_lookup_pa_sync(struct bt_le_per_adv_sync *sync)
{
	for (int i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (scan_delegator.recv_states[i].pa_sync == sync) {
			return &scan_delegator.recv_states[i];
		}
	}

	return NULL;
}

static struct bass_recv_state_internal *bass_lookup_addr(const bt_addr_le_t *addr)
{
	for (int i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		if (bt_addr_le_eq(&scan_delegator.recv_states[i].state.addr,
				  addr)) {
			return &scan_delegator.recv_states[i];
		}
	}

	return NULL;
}

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t pa_timeout;

	if (pa_interval == BT_BAP_PA_INTERVAL_UNKNOWN) {
		/* Use maximum value to maximize chance of success */
		pa_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	} else {
		/* Ensure that the following calculation does not overflow silently */
		__ASSERT(SYNC_RETRY_COUNT < 10,
			 "SYNC_RETRY_COUNT shall be less than 10");

		/* Add retries and convert to unit in 10's of ms */
		pa_timeout = ((uint32_t)pa_interval * SYNC_RETRY_COUNT) / 10;

		/* Enforce restraints */
		pa_timeout = CLAMP(pa_timeout, BT_GAP_PER_ADV_MIN_TIMEOUT,
				   BT_GAP_PER_ADV_MAX_TIMEOUT);
	}

	return pa_timeout;
}

static void pa_synced(struct bt_le_per_adv_sync *sync,
		      struct bt_le_per_adv_sync_synced_info *info)
{
	struct bass_recv_state_internal *state;

	LOG_DBG("Synced%s", info->conn ? " via PAST" : "");

	if (info->conn != NULL) {
		state = bass_lookup_addr(info->addr);
	} else {
		state = bass_lookup_pa_sync(sync);
	}

	if (state == NULL) {
		LOG_DBG("BASS receive state not found");
		return;
	}

	/* Update pointer if PAST */
	state->pa_sync = sync;

	(void)k_work_cancel_delayable(&state->pa_timer);
	state->state.pa_sync_state = BT_BAP_PA_STATE_SYNCED;
	state->pa_sync_pending = false;

	bt_debug_dump_recv_state(state);
	net_buf_put_recv_state(state);
	bass_notify_receive_state(state);

	if (scan_delegator_cbs != NULL && scan_delegator_cbs->pa_synced != NULL) {
		scan_delegator_cbs->pa_synced(&state->state, info);
	}
}

static void pa_terminated(struct bt_le_per_adv_sync *sync,
			  const struct bt_le_per_adv_sync_term_info *info)
{
	struct bass_recv_state_internal *state = bass_lookup_pa_sync(sync);

	LOG_DBG("Terminated");

	if (state != NULL) {
		state->state.pa_sync_state = BT_BAP_PA_STATE_NOT_SYNCED;
		state->pa_sync_pending = false;

		bt_debug_dump_recv_state(state);
		net_buf_put_recv_state(state);
		bass_notify_receive_state(state);

		if (scan_delegator_cbs != NULL && scan_delegator_cbs->pa_term != NULL) {
			scan_delegator_cbs->pa_term(&state->state, info);
		}
	}
}

static void pa_recv(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *state = bass_lookup_pa_sync(sync);

	if (state != NULL) {
		if (scan_delegator_cbs  != NULL && scan_delegator_cbs->pa_recv != NULL) {
			scan_delegator_cbs->pa_recv(&state->state, info, buf);
		}
	}
}

static void biginfo_recv(struct bt_le_per_adv_sync *sync,
			 const struct bt_iso_biginfo *biginfo)
{
	struct bass_recv_state_internal *state = bass_lookup_pa_sync(sync);

	if (state == NULL || state->biginfo_received) {
		return;
	}

	state->big_encrypted = biginfo->encryption;
	state->iso_interval = biginfo->iso_interval * 5 / 4; /* Convert to ms */
	state->biginfo_num_bis = biginfo->num_bis;
	state->biginfo_received = true;

	if (state->big_encrypted) {
		if (state->broadcast_code_received) {
			/* TODO: For now assume that the BIG can be decrypted
			 * given the broadcast code. When we have a proper
			 * broadcast source, that should be in charge of
			 * validating this.
			 */
			state->state.encrypt_state = BT_BAP_BIG_ENC_STATE_DEC;
		} else {
			/* Request broadcast code from assistant */
			state->state.encrypt_state = BT_BAP_BIG_ENC_STATE_BCODE_REQ;
		}

		bt_debug_dump_recv_state(state);
		net_buf_put_recv_state(state);
		bass_notify_receive_state(state);
	} else {
		int err = bis_sync(state);

		if (err != 0) {
			LOG_DBG("BIS sync failed %d", err);
		}
	}

	if (scan_delegator_cbs != NULL && scan_delegator_cbs->biginfo != NULL) {
		scan_delegator_cbs->biginfo(&state->state, biginfo);
	}
}

static struct bt_le_per_adv_sync_cb pa_sync_cb =  {
	.synced = pa_synced,
	.term = pa_terminated,
	.recv = pa_recv,
	.biginfo = biginfo_recv
};

static void pa_timer_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bass_recv_state_internal *recv_state = CONTAINER_OF(
		dwork, struct bass_recv_state_internal, pa_timer);

	LOG_DBG("PA timeout");

	__ASSERT(recv_state, "NULL receive state");

	if (recv_state->state.pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		recv_state->state.pa_sync_state = BT_BAP_PA_STATE_NO_PAST;
	} else {
		int err = bt_le_per_adv_sync_delete(recv_state->pa_sync);

		if (err != 0) {
			LOG_ERR("Could not delete BASS pa_sync");
		}

		recv_state->state.pa_sync_state = BT_BAP_PA_STATE_FAILED;
	}
	recv_state->pa_sync_pending = false;

	bt_debug_dump_recv_state(recv_state);
	net_buf_put_recv_state(recv_state);
	bass_notify_receive_state(recv_state);
}

static int bis_sync(struct bass_recv_state_internal *state)
{
	int err;
	struct bt_iso_big_sync_param param;
	struct bt_iso_chan *bis_channels[1] = { &bis_iso_chan };

	if (state->big != NULL) {
		return -EALREADY;
	}

	bis_iso_qos.tx = NULL;

	param.bis_channels = bis_channels;
	param.num_bis = ARRAY_SIZE(bis_channels);
	param.encryption = false;
	param.bis_bitfield = aggregated_bis_syncs_get(state);
	param.mse = 0;
	param.sync_timeout = interval_to_sync_timeout(state->iso_interval);

	LOG_DBG("Bitfield %x", param.bis_bitfield);

	if (param.bis_bitfield == 0) {
		/* Don't attempt to sync anything */
		return 0;
	} else if (param.bis_bitfield == BT_BAP_BIS_SYNC_NO_PREF) {
		param.bis_bitfield = 0;
		/* Attempt to sync to all BISes */
		for (int i = 0; i < state->biginfo_num_bis; i++) {
			param.bis_bitfield |= BIT(i);
		}
	} else {
		/* Ensure that we don't attempt to sync to more BISes that is possible */
		uint32_t max_bis_bitfield = 0;

		for (int i = 0; i < state->biginfo_num_bis; i++) {
			max_bis_bitfield |= BIT(i);
		}

		param.bis_bitfield &= max_bis_bitfield;
	}

	/* TODO: For now we only support syncing to a single BIS,
	 * so only sync to first BIS
	 */
	for (int i = 0; i < state->biginfo_num_bis; i++) {
		if (param.bis_bitfield & BIT(i)) {
			param.bis_bitfield = BIT(i);
			break;
		}
	}

	err = bt_iso_big_sync(state->pa_sync, &param, &state->big);
	if (err != 0) {
		return err;
	}

	/* We could start a timer for BIG sync but there is no way to let the
	 * assistant know if it has timed out, so it doesn't really matter.
	 */
	return 0;
}

static int bis_sync_cancel(struct bass_recv_state_internal *state)
{
	int err;

	if (!state->big) {
		return 0;
	}

	err = bt_iso_big_terminate(state->big);
	if (err != 0) {
		return err;
	}

	state->big = NULL;

	return 0;
}

static void scan_delegator_pa_sync_past(struct bt_conn *conn,
					struct bass_recv_state_internal *state)
{
	struct bt_bap_scan_delegator_recv_state *recv_state = &state->state;
	int err;
	struct bt_le_per_adv_sync_transfer_param param = { 0 };

	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(state->pa_interval);

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);
	if (err != 0) {
		recv_state->pa_sync_state = BT_BAP_PA_STATE_FAILED;
	} else {
		recv_state->pa_sync_state = BT_BAP_PA_STATE_INFO_REQ;
		state->pa_sync_pending = true;

		/* Multiply by 10 as param.timeout is in unit of 10ms */
		(void)k_work_reschedule(&state->pa_timer,
					K_MSEC(param.timeout * 10));
	}
}

static void scan_delegator_pa_sync_no_past(struct bass_recv_state_internal *state)
{
	struct bt_bap_scan_delegator_recv_state *recv_state = &state->state;
	int err;
	struct bt_le_per_adv_sync_param param = { 0 };

	if (state->pa_sync_pending) {
		LOG_DBG("PA sync pending");
		return;
	}

	bt_addr_le_copy(&param.addr, &recv_state->addr);
	param.sid = recv_state->adv_sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = interval_to_sync_timeout(state->pa_interval);

	/* TODO: Validate that the advertise is broadcasting the same
	 * broadcast_id that the receive state has
	 */
	err = bt_le_per_adv_sync_create(&param, &state->pa_sync);
	if (err != 0) {
		LOG_WRN("Could not sync per adv: %d", err);
		recv_state->pa_sync_state = BT_BAP_PA_STATE_FAILED;
	} else {
		LOG_DBG("PA sync pending for addr %s", bt_addr_le_str(&recv_state->addr));
		state->pa_sync_pending = true;
		(void)k_work_reschedule(&state->pa_timer,
					K_MSEC(param.timeout * 10));
	}
}

static void scan_delegator_pa_sync_cancel(struct bass_recv_state_internal *state)
{
	struct bt_bap_scan_delegator_recv_state *recv_state = &state->state;
	int err;

	(void)k_work_cancel_delayable(&state->pa_timer);

	if (state->pa_sync == NULL) {
		return;
	}

	err = bt_le_per_adv_sync_delete(state->pa_sync);
	if (err != 0) {
		LOG_WRN("Could not delete per adv sync: %d", err);
	} else {
		state->pa_sync_pending = false;
		state->pa_sync = NULL;
		recv_state->pa_sync_state = BT_BAP_PA_STATE_NOT_SYNCED;
	}
}

static void scan_delegator_pa_sync(struct bt_conn *conn,
				   struct bass_recv_state_internal *state,
				   bool pa_past)
{
	struct bt_bap_scan_delegator_recv_state *recv_state = &state->state;

	LOG_DBG("pa_past %u, pa_interval 0x%04x", pa_past, state->pa_interval);

	if (recv_state->pa_sync_state == BT_BAP_PA_STATE_SYNCED ||
	    recv_state->pa_sync_state == BT_BAP_PA_STATE_INFO_REQ) {
		return;
	}

	if (conn != NULL && pa_past &&
	    BT_FEAT_LE_PAST_SEND(conn->le.features) &&
	    BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
		scan_delegator_pa_sync_past(conn, state);
	} else {
		scan_delegator_pa_sync_no_past(state);
	}
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

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bt_bap_bass_cp_add_src) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (int i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		struct bass_recv_state_internal *state = &scan_delegator.recv_states[i];

		if (!state->active) {
			internal_state = state;
			break;
		}
	}

	if (internal_state == NULL) {
		LOG_DBG("Could not add src");
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

	state->broadcast_id = net_buf_simple_pull_le24(buf);

	pa_sync = net_buf_simple_pull_u8(buf);
	if (pa_sync > BT_BAP_BASS_PA_REQ_SYNC) {
		LOG_DBG("Invalid PA sync value %u", pa_sync);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	pa_interval = net_buf_simple_pull_le16(buf);

	state->num_subgroups = net_buf_simple_pull_u8(buf);
	if (state->num_subgroups > CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u", state->num_subgroups,
			CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	for (int i = 0; i < state->num_subgroups; i++) {
		struct bt_bap_scan_delegator_subgroup *subgroup = &state->subgroups[i];
		uint8_t *metadata;

		if (buf->len < (sizeof(subgroup->bis_sync) + sizeof(subgroup->metadata_len))) {
			LOG_DBG("Invalid length %u", buf->size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		subgroup->requested_bis_sync = net_buf_simple_pull_le32(buf);

		if (subgroup->requested_bis_sync &&
		    pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC) {
			LOG_DBG("Cannot sync to BIS without PA");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		/* Verify that the request BIS sync indexes are unique or no preference */
		if (bis_syncs_unique_or_no_pref(subgroup->requested_bis_sync,
						aggregated_bis_syncs)) {
			LOG_DBG("Duplicate BIS index [%d]%x (aggregated %x)", i,
				subgroup->requested_bis_sync, aggregated_bis_syncs);

			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		if (!valid_bis_syncs(subgroup->requested_bis_sync)) {
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
		aggregated_bis_syncs |= subgroup->requested_bis_sync;

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (buf->len < subgroup->metadata_len) {
			LOG_DBG("Invalid length %u", buf->size);

			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}


		if (subgroup->metadata_len > CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN) {
			LOG_WRN("Metadata too long %u/%u", subgroup->metadata_len,
				CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN);

			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);
		(void)memcpy(subgroup->metadata, metadata,
			     subgroup->metadata_len);
	}

	if (buf->len != 0) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	internal_state->active = true;
	state->req_pa_sync_value = pa_sync;
	internal_state->pa_interval = pa_interval;

	if (pa_sync != BT_BAP_BASS_PA_REQ_NO_SYNC) {
		scan_delegator_pa_sync(conn, internal_state,
				       (pa_sync == BT_BAP_BASS_PA_REQ_SYNC_PAST));
	}

	LOG_DBG("Index %u: New source added: ID 0x%02x", internal_state->index, state->src_id);

	bt_debug_dump_recv_state(internal_state);
	net_buf_put_recv_state(internal_state);
	bass_notify_receive_state(internal_state);

	return 0;
}

static int scan_delegator_mod_src(struct bt_conn *conn,
				  struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state;
	struct bt_bap_scan_delegator_recv_state *state;
	uint8_t src_id;
	uint8_t old_pa_sync_state;
	bool state_changed = false;
	bool need_synced = false;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bap_scan_delegator_subgroup
		subgroups[CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS] = { 0 };
	uint8_t pa_sync;
	uint32_t aggregated_bis_syncs = 0;
	int err;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bt_bap_bass_cp_mod_src) - 1) {
		LOG_DBG("Invalid length %u", buf->len);

		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

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
	if (num_subgroups > CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS) {
		LOG_WRN("Too many subgroups %u/%u", num_subgroups,
			CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS);

		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	for (int i = 0; i < num_subgroups; i++) {
		struct bt_bap_scan_delegator_subgroup *subgroup = &subgroups[i];
		uint8_t *metadata;

		if (buf->len < (sizeof(subgroup->bis_sync) + sizeof(subgroup->metadata_len))) {
			LOG_DBG("Invalid length %u", buf->len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		subgroup->requested_bis_sync = net_buf_simple_pull_le32(buf);
		if (subgroup->requested_bis_sync &&
		    pa_sync == BT_BAP_BASS_PA_REQ_NO_SYNC) {
			LOG_DBG("Cannot sync to BIS without PA");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		/* Verify that the request BIS sync indexes are unique or no preference */
		if (bis_syncs_unique_or_no_pref(subgroup->requested_bis_sync,
						aggregated_bis_syncs)) {
			LOG_DBG("Duplicate BIS index [%d]%x (aggregated %x)", i,
				subgroup->requested_bis_sync, aggregated_bis_syncs);
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		if (!valid_bis_syncs(subgroup->requested_bis_sync)) {
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
		aggregated_bis_syncs |= subgroup->requested_bis_sync;

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (buf->len < subgroup->metadata_len) {
			LOG_DBG("Invalid length %u", buf->len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (subgroup->metadata_len > CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN) {
			LOG_WRN("Metadata too long %u/%u", subgroup->metadata_len,
				CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_METADATA_LEN);
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);

		(void)memcpy(subgroup->metadata, metadata,
			     subgroup->metadata_len);
	}

	if (buf->len != 0) {
		LOG_DBG("Invalid length %u", buf->size);

		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* All input has been validated; update receive state and check for changes */
	state = &internal_state->state;
	old_pa_sync_state = state->pa_sync_state;

	if (state->num_subgroups != num_subgroups) {
		state->num_subgroups = num_subgroups;
		state_changed = true;
		need_synced = true;
	}

	for (int i = 0; i < num_subgroups; i++) {
		if (state->subgroups[i].requested_bis_sync !=
		    subgroups[i].requested_bis_sync) {
			state->subgroups[i].requested_bis_sync =
				subgroups[i].requested_bis_sync;
			need_synced = true;
			state_changed = true;
		}

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

	if (state->req_pa_sync_value != pa_sync) {
		state->req_pa_sync_value = pa_sync;
		need_synced = true;
		state_changed = true;
	}

	internal_state->pa_interval = pa_interval;

	if (need_synced) {
		/* Terminated BIG first if existed */
		err = bis_sync_cancel(internal_state);
		if (err != 0) {
			LOG_WRN("Could not terminate existing BIG %d", err);

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		/* Terminated PA let's re-sync later */
		scan_delegator_pa_sync_cancel(internal_state);

		if (pa_sync != BT_BAP_BASS_PA_REQ_NO_SYNC) {
			scan_delegator_pa_sync(conn, internal_state,
					       (pa_sync == BT_BAP_BASS_PA_REQ_SYNC_PAST));
		}
	}

	state_changed |= old_pa_sync_state != state->pa_sync_state;

	LOG_DBG("Index %u: Source modified: ID 0x%02x", internal_state->index, state->src_id);
	bt_debug_dump_recv_state(internal_state);

	/* Notify if changed */
	if (state_changed) {
		net_buf_put_recv_state(internal_state);
		bass_notify_receive_state(internal_state);
	}

	return 0;
}

static int scan_delegator_broadcast_code(struct net_buf_simple *buf)
{
	struct bass_recv_state_internal *internal_state;
	uint8_t src_id;
	uint8_t *broadcast_code;
	int err;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bt_bap_bass_cp_broadcase_code) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
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

	internal_state->broadcast_code_received = true;

	if (!internal_state->biginfo_received) {
		return 0;
	}

	LOG_DBG("Syncing to BIS");

	err = bis_sync(internal_state);
	if (err != 0) {
		LOG_DBG("BIS sync failed %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

static int scan_delegator_rem_src(struct net_buf_simple *buf)
{
	int err;
	struct bass_recv_state_internal *internal_state;
	uint8_t src_id;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bt_bap_bass_cp_rem_src) - 1) {
		LOG_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (internal_state == NULL) {
		LOG_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BT_BAP_BASS_ERR_INVALID_SRC_ID);
	}

	/* Terminate PA sync */
	scan_delegator_pa_sync_cancel(internal_state);

	/* Check if successful */
	if (internal_state->pa_sync) {
		LOG_WRN("Could not terminate PA sync");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = bis_sync_cancel(internal_state);
	if (err != 0) {
		LOG_WRN("Could not terminate BIG %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	LOG_DBG("Index %u: Removed source with ID 0x%02x", internal_state->index, src_id);

	internal_state->active = false;
	(void)memset(&internal_state->state, 0, sizeof(internal_state->state));
	(void)memset(internal_state->broadcast_code, 0,
		     sizeof(internal_state->broadcast_code));
	internal_state->pa_interval = 0;
	internal_state->big_encrypted = 0;
	internal_state->iso_interval = 0;
	internal_state->biginfo_received = false;

	(void)bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
				  internal_state->attr, NULL, 0);

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
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
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
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		bap_broadcast_assistant->scanning = false;
		break;
	case BT_BAP_BASS_OP_SCAN_START:
		LOG_DBG("Assistant starting scanning");

		if (buf.len != 0) {
			LOG_DBG("Invalid length %u", buf.size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
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

		err = scan_delegator_broadcast_code(&buf);
		if (err != 0) {
			LOG_DBG("Could not set broadcast code");
			return err;
		}

		break;
	case BT_BAP_BASS_OP_REM_SRC:
		LOG_DBG("Assistant removing source");

		err = scan_delegator_rem_src(&buf);
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
		LOG_DBG("Index %u: Source ID 0x%02x", idx, state->src_id);

		bt_debug_dump_recv_state(recv_state);

		net_buf_put_recv_state(recv_state);

		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 read_buf.data, read_buf.len);
	} else {
		LOG_DBG("Index %u: Not active", idx);

		return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
	}
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

static int bt_bap_scan_delegator_init(const struct device *unused)
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
	for (int i = 0; i < ARRAY_SIZE(scan_delegator.recv_states); i++) {
		k_work_init_delayable(&scan_delegator.recv_states[i].pa_timer,
				      pa_timer_handler);
	}
	return 0;
}

DEVICE_DEFINE(bt_bap_scan_delegator, "bt_bap_scan_delegator",
	      &bt_bap_scan_delegator_init, NULL, NULL, NULL,
	      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

/****************************** PUBLIC API ******************************/
void bt_bap_scan_delegator_register_cb(struct bt_bap_scan_delegator_cb *cb)
{
	scan_delegator_cbs = cb;
}

int bt_bap_scan_delegator_set_sync_state(
	uint8_t src_id, uint8_t pa_sync_state,
	uint32_t bis_synced[CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS],
	uint8_t encrypt_state)
{
	struct bass_recv_state_internal *recv_state =
		bass_lookup_src_id(src_id);
	bool notify = false;

	if (recv_state == NULL) {
		return -EINVAL;
	} else if (encrypt_state > BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		return -EINVAL;
	} else if (pa_sync_state > BT_BAP_PA_STATE_NO_PAST) {
		return -EINVAL;
	}

	for (int i = 0; i < recv_state->state.num_subgroups; i++) {
		struct bt_bap_scan_delegator_subgroup *subgroup = &recv_state->state.subgroups[i];

		if (bis_synced[i] != 0 &&
		    pa_sync_state == BT_BAP_PA_STATE_NOT_SYNCED) {
			LOG_DBG("Cannot set BIS sync when PA sync is not synced");

			return -EINVAL;
		}

		if (bits_subset_of(bis_synced[i],
				   subgroup->requested_bis_sync)) {
			LOG_DBG("Subgroup[%d] invalid bis_sync value %x for %x", i, bis_synced[i],
				subgroup->requested_bis_sync);

			return -EINVAL;
		}

		if (bis_synced[i] != subgroup->bis_sync) {
			notify = true;
			subgroup->bis_sync = bis_synced[i];
		}
	}

	LOG_DBG("Index %u: Source ID 0x%02x synced", recv_state->index, src_id);

	if (recv_state->state.pa_sync_state != pa_sync_state ||
	    recv_state->state.encrypt_state != encrypt_state) {
		notify = true;
	}

	recv_state->state.pa_sync_state = pa_sync_state;
	recv_state->state.encrypt_state = encrypt_state;

	if (recv_state->state.encrypt_state == BT_BAP_BIG_ENC_STATE_BAD_CODE) {
		(void)memcpy(recv_state->state.bad_code,
			     recv_state->broadcast_code,
			     sizeof(recv_state->state.bad_code));
	}

	bt_debug_dump_recv_state(recv_state);

	if (notify) {
		net_buf_put_recv_state(recv_state);
		bass_notify_receive_state(recv_state);
	}

	return 0;
}

int bt_bap_scan_delegator_remove_source(uint8_t src_id)
{
	struct net_buf_simple buf;
	struct bt_bap_bass_cp_rem_src cp = {
		.opcode = BT_BAP_BASS_OP_REM_SRC,
		.src_id = src_id
	};

	net_buf_simple_init_with_data(&buf, (void *)&cp, sizeof(cp));

	if (scan_delegator_rem_src(&buf) == 0) {
		return 0;
	} else {
		return -EINVAL;
	}
}
