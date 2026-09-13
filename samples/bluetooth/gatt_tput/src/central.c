/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Central role: scans for "TPUT", connects, discovers the throughput service,
 * and streams in either/both directions on demand (driven by the tput shell).
 * Used for the two-board (central <-> peripheral) test.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include "common.h"

#ifdef CONFIG_APP_ROLE_CENTRAL

LOG_MODULE_DECLARE(gatt_tput, LOG_LEVEL_INF);

static struct bt_uuid_128 meas_svc_uuid = BT_UUID_INIT_128(TPUT_MEAS_SVC_BYTES);
static struct bt_uuid_128 notify_uuid = BT_UUID_INIT_128(TPUT_NOTIFY_CHR_BYTES);
static struct bt_uuid_128 writeme_uuid = BT_UUID_INIT_128(TPUT_WRITEME_CHR_BYTES);
static struct bt_uuid_128 diag_svc_uuid = BT_UUID_INIT_128(TPUT_DIAG_SVC_BYTES);
static struct bt_uuid_128 throttle_uuid = BT_UUID_INIT_128(TPUT_THROTTLE_CHR_BYTES);

static struct bt_conn *default_conn;
static K_MUTEX_DEFINE(conn_lock); /* guards default_conn against the BT RX thread */

static struct {
	uint16_t meas_start;
	uint16_t meas_end;
	uint16_t notify_value;
	uint16_t notify_ccc;
	uint16_t writeme_value;
	uint16_t diag_start;
	uint16_t diag_end;
	uint16_t throttle_value;
	bool ready;
} h;

enum discover_step {
	STEP_MEAS_SVC,
	STEP_NOTIFY_CHR,
	STEP_WRITEME_CHR,
	STEP_DIAG_SVC,
	STEP_THROTTLE_CHR,
	STEP_DONE,
};

static enum discover_step step;
static struct bt_gatt_discover_params disc;
static struct bt_gatt_subscribe_params sub;

static atomic_t write_on;
static K_SEM_DEFINE(wr_credits, APP_TX_CREDITS, APP_TX_CREDITS);
static uint8_t wr_buf[APP_MAX_PAYLOAD];

static void start_scan(void);
static void gatt_discover(void);

bool app_central_ready(void)
{
	return h.ready && default_conn != NULL;
}

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	ARG_UNUSED(conn);

	if (data == NULL) {
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	atomic_add(&app_rx_bytes, length);
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	if (attr == NULL) {
		LOG_WRN("Discovery step %d: attribute not found "
			"(meas %u-%u notify_val %u)",
			step, h.meas_start, h.meas_end, h.notify_value);
		return BT_GATT_ITER_STOP;
	}

	switch (step) {
	case STEP_MEAS_SVC: {
		struct bt_gatt_service_val *v = attr->user_data;

		h.meas_start = attr->handle + 1U;
		h.meas_end = v->end_handle;
		step = STEP_NOTIFY_CHR;
		break;
	}
	case STEP_NOTIFY_CHR:
		h.notify_value = bt_gatt_attr_value_handle(attr);
		/* CCC always follows the value in the peer's fixed layout; derive it
		 * instead of a descriptor discovery the AIROC peer fails to answer.
		 */
		h.notify_ccc = h.notify_value + 1U;
		step = STEP_WRITEME_CHR;
		break;
	case STEP_WRITEME_CHR:
		h.writeme_value = bt_gatt_attr_value_handle(attr);
		step = STEP_DIAG_SVC;
		break;
	case STEP_DIAG_SVC: {
		struct bt_gatt_service_val *v = attr->user_data;

		h.diag_start = attr->handle + 1U;
		h.diag_end = v->end_handle;
		step = STEP_THROTTLE_CHR;
		break;
	}
	case STEP_THROTTLE_CHR:
		h.throttle_value = bt_gatt_attr_value_handle(attr);
		step = STEP_DONE;
		break;
	default:
		return BT_GATT_ITER_STOP;
	}

	if (step == STEP_DONE) {
		h.ready = true;
		LOG_INF("Discovery complete - ready. Try: tput start both");
		return BT_GATT_ITER_STOP;
	}

	gatt_discover();
	return BT_GATT_ITER_STOP;
}

static void gatt_discover(void)
{
	int err;

	memset(&disc, 0, sizeof(disc));
	disc.func = discover_func;

	switch (step) {
	case STEP_MEAS_SVC:
		disc.uuid = &meas_svc_uuid.uuid;
		disc.type = BT_GATT_DISCOVER_PRIMARY;
		disc.start_handle = 0x0001;
		disc.end_handle = 0xffff;
		break;
	case STEP_NOTIFY_CHR:
		disc.uuid = &notify_uuid.uuid;
		disc.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		disc.start_handle = h.meas_start;
		disc.end_handle = h.meas_end;
		break;
	case STEP_WRITEME_CHR:
		disc.uuid = &writeme_uuid.uuid;
		disc.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		disc.start_handle = h.meas_start;
		disc.end_handle = h.meas_end;
		break;
	case STEP_DIAG_SVC:
		disc.uuid = &diag_svc_uuid.uuid;
		disc.type = BT_GATT_DISCOVER_PRIMARY;
		disc.start_handle = 0x0001;
		disc.end_handle = 0xffff;
		break;
	case STEP_THROTTLE_CHR:
		disc.uuid = &throttle_uuid.uuid;
		disc.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		disc.start_handle = h.diag_start;
		disc.end_handle = h.diag_end;
		break;
	default:
		return;
	}

	err = bt_gatt_discover(default_conn, &disc);
	if (err) {
		LOG_ERR("Discover (step %d) failed (%d)", step, err);
	}
}

void app_central_set_notify(bool on)
{
	int err;

	if (!app_central_ready()) {
		LOG_WRN("Not ready (no connection/discovery)");
		return;
	}

	if (on) {
		sub.notify = notify_func;
		sub.value_handle = h.notify_value;
		sub.ccc_handle = h.notify_ccc;
		sub.value = BT_GATT_CCC_NOTIFY;

		err = bt_gatt_subscribe(default_conn, &sub);
		if (err && err != -EALREADY) {
			LOG_ERR("Subscribe failed (%d)", err);
		} else {
			LOG_INF("Notify (RX) started");
		}
	} else {
		bt_gatt_unsubscribe(default_conn, &sub);
		LOG_INF("Notify (RX) stopped");
	}
}

void app_central_set_write(bool on)
{
	if (!app_central_ready()) {
		LOG_WRN("Not ready (no connection/discovery)");
		return;
	}

	atomic_set(&write_on, on ? 1 : 0);
	LOG_INF("Write (TX) %s", on ? "started" : "stopped");
}

void app_central_set_throttle(uint16_t kbps)
{
	uint8_t val[2];
	int err;

	if (!app_central_ready()) {
		LOG_WRN("Not ready (no connection/discovery)");
		return;
	}

	sys_put_le16(kbps, val);
	err = bt_gatt_write_without_response(default_conn, h.throttle_value, val, sizeof(val),
					     false);
	if (err) {
		LOG_ERR("Throttle write failed (%d)", err);
	} else {
		LOG_INF("Throttle set to %u kbps", kbps);
	}
}

static void write_done(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(user_data);
	k_sem_give(&wr_credits);
}

/* Take a reference to the live connection so it can't be freed while in use. */
static struct bt_conn *conn_get(void)
{
	struct bt_conn *conn = NULL;

	k_mutex_lock(&conn_lock, K_FOREVER);
	if (default_conn != NULL) {
		conn = bt_conn_ref(default_conn);
	}
	k_mutex_unlock(&conn_lock);
	return conn;
}

static void writer_thread(void)
{
	while (1) {
		if (!atomic_get(&write_on) || !app_central_ready()) {
			k_msleep(20);
			continue;
		}

		if (k_sem_take(&wr_credits, K_MSEC(100)) != 0) {
			continue;
		}

		struct bt_conn *conn = conn_get();

		if (conn == NULL) {
			k_sem_give(&wr_credits);
			k_msleep(1);
			continue;
		}

		uint16_t len = app_payload_len(conn);
		int err = bt_gatt_write_without_response_cb(conn, h.writeme_value, wr_buf, len,
							    false, write_done, NULL);

		bt_conn_unref(conn);

		if (err) {
			k_sem_give(&wr_credits);
			k_msleep(1);
			continue;
		}

		atomic_add(&app_tx_bytes, len);
	}
}

K_THREAD_DEFINE(writer_tid, 2048, writer_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, 0);

static bool ad_has_tput(struct bt_data *data, void *user_data)
{
	bool *match = user_data;

	if (data->type == BT_DATA_NAME_COMPLETE || data->type == BT_DATA_NAME_SHORTENED) {
		if (data->data_len == 4U && memcmp(data->data, "TPUT", 4) == 0) {
			*match = true;
			return false;
		}
	}

	return true;
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
	struct bt_conn *conn = NULL;
	bool match = false;
	int err;

	ARG_UNUSED(rssi);
	ARG_UNUSED(type);

	if (default_conn != NULL) {
		return;
	}

	bt_data_parse(ad, ad_has_tput, &match);
	if (!match) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM(24, 40, 0, 400),
				&conn);
	if (err) {
		LOG_ERR("Create connection failed (%d)", err);
		start_scan();
		return;
	}

	k_mutex_lock(&conn_lock, K_FOREVER);
	default_conn = conn;
	k_mutex_unlock(&conn_lock);
}

static void start_scan(void)
{
	int err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, scan_cb);

	if (err) {
		LOG_ERR("Scan start failed (%d)", err);
		return;
	}

	LOG_INF("Scanning for \"TPUT\"...");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		struct bt_conn *old;

		LOG_ERR("Failed to connect (0x%02x)", err);
		k_mutex_lock(&conn_lock, K_FOREVER);
		old = default_conn;
		default_conn = NULL;
		k_mutex_unlock(&conn_lock);
		if (old != NULL) {
			bt_conn_unref(old);
		}
		start_scan();
		return;
	}

	LOG_INF("Connected");

	memset(&h, 0, sizeof(h));
	step = STEP_MEAS_SVC;
	gatt_discover();
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	LOG_INF("Disconnected (0x%02x)", reason);

	h.ready = false;
	atomic_set(&write_on, 0);

	struct bt_conn *old;

	k_mutex_lock(&conn_lock, K_FOREVER);
	old = default_conn;
	default_conn = NULL;
	k_mutex_unlock(&conn_lock);
	if (old != NULL) {
		bt_conn_unref(old);
	}

	/* Reclaim credits held by in-flight writes that won't complete now. */
	for (int i = 0; i < APP_TX_CREDITS; i++) {
		k_sem_give(&wr_credits);
	}

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void app_central_start(void)
{
	memset(wr_buf, 0x5A, sizeof(wr_buf));
	start_scan();
}

#endif /* CONFIG_APP_ROLE_CENTRAL */
