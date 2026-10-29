/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hogp_host.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bt_hogp_host, CONFIG_BT_HOGP_HOST_LOG_LEVEL);

/* Report Reference descriptor: Report ID and Report Type */
#define REPORT_REF_LEN 2U

/* HID Information: bcdHID, bCountryCode and Flags */
#define HID_INFO_LEN 4U

/* Only the first HID Service of a Device is used */
#define SERVICE_COUNT 1U

enum hogp_state {
	STATE_FREE,
	STATE_DISCOVERING,
	STATE_READY,
	STATE_FAILED,
};

enum hogp_op {
	OP_NONE,
	OP_READ,
	OP_WRITE,
};

struct hogp_report {
	struct bt_gatt_subscribe_params sub;

	uint16_t value_handle;
	uint16_t end_handle;
	uint16_t ref_handle;
	uint16_t ccc_handle;
	uint8_t properties;
	uint8_t id;
	uint8_t type;
};

struct hogp_conn {
	struct bt_conn *conn;

	/* One GATT procedure runs at a time on a connection */
	union {
		struct bt_gatt_discover_params discover;
		struct bt_gatt_read_params read;
		struct bt_gatt_write_params write;
	} params;

	struct hogp_report reports[CONFIG_BT_HOGP_HOST_MAX_REPORTS];
	struct bt_hogp_host_hid_info hid_info;
	enum hogp_state state;
	enum hogp_op op;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t report_map_handle;
	uint16_t hid_info_handle;
	uint16_t report_map_len;
	uint16_t value_len;
	uint16_t op_len;

	uint8_t report_count;

	/* Report the discovery is working on */
	uint8_t cursor;

	/* Report the read or write is on */
	uint8_t op_report;

	/* Report Reference and HID Information values while they are read */
	uint8_t value[HID_INFO_LEN];

	uint8_t report_map[CONFIG_BT_HOGP_HOST_REPORT_MAP_MAX_LEN];
	uint8_t op_buf[CONFIG_BT_HOGP_HOST_MAX_REPORT_LEN];
};

static struct hogp_conn conns[CONFIG_BT_HOGP_HOST_MAX_CONNS];
static const struct bt_hogp_host_cb *hogp_cb;
static K_MUTEX_DEFINE(hogp_mutex);

static void hogp_lock(void)
{
	(void)k_mutex_lock(&hogp_mutex, K_FOREVER);
}

static void hogp_unlock(void)
{
	(void)k_mutex_unlock(&hogp_mutex);
}

/* Assert that the current thread holds hogp_mutex. k_mutex resets the owner on
 * the final unlock, so the owner check alone is exact.
 */
#define HOGP_LOCK_ASSERT()                                                                         \
	__ASSERT(hogp_mutex.owner == k_current_get(), "hogp_mutex not held by %p", k_current_get())

/* Must be called with the lock held */
static struct hogp_conn *conn_find(const struct bt_conn *conn)
{
	HOGP_LOCK_ASSERT();

	ARRAY_FOR_EACH_PTR(conns, c) {
		if (c->conn == conn) {
			return c;
		}
	}

	return NULL;
}

/* Must be called with the lock held. Returns NULL unless the discovery succeeded. */
static struct hogp_conn *conn_ready(const struct bt_conn *conn, uint8_t service_index, int *err)
{
	struct hogp_conn *c;

	HOGP_LOCK_ASSERT();

	c = conn_find(conn);
	if (c == NULL || c->state != STATE_READY) {
		*err = -ENOENT;
		return NULL;
	}

	if (service_index >= SERVICE_COUNT) {
		*err = -EINVAL;
		return NULL;
	}

	return c;
}

/* Must be called with the lock held */
static struct hogp_report *report_find(struct hogp_conn *c, enum bt_hids_report_type type,
				       uint8_t report_id)
{
	HOGP_LOCK_ASSERT();

	for (uint8_t i = 0U; i < c->report_count; i++) {
		struct hogp_report *report = &c->reports[i];

		if (report->type == type && report->id == report_id) {
			return report;
		}
	}

	return NULL;
}

/* Drops the state of a connection, if it still has one */
static void conn_release(struct bt_conn *conn)
{
	struct hogp_conn *c;

	hogp_lock();

	c = conn_find(conn);
	if (c == NULL) {
		hogp_unlock();
		return;
	}

	(void)memset(c, 0, sizeof(*c));

	hogp_unlock();

	/* The reference taken by bt_hogp_host_discover() */
	bt_conn_unref(conn);
}

static void discovery_done(struct hogp_conn *c, int err)
{
	const struct bt_hogp_host_cb *cb;
	struct bt_conn *conn;

	hogp_lock();
	c->state = (err == 0) ? STATE_READY : STATE_FAILED;
	conn = c->conn;
	cb = hogp_cb;
	hogp_unlock();

	LOG_DBG("conn %p err %d", (void *)conn, err);

	if (cb != NULL && cb->discovered != NULL) {
		cb->discovered(conn, err);
	}
}

static void subscribe_next(struct hogp_conn *c);

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			 const void *data, uint16_t length)
{
	const struct hogp_report *report = CONTAINER_OF(params, struct hogp_report, sub);
	const struct bt_hogp_host_cb *cb = hogp_cb;

	if (data == NULL) {
		/* Unsubscribed, the connection is going away */
		return BT_GATT_ITER_STOP;
	}

	if (cb != NULL && cb->input_report != NULL) {
		cb->input_report(conn, 0U, report->id, data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void subscribe_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	struct hogp_report *report = CONTAINER_OF(params, struct hogp_report, sub);
	struct hogp_conn *c;

	hogp_lock();
	c = conn_find(conn);
	hogp_unlock();

	if (c == NULL || c->state != STATE_DISCOVERING) {
		return;
	}

	if (err != 0U) {
		LOG_DBG("Subscribing to handle 0x%04x failed: 0x%02x", report->value_handle, err);
		discovery_done(c, err);
		return;
	}

	c->cursor++;
	subscribe_next(c);
}

static void subscribe_next(struct hogp_conn *c)
{
	int err;

	for (; c->cursor < c->report_count; c->cursor++) {
		struct hogp_report *report = &c->reports[c->cursor];

		if (report->type != BT_HIDS_REPORT_TYPE_INPUT) {
			continue;
		}

		report->sub.notify = notify_cb;
		report->sub.subscribe = subscribe_cb;
		report->sub.value = BT_GATT_CCC_NOTIFY;
		report->sub.value_handle = report->value_handle;
		report->sub.ccc_handle = report->ccc_handle;
		/* The subscription lives in the connection state */
		atomic_set_bit(report->sub.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

		err = bt_gatt_subscribe(c->conn, &report->sub);
		if (err != 0) {
			LOG_DBG("bt_gatt_subscribe failed: %d", err);
			discovery_done(c, err);
		}

		return;
	}

	discovery_done(c, 0);
}

static uint8_t report_map_read_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params, const void *data,
				  uint16_t length)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.read);

	ARG_UNUSED(conn);

	if (err != 0U) {
		LOG_DBG("Reading the Report Map failed: 0x%02x", err);
		discovery_done(c, err);
		return BT_GATT_ITER_STOP;
	}

	if (data == NULL) {
		c->cursor = 0U;
		subscribe_next(c);
		return BT_GATT_ITER_STOP;
	}

	if (length > sizeof(c->report_map) - c->report_map_len) {
		LOG_DBG("The Report Map is longer than %zu octets", sizeof(c->report_map));
		discovery_done(c, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	(void)memcpy(&c->report_map[c->report_map_len], data, length);
	c->report_map_len += length;

	return BT_GATT_ITER_CONTINUE;
}

static void report_map_read(struct hogp_conn *c)
{
	int err;

	c->report_map_len = 0U;

	(void)memset(&c->params.read, 0, sizeof(c->params.read));
	c->params.read.func = report_map_read_cb;
	c->params.read.handle_count = 1U;
	c->params.read.single.handle = c->report_map_handle;

	err = bt_gatt_read(c->conn, &c->params.read);
	if (err != 0) {
		LOG_DBG("bt_gatt_read failed: %d", err);
		discovery_done(c, err);
	}
}

/* Collects a short value in c->value. Returns false once the discovery is done. */
static bool value_collect(struct hogp_conn *c, uint8_t err, const void *data, uint16_t length)
{
	if (err != 0U) {
		LOG_DBG("Reading handle 0x%04x failed: 0x%02x", c->params.read.single.handle, err);
		discovery_done(c, err);
		return false;
	}

	if (data == NULL) {
		return true;
	}

	if (length > sizeof(c->value) - c->value_len) {
		discovery_done(c, -EBADMSG);
		return false;
	}

	(void)memcpy(&c->value[c->value_len], data, length);
	c->value_len += length;

	return true;
}

static int value_read(struct hogp_conn *c, uint16_t handle, bt_gatt_read_func_t func)
{
	c->value_len = 0U;

	(void)memset(&c->params.read, 0, sizeof(c->params.read));
	c->params.read.func = func;
	c->params.read.handle_count = 1U;
	c->params.read.single.handle = handle;

	return bt_gatt_read(c->conn, &c->params.read);
}

static uint8_t hid_info_read_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_read_params *params, const void *data,
				uint16_t length)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.read);

	ARG_UNUSED(conn);

	if (!value_collect(c, err, data, length)) {
		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (c->value_len != HID_INFO_LEN) {
		LOG_DBG("HID Information of %u octets", c->value_len);
		discovery_done(c, -EBADMSG);
		return BT_GATT_ITER_STOP;
	}

	c->hid_info.bcd_hid = sys_get_le16(&c->value[0]);
	c->hid_info.country_code = c->value[2];
	c->hid_info.flags = c->value[3];

	report_map_read(c);

	return BT_GATT_ITER_STOP;
}

static void report_ref_read_next(struct hogp_conn *c);

static uint8_t report_ref_read_cb(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params, const void *data,
				  uint16_t length)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.read);
	struct hogp_report *report = &c->reports[c->cursor];

	ARG_UNUSED(conn);

	if (!value_collect(c, err, data, length)) {
		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (c->value_len != REPORT_REF_LEN) {
		LOG_DBG("Report Reference of %u octets", c->value_len);
		discovery_done(c, -EBADMSG);
		return BT_GATT_ITER_STOP;
	}

	report->id = c->value[0];
	report->type = c->value[1];

	if (report->type < BT_HIDS_REPORT_TYPE_INPUT ||
	    report->type > BT_HIDS_REPORT_TYPE_FEATURE) {
		LOG_DBG("Report Type 0x%02x", report->type);
		discovery_done(c, -EBADMSG);
		return BT_GATT_ITER_STOP;
	}

	if (report->type == BT_HIDS_REPORT_TYPE_INPUT && report->ccc_handle == 0U) {
		LOG_DBG("Input Report 0x%04x has no CCC", report->value_handle);
		discovery_done(c, -ENOENT);
		return BT_GATT_ITER_STOP;
	}

	c->cursor++;
	report_ref_read_next(c);

	return BT_GATT_ITER_STOP;
}

static void report_ref_read_next(struct hogp_conn *c)
{
	int err;

	if (c->cursor < c->report_count) {
		err = value_read(c, c->reports[c->cursor].ref_handle, report_ref_read_cb);
	} else {
		err = value_read(c, c->hid_info_handle, hid_info_read_cb);
	}

	if (err != 0) {
		LOG_DBG("bt_gatt_read failed: %d", err);
		discovery_done(c, err);
	}
}

static struct hogp_report *report_owning(struct hogp_conn *c, uint16_t handle)
{
	for (uint8_t i = 0U; i < c->report_count; i++) {
		struct hogp_report *report = &c->reports[i];

		if (handle > report->value_handle && handle <= report->end_handle) {
			return report;
		}
	}

	return NULL;
}

static uint8_t desc_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.discover);
	struct hogp_report *report;

	ARG_UNUSED(conn);

	if (attr == NULL) {
		for (uint8_t i = 0U; i < c->report_count; i++) {
			if (c->reports[i].ref_handle == 0U) {
				LOG_DBG("Report 0x%04x has no Report Reference",
					c->reports[i].value_handle);
				discovery_done(c, -ENOENT);
				return BT_GATT_ITER_STOP;
			}
		}

		c->cursor = 0U;
		report_ref_read_next(c);
		return BT_GATT_ITER_STOP;
	}

	report = report_owning(c, attr->handle);
	if (report == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (bt_uuid_cmp(attr->uuid, BT_UUID_HIDS_REPORT_REF) == 0) {
		report->ref_handle = attr->handle;
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
		report->ccc_handle = attr->handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void desc_discover(struct hogp_conn *c)
{
	int err;

	if (c->report_count == 0U) {
		c->cursor = 0U;
		report_ref_read_next(c);
		return;
	}

	(void)memset(&c->params.discover, 0, sizeof(c->params.discover));
	c->params.discover.func = desc_discover_cb;
	c->params.discover.type = BT_GATT_DISCOVER_DESCRIPTOR;
	c->params.discover.start_handle = c->reports[0].value_handle + 1U;
	c->params.discover.end_handle = c->end_handle;

	err = bt_gatt_discover(c->conn, &c->params.discover);
	if (err != 0) {
		LOG_DBG("bt_gatt_discover failed: %d", err);
		discovery_done(c, err);
	}
}

static uint8_t chrc_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.discover);
	const struct bt_gatt_chrc *chrc;
	struct hogp_report *report;

	ARG_UNUSED(conn);

	/* The last Report found so far ends where the next characteristic starts */
	if (c->report_count > 0U) {
		report = &c->reports[c->report_count - 1U];
		if (report->end_handle == 0U) {
			report->end_handle = (attr != NULL) ? attr->handle - 1U : c->end_handle;
		}
	}

	if (attr == NULL) {
		if (c->report_map_handle == 0U || c->hid_info_handle == 0U) {
			LOG_DBG("No Report Map or HID Information");
			discovery_done(c, -ENOENT);
			return BT_GATT_ITER_STOP;
		}

		desc_discover(c);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_REPORT) == 0) {
		if (c->report_count == ARRAY_SIZE(c->reports)) {
			LOG_DBG("More than %zu Reports", ARRAY_SIZE(c->reports));
			discovery_done(c, -ENOMEM);
			return BT_GATT_ITER_STOP;
		}

		report = &c->reports[c->report_count];
		report->value_handle = chrc->value_handle;
		report->properties = chrc->properties;
		c->report_count++;
	} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_REPORT_MAP) == 0) {
		if (c->report_map_handle == 0U) {
			c->report_map_handle = chrc->value_handle;
		}
	} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_INFO) == 0) {
		if (c->hid_info_handle == 0U) {
			c->hid_info_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t service_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   struct bt_gatt_discover_params *params)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.discover);
	const struct bt_gatt_service_val *service;
	int err;

	if (attr == NULL) {
		LOG_DBG("No HID Service");
		discovery_done(c, -ENOENT);
		return BT_GATT_ITER_STOP;
	}

	service = attr->user_data;
	c->start_handle = attr->handle;
	c->end_handle = service->end_handle;

	(void)memset(&c->params.discover, 0, sizeof(c->params.discover));
	c->params.discover.func = chrc_discover_cb;
	c->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	c->params.discover.start_handle = c->start_handle + 1U;
	c->params.discover.end_handle = c->end_handle;

	err = bt_gatt_discover(conn, &c->params.discover);
	if (err != 0) {
		LOG_DBG("bt_gatt_discover failed: %d", err);
		discovery_done(c, err);
	}

	return BT_GATT_ITER_STOP;
}

int bt_hogp_host_cb_register(const struct bt_hogp_host_cb *cb)
{
	int err = 0;

	if (cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	hogp_lock();
	if (hogp_cb == NULL) {
		hogp_cb = cb;
	} else {
		err = -EALREADY;
	}
	hogp_unlock();

	return err;
}

int bt_hogp_host_discover(struct bt_conn *conn)
{
	struct bt_conn_info info;
	struct hogp_conn *c;
	int err;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");
		return -EINVAL;
	}

	err = bt_conn_get_info(conn, &info);
	if (err != 0 || info.type != BT_CONN_TYPE_LE) {
		LOG_DBG("Not an LE connection");
		return -EINVAL;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	hogp_lock();

	if (conn_find(conn) != NULL) {
		hogp_unlock();
		return -EALREADY;
	}

	c = conn_find(NULL);
	if (c == NULL) {
		hogp_unlock();
		LOG_DBG("No free connection state");
		return -ENOMEM;
	}

	(void)memset(c, 0, sizeof(*c));
	c->conn = bt_conn_ref(conn);
	c->state = STATE_DISCOVERING;

	c->params.discover.func = service_discover_cb;
	c->params.discover.uuid = BT_UUID_HIDS;
	c->params.discover.type = BT_GATT_DISCOVER_PRIMARY;
	c->params.discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	c->params.discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	hogp_unlock();

	err = bt_gatt_discover(conn, &c->params.discover);
	if (err != 0) {
		LOG_DBG("bt_gatt_discover failed: %d", err);
		conn_release(conn);
	}

	return err;
}

int bt_hogp_host_service_count_get(struct bt_conn *conn, uint8_t *count)
{
	int err = 0;

	if (conn == NULL || count == NULL) {
		LOG_DBG("conn or count is NULL");
		return -EINVAL;
	}

	hogp_lock();
	if (conn_ready(conn, 0U, &err) != NULL) {
		*count = SERVICE_COUNT;
	}
	hogp_unlock();

	return err;
}

int bt_hogp_host_hid_info_get(struct bt_conn *conn, uint8_t service_index,
			      struct bt_hogp_host_hid_info *info)
{
	struct hogp_conn *c;
	int err = 0;

	if (conn == NULL || info == NULL) {
		LOG_DBG("conn or info is NULL");
		return -EINVAL;
	}

	hogp_lock();
	c = conn_ready(conn, service_index, &err);
	if (c != NULL) {
		*info = c->hid_info;
	}
	hogp_unlock();

	return err;
}

int bt_hogp_host_report_map_get(struct bt_conn *conn, uint8_t service_index, uint8_t *data,
				uint16_t *len)
{
	struct hogp_conn *c;
	int err = 0;

	if (conn == NULL || data == NULL || len == NULL) {
		LOG_DBG("conn, data or len is NULL");
		return -EINVAL;
	}

	hogp_lock();

	c = conn_ready(conn, service_index, &err);
	if (c == NULL) {
		hogp_unlock();
		return err;
	}

	if (*len >= c->report_map_len) {
		(void)memcpy(data, c->report_map, c->report_map_len);
	} else {
		err = -ENOMEM;
	}

	*len = c->report_map_len;

	hogp_unlock();

	return err;
}

int bt_hogp_host_reports_get(struct bt_conn *conn, uint8_t service_index,
			     struct bt_hogp_host_report_info *reports, uint8_t *count)
{
	struct hogp_conn *c;
	int err = 0;

	if (conn == NULL || reports == NULL || count == NULL) {
		LOG_DBG("conn, reports or count is NULL");
		return -EINVAL;
	}

	hogp_lock();

	c = conn_ready(conn, service_index, &err);
	if (c == NULL) {
		hogp_unlock();
		return err;
	}

	if (*count >= c->report_count) {
		for (uint8_t i = 0U; i < c->report_count; i++) {
			reports[i].type = c->reports[i].type;
			reports[i].id = c->reports[i].id;
			reports[i].properties = c->reports[i].properties;
		}
	} else {
		err = -ENOMEM;
	}

	*count = c->report_count;

	hogp_unlock();

	return err;
}

/* Ends a read or write and returns the Report it was on */
static struct hogp_report *op_end(struct hogp_conn *c, const struct bt_conn *conn)
{
	struct hogp_report *report;

	hogp_lock();

	report = &c->reports[c->op_report];

	/* The state is gone if the connection went away in the meantime */
	if (c->conn == conn) {
		c->op = OP_NONE;
	}

	hogp_unlock();

	return report;
}

static uint8_t report_read_cb(struct bt_conn *conn, uint8_t att_err,
			      struct bt_gatt_read_params *params, const void *data, uint16_t length)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.read);
	const struct bt_hogp_host_cb *cb = hogp_cb;
	const struct hogp_report *report;
	int err = att_err;
	uint16_t len;

	if (err == 0 && data != NULL) {
		if (length <= sizeof(c->op_buf) - c->op_len) {
			(void)memcpy(&c->op_buf[c->op_len], data, length);
			c->op_len += length;
			return BT_GATT_ITER_CONTINUE;
		}

		err = -ENOMEM;
	}

	/* Only this callback writes op_buf, so it stays valid after op_end() */
	len = c->op_len;
	report = op_end(c, conn);

	if (cb != NULL && cb->report_read != NULL) {
		if (err == 0) {
			cb->report_read(conn, 0U, report->type, report->id, 0, c->op_buf, len);
		} else {
			cb->report_read(conn, 0U, report->type, report->id, err, NULL, 0U);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void report_write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	struct hogp_conn *c = CONTAINER_OF(params, struct hogp_conn, params.write);
	const struct bt_hogp_host_cb *cb = hogp_cb;
	const struct hogp_report *report = op_end(c, conn);

	if (cb != NULL && cb->report_written != NULL) {
		cb->report_written(conn, 0U, report->type, report->id, err);
	}
}

/* Claims the connection for a read or write. Returns NULL and sets err on failure. */
static struct hogp_conn *op_start(struct bt_conn *conn, uint8_t service_index,
				  enum bt_hids_report_type type, uint8_t report_id, enum hogp_op op,
				  uint8_t required_props, int *err)
{
	struct hogp_report *report;
	struct hogp_conn *c;

	hogp_lock();

	c = conn_ready(conn, service_index, err);
	if (c == NULL) {
		hogp_unlock();
		return NULL;
	}

	report = report_find(c, type, report_id);
	if (report == NULL) {
		hogp_unlock();
		*err = -ENOENT;
		return NULL;
	}

	if ((report->properties & required_props) == 0U) {
		hogp_unlock();
		*err = -ENOTSUP;
		return NULL;
	}

	if (c->op != OP_NONE) {
		hogp_unlock();
		*err = -EBUSY;
		return NULL;
	}

	c->op = op;
	c->op_report = report - c->reports;
	c->op_len = 0U;

	if (op == OP_READ) {
		(void)memset(&c->params.read, 0, sizeof(c->params.read));
		c->params.read.func = report_read_cb;
		c->params.read.handle_count = 1U;
		c->params.read.single.handle = report->value_handle;
	} else {
		(void)memset(&c->params.write, 0, sizeof(c->params.write));
		c->params.write.func = report_write_cb;
		c->params.write.handle = report->value_handle;
	}

	hogp_unlock();

	return c;
}

int bt_hogp_host_report_read(struct bt_conn *conn, uint8_t service_index,
			     enum bt_hids_report_type type, uint8_t report_id)
{
	struct hogp_conn *c;
	int err = 0;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");
		return -EINVAL;
	}

	c = op_start(conn, service_index, type, report_id, OP_READ, BT_GATT_CHRC_READ, &err);
	if (c == NULL) {
		return err;
	}

	err = bt_gatt_read(conn, &c->params.read);
	if (err != 0) {
		LOG_DBG("bt_gatt_read failed: %d", err);
		(void)op_end(c, conn);
	}

	return err;
}

int bt_hogp_host_report_write(struct bt_conn *conn, uint8_t service_index,
			      enum bt_hids_report_type type, uint8_t report_id, const uint8_t *data,
			      uint16_t len)
{
	struct hogp_conn *c;
	int err = 0;

	if (conn == NULL || data == NULL || len == 0U || len > BT_ATT_MAX_ATTRIBUTE_LEN) {
		LOG_DBG("conn or data is NULL, or len %u is out of range", len);
		return -EINVAL;
	}

	c = op_start(conn, service_index, type, report_id, OP_WRITE, BT_GATT_CHRC_WRITE, &err);
	if (c == NULL) {
		return err;
	}

	c->params.write.data = data;
	c->params.write.length = len;

	err = bt_gatt_write(conn, &c->params.write);
	if (err != 0) {
		LOG_DBG("bt_gatt_write failed: %d", err);
		(void)op_end(c, conn);
	}

	return err;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	/* The GATT client has already ended the procedures and removed the
	 * subscriptions of the connection, so the state can be dropped.
	 */
	conn_release(conn);
}

BT_CONN_CB_DEFINE(hogp_host_conn_cb) = {
	.disconnected = disconnected,
};
