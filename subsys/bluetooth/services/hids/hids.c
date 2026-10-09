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
static struct hids_conn connections[CONFIG_BT_HIDS_MAX_CONNECTIONS];

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
static struct hids_report_ctx *report_lookup(uint8_t report_type, uint8_t report_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(reports); i++) {
		if (reports[i].ref.type == report_type && reports[i].ref.id == report_id) {
			return &reports[i];
		}
	}

	return NULL;
}

static const struct hids_report_ctx *input_report_lookup(uint8_t report_id)
{
	for (size_t i = 0U; i < ARRAY_SIZE(reports); i++) {
		if (reports[i].ref.type == BT_HID_REPORT_TYPE_INPUT &&
		    reports[i].ref.id == report_id) {
			return &reports[i];
		}
	}

	return NULL;
}

static struct hids_conn *conn_lookup(const struct bt_conn *conn)
{
	for (size_t i = 0U; i < ARRAY_SIZE(connections); i++) {
		if (atomic_ptr_get(&connections[i].conn) == conn) {
			return &connections[i];
		}
	}

	return NULL;
}

/* Claim a slot for a Host. The reference is taken before the slot is claimed,
 * so that a slot never holds a connection the service does not own.
 */
static struct hids_conn *conn_alloc(struct bt_conn *conn)
{
	for (size_t i = 0U; i < ARRAY_SIZE(connections); i++) {
		struct hids_conn *dc = &connections[i];

		(void)bt_conn_ref(conn);

		if (atomic_ptr_cas(&dc->conn, NULL, conn)) {
			dc->protocol_mode = BT_HID_PROTOCOL_REPORT;
			dc->suspended = false;

			return dc;
		}

		bt_conn_unref(conn);
	}

	return NULL;
}

/* Release a slot, but only while it still holds the given connection. The
 * disconnected callback, bt_hids_register() and bt_hids_unregister() can reach
 * the same slot from different contexts, so a slot that was already released,
 * and possibly claimed by another Host since, is left alone and only the caller
 * whose exchange succeeds drops the reference. The state is not reset here,
 * conn_alloc() initializes it after claiming the slot.
 */
static void conn_free(struct hids_conn *dc, struct bt_conn *conn)
{
	if (!atomic_ptr_cas(&dc->conn, conn, NULL)) {
		return;
	}

	bt_conn_unref(conn);
}

/* GATT callbacks */

#if defined(CONFIG_BT_HIDS_PROTOCOL_MODE)
static ssize_t read_protocol_mode(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const struct hids_conn *dc = conn_lookup(conn);
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
	struct hids_conn *dc = conn_lookup(conn);
	const uint8_t *val = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (len != sizeof(uint8_t) || offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (*val > BT_HID_PROTOCOL_REPORT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	dc->protocol_mode = (enum bt_hid_protocol_mode)(*val);

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
	struct hids_conn *dc = conn_lookup(conn);
	const uint8_t *val = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (len != sizeof(uint8_t) || offset != 0) {
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
		callbacks->set_report(conn, ctx->ref.type, ctx->ref.id, buf, len);
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
	 * included, which a managed CCC callback has no way to change. It is
	 * harmless: bt_gatt_is_subscribed() and the notification path both test
	 * the notify bit rather than compare the whole value.
	 */
	bits = value & (BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE);

	if ((bits & BT_GATT_CCC_INDICATE) != 0U) {
		LOG_WRN("CCC write of 0x%04x for Report ID %u asks for indications", value,
			reports[HIDS_INPUT_BASE + idx].ref.id);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (callbacks != NULL && callbacks->ccc_changed != NULL) {
		const struct hids_report_ctx *ctx = &reports[HIDS_INPUT_BASE + idx];

		callbacks->ccc_changed(conn, ctx->ref.id, ctx->ref.type,
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
	 * Reserved for Future Use bit is processed as if it were zero.
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

static void hids_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err != 0U || !hids.registered) {
		return;
	}

	if (bt_conn_get_info(conn, &info) < 0 || info.type != BT_CONN_TYPE_LE) {
		return;
	}

	if (conn_alloc(conn) == NULL) {
		LOG_ERR("No free connection slot, HID state not tracked for "
			"this Host (increase BT_HIDS_MAX_CONNECTIONS)");
	}
}

static void hids_disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	/* Every slot of this Host, in case bt_hids_register() and the
	 * connected callback both tracked it.
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(connections); i++) {
		conn_free(&connections[i], conn);
	}
}

BT_CONN_CB_DEFINE(hids_conn_cb) = {
	.connected = hids_connected,
	.disconnected = hids_disconnected,
};

static void track_existing_conn(struct bt_conn *conn, void *data)
{
	struct bt_conn_info info;
	struct hids_conn *dc;

	ARG_UNUSED(data);

	if (conn_lookup(conn) != NULL) {
		return;
	}

	/* bt_conn_foreach() also visits the objects of connection attempts and
	 * of connectable advertising. Those never reach the disconnected
	 * callback, and the placeholder of plain connectable advertising gets
	 * no callback at all when the advertiser is stopped, so a slot claimed
	 * for them would never be released. The connected callback tracks them
	 * once they are up.
	 */
	if (bt_conn_get_info(conn, &info) < 0 || info.state != BT_CONN_STATE_CONNECTED) {
		return;
	}

	dc = conn_alloc(conn);
	if (dc == NULL) {
		LOG_ERR("No free connection slot for an already connected Host");
		return;
	}

	/* The state was read before the slot was claimed. A disconnection in
	 * between ran the disconnected callback while the slot was still free,
	 * so release it here in that case.
	 */
	if (bt_conn_get_info(conn, &info) < 0 || info.state != BT_CONN_STATE_CONNECTED) {
		conn_free(dc, conn);
	}
}

/* Registration */

static int setup_report_group(struct hids_report_ctx *ctx, uint8_t count, const uint8_t *ids,
			      uint8_t type)
{
	for (uint8_t i = 0U; i < count; i++) {
		/* A Report ID of 0 identifies a report that does not carry a
		 * Report ID, so a Report Type that has more than one Report
		 * characteristic cannot use it: the Report Map would describe
		 * some of its reports with an ID and some without.
		 */
		if (count > 1U && ids[i] == 0U) {
			LOG_ERR("Report ID 0 with %u Reports of type %u", count, type);
			return -EINVAL;
		}

		/* The HID over GATT Profile allows one Report characteristic per
		 * Report ID and Report Type combination. The Report Type is
		 * part of the Report Reference descriptor, so only Report IDs
		 * within a type can collide.
		 */
		for (uint8_t j = 0U; j < i; j++) {
			if (ids[j] == ids[i]) {
				LOG_ERR("Duplicate Report ID %u type %u", ids[i], type);
				return -EINVAL;
			}
		}

		ctx[i].ref.id = ids[i];
		ctx[i].ref.type = type;
	}

	return 0;
}

static int setup_reports(const struct bt_hids_register_param *param)
{
	int err;

#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
	err = setup_report_group(&reports[HIDS_INPUT_BASE], CONFIG_BT_HIDS_INPUT_REPORT_COUNT,
				 param->input_report_ids, BT_HID_REPORT_TYPE_INPUT);
	if (err != 0) {
		return err;
	}
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
	err = setup_report_group(&reports[HIDS_OUTPUT_BASE], CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT,
				 param->output_report_ids, BT_HID_REPORT_TYPE_OUTPUT);
	if (err != 0) {
		return err;
	}
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
	err = setup_report_group(&reports[HIDS_FEATURE_BASE], CONFIG_BT_HIDS_FEATURE_REPORT_COUNT,
				 param->feature_report_ids, BT_HID_REPORT_TYPE_FEATURE);
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

	/* Hosts that are already connected get their HID state set up here,
	 * as they will not generate a connected callback anymore.
	 */
	bt_conn_foreach(BT_CONN_TYPE_LE, track_existing_conn, NULL);

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
	 * unregistered at runtime.
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(connections); i++) {
		struct bt_conn *conn = atomic_ptr_get(&connections[i].conn);

		if (conn != NULL) {
			conn_free(&connections[i], conn);
		}
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

int bt_hids_report_set(uint8_t report_type, uint8_t report_id, const uint8_t *data, uint16_t len)
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

int bt_hids_report_get(uint8_t report_type, uint8_t report_id, uint8_t *data, uint16_t *len)
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

	if (*len < ctx->value_len) {
		return -ENOMEM;
	}

	(void)memcpy(data, ctx->value, ctx->value_len);
	*len = ctx->value_len;

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

int bt_hids_get_protocol_mode(struct bt_conn *conn, enum bt_hid_protocol_mode *mode)
{
	const struct hids_conn *dc;

	if (conn == NULL || mode == NULL) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

	dc = conn_lookup(conn);
	if (dc == NULL) {
		return -ENOTCONN;
	}

	*mode = dc->protocol_mode;

	return 0;
}

int bt_hids_get_suspend_state(struct bt_conn *conn, bool *suspended)
{
	const struct hids_conn *dc;

	if (conn == NULL || suspended == NULL) {
		return -EINVAL;
	}

	if (!hids.registered) {
		return -ESRCH;
	}

	dc = conn_lookup(conn);
	if (dc == NULL) {
		return -ENOTCONN;
	}

	*suspended = dc->suspended;

	return 0;
}
