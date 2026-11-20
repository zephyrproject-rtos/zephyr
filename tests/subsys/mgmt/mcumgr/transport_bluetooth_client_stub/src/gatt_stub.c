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
 *                         has no subscriptions left, and runs no notify(NULL) for a node
 *                         that is no longer linked.
 *   disconnect            fails pending ATT requests, drops the completions of the link's
 *                         writes, sweeps volatile subscriptions with notify(NULL), then runs
 *                         the bt_conn_cb.disconnected callbacks.
 *   bt_conn_le_create()   returns a connecting link; the connection complete runs the
 *                         bt_conn_cb.connected callbacks, with an error if it failed.
 *   bt_conn_disconnect()  cancels the creation of a connecting link, whose connection
 *                         complete then reports an error, or terminates an established
 *                         one, which later runs the disconnect sequence.
 *
 * Responses are delivered from the system work queue, or from inside the call when the
 * response delay is zero.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include "gatt_stub.h"

struct gatt_stub_peer_state gatt_stub_peer;
struct gatt_stub_call_log gatt_stub_log;
struct gatt_stub_fault_config gatt_stub_faults;
struct bt_conn gatt_stub_created_conn;

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

/* A CCC write response whose subscribe callback is still to run, see ccc_cb_delay_ms */
static struct {
	struct bt_conn *conn;
	struct bt_gatt_subscribe_params *params;
	uint8_t att_err;
	bool active;
} ccc_late;

static struct {
	struct bt_conn *conn;
	bt_gatt_complete_func_t func;
	void *user_data;
} tx_done[GATT_STUB_MAX_FRAGS];
static int tx_done_count;

static struct k_work_delayable discover_work;
static struct k_work_delayable ccc_work;
static struct k_work_delayable ccc_late_work;
static struct k_work_delayable connect_work;
static struct k_work_delayable terminate_work;

/* The HCI error the pending connection complete carries */
static uint8_t connect_status;
/* The created link is connected and its connected callbacks have not run */
static bool connect_callbacks_pending;
/* The link whose termination is pending */
static struct bt_conn *terminate_conn;

static void discover_deliver(void);
static void ccc_deliver(void);
static void ccc_late_deliver(void);

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

static void ccc_late_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	ccc_late_deliver();
}

static void connect_deliver(void);

static void connect_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	connect_deliver();
}

static void terminate_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	gatt_stub_complete_disconnect();
}

void gatt_stub_reset(void)
{
	struct k_work_sync sync;
	static bool work_inited;

	if (!work_inited) {
		k_work_init_delayable(&discover_work, discover_work_handler);
		k_work_init_delayable(&ccc_work, ccc_work_handler);
		k_work_init_delayable(&ccc_late_work, ccc_late_work_handler);
		k_work_init_delayable(&connect_work, connect_work_handler);
		k_work_init_delayable(&terminate_work, terminate_work_handler);
		work_inited = true;
	}

	(void)k_work_cancel_delayable_sync(&discover_work, &sync);
	(void)k_work_cancel_delayable_sync(&ccc_work, &sync);
	(void)k_work_cancel_delayable_sync(&ccc_late_work, &sync);
	(void)k_work_cancel_delayable_sync(&connect_work, &sync);
	(void)k_work_cancel_delayable_sync(&terminate_work, &sync);

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
	memset(&ccc_late, 0, sizeof(ccc_late));
	tx_done_count = 0;
	sub_node = NULL;
	sub_conn = NULL;
	stub_mtu = 247;
	connect_status = 0U;
	connect_callbacks_pending = false;
	terminate_conn = NULL;
	memset(&gatt_stub_created_conn, 0, sizeof(gatt_stub_created_conn));
	/* The links of the tests use index 0 */
	gatt_stub_created_conn.index = 1U;
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

void gatt_stub_deliver_discovery(void)
{
	discover_deliver();
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

uint8_t bt_conn_index(const struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn->index < CONFIG_BT_MAX_CONN);

	return conn->index;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	memset(info, 0, sizeof(*info));

	if (conn->connecting) {
		info->state = BT_CONN_STATE_CONNECTING;
	} else if (conn->connected) {
		info->state =
			conn->disconnecting ? BT_CONN_STATE_DISCONNECTING : BT_CONN_STATE_CONNECTED;
	} else {
		info->state = BT_CONN_STATE_DISCONNECTED;
	}

	return 0;
}

struct bt_conn_tmp_str bt_conn_dst_tmp_str(const struct bt_conn *conn)
{
	struct bt_conn_tmp_str str = {.str = "stub peer"};

	ARG_UNUSED(conn);

	return str;
}

/* As in subsys/bluetooth/host/addr.c, which is not built without CONFIG_BT */
int bt_addr_from_str(const char *str, bt_addr_t *addr)
{
	if (strlen(str) != (BT_ADDR_STR_LEN - 1)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(addr->val); i++) {
		if (i > 0 && str[(i * 3) - 1] != ':') {
			return -EINVAL;
		}

		if (hex2bin(&str[i * 3], 2, &addr->val[ARRAY_SIZE(addr->val) - 1 - i], 1) != 1) {
			return -EINVAL;
		}
	}

	return 0;
}

/* --- Connection creation and termination ------------------------------------------ */

/* hci_le_conn_complete(): the connection complete event of a link this device created */
static void connect_deliver(void)
{
	struct bt_conn *conn = &gatt_stub_created_conn;
	uint8_t err = connect_status;
	bool deliver;

	if (connect_callbacks_pending) {
		/* Unless the link has ended since */
		connect_callbacks_pending = false;
		deliver = conn->connected;
	} else if (conn->connecting) {
		conn->connecting = false;
		conn->connected = (err == 0U);
		deliver = true;
	} else {
		deliver = false;
	}

	if (!deliver) {
		return;
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->connected != NULL) {
			cb->connected(conn, err);
		}
	}
}

static void connect_schedule(uint8_t err)
{
	connect_status = err;

	if (gatt_stub_faults.rsp_delay_ms == 0) {
		connect_deliver();
	} else {
		(void)k_work_schedule(&connect_work, K_MSEC(gatt_stub_faults.rsp_delay_ms));
	}
}

void gatt_stub_complete_connection(uint8_t err)
{
	connect_status = err;
	connect_deliver();
}

int bt_conn_le_create(const bt_addr_le_t *peer, const struct bt_conn_le_create_param *create_param,
		      const struct bt_le_conn_param *conn_param, struct bt_conn **conn)
{
	struct bt_conn *created = &gatt_stub_created_conn;

	ARG_UNUSED(conn_param);

	gatt_stub_log.create_calls++;
	bt_addr_le_copy(&gatt_stub_log.create_addr, peer);
	gatt_stub_log.create_options = create_param->options;

	if (gatt_stub_faults.create_err != 0) {
		return gatt_stub_faults.create_err;
	}

	if (created->connecting || created->connected) {
		return -EINVAL;
	}

	created->disconnecting = false;
	*conn = bt_conn_ref(created);

	if (gatt_stub_faults.connect_state_early) {
		created->connected = true;
		connect_callbacks_pending = true;
		return 0;
	}

	created->connecting = true;

	if (!gatt_stub_faults.defer_connect) {
		connect_schedule(gatt_stub_faults.connect_err);
	}

	return 0;
}

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	gatt_stub_log.disconnect_calls++;

	if (conn->connecting) {
		/* bt_le_create_conn_cancel(): the connection complete follows, with an error
		 * unless the link was established before the cancel reached the controller.
		 */
		(void)k_work_cancel_delayable(&connect_work);
		connect_schedule(gatt_stub_faults.cancel_too_late ? 0U
								  : BT_HCI_ERR_UNKNOWN_CONN_ID);
		return 0;
	}

	if (!conn->connected) {
		return -ENOTCONN;
	}

	if (conn->disconnecting) {
		return 0;
	}

	if (gatt_stub_faults.disconnect_err != 0) {
		return gatt_stub_faults.disconnect_err;
	}

	conn->disconnecting = true;
	terminate_conn = conn;

	if (!gatt_stub_faults.defer_disconnect) {
		(void)k_work_schedule(&terminate_work,
				      K_MSEC(MAX(gatt_stub_faults.rsp_delay_ms, 1)));
	}

	return 0;
}

void gatt_stub_complete_disconnect(void)
{
	struct bt_conn *conn = terminate_conn;

	terminate_conn = NULL;

	if (conn != NULL && conn->connected) {
		gatt_stub_disconnect(conn);
	}
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

	if (gatt_stub_faults.defer_discovery) {
		return 0;
	}

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

/* The callbacks of gatt_write_ccc_rsp() */
static void ccc_complete(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 uint8_t att_err)
{
	if (att_err != 0U) {
		/* gatt_sub_find() finds nothing once the list is empty */
		if ((sub_node == NULL && !gatt_stub_faults.other_subscriptions) ||
		    sub_conn != conn) {
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

static void ccc_late_deliver(void)
{
	if (!ccc_late.active) {
		return;
	}

	ccc_late.active = false;
	ccc_complete(ccc_late.conn, ccc_late.params, ccc_late.att_err);
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

	if (gatt_stub_faults.ccc_cb_delay_ms > 0) {
		ccc_late.conn = conn;
		ccc_late.params = params;
		ccc_late.att_err = att_err;
		ccc_late.active = true;
		(void)k_work_schedule(&ccc_late_work, K_MSEC(gatt_stub_faults.ccc_cb_delay_ms));
		return;
	}

	ccc_complete(conn, params, att_err);
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
		/* gatt_write_ccc() failed, so the node is never added to the list */
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
	}

	if (gatt_stub_faults.unsubscribe_err != 0) {
		/* gatt_write_ccc() failed: the node stays linked and no callback follows */
		params->value = 0U;
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
	ccc_schedule(conn, params, 0U);

	return 0;
}

void bt_gatt_cancel(struct bt_conn *conn, void *params)
{
	if (discover_req.active && discover_req.params == params &&
	    !gatt_stub_faults.discovery_cancel_misses) {
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
	int kept = 0;

	conn->connected = false;
	conn->connecting = false;
	conn->disconnecting = false;

	if (conn == &gatt_stub_created_conn) {
		connect_callbacks_pending = false;
	}

	if (terminate_conn == conn) {
		terminate_conn = NULL;
	}

	/* Completion callbacks of the link's queued writes never run */
	for (int i = 0; i < tx_done_count; i++) {
		if (tx_done[i].conn != conn) {
			tx_done[kept] = tx_done[i];
			kept++;
		}
	}

	tx_done_count = kept;

	/* A CCC write response already received runs its callback */
	(void)k_work_cancel_delayable_sync(&ccc_late_work, &sync);
	ccc_late_deliver();

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

void gatt_stub_drop_link(struct bt_conn *conn)
{
	conn->connected = false;
}
