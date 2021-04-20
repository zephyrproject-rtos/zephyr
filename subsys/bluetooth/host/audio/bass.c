/*  Bluetooth BASS */

/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/buf.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_BASS)
#define LOG_MODULE_NAME bt_bass
#include "common/log.h"

#include "bass_internal.h"
#include "../conn_internal.h"
#include "../hci_core.h"

#define PA_SYNC_TIMEOUT           10000 /* ms */
#define PA_SYNC_SKIP              5
#define PA_SYNC_RETRY_COUNT       6 /* similar to retries for connections */
#define PAST_TIMEOUT              K_SECONDS(10)

NET_BUF_SIMPLE_DEFINE_STATIC(read_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

struct bass_client_t {
	struct bt_conn *conn;
	uint8_t scanning;
};

struct bass_recv_state_internal_t {
	const struct bt_gatt_attr *attr;

	bool active;
	uint8_t index;
	struct bt_bass_recv_state state;
	uint32_t requested_bis_sync[CONFIG_BT_BASS_MAX_SUBGROUPS];
	uint8_t broadcast_code[BASS_BROADCAST_CODE_SIZE];
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	struct bt_le_per_adv_sync *pa_sync;
	bool pa_sync_pending;
	struct k_work_delayable pa_timer;
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */
};

struct bass_inst_t {
	uint8_t next_src_id;
	struct bass_client_t client_configs[CONFIG_BT_MAX_CONN];
	struct bass_recv_state_internal_t recv_states
		[CONFIG_BT_BASS_RECV_STATE_COUNT];
};

static bool conn_cb_registered;
static struct bass_inst_t bass_inst;
static struct bt_bass_cb_t *bass_cbs;

static void bt_debug_dump_recv_state(const struct bass_recv_state_internal_t *recv_state)
{
	const struct bt_bass_recv_state *state = &recv_state->state;

	BT_DBG("Receive State[%d]: src ID %u, addr %s, adv_sid %u, pa_sync_state %u, "
	       "encrypt state %u%s%s, num_subgroups %u",
	       recv_state->index, state->src_id, bt_addr_le_str(&state->addr),
	       state->adv_sid, state->pa_sync_state, state->encrypt_state,
	       state->encrypt_state == BASS_BIG_ENC_STATE_BAD_CODE ? ", bad code" : "",
	       state->encrypt_state == BASS_BIG_ENC_STATE_BAD_CODE ?
					bt_hex(state->bad_code, sizeof(state->bad_code)) : "",
	       state->num_subgroups);

	for (int i = 0; i < state->num_subgroups; i++) {
		const struct bt_bass_subgroup *subgroup = &state->subgroups[i];

		BT_DBG("\tSubsgroup[%d]: BIS sync %u (requested %u), metadata_len %u, metadata: %s",
		       i, subgroup->bis_sync, recv_state->requested_bis_sync[i],
		       subgroup->metadata_len,
		       bt_hex(subgroup->metadata, subgroup->metadata_len));
	}
}

static void net_buf_put_recv_state(const struct bass_recv_state_internal_t *recv_state)
{
	net_buf_simple_reset(&read_buf);

	__ASSERT(recv_state, "NULL receive state");

	net_buf_simple_add_u8(&read_buf, recv_state->state.src_id);
	net_buf_simple_add_u8(&read_buf, recv_state->state.addr.type);
	net_buf_simple_add_mem(&read_buf, &recv_state->state.addr.a,
			       sizeof(recv_state->state.addr.a));
	net_buf_simple_add_u8(&read_buf, recv_state->state.adv_sid);
	net_buf_simple_add_u8(&read_buf, recv_state->state.pa_sync_state);
	net_buf_simple_add_u8(&read_buf, recv_state->state.encrypt_state);
	if (recv_state->state.encrypt_state == BASS_BIG_ENC_STATE_BAD_CODE) {
		net_buf_simple_add_mem(&read_buf, &recv_state->state.bad_code,
				       sizeof(recv_state->state.bad_code));
	}
	net_buf_simple_add_u8(&read_buf, recv_state->state.num_subgroups);
	for (int i = 0; i < recv_state->state.num_subgroups; i++) {
		const struct bt_bass_subgroup *subgroup = &recv_state->state.subgroups[i];

		net_buf_simple_add_le32(&read_buf, subgroup->bis_sync);
		net_buf_simple_add_u8(&read_buf, subgroup->metadata_len);
		net_buf_simple_add_mem(&read_buf, subgroup, subgroup->metadata_len);
	}
}

static void bass_disconnected(struct bt_conn *conn, uint8_t reason)
{
	int i;
	struct bass_client_t *client = NULL;

	for (i = 0; i < ARRAY_SIZE(bass_inst.client_configs); i++) {
		if (bass_inst.client_configs[i].conn == conn) {
			client = &bass_inst.client_configs[i];
			break;
		}
	}

	if (client) {
		BT_DBG("Instance %u with addr %s disconnected",
		       i, bt_addr_le_str(bt_conn_get_dst(conn)));
		memset(client, 0, sizeof(*client));
	}
}

static void bass_security_changed(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	if (err || !conn->encrypt) {
		return;
	}

	if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		return;
	}

	/* Notify all receive states after a bonded device reconnects */
	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		struct bass_recv_state_internal_t *state = &bass_inst.recv_states[i];
		int err;

		if (!bass_inst.recv_states[i].active) {
			continue;
		}

		net_buf_put_recv_state(state);

		err = bt_gatt_notify_uuid(conn, BT_UUID_BASS_RECV_STATE,
					  state->attr, read_buf.data,
					  read_buf.len);
		if (err) {
			BT_WARN("Could not notify receive state[%d] to reconnecting client: %d",
				i, err);
		}
	}
}

static struct bt_conn_cb conn_cb = {
	.disconnected = bass_disconnected,
	.security_changed = bass_security_changed,
};

static struct bass_client_t *get_bass_client(struct bt_conn *conn)
{
	struct bass_client_t *new = NULL;

	for (int i = 0; i < ARRAY_SIZE(bass_inst.client_configs); i++) {
		if (bass_inst.client_configs[i].conn == conn) {
			return &bass_inst.client_configs[i];
		} else if (new == NULL &&
			   bass_inst.client_configs[i].conn == NULL) {
			new = &bass_inst.client_configs[i];
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
		next_src_id = bass_inst.next_src_id++;
		unique = true;
		for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
			if (bass_inst.recv_states[i].active &&
			    bass_inst.recv_states[i].state.src_id
				== next_src_id) {
				unique = false;
				break;
			}
		}
	}
	return next_src_id;
}

static struct bass_recv_state_internal_t *bass_lookup_src_id(uint8_t src_id)
{
	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		if (bass_inst.recv_states[i].active &&
		    bass_inst.recv_states[i].state.src_id == src_id) {
			return &bass_inst.recv_states[i];
		}
	}
	return NULL;
}

#if defined(CONFIG_BT_BASS_AUTO_SYNC)
static struct bass_recv_state_internal_t *bass_lookup_pa_sync(
	struct bt_le_per_adv_sync *sync)
{
	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		if (bass_inst.recv_states[i].pa_sync == sync) {
			return &bass_inst.recv_states[i];
		}
	}
	return NULL;
}

static struct bass_recv_state_internal_t *bass_lookup_addr(
	const bt_addr_le_t *addr)
{
	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		if (!bt_addr_le_cmp(&bass_inst.recv_states[i].state.addr,
				    addr)) {
			return &bass_inst.recv_states[i];
		}
	}

	return NULL;
}

static void pa_synced(struct bt_le_per_adv_sync *sync,
		      struct bt_le_per_adv_sync_synced_info *info)
{
	struct bass_recv_state_internal_t *state;

	if (info->conn) {
		state = bass_lookup_addr(info->addr);
	} else {
		state = bass_lookup_pa_sync(sync);
	}

	if (!state) {
		return;
	}

	/* Update pointer if PAST */
	state->pa_sync = sync;

	BT_DBG("Synced%s", info->conn ? " via PAST" : "");

	(void)k_work_cancel_delayable(&state->pa_timer);
	state->state.pa_sync_state = BASS_PA_STATE_SYNCED;
	state->pa_sync_pending = false;

	bt_debug_dump_recv_state(state);

	net_buf_put_recv_state(state);

	bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, state->attr,
			    read_buf.data, read_buf.len);

	if (bass_cbs && bass_cbs->pa_synced) {
		bass_cbs->pa_synced(&state->state, info);
	}

	/* TODO: Sync with all BIS indexes */
}


static void pa_terminated(struct bt_le_per_adv_sync *sync,
			  const struct bt_le_per_adv_sync_term_info *info)
{
	struct bass_recv_state_internal_t *state = bass_lookup_pa_sync(sync);

	BT_DBG("Terminated");

	if (state) {
		state->state.pa_sync_state = BASS_PA_STATE_NOT_SYNCED;
		state->pa_sync_pending = false;

		bt_debug_dump_recv_state(state);

		net_buf_put_recv_state(state);

		bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, state->attr,
				    read_buf.data, read_buf.len);

		if (bass_cbs && bass_cbs->pa_term) {
			bass_cbs->pa_term(&state->state, info);
		}
	}
}

static void pa_recv(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	struct bass_recv_state_internal_t *state = bass_lookup_pa_sync(sync);

	if (state) {
		if (bass_cbs && bass_cbs->pa_recv) {
			bass_cbs->pa_recv(&state->state, info, buf);
		}
	}
}

static struct bt_le_per_adv_sync_cb pa_sync_cb =  {
	.synced = pa_synced,
	.term = pa_terminated,
	.recv = pa_recv
};


static void pa_timer_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bass_recv_state_internal_t *recv_state = CONTAINER_OF(
		dwork, struct bass_recv_state_internal_t, pa_timer);

	BT_DBG("PA timeout");

	__ASSERT(recv_state, "NULL receive state");

	if (recv_state->state.pa_sync_state == BASS_PA_STATE_INFO_REQ) {
		recv_state->state.pa_sync_state = BASS_PA_STATE_NO_PAST;
	} else {
		int err = bt_le_per_adv_sync_delete(recv_state->pa_sync);

		if (err) {
			BT_ERR("Could not delete BASS pa_sync");
		}

		recv_state->state.pa_sync_state = BASS_PA_STATE_FAILED;

	}

	bt_debug_dump_recv_state(recv_state);

	net_buf_put_recv_state(recv_state);

	bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, recv_state->attr,
			    read_buf.data, read_buf.len);
}

#endif /* CONFIG_BT_BASS_AUTO_SYNC */

static void bass_pa_sync_past(struct bt_conn *conn,
			      struct bass_recv_state_internal_t *state,
			      uint16_t pa_interval)
{
	struct bt_bass_recv_state *recv_state = &state->state;
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	int err;
	struct bt_le_per_adv_sync_transfer_param param = { 0 };

	param.skip = PA_SYNC_SKIP;
	if (pa_interval == BASS_PA_INTERVAL_UNKNOWN) {
		param.timeout = PA_SYNC_TIMEOUT / 10;
	} else {
		param.timeout = MIN(BT_GAP_PER_ADV_MAX_MAX_TIMEOUT,
				    MAX(100, pa_interval * PA_SYNC_RETRY_COUNT) / 10);
	}

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);
	if (err) {
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	} else {
		recv_state->pa_sync_state = BASS_PA_STATE_INFO_REQ;
	}

	(void)k_work_reschedule(&state->pa_timer, K_MSEC(param.timeout * 10));
#else
	if (bass_cbs && bass_cbs->past_req) {
		bass_cbs->past_req(recv_state, conn);
	} else {
		BT_WARN("PAST not possible");
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	}
#endif /* CONFIG_BT_BASS_AUTO_SYNC */
}

static void bass_pa_sync_no_past(struct bass_recv_state_internal_t *state,
				 uint16_t pa_interval)
{
	struct bt_bass_recv_state *recv_state = &state->state;
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	int err;
	struct bt_le_per_adv_sync_param param = { 0 };

	if (state->pa_sync_pending) {
		BT_DBG("PA sync pending");
		return;
	}

	bt_addr_le_copy(&param.addr, &recv_state->addr);
	param.sid = recv_state->adv_sid;
	param.skip = PA_SYNC_SKIP;

	if (pa_interval == BASS_PA_INTERVAL_UNKNOWN) {
		param.timeout = PA_SYNC_TIMEOUT / 10;
	} else {
		param.timeout = MIN(BT_GAP_PER_ADV_MAX_MAX_TIMEOUT,
				    MAX(100, pa_interval * PA_SYNC_RETRY_COUNT) / 10);
	}

	err = bt_le_per_adv_sync_create(&param, &state->pa_sync);
	if (err) {
		BT_WARN("Could not sync per adv: %d", err);
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	} else {
		BT_DBG("PA sync pending for addr %s",
		       bt_addr_le_str(&recv_state->addr));
		state->pa_sync_pending = true;
		(void)k_work_reschedule(&state->pa_timer,
					K_MSEC(param.timeout * 10));
	}
#else
	if (bass_cbs && bass_cbs->pa_sync_req) {
		bass_cbs->pa_sync_req(recv_state, pa_interval);
	} else {
		BT_WARN("PA sync not possible");
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	}
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */
}

static void bass_pa_sync_cancel(struct bass_recv_state_internal_t *state)
{
	struct bt_bass_recv_state *recv_state = &state->state;
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	int err;

	(void)k_work_cancel_delayable(&state->pa_timer);

	if (!state->pa_sync) {
		return;
	}

	err = bt_le_per_adv_sync_delete(state->pa_sync);
	if (err) {
		BT_WARN("Could not delete per adv sync: %d", err);
	} else {
		state->pa_sync = NULL;
		recv_state->pa_sync_state = BASS_PA_STATE_NOT_SYNCED;
	}
#else
	if (bass_cbs && bass_cbs->pa_sync_term_req) {
		bass_cbs->pa_sync_term_req(recv_state);
	} else {
		BT_WARN("PA sync terminate not possible");
	}
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */
}

static void bass_pa_sync(struct bt_conn *conn, struct bass_recv_state_internal_t *state,
			 uint8_t sync_pa, uint16_t pa_interval)
{
	struct bt_bass_recv_state *recv_state = &state->state;

	BT_DBG("sync_pa %u, pa_interval 0x%04x", sync_pa, pa_interval);

	if (sync_pa) {
		if (recv_state->pa_sync_state == BASS_PA_STATE_SYNCED) {
			/* TODO: If the addr changed we need to resync */
			return;
		}

		/* TODO: Handle case where current state is BASS_PA_STATE_INFO_REQ */

		if (conn && sync_pa == BASS_PA_REQ_SYNC_PAST &&
		    BT_FEAT_LE_PAST_SEND(conn->le.features) &&
		    BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
			bass_pa_sync_past(conn, state, pa_interval);
		} else {
			bass_pa_sync_no_past(state, pa_interval);
		}
	} else {
		bass_pa_sync_cancel(state);
	}
}

static int bass_add_source(struct bt_conn *conn, struct net_buf_simple *buf)
{
	struct bass_recv_state_internal_t *internal_state = NULL;
	struct bt_bass_recv_state *state;
	bt_addr_t *addr;
	uint8_t pa_sync;
	uint16_t pa_interval;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bass_cp_add_src_t) - 1) {
		BT_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		if (!bass_inst.recv_states[i].active) {
			internal_state = &bass_inst.recv_states[i];
			break;
		}
	}

	if (!internal_state) {
		BT_DBG("Could not add src");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	state = &internal_state->state;

	state->src_id = next_src_id();
	state->addr.type = net_buf_simple_pull_u8(buf);
	if (state->addr.type > BT_ADDR_LE_RANDOM) {
		BT_DBG("Invalid address type %u", state->addr.type);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);

	}

	addr = net_buf_simple_pull_mem(buf, sizeof(*addr));
	bt_addr_copy(&state->addr.a, addr);

	state->adv_sid = net_buf_simple_pull_u8(buf);
	if (state->adv_sid > BT_GAP_SID_MAX) {
		BT_DBG("Invalid adv SID %u", state->adv_sid);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);

	}

	pa_sync = net_buf_simple_pull_u8(buf);
	if (pa_sync > BASS_PA_REQ_SYNC_PAST) {
		BT_DBG("Invalid PA sync value %u", pa_sync);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	pa_interval = net_buf_simple_pull_le16(buf);
	if (pa_interval < 0x0006) {
		BT_DBG("Invalid PA interval %u", pa_interval);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	state->num_subgroups = net_buf_simple_pull_u8(buf);
	for (int i = 0; i < state->num_subgroups; i++) {
		struct bt_bass_subgroup *subgroup = &state->subgroups[i];
		uint8_t *metadata;

		if (buf->len < (sizeof(subgroup->bis_sync) + sizeof(subgroup->metadata_len))) {
			BT_DBG("Invalid length %u", buf->size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		internal_state->requested_bis_sync[i] = net_buf_simple_pull_le32(buf);

		if (internal_state->requested_bis_sync[i] &&
		    pa_sync == BASS_PA_REQ_NO_SYNC) {
			BT_DBG("Cannot sync to BIS without PA");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (buf->len < subgroup->metadata_len) {
			BT_DBG("Invalid length %u", buf->size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}


		if (subgroup->metadata_len > CONFIG_BT_BASS_MAX_METADATA_LEN) {
			BT_WARN("Metadata too long %u/%u",
				subgroup->metadata_len,
				CONFIG_BT_BASS_MAX_METADATA_LEN);
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);
		memcpy(subgroup->metadata, metadata, subgroup->metadata_len);
	}

	if (buf->len != 0) {
		BT_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	internal_state->active = true;

	bass_pa_sync(conn, internal_state, pa_sync, pa_interval);

	BT_DBG("Index %u: New source added: ID 0x%02x",
	       internal_state->index, state->src_id);

	bt_debug_dump_recv_state(internal_state);

	net_buf_put_recv_state(internal_state);

	bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, internal_state->attr,
			    read_buf.data, read_buf.len);


	return 0;
}

static int bass_mod_src(struct bt_conn *conn, struct net_buf_simple *buf)
{
	struct bass_recv_state_internal_t *internal_state;
	struct bt_bass_recv_state *state;
	bt_addr_t *addr;
	uint8_t src_id;
	uint8_t old_pa_sync_state;
	bool notify = false;
	uint16_t pa_interval;
	uint8_t num_subgroups;
	struct bt_bass_subgroup subgroups[CONFIG_BT_BASS_MAX_SUBGROUPS];
	uint8_t pa_sync;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len < sizeof(struct bass_cp_mod_src_t) - 1) {
		BT_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (!internal_state) {
		BT_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BASS_ERR_OPCODE_INVALID_SRC_ID);
	}

	addr = net_buf_simple_pull_mem(buf, sizeof(*addr));

	pa_sync = net_buf_simple_pull_u8(buf);
	if (pa_sync > BASS_PA_REQ_SYNC_PAST) {
		BT_DBG("Invalid PA sync value %u", pa_sync);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	pa_interval = net_buf_simple_pull_le16(buf);

	num_subgroups = net_buf_simple_pull_u8(buf);
	for (int i = 0; i < num_subgroups; i++) {
		struct bt_bass_subgroup *subgroup = &subgroups[i];
		uint8_t *metadata;

		if (buf->len < (sizeof(subgroup->bis_sync) + sizeof(subgroup->metadata_len))) {
			BT_DBG("Invalid length %u", buf->size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		subgroup->bis_sync = net_buf_simple_pull_le32(buf);
		if (subgroup->bis_sync && pa_sync == BASS_PA_REQ_NO_SYNC) {
			BT_DBG("Cannot sync to BIS without PA");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}

		subgroup->metadata_len = net_buf_simple_pull_u8(buf);

		if (buf->len < subgroup->metadata_len) {
			BT_DBG("Invalid length %u", buf->size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (subgroup->metadata_len > CONFIG_BT_BASS_MAX_METADATA_LEN) {
			BT_WARN("Metadata too long %u/%u",
				subgroup->metadata_len,
				CONFIG_BT_BASS_MAX_METADATA_LEN);
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		metadata = net_buf_simple_pull_mem(buf, subgroup->metadata_len);

		memcpy(subgroup->metadata, metadata, subgroup->metadata_len);
	}

	if (buf->len != 0) {
		BT_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* All input has been validated; update receive state and check for changes */
	state = &internal_state->state;
	old_pa_sync_state = state->pa_sync_state;

	if (bt_addr_cmp(&state->addr.a, addr)) {
		bt_addr_copy(&state->addr.a, addr);
		notify = true;
	}

	if (state->num_subgroups != num_subgroups) {
		state->num_subgroups = num_subgroups;
		notify = true;
	}

	for (int i = 0; i < num_subgroups; i++) {
		internal_state->requested_bis_sync[i] = subgroups[i].bis_sync;

		if (subgroups[i].metadata_len != state->subgroups[i].metadata_len) {
			state->subgroups[i].metadata_len = subgroups[i].metadata_len;
			notify = true;
		}

		if (memcmp(subgroups[i].metadata, state->subgroups[i].metadata,
			   sizeof(subgroups[i].metadata))) {
			memcpy(state->subgroups[i].metadata, subgroups[i].metadata,
			       state->subgroups[i].metadata_len);
			state->subgroups[i].metadata_len = subgroups[i].metadata_len;
			notify = true;
		}
	}

	if (!notify || pa_sync != state->pa_sync_state) {
		/* If no change to the state don't sync PA */
		bass_pa_sync(conn, internal_state, pa_sync, pa_interval);
	}

	notify |= old_pa_sync_state != state->pa_sync_state;

	BT_DBG("Index %u: Source modifed: ID 0x%02x",
	       internal_state->index, state->src_id);
	bt_debug_dump_recv_state(internal_state);

	/* Notify if changed */
	if (notify) {
		net_buf_put_recv_state(internal_state);

		bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
				    internal_state->attr, read_buf.data,
				    read_buf.len);
	}

	return 0;
}

static int bass_broadcast_code(struct net_buf_simple *buf)
{
	struct bass_recv_state_internal_t *internal_state;
	uint8_t src_id;
	uint8_t *broadcast_code;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bass_cp_broadcase_code_t) - 1) {
		BT_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (!internal_state) {
		BT_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BASS_ERR_OPCODE_INVALID_SRC_ID);
	}

	broadcast_code = net_buf_simple_pull_mem(buf, sizeof(internal_state->broadcast_code));

	memcpy(internal_state->broadcast_code, broadcast_code,
	       sizeof(internal_state->broadcast_code));

	BT_DBG("Index %u: broadcast code added: %s", internal_state->index,
	       bt_hex(internal_state->broadcast_code, sizeof(internal_state->broadcast_code)));

	return 0;
}

static int bass_rem_src(struct net_buf_simple *buf)
{
	struct bass_recv_state_internal_t *internal_state;
	uint8_t src_id;

	/* subtract 1 as the opcode has already been pulled */
	if (buf->len != sizeof(struct bass_cp_rem_src_t) - 1) {
		BT_DBG("Invalid length %u", buf->size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	src_id = net_buf_simple_pull_u8(buf);
	internal_state = bass_lookup_src_id(src_id);

	if (!internal_state) {
		BT_DBG("Could not find state by src id %u", src_id);
		return BT_GATT_ERR(BASS_ERR_OPCODE_INVALID_SRC_ID);
	}

	bass_pa_sync(NULL /* unused */, internal_state, false, 0 /* unused */); /* delete syncs */

	BT_DBG("Index %u: Removed source with ID 0x%02x", internal_state->index, src_id);

	internal_state->active = false;
	memset(&internal_state->state, 0, sizeof(internal_state->state));
	memset(internal_state->requested_bis_sync, 0, sizeof(internal_state->requested_bis_sync));
	memset(internal_state->broadcast_code, 0, sizeof(internal_state->broadcast_code));

	/* restore src_id */
	internal_state->state.src_id = src_id;

	bt_debug_dump_recv_state(internal_state);

	net_buf_put_recv_state(internal_state);

	bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, internal_state->attr,
			    read_buf.data, read_buf.len);

	return 0;
}


static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	struct bass_client_t *bass_client;
	struct net_buf_simple buf;
	uint8_t opcode;
	int err;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (!len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	opcode = net_buf_simple_pull_u8(&buf);

	if (!BASS_VALID_OPCODE(opcode)) {
		return BT_GATT_ERR(BASS_ERR_OPCODE_NOT_SUPPORTED);
	}

	bass_client = get_bass_client(conn);

	if (!bass_client) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	BT_HEXDUMP_DBG(data, len, "Data");

	switch (opcode) {
	case BASS_OP_SCAN_STOP:
		BT_DBG("Client stopping scanning");

		if (buf.len != 0) {
			BT_DBG("Invalid length %u", buf.size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		bass_client->scanning = false;
		break;
	case BASS_OP_SCAN_START:
		BT_DBG("Client starting scanning");

		if (buf.len != 0) {
			BT_DBG("Invalid length %u", buf.size);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
		bass_client->scanning = true;
		break;
	case BASS_OP_ADD_SRC:
		BT_DBG("Client adding source");

		err = bass_add_source(conn, &buf);
		if (err) {
			BT_DBG("Could not add source %d", err);
			return err;
		}

		break;
	case BASS_OP_MOD_SRC:
		BT_DBG("Client modifying source");

		err = bass_mod_src(conn, &buf);
		if (err) {
			BT_DBG("Could not modfiy source %d", err);
			return err;
		}

		break;
	case BASS_OP_BROADCAST_CODE:
		BT_DBG("Client setting broadcast code");

		err = bass_broadcast_code(&buf);
		if (err) {
			BT_DBG("Could not set broadcast code");
			return err;
		}

		break;
	case BASS_OP_REM_SRC:
		BT_DBG("Client removing source");

		err = bass_rem_src(&buf);
		if (err) {
			BT_DBG("Could not remove source %d", err);
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
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_recv_state(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint8_t idx = (uint8_t)(uintptr_t)attr->user_data;
	struct bass_recv_state_internal_t *recv_state = &bass_inst.recv_states[idx];
	struct bt_bass_recv_state *state = &recv_state->state;

	BT_DBG("Index %u: Source ID 0x%02x", idx, state->src_id);
	bt_debug_dump_recv_state(recv_state);

	net_buf_put_recv_state(recv_state);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, read_buf.data,
				 read_buf.len);
}

#define RECEIVE_STATE_CHARACTERISTIC(idx) \
	BT_GATT_CHARACTERISTIC(BT_UUID_BASS_RECV_STATE, \
		BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,\
		BT_GATT_PERM_READ_ENCRYPT, \
		read_recv_state, NULL, (void *)idx), \
	BT_GATT_CCC(recv_state_cfg_changed, \
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

BT_GATT_SERVICE_DEFINE(bass_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BASS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BASS_CONTROL_POINT,
		BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE_ENCRYPT,
		NULL, write_control_point, NULL),
	RECEIVE_STATE_CHARACTERISTIC(0),
#if CONFIG_BT_BASS_RECV_STATE_COUNT > 1
	RECEIVE_STATE_CHARACTERISTIC(1),
#if CONFIG_BT_BASS_RECV_STATE_COUNT > 2
	RECEIVE_STATE_CHARACTERISTIC(2)
#endif /* CONFIG_BT_BASS_RECV_STATE_COUNT > 2 */
#endif /* CONFIG_BT_BASS_RECV_STATE_COUNT > 1 */
);

static int bt_bass_init(const struct device *unused)
{
	/* Store the point to the first characteristic in each receive state */
	bass_inst.recv_states[0].attr = &bass_svc.attrs[3];
	bass_inst.recv_states[0].index = 0;
#if CONFIG_BT_BASS_RECV_STATE_COUNT > 1
	bass_inst.recv_states[1].attr = &bass_svc.attrs[6];
	bass_inst.recv_states[1].index = 1;
#if CONFIG_BT_BASS_RECV_STATE_COUNT > 2
	bass_inst.recv_states[2].attr = &bass_svc.attrs[9];
	bass_inst.recv_states[2].index = 2;
#endif /* CONFIG_BT_BASS_RECV_STATE_COUNT > 2 */
#endif /* CONFIG_BT_BASS_RECV_STATE_COUNT > 1 */

#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	bt_le_per_adv_sync_cb_register(&pa_sync_cb);
	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		k_work_init_delayable(&bass_inst.recv_states[i].pa_timer,
				      pa_timer_handler);
	}
#endif
	return 0;
}

DEVICE_DEFINE(bt_bass, "bt_bass", &bt_bass_init, NULL, NULL, NULL,
	      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

/****************************** PUBLIC API ******************************/
void bt_bass_register_cb(struct bt_bass_cb_t *cb)
{
	bass_cbs = cb;
}

int bt_bass_set_synced(uint8_t src_id, uint8_t pa_sync_state,
		       uint32_t bis_synced[CONFIG_BT_BASS_MAX_SUBGROUPS],
		       uint8_t encrypt_state)
{
	struct bass_recv_state_internal_t *recv_state =
		bass_lookup_src_id(src_id);
	bool notify = false;

	/* Only allow setting the sync if AUTO_SYNC is off, but keep enabled for testing */
	if (IS_ENABLED(CONFIG_BT_BASS_AUTO_SYNC) &&
	    !IS_ENABLED(CONFIG_BT_TESTING)) {
		return -EACCES;
	}

	if (!recv_state) {
		return -EINVAL;
	} else if (encrypt_state > BASS_BIG_ENC_STATE_BAD_CODE) {
		return -EINVAL;
	} else if (pa_sync_state > BASS_PA_STATE_NO_PAST) {
		return -EINVAL;
	}

	for (int i = 0; i < recv_state->state.num_subgroups; i++) {
		if (bis_synced[i] && pa_sync_state == BASS_PA_STATE_NOT_SYNCED) {
			BT_DBG("Cannot set BIS sync when PA sync is not synced");
			return -EINVAL;
		}
		if (bis_synced[i] > recv_state->requested_bis_sync[i]) {
			BT_DBG("Subgroup[%d] invalid bis_sync value %x",
			       i, bis_synced[i]);
			return -EINVAL;
		}
		if (bis_synced[i] != recv_state->state.subgroups[i].bis_sync) {
			notify = true;
			recv_state->state.subgroups[i].bis_sync = bis_synced[i];
		}
	}

	BT_DBG("Index %u: Source ID 0x%02x synced", recv_state->index, src_id);

	if (recv_state->state.pa_sync_state != pa_sync_state ||
	    recv_state->state.encrypt_state != encrypt_state) {
		notify = true;
	}

	recv_state->state.pa_sync_state = pa_sync_state;
	recv_state->state.encrypt_state = encrypt_state;

	if (recv_state->state.encrypt_state == BASS_BIG_ENC_STATE_BAD_CODE) {
		memcpy(recv_state->state.bad_code, recv_state->broadcast_code,
		       sizeof(recv_state->state.bad_code));
	}

	bt_debug_dump_recv_state(recv_state);

	if (notify) {
		net_buf_put_recv_state(recv_state);

		bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
				    recv_state->attr, read_buf.data,
				    read_buf.len);
	}

	return 0;
}


int bt_bass_remove_source(uint8_t src_id)
{
	struct net_buf_simple buf;
	struct bass_cp_rem_src_t cp = {
		.opcode = BASS_OP_REM_SRC,
		.src_id = src_id
	};

	net_buf_simple_init_with_data(&buf, (void *)&cp, sizeof(cp));

	if (!bass_rem_src(&buf)) {
		return 0;
	} else {
		return -EINVAL;
	}
}
