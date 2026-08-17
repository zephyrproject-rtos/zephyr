/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/authentication/fido2/fido2_transport_ble.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/authentication/fido2/fido2_ble_internal.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define FIDO2_BLE_INITIAL_CID 1U

#if defined(CONFIG_FIDO2_BLE_REQUIRE_AUTHENTICATED_LINK)
#define FIDO2_BLE_REQUIRED_SECURITY BT_SECURITY_L3
#define FIDO2_BLE_READ_PERM         (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_READ_AUTHEN)
#define FIDO2_BLE_WRITE_PERM        (BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_WRITE_AUTHEN)
#define FIDO2_BLE_CCC_PERM                                                                         \
	(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_READ_AUTHEN |       \
	 BT_GATT_PERM_WRITE_AUTHEN)
#else
#define FIDO2_BLE_REQUIRED_SECURITY BT_SECURITY_L2
#define FIDO2_BLE_READ_PERM         BT_GATT_PERM_READ_ENCRYPT
#define FIDO2_BLE_WRITE_PERM        BT_GATT_PERM_WRITE_ENCRYPT
#define FIDO2_BLE_CCC_PERM          (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT)
#endif

/**
 * @brief Attribute indexes within the statically defined FIDO BLE GATT service.
 */
enum fido2_ble_gatt_attr_index {
	/** Primary service declaration. */
	FIDO2_BLE_ATTR_SERVICE,
	/** Control Point characteristic declaration. */
	FIDO2_BLE_ATTR_CONTROL_POINT_CHRC,
	/** Control Point characteristic value. */
	FIDO2_BLE_ATTR_CONTROL_POINT_VALUE,
	/** Status characteristic declaration. */
	FIDO2_BLE_ATTR_STATUS_CHRC,
	/** Status characteristic value. */
	FIDO2_BLE_ATTR_STATUS_VALUE,
	/** Status Client Characteristic Configuration descriptor. */
	FIDO2_BLE_ATTR_STATUS_CCC,
	/** Control Point Length characteristic declaration. */
	FIDO2_BLE_ATTR_CONTROL_POINT_LENGTH_CHRC,
	/** Control Point Length characteristic value. */
	FIDO2_BLE_ATTR_CONTROL_POINT_LENGTH_VALUE,
	/** Service Revision Bitfield characteristic declaration. */
	FIDO2_BLE_ATTR_REVISION_CHRC,
	/** Service Revision Bitfield characteristic value. */
	FIDO2_BLE_ATTR_REVISION_VALUE,
};

/**
 * @brief Runtime state for the FIDO BLE GATT service.
 */
struct fido2_ble_gatt_context {
	/** Callbacks used to pass GATT events to the framing/transport layers. */
	struct fido2_ble_gatt_callbacks callbacks;
	/** Referenced active BLE connection. Only one FIDO BLE connection is supported. */
	struct bt_conn *conn;
	/** Transport connection identifier assigned to the active connection. */
	uint32_t cid;
	/** Next transport connection identifier to allocate. */
	uint32_t next_cid;
	/** Set while the GATT layer accepts FIDO BLE connections and operations. */
	atomic_t initialized;
	/** Set when the client subscribed to Status notifications. */
	atomic_t notifications_enabled;
	/** Set after the client selected the supported FIDO BLE revision. */
	atomic_t revision_selected;
	/** Set after the connection reached the required Bluetooth security level. */
	atomic_t security_ready;
};

static struct fido2_ble_gatt_context gatt_ctx;
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
 * FIDO BLE service layout: Control Point receives fragments, Status sends notifications,
 * Control Point Length exposes the supported write size, and Revision negotiates the protocol.
 */
BT_GATT_SERVICE_DEFINE(fido2_ble_service, BT_GATT_PRIMARY_SERVICE(BT_UUID_FIDO2_SERVICE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_CONTROL_POINT, BT_GATT_CHRC_WRITE,
					      FIDO2_BLE_WRITE_PERM, NULL, control_point_write,
					      NULL),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_STATUS, BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_NONE, NULL, NULL, NULL),
		       BT_GATT_CCC(status_ccc_changed, FIDO2_BLE_CCC_PERM),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_CONTROL_POINT_LENGTH,
					      BT_GATT_CHRC_READ, FIDO2_BLE_READ_PERM,
					      control_point_length_read, NULL, NULL),
		       BT_GATT_CHARACTERISTIC(BT_UUID_FIDO2_BLE_REVISION_BITFIELD,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
					      FIDO2_BLE_READ_PERM | FIDO2_BLE_WRITE_PERM,
					      revision_read, revision_write, NULL));

static struct bt_conn *conn_detach(struct bt_conn *conn, uint32_t *cid)
{
	struct bt_conn *detached = NULL;

	k_mutex_lock(&conn_mutex, K_FOREVER);

	/* Move ownership of the stored connection reference to the caller. */
	if ((gatt_ctx.conn != NULL) && ((conn == NULL) || (gatt_ctx.conn == conn))) {
		detached = gatt_ctx.conn;
		gatt_ctx.conn = NULL;
		if (cid != NULL) {
			*cid = gatt_ctx.cid;
		}
		gatt_ctx.cid = 0U;
	}

	k_mutex_unlock(&conn_mutex);

	return detached;
}

static bool conn_get_cid(struct bt_conn *conn, uint32_t *cid, bool require_security)
{
	bool current;

	k_mutex_lock(&conn_mutex, K_FOREVER);
	/* Protocol traffic is accepted only for the tracked connection after security setup. */
	current = atomic_get(&gatt_ctx.initialized) && (conn != NULL) && (gatt_ctx.conn == conn) &&
		  (!require_security || atomic_get(&gatt_ctx.security_ready));
	if (current && (cid != NULL)) {
		*cid = gatt_ctx.cid;
	}
	k_mutex_unlock(&conn_mutex);

	return current;
}

static bool current_cid_get(uint32_t *cid)
{
	bool current;

	k_mutex_lock(&conn_mutex, K_FOREVER);
	current = atomic_get(&gatt_ctx.initialized) && (gatt_ctx.conn != NULL);
	if (current) {
		*cid = gatt_ctx.cid;
	}
	k_mutex_unlock(&conn_mutex);

	return current;
}

bool fido2_ble_gatt_conn_is_ready(struct bt_conn *conn)
{
	return conn_get_cid(conn, NULL, true);
}

bool fido2_ble_gatt_conn_get_cid(struct bt_conn *conn, uint32_t *cid)
{
	if (cid == NULL) {
		return false;
	}

	return conn_get_cid(conn, cid, true);
}

bool fido2_ble_gatt_cid_is_current(uint32_t cid)
{
	bool current;

	k_mutex_lock(&conn_mutex, K_FOREVER);
	current = atomic_get(&gatt_ctx.initialized) && (gatt_ctx.conn != NULL) &&
		  atomic_get(&gatt_ctx.security_ready) && (gatt_ctx.cid == cid);
	k_mutex_unlock(&conn_mutex);

	return current;
}

struct bt_conn *fido2_ble_gatt_conn_ref(uint32_t cid)
{
	struct bt_conn *conn = NULL;

	k_mutex_lock(&conn_mutex, K_FOREVER);
	if (atomic_get(&gatt_ctx.initialized) && (gatt_ctx.conn != NULL) &&
	    atomic_get(&gatt_ctx.security_ready) && (gatt_ctx.cid == cid)) {
		conn = bt_conn_ref(gatt_ctx.conn);
	}
	k_mutex_unlock(&conn_mutex);

	return conn;
}

int fido2_ble_gatt_notify(struct bt_conn *conn, const void *data, uint16_t len,
			  bt_gatt_complete_func_t func, void *user_data)
{
	const struct bt_gatt_attr *status_attr =
		&fido2_ble_service.attrs[FIDO2_BLE_ATTR_STATUS_VALUE];
	struct bt_gatt_notify_params params = {
		.attr = status_attr,
		.data = data,
		.len = len,
		.func = func,
		.user_data = user_data,
	};

	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return -ENOTCONN;
	}

	if (!atomic_get(&gatt_ctx.notifications_enabled) ||
	    !bt_gatt_is_subscribed(conn, status_attr, BT_GATT_CCC_NOTIFY)) {
		return -EACCES;
	}

	return bt_gatt_notify_cb(conn, &params);
}

static ssize_t control_point_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	int err;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if ((len == 0U) || (len > CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* FIDO BLE requires revision selection and Status subscription before commands. */
	if (!atomic_get(&gatt_ctx.revision_selected)) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	if (!atomic_get(&gatt_ctx.notifications_enabled)) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	LOG_HEXDUMP_DBG(buf, len, "FIDO BLE control point");

	err = gatt_ctx.callbacks.fragment_received(conn, buf, len);
	if (err != 0) {
		LOG_WRN("Failed to queue FIDO BLE fragment: %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return len;
}

static ssize_t control_point_length_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[2];

	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	/* The characteristic encodes the maximum Control Point write length in big endian. */
	sys_put_be16(CONFIG_FIDO2_BLE_CONTROL_POINT_LENGTH, value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

static ssize_t revision_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	static const uint8_t supported_revision = FIDO2_BLE_REVISION;

	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &supported_revision,
				 sizeof(supported_revision));
}

static ssize_t revision_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *value = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (!fido2_ble_gatt_conn_is_ready(conn)) {
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != 1U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (value[0] != FIDO2_BLE_REVISION) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	/* Control Point writes remain blocked until this negotiation completes. */
	atomic_set(&gatt_ctx.revision_selected, 1);

	return len;
}

static void status_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	uint32_t cid;
	bool enabled;

	ARG_UNUSED(attr);

	/* Keep the transport informed so it can stop keepalives when notifications are disabled. */
	enabled = (value == BT_GATT_CCC_NOTIFY);
	atomic_set(&gatt_ctx.notifications_enabled, enabled);

	if (current_cid_get(&cid)) {
		gatt_ctx.callbacks.notifications_changed(cid, enabled);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn *detached;
	bool inactive;
	bool reject;
	int security_err;

	if ((err != 0U) || !bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		return;
	}

	/* The framing layer has one global RX/TX context, so only one BLE client is admitted. */
	k_mutex_lock(&conn_mutex, K_FOREVER);
	inactive = !atomic_get(&gatt_ctx.initialized);
	reject = (gatt_ctx.conn != NULL);
	if (!inactive && !reject) {
		gatt_ctx.conn = bt_conn_ref(conn);
		gatt_ctx.cid = gatt_ctx.next_cid++;
		if (gatt_ctx.next_cid == 0U) {
			gatt_ctx.next_cid = FIDO2_BLE_INITIAL_CID;
		}
	}
	k_mutex_unlock(&conn_mutex);

	if (inactive) {
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	if (reject) {
		LOG_WRN("Second FIDO BLE connection rejected");
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	atomic_clear(&gatt_ctx.notifications_enabled);
	atomic_clear(&gatt_ctx.revision_selected);
	atomic_clear(&gatt_ctx.security_ready);

	/* FIDO BLE protocol access is gated until the required link security is established. */
	security_err = bt_conn_set_security(conn, FIDO2_BLE_REQUIRED_SECURITY);
	if ((security_err != 0) && (security_err != -EALREADY)) {
		LOG_ERR("Failed to request FIDO BLE security level %u: %d",
			(unsigned int)FIDO2_BLE_REQUIRED_SECURITY, security_err);
		detached = conn_detach(conn, NULL);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		if (detached != NULL) {
			bt_conn_unref(detached);
		}
		return;
	}

	if (bt_conn_get_security(conn) >= FIDO2_BLE_REQUIRED_SECURITY) {
		atomic_set(&gatt_ctx.security_ready, 1);
	}

	LOG_INF("FIDO BLE link established; security level %u requested",
		(unsigned int)FIDO2_BLE_REQUIRED_SECURITY);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (!conn_get_cid(conn, NULL, false)) {
		return;
	}

	if ((err != BT_SECURITY_ERR_SUCCESS) || (level < FIDO2_BLE_REQUIRED_SECURITY)) {
		LOG_ERR("FIDO BLE security failed (level %u, error %u)", (unsigned int)level,
			(unsigned int)err);
		atomic_clear(&gatt_ctx.security_ready);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		return;
	}

	atomic_set(&gatt_ctx.security_ready, 1);
	LOG_INF("FIDO BLE security level %u established", (unsigned int)level);
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
	atomic_clear(&gatt_ctx.security_ready);

	LOG_INF("FIDO BLE link closed: 0x%02x", reason);

	if (atomic_get(&gatt_ctx.initialized)) {
		gatt_ctx.callbacks.disconnected(conn, cid);
	}

	bt_conn_unref(detached);
}

int fido2_ble_gatt_init(const struct fido2_ble_gatt_callbacks *callbacks)
{
	if ((callbacks == NULL) || (callbacks->fragment_received == NULL) ||
	    (callbacks->disconnected == NULL) || (callbacks->notifications_changed == NULL)) {
		return -EINVAL;
	}

	k_mutex_lock(&conn_mutex, K_FOREVER);
	if (atomic_get(&gatt_ctx.initialized)) {
		k_mutex_unlock(&conn_mutex);
		return -EALREADY;
	}

	/* Connection-specific protocol state is reset for the next admitted client. */
	gatt_ctx.callbacks = *callbacks;
	gatt_ctx.cid = 0U;
	gatt_ctx.next_cid = FIDO2_BLE_INITIAL_CID;
	atomic_clear(&gatt_ctx.notifications_enabled);
	atomic_clear(&gatt_ctx.revision_selected);
	atomic_clear(&gatt_ctx.security_ready);
	atomic_set(&gatt_ctx.initialized, 1);
	k_mutex_unlock(&conn_mutex);

	return 0;
}

void fido2_ble_gatt_shutdown(void)
{
	struct bt_conn *conn;

	if (!atomic_cas(&gatt_ctx.initialized, 1, 0)) {
		return;
	}

	conn = conn_detach(NULL, NULL);
	atomic_clear(&gatt_ctx.notifications_enabled);
	atomic_clear(&gatt_ctx.revision_selected);
	atomic_clear(&gatt_ctx.security_ready);
	memset(&gatt_ctx.callbacks, 0, sizeof(gatt_ctx.callbacks));

	if (conn != NULL) {
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		bt_conn_unref(conn);
	}
}

BT_CONN_CB_DEFINE(fido2_ble_conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};
