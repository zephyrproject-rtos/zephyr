/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/authentication/fido2/fido2_transport.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/authentication/fido2/fido2_ble_internal.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define FIDO2_BLE_KEEPALIVE_INTERVAL_MS 500U
#define FIDO2_BLE_KEEPALIVE_PROCESSING  0x01U
#define FIDO2_BLE_KEEPALIVE_UP_NEEDED   0x02U

/**
 * @brief Runtime state for the FIDO2 BLE transport.
 */
struct fido2_ble_transport_context {
	/** Callback used to deliver complete CTAP requests to the FIDO2 core. */
	fido2_transport_recv_cb_t recv_cb;
	/** Callback used to cancel the operation currently handled by the FIDO2 core. */
	fido2_transport_cancel_cb_t cancel_cb;
	/** Set while the transport is initialized and may accept work. */
	atomic_t initialized;
	/** Set from request dispatch until the core completes the transaction. */
	atomic_t transaction_active;
	/** Periodic timer used to trigger keepalive notifications. */
	struct k_timer keepalive_timer;
	/** Work item that sends keepalive notifications outside timer context. */
	struct k_work keepalive_work;
	/** Set while periodic keepalive notifications are required. */
	atomic_t keepalive_active;
	/** CID associated with the current keepalive sequence. */
	atomic_t keepalive_cid;
	/** FIDO BLE keepalive status byte to transmit. */
	atomic_t keepalive_status;
};

static struct fido2_ble_transport_context transport_ctx;
static K_MUTEX_DEFINE(keepalive_mutex);

extern struct fido2_transport ble_transport;

static void keepalive_work_handler(struct k_work *work)
{
	struct bt_conn *conn;
	uint32_t cid;
	uint8_t status;
	int err;

	ARG_UNUSED(work);

	/* Serialize keepalive state updates with the initial notification path. */
	k_mutex_lock(&keepalive_mutex, K_FOREVER);
	if (!atomic_get(&transport_ctx.keepalive_active)) {
		k_mutex_unlock(&keepalive_mutex);
		return;
	}

	cid = (uint32_t)atomic_get(&transport_ctx.keepalive_cid);
	conn = fido2_ble_gatt_conn_ref(cid);
	if (conn == NULL) {
		k_mutex_unlock(&keepalive_mutex);
		return;
	}

	status = (uint8_t)atomic_get(&transport_ctx.keepalive_status);
	err = fido2_ble_framing_send(conn, FIDO2_BLE_CMD_KEEPALIVE, &status, sizeof(status));
	k_mutex_unlock(&keepalive_mutex);
	bt_conn_unref(conn);

	if ((err != 0) && (err != -ENOTCONN) && (err != -EACCES) && (err != -ESHUTDOWN)) {
		LOG_WRN("Failed to queue FIDO BLE KEEPALIVE: %d", err);
	}
}

static void keepalive_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	/* GATT notifications are sent from work context, not from the timer callback. */
	if (atomic_get(&transport_ctx.keepalive_active)) {
		(void)k_work_submit(&transport_ctx.keepalive_work);
	}
}

static void keepalive_stop(uint32_t cid)
{
	bool stop;

	k_mutex_lock(&keepalive_mutex, K_FOREVER);
	stop = (cid == 0U) || ((uint32_t)atomic_get(&transport_ctx.keepalive_cid) == cid);
	if (stop) {
		atomic_clear(&transport_ctx.keepalive_active);
		k_timer_stop(&transport_ctx.keepalive_timer);
	}
	k_mutex_unlock(&keepalive_mutex);
}

static int message_received(struct bt_conn *conn, const uint8_t *data, size_t len)
{
	uint32_t cid;

	if (!atomic_get(&transport_ctx.initialized) || (transport_ctx.recv_cb == NULL)) {
		return -ESHUTDOWN;
	}

	if (!fido2_ble_gatt_conn_get_cid(conn, &cid)) {
		return -ENOTCONN;
	}

	/* Only one CTAP transaction can be owned by the shared BLE transport at a time. */
	if (!atomic_cas(&transport_ctx.transaction_active, 0, 1)) {
		return -EBUSY;
	}

	LOG_HEXDUMP_DBG(data, len, "FIDO BLE complete MSG");
	transport_ctx.recv_cb(&ble_transport, cid, data, len);

	return 0;
}

static void cancel_received(void)
{
	if (atomic_get(&transport_ctx.initialized) && (transport_ctx.cancel_cb != NULL)) {
		transport_ctx.cancel_cb();
	}
}

static bool transaction_active(void)
{
	return atomic_get(&transport_ctx.transaction_active);
}

static void connection_disconnected(struct bt_conn *conn, uint32_t cid)
{
	bool active;

	/* A disconnect invalidates both transport framing and any pending core operation. */
	keepalive_stop(cid);
	fido2_ble_framing_connection_closed(conn);
	active = atomic_cas(&transport_ctx.transaction_active, 1, 0);
	if (active && (transport_ctx.cancel_cb != NULL)) {
		transport_ctx.cancel_cb();
	}
}

static void notifications_changed(uint32_t cid, bool enabled)
{
	if (!enabled) {
		keepalive_stop(cid);
	}
}

static int fido2_ble_init(fido2_transport_recv_cb_t recv_cb, fido2_transport_cancel_cb_t cancel_cb)
{
	static const struct fido2_ble_framing_callbacks framing_callbacks = {
		.message_received = message_received,
		.cancel_received = cancel_received,
		.transaction_active = transaction_active,
	};
	static const struct fido2_ble_gatt_callbacks gatt_callbacks = {
		.fragment_received = fido2_ble_framing_submit_fragment,
		.disconnected = connection_disconnected,
		.notifications_changed = notifications_changed,
	};
	int err;

	if ((recv_cb == NULL) || (cancel_cb == NULL)) {
		return -EINVAL;
	}

	if (atomic_get(&transport_ctx.initialized)) {
		return -EALREADY;
	}

	transport_ctx.recv_cb = recv_cb;
	transport_ctx.cancel_cb = cancel_cb;
	atomic_clear(&transport_ctx.transaction_active);
	atomic_clear(&transport_ctx.keepalive_active);
	atomic_clear(&transport_ctx.keepalive_cid);
	k_work_init(&transport_ctx.keepalive_work, keepalive_work_handler);
	k_timer_init(&transport_ctx.keepalive_timer, keepalive_timer_handler, NULL);

	/* Framing is initialized before GATT so incoming writes always have a consumer. */
	err = fido2_ble_framing_init(&framing_callbacks);
	if (err != 0) {
		goto fail;
	}

	atomic_set(&transport_ctx.initialized, 1);
	err = fido2_ble_gatt_init(&gatt_callbacks);
	if (err != 0) {
		atomic_clear(&transport_ctx.initialized);
		fido2_ble_framing_shutdown();
		goto fail;
	}

	LOG_INF("FIDO BLE transport initialized");

	return 0;

fail:
	transport_ctx.recv_cb = NULL;
	transport_ctx.cancel_cb = NULL;
	return err;
}

static int fido2_ble_send(uint32_t cid, const uint8_t *data, size_t len)
{
	struct bt_conn *conn;
	int err;

	if ((data == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	if (!atomic_get(&transport_ctx.initialized)) {
		return -ESHUTDOWN;
	}

	conn = fido2_ble_gatt_conn_ref(cid);
	if (conn == NULL) {
		return -ENOTCONN;
	}

	/* The final MSG response terminates the keepalive sequence for this transaction. */
	keepalive_stop(cid);
	err = fido2_ble_framing_send(conn, FIDO2_BLE_CMD_MSG, data, len);
	bt_conn_unref(conn);

	if (fido2_ble_gatt_cid_is_current(cid)) {
		atomic_clear(&transport_ctx.transaction_active);
	}

	return err;
}

static void fido2_ble_notify(uint32_t cid, enum fido2_wire_status status)
{
	struct bt_conn *conn;
	uint8_t status_byte;
	int err;

	if (!atomic_get(&transport_ctx.initialized) || !fido2_ble_gatt_cid_is_current(cid)) {
		return;
	}

	if (status == FIDO2_WIRE_STATUS_DONE) {
		keepalive_stop(cid);
		return;
	}

	switch (status) {
	case FIDO2_WIRE_STATUS_UP_NEEDED:
		status_byte = FIDO2_BLE_KEEPALIVE_UP_NEEDED;
		break;
	case FIDO2_WIRE_STATUS_PROCESSING:
		status_byte = FIDO2_BLE_KEEPALIVE_PROCESSING;
		break;
	default:
		LOG_WRN("Ignoring unknown FIDO BLE wire status: %d", status);
		return;
	}

	conn = fido2_ble_gatt_conn_ref(cid);
	if (conn == NULL) {
		return;
	}

	/* Send one status immediately, then repeat it at the FIDO BLE keepalive interval. */
	k_mutex_lock(&keepalive_mutex, K_FOREVER);
	atomic_set(&transport_ctx.keepalive_cid, (atomic_val_t)cid);
	atomic_set(&transport_ctx.keepalive_status, status_byte);
	atomic_set(&transport_ctx.keepalive_active, 1);
	k_timer_start(&transport_ctx.keepalive_timer, K_MSEC(FIDO2_BLE_KEEPALIVE_INTERVAL_MS),
		      K_MSEC(FIDO2_BLE_KEEPALIVE_INTERVAL_MS));
	err = fido2_ble_framing_send(conn, FIDO2_BLE_CMD_KEEPALIVE, &status_byte,
				     sizeof(status_byte));
	k_mutex_unlock(&keepalive_mutex);
	bt_conn_unref(conn);

	if (err != 0) {
		LOG_WRN("Failed to queue initial FIDO BLE KEEPALIVE: %d", err);
	}
}

static void fido2_ble_shutdown(void)
{
	fido2_transport_cancel_cb_t cancel_cb;
	bool active;

	if (!atomic_cas(&transport_ctx.initialized, 1, 0)) {
		return;
	}

	/* Stop producers before clearing callbacks or releasing an active transaction. */
	keepalive_stop(0U);
	fido2_ble_gatt_shutdown();
	fido2_ble_framing_shutdown();

	cancel_cb = transport_ctx.cancel_cb;
	active = atomic_cas(&transport_ctx.transaction_active, 1, 0);
	transport_ctx.recv_cb = NULL;
	transport_ctx.cancel_cb = NULL;
	atomic_clear(&transport_ctx.keepalive_cid);

	if (active && (cancel_cb != NULL)) {
		cancel_cb();
	}

	LOG_INF("FIDO BLE transport shut down");
}

static const struct fido2_transport_api ble_api = {
	.init = fido2_ble_init,
	.send = fido2_ble_send,
	.notify = fido2_ble_notify,
	.shutdown = fido2_ble_shutdown,
};

FIDO2_TRANSPORT_DEFINE(ble_transport, "BLE", &ble_api);
