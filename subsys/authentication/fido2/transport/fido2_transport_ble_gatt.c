/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include "fido2_transport_ble.h"

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define CTAPBLE_INITIAL_CID 1U

#define CTAPBLE_MAX_CONN_INTERVAL_MS 500U

#if defined(CONFIG_FIDO2_BLE_REQUIRE_AUTHENTICATED_LINK)
#define CTAPBLE_READ_PERM  (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_READ_AUTHEN)
#define CTAPBLE_WRITE_PERM (BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_WRITE_AUTHEN)
#define CTAPBLE_CCC_PERM                                                                           \
	(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_READ_AUTHEN |       \
	 BT_GATT_PERM_WRITE_AUTHEN)
#else
#define CTAPBLE_READ_PERM  BT_GATT_PERM_READ_ENCRYPT
#define CTAPBLE_WRITE_PERM BT_GATT_PERM_WRITE_ENCRYPT
#define CTAPBLE_CCC_PERM   (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT)
#endif

/**
 * @brief Runtime state for the CTAP Bluetooth LE GATT service.
 */
struct ctapble_gatt_context {
	/** Callbacks used to pass GATT events to the framing/transport layers. */
	struct ctapble_gatt_callbacks callbacks;
	/**
	 * Referenced active Bluetooth LE connection.
	 *
	 * Only one CTAP connection over Bluetooth LE is supported.
	 */
	struct bt_conn *conn;
	/** Transport connection identifier assigned to the active connection. */
	uint32_t cid;
	/** Next transport connection identifier to allocate. */
	uint32_t next_cid;
	/** Set while the GATT layer accepts CTAP connections over Bluetooth LE. */
	atomic_t initialized;
	/** Set when the client subscribed to Status notifications. */
	atomic_t notifications_enabled;
	/** Set after the client selected the supported FIDO service revision. */
	atomic_t revision_selected;
};

static struct ctapble_gatt_context gatt_ctx;
static K_MUTEX_DEFINE(conn_mutex);

static ssize_t control_point_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static ssize_t control_point_length_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len, uint16_t offset);
static ssize_t revision_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset);
static ssize_t revision_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static void status_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);

/*
 * CTAP Bluetooth LE service layout: Control Point receives fragments, Status sends notifications,
 * Control Point Length exposes the supported write size, and Revision negotiates the protocol.
 */
BT_GATT_SERVICE_DEFINE(ctapble_service, BT_GATT_PRIMARY_SERVICE(BT_UUID_FIDO2_SERVICE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_CONTROL_POINT, BT_GATT_CHRC_WRITE,
					      CTAPBLE_WRITE_PERM, NULL, control_point_write, NULL),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_STATUS, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(status_ccc_changed, CTAPBLE_CCC_PERM),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_CONTROL_POINT_LENGTH,
					      BT_GATT_CHRC_READ, CTAPBLE_READ_PERM,
					      control_point_length_read, NULL, NULL),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_REVISION_BITFIELD,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
					      CTAPBLE_READ_PERM | CTAPBLE_WRITE_PERM, revision_read,
					      revision_write, NULL));

static void mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)
{
	int ret;

	ret = k_mutex_lock(mutex, timeout);
	__ASSERT(ret == 0, "Failed to lock mutex: %d", ret);
	ARG_UNUSED(ret);
}

static void mutex_unlock(struct k_mutex *mutex)
{
	int ret;

	ret = k_mutex_unlock(mutex);
	__ASSERT(ret == 0, "Failed to unlock mutex: %d", ret);
	ARG_UNUSED(ret);
}

static struct bt_conn *conn_detach(struct bt_conn *conn, uint32_t *cid)
{
	struct bt_conn *detached = NULL;

	mutex_lock(&conn_mutex, K_FOREVER);

	/* Move ownership of the stored connection reference to the caller. */
	if ((gatt_ctx.conn != NULL) && ((conn == NULL) || (gatt_ctx.conn == conn))) {
		detached = bt_conn_take(&gatt_ctx.conn);

		if (cid != NULL) {
			*cid = gatt_ctx.cid;
		}

		gatt_ctx.cid = 0;
	}
	mutex_unlock(&conn_mutex);

	return detached;
}

static bool conn_access_allowed(struct bt_conn *conn)
{
	bool allowed;

	mutex_lock(&conn_mutex, K_FOREVER);

	allowed = atomic_get(&gatt_ctx.initialized) && (conn != NULL) &&
		  ((gatt_ctx.conn == NULL) || (gatt_ctx.conn == conn));

	mutex_unlock(&conn_mutex);

	return allowed;
}

static bool conn_claim(struct bt_conn *conn)
{
	bool claimed = false;

	mutex_lock(&conn_mutex, K_FOREVER);

	if (!atomic_get(&gatt_ctx.initialized) || (conn == NULL)) {
		goto out;
	}

	if (gatt_ctx.conn == NULL) {
		gatt_ctx.conn = bt_conn_ref(conn);
		gatt_ctx.cid = gatt_ctx.next_cid++;

		if (gatt_ctx.next_cid == 0) {
			gatt_ctx.next_cid = CTAPBLE_INITIAL_CID;
		}

		claimed = true;
	} else {
		claimed = (gatt_ctx.conn == conn);
	}

out:
	mutex_unlock(&conn_mutex);

	return claimed;
}

static bool conn_get_cid(struct bt_conn *conn, uint32_t *cid)
{
	bool current;

	mutex_lock(&conn_mutex, K_FOREVER);
	/* Protocol traffic is accepted only for the claimed FIDO connection. */
	current = atomic_get(&gatt_ctx.initialized) && (conn != NULL) && (gatt_ctx.conn == conn);
	if (current && (cid != NULL)) {
		*cid = gatt_ctx.cid;
	}
	mutex_unlock(&conn_mutex);

	return current;
}

bool ctapble_gatt_conn_is_ready(struct bt_conn *conn)
{
	return conn_get_cid(conn, NULL);
}

bool ctapble_gatt_conn_get_cid(struct bt_conn *conn, uint32_t *cid)
{
	if (cid == NULL) {
		return false;
	}

	return conn_get_cid(conn, cid);
}

bool ctapble_gatt_cid_is_current(uint32_t cid)
{
	bool current;

	mutex_lock(&conn_mutex, K_FOREVER);
	current = atomic_get(&gatt_ctx.initialized) && (gatt_ctx.conn != NULL) &&
		  (gatt_ctx.cid == cid);
	mutex_unlock(&conn_mutex);

	return current;
}

struct bt_conn *ctapble_gatt_conn_ref(uint32_t cid)
{
	struct bt_conn *conn = NULL;

	mutex_lock(&conn_mutex, K_FOREVER);
	if (atomic_get(&gatt_ctx.initialized) && (gatt_ctx.conn != NULL) && (gatt_ctx.cid == cid)) {
		conn = bt_conn_ref(gatt_ctx.conn);
	}
	mutex_unlock(&conn_mutex);

	return conn;
}

int ctapble_gatt_notify(struct bt_conn *conn, const void *data, uint16_t len,
			bt_gatt_complete_func_t func, void *user_data)
{
	const struct bt_gatt_attr *status_attr = &ctapble_service.attrs[CTAPBLE_ATTR_STATUS_VALUE];
	struct bt_gatt_notify_params params = {
		.attr = status_attr,
		.data = data,
		.len = len,
		.func = func,
		.user_data = user_data,
	};

	if (!ctapble_gatt_conn_is_ready(conn)) {
		return -ENOTCONN;
	}

	if (!atomic_get(&gatt_ctx.notifications_enabled)) {
		return -EACCES;
	}

	return bt_gatt_notify_cb(conn, &params);
}

static ssize_t control_point_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	int ret;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (!ctapble_gatt_conn_is_ready(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if ((len == 0) || (len > CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* CTAP Bluetooth LE requires revision selection and Status subscription before commands. */
	if (!atomic_get(&gatt_ctx.revision_selected)) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	if (!atomic_get(&gatt_ctx.notifications_enabled)) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	LOG_HEXDUMP_DBG(buf, len, "CTAP BLE control point");

	ret = gatt_ctx.callbacks.fragment_received(conn, buf, len);
	if (ret != 0) {
		LOG_WRN("Failed to queue CTAP BLE fragment: %d", ret);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return len;
}

static ssize_t control_point_length_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[2];

	if (!conn_access_allowed(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	/* The characteristic encodes the maximum Control Point write length in big endian. */
	sys_put_be16(CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH, value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

static ssize_t revision_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	static const uint8_t supported_revision = FIDO2_BLE_REVISION;

	if (!conn_access_allowed(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &supported_revision,
				 sizeof(supported_revision));
}

static ssize_t revision_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const struct bt_gatt_attr *status_attr = &ctapble_service.attrs[CTAPBLE_ATTR_STATUS_VALUE];
	const uint8_t *value = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (value[0] != FIDO2_BLE_REVISION) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (!conn_claim(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	/*
	 * The client may already have subscribed to Status before selecting
	 * the service revision.
	 */
	atomic_set(&gatt_ctx.notifications_enabled,
		   bt_gatt_is_subscribed(conn, status_attr, BT_GATT_CCC_NOTIFY));

	/* Control Point writes remain blocked until this negotiation completes. */
	atomic_set(&gatt_ctx.revision_selected, 1);

	return len;
}

static void status_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	const struct bt_gatt_attr *status_attr = &ctapble_service.attrs[CTAPBLE_ATTR_STATUS_VALUE];
	struct bt_conn *conn = NULL;
	uint32_t cid = 0;
	bool enabled;

	ARG_UNUSED(attr);
	ARG_UNUSED(value);

	mutex_lock(&conn_mutex, K_FOREVER);

	if (atomic_get(&gatt_ctx.initialized) && (gatt_ctx.conn != NULL)) {
		conn = bt_conn_ref(gatt_ctx.conn);
		cid = gatt_ctx.cid;
	}

	mutex_unlock(&conn_mutex);

	if (conn == NULL) {
		return;
	}

	enabled = bt_gatt_is_subscribed(conn, status_attr, BT_GATT_CCC_NOTIFY);
	atomic_set(&gatt_ctx.notifications_enabled, enabled);

	gatt_ctx.callbacks.notifications_changed(cid, enabled);

	bt_conn_unref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn *detached;
	uint32_t cid;

	/* Detach first so no subsequent lookup can treat this connection as current. */
	detached = conn_detach(conn, &cid);
	if (detached == NULL) {
		return;
	}

	atomic_clear(&gatt_ctx.notifications_enabled);
	atomic_clear(&gatt_ctx.revision_selected);

	LOG_INF("CTAP BLE link closed: 0x%02x", reason);

	if (atomic_get(&gatt_ctx.initialized)) {
		gatt_ctx.callbacks.disconnected(conn, cid);
	}

	bt_conn_unref(detached);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	uint32_t effective_interval;

	if (!ctapble_gatt_conn_is_ready(conn)) {
		return true;
	}

	effective_interval = (uint32_t)param->interval_max * (param->latency + 1U);

	/*
	 * Keep the effective interval below the FIDO BLE keepalive interval.
	 * Longer intervals may also violate the client's command fragment
	 * transmission deadline.
	 */
	if (effective_interval >= BT_GAP_MS_TO_CONN_INTERVAL(CTAPBLE_MAX_CONN_INTERVAL_MS)) {
		return false;
	}

	return true;
}

static void gatt_runtime_reset_locked(void)
{
	gatt_ctx.cid = 0;
	gatt_ctx.next_cid = CTAPBLE_INITIAL_CID;

	atomic_clear(&gatt_ctx.notifications_enabled);
	atomic_clear(&gatt_ctx.revision_selected);
}

int ctapble_gatt_init(const struct ctapble_gatt_callbacks *callbacks)
{

	if ((callbacks == NULL) || (callbacks->fragment_received == NULL) ||
	    (callbacks->disconnected == NULL) || (callbacks->notifications_changed == NULL)) {
		return -EINVAL;
	}

	mutex_lock(&conn_mutex, K_FOREVER);

	if (atomic_get(&gatt_ctx.initialized)) {
		mutex_unlock(&conn_mutex);
		return -EALREADY;
	}

	gatt_ctx.callbacks = *callbacks;

	/* Initialize the connection-specific protocol state. */
	gatt_runtime_reset_locked();

	atomic_set(&gatt_ctx.initialized, 1);

	mutex_unlock(&conn_mutex);

	return 0;
}

void ctapble_gatt_shutdown(void)
{
	struct bt_conn *conn;

	if (!atomic_get(&gatt_ctx.initialized)) {
		return;
	}

	/*
	 * Detach the current connection before resetting the
	 * connection-specific protocol state.
	 */
	conn = conn_detach(NULL, NULL);

	mutex_lock(&conn_mutex, K_FOREVER);

	gatt_runtime_reset_locked();

	mutex_unlock(&conn_mutex);

	if (conn != NULL) {
		bt_conn_unref(conn);
	}
}

BT_CONN_CB_DEFINE(ctapble_conn_callbacks) = {
	.disconnected = disconnected,
	.le_param_req = le_param_req,
};
