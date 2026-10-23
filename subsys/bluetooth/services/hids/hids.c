/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "hids_internal.h"

LOG_MODULE_REGISTER(bt_hids, CONFIG_BT_HIDS_LOG_LEVEL);

BUILD_ASSERT(HIDS_REPORT_COUNT > 0, "The HID Service needs at least one Report characteristic");

/* The HID Service sets no security requirements of its own; a profile built
 * on it picks them, see CONFIG_BT_HIDS_SECURITY. HIDS_SEC_LEVEL is the
 * security level a link needs for the permissions to be met.
 */
#if defined(CONFIG_BT_HIDS_SECURITY_AUTHEN)
#define HIDS_PERM_READ  BT_GATT_PERM_READ_AUTHEN
#define HIDS_PERM_WRITE BT_GATT_PERM_WRITE_AUTHEN
#define HIDS_SEC_LEVEL  BT_SECURITY_L3
#elif defined(CONFIG_BT_HIDS_SECURITY_ENCRYPT)
#define HIDS_PERM_READ  BT_GATT_PERM_READ_ENCRYPT
#define HIDS_PERM_WRITE BT_GATT_PERM_WRITE_ENCRYPT
#define HIDS_SEC_LEVEL  BT_SECURITY_L2
#else
#define HIDS_PERM_READ  BT_GATT_PERM_READ
#define HIDS_PERM_WRITE BT_GATT_PERM_WRITE
#define HIDS_SEC_LEVEL  BT_SECURITY_L1
#endif
#define HIDS_PERM_RW (HIDS_PERM_READ | HIDS_PERM_WRITE)

/* Encrypting an LE link takes an entry of the key pool, bonded or not, and
 * the entry is held until the link is gone, so no more than
 * CONFIG_BT_MAX_PAIRED links reach a security level above L1 at once.
 */
#if defined(CONFIG_BT_HIDS_SECURITY_NONE)
#define HIDS_MAX_INSTS CONFIG_BT_MAX_CONN
#else
#define HIDS_MAX_INSTS MIN(CONFIG_BT_MAX_CONN, CONFIG_BT_MAX_PAIRED)
#endif

BUILD_ASSERT(sizeof(struct bt_hids_boot_kb_in_report) == 8U,
	     "Boot Keyboard Input Report layout must match the USB HID Specification");
BUILD_ASSERT(sizeof(struct bt_hids_boot_kb_out_report) == 1U,
	     "Boot Keyboard Output Report layout must match the USB HID Specification");
BUILD_ASSERT(sizeof(struct bt_hids_boot_mouse_in_report) == 3U,
	     "Boot Mouse Input Report layout must match the USB HID Specification");

/* Module context.
 *
 * The GATT handlers run on the thread the Bluetooth stack processes ATT
 * requests on, and the API on application threads. hids_mutex protects every
 * field below: the registration state, the callbacks, the Report Map, the HID
 * Information, the Report and Boot Report contexts and the per-connection
 * slots. The slots are
 * claimed and released on the Bluetooth thread, and read from application
 * threads. It is never held while an application
 * callback runs or while a notification is sent, so that a callback can call
 * the API and a notification waiting for a buffer does not hold up the
 * handlers.
 */
static K_MUTEX_DEFINE(hids_mutex);
static const struct bt_hids_cb *callbacks;
static struct hids_state hids;
static struct hids_conn connections[HIDS_MAX_INSTS];

/* Report characteristic contexts, laid out Input, then Output, then Feature, in
 * the same order as the Report characteristics appear in the service. The
 * BUILD_ASSERT above guarantees the array is not empty.
 */
static struct hids_report_ctx reports[HIDS_REPORT_COUNT];

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD) || defined(CONFIG_BT_HIDS_BOOT_MOUSE)
/* Boot Report characteristic contexts, indexed by enum bt_hids_boot_report. All
 * three slots exist whenever any Boot Report does, so that the index of a
 * context does not depend on the configuration. Only the slots of the
 * characteristics that are part of the service are reachable.
 */
static struct hids_boot_report_ctx boot_reports[] = {
	[BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT] = {.len = sizeof(struct bt_hids_boot_kb_in_report)},
	[BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT] = {.len = sizeof(struct bt_hids_boot_kb_out_report)},
	[BT_HIDS_BOOT_REPORT_MOUSE_INPUT] = {.len = sizeof(struct bt_hids_boot_mouse_in_report)},
};

BUILD_ASSERT(sizeof(struct bt_hids_boot_kb_out_report) <= HIDS_BOOT_REPORT_MAX_LEN &&
		     sizeof(struct bt_hids_boot_mouse_in_report) <= HIDS_BOOT_REPORT_MAX_LEN,
	     "A Boot Report is longer than the Boot Report buffer");
#endif /* CONFIG_BT_HIDS_BOOT_KEYBOARD || CONFIG_BT_HIDS_BOOT_MOUSE */

static void hids_lock(void)
{
	__maybe_unused int err = k_mutex_lock(&hids_mutex, K_FOREVER);

	__ASSERT(err == 0, "Failed to lock the HID Service (err %d)", err);
}

static void hids_unlock(void)
{
	__maybe_unused int err = k_mutex_unlock(&hids_mutex);

	__ASSERT(err == 0, "Failed to unlock the HID Service (err %d)", err);
}

/* Assert that the current thread holds hids_mutex. k_mutex resets the owner on
 * the final unlock, so the owner check alone is exact.
 */
#define HIDS_LOCK_ASSERT()                                                                         \
	__ASSERT(hids_mutex.owner == k_current_get(), "hids_mutex not held by %p", k_current_get())

/* Return the module to its unregistered state */
static void hids_reset(void)
{
	HIDS_LOCK_ASSERT();

	callbacks = NULL;
	(void)memset(&hids, 0, sizeof(hids));
	(void)memset(reports, 0, sizeof(reports));

	/* The GATT service is gone, so the per-Host HID state goes with it */
	(void)memset(connections, 0, sizeof(connections));

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD) || defined(CONFIG_BT_HIDS_BOOT_MOUSE)
	/* The length is fixed and the attribute does not move, so only the
	 * value goes
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(boot_reports); i++) {
		(void)memset(boot_reports[i].value, 0, sizeof(boot_reports[i].value));
	}
#endif
}

/* Resolve a Boot Report of the registered service. A Boot Report the
 * configuration leaves out has no attribute bound to its context.
 */
static int boot_report_resolve(enum bt_hids_boot_report report, struct hids_boot_report_ctx **ctx)
{
	HIDS_LOCK_ASSERT();

	if (report != BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT &&
	    report != BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT &&
	    report != BT_HIDS_BOOT_REPORT_MOUSE_INPUT) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD) || defined(CONFIG_BT_HIDS_BOOT_MOUSE)
	*ctx = &boot_reports[report];

	if ((*ctx)->attr != NULL) {
		return 0;
	}
#else
	ARG_UNUSED(ctx);
#endif

	return -ENOTSUP;
}

/* Only Input Reports are looked up by Report ID, to notify them. Reads and
 * writes reach their context through the attribute user data instead.
 */
static struct hids_report_ctx *report_lookup(enum bt_hids_report_type report_type,
					     uint8_t report_id)
{
	HIDS_LOCK_ASSERT();

	for (size_t i = 0U; i < ARRAY_SIZE(reports); i++) {
		if (reports[i].ref.type == (uint8_t)report_type && reports[i].ref.id == report_id) {
			return &reports[i];
		}
	}

	return NULL;
}

/* Resolve a Report of the registered service */
static int report_resolve(enum bt_hids_report_type report_type, uint8_t report_id,
			  struct hids_report_ctx **ctx)
{
	HIDS_LOCK_ASSERT();

	if (!hids.registered) {
		return -ESRCH;
	}

	*ctx = report_lookup(report_type, report_id);
	if (*ctx == NULL) {
		return -ENOENT;
	}

	return 0;
}

/* The per-connection state of a Host lives in a slot claimed when its link
 * reaches HIDS_SEC_LEVEL and freed on disconnect. The pool holds one slot per
 * Host that can reach that level, rather than one per bt_conn_index(). A slot
 * is found by its conn, so a Host that has no slot (the pool was full) is
 * simply not tracked.
 */

/* Slot currently tracking conn, or NULL if none is */
static struct hids_conn *conn_find(const struct bt_conn *conn)
{
	HIDS_LOCK_ASSERT();

	ARRAY_FOR_EACH_PTR(connections, dc) {
		if (dc->conn == conn) {
			return dc;
		}
	}

	return NULL;
}

/* Claim a per-connection slot for conn, or return the existing slot if one
 * is already held. Called with hids_mutex held, so that the lookup and the
 * claim are one step and a slot is never claimed after bt_hids_unregister()
 * released the pool.
 *
 * A Host whose link reached HIDS_SEC_LEVEL before bt_hids_register() gets its
 * slot claimed on its first GATT access. Until then,
 * bt_hids_get_protocol_mode() and bt_hids_get_suspend_state() return -ENOTCONN
 * for that Host.
 */
static struct hids_conn *conn_claim(struct bt_conn *conn)
{
	struct hids_conn *dc;

	HIDS_LOCK_ASSERT();

	if (!hids.registered) {
		return NULL;
	}

	dc = conn_find(conn);
	if (dc != NULL) {
		return dc;
	}

	dc = conn_find(NULL);
	if (dc == NULL) {
		return NULL;
	}

	dc->conn = conn;
	dc->protocol_mode = BT_HIDS_PROTOCOL_REPORT;
	dc->suspended = false;

	return dc;
}

/* GATT callbacks
 *
 * The stack resolves the attribute before it calls a handler, and
 * bt_hids_unregister() can run while the handler waits for hids_mutex. Every
 * handler therefore checks that the service is still registered once it holds
 * the mutex, and otherwise answers as the server does for a handle it no
 * longer has.
 */
#define HIDS_ERR_UNREGISTERED BT_GATT_ERR(BT_ATT_ERR_INVALID_HANDLE)

#if defined(CONFIG_BT_HIDS_PROTOCOL_MODE)
static ssize_t read_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	struct hids_conn *dc;
	uint8_t mode = 0U;

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	dc = conn_claim(conn);
	if (dc == NULL) {
		hids_unlock();
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	mode = (uint8_t)dc->protocol_mode;

	hids_unlock();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &mode, sizeof(mode));
}

static ssize_t write_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	void (*protocol_mode_cb)(struct bt_conn *conn, enum bt_hids_protocol_mode protocol) = NULL;
	const uint8_t *val = buf;
	enum bt_hids_protocol_mode mode;
	struct hids_conn *dc;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(uint8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (*val > BT_HIDS_PROTOCOL_REPORT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	mode = (enum bt_hids_protocol_mode)(*val);

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	dc = conn_claim(conn);
	if (dc == NULL) {
		hids_unlock();
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	dc->protocol_mode = mode;
	if (callbacks != NULL) {
		protocol_mode_cb = callbacks->protocol_mode_changed;
	}

	hids_unlock();

	if (protocol_mode_cb != NULL) {
		protocol_mode_cb(conn, mode);
	}

	return len;
}
#endif /* CONFIG_BT_HIDS_PROTOCOL_MODE */

static ssize_t read_report_map(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	ssize_t ret;

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	ret = bt_gatt_attr_read(conn, attr, buf, len, offset, hids.report_map, hids.report_map_len);

	hids_unlock();

	return ret;
}

static ssize_t read_hid_info(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	ssize_t ret;

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	ret = bt_gatt_attr_read(conn, attr, buf, len, offset, hids.hid_info, sizeof(hids.hid_info));

	hids_unlock();

	return ret;
}

static ssize_t write_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	void (*ctrl_point_cb)(struct bt_conn *conn, enum bt_hids_ctrl_point cmd) = NULL;
	const uint8_t *val = buf;
	struct hids_conn *dc;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* The value is the one octet command, which has no parameters */
	if (len != sizeof(uint8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	switch (*val) {
	case BT_HIDS_CTRL_SUSPEND:
	case BT_HIDS_CTRL_EXIT_SUSPEND:
		break;
	default:
		/* The HID Service defines four more commands, for the HID SCI
		 * feature this service does not implement. The characteristic
		 * declares Write Without Response only, and a Host gets no
		 * response to a Write Command, so this error reaches only a
		 * Host that used a Write Request.
		 */
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	dc = conn_claim(conn);
	if (dc == NULL) {
		hids_unlock();
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	dc->suspended = (*val == BT_HIDS_CTRL_SUSPEND);
	if (callbacks != NULL) {
		ctrl_point_cb = callbacks->ctrl_point;
	}

	hids_unlock();

	if (ctrl_point_cb != NULL) {
		ctrl_point_cb(conn, (enum bt_hids_ctrl_point)(*val));
	}

	return len;
}

static ssize_t read_report_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const struct hids_report_ref *ref = attr->user_data;
	ssize_t ret;

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	ret = bt_gatt_attr_read(conn, attr, buf, len, offset, ref, sizeof(*ref));

	hids_unlock();

	return ret;
}

static ssize_t read_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	const struct hids_report_ctx *ctx = attr->user_data;
	ssize_t ret;

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	ret = bt_gatt_attr_read(conn, attr, buf, len, offset, ctx->value, ctx->value_len);

	hids_unlock();

	return ret;
}

/* Only Output and Feature Reports are writable */
#if (CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT + CONFIG_BT_HIDS_FEATURE_REPORT_COUNT) > 0
static ssize_t write_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	void (*set_report_cb)(struct bt_conn *conn, enum bt_hids_report_type report_type,
			      uint8_t report_id, const uint8_t *data, uint16_t len) = NULL;
	struct hids_report_ctx *ctx = attr->user_data;
	struct hids_report_ref ref;

	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	hids_lock();

	/* Without this check a write that waited for the mutex while the
	 * service was unregistered would leave its value behind for the next
	 * registration.
	 */
	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	ref = ctx->ref;

	if (len > CONFIG_BT_HIDS_MAX_REPORT_LEN) {
		hids_unlock();
		LOG_DBG("SET_REPORT of %u octets for Report ID %u type %u, at most %u", len, ref.id,
			ref.type, CONFIG_BT_HIDS_MAX_REPORT_LEN);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* A Host write replaces the value a read returns */
	(void)memcpy(ctx->value, buf, len);
	ctx->value_len = len;

	if (callbacks != NULL) {
		set_report_cb = callbacks->set_report;
	}

	hids_unlock();

	if (set_report_cb != NULL) {
		set_report_cb(conn, (enum bt_hids_report_type)ref.type, ref.id, buf, len);
	}

	return len;
}
#endif

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD) || defined(CONFIG_BT_HIDS_BOOT_MOUSE)
static ssize_t read_boot_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	const struct hids_boot_report_ctx *ctx = attr->user_data;
	ssize_t ret;

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	ret = bt_gatt_attr_read(conn, attr, buf, len, offset, ctx->value, ctx->len);

	hids_unlock();

	return ret;
}
#endif /* CONFIG_BT_HIDS_BOOT_KEYBOARD || CONFIG_BT_HIDS_BOOT_MOUSE */

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD)
/* The Boot Keyboard Output Report is the only writable Boot Report */
static ssize_t write_boot_report(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	void (*set_boot_report_cb)(struct bt_conn *conn, enum bt_hids_boot_report report,
				   const uint8_t *data, uint16_t len) = NULL;
	struct hids_boot_report_ctx *ctx = attr->user_data;

	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* A Boot Report has a fixed length, which never changes */
	if (len != ctx->len) {
		LOG_DBG("SET_REPORT of %u octets for the Boot Keyboard Output Report, %u expected",
			len, ctx->len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return HIDS_ERR_UNREGISTERED;
	}

	/* A Host write replaces the value a read returns */
	(void)memcpy(ctx->value, buf, len);

	if (callbacks != NULL) {
		set_boot_report_cb = callbacks->set_boot_report;
	}

	hids_unlock();

	if (set_boot_report_cb != NULL) {
		set_boot_report_cb(conn, BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT, buf, len);
	}

	return len;
}
#endif /* CONFIG_BT_HIDS_BOOT_KEYBOARD */

/* Service definition
 *
 * The number of Report characteristics of each type is a build time
 * configuration, so that the attribute table only contains characteristics
 * that the application actually uses. The Report ID of each characteristic is
 * assigned at registration time.
 */

#if defined(CONFIG_BT_HIDS_PROTOCOL_MODE)
#define HIDS_PROTOCOL_MODE_ATTRS                                                                   \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE,                                         \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP, HIDS_PERM_RW,  \
			       read_protocol_mode, write_protocol_mode, NULL),
#else
#define HIDS_PROTOCOL_MODE_ATTRS
#endif

#define HIDS_INPUT_REPORT_ATTRS(_n, _)                                                             \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,       \
			       HIDS_PERM_READ, read_report, NULL,                                  \
			       &reports[HIDS_INPUT_BASE + (_n)]),                                  \
		BT_GATT_CCC(NULL, HIDS_PERM_RW),                                                   \
		BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, HIDS_PERM_READ, read_report_ref, NULL, \
				   &reports[HIDS_INPUT_BASE + (_n)].ref),

#define HIDS_OUTPUT_REPORT_ATTRS(_n, _)                                                            \
	BT_GATT_CHARACTERISTIC(                                                                    \
		BT_UUID_HIDS_REPORT,                                                               \
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,          \
		HIDS_PERM_RW, read_report, write_report, &reports[HIDS_OUTPUT_BASE + (_n)]),       \
		BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, HIDS_PERM_READ, read_report_ref, NULL, \
				   &reports[HIDS_OUTPUT_BASE + (_n)].ref),

#define HIDS_FEATURE_REPORT_ATTRS(_n, _)                                                           \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,        \
			       HIDS_PERM_RW, read_report, write_report,                            \
			       &reports[HIDS_FEATURE_BASE + (_n)]),                                \
		BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, HIDS_PERM_READ, read_report_ref, NULL, \
				   &reports[HIDS_FEATURE_BASE + (_n)].ref),

/* The Boot Reports have no Report Reference descriptor: their format and length
 * are fixed, so they are not described in the Report Map. A Client
 * Characteristic Configuration descriptor is required for each Boot Input
 * Report. Write is optional for a Boot Input Report and is not supported.
 */
#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD)
#define HIDS_BOOT_KEYBOARD_ATTRS                                                                   \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_IN_REPORT,                                     \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, HIDS_PERM_READ,            \
			       read_boot_report, NULL,                                             \
			       &boot_reports[BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT]),                 \
		BT_GATT_CCC(NULL, HIDS_PERM_RW),                                                   \
		BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_KB_OUT_REPORT,                            \
				       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |                    \
					       BT_GATT_CHRC_WRITE_WITHOUT_RESP,                    \
				       HIDS_PERM_RW, read_boot_report, write_boot_report,          \
				       &boot_reports[BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT]),
#else
#define HIDS_BOOT_KEYBOARD_ATTRS
#endif

#if defined(CONFIG_BT_HIDS_BOOT_MOUSE)
#define HIDS_BOOT_MOUSE_ATTRS                                                                      \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_BOOT_MOUSE_IN_REPORT,                                  \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, HIDS_PERM_READ,            \
			       read_boot_report, NULL,                                             \
			       &boot_reports[BT_HIDS_BOOT_REPORT_MOUSE_INPUT]),                    \
		BT_GATT_CCC(NULL, HIDS_PERM_RW),
#else
#define HIDS_BOOT_MOUSE_ATTRS
#endif

/* clang-format off */
#define BT_HIDS_SERVICE_DEFINITION() \
{ \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS), \
	HIDS_PROTOCOL_MODE_ATTRS \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, \
			       HIDS_PERM_READ, read_report_map, NULL, NULL), \
	LISTIFY(CONFIG_BT_HIDS_INPUT_REPORT_COUNT, HIDS_INPUT_REPORT_ATTRS, ()) \
	LISTIFY(CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT, HIDS_OUTPUT_REPORT_ATTRS, ()) \
	LISTIFY(CONFIG_BT_HIDS_FEATURE_REPORT_COUNT, HIDS_FEATURE_REPORT_ATTRS, ()) \
	HIDS_BOOT_KEYBOARD_ATTRS \
	HIDS_BOOT_MOUSE_ATTRS \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, \
			       HIDS_PERM_READ, read_hid_info, NULL, NULL), \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
			       HIDS_PERM_WRITE, NULL, write_ctrl_point, NULL), \
}

/* clang-format on */
static struct bt_gatt_attr hids_attrs[] = BT_HIDS_SERVICE_DEFINITION();
static struct bt_gatt_service hids_gatt_svc = BT_GATT_SERVICE(hids_attrs);

/* Connection management */

/* A Host is tracked from the moment its link has the security level the
 * characteristics require, so a Host that never reaches it does not hold a
 * slot another Host could use. HIDS is LE only (HIDS v1.1 section 1.5) and LE
 * and BR/EDR share the connection pool, so a BR/EDR link must not claim one.
 */
static void conn_claim_if_secure(struct bt_conn *conn, bt_security_t level)
{
	if (!bt_conn_is_type(conn, BT_CONN_TYPE_LE) || level < HIDS_SEC_LEVEL) {
		return;
	}

	hids_lock();
	(void)conn_claim(conn);
	hids_unlock();
}

static void hids_connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		return;
	}

	conn_claim_if_secure(conn, bt_conn_get_security(conn));
}

#if defined(CONFIG_BT_SMP)
static void hids_security_changed(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	if (err != BT_SECURITY_ERR_SUCCESS) {
		return;
	}

	conn_claim_if_secure(conn, level);
}
#endif /* CONFIG_BT_SMP */

static void hids_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct hids_conn *dc;

	ARG_UNUSED(reason);

	hids_lock();
	dc = conn_find(conn);
	if (dc != NULL) {
		dc->conn = NULL;
	}
	hids_unlock();
}

BT_CONN_CB_DEFINE(hids_conn_cb) = {
	.connected = hids_connected,
	.disconnected = hids_disconnected,
#if defined(CONFIG_BT_SMP)
	.security_changed = hids_security_changed,
#endif
};

/* Registration */

static bool report_ids_valid(uint8_t count, const uint8_t *ids, enum bt_hids_report_type type)
{
	for (uint8_t i = 0U; i < count; i++) {
		/* A Report ID of 0 identifies a report that does not carry a
		 * Report ID, so a Report Type that has more than one Report
		 * characteristic cannot use it: the Report Map would describe
		 * some of its reports with an ID and some without.
		 */
		if (count > 1U && ids[i] == 0U) {
			LOG_DBG("Report ID 0 with %u Reports of type %u", count,
				(unsigned int)type);
			return false;
		}

		/* The HID over GATT Profile allows one Report characteristic per
		 * Report ID and Report Type combination. The Report Type is
		 * part of the Report Reference descriptor, so only Report IDs
		 * within a type can collide.
		 */
		for (uint8_t j = 0U; j < i; j++) {
			if (ids[j] == ids[i]) {
				LOG_DBG("Duplicate Report ID %u type %u", ids[i],
					(unsigned int)type);
				return false;
			}
		}
	}

	return true;
}

static bool reports_valid(const struct bt_hids_register_param *param)
{
#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
	if (!report_ids_valid(CONFIG_BT_HIDS_INPUT_REPORT_COUNT, param->input_report_ids,
			      BT_HIDS_REPORT_TYPE_INPUT)) {
		return false;
	}
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
	if (!report_ids_valid(CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT, param->output_report_ids,
			      BT_HIDS_REPORT_TYPE_OUTPUT)) {
		return false;
	}
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
	if (!report_ids_valid(CONFIG_BT_HIDS_FEATURE_REPORT_COUNT, param->feature_report_ids,
			      BT_HIDS_REPORT_TYPE_FEATURE)) {
		return false;
	}
#endif

	return true;
}

static void report_group_assign(struct hids_report_ctx *ctx, uint8_t count, const uint8_t *ids,
				enum bt_hids_report_type type)
{
	for (uint8_t i = 0U; i < count; i++) {
		ctx[i].ref.id = ids[i];
		ctx[i].ref.type = (uint8_t)type;
	}
}

/* Called on parameters reports_valid() accepted */
static void reports_assign(const struct bt_hids_register_param *param)
{
	HIDS_LOCK_ASSERT();

#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
	report_group_assign(&reports[HIDS_INPUT_BASE], CONFIG_BT_HIDS_INPUT_REPORT_COUNT,
			    param->input_report_ids, BT_HIDS_REPORT_TYPE_INPUT);
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
	report_group_assign(&reports[HIDS_OUTPUT_BASE], CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT,
			    param->output_report_ids, BT_HIDS_REPORT_TYPE_OUTPUT);
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
	report_group_assign(&reports[HIDS_FEATURE_BASE], CONFIG_BT_HIDS_FEATURE_REPORT_COUNT,
			    param->feature_report_ids, BT_HIDS_REPORT_TYPE_FEATURE);
#endif

	/* Let every Report characteristic context point at its value
	 * attribute, so that notifications do not have to look it up.
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(hids_attrs); i++) {
		if (hids_attrs[i].read == read_report) {
			((struct hids_report_ctx *)hids_attrs[i].user_data)->attr = &hids_attrs[i];
		}
#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD) || defined(CONFIG_BT_HIDS_BOOT_MOUSE)
		else if (hids_attrs[i].read == read_boot_report) {
			((struct hids_boot_report_ctx *)hids_attrs[i].user_data)->attr =
				&hids_attrs[i];
		}
#endif
	}
}

/* Every nibble of a binary coded decimal value is a decimal digit */
static bool bcd_is_valid(uint16_t bcd)
{
	for (uint8_t shift = 0U; shift < 16U; shift += 4U) {
		if (((bcd >> shift) & 0xFU) > 9U) {
			return false;
		}
	}

	return true;
}

static bool register_param_valid(const struct bt_hids_register_param *param)
{
	if (param == NULL || param->cb == NULL || param->report_map == NULL ||
	    param->report_map_len == 0) {
		return false;
	}

	/* A composite HID Device that needs more than 512 octets to describe
	 * its functions uses multiple HID Service instances, which is not
	 * supported.
	 */
	if (param->report_map_len > BT_HIDS_REPORT_MAP_MAX_LEN) {
		LOG_DBG("Report Map is %u octets, at most %u per HID Service",
			param->report_map_len, BT_HIDS_REPORT_MAP_MAX_LEN);
		return false;
	}

	/* An application must not announce a feature the service cannot back:
	 * the SCI flags make the HID SCI Information and HID SCI Mode
	 * characteristics mandatory, and the remaining bits are reserved for
	 * future use and have to be zero.
	 */
	if ((param->flags & (uint8_t)~HID_INFO_FLAGS_SUPPORTED) != 0U) {
		LOG_DBG("HID Information flags 0x%02x, supported 0x%02x", param->flags,
			HID_INFO_FLAGS_SUPPORTED);
		return false;
	}

	if (!bcd_is_valid(param->bcd_hid)) {
		LOG_DBG("bcdHID 0x%04x is not binary coded decimal", param->bcd_hid);
		return false;
	}

	return reports_valid(param);
}

/* Public API */

int bt_hids_register(const struct bt_hids_register_param *param)
{
	int err;

	/* Validate everything before touching the state, so that a rejected
	 * registration leaves nothing behind.
	 */
	if (!register_param_valid(param)) {
		return -EINVAL;
	}

	hids_lock();

	if (hids.registered) {
		hids_unlock();
		LOG_DBG("HID Service already registered");
		return -EALREADY;
	}

	reports_assign(param);

	callbacks = param->cb;
	hids.report_map = param->report_map;
	hids.report_map_len = param->report_map_len;

	sys_put_le16(param->bcd_hid, &hids.hid_info[HID_INFO_BCDHID_OFFSET]);
	hids.hid_info[HID_INFO_COUNTRY_CODE_OFFSET] = param->country_code;
	hids.hid_info[HID_INFO_FLAGS_OFFSET] = param->flags;

	/* bt_gatt_service_register() takes the host lock with hids_mutex held.
	 * The stack does not hold the host lock while it calls the GATT
	 * handlers above, so the two are never taken in the other order.
	 */
	err = bt_gatt_service_register(&hids_gatt_svc);
	if (err == 0) {
		hids.registered = true;
		LOG_DBG("HID Service registered, %u reports", HIDS_REPORT_COUNT);
	} else {
		LOG_DBG("Failed to register the HID Service (err %d)", err);
		hids_reset();
	}

	hids_unlock();

	return err;
}

int bt_hids_unregister(void)
{
	int err;

	hids_lock();

	if (!hids.registered) {
		hids_unlock();
		return -EALREADY;
	}

	err = bt_gatt_service_unregister(&hids_gatt_svc);
	if (err != 0) {
		hids_unlock();
		LOG_DBG("Failed to unregister the HID Service (err %d)", err);
		return err;
	}

	/* The values and the per-Host HID state go with the service. Hosts
	 * stay connected and are notified through the Service Changed
	 * characteristic, like for any other GATT service that is unregistered
	 * at runtime. Clearing the flag and the pool in one step keeps a Host
	 * that connects meanwhile from claiming a slot that would outlive the
	 * registration.
	 */
	hids_reset();

	hids_unlock();

	return 0;
}

int bt_hids_report_set(enum bt_hids_report_type report_type, uint8_t report_id,
		       const uint8_t *data, uint16_t len)
{
	struct hids_report_ctx *ctx;
	int err;

	if (data == NULL || len == 0U || len > CONFIG_BT_HIDS_MAX_REPORT_LEN) {
		return -EINVAL;
	}

	hids_lock();

	err = report_resolve(report_type, report_id, &ctx);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	(void)memcpy(ctx->value, data, len);
	ctx->value_len = len;

	hids_unlock();

	return 0;
}

int bt_hids_report_get(enum bt_hids_report_type report_type, uint8_t report_id, uint8_t *data,
		       uint16_t *len)
{
	struct hids_report_ctx *ctx;
	int err;

	if (data == NULL || len == NULL) {
		return -EINVAL;
	}

	hids_lock();

	err = report_resolve(report_type, report_id, &ctx);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	if (*len < ctx->value_len) {
		hids_unlock();
		return -ENOMEM;
	}

	(void)memcpy(data, ctx->value, ctx->value_len);
	*len = ctx->value_len;

	hids_unlock();

	return 0;
}

int bt_hids_send_report(struct bt_conn *conn, uint8_t report_id, const uint8_t *data, uint16_t len,
			bt_gatt_complete_func_t func, void *user_data)
{
	const struct bt_gatt_attr *attr;
	struct hids_report_ctx *ctx;
	int err;

	if (data == NULL || len == 0U || len > CONFIG_BT_HIDS_MAX_REPORT_LEN) {
		return -EINVAL;
	}

	hids_lock();

	err = report_resolve(BT_HIDS_REPORT_TYPE_INPUT, report_id, &ctx);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	attr = ctx->attr;

	hids_unlock();

	/* Sent without hids_mutex, as the notification can wait for a buffer.
	 * The attribute is static, and once the service is unregistered it has
	 * no handle, so the notification fails with -ENOENT.
	 */
	struct bt_gatt_notify_params params = {
		.attr = attr,
		.data = data,
		.len = len,
		.func = func,
		.user_data = user_data,
	};

	return bt_gatt_notify_cb(conn, &params);
}

int bt_hids_boot_report_set(enum bt_hids_boot_report report, const uint8_t *data, uint16_t len)
{
	struct hids_boot_report_ctx *ctx;
	int err;

	if (data == NULL) {
		return -EINVAL;
	}

	hids_lock();

	err = boot_report_resolve(report, &ctx);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	if (len != ctx->len) {
		hids_unlock();
		return -EINVAL;
	}

	(void)memcpy(ctx->value, data, len);

	hids_unlock();

	return 0;
}

int bt_hids_boot_report_get(enum bt_hids_boot_report report, uint8_t *data, uint16_t *len)
{
	struct hids_boot_report_ctx *ctx;
	int err;

	if (data == NULL || len == NULL) {
		return -EINVAL;
	}

	hids_lock();

	err = boot_report_resolve(report, &ctx);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	if (*len < ctx->len) {
		hids_unlock();
		return -ENOMEM;
	}

	(void)memcpy(data, ctx->value, ctx->len);
	*len = ctx->len;

	hids_unlock();

	return 0;
}

int bt_hids_boot_report_send(struct bt_conn *conn, enum bt_hids_boot_report report,
			     const uint8_t *data, uint16_t len, bt_gatt_complete_func_t func,
			     void *user_data)
{
	const struct bt_gatt_attr *attr;
	struct hids_boot_report_ctx *ctx;
	int err;

	/* Only the Boot Input Reports are notifiable */
	if (data == NULL || report == BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT) {
		return -EINVAL;
	}

	hids_lock();

	err = boot_report_resolve(report, &ctx);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	if (len != ctx->len) {
		hids_unlock();
		return -EINVAL;
	}

	attr = ctx->attr;

	hids_unlock();

	/* Sent without hids_mutex, see bt_hids_send_report() */
	struct bt_gatt_notify_params params = {
		.attr = attr,
		.data = data,
		.len = len,
		.func = func,
		.user_data = user_data,
	};

	return bt_gatt_notify_cb(conn, &params);
}

static int conn_state_get(struct bt_conn *conn, const struct hids_conn **dc)
{
	struct bt_conn_info info;

	HIDS_LOCK_ASSERT();

	if (!hids.registered) {
		return -ESRCH;
	}

	if (bt_conn_get_info(conn, &info) < 0 || info.state != BT_CONN_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	/* Look the slot up without claiming one: only the Bluetooth thread
	 * claims slots.
	 */
	*dc = conn_find(conn);

	return (*dc != NULL) ? 0 : -ENOTCONN;
}

int bt_hids_get_protocol_mode(struct bt_conn *conn, enum bt_hids_protocol_mode *mode)
{
	const struct hids_conn *dc;
	int err;

	if (conn == NULL || mode == NULL) {
		return -EINVAL;
	}

	hids_lock();

	err = conn_state_get(conn, &dc);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	*mode = dc->protocol_mode;

	hids_unlock();

	return 0;
}

int bt_hids_get_suspend_state(struct bt_conn *conn, bool *suspended)
{
	const struct hids_conn *dc;
	int err;

	if (conn == NULL || suspended == NULL) {
		return -EINVAL;
	}

	hids_lock();

	err = conn_state_get(conn, &dc);
	if (err != 0) {
		hids_unlock();
		return err;
	}

	*suspended = dc->suspended;

	hids_unlock();

	return 0;
}
