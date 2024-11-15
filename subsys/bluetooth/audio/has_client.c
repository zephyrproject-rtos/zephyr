/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "has_internal.h"

LOG_MODULE_REGISTER(bt_has_client, CONFIG_BT_HAS_CLIENT_LOG_LEVEL);

#define HAS_INST(_has) CONTAINER_OF(_has, struct bt_has_client, has)
#define HANDLE_IS_VALID(handle) ((handle) != 0x0000)
static struct bt_has_client clients[CONFIG_BT_MAX_CONN];
static const struct bt_has_client_cb *client_cb;

static struct bt_has_client *inst_by_conn(struct bt_conn *conn)
{
	struct bt_has_client *inst = &clients[bt_conn_index(conn)];

	if (inst->conn == conn) {
		return inst;
	}

	return NULL;
}

static void inst_cleanup(struct bt_has_client *inst)
{
	bt_conn_unref(inst->conn);

	(void)memset(inst, 0, sizeof(*inst));
}

static enum bt_has_capabilities get_capabilities(const struct bt_has_client *inst)
{
	enum bt_has_capabilities caps = 0;

	/* The Control Point support is optional, as the server might have no presets support */
	if (HANDLE_IS_VALID(inst->control_point_subscription.value_handle)) {
		caps |= BT_HAS_PRESET_SUPPORT;
	}

	return caps;
}

static void handle_read_preset_rsp(struct bt_has_client *inst, struct net_buf_simple *buf)
{
	const struct bt_has_cp_read_preset_rsp *pdu;
	struct bt_has_preset_record record;
	char name[BT_HAS_PRESET_NAME_MAX + 1]; /* + 1 byte for null-terminator */
	size_t name_len;

	LOG_DBG("conn %p buf %p", (void *)inst->conn, buf);

	if (buf->len < sizeof(*pdu)) {
		LOG_ERR("malformed PDU");
		return;
	}

	pdu = net_buf_simple_pull_mem(buf, sizeof(*pdu));

	if (pdu->is_last > BT_HAS_IS_LAST) {
		LOG_WRN("unexpected is_last value 0x%02x", pdu->is_last);
	}

	record.index = pdu->index;
	record.properties = pdu->properties;
	record.name = name;

	name_len = buf->len + 1; /* + 1 byte for NULL terminator */
	if (name_len > ARRAY_SIZE(name)) {
		LOG_WRN("name is too long (%zu > %u)", buf->len, BT_HAS_PRESET_NAME_MAX);

		name_len = ARRAY_SIZE(name);
	}

	utf8_lcpy(name, pdu->name, name_len);

	client_cb->preset_read_rsp(&inst->has, 0, &record, !!pdu->is_last);
}

static void handle_generic_update(struct bt_has_client *inst, struct net_buf_simple *buf,
				  bool is_last)
{
	const struct bt_has_cp_generic_update *pdu;
	struct bt_has_preset_record record;
	char name[BT_HAS_PRESET_NAME_MAX + 1]; /* + 1 byte for null-terminator */
	size_t name_len;

	if (buf->len < sizeof(*pdu)) {
		LOG_ERR("malformed PDU");
		return;
	}

	pdu = net_buf_simple_pull_mem(buf, sizeof(*pdu));

	record.index = pdu->index;
	record.properties = pdu->properties;
	record.name = name;

	name_len = buf->len + 1; /* + 1 byte for NULL terminator */
	if (name_len > ARRAY_SIZE(name)) {
		LOG_WRN("name is too long (%zu > %u)", buf->len, BT_HAS_PRESET_NAME_MAX);

		name_len = ARRAY_SIZE(name);
	}

	utf8_lcpy(name, pdu->name, name_len);

	client_cb->preset_update(&inst->has, pdu->prev_index, &record, is_last);
}

static void handle_preset_deleted(struct bt_has_client *inst, struct net_buf_simple *buf,
				  bool is_last)
{
	if (buf->len < sizeof(uint8_t)) {
		LOG_ERR("malformed PDU");
		return;
	}

	client_cb->preset_deleted(&inst->has, net_buf_simple_pull_u8(buf), is_last);
}

static void handle_preset_availability(struct bt_has_client *inst, struct net_buf_simple *buf,
				       bool available, bool is_last)
{
	if (buf->len < sizeof(uint8_t)) {
		LOG_ERR("malformed PDU");
		return;
	}

	client_cb->preset_availability(&inst->has, net_buf_simple_pull_u8(buf), available,
				       is_last);
}

static void handle_preset_changed(struct bt_has_client *inst, struct net_buf_simple *buf)
{
	const struct bt_has_cp_preset_changed *pdu;

	LOG_DBG("conn %p buf %p", (void *)inst->conn, buf);

	if (buf->len < sizeof(*pdu)) {
		LOG_ERR("malformed PDU");
		return;
	}

	pdu = net_buf_simple_pull_mem(buf, sizeof(*pdu));

	if (pdu->is_last > BT_HAS_IS_LAST) {
		LOG_WRN("unexpected is_last 0x%02x", pdu->is_last);
	}

	switch (pdu->change_id) {
	case BT_HAS_CHANGE_ID_GENERIC_UPDATE:
		if (client_cb->preset_update) {
			handle_generic_update(inst, buf, !!pdu->is_last);
		}
		break;
	case BT_HAS_CHANGE_ID_PRESET_DELETED:
		if (client_cb->preset_deleted) {
			handle_preset_deleted(inst, buf, !!pdu->is_last);
		}
		break;
	case BT_HAS_CHANGE_ID_PRESET_AVAILABLE:
		if (client_cb->preset_availability) {
			handle_preset_availability(inst, buf, !!pdu->is_last, true);
		}
		return;
	case BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE:
		if (client_cb->preset_availability) {
			handle_preset_availability(inst, buf, !!pdu->is_last, false);
		}
		return;
	default:
		LOG_WRN("unknown change_id 0x%02x", pdu->change_id);
	}
}

static uint8_t control_point_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client,
						  control_point_subscription);
	const struct bt_has_cp_hdr *hdr;
	struct net_buf_simple buf;

	LOG_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (!conn) { /* Unpaired, continue receiving notifications */
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

	switch (hdr->opcode) {
	case BT_HAS_OP_READ_PRESET_RSP:
		handle_read_preset_rsp(inst, &buf);
		break;
	case BT_HAS_OP_PRESET_CHANGED:
		handle_preset_changed(inst, &buf);
		break;
	};

	return BT_GATT_ITER_CONTINUE;
}

static void discover_complete(struct bt_has_client *inst)
{
	LOG_DBG("conn %p", (void *)inst->conn);

	atomic_clear_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS);

	client_cb->discover(inst->conn, 0, &inst->has,
			    inst->has.features & BT_HAS_FEAT_HEARING_AID_TYPE_MASK,
			    get_capabilities(inst));

	/* If Active Preset Index supported, notify it's value */
	if (client_cb->preset_switch &&
	    HANDLE_IS_VALID(inst->active_index_subscription.value_handle)) {
		client_cb->preset_switch(&inst->has, 0, inst->has.active_index);
	}
}

static void discover_failed(struct bt_conn *conn, int err)
{
	LOG_DBG("conn %p", (void *)conn);

	client_cb->discover(conn, err, NULL, 0, 0);
}

static int cp_write(struct bt_has_client *inst, struct net_buf_simple *buf,
		    bt_gatt_write_func_t func)
{
	const uint16_t value_handle = inst->control_point_subscription.value_handle;

	if (!HANDLE_IS_VALID(value_handle)) {
		return -ENOTSUP;
	}

	inst->params.write.func = func;
	inst->params.write.handle = value_handle;
	inst->params.write.offset = 0U;
	inst->params.write.data = buf->data;
	inst->params.write.length = buf->len;

	return bt_gatt_write(inst->conn, &inst->params.write);
}

static void read_presets_req_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_write_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, params.write);

	LOG_DBG("conn %p err 0x%02x param %p", (void *)conn, err, params);

	atomic_clear_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS);

	if (err) {
		client_cb->preset_read_rsp(&inst->has, err, NULL, true);
	}
}

static int read_presets_req(struct bt_has_client *inst, uint8_t start_index, uint8_t num_presets)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_read_presets_req *req;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*hdr) + sizeof(*req));

	LOG_DBG("conn %p start_index 0x%02x num_presets %d", (void *)inst->conn, start_index,
		num_presets);

	hdr = net_buf_simple_add(&buf, sizeof(*hdr));
	hdr->opcode = BT_HAS_OP_READ_PRESET_REQ;
	req = net_buf_simple_add(&buf, sizeof(*req));
	req->start_index = start_index;
	req->num_presets = num_presets;

	return cp_write(inst, &buf, read_presets_req_cb);
}

static void set_active_preset_cb(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_write_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, params.write);

	LOG_DBG("conn %p err 0x%02x param %p", (void *)conn, err, params);

	atomic_clear_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS);

	if (err) {
		client_cb->preset_switch(&inst->has, err, inst->has.active_index);
	}
}

static int preset_set(struct bt_has_client *inst, uint8_t opcode, uint8_t index)
{
	struct bt_has_cp_hdr *hdr;
	struct bt_has_cp_set_active_preset *req;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*hdr) + sizeof(*req));

	LOG_DBG("conn %p opcode 0x%02x index 0x%02x", (void *)inst->conn, opcode, index);

	hdr = net_buf_simple_add(&buf, sizeof(*hdr));
	hdr->opcode = opcode;
	req = net_buf_simple_add(&buf, sizeof(*req));
	req->index = index;

	return cp_write(inst, &buf, set_active_preset_cb);
}

static int preset_set_next_or_prev(struct bt_has_client *inst, uint8_t opcode)
{
	struct bt_has_cp_hdr *hdr;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*hdr));

	LOG_DBG("conn %p opcode 0x%02x", (void *)inst->conn, opcode);

	hdr = net_buf_simple_add(&buf, sizeof(*hdr));
	hdr->opcode = opcode;

	return cp_write(inst, &buf, set_active_preset_cb);
}

static uint8_t active_index_update(struct bt_has_client *inst, const void *data, uint16_t len)
{
	struct net_buf_simple buf;
	const uint8_t prev = inst->has.active_index;

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	inst->has.active_index = net_buf_simple_pull_u8(&buf);

	LOG_DBG("conn %p index 0x%02x", (void *)inst->conn, inst->has.active_index);

	return prev;
}

static uint8_t active_preset_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client,
						  active_index_subscription);
	uint8_t prev;

	LOG_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (!conn) {
		/* Unpaired, stop receiving notifications from device */
		return BT_GATT_ITER_STOP;
	}

	if (!data) {
		/* Unsubscribed */
		params->value_handle = 0u;

		return BT_GATT_ITER_STOP;
	}

	if (len == 0) {
		/* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	prev = active_index_update(inst, data, len);

	if (atomic_test_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS)) {
		/* Got notification during discovery process, postpone the active_index callback
		 * until discovery is complete.
		 */
		return BT_GATT_ITER_CONTINUE;
	}

	if (client_cb && client_cb->preset_switch && inst->has.active_index != prev) {
		client_cb->preset_switch(&inst->has, 0, inst->has.active_index);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void active_index_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				      struct bt_gatt_subscribe_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client,
						  active_index_subscription);

	LOG_DBG("conn %p att_err 0x%02x params %p", (void *)inst->conn, att_err, params);

	if (att_err != BT_ATT_ERR_SUCCESS) {
		/* Cleanup instance so that it can be reused */
		inst_cleanup(inst);

		discover_failed(conn, att_err);
	} else {
		discover_complete(inst);
	}
}

static int active_index_subscribe(struct bt_has_client *inst, uint16_t value_handle)
{
	int err;

	LOG_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	inst->active_index_subscription.notify = active_preset_notify_cb;
	inst->active_index_subscription.subscribe = active_index_subscribe_cb;
	inst->active_index_subscription.value_handle = value_handle;
	inst->active_index_subscription.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
	inst->active_index_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->active_index_subscription.disc_params = &inst->params.discover;
	inst->active_index_subscription.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(inst->active_index_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(inst->conn, &inst->active_index_subscription);
	if (err != 0 && err != -EALREADY) {
		return err;
	}

	return 0;
}

static uint8_t active_index_read_cb(struct bt_conn *conn, uint8_t att_err,
				    struct bt_gatt_read_params *params, const void *data,
				    uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, params.read);
	int err = att_err;

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	if (att_err != BT_ATT_ERR_SUCCESS || len == 0) {
		goto fail;
	}

	active_index_update(inst, data, len);

	err = active_index_subscribe(inst, params->by_uuid.start_handle);
	if (err) {
		LOG_ERR("Subscribe failed (err %d)", err);
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);

	return BT_GATT_ITER_STOP;
}

static int active_index_read(struct bt_has_client *inst)
{
	LOG_DBG("conn %p", (void *)inst->conn);

	(void)memset(&inst->params.read, 0, sizeof(inst->params.read));

	(void)memcpy(&inst->params.uuid, BT_UUID_HAS_ACTIVE_PRESET_INDEX,
		     sizeof(inst->params.uuid));
	inst->params.read.func = active_index_read_cb;
	inst->params.read.handle_count = 0u;
	inst->params.read.by_uuid.uuid = &inst->params.uuid.uuid;
	inst->params.read.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	inst->params.read.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	return bt_gatt_read(inst->conn, &inst->params.read);
}

static void control_point_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				       struct bt_gatt_subscribe_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client,
						  control_point_subscription);
	int err = att_err;

	LOG_DBG("conn %p att_err 0x%02x", (void *)inst->conn, att_err);

	if (att_err != BT_ATT_ERR_SUCCESS) {
		goto fail;
	}

	err = active_index_read(inst);
	if (err) {
		LOG_ERR("Active Preset Index read failed (err %d)", err);
		goto fail;
	}

	return;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);
}

static int control_point_subscribe(struct bt_has_client *inst, uint16_t value_handle,
				   uint8_t properties)
{
	int err;

	LOG_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	inst->control_point_subscription.notify = control_point_notify_cb;
	inst->control_point_subscription.subscribe = control_point_subscribe_cb;
	inst->control_point_subscription.value_handle = value_handle;
	inst->control_point_subscription.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
	inst->control_point_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->control_point_subscription.disc_params = &inst->params.discover;
	atomic_set_bit(inst->control_point_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	if (IS_ENABLED(CONFIG_BT_EATT) && properties & BT_GATT_CHRC_NOTIFY) {
		inst->control_point_subscription.value = BT_GATT_CCC_INDICATE | BT_GATT_CCC_NOTIFY;
	} else {
		inst->control_point_subscription.value = BT_GATT_CCC_INDICATE;
	}

	err = bt_gatt_subscribe(inst->conn, &inst->control_point_subscription);
	if (err != 0 && err != -EALREADY) {
		return err;
	}

	return 0;
}

static uint8_t control_point_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params, int err)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, params.discover);
	const struct bt_gatt_chrc *chrc;

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (!attr) {
		LOG_INF("Control Point not found");
		discover_complete(inst);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	err = control_point_subscribe(inst, chrc->value_handle, chrc->properties);
	if (err) {
		LOG_ERR("Subscribe failed (err %d)", err);

		/* Cleanup instance so that it can be reused */
		inst_cleanup(inst);

		discover_failed(conn, err);
	}

	return BT_GATT_ITER_STOP;
}

static int control_point_discover(struct bt_has_client *inst)
{
	LOG_DBG("conn %p", (void *)inst->conn);

	(void)memset(&inst->params.discover, 0, sizeof(inst->params.discover));

	(void)memcpy(&inst->params.uuid, BT_UUID_HAS_PRESET_CONTROL_POINT,
		     sizeof(inst->params.uuid));
	inst->params.discover.uuid = &inst->params.uuid.uuid;
	inst->params.discover.func = control_point_discover_cb;
	inst->params.discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	inst->params.discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(inst->conn, &inst->params.discover);
}

static void features_update(struct bt_has_client *inst, const void *data, uint16_t len)
{
	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	inst->has.features = net_buf_simple_pull_u8(&buf);

	LOG_DBG("conn %p features 0x%02x", (void *)inst->conn, inst->has.features);
}

static uint8_t features_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, params.read);
	int err = att_err;

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	if (att_err != BT_ATT_ERR_SUCCESS || len == 0) {
		goto fail;
	}

	features_update(inst, data, len);

	if (!client_cb->preset_switch) {
		/* Complete the discovery if client is not interested in active preset changes */
		discover_complete(inst);
		return BT_GATT_ITER_STOP;
	}

	err = control_point_discover(inst);
	if (err) {
		LOG_ERR("Control Point discover failed (err %d)", err);
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);

	return BT_GATT_ITER_STOP;
}

static int features_read(struct bt_has_client *inst, uint16_t value_handle)
{
	LOG_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	(void)memset(&inst->params.read, 0, sizeof(inst->params.read));

	inst->params.read.func = features_read_cb;
	inst->params.read.handle_count = 1u;
	inst->params.read.single.handle = value_handle;
	inst->params.read.single.offset = 0u;

	return bt_gatt_read(inst->conn, &inst->params.read);
}

static void features_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				  struct bt_gatt_subscribe_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client,
						  features_subscription);
	int err = att_err;

	LOG_DBG("conn %p att_err 0x%02x params %p", (void *)conn, att_err, params);

	if (att_err != BT_ATT_ERR_SUCCESS) {
		goto fail;
	}

	err = features_read(inst, inst->features_subscription.value_handle);
	if (err) {
		LOG_ERR("Read failed (err %d)", err);
		goto fail;
	}

	return;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);
}

static uint8_t features_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client,
						  features_subscription);

	LOG_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (!conn) {
		/* Unpaired, stop receiving notifications from device */
		return BT_GATT_ITER_STOP;
	}

	if (!data) {
		/* Unsubscribed */
		params->value_handle = 0u;

		return BT_GATT_ITER_STOP;
	}

	if (len == 0) {
		/* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	features_update(inst, data, len);

	return BT_GATT_ITER_CONTINUE;
}

static int features_subscribe(struct bt_has_client *inst, uint16_t value_handle)
{
	int err;

	LOG_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	inst->features_subscription.notify = features_notify_cb;
	inst->features_subscription.subscribe = features_subscribe_cb;
	inst->features_subscription.value_handle = value_handle;
	inst->features_subscription.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
	inst->features_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->features_subscription.disc_params = &inst->params.discover;
	inst->features_subscription.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(inst->features_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(inst->conn, &inst->features_subscription);
	if (err != 0 && err != -EALREADY) {
		return err;
	}

	return 0;
}

static uint8_t features_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    struct bt_gatt_discover_params *params, int err)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, params.discover);
	const struct bt_gatt_chrc *chrc;

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (!attr) {
		err = -ENOENT;
		goto fail;
	}

	chrc = attr->user_data;

	/* Subscribe first if notifications are supported, otherwise read the features */
	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		err = features_subscribe(inst, chrc->value_handle);
		if (err) {
			LOG_ERR("Subscribe failed (err %d)", err);
			goto fail;
		}
	} else {
		err = features_read(inst, chrc->value_handle);
		if (err) {
			LOG_ERR("Read failed (err %d)", err);
			goto fail;
		}
	}

	return BT_GATT_ITER_STOP;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);

	return BT_GATT_ITER_STOP;
}

static int features_discover(struct bt_has_client *inst)
{
	LOG_DBG("conn %p", (void *)inst->conn);

	(void)memset(&inst->params.discover, 0, sizeof(inst->params.discover));

	(void)memcpy(&inst->params.uuid, BT_UUID_HAS_HEARING_AID_FEATURES,
		     sizeof(inst->params.uuid));
	inst->params.discover.uuid = &inst->params.uuid.uuid;
	inst->params.discover.func = features_discover_cb;
	inst->params.discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	inst->params.discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	return bt_gatt_discover(inst->conn, &inst->params.discover);
}

int bt_has_client_cb_register(const struct bt_has_client_cb *cb)
{
	CHECKIF(!cb) {
		return -EINVAL;
	}

	CHECKIF(client_cb) {
		return -EALREADY;
	}

	client_cb = cb;

	return 0;
}

/* Hearing Access Service discovery
 *
 * This will initiate a discover procedure. The procedure will do the following sequence:
 * 1) HAS related characteristic discovery
 * 2) CCC subscription
 * 3) Hearing Aid Features and Active Preset Index characteristic read
 * 5) When everything above have been completed, the callback is called
 */
int bt_has_client_discover(struct bt_conn *conn)
{
	struct bt_has_client *inst;
	int err;

	LOG_DBG("conn %p", (void *)conn);

	CHECKIF(!conn || !client_cb || !client_cb->discover) {
		return -EINVAL;
	}

	inst = &clients[bt_conn_index(conn)];

	if (atomic_test_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS) ||
	    atomic_test_and_set_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS)) {
		return -EBUSY;
	}

	if (inst->conn) {
		return -EALREADY;
	}

	inst->conn = bt_conn_ref(conn);

	err = features_discover(inst);
	if (err) {
		atomic_clear_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS);
	}

	return err;
}

int bt_has_client_conn_get(const struct bt_has *has, struct bt_conn **conn)
{
	struct bt_has_client *inst = HAS_INST(has);

	*conn = bt_conn_ref(inst->conn);

	return 0;
}

int bt_has_client_presets_read(struct bt_has *has, uint8_t start_index, uint8_t count)
{
	struct bt_has_client *inst = HAS_INST(has);
	int err;

	LOG_DBG("conn %p start_index 0x%02x count %d", (void *)inst->conn, start_index, count);

	if (!inst->conn) {
		return -ENOTCONN;
	}

	if (atomic_test_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS) ||
	    atomic_test_and_set_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS)) {
		return -EBUSY;
	}

	CHECKIF(start_index == BT_HAS_PRESET_INDEX_NONE) {
		return -EINVAL;
	}

	CHECKIF(count == 0u) {
		return -EINVAL;
	}

	err = read_presets_req(inst, start_index, count);
	if (err) {
		atomic_clear_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS);
	}

	return err;
}

int bt_has_client_preset_set(struct bt_has *has, uint8_t index, bool sync)
{
	struct bt_has_client *inst = HAS_INST(has);
	uint8_t opcode;

	LOG_DBG("conn %p index 0x%02x", (void *)inst->conn, index);

	if (!inst->conn) {
		return -ENOTCONN;
	}

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		return -EINVAL;
	}

	if (sync && (inst->has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) == 0) {
		return -EOPNOTSUPP;
	}

	if (atomic_test_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS) ||
	    atomic_test_and_set_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS)) {
		return -EBUSY;
	}

	opcode = sync ? BT_HAS_OP_SET_ACTIVE_PRESET_SYNC : BT_HAS_OP_SET_ACTIVE_PRESET;

	return preset_set(inst, opcode, index);
}

int bt_has_client_preset_next(struct bt_has *has, bool sync)
{
	struct bt_has_client *inst = HAS_INST(has);
	uint8_t opcode;

	LOG_DBG("conn %p sync %d", (void *)inst->conn, sync);

	if (!inst->conn) {
		return -ENOTCONN;
	}

	if (sync && (inst->has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) == 0) {
		return -EOPNOTSUPP;
	}

	if (atomic_test_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS) ||
	    atomic_test_and_set_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS)) {
		return -EBUSY;
	}

	opcode = sync ? BT_HAS_OP_SET_NEXT_PRESET_SYNC : BT_HAS_OP_SET_NEXT_PRESET;

	return preset_set_next_or_prev(inst, opcode);
}

int bt_has_client_preset_prev(struct bt_has *has, bool sync)
{
	struct bt_has_client *inst = HAS_INST(has);
	uint8_t opcode;

	LOG_DBG("conn %p sync %d", (void *)inst->conn, sync);

	if (!inst->conn) {
		return -ENOTCONN;
	}

	if (sync && (inst->has.features & BT_HAS_FEAT_PRESET_SYNC_SUPP) == 0) {
		return -EOPNOTSUPP;
	}

	if (atomic_test_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS) ||
	    atomic_test_and_set_bit(inst->flags, HAS_CLIENT_CP_OPERATION_IN_PROGRESS)) {
		return -EBUSY;
	}

	opcode = sync ? BT_HAS_OP_SET_PREV_PRESET_SYNC : BT_HAS_OP_SET_PREV_PRESET;

	return preset_set_next_or_prev(inst, opcode);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_has_client *inst = inst_by_conn(conn);

	if (!inst) {
		return;
	}

	if (atomic_test_bit(inst->flags, HAS_CLIENT_DISCOVER_IN_PROGRESS)) {
		discover_failed(conn, -ECONNABORTED);
	}

	inst_cleanup(inst);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.disconnected = disconnected,
};
