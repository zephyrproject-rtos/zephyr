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
#define PAST_TIMEOUT              K_SECONDS(10)

struct bass_client_t {
	struct bt_conn *conn;
	uint8_t scanning;
};

struct bass_recv_state_internal_t {
	const struct bt_gatt_attr *attr;

	bool active;
	uint8_t index;
	struct bass_recv_state_t state;
	uint32_t requested_bis_sync;
	uint8_t broadcast_code[BASS_BROADCAST_CODE_SIZE];
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	struct bt_le_per_adv_sync *pa_sync;
	bool pa_sync_pending;
	struct k_delayed_work past_timer;
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

static void bass_disconnected(struct bt_conn *conn, uint8_t reason)
{
	int i;
	struct bass_client_t *client = NULL;

	for (i = 0; i < ARRAY_SIZE(bass_inst.client_configs); i++) {
		if (bass_inst.client_configs[i].conn == conn) {
			client = &bass_inst.client_configs[i];
		}
	}

	if (client) {
		BT_DBG("Instance %u with addr %s disconnected",
		       i, bt_addr_le_str(bt_conn_get_dst(conn)));
		memset(client, 0, sizeof(*client));
	}
}

static struct bt_conn_cb conn_cb = {
	.disconnected = bass_disconnected,
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
		if (bass_inst.recv_states[i].state.src_id == src_id) {
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

static struct bass_recv_state_internal_t *bass_lookup_work(struct k_work *work)
{
	if (!work) {
		return NULL;
	}


	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		if (&bass_inst.recv_states[i].past_timer.work.work == work) {
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

	(void)k_delayed_work_cancel(&state->past_timer);
	state->state.pa_sync_state = BASS_PA_STATE_SYNCED;
	state->pa_sync_pending = false;

	bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, state->attr,
			    &state->state, BASS_ACTUAL_SIZE(state->state));

	if (bass_cbs && bass_cbs->pa_synced) {
		bass_cbs->pa_synced(state->state.src_id,
				    state->state.bis_sync_state, info);
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

		bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, state->attr,
				    &state->state,
				    BASS_ACTUAL_SIZE(state->state));

		if (bass_cbs && bass_cbs->pa_term) {
			bass_cbs->pa_term(state->state.src_id, info);
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
			bass_cbs->pa_recv(state->state.src_id, info, buf);
		}
	}
}

static struct bt_le_per_adv_sync_cb pa_sync_cb =  {
	.synced = pa_synced,
	.term = pa_terminated,
	.recv = pa_recv
};


static void past_timer_handler(struct k_work *work)
{
	struct bass_recv_state_internal_t *recv_state = bass_lookup_work(work);

	BT_DBG("PAST timeout");

	__ASSERT(recv_state, "NULL receive state");

	__ASSERT(recv_state->state.pa_sync_state == BASS_PA_STATE_INFO_REQ,
		 "Invalid receive state PA sync state");

	recv_state->state.pa_sync_state = BASS_PA_STATE_NO_PAST;

	bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, recv_state->attr,
			    &recv_state->state,
			    BASS_ACTUAL_SIZE((recv_state->state)));
}

#endif /* CONFIG_BT_BASS_AUTO_SYNC */

static void bass_pa_sync_past(struct bt_conn *conn, struct bass_recv_state_internal_t *state)
{
	struct bass_recv_state_t *recv_state = &state->state;
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	int err;
	struct bt_le_per_adv_sync_transfer_param param = { 0 };

	param.skip = PA_SYNC_SKIP;
	param.timeout = PA_SYNC_TIMEOUT;

	err = bt_le_per_adv_sync_transfer_subscribe(conn, &param);
	if (err) {
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	} else {
		recv_state->pa_sync_state = BASS_PA_STATE_INFO_REQ;
	}

	(void)k_delayed_work_submit(&state->past_timer, PAST_TIMEOUT);
#else
	if (bass_cbs && bass_cbs->past_req) {
		bass_cbs->past_req(conn, recv_state->src_id, state->requested_bis_sync,
				   &recv_state->addr, recv_state->metadata_len,
				   recv_state->metadata);
	} else {
		BT_WARN("PAST not possible");
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	}
#endif /* CONFIG_BT_BASS_AUTO_SYNC */
}

static void bass_pa_sync_no_past(struct bass_recv_state_internal_t *state)
{
	struct bass_recv_state_t *recv_state = &state->state;
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	int err;
	struct bt_le_per_adv_sync_param param = { 0 };

	if (state->pa_sync_pending) {
		return;
	}

	bt_addr_le_copy(&param.addr, &recv_state->addr);
	param.sid = recv_state->adv_sid;
	param.skip = PA_SYNC_SKIP;
	param.timeout = PA_SYNC_TIMEOUT;
	err = bt_le_per_adv_sync_create(&param, &state->pa_sync);
	if (err) {
		BT_WARN("Could not sync per adv: %d", err);
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	} else {
		BT_DBG("PA sync pending");
		state->pa_sync_pending = true;
	}
#else
	if (bass_cbs && bass_cbs->pa_sync_req) {
		bass_cbs->pa_sync_req(recv_state->src_id, state->requested_bis_sync,
				      &recv_state->addr, recv_state->adv_sid,
				      recv_state->metadata_len, recv_state->metadata);
	} else {
		BT_WARN("PA sync not possible");
		recv_state->pa_sync_state = BASS_PA_STATE_FAILED;
	}
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */
}

static void bass_pa_sync_cancel(struct bass_recv_state_internal_t *state)
{
	struct bass_recv_state_t *recv_state = &state->state;
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	int err;

	(void)k_delayed_work_cancel(&state->past_timer);

	if (!state->pa_sync) {
		return;
	}

	err = bt_le_per_adv_sync_delete(state->pa_sync);
	if (err) {
		BT_WARN("Could not delete per adv sync: %d", err);
	} else {
		state->pa_sync = NULL;
		state->pa_sync_pending = true;
		recv_state->pa_sync_state = BASS_PA_STATE_NOT_SYNCED;
	}
#else
	if (recv_state->pa_sync_state == BASS_PA_STATE_SYNCED &&
		bass_cbs && bass_cbs->pa_sync_term_req) {
		bass_cbs->pa_sync_term_req(recv_state->src_id);
	} else {
		BT_WARN("PA sync terminate not possible");
	}
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */
}

static void bass_pa_sync(struct bt_conn *conn,
			 struct bass_recv_state_internal_t *state, bool sync_pa)
{
	struct bass_recv_state_t *recv_state = &state->state;

	if (sync_pa) {
		if (recv_state->pa_sync_state == BASS_PA_STATE_SYNCED) {
			return;
		}

		if (conn &&
		    BT_FEAT_LE_PAST_SEND(conn->le.features) &&
		    BT_FEAT_LE_PAST_RECV(bt_dev.le.features)) {
			bass_pa_sync_past(conn, state);
		} else {
			bass_pa_sync_no_past(state);
		}
	} else {
		bass_pa_sync_cancel(state);
	}
}

static struct bass_recv_state_internal_t *bass_add_source(
	struct bt_conn *conn, const struct bass_cp_add_src_t *add_src)
{
	struct bass_recv_state_internal_t *internal_state = NULL;

	for (int i = 0; i < ARRAY_SIZE(bass_inst.recv_states); i++) {
		if (!bass_inst.recv_states[i].active) {
			internal_state = &bass_inst.recv_states[i];
			break;
		}
	}

	if (internal_state) {
		struct bass_recv_state_t *state = &internal_state->state;

		state->src_id = next_src_id();
		bt_addr_le_copy(&state->addr, &add_src->addr);
		state->adv_sid = add_src->adv_sid;
		state->metadata_len = add_src->metadata_len;
		if (state->metadata_len) {
			memcpy(state->metadata, add_src->metadata,
			       state->metadata_len);
		}
		internal_state->requested_bis_sync = add_src->bis_sync;
		internal_state->active = true;

		bass_pa_sync(conn, internal_state, add_src->pa_sync);

		BT_DBG("Index %u: New source added: ID 0x%02x, PA %u, BIS %u, "
		       "encrypt %u, addr %s, sid %u, metadata_len %u",
		       internal_state->index, state->src_id,
		       state->pa_sync_state, state->bis_sync_state,
		       state->big_enc, bt_addr_le_str(&state->addr),
		       state->adv_sid, state->metadata_len);

		bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
				    internal_state->attr, state,
				    BASS_ACTUAL_SIZE((*state)));
	} else {
		BT_DBG("Could not add src");
	}
	return internal_state;
}

static struct bass_recv_state_internal_t *bass_mod_src(
	struct bt_conn *conn, const struct bass_cp_mod_src_t *mod_src)
{
	struct bass_recv_state_internal_t *internal_state =
		bass_lookup_src_id(mod_src->src_id);
	uint8_t pa_sync;
	uint32_t bis_sync;
	bool notify = false;

	if (internal_state) {
		struct bass_recv_state_t *state = &internal_state->state;

		pa_sync = state->pa_sync_state;
		bis_sync = state->bis_sync_state;

		internal_state->requested_bis_sync = mod_src->bis_sync;
		bass_pa_sync(conn, internal_state, mod_src->pa_sync);

		notify |= state->metadata_len != mod_src->metadata_len;
		state->metadata_len = mod_src->metadata_len;

		notify |= memcmp(state->metadata, mod_src->metadata,
				 mod_src->metadata_len);
		if (state->metadata_len) {
			memcpy(state->metadata, mod_src->metadata,
			       state->metadata_len);
		}

		notify |= pa_sync != state->pa_sync_state;
		notify |= bis_sync != state->bis_sync_state;

		BT_DBG("Index %u: Source modifed: ID 0x%02x, PA %u, BIS %u, "
		       "encrypt %u, addr %s, sid %u, metadata len %u",
		       internal_state->index, state->src_id,
		       state->pa_sync_state, state->bis_sync_state,
		       state->big_enc, bt_addr_le_str(&state->addr),
		       state->adv_sid, state->metadata_len);

		/* Notify if changed */
		if (notify) {
			bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
					    internal_state->attr, state,
					    BASS_ACTUAL_SIZE(*state));
		}
	}
	return internal_state;
}

static struct bass_recv_state_internal_t *bass_broadcast_code(
	const struct bass_cp_broadcase_code_t *broadcast_code)
{
	struct bass_recv_state_internal_t *state =
		bass_lookup_src_id(broadcast_code->src_id);

	if (state) {
		memcpy(state->broadcast_code, broadcast_code->broadcast_code,
		       sizeof(state->broadcast_code));
		BT_DBG("Index %u: %s",
		       state->index,
		       bt_hex(state->broadcast_code,
			      sizeof(state->broadcast_code)));
	}
	return state;
}

static struct bass_recv_state_internal_t *bass_rem_src(
	const struct bass_cp_rem_src_t *rem_src)
{
	struct bass_recv_state_internal_t *state =
		bass_lookup_src_id(rem_src->src_id);

	if (state) {
		bass_pa_sync(NULL, state, false); /* delete syncs */
		memset(state->broadcast_code, 0, sizeof(state->broadcast_code));
		memset(&state->state, 0, sizeof(state->state));
		state->requested_bis_sync = 0;
		state->active = false;

		BT_DBG("Index %u: Removed source with ID 0x%02x",
		       state->index, rem_src->src_id);

		bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE, state->attr,
				    &state->state,
				    BASS_ACTUAL_SIZE(state->state));
	}
	return state;
}


static ssize_t write_control_point(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	const union bass_cp_t *cp = (const union bass_cp_t *)buf;
	struct bass_client_t *bass_client;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (!len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (!BASS_VALID_OPCODE(cp->opcode)) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	bass_client = get_bass_client(conn);

	if (!bass_client) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	BT_HEXDUMP_DBG(buf, len, "Data");

	switch (cp->opcode) {
	case BASS_OP_SCAN_STOP:
		BT_DBG("Client stopped scanning");
		if (len != sizeof(cp->scan_stop)) {
			BT_DBG("Invalid length %u", len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
		bass_client->scanning = false;
		break;
	case BASS_OP_SCAN_START:
		BT_DBG("Client started scanning");
		if (len != sizeof(cp->scan_start)) {
			BT_DBG("Invalid length %u", len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
		bass_client->scanning = true;
		break;
	case BASS_OP_ADD_SRC:
		BT_DBG("Client adding source");
		if (len >= sizeof(cp->add_src) ||
		    len < sizeof(cp->add_src) - sizeof(cp->add_src.metadata)) {
			BT_DBG("Invalid length %u", len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		} else if (cp->add_src.pa_sync > BASS_PA_REQ_SYNC ||
			   cp->add_src.addr.type > BT_ADDR_LE_RANDOM_ID ||
			   cp->add_src.adv_sid > BT_GAP_SID_MAX) {
			BT_DBG("Invalid data");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		} else if (cp->add_src.metadata_len > CONFIG_BT_BASS_MAX_METADATA_LEN) {
			BT_DBG("Metadata too long %u/%u",
			       cp->add_src.metadata_len, CONFIG_BT_BASS_MAX_METADATA_LEN);
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		if (!bass_add_source(conn, &cp->add_src)) {
			BT_DBG("Could not add source");
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
		break;
	case BASS_OP_MOD_SRC:
		BT_DBG("Client modifying source");
		if (len >= sizeof(cp->mod_src) ||
		    len < sizeof(cp->mod_src) - sizeof(cp->mod_src.metadata)) {
			BT_DBG("Invalid length %u", len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		} else if (cp->mod_src.pa_sync > BASS_PA_REQ_SYNC) {
			BT_DBG("Invalid data");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		} else if (cp->mod_src.metadata_len > CONFIG_BT_BASS_MAX_METADATA_LEN) {
			BT_DBG("Metadata too long %u/%u",
			       cp->mod_src.metadata_len, CONFIG_BT_BASS_MAX_METADATA_LEN);
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		if (!bass_mod_src(conn, &cp->mod_src)) {
			BT_DBG("Could not modfiy source");
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		break;
	case BASS_OP_BROADCAST_CODE:
		BT_DBG("Client adding broadcast code");
		if (len != sizeof(cp->broadcast_code)) {
			BT_DBG("Invalid length %u", len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (!bass_broadcast_code(&cp->broadcast_code)) {
			BT_DBG("Could not add broadcast code");
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
		break;
	case BASS_OP_REM_SRC:
		BT_DBG("Client removing source");
		if (len != sizeof(cp->rem_src)) {
			BT_DBG("Invalid length %u", len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (!bass_rem_src(&cp->rem_src)) {
			BT_DBG("Could not remove source ID 0x%02X",
			       cp->rem_src.src_id);
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
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
	struct bass_recv_state_t *state = &bass_inst.recv_states[idx].state;
	BT_DBG("idx %u, PA %u, BIS %u, encrypt %u, addr %s, sid %u, "
	       "metadata len %u",
	       idx, state->pa_sync_state, state->bis_sync_state, state->big_enc,
	       bt_addr_le_str(&state->addr), state->adv_sid,
	       state->metadata_len);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 state, BASS_ACTUAL_SIZE(*state));
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
		k_delayed_work_init(&bass_inst.recv_states[i].past_timer,
				    past_timer_handler);
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
		       uint32_t bis_synced, uint8_t encrypted)
{
	struct bass_recv_state_internal_t *recv_state =
		bass_lookup_src_id(src_id);
	bool notify = false;

	if (IS_ENABLED(CONFIG_BT_BASS_AUTO_SYNC)) {
		return -EACCES;
	}

	if (!recv_state) {
		return -EINVAL;
	} else if ((!pa_sync_state && bis_synced) ||
		   encrypted > BASS_BIG_ENC_STATE_DEC) {
		return -EINVAL;
	} else if (pa_sync_state > BASS_PA_STATE_NO_PAST) {
		return -EINVAL;
	} else if (bis_synced > recv_state->requested_bis_sync) {
		return -EINVAL;
	}

	BT_DBG("Index %u: Source ID 0x%02x synced %u BIS %u, encrypt %u",
	       recv_state->index, src_id, pa_sync_state, bis_synced, encrypted);

	if (recv_state->state.pa_sync_state != pa_sync_state ||
	    recv_state->state.bis_sync_state != bis_synced ||
	    recv_state->state.big_enc != encrypted) {
		notify = true;
	}

	recv_state->state.pa_sync_state = pa_sync_state;
	recv_state->state.bis_sync_state = bis_synced;
	recv_state->state.big_enc = encrypted;

	if (notify) {
		bt_gatt_notify_uuid(NULL, BT_UUID_BASS_RECV_STATE,
				    recv_state->attr, &recv_state->state,
				    BASS_ACTUAL_SIZE(recv_state->state));
	}

	return 0;
}


int bt_bass_remove_source(uint8_t src_id)
{
	struct bass_cp_rem_src_t cp = {
		.opcode = BASS_OP_REM_SRC,
		.src_id = src_id
	};

	if (bass_rem_src(&cp)) {
		return 0;
	} else {
		return -EINVAL;
	}

	return 0;
}
