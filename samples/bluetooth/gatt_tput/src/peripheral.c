/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Peripheral role: GATT server exposing the custom "TPUT" throughput service
 * so a matching GATT client (see tput_client/) drives it unchanged.
 *
 * TX is dynamic:
 *   - payload size is chosen from the negotiated ATT MTU (3 tiers),
 *   - the number of notifications per connection event is derived from the live
 *     connection interval and PHY (airtime model), optionally throttled to a
 *     peer-requested rate,
 *   - notifications are pipelined with a fixed TX-credit budget and back off on
 *     congestion (-ENOMEM), resuming when a buffer frees.
 * RX has no special logic: written bytes are just accumulated for the report.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include "common.h"

#ifdef CONFIG_APP_ROLE_PERIPHERAL

LOG_MODULE_DECLARE(gatt_tput, LOG_LEVEL_INF);

/* Airtime model constants. */
#define TIFS_US        150U
#define US_PER_BYTE_2M 4U
#define US_PER_BYTE_1M 8U
#define PENDING_CAP    255U

static struct bt_uuid_128 meas_svc_uuid = BT_UUID_INIT_128(TPUT_MEAS_SVC_BYTES);
static struct bt_uuid_128 notify_uuid = BT_UUID_INIT_128(TPUT_NOTIFY_CHR_BYTES);
static struct bt_uuid_128 writeme_uuid = BT_UUID_INIT_128(TPUT_WRITEME_CHR_BYTES);
static struct bt_uuid_128 diag_svc_uuid = BT_UUID_INIT_128(TPUT_DIAG_SVC_BYTES);
static struct bt_uuid_128 throttle_uuid = BT_UUID_INIT_128(TPUT_THROTTLE_CHR_BYTES);

static struct bt_conn *default_conn;
static K_MUTEX_DEFINE(conn_lock); /* guards default_conn against the BT RX thread */
static volatile bool notify_enabled;

/* Live link state driving the pacing model. */
static uint16_t conn_interval; /* units of 1.25 ms */
static uint8_t tx_phy = 1U;    /* 1 or 2 (Mbit/s) */
static uint16_t packet_size = APP_DATA_PACKET_SIZE_1;
static uint8_t max_packets_per_event;
static uint8_t notifications_per_event;
static uint16_t target_kb_s; /* raw Throttle value in KB/s; 0 = max */

static atomic_t pending; /* notifications queued for this window */

static K_SEM_DEFINE(tx_credits, APP_TX_CREDITS, APP_TX_CREDITS);
static K_SEM_DEFINE(stream_wake, 0, 1);
static struct k_timer notif_timer;
static uint8_t stream_buf[APP_MAX_PAYLOAD];

/* 3-tier payload selection from the negotiated ATT MTU. */
static uint16_t notif_packet_size(uint16_t mtu)
{
	if (mtu < APP_DATA_PACKET_SIZE_1 + APP_ATT_HEADER_SIZE) {
		return mtu - APP_ATT_HEADER_SIZE;
	}

	if (mtu < APP_DATA_PACKET_SIZE_2 + APP_ATT_HEADER_SIZE) {
		return APP_DATA_PACKET_SIZE_1;
	}

	return APP_DATA_PACKET_SIZE_2;
}

/* Recompute max packets/event (airtime) and notifications/event (throttle). */
static void calc_pacing(void)
{
	if (default_conn == NULL || conn_interval == 0U) {
		max_packets_per_event = 0U;
		notifications_per_event = 0U;
		return;
	}

	uint32_t per_byte = (tx_phy == 2U) ? US_PER_BYTE_2M : US_PER_BYTE_1M;
	uint32_t ci_us = (uint32_t)conn_interval * 1250U;
	uint8_t preamble = (tx_phy == 2U) ? 2U : 1U;

	/* On-air bytes: preamble + AA(4) + LL(2) + L2CAP(4) + ATT hdr(3) + payload + CRC(3). */
	uint16_t notif_air =
		preamble + 4U + 2U + APP_L2CAP_HEADER_SIZE + APP_ATT_HEADER_SIZE + packet_size + 3U;
	uint16_t empty_air = preamble + 4U + 2U + 3U;

	uint32_t t_txn =
		TIFS_US + (uint32_t)notif_air * per_byte + TIFS_US + (uint32_t)empty_air * per_byte;

	max_packets_per_event = (uint8_t)(ci_us / t_txn);

	if (target_kb_s > 0U) {
		/* target is KB/s; x8 -> kbps. needed = kbps*interval*5 / (payload*32). */
		uint32_t kbps = (uint32_t)target_kb_s * 8U;
		uint32_t payload = (packet_size > 0U) ? packet_size : 1U;
		uint32_t num = kbps * (uint32_t)conn_interval * 5U;
		uint32_t den = payload * 32U;
		uint32_t needed = (num + den / 2U) / den;

		if (needed < 1U) {
			needed = 1U;
		}
		if (needed > max_packets_per_event) {
			needed = max_packets_per_event;
		}
		notifications_per_event = (uint8_t)needed;
	} else {
		notifications_per_event = max_packets_per_event;
	}

	LOG_INF("Pacing: max/evt=%u notif/evt=%u PHY=%uM interval=%u.%02ums payload=%u",
		max_packets_per_event, notifications_per_event, tx_phy,
		(conn_interval * 125U) / 100U, (conn_interval * 125U) % 100U, packet_size);
}

static void notif_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (!notify_enabled || default_conn == NULL) {
		return;
	}

	uint8_t add = (notifications_per_event != 0U) ? notifications_per_event : APP_TX_CREDITS;

	if ((uint32_t)atomic_add(&pending, add) + add > PENDING_CAP) {
		atomic_set(&pending, PENDING_CAP);
	}

	k_sem_give(&stream_wake);
}

static void timer_restart(void)
{
	uint32_t period_us = (conn_interval != 0U) ? (uint32_t)conn_interval * 1250U : 30000U;

	k_timer_start(&notif_timer, K_USEC(period_us), K_USEC(period_us));
}

static void notify_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Notifications %s", notify_enabled ? "enabled" : "disabled");

	if (notify_enabled) {
		atomic_set(&pending, 0);
		calc_pacing();
		timer_restart();
		k_sem_give(&stream_wake);
	} else {
		k_timer_stop(&notif_timer);
		atomic_set(&pending, 0);
	}
}

static ssize_t writeme_write(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(buf);
	ARG_UNUSED(offset);
	ARG_UNUSED(flags);

	/* RX: no special logic, just accumulate the byte count. */
	atomic_add(&app_rx_bytes, len);
	return len;
}

static ssize_t throttle_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	uint16_t val = target_kb_s;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &val, sizeof(val));
}

static ssize_t throttle_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *p = buf;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len == 1U) {
		target_kb_s = p[0];
	} else if (len == 2U) {
		target_kb_s = sys_get_le16(p);
	} else {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	LOG_INF("Throttle target = %u KB/s (0 = max)", target_kb_s);
	calc_pacing();
	return len;
}

BT_GATT_SERVICE_DEFINE(tput_meas_svc, BT_GATT_PRIMARY_SERVICE(&meas_svc_uuid),
		       BT_GATT_CHARACTERISTIC(&notify_uuid.uuid, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(notify_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		       BT_GATT_CHARACTERISTIC(&writeme_uuid.uuid,
					      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
					      BT_GATT_PERM_WRITE, NULL, writeme_write, NULL));

BT_GATT_SERVICE_DEFINE(diag_svc, BT_GATT_PRIMARY_SERVICE(&diag_svc_uuid),
		       BT_GATT_CHARACTERISTIC(&throttle_uuid.uuid,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, throttle_read,
					      throttle_write, NULL));

/* attrs[1] is the Notify characteristic declaration; bt_gatt_notify uses its value. */
#define NOTIFY_ATTR (&tput_meas_svc.attrs[1])

static void notify_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(user_data);

	/* Free one credit and nudge the stream thread (congestion recovery). */
	k_sem_give(&tx_credits);
	k_sem_give(&stream_wake);
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

static void stream_thread(void)
{
	while (1) {
		if (!notify_enabled || default_conn == NULL) {
			k_sem_take(&stream_wake, K_FOREVER);
			continue;
		}

		if (atomic_get(&pending) == 0) {
			k_sem_take(&stream_wake, K_MSEC(100));
			continue;
		}

		if (k_sem_take(&tx_credits, K_MSEC(100)) != 0) {
			continue;
		}

		struct bt_conn *conn = conn_get();

		if (!notify_enabled || conn == NULL || atomic_get(&pending) == 0) {
			if (conn != NULL) {
				bt_conn_unref(conn);
			}
			k_sem_give(&tx_credits);
			continue;
		}

		/* Never exceed the usable ATT MTU, whatever the tier picked. */
		uint16_t mtu = bt_gatt_get_mtu(conn);
		uint16_t max_payload =
			(mtu > APP_ATT_HEADER_SIZE) ? (mtu - APP_ATT_HEADER_SIZE) : 20U;
		uint16_t len = MIN(packet_size, max_payload);
		struct bt_gatt_notify_params params = {
			.attr = NOTIFY_ATTR,
			.data = stream_buf,
			.len = len,
			.func = notify_sent,
		};

		int err = bt_gatt_notify_cb(conn, &params);

		bt_conn_unref(conn);

		if (err) {
			/* -ENOMEM = congested: back off, retry on credit free or next tick. */
			k_sem_give(&tx_credits);
			if (err != -ENOMEM) {
				LOG_WRN("notify failed (%d)", err);
			}
			k_sem_take(&stream_wake, K_MSEC(10));
			continue;
		}

		atomic_dec(&pending);
		atomic_add(&app_tx_bytes, len);
	}
}

K_THREAD_DEFINE(stream_tid, 2048, stream_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, 0);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, "TPUT", 4),
};

static void adv_start(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		LOG_ERR("Advertising failed to start (%d)", err);
		return;
	}

	LOG_INF("Advertising as \"TPUT\"");
}

/* Restart advertising off the connection callback; the disconnected conn is not
 * freed yet when the callback runs, so a direct restart can return -ENOMEM.
 */
static void adv_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	adv_start();
}

static K_WORK_DEFINE(adv_work, adv_work_handler);

static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	ARG_UNUSED(conn);
	/* Effective ATT MTU is symmetric = min(tx, rx); use it to size notifications. */
	uint16_t mtu = MIN(tx, rx);

	packet_size = notif_packet_size(mtu);
	LOG_INF("ATT MTU updated: tx=%u rx=%u -> payload=%u", tx, rx, packet_size);
	calc_pacing();
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err) {
		LOG_ERR("Connection failed (0x%02x)", err);
		return;
	}

	k_mutex_lock(&conn_lock, K_FOREVER);
	default_conn = bt_conn_ref(conn);
	k_mutex_unlock(&conn_lock);
	LOG_INF("Connected");
	app_reset_totals();

	if (bt_conn_get_info(conn, &info) == 0 && info.type == BT_CONN_TYPE_LE) {
		conn_interval = (uint16_t)(info.le.interval_us / 1250U);
		tx_phy = (info.le.phy->tx_phy == BT_GAP_LE_PHY_2M) ? 2U : 1U;
	}

	packet_size = notif_packet_size(bt_gatt_get_mtu(conn));
	calc_pacing();
	app_link_optimize(conn);
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(latency);
	ARG_UNUSED(timeout);

	conn_interval = interval;
	calc_pacing();

	if (notify_enabled) {
		timer_restart();
	}
}

static void le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	ARG_UNUSED(conn);
	tx_phy = (param->tx_phy == BT_GAP_LE_PHY_2M) ? 2U : 1U;
	calc_pacing();
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	LOG_INF("Disconnected (0x%02x)", reason);

	notify_enabled = false;
	k_timer_stop(&notif_timer);
	atomic_set(&pending, 0);
	conn_interval = 0U;

	struct bt_conn *old;

	k_mutex_lock(&conn_lock, K_FOREVER);
	old = default_conn;
	default_conn = NULL;
	k_mutex_unlock(&conn_lock);
	if (old != NULL) {
		bt_conn_unref(old);
	}

	/* Reclaim credits held by in-flight notifications that won't complete now. */
	for (int i = 0; i < APP_TX_CREDITS; i++) {
		k_sem_give(&tx_credits);
	}

	k_work_submit(&adv_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated,
};

void app_peripheral_start(void)
{
	for (size_t i = 0; i < sizeof(stream_buf); i++) {
		stream_buf[i] = (uint8_t)i;
	}

	k_timer_init(&notif_timer, notif_timer_expiry, NULL);
	bt_gatt_cb_register(&gatt_callbacks);
	adv_start();
}

#endif /* CONFIG_APP_ROLE_PERIPHERAL */
