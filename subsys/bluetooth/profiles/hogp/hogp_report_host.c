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
#include <zephyr/bluetooth/profiles/hogp.h>
#include <zephyr/bluetooth/services/hids.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bt_hogp_report_host, CONFIG_BT_HOGP_REPORT_HOST_LOG_LEVEL);

/* Report Reference descriptor: Report ID and Report Type */
#define REPORT_REF_LEN 2U

/* HID Information: bcdHID, bCountryCode and Flags */
#define HID_INFO_LEN 4U

/* Only the first HID Service of a HID Device is used */
#define SERVICE_COUNT 1U

enum hogp_state {
	/* No discovery is running or has succeeded on the connection */
	HOGP_STATE_IDLE,
	HOGP_STATE_DISCOVERING,
	HOGP_STATE_READY,
};

enum hogp_req {
	HOGP_REQ_NONE,
	HOGP_REQ_READ,
	HOGP_REQ_WRITE,
};

struct hogp_report {
	uint16_t value_handle;
	uint16_t end_handle;
	uint16_t ref_handle;
	uint16_t ccc_handle;
	uint8_t properties;
	uint8_t id;
	uint8_t type;
};

/* Subscription to the Input Report at the same index of the Report table */
struct hogp_sub {
	struct bt_gatt_subscribe_params params;

	/* Whether the GATT client holds params */
	bool active;
};

struct bt_hogp_report_host_hids {
	struct hogp_report reports[CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORTS];

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t report_map_handle;
	uint16_t hid_info_handle;

	uint8_t report_count;
};

struct hogp_client {
	struct bt_conn *conn;

	/* One GATT procedure runs at a time on a connection */
	union {
		struct bt_gatt_discover_params discover;
		struct bt_gatt_read_params read;
		struct bt_gatt_write_params write;
	} params;

	struct bt_hogp_report_host_hids hids[SERVICE_COUNT];

	/* What bt_hogp_report_host_services.hids points to */
	struct bt_hogp_report_host_hids *hids_list[SERVICE_COUNT];

	enum hogp_state state;
	enum hogp_req req;

	/* Length of the Report Map read so far */
	uint16_t report_map_len;
	uint16_t value_len;
	uint16_t req_len;

	/* Report the discovery is working on */
	uint8_t cursor;

	/* Report the read or write is on */
	uint8_t req_report;

	/* Report Reference and HID Information values while they are read */
	uint8_t value[HID_INFO_LEN];

	uint8_t req_buf[CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORT_LEN];

	/* Kept when the discovery runs again: a failed discovery leaves its subscriptions with the
	 * GATT client, which holds them until the connection goes away. Must stay last.
	 */
	struct hogp_sub subs[CONFIG_BT_HOGP_REPORT_HOST_MAX_REPORTS];
};

static struct hogp_client hogp_clients[CONFIG_BT_MAX_CONN];
static sys_slist_t hogp_cbs = SYS_SLIST_STATIC_INIT(&hogp_cbs);
static K_MUTEX_DEFINE(hogp_mutex);

static void hogp_lock(void)
{
	__maybe_unused int err = k_mutex_lock(&hogp_mutex, K_FOREVER);

	__ASSERT(err == 0, "Failed to lock the HOGP Report Host (err %d)", err);
}

static void hogp_unlock(void)
{
	__maybe_unused int err = k_mutex_unlock(&hogp_mutex);

	__ASSERT(err == 0, "Failed to unlock the HOGP Report Host (err %d)", err);
}

/* Assert that the current thread holds hogp_mutex. k_mutex resets the owner on the final unlock,
 * so the owner check alone is exact.
 */
#define HOGP_LOCK_ASSERT()                                                                         \
	__ASSERT(hogp_mutex.owner == k_current_get(), "hogp_mutex not held by %p", k_current_get())

static struct hogp_client *client_get(const struct bt_conn *conn)
{
	__ASSERT(bt_conn_is_type(conn, BT_CONN_TYPE_LE), "Not an LE connection %p", conn);

	return &hogp_clients[bt_conn_index(conn)];
}

/* Must be called with the lock held. Returns the client of hids if its connection is still there,
 * otherwise NULL with err set.
 */
static struct hogp_client *hids_client_find(const struct bt_hogp_report_host_hids *hids, int *err)
{
	HOGP_LOCK_ASSERT();

	if (hids == NULL) {
		*err = -EINVAL;
		return NULL;
	}

	ARRAY_FOR_EACH_PTR(hogp_clients, c) {
		if (!IS_ARRAY_ELEMENT(c->hids, hids)) {
			continue;
		}

		if (c->state == HOGP_STATE_IDLE) {
			*err = -ENOTCONN;
			return NULL;
		}

		return c;
	}

	LOG_DBG("hids %p is not a HID Service of this profile", (const void *)hids);
	*err = -EINVAL;

	return NULL;
}

/* Must be called with the lock held */
static struct hogp_report *report_find(struct bt_hogp_report_host_hids *hids,
				       enum bt_hids_report_type type, uint8_t report_id)
{
	HOGP_LOCK_ASSERT();

	for (uint8_t i = 0U; i < hids->report_count; i++) {
		struct hogp_report *report = &hids->reports[i];

		if (report->type == type && report->id == report_id) {
			return report;
		}
	}

	return NULL;
}

/* The HID Service that holds report */
static struct bt_hogp_report_host_hids *report_hids_get(struct hogp_client *c,
							const struct hogp_report *report)
{
	ARRAY_FOR_EACH_PTR(c->hids, hids) {
		if (IS_ARRAY_ELEMENT(hids->reports, report)) {
			return hids;
		}
	}

	__ASSERT(false, "Report %p of no HID Service", (const void *)report);

	return NULL;
}

/* Whether the connection of c is still up. A GATT procedure that ends with an error once it is not
 * reports -ENOTCONN.
 */
static bool conn_is_connected(const struct bt_conn *conn)
{
	struct bt_conn_info info;

	return bt_conn_get_info(conn, &info) == 0 && info.state == BT_CONN_STATE_CONNECTED;
}

static bool client_is_connected(const struct hogp_client *c)
{
	return conn_is_connected(c->conn);
}

/* Ends a failed discovery. Its subscriptions stay with the GATT client, report_notify_cb() drops
 * their notifications, and the discovery can run again on the connection.
 */
static void discovery_fail(struct hogp_client *c, int err)
{
	struct bt_hogp_report_host_cb *cb;
	struct bt_conn *conn;

	hogp_lock();
	conn = bt_conn_take(&c->conn);
	c->state = HOGP_STATE_IDLE;
	c->req = HOGP_REQ_NONE;
	hogp_unlock();

	LOG_DBG("conn %p err %d", (void *)conn, err);

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, cb, _node) {
		if (cb->discovered != NULL) {
			cb->discovered(conn, err, NULL);
		}
	}

	bt_conn_unref(conn);
}

/* Called on the Bluetooth RX thread, like the other GATT callbacks of the discovery, so the
 * discovery state of c is only changed from there.
 */
static void discovery_done(struct hogp_client *c, int err)
{
	struct bt_hogp_report_host_services services = {0};
	struct bt_hogp_report_host_cb *cb;
	struct bt_conn *conn;

	if (!client_is_connected(c)) {
		LOG_DBG("The connection went away during the discovery");
		err = -ENOTCONN;
	}

	/* HOGP 8.1: the characteristics of the HID Service require an encrypted link */
	if (err == 0 && bt_conn_get_security(c->conn) < BT_SECURITY_L2) {
		LOG_WRN("The link to the HID Device is not encrypted");
		err = -EPERM;
	}

	if (err != 0) {
		discovery_fail(c, err);
		return;
	}

	hogp_lock();

	ARRAY_FOR_EACH(c->hids, i) {
		c->hids_list[i] = &c->hids[i];
	}

	services.count = SERVICE_COUNT;
	services.hids = c->hids_list;
	c->state = HOGP_STATE_READY;
	conn = bt_conn_ref(c->conn);

	hogp_unlock();

	LOG_DBG("conn %p discovered", (void *)conn);

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, cb, _node) {
		if (cb->discovered != NULL) {
			cb->discovered(conn, 0, &services);
		}
	}

	bt_conn_unref(conn);
}

static void subscribe_next(struct hogp_client *c);

static uint8_t report_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				const void *data, uint16_t length)
{
	struct hogp_sub *sub = CONTAINER_OF(params, struct hogp_sub, params);
	struct hogp_client *c = client_get(conn);
	size_t i = sub - c->subs;
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	struct hogp_report *report = &hids->reports[i];
	struct bt_hogp_report_host_cb *cb;
	bool ready;

	if (data == NULL) {
		/* The GATT client no longer holds the subscription parameters */
		sub->active = false;
		return BT_GATT_ITER_STOP;
	}

	/* The HID Service is handed to the application once the discovery succeeds */
	hogp_lock();
	ready = (c->conn == conn && c->state == HOGP_STATE_READY);
	hogp_unlock();

	if (!ready) {
		return BT_GATT_ITER_CONTINUE;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, cb, _node) {
		if (cb->input_report != NULL) {
			cb->input_report(hids, report->id, data, length);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void report_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_subscribe_params *params)
{
	struct hogp_client *c = client_get(conn);
	bool discovering;

	hogp_lock();
	discovering = (c->conn == conn && c->state == HOGP_STATE_DISCOVERING);
	hogp_unlock();

	if (!discovering) {
		return;
	}

	if (att_err != 0U) {
		/* The GATT client removes the subscription and calls report_notify_cb() with NULL
		 * data after this returns.
		 */
		discovery_done(c, att_err);
		return;
	}

	c->cursor++;
	subscribe_next(c);
}

static void subscribe_next(struct hogp_client *c)
{
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	int err;

	for (; c->cursor < hids->report_count; c->cursor++) {
		struct hogp_report *report = &hids->reports[c->cursor];
		struct hogp_sub *sub = &c->subs[c->cursor];

		if (report->type != BT_HIDS_REPORT_TYPE_INPUT) {
			continue;
		}

		if (!sub->active) {
			(void)memset(&sub->params, 0, sizeof(sub->params));
			sub->params.notify = report_notify_cb;
			sub->params.subscribe = report_subscribe_cb;
			sub->params.value = BT_GATT_CCC_NOTIFY;
			sub->params.value_handle = report->value_handle;
			sub->params.ccc_handle = report->ccc_handle;
			/* HOGP 8.1: notifications require an encrypted link */
			sub->params.min_security = BT_SECURITY_L2;
			/* The subscription lives in the connection state */
			atomic_set_bit(sub->params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
		}

		err = bt_gatt_subscribe(c->conn, &sub->params);
		if (err == -EALREADY) {
			/* Still subscribed since a failed discovery */
			continue;
		}

		if (err != 0) {
			LOG_DBG("bt_gatt_subscribe failed: %d", err);
			discovery_done(c, err);
			return;
		}

		sub->active = true;
		return;
	}

	discovery_done(c, 0);
}

static uint8_t report_map_read_cb(struct bt_conn *conn, uint8_t att_err,
				  struct bt_gatt_read_params *params, const void *data,
				  uint16_t length)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.read);
	struct bt_hogp_report_host_cb *cb;
	uint16_t offset;

	ARG_UNUSED(conn);

	if (att_err != 0U) {
		discovery_done(c, att_err);
		return BT_GATT_ITER_STOP;
	}

	if (data == NULL) {
		const struct bt_hogp_report_host_hids *hids = &c->hids[0];

		/* A failed discovery leaves its subscriptions with the GATT client, which never
		 * calls back for a second subscription to the same value handle, so each one has
		 * to be for the Report at its index.
		 */
		ARRAY_FOR_EACH(c->subs, i) {
			const struct hogp_sub *sub = &c->subs[i];

			if (sub->active &&
			    (i >= hids->report_count ||
			     hids->reports[i].value_handle != sub->params.value_handle)) {
				LOG_WRN("Report 0x%04x moved", sub->params.value_handle);
				discovery_done(c, -EBADMSG);
				return BT_GATT_ITER_STOP;
			}
		}

		c->cursor = 0U;
		subscribe_next(c);
		return BT_GATT_ITER_STOP;
	}

	/* The HID Service limits the Report Map to the length of an attribute value */
	if (length > BT_ATT_MAX_ATTRIBUTE_LEN - c->report_map_len) {
		LOG_WRN("The Report Map is longer than %u octets", BT_ATT_MAX_ATTRIBUTE_LEN);
		discovery_done(c, -EBADMSG);
		return BT_GATT_ITER_STOP;
	}

	offset = c->report_map_len;
	c->report_map_len += length;

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, cb, _node) {
		if (cb->report_map != NULL) {
			cb->report_map(&c->hids[0], offset, data, length);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void report_map_read(struct hogp_client *c)
{
	int err;

	c->report_map_len = 0U;

	(void)memset(&c->params.read, 0, sizeof(c->params.read));
	c->params.read.func = report_map_read_cb;
	c->params.read.handle_count = 1U;
	c->params.read.single.handle = c->hids[0].report_map_handle;

	err = bt_gatt_read(c->conn, &c->params.read);
	if (err != 0) {
		LOG_DBG("bt_gatt_read failed: %d", err);
		discovery_done(c, err);
	}
}

/* Collects a short value in c->value. Returns false once the discovery is done. */
static bool value_collect(struct hogp_client *c, uint8_t att_err, const void *data, uint16_t length)
{
	if (att_err != 0U) {
		discovery_done(c, att_err);
		return false;
	}

	if (data == NULL) {
		return true;
	}

	if (length > sizeof(c->value) - c->value_len) {
		LOG_WRN("Value of handle 0x%04x longer than %zu octets",
			c->params.read.single.handle, sizeof(c->value));
		discovery_done(c, -EBADMSG);
		return false;
	}

	(void)memcpy(&c->value[c->value_len], data, length);
	c->value_len += length;

	return true;
}

static int value_read(struct hogp_client *c, uint16_t handle, bt_gatt_read_func_t func)
{
	c->value_len = 0U;

	(void)memset(&c->params.read, 0, sizeof(c->params.read));
	c->params.read.func = func;
	c->params.read.handle_count = 1U;
	c->params.read.single.handle = handle;

	return bt_gatt_read(c->conn, &c->params.read);
}

static uint8_t hid_info_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data,
				uint16_t length)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.read);
	struct bt_hogp_report_host_hid_info info;
	struct bt_hogp_report_host_cb *cb;

	ARG_UNUSED(conn);

	if (!value_collect(c, att_err, data, length)) {
		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (c->value_len != HID_INFO_LEN) {
		LOG_WRN("HID Information of %u octets", c->value_len);
		discovery_done(c, -EBADMSG);
		return BT_GATT_ITER_STOP;
	}

	info.bcd_hid = sys_get_le16(&c->value[0]);
	info.country_code = c->value[2];
	info.flags = c->value[3];

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, cb, _node) {
		if (cb->hid_info != NULL) {
			cb->hid_info(&c->hids[0], &info);
		}
	}

	report_map_read(c);

	return BT_GATT_ITER_STOP;
}

static void report_ref_read_next(struct hogp_client *c);

static uint8_t report_ref_read_cb(struct bt_conn *conn, uint8_t att_err,
				  struct bt_gatt_read_params *params, const void *data,
				  uint16_t length)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.read);
	struct hogp_report *report = &c->hids[0].reports[c->cursor];

	ARG_UNUSED(conn);

	if (!value_collect(c, att_err, data, length)) {
		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (c->value_len != REPORT_REF_LEN) {
		LOG_WRN("Report Reference of %u octets", c->value_len);
		discovery_done(c, -EBADMSG);
		return BT_GATT_ITER_STOP;
	}

	report->id = c->value[0];
	report->type = c->value[1];

	if (report->type < BT_HIDS_REPORT_TYPE_INPUT ||
	    report->type > BT_HIDS_REPORT_TYPE_FEATURE) {
		LOG_WRN("Report Type 0x%02x", report->type);
		discovery_done(c, -EBADMSG);
		return BT_GATT_ITER_STOP;
	}

	/* report_find() would only ever find the first of them */
	for (uint8_t i = 0U; i < c->cursor; i++) {
		const struct hogp_report *other = &c->hids[0].reports[i];

		if (other->type == report->type && other->id == report->id) {
			LOG_WRN("Two Reports of Report Type %u with Report ID %u", report->type,
				report->id);
			discovery_done(c, -EBADMSG);
			return BT_GATT_ITER_STOP;
		}
	}

	if (report->type == BT_HIDS_REPORT_TYPE_INPUT && report->ccc_handle == 0U) {
		LOG_WRN("Input Report 0x%04x has no CCC", report->value_handle);
		discovery_done(c, -ENOENT);
		return BT_GATT_ITER_STOP;
	}

	c->cursor++;
	report_ref_read_next(c);

	return BT_GATT_ITER_STOP;
}

static void hid_info_read(struct hogp_client *c)
{
	int err;

	err = value_read(c, c->hids[0].hid_info_handle, hid_info_read_cb);
	if (err != 0) {
		LOG_DBG("bt_gatt_read failed: %d", err);
		discovery_done(c, err);
	}
}

/* Reads the Report Reference of the Report at the cursor, then the HID Information once the last
 * one is read
 */
static void report_ref_read_next(struct hogp_client *c)
{
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	int err;

	if (c->cursor >= hids->report_count) {
		hid_info_read(c);
		return;
	}

	err = value_read(c, hids->reports[c->cursor].ref_handle, report_ref_read_cb);
	if (err != 0) {
		LOG_DBG("bt_gatt_read failed: %d", err);
		discovery_done(c, err);
	}
}

static struct hogp_report *desc_report_find(struct bt_hogp_report_host_hids *hids, uint16_t handle)
{
	for (uint8_t i = 0U; i < hids->report_count; i++) {
		struct hogp_report *report = &hids->reports[i];

		if (handle > report->value_handle && handle <= report->end_handle) {
			return report;
		}
	}

	return NULL;
}

static uint8_t desc_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.discover);
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	struct hogp_report *report;

	ARG_UNUSED(conn);

	if (attr == NULL) {
		if (!client_is_connected(c)) {
			discovery_done(c, -ENOTCONN);
			return BT_GATT_ITER_STOP;
		}

		for (uint8_t i = 0U; i < hids->report_count; i++) {
			if (hids->reports[i].ref_handle == 0U) {
				LOG_WRN("Report 0x%04x has no Report Reference",
					hids->reports[i].value_handle);
				discovery_done(c, -ENOENT);
				return BT_GATT_ITER_STOP;
			}
		}

		c->cursor = 0U;
		report_ref_read_next(c);
		return BT_GATT_ITER_STOP;
	}

	report = desc_report_find(hids, attr->handle);
	if (report == NULL) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (bt_uuid_cmp(attr->uuid, BT_UUID_HIDS_REPORT_REF) == 0) {
		report->ref_handle = attr->handle;
		return BT_GATT_ITER_CONTINUE;
	}

	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
		report->ccc_handle = attr->handle;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void desc_discover(struct hogp_client *c)
{
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	int err;

	if (hids->report_count == 0U) {
		c->cursor = 0U;
		report_ref_read_next(c);
		return;
	}

	/* A Report that ends the service has no Report Reference descriptor */
	if (hids->reports[0].value_handle >= hids->end_handle) {
		LOG_WRN("Report 0x%04x has no Report Reference", hids->reports[0].value_handle);
		discovery_done(c, -ENOENT);
		return;
	}

	(void)memset(&c->params.discover, 0, sizeof(c->params.discover));
	c->params.discover.func = desc_discover_cb;
	c->params.discover.type = BT_GATT_DISCOVER_DESCRIPTOR;
	c->params.discover.start_handle = hids->reports[0].value_handle + 1U;
	c->params.discover.end_handle = hids->end_handle;

	err = bt_gatt_discover(c->conn, &c->params.discover);
	if (err != 0) {
		LOG_DBG("bt_gatt_discover failed: %d", err);
		discovery_done(c, err);
	}
}

static uint8_t chrc_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.discover);
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	const struct bt_gatt_chrc *chrc;
	struct hogp_report *report;

	ARG_UNUSED(conn);

	/* A characteristic, or the end of the HID Service, ends the Report that the previous call
	 * found
	 */
	if (hids->report_count > 0U) {
		struct hogp_report *last = &hids->reports[hids->report_count - 1U];

		if (last->end_handle == 0U) {
			last->end_handle = (attr != NULL) ? attr->handle - 1U : hids->end_handle;
		}
	}

	if (attr == NULL) {
		if (!client_is_connected(c)) {
			discovery_done(c, -ENOTCONN);
			return BT_GATT_ITER_STOP;
		}

		if (hids->report_map_handle == 0U || hids->hid_info_handle == 0U) {
			LOG_WRN("No Report Map or HID Information");
			discovery_done(c, -ENOENT);
			return BT_GATT_ITER_STOP;
		}

		desc_discover(c);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_REPORT_MAP) == 0) {
		if (hids->report_map_handle == 0U) {
			hids->report_map_handle = chrc->value_handle;
		}

		return BT_GATT_ITER_CONTINUE;
	}

	if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_INFO) == 0) {
		if (hids->hid_info_handle == 0U) {
			hids->hid_info_handle = chrc->value_handle;
		}

		return BT_GATT_ITER_CONTINUE;
	}

	if (bt_uuid_cmp(chrc->uuid, BT_UUID_HIDS_REPORT) != 0) {
		return BT_GATT_ITER_CONTINUE;
	}

	if (hids->report_count == ARRAY_SIZE(hids->reports)) {
		LOG_WRN("More than %zu Reports", ARRAY_SIZE(hids->reports));
		discovery_done(c, -ENOMEM);
		return BT_GATT_ITER_STOP;
	}

	/* The GATT client would not write the CCC of a second subscription to the same value
	 * handle, and the discovery would not end.
	 */
	for (uint8_t i = 0U; i < hids->report_count; i++) {
		if (hids->reports[i].value_handle == chrc->value_handle) {
			LOG_WRN("Two Reports with value handle 0x%04x", chrc->value_handle);
			discovery_done(c, -EBADMSG);
			return BT_GATT_ITER_STOP;
		}
	}

	report = &hids->reports[hids->report_count];
	report->value_handle = chrc->value_handle;
	report->properties = chrc->properties;
	hids->report_count++;

	return BT_GATT_ITER_CONTINUE;
}

static void chrc_discover(struct hogp_client *c)
{
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	int err;

	(void)memset(&c->params.discover, 0, sizeof(c->params.discover));
	c->params.discover.func = chrc_discover_cb;
	c->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	c->params.discover.start_handle = hids->start_handle + 1U;
	c->params.discover.end_handle = hids->end_handle;

	err = bt_gatt_discover(c->conn, &c->params.discover);
	if (err != 0) {
		LOG_DBG("bt_gatt_discover failed: %d", err);
		discovery_done(c, err);
	}
}

static uint8_t service_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   struct bt_gatt_discover_params *params)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.discover);
	struct bt_hogp_report_host_hids *hids = &c->hids[0];
	const struct bt_gatt_service_val *service;

	ARG_UNUSED(conn);

	if (attr == NULL) {
		if (!client_is_connected(c)) {
			discovery_done(c, -ENOTCONN);
			return BT_GATT_ITER_STOP;
		}

		LOG_WRN("No HID Service");
		discovery_done(c, -ENOENT);
		return BT_GATT_ITER_STOP;
	}

	service = attr->user_data;

	/* Also catches a service at handle 0xffff, where the next handle wraps to 0 */
	if (attr->handle >= service->end_handle) {
		LOG_WRN("HID Service 0x%04x has no characteristics", attr->handle);
		discovery_done(c, -ENOENT);
		return BT_GATT_ITER_STOP;
	}

	hids->start_handle = attr->handle;
	hids->end_handle = service->end_handle;

	chrc_discover(c);

	return BT_GATT_ITER_STOP;
}

int bt_hogp_report_host_cb_register(struct bt_hogp_report_host_cb *cb)
{
	struct bt_hogp_report_host_cb *tmp;

	if (cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	hogp_lock();

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, tmp, _node) {
		if (tmp == cb) {
			hogp_unlock();
			LOG_DBG("cb %p is already registered", (void *)cb);
			return -EALREADY;
		}
	}

	sys_slist_append(&hogp_cbs, &cb->_node);

	hogp_unlock();

	return 0;
}

int bt_hogp_report_host_cb_unregister(struct bt_hogp_report_host_cb *cb)
{
	bool removed;

	if (cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	hogp_lock();
	removed = sys_slist_find_and_remove(&hogp_cbs, &cb->_node);
	hogp_unlock();

	if (!removed) {
		LOG_DBG("cb %p is not registered", (void *)cb);
		return -EALREADY;
	}

	return 0;
}

/* Drops the state of a connection, if it still has one */
static void client_release(struct bt_conn *conn)
{
	struct hogp_client *c = client_get(conn);
	struct bt_conn *ref;

	hogp_lock();

	if (c->conn != conn) {
		hogp_unlock();
		return;
	}

	/* The GATT parameters stay as they are: a read or write that is starting uses them, and
	 * then fails with -ENOTCONN. bt_hogp_report_host_discover() clears them.
	 */
	ref = bt_conn_take(&c->conn);
	c->state = HOGP_STATE_IDLE;
	c->req = HOGP_REQ_NONE;

	hogp_unlock();

	/* The reference taken by bt_hogp_report_host_discover() */
	bt_conn_unref(ref);
}

int bt_hogp_report_host_discover(struct bt_conn *conn)
{
	struct bt_conn_info info;
	struct hogp_client *c;
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

	c = client_get(conn);

	hogp_lock();

	if (c->state == HOGP_STATE_READY) {
		hogp_unlock();
		return -EALREADY;
	}

	if (c->state != HOGP_STATE_IDLE) {
		hogp_unlock();
		return -EBUSY;
	}

	(void)memset(c, 0, offsetof(struct hogp_client, subs));
	c->conn = bt_conn_ref(conn);
	c->state = HOGP_STATE_DISCOVERING;

	c->params.discover.func = service_discover_cb;
	c->params.discover.uuid = BT_UUID_HIDS;
	c->params.discover.type = BT_GATT_DISCOVER_PRIMARY;
	c->params.discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	c->params.discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	hogp_unlock();

	err = bt_gatt_discover(conn, &c->params.discover);
	if (err != 0) {
		LOG_DBG("bt_gatt_discover failed: %d", err);
		client_release(conn);
	}

	return err;
}

int bt_hogp_report_host_services_get(struct bt_conn *conn,
				     struct bt_hogp_report_host_services *services)
{
	struct hogp_client *c;

	if (conn == NULL || services == NULL || !bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		LOG_DBG("conn or services is NULL, or conn is not an LE connection");
		return -EINVAL;
	}

	c = client_get(conn);

	hogp_lock();

	if (c->conn != conn || c->state != HOGP_STATE_READY) {
		hogp_unlock();
		return -ENOENT;
	}

	services->count = SERVICE_COUNT;
	services->hids = c->hids_list;

	hogp_unlock();

	return 0;
}

int bt_hogp_report_host_conn_get(const struct bt_hogp_report_host_hids *hids, struct bt_conn **conn)
{
	struct hogp_client *c;
	int err;

	if (conn == NULL) {
		LOG_DBG("conn is NULL");
		return -EINVAL;
	}

	hogp_lock();

	c = hids_client_find(hids, &err);
	if (c == NULL) {
		hogp_unlock();
		return err;
	}

	*conn = bt_conn_ref(c->conn);

	hogp_unlock();

	return 0;
}

int bt_hogp_report_host_reports_get(const struct bt_hogp_report_host_hids *hids,
				    struct bt_hogp_report_host_report_info *reports, uint8_t *count)
{
	struct hogp_client *c;
	int err;

	if (reports == NULL || count == NULL) {
		LOG_DBG("reports or count is NULL");
		return -EINVAL;
	}

	hogp_lock();

	c = hids_client_find(hids, &err);
	if (c == NULL) {
		hogp_unlock();
		return err;
	}

	/* The discovery fills the Report table without holding the lock */
	if (c->state != HOGP_STATE_READY) {
		hogp_unlock();
		return -EBUSY;
	}

	if (*count < hids->report_count) {
		*count = hids->report_count;
		hogp_unlock();
		return -ENOMEM;
	}

	for (uint8_t i = 0U; i < hids->report_count; i++) {
		reports[i].type = hids->reports[i].type;
		reports[i].id = hids->reports[i].id;
		reports[i].properties = hids->reports[i].properties;
	}

	*count = hids->report_count;

	hogp_unlock();

	return 0;
}

/* Ends a read or write and returns the Report it was on */
static struct hogp_report *req_end(struct hogp_client *c, const struct bt_conn *conn)
{
	struct hogp_report *report;

	hogp_lock();

	report = &c->hids[0].reports[c->req_report];

	/* The state is gone if the connection went away in the meantime */
	if (c->conn == conn) {
		c->req = HOGP_REQ_NONE;
	}

	hogp_unlock();

	return report;
}

static uint8_t report_read_cb(struct bt_conn *conn, uint8_t att_err,
			      struct bt_gatt_read_params *params, const void *data, uint16_t length)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.read);
	struct bt_hogp_report_host_hids *hids;
	const struct hogp_report *report;
	struct bt_hogp_report_host_cb *cb;
	const uint8_t *value = NULL;
	uint16_t len = 0U;
	int err;

	if (att_err != 0U) {
		/* The GATT client ends the procedures of a connection that goes away with an
		 * ATT error
		 */
		err = conn_is_connected(conn) ? att_err : -ENOTCONN;
		goto end;
	}

	/* The whole value is read */
	if (data == NULL) {
		err = 0;
		value = c->req_buf;
		len = c->req_len;
		goto end;
	}

	if (length > sizeof(c->req_buf) - c->req_len) {
		err = -ENOMEM;
		goto end;
	}

	/* A long value comes in fragments */
	(void)memcpy(&c->req_buf[c->req_len], data, length);
	c->req_len += length;

	return BT_GATT_ITER_CONTINUE;

end:
	/* Ended before the callbacks run, so that they can start the next read or write. That one
	 * does not write req_buf before its own callback, on this thread, so value stays valid.
	 */
	report = req_end(c, conn);
	hids = report_hids_get(c, report);

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, cb, _node) {
		if (cb->report_read != NULL) {
			cb->report_read(hids, report->type, report->id, err, value, len);
		}
	}

	return BT_GATT_ITER_STOP;
}

static void report_write_cb(struct bt_conn *conn, uint8_t att_err,
			    struct bt_gatt_write_params *params)
{
	struct hogp_client *c = CONTAINER_OF(params, struct hogp_client, params.write);
	const struct hogp_report *report = req_end(c, conn);
	struct bt_hogp_report_host_hids *hids = report_hids_get(c, report);
	struct bt_hogp_report_host_cb *cb;
	/* The GATT client ends the procedures of a connection that goes away with an ATT error */
	int err = (att_err == 0U || conn_is_connected(conn)) ? att_err : -ENOTCONN;

	SYS_SLIST_FOR_EACH_CONTAINER(&hogp_cbs, cb, _node) {
		if (cb->report_written != NULL) {
			cb->report_written(hids, report->type, report->id, err);
		}
	}
}

/* Claims the connection for a read or write. Returns NULL and sets err on failure, otherwise the
 * client, with a reference to its connection in conn that the caller releases once it has
 * started the GATT procedure.
 */
static struct hogp_client *req_start(struct bt_hogp_report_host_hids *hids,
				     enum bt_hids_report_type type, uint8_t report_id,
				     enum hogp_req req, uint8_t required_props, const uint8_t *data,
				     uint16_t len, struct bt_conn **conn, int *err)
{
	struct hogp_report *report;
	struct hogp_client *c;

	hogp_lock();

	c = hids_client_find(hids, err);
	if (c == NULL) {
		hogp_unlock();
		return NULL;
	}

	/* The discovery uses the GATT parameters of the connection */
	if (c->state != HOGP_STATE_READY || c->req != HOGP_REQ_NONE) {
		hogp_unlock();
		*err = -EBUSY;
		return NULL;
	}

	report = report_find(hids, type, report_id);
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

	c->req = req;
	c->req_report = report - hids->reports;
	c->req_len = 0U;

	if (req == HOGP_REQ_READ) {
		(void)memset(&c->params.read, 0, sizeof(c->params.read));
		c->params.read.func = report_read_cb;
		c->params.read.handle_count = 1U;
		c->params.read.single.handle = report->value_handle;
	} else if (req == HOGP_REQ_WRITE) {
		(void)memset(&c->params.write, 0, sizeof(c->params.write));
		c->params.write.func = report_write_cb;
		c->params.write.handle = report->value_handle;
		c->params.write.data = data;
		c->params.write.length = len;
	} else {
		__ASSERT(false, "Unknown request %d", req);
	}

	*conn = bt_conn_ref(c->conn);

	hogp_unlock();

	return c;
}

int bt_hogp_report_host_report_read(struct bt_hogp_report_host_hids *hids,
				    enum bt_hids_report_type type, uint8_t report_id)
{
	struct hogp_client *c;
	struct bt_conn *conn;
	int err = 0;

	c = req_start(hids, type, report_id, HOGP_REQ_READ, BT_GATT_CHRC_READ, NULL, 0U, &conn,
		      &err);
	if (c == NULL) {
		return err;
	}

	err = bt_gatt_read(conn, &c->params.read);
	if (err != 0) {
		LOG_DBG("bt_gatt_read failed: %d", err);
		(void)req_end(c, conn);
	}

	bt_conn_unref(conn);

	return err;
}

int bt_hogp_report_host_report_write(struct bt_hogp_report_host_hids *hids,
				     enum bt_hids_report_type type, uint8_t report_id,
				     const uint8_t *data, uint16_t len)
{
	struct hogp_client *c;
	struct bt_conn *conn;
	int err = 0;

	if (data == NULL || len == 0U || len > BT_ATT_MAX_ATTRIBUTE_LEN) {
		LOG_DBG("data is NULL, or len %u is out of range", len);
		return -EINVAL;
	}

	c = req_start(hids, type, report_id, HOGP_REQ_WRITE, BT_GATT_CHRC_WRITE, data, len, &conn,
		      &err);
	if (c == NULL) {
		return err;
	}

	err = bt_gatt_write(conn, &c->params.write);
	if (err != 0) {
		LOG_DBG("bt_gatt_write failed: %d", err);
		(void)req_end(c, conn);
	}

	bt_conn_unref(conn);

	return err;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (!bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		return;
	}

	/* The GATT client has already ended the procedures and removed the subscriptions of the
	 * connection, so the state can be dropped.
	 */
	client_release(conn);
}

BT_CONN_CB_DEFINE(hogp_report_host_conn_cb) = {
	.disconnected = disconnected,
};
