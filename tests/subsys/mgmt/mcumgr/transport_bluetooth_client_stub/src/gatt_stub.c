/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Stand-in for the Bluetooth host's GATT client API, following the callback order of
 * subsys/bluetooth/host/gatt.c:
 *
 *   bt_gatt_subscribe()   writes the CCC and links the node; a failed write links nothing.
 *   bt_gatt_unsubscribe() cancels an in-flight CCC write without a callback, writes the
 *                         CCC to zero and unlinks the node. notify(NULL) follows from the
 *                         write response.
 *   bt_gatt_cancel()      ends a pending request and runs its completion with an error.
 *   gatt_write_ccc_rsp()  runs the subscribe callback, then reports removal through
 *                         notify(NULL); an error response is dropped when the connection
 *                         has no subscriptions left.
 *   disconnect            fails pending ATT requests, sweeps volatile subscriptions with
 *                         notify(NULL), then runs the bt_conn_cb.disconnected callbacks.
 *
 * Responses are delivered from the system work queue, never from inside the call.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include "gatt_stub.h"

struct gatt_stub_peer_state gatt_stub_peer;
struct gatt_stub_call_log gatt_stub_log;
struct gatt_stub_fault_config gatt_stub_faults;

static const struct bt_uuid_128 stub_svc_uuid = BT_UUID_INIT_128(SMP_BT_SVC_UUID_VAL);
static const struct bt_uuid_128 stub_chr_uuid = BT_UUID_INIT_128(SMP_BT_CHR_UUID_VAL);

static uint16_t stub_mtu;

/* The host's subscription list, holding at most the transport's node */
static struct bt_gatt_subscribe_params *sub_node;
static struct bt_conn *sub_conn;

/* The outstanding ATT request of each kind */
static struct {
	struct bt_conn *conn;
	struct bt_gatt_discover_params *params;
	bool active;
} discover_req;

static struct {
	struct bt_conn *conn;
	struct bt_gatt_subscribe_params *params;
	uint8_t att_err;
	bool active;
} ccc_req;

static struct {
	struct bt_conn *conn;
	bt_gatt_complete_func_t func;
	void *user_data;
} tx_done[GATT_STUB_MAX_FRAGS];
static int tx_done_count;

static struct k_work_delayable discover_work;
static struct k_work_delayable ccc_work;

static void discover_deliver(void);
static void ccc_deliver(void);

static void discover_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	discover_deliver();
}

static void ccc_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	ccc_deliver();
}

void gatt_stub_reset(void)
{
	struct k_work_sync sync;
	static bool work_inited;

	if (!work_inited) {
		k_work_init_delayable(&discover_work, discover_work_handler);
		k_work_init_delayable(&ccc_work, ccc_work_handler);
		work_inited = true;
	}

	(void)k_work_cancel_delayable_sync(&discover_work, &sync);
	(void)k_work_cancel_delayable_sync(&ccc_work, &sync);

	gatt_stub_peer = (struct gatt_stub_peer_state){
		.svc_handle = 0x0010,
		.svc_end_handle = 0x0013,
		.chrc_handle = 0x0011,
		.value_handle = 0x0012,
		.ccc_handle = 0x0013,
		.properties = BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY,
		.has_svc = true,
		.has_chrc = true,
		.has_ccc = true,
	};

	memset(&gatt_stub_log, 0, sizeof(gatt_stub_log));
	memset(&gatt_stub_faults, 0, sizeof(gatt_stub_faults));
	/* Responses come from the work queue after a short delay by default */
	gatt_stub_faults.rsp_delay_ms = 5;

	memset(&discover_req, 0, sizeof(discover_req));
	memset(&ccc_req, 0, sizeof(ccc_req));
	tx_done_count = 0;
	sub_node = NULL;
	sub_conn = NULL;
	stub_mtu = 247;
}

void gatt_stub_set_mtu(uint16_t mtu)
{
	stub_mtu = mtu;
}

bool gatt_stub_is_subscribed(void)
{
	return sub_node != NULL;
}

const struct bt_gatt_subscribe_params *gatt_stub_sub_params(void)
{
	return sub_node;
}

bool gatt_stub_ccc_response_pending(void)
{
	return ccc_req.active;
}

bool gatt_stub_discovery_pending(void)
{
	return discover_req.active;
}

/* --- Connection references ------------------------------------------------------- */

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	conn->ref++;

	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	conn->ref--;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	memset(info, 0, sizeof(*info));
	info->state = conn->connected ? BT_CONN_STATE_CONNECTED : BT_CONN_STATE_DISCONNECTED;

	return 0;
}

/* --- ATT_MTU --------------------------------------------------------------------- */

uint16_t bt_gatt_get_mtu(struct bt_conn *conn)
{
	ARG_UNUSED(conn);

	return stub_mtu;
}

/* --- Discovery ------------------------------------------------------------------- */

/* The service and characteristic legs hand back the declaration attribute, as
 * gatt_find_type_rsp() and parse_characteristic() do.
 */
static void discover_deliver(void)
{
	static struct bt_gatt_service_val svc;
	static struct bt_gatt_chrc chrc;
	struct bt_gatt_discover_params *params = discover_req.params;
	struct bt_conn *conn = discover_req.conn;
	struct bt_gatt_attr attr;

	if (!discover_req.active) {
		return;
	}

	discover_req.active = false;
	memset(&attr, 0, sizeof(attr));

	switch (params->type) {
	case BT_GATT_DISCOVER_PRIMARY:
		if (!gatt_stub_peer.has_svc) {
			break;
		}

		svc.uuid = &stub_svc_uuid.uuid;
		svc.end_handle = gatt_stub_peer.svc_end_handle;
		attr.uuid = BT_UUID_GATT_PRIMARY;
		attr.handle = gatt_stub_peer.svc_handle;
		attr.user_data = &svc;
		(void)params->func(conn, &attr, params);
		return;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		if (!gatt_stub_peer.has_chrc) {
			break;
		}

		chrc.uuid = &stub_chr_uuid.uuid;
		chrc.value_handle = gatt_stub_peer.value_handle;
		chrc.properties = gatt_stub_peer.properties;
		attr.uuid = BT_UUID_GATT_CHRC;
		attr.handle = gatt_stub_peer.chrc_handle;
		attr.user_data = &chrc;
		(void)params->func(conn, &attr, params);
		return;
	case BT_GATT_DISCOVER_DESCRIPTOR:
		if (!gatt_stub_peer.has_ccc) {
			break;
		}

		attr.uuid = BT_UUID_GATT_CCC;
		attr.handle = gatt_stub_peer.ccc_handle;
		(void)params->func(conn, &attr, params);
		return;
	default:
		break;
	}

	/* Nothing matched: terminal callback with a NULL attribute */
	(void)params->func(conn, NULL, params);
}

int bt_gatt_discover(struct bt_conn *conn, struct bt_gatt_discover_params *params)
{
	if (gatt_stub_faults.discover_err != 0 &&
	    gatt_stub_log.discover_calls >= gatt_stub_faults.discover_err_after) {
		gatt_stub_log.discover_calls++;
		return gatt_stub_faults.discover_err;
	}

	gatt_stub_log.discover_calls++;

	if (!conn->connected) {
		return -ENOTCONN;
	}

	discover_req.conn = conn;
	discover_req.params = params;
	discover_req.active = true;

	if (gatt_stub_faults.rsp_delay_ms == 0) {
		discover_deliver();
	} else {
		(void)k_work_schedule(&discover_work, K_MSEC(gatt_stub_faults.rsp_delay_ms));
	}

	return 0;
}

/* --- Subscription ---------------------------------------------------------------- */

static void ccc_schedule(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 uint8_t att_err)
{
	ccc_req.conn = conn;
	ccc_req.params = params;
	ccc_req.att_err = att_err;
	ccc_req.active = true;

	if (gatt_stub_faults.defer_ccc_rsp) {
		return;
	}

	if (gatt_stub_faults.rsp_delay_ms == 0) {
		ccc_deliver();
	} else {
		(void)k_work_schedule(&ccc_work, K_MSEC(gatt_stub_faults.rsp_delay_ms));
	}
}

/* gatt_write_ccc_rsp(). */
static void ccc_deliver(void)
{
	struct bt_gatt_subscribe_params *params = ccc_req.params;
	struct bt_conn *conn = ccc_req.conn;
	uint8_t att_err = ccc_req.att_err;

	if (!ccc_req.active) {
		return;
	}

	ccc_req.active = false;
	/* gatt_write_ccc_rsp() clears this before anything else, whatever the outcome. */
	atomic_clear_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);

	if (att_err != 0U) {
		/* gatt_sub_find() finds nothing once the list is empty */
		if (sub_node == NULL || sub_conn != conn) {
			return;
		}

		if (params->subscribe != NULL) {
			params->subscribe(conn, att_err, params);
		}

		if (sub_node == params) {
			sub_node = NULL;
			params->notify(conn, params, NULL, 0);
		}

		return;
	}

	if (params->subscribe != NULL) {
		params->subscribe(conn, BT_ATT_ERR_SUCCESS, params);
	}

	if (params->value == 0U) {
		/* Completes an unsubscribe */
		params->notify(conn, params, NULL, 0);
	}
}

void gatt_stub_deliver_ccc_response(uint8_t att_err)
{
	ccc_req.att_err = att_err;
	ccc_deliver();
}

int bt_gatt_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params)
{
	gatt_stub_log.subscribe_calls++;

	if (!conn->connected) {
		return -ENOTCONN;
	}

	if (sub_node == params) {
		return -EALREADY;
	}

	if (gatt_stub_faults.subscribe_err != 0) {
		/* gatt_write_ccc() failed, so the node is never added to the list. */
		return gatt_stub_faults.subscribe_err;
	}

	sub_node = params;
	sub_conn = conn;

	if (gatt_stub_faults.subscribe_already_enabled) {
		/* No CCC write and no callback when another subscription covers the
		 * handle.
		 */
		return 0;
	}

	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);
	ccc_schedule(conn, params, gatt_stub_faults.ccc_att_err);

	return 0;
}

int bt_gatt_unsubscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params)
{
	gatt_stub_log.unsubscribe_calls++;

	if (!conn->connected) {
		return -ENOTCONN;
	}

	if (sub_node != params || sub_conn != conn) {
		return -EINVAL;
	}

	/* An in-flight CCC write is cancelled silently */
	if (ccc_req.active && ccc_req.params == params) {
		(void)k_work_cancel_delayable(&ccc_work);
		ccc_req.active = false;
		atomic_clear_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);
	}

	if (gatt_stub_faults.unsubscribe_err != 0) {
		/* gatt_write_ccc() failed: the node stays linked and no callback follows */
		return gatt_stub_faults.unsubscribe_err;
	}

	params->value = 0U;
	sub_node = NULL;

	if (gatt_stub_faults.subscribe_already_enabled) {
		/* No write; removal is reported synchronously */
		params->notify(conn, params, NULL, 0);
		return 0;
	}

	/* Unlinked now; the write response reports the removal */
	atomic_set_bit(params->flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING);
	ccc_schedule(conn, params, 0U);

	return 0;
}

void bt_gatt_cancel(struct bt_conn *conn, void *params)
{
	if (discover_req.active && discover_req.params == params) {
		struct bt_gatt_discover_params *discover = params;

		(void)k_work_cancel_delayable(&discover_work);
		discover_req.active = false;
		(void)discover->func(conn, NULL, discover);
	}

	if (ccc_req.active && ccc_req.params == params) {
		(void)k_work_cancel_delayable(&ccc_work);
		gatt_stub_deliver_ccc_response(BT_ATT_ERR_UNLIKELY);
	}
}

void gatt_stub_notify(const void *data, uint16_t len)
{
	if (sub_node != NULL) {
		(void)sub_node->notify(sub_conn, sub_node, data, len);
	}
}

/* --- Write Without Response ------------------------------------------------------ */

int bt_gatt_write_without_response_cb(struct bt_conn *conn, uint16_t handle, const void *data,
				      uint16_t length, bool sign, bt_gatt_complete_func_t func,
				      void *user_data)
{
	ARG_UNUSED(sign);

	if (!conn->connected) {
		return -ENOTCONN;
	}

	if (gatt_stub_faults.write_err != 0 &&
	    gatt_stub_log.write_calls >= gatt_stub_faults.write_err_after) {
		return gatt_stub_faults.write_err;
	}

	gatt_stub_log.write_calls++;
	gatt_stub_log.last_write_handle = handle;

	if (gatt_stub_log.frag_count < (int)ARRAY_SIZE(gatt_stub_log.frag_len)) {
		gatt_stub_log.frag_len[gatt_stub_log.frag_count] = length;
	}

	gatt_stub_log.frag_count++;

	if ((gatt_stub_log.stream_len + length) <= sizeof(gatt_stub_log.stream)) {
		memcpy(&gatt_stub_log.stream[gatt_stub_log.stream_len], data, length);
		gatt_stub_log.stream_len += length;
	}

	if (func == NULL) {
		return 0;
	}

	if (gatt_stub_faults.defer_tx_done) {
		if (tx_done_count < (int)ARRAY_SIZE(tx_done)) {
			tx_done[tx_done_count].conn = conn;
			tx_done[tx_done_count].func = func;
			tx_done[tx_done_count].user_data = user_data;
			tx_done_count++;
		}
	} else {
		func(conn, user_data);
	}

	return 0;
}

void gatt_stub_flush_tx_done(void)
{
	int count = tx_done_count;

	tx_done_count = 0;

	for (int i = 0; i < count; i++) {
		tx_done[i].func(tx_done[i].conn, tx_done[i].user_data);
	}
}

/* --- Disconnect ------------------------------------------------------------------ */

void gatt_stub_disconnect(struct bt_conn *conn)
{
	struct k_work_sync sync;

	conn->connected = false;

	/* Completion callbacks of queued writes never run */
	tx_done_count = 0;

	/* att_reset(): pending requests fail */
	(void)k_work_cancel_delayable_sync(&discover_work, &sync);
	(void)k_work_cancel_delayable_sync(&ccc_work, &sync);

	if (discover_req.active && discover_req.conn == conn) {
		struct bt_gatt_discover_params *params = discover_req.params;

		discover_req.active = false;
		(void)params->func(conn, NULL, params);
	}

	if (ccc_req.active && ccc_req.conn == conn) {
		gatt_stub_deliver_ccc_response(BT_ATT_ERR_UNLIKELY);
	}

	/* remove_subscriptions(): volatile subscriptions are removed */
	if (sub_node != NULL && sub_conn == conn) {
		struct bt_gatt_subscribe_params *params = sub_node;

		sub_node = NULL;
		params->value = 0U;
		(void)params->notify(conn, params, NULL, 0);
	}

	/* notify_disconnected() */
	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->disconnected != NULL) {
			cb->disconnected(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}
}
