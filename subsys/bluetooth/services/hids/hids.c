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

BUILD_ASSERT(HIDS_REPORT_COUNT > 0,
	     "The HID Service needs at least one Report characteristic");

/* HOGP v1.1, Section 7.1: HID Service characteristics require an encrypted
 * link for reading, writing and notification.
 */
#define HIDS_PERM_READ  (BT_GATT_PERM_READ_ENCRYPT)
#define HIDS_PERM_WRITE (BT_GATT_PERM_WRITE_ENCRYPT)
#define HIDS_PERM_RW    (HIDS_PERM_READ | HIDS_PERM_WRITE)

/* Module context */
static const struct bt_hids_cb *callbacks;
static struct hids_state hids;
static struct hids_conn connections[CONFIG_BT_HIDS_MAX_CONNECTIONS];

#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
static struct hids_report_ctx input_reports[CONFIG_BT_HIDS_INPUT_REPORT_COUNT];
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
static struct hids_report_ctx output_reports[CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT];
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
static struct hids_report_ctx feature_reports[CONFIG_BT_HIDS_FEATURE_REPORT_COUNT];
#endif

struct hids_report_group {
	struct hids_report_ctx *ctx;
	uint8_t count;
	uint8_t type;
};

static const struct hids_report_group report_groups[] = {
#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
	{input_reports, ARRAY_SIZE(input_reports), BT_HID_REPORT_TYPE_INPUT},
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
	{output_reports, ARRAY_SIZE(output_reports), BT_HID_REPORT_TYPE_OUTPUT},
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
	{feature_reports, ARRAY_SIZE(feature_reports), BT_HID_REPORT_TYPE_FEATURE},
#endif
};

static struct hids_report_ctx *report_lookup(uint8_t report_id, uint8_t report_type)
{
	for (size_t i = 0U; i < ARRAY_SIZE(report_groups); i++) {
		const struct hids_report_group *group = &report_groups[i];

		if (group->type != report_type) {
			continue;
		}

		for (uint8_t j = 0U; j < group->count; j++) {
			if (group->ctx[j].ref.id == report_id) {
				return &group->ctx[j];
			}
		}
	}

	return NULL;
}

static struct hids_conn *conn_lookup(const struct bt_conn *conn)
{
	for (size_t i = 0U; i < ARRAY_SIZE(connections); i++) {
		if (connections[i].conn == conn) {
			return &connections[i];
		}
	}

	return NULL;
}

static struct hids_conn *conn_alloc(struct bt_conn *conn)
{
	for (size_t i = 0U; i < ARRAY_SIZE(connections); i++) {
		struct hids_conn *dc = &connections[i];

		if (dc->conn == NULL) {
			dc->conn = bt_conn_ref(conn);
			dc->protocol_mode = BT_HID_PROTOCOL_REPORT;
			dc->suspended = false;

			return dc;
		}
	}

	return NULL;
}

static void conn_free(struct hids_conn *dc)
{
	bt_conn_unref(dc->conn);
	dc->conn = NULL;
	dc->protocol_mode = BT_HID_PROTOCOL_REPORT;
	dc->suspended = false;
}

/* GATT callbacks */

#if defined(CONFIG_BT_HIDS_PROTOCOL_MODE)
static ssize_t read_protocol_mode(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const struct hids_conn *dc = conn_lookup(conn);
	uint8_t mode;

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	mode = (uint8_t)dc->protocol_mode;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &mode,
				sizeof(mode));
}

static ssize_t write_protocol_mode(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len,
				   uint16_t offset, uint8_t flags)
{
	struct hids_conn *dc = conn_lookup(conn);
	const uint8_t *val = buf;

	ARG_UNUSED(attr);

	if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
		/* Nothing to do until the Host executes the write */
		return 0;
	}

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (len != sizeof(uint8_t) || offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (*val > BT_HID_PROTOCOL_REPORT) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	dc->protocol_mode = (enum bt_hid_protocol_mode)*val;

	if (callbacks != NULL && callbacks->protocol_mode_changed != NULL) {
		callbacks->protocol_mode_changed(conn, dc->protocol_mode);
	}

	return len;
}
#endif /* CONFIG_BT_HIDS_PROTOCOL_MODE */

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids.report_map,
				hids.report_map_len);
}

static ssize_t read_hid_info(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hids.hid_info,
				sizeof(hids.hid_info));
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len,
				uint16_t offset, uint8_t flags)
{
	struct hids_conn *dc = conn_lookup(conn);
	const uint8_t *val = buf;

	ARG_UNUSED(attr);

	if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
		/* Nothing to do until the Host executes the write */
		return 0;
	}

	if (dc == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (len != sizeof(uint8_t) || offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (*val > BT_HIDS_CTRL_EXIT_SUSPEND) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	dc->suspended = (*val == BT_HIDS_CTRL_SUSPEND);

	if (callbacks != NULL && callbacks->suspend_changed != NULL) {
		callbacks->suspend_changed(conn, dc->suspended);
	}

	return len;
}

static ssize_t read_report_ref(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	const struct hids_report_ref *ref = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ref,
				sizeof(*ref));
}

static ssize_t read_report(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	const struct hids_report_ctx *ctx = attr->user_data;
	uint8_t tmp[CONFIG_BT_HIDS_MAX_REPORT_LEN];
	ssize_t ret;

	if (callbacks != NULL && callbacks->get_report != NULL) {
		ret = callbacks->get_report(conn, ctx->ref.type, ctx->ref.id,
					   tmp, sizeof(tmp));
		if (ret < 0) {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		if (ret > (ssize_t)sizeof(tmp)) {
			LOG_ERR("get_report returned %zd for a %zu byte buffer",
				ret, sizeof(tmp));
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		return bt_gatt_attr_read(conn, attr, buf, len, offset, tmp,
					(uint16_t)ret);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

/* Only Output and Feature Reports are writable */
#if (CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT + CONFIG_BT_HIDS_FEATURE_REPORT_COUNT) > 0
static ssize_t write_report(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	const struct hids_report_ctx *ctx = attr->user_data;

	if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
		/* Nothing to do until the Host executes the write */
		return 0;
	}

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (callbacks != NULL && callbacks->set_report != NULL) {
		callbacks->set_report(conn, ctx->ref.type, ctx->ref.id, buf,
				     len);
	}

	return len;
}
#endif

#if CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0
static ssize_t report_ccc_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				uint16_t value);

#define HIDS_CCC_USER_DATA(_n, _) \
	BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, report_ccc_write, NULL)

static struct bt_gatt_ccc_managed_user_data
	input_report_ccc[CONFIG_BT_HIDS_INPUT_REPORT_COUNT] = {
		LISTIFY(CONFIG_BT_HIDS_INPUT_REPORT_COUNT, HIDS_CCC_USER_DATA, (,))
	};

static ssize_t report_ccc_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				uint16_t value)
{
	const struct bt_gatt_ccc_managed_user_data *ccc = attr->user_data;
	size_t idx = (size_t)(ccc - input_report_ccc);

	if (idx >= ARRAY_SIZE(input_reports)) {
		return sizeof(value);
	}

	if (callbacks != NULL && callbacks->ccc_changed != NULL) {
		callbacks->ccc_changed(conn, input_reports[idx].ref.id,
				      input_reports[idx].ref.type,
				      (value & BT_GATT_CCC_NOTIFY) != 0U);
	}

	return sizeof(value);
}
#endif /* CONFIG_BT_HIDS_INPUT_REPORT_COUNT > 0 */

/* Service definition
 *
 * The number of Report characteristics of each type is a build time
 * configuration, so that the attribute table only contains characteristics
 * that the application actually uses. The Report ID of each characteristic is
 * assigned at registration time.
 */

#if defined(CONFIG_BT_HIDS_PROTOCOL_MODE)
#define HIDS_PROTOCOL_MODE_ATTRS \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_PROTOCOL_MODE, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
			       HIDS_PERM_RW, read_protocol_mode, write_protocol_mode, NULL),
#else
#define HIDS_PROTOCOL_MODE_ATTRS
#endif

#define HIDS_INPUT_REPORT_ATTRS(_n, _) \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       HIDS_PERM_READ, read_report, NULL, &input_reports[_n]), \
	BT_GATT_CCC_MANAGED(&input_report_ccc[_n], \
			    HIDS_PERM_READ | HIDS_PERM_WRITE), \
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, HIDS_PERM_READ, \
			   read_report_ref, NULL, &input_reports[_n].ref),

#define HIDS_OUTPUT_REPORT_ATTRS(_n, _) \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | \
				       BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
			       HIDS_PERM_RW, read_report, write_report, \
			       &output_reports[_n]), \
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, HIDS_PERM_READ, \
			   read_report_ref, NULL, &output_reports[_n].ref),

#define HIDS_FEATURE_REPORT_ATTRS(_n, _) \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE, \
			       HIDS_PERM_RW, read_report, write_report, \
			       &feature_reports[_n]), \
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, HIDS_PERM_READ, \
			   read_report_ref, NULL, &feature_reports[_n].ref),

#define BT_HIDS_SERVICE_DEFINITION() \
{ \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS), \
	HIDS_PROTOCOL_MODE_ATTRS \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ, \
			       HIDS_PERM_READ, read_report_map, NULL, NULL), \
	LISTIFY(CONFIG_BT_HIDS_INPUT_REPORT_COUNT, HIDS_INPUT_REPORT_ATTRS, ()) \
	LISTIFY(CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT, HIDS_OUTPUT_REPORT_ATTRS, ()) \
	LISTIFY(CONFIG_BT_HIDS_FEATURE_REPORT_COUNT, HIDS_FEATURE_REPORT_ATTRS, ()) \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ, \
			       HIDS_PERM_READ, read_hid_info, NULL, NULL), \
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT, \
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP, \
			       HIDS_PERM_WRITE, NULL, write_ctrl_point, NULL), \
}

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
	struct hids_conn *dc = conn_lookup(conn);

	ARG_UNUSED(reason);

	if (dc != NULL) {
		conn_free(dc);
	}
}

BT_CONN_CB_DEFINE(hids_conn_cb) = {
	.connected = hids_connected,
	.disconnected = hids_disconnected,
};

static void track_existing_conn(struct bt_conn *conn, void *data)
{
	ARG_UNUSED(data);

	if (conn_lookup(conn) != NULL) {
		return;
	}

	if (conn_alloc(conn) == NULL) {
		LOG_ERR("No free connection slot for an already connected Host");
	}
}

/* Registration */

static int setup_report_group(struct hids_report_ctx *ctx, uint8_t count,
			      const uint8_t *ids, uint8_t type)
{
	for (uint8_t i = 0U; i < count; i++) {
		/* HOGP v1.1, Section 3.1.1: two characteristics shall not have
		 * identical Report Reference descriptors.
		 */
		for (uint8_t j = 0U; j < i; j++) {
			if (ids[j] == ids[i]) {
				LOG_ERR("Duplicate Report ID %u type %u", ids[i],
					type);
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
	err = setup_report_group(input_reports, ARRAY_SIZE(input_reports),
				 param->input_report_ids,
				 BT_HID_REPORT_TYPE_INPUT);
	if (err != 0) {
		return err;
	}
#endif
#if CONFIG_BT_HIDS_OUTPUT_REPORT_COUNT > 0
	err = setup_report_group(output_reports, ARRAY_SIZE(output_reports),
				 param->output_report_ids,
				 BT_HID_REPORT_TYPE_OUTPUT);
	if (err != 0) {
		return err;
	}
#endif
#if CONFIG_BT_HIDS_FEATURE_REPORT_COUNT > 0
	err = setup_report_group(feature_reports, ARRAY_SIZE(feature_reports),
				 param->feature_report_ids,
				 BT_HID_REPORT_TYPE_FEATURE);
	if (err != 0) {
		return err;
	}
#endif

	/* Let every Report characteristic context point at its value
	 * attribute, so that notifications do not have to look it up.
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(hids_attrs); i++) {
		if (hids_attrs[i].read == read_report) {
			((struct hids_report_ctx *)hids_attrs[i].user_data)
				->attr = &hids_attrs[i];
		}
	}

	return 0;
}

/* Public API */

bool bt_hids_is_registered(void)
{
	return hids.registered;
}

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

	/* HOGP v1.1, Section 2.5: a composite HID Device that needs more than
	 * 512 octets to describe its functions uses multiple HID Service
	 * instances, which is not supported.
	 */
	if (param->report_map_len > BT_HIDS_REPORT_MAP_MAX_LEN) {
		LOG_ERR("Report Map is %u octets, at most %u per HID Service",
			param->report_map_len, BT_HIDS_REPORT_MAP_MAX_LEN);
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

	/* The GATT service is gone, so the per-Host HID state goes with it.
	 * Hosts stay connected and are notified through the Service Changed
	 * characteristic, like for any other GATT service that is
	 * unregistered at runtime.
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(connections); i++) {
		if (connections[i].conn != NULL) {
			conn_free(&connections[i]);
		}
	}

	hids.registered = false;
	callbacks = NULL;
	hids.report_map = NULL;
	hids.report_map_len = 0;

	return 0;
}

int bt_hids_send_report(struct bt_conn *conn, uint8_t report_id,
			const uint8_t *data, uint16_t len,
			bt_gatt_complete_func_t func, void *user_data)
{
	const struct hids_report_ctx *ctx;

	if (!hids.registered) {
		return -ESRCH;
	}

	ctx = report_lookup(report_id, BT_HID_REPORT_TYPE_INPUT);
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

int bt_hids_get_protocol_mode(struct bt_conn *conn,
			      enum bt_hid_protocol_mode *mode)
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
