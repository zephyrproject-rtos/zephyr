/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/authentication/fido2/fido2_transport.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "fido2_transport_ble.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define CTAPBLE_KEEPALIVE_PROCESSING 0x01U
#define CTAPBLE_KEEPALIVE_UP_NEEDED  0x02U

/**
 * @brief Runtime state for the FIDO2 Bluetooth LE transport.
 */
struct ctapble_transport_context {
	/** Callback used to deliver complete CTAP requests to the FIDO2 core. */
	fido2_transport_recv_cb_t recv_cb;
	/** Callback used to cancel the operation currently handled by the FIDO2 core. */
	fido2_transport_cancel_cb_t cancel_cb;
	/** Set while the transport is initialized and may accept work. */
	atomic_t initialized;
	/** Protects the active transaction state and its associated CID. */
	struct k_spinlock transaction_lock;
	/** Set while a CTAP transaction is owned by this transport. */
	bool transaction_active;
	/** CID of the active transaction, protected by @ref transaction_lock. */
	uint32_t transaction_cid;
	/** Periodic timer used to trigger keepalive notifications. */
	struct k_timer keepalive_timer;
	/** Work item that sends keepalive notifications outside timer context. */
	struct k_work keepalive_work;
	/** Set while periodic keepalive notifications are required. */
	atomic_t keepalive_active;
	/** CID associated with the current keepalive sequence. */
	atomic_t keepalive_cid;
	/** FIDO keepalive status byte to transmit over Bluetooth LE. */
	atomic_t keepalive_status;
};

static struct ctapble_transport_context transport_ctx;
static K_MUTEX_DEFINE(keepalive_mutex);

extern struct fido2_transport ctapble_transport;

static void mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
	__maybe_unused int ret;

	ret = k_mutex_lock(mutex, timeout);
	__ASSERT(ret == 0, "Failed to lock mutex: %d", ret);
}

static void mutex_unlock(struct k_mutex *mutex)
{
	__maybe_unused int ret;

	ret = k_mutex_unlock(mutex);
	__ASSERT(ret == 0, "Failed to unlock mutex: %d", ret);
}

static void keepalive_work_handler(struct k_work *work)
{
	struct bt_conn *conn;
	uint32_t cid;
	uint8_t status;
	int ret;

	ARG_UNUSED(work);

	/* Serialize keepalive state updates with the initial notification path. */
	mutex_lock(&keepalive_mutex, K_FOREVER);
	if (!atomic_get(&transport_ctx.keepalive_active)) {
		mutex_unlock(&keepalive_mutex);
		return;
	}

	cid = (uint32_t)atomic_get(&transport_ctx.keepalive_cid);
	conn = ctapble_gatt_conn_ref(cid);
	if (conn == NULL) {
		mutex_unlock(&keepalive_mutex);
		return;
	}

	status = (uint8_t)atomic_get(&transport_ctx.keepalive_status);
	ret = ctapble_framing_send(conn, CTAPBLE_CMD_KEEPALIVE, &status, sizeof(status));
	mutex_unlock(&keepalive_mutex);
	bt_conn_unref(conn);

	if ((ret != 0) && (ret != -ENOTCONN) && (ret != -EACCES) && (ret != -ESHUTDOWN)) {
		LOG_WRN("Failed to queue Bluetooth LE KEEPALIVE: %d", ret);
	}
}

static void keepalive_timer_handler(struct k_timer *timer)
{
	int ret;

	ARG_UNUSED(timer);

	/* GATT notifications are sent from work context, not from the timer callback. */
	if (atomic_get(&transport_ctx.keepalive_active)) {
		ret = k_work_submit(&transport_ctx.keepalive_work);
		if (ret < 0) {
			LOG_WRN("Failed to submit keepalive work: %d", ret);
		}
	}
}

static void keepalive_stop(uint32_t cid)
{
	bool stop;

	mutex_lock(&keepalive_mutex, K_FOREVER);
	stop = (cid == 0) || ((uint32_t)atomic_get(&transport_ctx.keepalive_cid) == cid);
	if (stop) {
		atomic_clear(&transport_ctx.keepalive_active);
		k_timer_stop(&transport_ctx.keepalive_timer);
	}
	mutex_unlock(&keepalive_mutex);
}

static int transaction_claim(uint32_t cid)
{
	k_spinlock_key_t key;
	int ret = 0;

	key = k_spin_lock(&transport_ctx.transaction_lock);

	if (!atomic_get(&transport_ctx.initialized)) {
		ret = -ESHUTDOWN;
	} else if (transport_ctx.transaction_active) {
		ret = -EBUSY;
	} else {
		transport_ctx.transaction_active = true;
		transport_ctx.transaction_cid = cid;
	}

	k_spin_unlock(&transport_ctx.transaction_lock, key);

	return ret;
}

static bool transaction_matches(uint32_t cid)
{
	k_spinlock_key_t key;
	bool matches;

	key = k_spin_lock(&transport_ctx.transaction_lock);

	matches = transport_ctx.transaction_active && transport_ctx.transaction_cid == cid;

	k_spin_unlock(&transport_ctx.transaction_lock, key);

	return matches;
}

static bool transaction_release(uint32_t cid)
{
	k_spinlock_key_t key;
	bool released = false;

	key = k_spin_lock(&transport_ctx.transaction_lock);

	if (transport_ctx.transaction_active && transport_ctx.transaction_cid == cid) {
		transport_ctx.transaction_active = false;
		transport_ctx.transaction_cid = 0;
		released = true;
	}

	k_spin_unlock(&transport_ctx.transaction_lock, key);

	return released;
}

static bool transaction_detach(uint32_t *cid)
{
	k_spinlock_key_t key;
	bool active;

	key = k_spin_lock(&transport_ctx.transaction_lock);

	active = transport_ctx.transaction_active;
	if (active) {
		*cid = transport_ctx.transaction_cid;
		transport_ctx.transaction_active = false;
		transport_ctx.transaction_cid = 0;
	}

	k_spin_unlock(&transport_ctx.transaction_lock, key);

	return active;
}

static bool transaction_is_active(void)
{
	k_spinlock_key_t key;
	bool active;

	key = k_spin_lock(&transport_ctx.transaction_lock);
	active = transport_ctx.transaction_active;
	k_spin_unlock(&transport_ctx.transaction_lock, key);

	return active;
}

static int message_received(struct bt_conn *conn, const uint8_t *data, size_t len)
{
	uint32_t cid;
	int ret;

	if (!atomic_get(&transport_ctx.initialized) || (transport_ctx.recv_cb == NULL)) {
		return -ESHUTDOWN;
	}

	if (!ctapble_gatt_conn_get_cid(conn, &cid)) {
		return -ENOTCONN;
	}

	/* Only one CTAP transaction can be owned by the shared Bluetooth LE transport at a time. */
	ret = transaction_claim(cid);
	if (ret != 0) {
		return ret;
	}

	LOG_HEXDUMP_DBG(data, len, "Complete CTAP message received over Bluetooth LE");
	ret = transport_ctx.recv_cb(&ctapble_transport, cid, data, len);
	if (ret != 0) {
		transaction_release(cid);
		return ret;
	}

	return 0;
}

static void cancel_received(struct bt_conn *conn)
{
	uint32_t cid;

	if (!atomic_get(&transport_ctx.initialized)) {
		return;
	}

	if (!ctapble_gatt_conn_get_cid(conn, &cid)) {
		return;
	}

	if (transport_ctx.cancel_cb != NULL && transaction_matches(cid)) {
		transport_ctx.cancel_cb(&ctapble_transport);
	}
}

static void connection_disconnected(struct bt_conn *conn, uint32_t cid)
{
	fido2_transport_cancel_cb_t cancel_cb;
	bool active;

	keepalive_stop(cid);
	ctapble_framing_connection_closed(conn);

	cancel_cb = transport_ctx.cancel_cb;
	active = transaction_release(cid);

	if (active && cancel_cb != NULL) {
		cancel_cb(&ctapble_transport);
	}
}

static void notifications_changed(uint32_t cid, bool enabled)
{
	if (!enabled) {
		keepalive_stop(cid);
	}
}

static const struct ctapble_framing_callbacks framing_callbacks = {
	.message_received = message_received,
	.cancel_received = cancel_received,
	.transaction_active = transaction_is_active,
};
static const struct ctapble_gatt_callbacks gatt_callbacks = {
	.fragment_received = ctapble_framing_submit_fragment,
	.disconnected = connection_disconnected,
	.notifications_changed = notifications_changed,
};

static int bluetooth_le_init(fido2_transport_recv_cb_t recv_cb,
			     fido2_transport_cancel_cb_t cancel_cb)
{
	int ret;

	if ((recv_cb == NULL) || (cancel_cb == NULL)) {
		return -EINVAL;
	}

	if (atomic_get(&transport_ctx.initialized)) {
		return -EALREADY;
	}

	transport_ctx.recv_cb = recv_cb;
	transport_ctx.cancel_cb = cancel_cb;
	transport_ctx.transaction_active = false;
	transport_ctx.transaction_cid = 0;
	atomic_clear(&transport_ctx.keepalive_active);
	atomic_clear(&transport_ctx.keepalive_cid);
	k_work_init(&transport_ctx.keepalive_work, keepalive_work_handler);
	k_timer_init(&transport_ctx.keepalive_timer, keepalive_timer_handler, NULL);

	/* Framing is initialized before GATT so incoming writes always have a consumer. */
	ret = ctapble_framing_init(&framing_callbacks);
	if (ret != 0) {
		goto fail;
	}

	atomic_set(&transport_ctx.initialized, 1);
	ret = ctapble_gatt_init(&gatt_callbacks);
	if (ret != 0) {
		atomic_clear(&transport_ctx.initialized);
		ctapble_framing_shutdown();
		goto fail;
	}

	LOG_INF("CTAP transport over Bluetooth LE initialized");

	return 0;

fail:
	transport_ctx.recv_cb = NULL;
	transport_ctx.cancel_cb = NULL;
	return ret;
}

static int bluetooth_le_send(uint32_t cid, const uint8_t *data, size_t len)
{
	struct bt_conn *conn;
	int ret;

	if ((data == NULL) || (len == 0)) {
		transaction_release(cid);
		return -EINVAL;
	}

	if (!atomic_get(&transport_ctx.initialized)) {
		transaction_release(cid);
		return -ESHUTDOWN;
	}

	conn = ctapble_gatt_conn_ref(cid);
	if (conn == NULL) {
		transaction_release(cid);
		return -ENOTCONN;
	}

	/* The final MSG response terminates the keepalive sequence for this transaction. */
	keepalive_stop(cid);
	ctapble_framing_purge_keepalives(conn);

	ret = ctapble_framing_send(conn, CTAPBLE_CMD_MSG, data, len);
	bt_conn_unref(conn);

	transaction_release(cid);

	return ret;
}

static void bluetooth_le_notify(uint32_t cid, enum fido2_wire_status status)
{
	struct bt_conn *conn;
	uint8_t status_byte;
	int ret;

	if (!atomic_get(&transport_ctx.initialized) || !ctapble_gatt_cid_is_current(cid)) {
		return;
	}

	if (status == FIDO2_WIRE_STATUS_DONE) {
		keepalive_stop(cid);
		return;
	}

	switch (status) {
	case FIDO2_WIRE_STATUS_UP_NEEDED:
		status_byte = CTAPBLE_KEEPALIVE_UP_NEEDED;
		break;
	case FIDO2_WIRE_STATUS_PROCESSING:
		status_byte = CTAPBLE_KEEPALIVE_PROCESSING;
		break;
	default:
		LOG_WRN("Ignoring unknown FIDO2 wire status for Bluetooth LE: %d", status);
		return;
	}

	conn = ctapble_gatt_conn_ref(cid);
	if (conn == NULL) {
		return;
	}

	/* Send one status immediately, then repeat it at the FIDO Bluetooth LE keepalive interval.
	 */
	mutex_lock(&keepalive_mutex, K_FOREVER);
	atomic_set(&transport_ctx.keepalive_cid, (atomic_val_t)cid);
	atomic_set(&transport_ctx.keepalive_status, status_byte);
	atomic_set(&transport_ctx.keepalive_active, 1);
	k_timer_start(&transport_ctx.keepalive_timer,
		      K_MSEC(CONFIG_FIDO2_BLE_KEEPALIVE_INTERVAL_MS),
		      K_MSEC(CONFIG_FIDO2_BLE_KEEPALIVE_INTERVAL_MS));
	ret = ctapble_framing_send(conn, CTAPBLE_CMD_KEEPALIVE, &status_byte, sizeof(status_byte));
	mutex_unlock(&keepalive_mutex);
	bt_conn_unref(conn);

	if (ret != 0) {
		LOG_WRN("Failed to queue initial Bluetooth LE KEEPALIVE: %d", ret);
	}
}

static void bluetooth_le_shutdown(void)
{
	fido2_transport_cancel_cb_t cancel_cb;
	uint32_t cid = 0;
	bool active;

	if (!atomic_get(&transport_ctx.initialized)) {
		return;
	}

	/* Stop current transport activity and clean up the active transaction. */
	keepalive_stop(0);

	cancel_cb = transport_ctx.cancel_cb;
	active = transaction_detach(&cid);

	atomic_clear(&transport_ctx.keepalive_cid);

	if (active && (cancel_cb != NULL)) {
		cancel_cb(&ctapble_transport);
	}

	ctapble_framing_shutdown();
	ctapble_gatt_shutdown();

	LOG_INF("CTAP transport over Bluetooth LE shut down");
}

static const struct fido2_transport_api ble_api = {
	.init = bluetooth_le_init,
	.send = bluetooth_le_send,
	.notify = bluetooth_le_notify,
	.shutdown = bluetooth_le_shutdown,
};

FIDO2_TRANSPORT_DEFINE(ctapble_transport, "Bluetooth LE", &ble_api);
