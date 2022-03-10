/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/check.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/has.h>

#include "has_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HAS_CLIENT)
#define LOG_MODULE_NAME bt_has_client
#include "common/log.h"

#define BT_HAS_CLIENT(_has) CONTAINER_OF(_has, struct bt_has_client, has)
#define IS_HANDLE_VALID(handle) ((handle) != 0x0000)

/* Check whether client shall perform preset synchronization */
#define SHALL_SYNC_PRESETS(_feat) \
	 ((_feat & BT_HAS_FEAT_HEARING_AID_TYPE_MASK) == BT_HAS_HEARING_AID_TYPE_BINAURAL && \
	  (_feat & BT_HAS_FEAT_BIT_INDEPENDENT_PRESETS) > 0)

/* Check whether server supports autonomous preset synchronization */
#define IS_PRESET_SYNC_SUPPORTED(_feat) ((_feat & BT_HAS_FEAT_BIT_PRESET_SYNC) > 0)

static struct bt_has_client_cb *has_cb;
static struct bt_has_client {
	/** Common profile reference object */
	struct bt_has has;

	/** Profile connection reference */
	struct bt_conn *conn;

	/** Busy flag indicating GATT operation in progress */
	bool busy;

	/* GATT procedure parameters */
	union {
		struct {
			struct bt_uuid_16 uuid;
			union {
				struct bt_gatt_read_params read;
				struct bt_gatt_discover_params discover;
			};
		};
		struct {
			struct bt_gatt_write_params write;
		};
	} params;

	uint8_t read_preset_idx;

	struct bt_gatt_subscribe_params features_subscription;
	struct bt_gatt_subscribe_params cp_subscription;
	struct bt_gatt_subscribe_params active_preset_subscription;
} client_list[CONFIG_BT_MAX_CONN];

static struct bt_has_client *client_by_conn(struct bt_conn *conn)
{
	return &client_list[bt_conn_index(conn)];
}

static void set_preset_common_cb(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_write_params *params)
{
	struct bt_has_client *client = client_by_conn(conn);

	client->busy = false;

	if (!err) {
		return;
	}

	/* If the operation failed, notify the application that active preset did not change. */
	if (has_cb && has_cb->active_preset) {
		has_cb->active_preset(&client->has, err, client->has.active_index);
	}
}

static int control_point_write(struct bt_has_client *client, bt_gatt_write_func_t func,
			       const void *data, uint16_t len)
{
	uint16_t value_handle;
	int err;

	if (!client->conn) {
		return -ENOTCONN;
	}

	value_handle = client->cp_subscription.value_handle;
	if (!IS_HANDLE_VALID(value_handle)) {
		return -ENOTSUP;
	}

	if (client->busy) {
		return -EBUSY;
	}

	client->params.write.func = func;
	client->params.write.handle = value_handle;
	client->params.write.offset = 0U;
	client->params.write.data = data;
	client->params.write.length = len;

	err = bt_gatt_write(client->conn, &client->params.write);
	if (err < 0) {
		return err;
	}

	client->busy = true;

	return 0;
}

static void read_preset_req_cb(struct bt_conn *conn, uint8_t err,
			       struct bt_gatt_write_params *write)
{
	struct bt_has_client *client = client_by_conn(conn);

	BT_DBG("conn %p err 0x%02x", conn, err);

	client->busy = false;

	if (!err) {
		/* The response will come in Control Point notification */
		return;
	}

	/* The operation failed, provide the error code. */
	if (has_cb && has_cb->preset_read_complete) {
		has_cb->preset_read_complete(&client->has, err);
	}
}

static void discovery_complete(struct bt_has_client *client, bool success)
{
	client->busy = false;

	if (has_cb && has_cb->discover) {
		has_cb->discover(client->conn, success ? &client->has : NULL,
				client->has.features & BT_HAS_FEAT_HEARING_AID_TYPE_MASK);
	}
}

static uint8_t active_preset_index(struct bt_conn *conn,
				   struct bt_gatt_subscribe_params *params, const void *data,
				   uint16_t len)
{
	struct bt_has_client *client;
	struct net_buf_simple buf;
	uint8_t id;

	BT_DBG("conn %p len %u", conn, len);

	if (!conn) { /* Unpaired, continure receiving notifications */
		return BT_GATT_ITER_CONTINUE;
	}

	if (!data) { /* Unsubscribed */
		params->value_handle = 0u;

		return BT_GATT_ITER_STOP;
	}

	if (len == 0) { /* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	id = net_buf_simple_pull_u8(&buf);

	client = client_by_conn(conn);
	if (id != client->has.active_index) {
		client->has.active_index = id;

		if (has_cb && has_cb->active_preset) {
			has_cb->active_preset(&client->has, BT_ATT_ERR_SUCCESS,
					      client->has.active_index);
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void active_preset_index_subscribe_cb(struct bt_conn *conn, uint8_t err,
					     struct bt_gatt_write_params *write)
{
	struct bt_has_client *client = client_by_conn(conn);

	BT_DBG("conn %p err 0x%02x", client->conn, err);

	discovery_complete(client, err == BT_ATT_ERR_SUCCESS);
}

static int active_preset_index_subscribe(struct bt_has_client *client, uint16_t handle)
{
	BT_DBG("conn %p handle 0x%04x", client->conn, handle);

	client->active_preset_subscription.notify = active_preset_index;
	client->active_preset_subscription.write = active_preset_index_subscribe_cb;
	client->active_preset_subscription.value_handle = handle;
	client->active_preset_subscription.ccc_handle = 0x0000;
	client->active_preset_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client->active_preset_subscription.disc_params = &client->params.discover;
	client->active_preset_subscription.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(client->active_preset_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(client->conn, &client->active_preset_subscription);
}

static uint8_t read_active_preset_idx_cb(struct bt_conn *conn, uint8_t err,
					 struct bt_gatt_read_params *read, const void *data,
					 uint16_t length)
{
	struct bt_has_client *client = client_by_conn(conn);
	struct net_buf_simple buf;

	BT_DBG("conn %p err 0x%02x len %u", conn, err, length);

	if (err || length == 0) {
		discovery_complete(client, false);
		return BT_GATT_ITER_STOP;
	}

	BT_DBG("handle 0x%04x", read->by_uuid.start_handle);

	net_buf_simple_init_with_data(&buf, (void *)data, length);

	client->has.active_index = net_buf_simple_pull_u8(&buf);

	if (active_preset_index_subscribe(client, read->by_uuid.start_handle) < 0) {
		discovery_complete(client, false);
	}

	return BT_GATT_ITER_STOP;
}

static int read_active_preset_idx(struct bt_has_client *client)
{
	BT_DBG("conn %p", client->conn);

	client->params.read.func = read_active_preset_idx_cb;
	client->params.read.handle_count = 0u;
	(void)memcpy(&client->params.uuid, BT_UUID_HAS_ACTIVE_PRESET_INDEX, sizeof(client->params.uuid));
	client->params.read.by_uuid.uuid = &client->params.uuid.uuid;
	client->params.read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	client->params.read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_read(client->conn, &client->params.read);
}

static void call_has_preset_cb(struct bt_has *has, uint8_t idx, uint8_t properties,
			       const uint8_t *name, uint8_t len)
{
	uint8_t buf[BT_HAS_PRESET_NAME_MAX + 1];
	uint8_t name_len = MIN(BT_HAS_PRESET_NAME_MAX, len);

	utf8_lcpy(buf, name, ARRAY_SIZE(buf));
	buf[name_len] = '\0';

	if (has_cb && has_cb->preset) {
		has_cb->preset(has, BT_ATT_ERR_SUCCESS, idx, properties, buf);
	}
}

static void read_preset_rsp(struct bt_has_client *client, struct net_buf_simple *buf)
{
	const struct bt_has_cp_read_preset_rsp *rsp;
	int err = 0;

	if (buf->len < sizeof(*rsp)) {
		/* discard the notification */
		return;
	}

	rsp = net_buf_simple_pull_mem(buf, sizeof(*rsp));

	if (client->read_preset_idx) { /* Read single preset by index */
		uint8_t requested_preset_index = client->read_preset_idx;

		client->read_preset_idx = 0;

		if (rsp->index == requested_preset_index) {
			call_has_preset_cb(&client->has, rsp->index, rsp->properties, rsp->name,
					   buf->len);
		} else {
			err = -ENOENT;
		}
	} else { /* Read multiple */
		call_has_preset_cb(&client->has, rsp->index, rsp->properties, rsp->name, buf->len);
		if (!rsp->is_last) {
			return;
		}
	}

	if (has_cb && has_cb->preset_read_complete) {
		has_cb->preset_read_complete(&client->has, err);
	}
}

static void preset_changed(struct bt_has_client *client, struct net_buf_simple *buf)
{
	const struct bt_has_cp_preset_changed *pc;

	if (buf->len < sizeof(*pc)) {
		/* discard the notification */
		return;
	}

	pc = net_buf_simple_pull_mem(buf, sizeof(*pc));

	BT_DBG("client %p %s is_last %d", client, bt_has_change_id_str(pc->change_id), pc->is_last);

	if (pc->change_id == BT_HAS_CHANGE_ID_GENERIC_UPDATE) {
		const struct bt_has_cp_generic_update *gu;

		if (buf->len < sizeof(*gu)) {
			/* discard the notification */
			return;
		}

		gu = net_buf_simple_pull_mem(buf, sizeof(*gu));

		call_has_preset_cb(&client->has, gu->index, gu->properties, gu->name, buf->len);

	} else {
		uint8_t index;

		if (buf->len < sizeof(index)) {
			/* discard the notification */
			return;
		}

		index = net_buf_simple_pull_u8(buf);

		if (pc->change_id == BT_HAS_CHANGE_ID_PRESET_DELETED &&
		    has_cb && has_cb->preset_deleted) {
			has_cb->preset_deleted(&client->has, index);
		} else if (pc->change_id == BT_HAS_CHANGE_ID_PRESET_AVAILABLE &&
			   has_cb && has_cb->preset_availibility) {
			has_cb->preset_availibility(&client->has, index, true);
		} else if (pc->change_id == BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE &&
			   has_cb && has_cb->preset_availibility) {
			has_cb->preset_availibility(&client->has, index, false);
		}
	}
}

static uint8_t control_point_receive(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				     const void *data, uint16_t len)
{
	struct bt_has_client *client;
	const struct bt_has_cp_hdr *hdr;
	struct net_buf_simple buf;

	BT_DBG("conn %p data %p len %u", conn, data, len);

	if (!conn) { /* Unpaired, continure receiving notifications */
		return BT_GATT_ITER_CONTINUE;
	}

	if (!data) { /* Unsubscribed */
		params->value_handle = 0u;

		return BT_GATT_ITER_STOP;
	}

	if (len < sizeof(*hdr)) { /* Ignore malformed notification */
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	hdr = net_buf_simple_pull_mem(&buf, sizeof(*hdr));

	client = client_by_conn(conn);

	BT_DBG("client %p op %s (0x%02x)", client, bt_has_op_str(hdr->op), hdr->op);

	switch (hdr->op) {
	case BT_HAS_OP_READ_PRESET_RSP:
		read_preset_rsp(client, &buf);
		break;
	case BT_HAS_OP_PRESET_CHANGED:
		preset_changed(client, &buf);
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void control_point_subscribe_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_write_params *write)
{
	struct bt_has_client *client = client_by_conn(conn);

	BT_DBG("conn %p err 0x%02x", client->conn, err);

	if (err || read_active_preset_idx(client) < 0) {
		discovery_complete(client, false);
	}
}

static int control_point_subscribe(struct bt_has_client *client, uint16_t handle)
{
	BT_DBG("conn %p handle 0x%04x", client->conn, handle);

	client->cp_subscription.notify = control_point_receive;
	client->cp_subscription.write = control_point_subscribe_cb;
	client->cp_subscription.value_handle = handle;
	client->cp_subscription.ccc_handle = 0x0000;
	client->cp_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client->cp_subscription.disc_params = &client->params.discover;
	client->cp_subscription.value = BT_GATT_CCC_INDICATE;
	atomic_set_bit(client->cp_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(client->conn, &client->cp_subscription);
}

static uint8_t control_point_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				         struct bt_gatt_discover_params *discover)
{
	struct bt_has_client *client = client_by_conn(conn);
	const struct bt_gatt_chrc *chrc;

	BT_DBG("conn %p attr %p", client->conn, attr);

	if (!attr) {
		BT_INFO("HAS Control Point not found");
		discovery_complete(client, true);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if (control_point_subscribe(client, chrc->value_handle) < 0) {
		discovery_complete(client, false);
	}

	return BT_GATT_ITER_STOP;
}

static int control_point_discover(struct bt_has_client *client)
{
	BT_DBG("conn %p", client->conn);

	(void)memcpy(&client->params.uuid, BT_UUID_HAS_PRESET_CONTROL_POINT, sizeof(client->params.uuid));
	client->params.discover.uuid = &client->params.uuid.uuid;
	client->params.discover.func = control_point_discover_cb;
	client->params.discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	client->params.discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(client->conn, &client->params.discover);
}

static uint8_t hearing_aid_features_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params,
				       const void *data, uint16_t len)
{
	struct bt_has_client *client;
	struct net_buf_simple buf;

	BT_DBG("conn %p data %p len %u", conn, data, len);

	if (!conn) { /* Unpaired, continure receiving notifications */
		return BT_GATT_ITER_CONTINUE;
	}

	if (!data) { /* Unsubscribed */
		params->value_handle = 0u;

		return BT_GATT_ITER_STOP;
	}

	if (len == 0) { /* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	client = client_by_conn(conn);
	client->has.features = net_buf_simple_pull_u8(&buf);

	BT_DBG("features 0x%02x", client->has.features);

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t hearing_aid_features_read_cb(struct bt_conn *conn, uint8_t err,
					    struct bt_gatt_read_params *read, const void *data,
					    uint16_t len)
{
	struct bt_has_client *client = client_by_conn(conn);
	struct net_buf_simple buf;

	BT_DBG("conn %p err 0x%02x len %u", conn, err, len);

	if (err || len == 0) {
		discovery_complete(client, false);
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	client->has.features = net_buf_simple_pull_u8(&buf);

	BT_DBG("features 0x%02x", client->has.features);

	if (control_point_discover(client) < 0) {
		discovery_complete(client, false);
	}

	return BT_GATT_ITER_STOP;
}

static int hearing_aid_features_read(struct bt_has_client *client, uint16_t value_handle)
{
	BT_DBG("conn %p handle 0x%04x", client->conn, value_handle);

	client->params.read.func = hearing_aid_features_read_cb;
	client->params.read.handle_count = 1u;
	client->params.read.single.handle = value_handle;
	client->params.read.single.offset = 0u;

	return bt_gatt_read(client->conn, &client->params.read);
}

static void hearing_aid_features_subscribe_cb(struct bt_conn *conn, uint8_t err,
					      struct bt_gatt_write_params *write)
{
	struct bt_has_client *client = client_by_conn(conn);
	const uint16_t handle = client->features_subscription.value_handle;

	BT_DBG("conn %p err 0x%02x", client->conn, err);

	if (err || hearing_aid_features_read(client, handle) < 0) {
		discovery_complete(client, false);
	}
}

static int hearing_aid_features_subscribe(struct bt_has_client *client, uint16_t handle)
{
	BT_DBG("conn %p handle 0x%04x", client->conn, handle);

	client->features_subscription.notify = hearing_aid_features_cb;
	client->features_subscription.write = hearing_aid_features_subscribe_cb;
	client->features_subscription.value_handle = handle;
	client->features_subscription.ccc_handle = 0x0000;
	client->features_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client->features_subscription.disc_params = &client->params.discover;
	client->features_subscription.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(client->features_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(client->conn, &client->features_subscription);
}

static uint8_t hearing_aid_features_discover_cb(struct bt_conn *conn,
						const struct bt_gatt_attr *attr,
						struct bt_gatt_discover_params *discover)
{
	struct bt_has_client *client = client_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	BT_DBG("conn %p attr %p", conn, attr);

	if (!attr) {
		discovery_complete(client, false);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		err = hearing_aid_features_subscribe(client, chrc->value_handle);
	} else {
		err = hearing_aid_features_read(client, chrc->value_handle);
	}

	if (err < 0) {
		discovery_complete(client, false);
	}

	return BT_GATT_ITER_STOP;
}

static int hearing_aid_features_discover(struct bt_has_client *client)
{
	BT_DBG("conn %p", client->conn);

	(void)memcpy(&client->params.uuid, BT_UUID_HAS_HEARING_AID_FEATURES, sizeof(client->params.uuid));
	client->params.discover.uuid = &client->params.uuid.uuid;
	client->params.discover.func = hearing_aid_features_discover_cb;
	client->params.discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	client->params.discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(client->conn, &client->params.discover);
}

static uint8_t preset_active_get_cb(struct bt_conn *conn, uint8_t err,
				    struct bt_gatt_read_params *params, const void *data,
				    uint16_t len)
{
	struct bt_has_client *client = client_by_conn(conn);
	struct net_buf_simple buf;

	BT_DBG("conn %p params %p err 0x%02x len %u", conn, params, err, len);

	if (err == BT_ATT_ERR_SUCCESS && len > 0) {
		net_buf_simple_init_with_data(&buf, (void *)data, len);

		client->has.active_index = net_buf_simple_pull_u8(&buf);

		BT_DBG("Active Preset ID 0x%02x", client->has.active_index);
	}

	client->busy = false;

	if (has_cb && has_cb->active_preset) {
		has_cb->active_preset(&client->has, len > 0 ? err : -EINVAL,
				      client->has.active_index);
	}

	return BT_GATT_ITER_STOP;
}

int bt_has_client_preset_active_get(struct bt_has *has)
{
	struct bt_has_client *client = BT_HAS_CLIENT(has);
	uint16_t value_handle;
	int err;

	BT_DBG("has %p", has);

	CHECKIF(!has_cb || !has_cb->active_preset) {
		return -EINVAL;
	}

	if (!client->conn) {
		return -ENOTCONN;
	}

	if (client->busy) {
		return -EBUSY;
	}

	value_handle = client->active_preset_subscription.value_handle;
	if (!IS_HANDLE_VALID(value_handle)) {
		return -ENOTSUP;
	}

	client->params.read.func = preset_active_get_cb;
	client->params.read.handle_count = 1u;
	client->params.read.single.handle = value_handle;
	client->params.read.single.offset = 0u;

	err = bt_gatt_read(client->conn, &client->params.read);
	if (err < 0) {
		return err;
	}

	client->busy = true;

	return 0;
}

int bt_has_client_preset_active_set(struct bt_has *has, uint8_t id)
{
	struct bt_has_client *client = BT_HAS_CLIENT(has);
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_set_active_preset_req *req;
	uint8_t buf[sizeof(*hdr) + sizeof(*req)];

	CHECKIF(!has_cb || !has_cb->active_preset) {
		return -EINVAL;
	}

	CHECKIF(id == BT_HAS_PRESET_INDEX_NONE) {
		return -EINVAL;
	}

	hdr = (void *)buf;

	/* Request synchronization of Binaural Set if supported */
	if (SHALL_SYNC_PRESETS(has->features) && IS_PRESET_SYNC_SUPPORTED(has->features)) {
		hdr->op = BT_HAS_OP_SET_ACTIVE_PRESET_SYNC;
	} else {
		hdr->op = BT_HAS_OP_SET_ACTIVE_PRESET;
	}

	req = (void *)hdr->data;
	req->index = id;

	return control_point_write(client, set_preset_common_cb, buf, sizeof(buf));
}

int bt_has_client_preset_active_set_next(struct bt_has *has)
{
	struct bt_has_client *client = BT_HAS_CLIENT(has);
	struct bt_has_cp_hdr hdr;

	/* Request synchronization of Binaural Set if supported */
	if (SHALL_SYNC_PRESETS(has->features) && IS_PRESET_SYNC_SUPPORTED(has->features)) {
		hdr.op = BT_HAS_OP_SET_NEXT_PRESET_SYNC;
	} else {
		hdr.op = BT_HAS_OP_SET_NEXT_PRESET;
	}

	return control_point_write(client, set_preset_common_cb, &hdr, sizeof(hdr));
}

int bt_has_client_preset_active_set_prev(struct bt_has *has)
{
	struct bt_has_client *client = BT_HAS_CLIENT(has);
	struct bt_has_cp_hdr hdr;

	/* Request synchronization of Binaural Set if supported */
	if (SHALL_SYNC_PRESETS(has->features) && IS_PRESET_SYNC_SUPPORTED(has->features)) {
		hdr.op = BT_HAS_OP_SET_PREV_PRESET_SYNC;
	} else {
		hdr.op = BT_HAS_OP_SET_PREV_PRESET;
	}

	return control_point_write(client, set_preset_common_cb, &hdr, sizeof(hdr));
}

static int bt_has_read_preset_req(struct bt_has_client *client, uint8_t start_idx, uint8_t count)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_read_preset_req *req;
	uint8_t buf[sizeof(*hdr) + sizeof(*req)];

	hdr = (void *)buf;
	hdr->op = BT_HAS_OP_READ_PRESET_REQ;
	req = (void *)hdr->data;
	req->start_index = start_idx;
	req->num_presets = count;

	return control_point_write(client, read_preset_req_cb, buf, sizeof(buf));
}

int bt_has_client_preset_read(struct bt_has *has, uint8_t idx)
{
	struct bt_has_client *client = BT_HAS_CLIENT(has);
	int err;

	CHECKIF(!has_cb || !has_cb->preset || !has_cb->preset_read_complete) {
		return -EINVAL;
	}

	if (client->read_preset_idx) {
		return -EBUSY;
	}

	err = bt_has_read_preset_req(client, idx, 1u);
	if (err == 0) {
		client->read_preset_idx = idx;
	}

	return err;
}

int bt_has_client_preset_read_multiple(struct bt_has *has, uint8_t start_idx, uint8_t count)
{
	CHECKIF(!has_cb || !has_cb->preset || !has_cb->preset_read_complete) {
		return -EINVAL;
	}

	return bt_has_read_preset_req(BT_HAS_CLIENT(has), start_idx, count);
}

int bt_has_client_preset_name_set(struct bt_has *has, uint8_t id, const char *name)
{
	struct bt_has_client *client = BT_HAS_CLIENT(has);
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_write_preset_name_req *req;
	uint8_t buf[sizeof(*hdr) + sizeof(*req) + strlen(name)];

	/* FIXME */

	if (strlen(name) < BT_HAS_PRESET_NAME_MIN || strlen(name) > BT_HAS_PRESET_NAME_MAX) {
		return -EINVAL;
	}

	hdr = (void *)buf;
	hdr->op = BT_HAS_OP_WRITE_PRESET_NAME;
	req = (void *)hdr->data;
	req->index = id;
	strncpy(req->name, name, strlen(name));

	return control_point_write(client, set_preset_common_cb, buf, sizeof(buf));
}

/** @brief Hearing Access Service discovery
 *
 *  This will initiate a discover procedure. The procedure will do the following sequence:
 *  1) HAS related characteristic discovery
 *  2) CCC subscription
 *  3) Hearing Aid Features and Active Preset Index characteristic read
 *  5) When everything above have been completed, the callback is called
 *
 *  @param conn Connection object
 *  @return 0 if success, errno on failure.
 */
int bt_has_discover(struct bt_conn *conn)
{
	struct bt_has_client *client;
	int err;

	BT_DBG("conn %p", conn);

	CHECKIF(!has_cb || !has_cb->discover) {
		return -EINVAL;
	}

	if (!conn) {
		return -ENOTCONN;
	}

	client = client_by_conn(conn);
	if (client->busy) {
		return -EBUSY;
	}

	client->conn = conn;

	err = hearing_aid_features_discover(client);
	if (!err) {
		client->busy = true;
	}

	return err;
}

int bt_has_client_cb_register(struct bt_has_client_cb *cb)
{
	CHECKIF(has_cb) {
		return -EALREADY;
	}

	has_cb = cb;

	return 0;
}

int bt_has_client_conn_get(const struct bt_has *has, struct bt_conn **conn)
{
	struct bt_has_client *client = BT_HAS_CLIENT(has);

	*conn = bt_conn_ref(client->conn);

	return 0;
}
