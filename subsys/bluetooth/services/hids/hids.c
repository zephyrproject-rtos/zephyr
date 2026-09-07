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
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "hids_internal.h"

LOG_MODULE_REGISTER(bt_hids, CONFIG_BT_HIDS_LOG_LEVEL);

BUILD_ASSERT(HIDS_REPORT_COUNT > 0, "The HID Service needs at least one Report characteristic");

/* The HID over GATT Profile requires an encrypted link for reading, writing
 * and notifying HID Service characteristics.
 */
#define HIDS_PERM_READ  (BT_GATT_PERM_READ_ENCRYPT)
#define HIDS_PERM_WRITE (BT_GATT_PERM_WRITE_ENCRYPT)
#define HIDS_PERM_RW    (HIDS_PERM_READ | HIDS_PERM_WRITE)

/* Module context */
static const struct bt_hids_cb *callbacks;
static struct hids_state hids;
#define HIDS_MAX_INSTS MIN(CONFIG_BT_MAX_CONN, CONFIG_BT_MAX_PAIRED)
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
	[BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT] = {.len = BT_HIDS_BOOT_KB_IN_LEN},
	[BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT] = {.len = BT_HIDS_BOOT_KB_OUT_LEN},
	[BT_HIDS_BOOT_REPORT_MOUSE_INPUT] = {.len = BT_HIDS_BOOT_MOUSE_IN_LEN},
};

BUILD_ASSERT(BT_HIDS_BOOT_KB_IN_LEN <= HIDS_BOOT_REPORT_MAX_LEN &&
		     BT_HIDS_BOOT_KB_OUT_LEN <= HIDS_BOOT_REPORT_MAX_LEN &&
		     BT_HIDS_BOOT_MOUSE_IN_LEN <= HIDS_BOOT_REPORT_MAX_LEN,
	     "A Boot Report is longer than the Boot Report buffer");
#endif /* CONFIG_BT_HIDS_BOOT_KEYBOARD || CONFIG_BT_HIDS_BOOT_MOUSE */

/* Resolve a Boot Report of the registered service. A Boot Report the
 * configuration leaves out has no attribute bound to its context.
 */
static int boot_report_resolve(enum bt_hids_boot_report report, struct hids_boot_report_ctx **ctx)
{
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
	for (size_t i = 0U; i < ARRAY_SIZE(reports); i++) {
		if (reports[i].ref.type == (uint8_t)report_type && reports[i].ref.id == report_id) {
			return &reports[i];
		}
	}

	return NULL;
}

static const struct hids_report_ctx *input_report_lookup(uint8_t report_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(reports); i++) {
		if (reports[i].ref.type == BT_HIDS_REPORT_TYPE_INPUT &&
		    reports[i].ref.id == report_id) {
			return &reports[i];
		}
	}

	return NULL;
}

/* The per-connection state of a Host lives in a slot claimed when its link is
 * encrypted and freed on disconnect. The pool holds MIN(BT_MAX_CONN,
 * BT_MAX_PAIRED) slots, one per Host that can hold the encrypted link HOGP
 * requires, rather than one per bt_conn_index(). A slot is found by its conn,
 * so a Host that has no slot (the pool was full when it encrypted) is simply
 * not tracked.
 */

/* Slot currently tracking conn, or NULL if none is */
static struct hids_conn *conn_find(const struct bt_conn *conn)
{
	ARRAY_FOR_EACH_PTR(connections, dc) {
		if (dc->conn == conn) {
			return dc;
		}
	}

	return NULL;
}

/* Claim a per-connection slot for conn, or return the existing slot if one
 * is already held.  Called from hids_security_changed() and from the GATT
 * callbacks for Protocol Mode and HID Control Point; all three share the BT
 * RX thread, so claims never race with each other.
 *
 * A Host encrypted before bt_hids_register() gets its slot claimed on its
 * first GATT access.  Until then, bt_hids_get_protocol_mode() and
 * bt_hids_get_suspend_state() return -ENOTCONN for that Host.
 */
static struct hids_conn *conn_alloc(struct bt_conn *conn)
{
	struct hids_conn *dc = conn_find(conn);

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

/* GATT callbacks */

#if defined(CONFIG_BT_HIDS_PROTOCOL_MODE)
static ssize_t read_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	struct hids_conn *dc = conn_alloc(conn);
	uint8_t mode;

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	mode = (uint8_t)dc->protocol_mode;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &mode, sizeof(mode));
}

static ssize_t write_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct hids_conn *dc = conn_alloc(conn);
	const uint8_t *val = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(uint8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (*val > BT_HIDS_PROTOCOL_REPORT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	dc->protocol_mode = (enum bt_hids_protocol_mode)(*val);

	if (callbacks != NULL && callbacks->protocol_mode_changed != NULL) {
		callbacks->protocol_mode_changed(conn, dc->protocol_mode);
	}

	return len;
}
#endif /* CONFIG_BT_HIDS_PROTOCOL_MODE */

static ssize_t read_report_map(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids.report_map,
				 hids.report_map_len);
}

static ssize_t read_hid_info(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids.hid_info,
				 sizeof(hids.hid_info));
}

static ssize_t write_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct hids_conn *dc = conn_alloc(conn);
	const uint8_t *val = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(uint8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	switch (*val) {
	case BT_HIDS_CTRL_SUSPEND:
		dc->suspended = true;
		break;
	case BT_HIDS_CTRL_EXIT_SUSPEND:
		dc->suspended = false;
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

	if (callbacks != NULL && callbacks->ctrl_point != NULL) {
		callbacks->ctrl_point(conn, (enum bt_hids_ctrl_point)(*val));
	}

	return len;
}

static ssize_t read_report_ref(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const struct hids_report_ref *ref = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ref, sizeof(*ref));
}

static ssize_t read_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	const struct hids_report_ctx *ctx = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ctx->value, ctx->value_len);
}

/* Only Output and Feature Reports are writable */
#if (CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT + CONFIG_BT_HIDS_FEATURE_REPORT_COUNT) > 0
static ssize_t write_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	struct hids_report_ctx *ctx = attr->user_data;

	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len > CONFIG_BT_HIDS_MAX_REPORT_LEN) {
		LOG_WRN("SET_REPORT of %u octets for Report ID %u type %u, at "
			"most %u",
			len, ctx->ref.id, ctx->ref.type, CONFIG_BT_HIDS_MAX_REPORT_LEN);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* A Host write replaces the value a read returns */
	(void)memcpy(ctx->value, buf, len);
	ctx->value_len = len;

	if (callbacks != NULL && callbacks->set_report != NULL) {
		callbacks->set_report(conn, (enum bt_hids_report_type)ctx->ref.type,
				      ctx->ref.id, buf, len);
	}

	return len;
}
#endif

#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
static ssize_t report_ccc_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				uint16_t value);

#define HIDS_CCC_USER_DATA(_n, _) BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, report_ccc_write, NULL)

static struct bt_gatt_ccc_managed_user_data input_report_ccc[CONFIG_BT_HIDS_INPUT_REPORT_COUNT] = {
	LISTIFY(CONFIG_BT_HIDS_INPUT_REPORT_COUNT, HIDS_CCC_USER_DATA, (,)) };

static ssize_t report_ccc_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				uint16_t value)
{
	const struct bt_gatt_ccc_managed_user_data *ccc = attr->user_data;
	size_t idx = (size_t)(ccc - input_report_ccc);
	uint16_t bits;

	if (idx >= CONFIG_BT_HIDS_INPUT_REPORT_COUNT) {
		return sizeof(value);
	}

	/* HID Service, section 1.9.2: a Reserved for Future Use bit is
	 * processed as if it were zero. An Input Report supports notifications
	 * only, so the indication bit is refused rather than masked, as the Host
	 * would otherwise believe indications are on.
	 *
	 * What the core stores is the value the Host wrote, Reserved bits
	 * included: a managed CCC callback is given the value and has no way to
	 * change what is stored. A notification to one connection still reaches
	 * such a Host, because bt_gatt_is_subscribed() tests the notify bit, but
	 * the broadcast of bt_gatt_notify_cb(NULL, ...) does not, because it
	 * matches the stored value against BT_GATT_CCC_NOTIFY exactly. See the
	 * note on the conn argument of bt_hids_send_report().
	 */
	bits = value & (BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE);

	if ((bits & BT_GATT_CCC_INDICATE) != 0U) {
		LOG_WRN("CCC write of 0x%04x for Report ID %u asks for indications", value,
			reports[HIDS_INPUT_BASE + idx].ref.id);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (callbacks != NULL && callbacks->ccc_changed != NULL) {
		const struct hids_report_ctx *ctx = &reports[HIDS_INPUT_BASE + idx];

		callbacks->ccc_changed(conn, ctx->ref.id, (enum bt_hids_report_type)ctx->ref.type,
				       (bits & BT_GATT_CCC_NOTIFY) != 0U);
	}

	return sizeof(value);
}
#endif /* CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0 */

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD) || defined(CONFIG_BT_HIDS_BOOT_MOUSE)
static ssize_t read_boot_report(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	const struct hids_boot_report_ctx *ctx = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ctx->value, ctx->len);
}

static ssize_t boot_report_ccc_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     uint16_t value);

/* Both descriptors are defined whenever any Boot Report is, so that
 * boot_report_ccc_write() compiles either way. Only the one of a Boot Input
 * Report that is part of the service is referenced by an attribute.
 */
static struct bt_gatt_ccc_managed_user_data boot_kb_in_ccc =
	BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, boot_report_ccc_write, NULL);
static struct bt_gatt_ccc_managed_user_data boot_mouse_in_ccc =
	BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, boot_report_ccc_write, NULL);

static ssize_t boot_report_ccc_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     uint16_t value)
{
	enum bt_hids_boot_report report;
	uint16_t bits;

	if (attr->user_data == (void *)&boot_kb_in_ccc) {
		report = BT_HIDS_BOOT_REPORT_KEYBOARD_INPUT;
	} else if (attr->user_data == (void *)&boot_mouse_in_ccc) {
		report = BT_HIDS_BOOT_REPORT_MOUSE_INPUT;
	} else {
		return sizeof(value);
	}

	/* A Boot Input Report supports notifications only, like a Report, and a
	 * Reserved for Future Use bit is processed as if it were zero. The
	 * Reserved bits the core stores keep this Host out of a broadcast, as
	 * described for report_ccc_write().
	 */
	bits = value & (BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE);

	if ((bits & BT_GATT_CCC_INDICATE) != 0U) {
		LOG_WRN("CCC write of 0x%04x for Boot Report %u asks for indications", value,
			(unsigned int)report);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (callbacks != NULL && callbacks->boot_ccc_changed != NULL) {
		callbacks->boot_ccc_changed(conn, report, (bits & BT_GATT_CCC_NOTIFY) != 0U);
	}

	return sizeof(value);
}
#endif /* CONFIG_BT_HIDS_BOOT_KEYBOARD || CONFIG_BT_HIDS_BOOT_MOUSE */

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD)
/* The Boot Keyboard Output Report is the only writable Boot Report */
static ssize_t write_boot_report(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct hids_boot_report_ctx *ctx = attr->user_data;

	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* A Boot Report has a fixed length */
	if (len != ctx->len) {
		LOG_WRN("SET_REPORT of %u octets for the Boot Keyboard Output Report, %u expected",
			len, ctx->len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* A Host write replaces the value a read returns */
	(void)memcpy(ctx->value, buf, len);

	if (callbacks != NULL && callbacks->set_boot_report != NULL) {
		callbacks->set_boot_report(conn, BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT, buf, len);
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
		BT_GATT_CCC_MANAGED(&input_report_ccc[_n], HIDS_PERM_READ | HIDS_PERM_WRITE),      \
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
		BT_GATT_CCC_MANAGED(&boot_kb_in_ccc, HIDS_PERM_READ | HIDS_PERM_WRITE),            \
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
		BT_GATT_CCC_MANAGED(&boot_mouse_in_ccc, HIDS_PERM_READ | HIDS_PERM_WRITE),
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

static void hids_security_changed(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	/* Every HID Service characteristic needs an encrypted link, so a Host
	 * is tracked from the moment its link is encrypted. Claiming the slot
	 * here rather than on connect keeps a Host that never encrypts from
	 * holding a slot a Host that does encrypt could use. HIDS is LE only
	 * (HIDS v1.1 section 1.5) and LE and BR/EDR share the connection pool,
	 * so an encrypted BR/EDR link must not claim a slot.
	 */
	if (!hids.registered || !bt_conn_is_type(conn, BT_CONN_TYPE_LE) ||
	    err != BT_SECURITY_ERR_SUCCESS || level < BT_SECURITY_L2) {
		return;
	}

	(void)conn_alloc(conn);
}

static void hids_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct hids_conn *dc = conn_find(conn);

	ARG_UNUSED(reason);

	if (dc != NULL) {
		dc->conn = NULL;
	}
}

BT_CONN_CB_DEFINE(hids_conn_cb) = {
	.security_changed = hids_security_changed,
	.disconnected = hids_disconnected,
};

/* Registration */

static int setup_report_group(struct hids_report_ctx *ctx, uint8_t count, const uint8_t *ids,
			      enum bt_hids_report_type type)
{
	for (uint8_t i = 0U; i < count; i++) {
		/* A Report ID of 0 identifies a report that does not carry a
		 * Report ID, so a Report Type that has more than one Report
		 * characteristic cannot use it: the Report Map would describe
		 * some of its reports with an ID and some without.
		 */
		if (count > 1U && ids[i] == 0U) {
			LOG_ERR("Report ID 0 with %u Reports of type %u", count,
				(unsigned int)type);
			return -EINVAL;
		}

		/* The HID over GATT Profile allows one Report characteristic per
		 * Report ID and Report Type combination. The Report Type is
		 * part of the Report Reference descriptor, so only Report IDs
		 * within a type can collide.
		 */
		for (uint8_t j = 0U; j < i; j++) {
			if (ids[j] == ids[i]) {
				LOG_ERR("Duplicate Report ID %u type %u", ids[i],
					(unsigned int)type);
				return -EINVAL;
			}
		}

		ctx[i].ref.id = ids[i];
		ctx[i].ref.type = (uint8_t)type;
	}

	return 0;
}

static int setup_reports(const struct bt_hids_register_param *param)
{
	int err;

#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
	err = setup_report_group(&reports[HIDS_INPUT_BASE], CONFIG_BT_HIDS_INPUT_REPORT_COUNT,
				 param->input_report_ids, BT_HIDS_REPORT_TYPE_INPUT);
	if (err != 0) {
		return err;
	}
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
	err = setup_report_group(&reports[HIDS_OUTPUT_BASE], CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT,
				 param->output_report_ids, BT_HIDS_REPORT_TYPE_OUTPUT);
	if (err != 0) {
		return err;
	}
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
	err = setup_report_group(&reports[HIDS_FEATURE_BASE], CONFIG_BT_HIDS_FEATURE_REPORT_COUNT,
				 param->feature_report_ids, BT_HIDS_REPORT_TYPE_FEATURE);
	if (err != 0) {
		return err;
	}
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

	return 0;
}

/* Public API */

int bt_hids_register(const struct bt_hids_register_param *param)
{
	int err;

	if (hids.registered) {
		LOG_ERR("HID Service already registered");
		return -EALREADY;
	}

	if (param == NULL || param->cb == NULL || param->report_map == NULL ||
	    param->report_map_len == 0) {
		return -EINVAL;
	}

	/* A composite HID Device that needs more than 512 octets to describe
	 * its functions uses multiple HID Service instances, which is not
	 * supported.
	 */
	if (param->report_map_len > BT_HIDS_REPORT_MAP_MAX_LEN) {
		LOG_ERR("Report Map is %u octets, at most %u per HID Service",
			param->report_map_len, BT_HIDS_REPORT_MAP_MAX_LEN);
		return -EINVAL;
	}

	/* An application must not announce a feature the service cannot back:
	 * the SCI flags make the HID SCI Information and HID SCI Mode
	 * characteristics mandatory, and the remaining bits are reserved for
	 * future use and have to be zero.
	 */
	if ((param->info.flags & (uint8_t)~HID_INFO_FLAGS_SUPPORTED) != 0U) {
		LOG_ERR("HID Information flags 0x%02x, supported 0x%02x", param->info.flags,
			HID_INFO_FLAGS_SUPPORTED);
		return -EINVAL;
	}

	err = setup_reports(param);
	if (err != 0) {
		return err;
	}

	callbacks = param->cb;
	hids.report_map = param->report_map;
	hids.report_map_len = param->report_map_len;

	sys_put_le16(param->info.bcd_hid, &hids.hid_info[HID_INFO_BCDHID_OFFSET]);
	hids.hid_info[HID_INFO_COUNTRY_CODE_OFFSET] = param->info.b_country_code;
	hids.hid_info[HID_INFO_FLAGS_OFFSET] = param->info.flags;

	err = bt_gatt_service_register(&hids_gatt_svc);
	if (err != 0) {
		LOG_ERR("Failed to register the HID Service (err %d)", err);
		callbacks = NULL;
		return err;
	}

	hids.registered = true;

	LOG_DBG("HID Service registered, %u reports", HIDS_REPORT_COUNT);

	return 0;
}

int bt_hids_unregister(void)
{
	int err;

	if (!hids.registered) {
		return -EALREADY;
	}

	err = bt_gatt_service_unregister(&hids_gatt_svc);
	if (err != 0) {
		LOG_ERR("Failed to unregister the HID Service (err %d)", err);
		return err;
	}

	/* Stop tracking Hosts before releasing the slots, so that a Host
	 * connecting while this runs is less likely to claim one of them back.
	 */
	hids.registered = false;
	callbacks = NULL;
	hids.report_map = NULL;
	hids.report_map_len = 0;

	/* The GATT service is gone, so the per-Host HID state goes with it.
	 * Hosts stay connected and are notified through the Service Changed
	 * characteristic, like for any other GATT service that is
	 * unregistered at runtime. Freeing every slot leaves the pool empty for
	 * the next registration.
	 */
	ARRAY_FOR_EACH_PTR(connections, dc) {
		dc->conn = NULL;
	}

	/* The values go with the service */
	for (size_t i = 0U; i < ARRAY_SIZE(reports); i++) {
		(void)memset(reports[i].value, 0, sizeof(reports[i].value));
		reports[i].value_len = 0U;
	}

#if defined(CONFIG_BT_HIDS_BOOT_KEYBOARD) || defined(CONFIG_BT_HIDS_BOOT_MOUSE)
	for (size_t i = 0U; i < ARRAY_SIZE(boot_reports); i++) {
		(void)memset(boot_reports[i].value, 0, sizeof(boot_reports[i].value));
	}
#endif

	return 0;
}

int bt_hids_report_set(enum bt_hids_report_type report_type, uint8_t report_id,
		       const uint8_t *data, uint16_t len)
{
	struct hids_report_ctx *ctx;

	if (data == NULL || len == 0U || len > CONFIG_BT_HIDS_MAX_REPORT_LEN) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

	ctx = report_lookup(report_type, report_id);
	if (ctx == NULL) {
		return -ENOENT;
	}

	(void)memcpy(ctx->value, data, len);
	ctx->value_len = len;

	return 0;
}

int bt_hids_report_get(enum bt_hids_report_type report_type, uint8_t report_id, uint8_t *data,
		       uint16_t *len)
{
	const struct hids_report_ctx *ctx;

	if (data == NULL || len == NULL) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

	ctx = report_lookup(report_type, report_id);
	if (ctx == NULL) {
		return -ENOENT;
	}

	/* Snapshot value_len once to avoid a TOCTOU race with a concurrent
	 * set_report callback updating the same field from the stack thread.
	 * The caller is responsible for synchronising with set_report.
	 */
	uint16_t snap = ctx->value_len;

	if (*len < snap) {
		return -ENOMEM;
	}

	(void)memcpy(data, ctx->value, snap);
	*len = snap;

	return 0;
}

int bt_hids_send_report(struct bt_conn *conn, uint8_t report_id, const uint8_t *data, uint16_t len,
			bt_gatt_complete_func_t func, void *user_data)
{
	const struct hids_report_ctx *ctx;

	if (data == NULL || len == 0U || len > CONFIG_BT_HIDS_MAX_REPORT_LEN) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

	ctx = input_report_lookup(report_id);
	if (ctx == NULL) {
		return -ENOENT;
	}

	struct bt_gatt_notify_params params = {
		.attr = ctx->attr,
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

	err = boot_report_resolve(report, &ctx);
	if (err != 0) {
		return err;
	}

	if (len != ctx->len) {
		return -EINVAL;
	}

	(void)memcpy(ctx->value, data, len);

	return 0;
}

int bt_hids_boot_report_get(enum bt_hids_boot_report report, uint8_t *data, uint16_t *len)
{
	struct hids_boot_report_ctx *ctx;
	int err;

	if (data == NULL || len == NULL) {
		return -EINVAL;
	}

	err = boot_report_resolve(report, &ctx);
	if (err != 0) {
		return err;
	}

	if (*len < ctx->len) {
		return -ENOMEM;
	}

	(void)memcpy(data, ctx->value, ctx->len);
	*len = ctx->len;

	return 0;
}

int bt_hids_boot_report_send(struct bt_conn *conn, enum bt_hids_boot_report report,
			     const uint8_t *data, uint16_t len, bt_gatt_complete_func_t func,
			     void *user_data)
{
	struct hids_boot_report_ctx *ctx;
	int err;

	/* Only the Boot Input Reports are notifiable */
	if (data == NULL || report == BT_HIDS_BOOT_REPORT_KEYBOARD_OUTPUT) {
		return -EINVAL;
	}

	err = boot_report_resolve(report, &ctx);
	if (err != 0) {
		return err;
	}

	if (len != ctx->len) {
		return -EINVAL;
	}

	struct bt_gatt_notify_params params = {
		.attr = ctx->attr,
		.data = data,
		.len = len,
		.func = func,
		.user_data = user_data,
	};

	return bt_gatt_notify_cb(conn, &params);
}

int bt_hids_get_protocol_mode(struct bt_conn *conn, enum bt_hids_protocol_mode *mode)
{
	const struct hids_conn *dc;
	struct bt_conn_info info;

	if (conn == NULL || mode == NULL) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

	if (bt_conn_get_info(conn, &info) < 0 || info.state != BT_CONN_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	/* conn_find rather than conn_alloc: public API runs on the app thread
	 * and must not race with GATT callbacks on the BT RX thread.
	 */
	dc = conn_find(conn);
	if (dc == NULL) {
		return -ENOTCONN;
	}

	*mode = dc->protocol_mode;

	return 0;
}

int bt_hids_get_suspend_state(struct bt_conn *conn, bool *suspended)
{
	const struct hids_conn *dc;
	struct bt_conn_info info;

	if (conn == NULL || suspended == NULL) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

	if (bt_conn_get_info(conn, &info) < 0 || info.state != BT_CONN_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	/* conn_find rather than conn_alloc: public API runs on the app thread
	 * and must not race with GATT callbacks on the BT RX thread.
	 */
	dc = conn_find(conn);
	if (dc == NULL) {
		return -ENOTCONN;
	}

	*suspended = dc->suspended;

	return 0;
}
