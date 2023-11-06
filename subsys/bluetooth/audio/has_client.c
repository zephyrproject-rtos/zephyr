/*
 * Copyright (c) 2022-2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>

#include "../bluetooth/host/hci_core.h"
#include "has_internal.h"

#include "common/bt_str.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_has_client, CONFIG_BT_HAS_CLIENT_LOG_LEVEL);

enum bt_has_client_state {
	STATE_IDLE,
	STATE_DISCONNECTED,
	STATE_CONNECTED,
	STATE_CONNECTING,
	STATE_DISCONNECTING,
	STATE_RECONNECTING,
};

static struct bt_has_client {
	/** Profile connection reference */
	struct bt_conn *conn;

	/** Instance state */
	enum bt_has_client_state state;

	/** Which local identity the connection was created with */
	uint8_t id;

	/** Peer device address */
	bt_addr_le_t addr;

	/** Cached Hearing Aid Features characteristic value */
	enum bt_has_features features;

	/** Cached Active preset index characteristic value */
	uint8_t active_index;

	/** GATT procedure parameters */
	union {
		struct {
			union {
				struct bt_gatt_read_params read_params;
				struct bt_gatt_discover_params discover_params;
			};
			struct bt_uuid_16 uuid;
		};
		struct bt_gatt_write_params write_params;
	};

	struct bt_gatt_subscribe_params features_sub;
	struct bt_gatt_subscribe_params control_point_sub;
	struct bt_gatt_subscribe_params active_index_sub;

	/** Parameters currently used */
	void *params;

	/* Profile connection error */
	int err;

	/** TODO: Make as a state */
	bool unbind;
} clients[CONFIG_BT_MAX_CONN];

static const struct bt_has_client_cb *client_cb;

static void inst_state_set(struct bt_has_client *inst, enum bt_has_client_state state);

static struct bt_has_client *inst_find_by_conn(const struct bt_conn *conn)
{
	struct bt_has_client *inst = &clients[bt_conn_index(conn)];

	if (inst->conn == conn) {
		return inst;
	}

	return NULL;
}

static struct bt_has_client *inst_find_by_addr(const bt_addr_le_t *addr)
{
	for (size_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (bt_addr_le_eq(&clients[i].addr, addr)) {
			return &clients[i];
		}
	}

	return NULL;
}

static struct bt_has_client *inst_alloc(const bt_addr_le_t *addr)
{
	struct bt_has_client *inst = NULL;

	__ASSERT_NO_MSG(addr != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (bt_addr_le_eq(&clients[i].addr, BT_ADDR_LE_ANY)) {
			inst = &clients[i];
			break;
		}
	}

	if (inst == NULL) {
		return NULL;
	}

	memset(inst, 0, sizeof(*inst));

	bt_addr_le_copy(&inst->addr, addr);

	return inst;
}

static bool is_chrc_discovered(const struct bt_gatt_subscribe_params *params)
{
	return params->value_handle != 0;
}

static void notify_inst_connected(struct bt_has_client *inst)
{
	if (client_cb->connected) {
		client_cb->connected(inst, inst->err);
	}

	if (inst->err != 0) {
		return;
	}

	/* If Active Preset Index supported, notify it's value */
	if (client_cb->preset_switch && is_chrc_discovered(&inst->active_index_sub)) {
		client_cb->preset_switch(inst, inst->active_index);
	}
}

static void notify_inst_disconnected(struct bt_has_client *inst)
{
	if (client_cb->disconnected) {
		client_cb->disconnected(inst);
	}
}

static void notify_inst_unbound(struct bt_has_client *inst, int err)
{
	if (client_cb->unbound) {
		client_cb->unbound(inst, err);
	}
}

static inline const char *state2str(enum bt_has_client_state state)
{
	switch (state) {
	case STATE_IDLE:
		return "idle";
	case STATE_CONNECTED:
		return "connected";
	case STATE_CONNECTING:
		return "connecting";
	case STATE_DISCONNECTING:
		return "disconnecting";
	case STATE_DISCONNECTED:
		return "disconnected";
	case STATE_RECONNECTING:
		return "reconnecting";
	default:
		return "(unknown)";
	}
}

static int inst_disconnect(struct bt_has_client *inst)
{
	int err = 0;

	LOG_DBG("conn %p", (void *)inst->conn);

	if (inst->params == NULL &&
	    inst->features_sub.notify == NULL &&
	    inst->active_index_sub.notify == NULL &&
	    inst->control_point_sub.notify == NULL) {
		inst_state_set(inst, STATE_DISCONNECTED);
		return 0;
	}

	if (inst->conn == NULL) {
		return -ENOTCONN;
	}

	if (inst->params != NULL) {
		bt_gatt_cancel(inst->conn, inst->params);
	}

	/* TODO: Handle unsubscribe return value */
	if (inst->features_sub.notify != NULL) {
		err = bt_gatt_unsubscribe(inst->conn, &inst->features_sub);
		if (err != 0) {
			LOG_DBG("features unsubscribe err %d", err);
		}
	} else if (inst->active_index_sub.notify != NULL) {
		err = bt_gatt_unsubscribe(inst->conn, &inst->active_index_sub);
		if (inst->err != 0) {
			LOG_DBG("active index unsubscribe err %d", err);
		}
	} else if (inst->control_point_sub.notify != NULL) {
		err = bt_gatt_unsubscribe(inst->conn, &inst->control_point_sub);
		if (err != 0) {
			LOG_DBG("control point unsubscribe err %d", err);
		}
	}

	return err;
}

static void inst_state_set(struct bt_has_client *inst, enum bt_has_client_state state)
{
	enum bt_has_client_state old_state;

	LOG_DBG("conn %p %s -> %s", (void *)inst->conn, state2str(inst->state), state2str(state));

	old_state = inst->state;
	inst->state = state;

	/* Actions needed for exiting the old state */
	switch (old_state) {
	default:
		break;
	}

	/* Actions needed for entering the new state */
	switch (state) {
	case STATE_CONNECTED:
		notify_inst_connected(inst);
		break;
	case STATE_DISCONNECTED:
		/* Pass error to bt_has_client_cb.connected if connection failed */
		if (inst->err != 0) {
			notify_inst_connected(inst);
		} else {
			notify_inst_disconnected(inst);
		}

		bt_conn_unref(inst->conn);
		inst->conn = NULL;

		if (inst->unbind || !bt_addr_le_is_bonded(inst->id, &inst->addr)) {
			inst_state_set(inst, STATE_IDLE);
		}

		break;
	case STATE_CONNECTING:
	case STATE_RECONNECTING:
		break;
	case STATE_DISCONNECTING:
		inst_disconnect(inst);
		break;
	case STATE_IDLE:
		if (inst->err == 0) {
			/* Call bt_has_client_cb.unbound callback only if the service
			 * has been bound (connected)
			 */
			notify_inst_unbound(inst, 0);
		}

		memset(inst, 0, sizeof(*inst));
		break;
	default:
		LOG_WRN("no valid (%u) state was set", state);
		break;
	}
}

static enum bt_has_capabilities get_capabilities(const struct bt_has_client *inst)
{
	enum bt_has_capabilities caps = 0;

	/* The Control Point support is optional, as the server might have no presets support */
	if (is_chrc_discovered(&inst->control_point_sub)) {
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

	client_cb->preset_read_rsp(inst, &record, !!pdu->is_last);
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

	client_cb->preset_update(inst, pdu->prev_index, &record, is_last);
}

static void handle_preset_deleted(struct bt_has_client *inst, struct net_buf_simple *buf,
				  bool is_last)
{
	if (buf->len < sizeof(uint8_t)) {
		LOG_ERR("malformed PDU");
		return;
	}

	client_cb->preset_deleted(inst, net_buf_simple_pull_u8(buf), is_last);
}

static void handle_preset_availability(struct bt_has_client *inst, struct net_buf_simple *buf,
				       bool available, bool is_last)
{
	if (buf->len < sizeof(uint8_t)) {
		LOG_ERR("malformed PDU");
		return;
	}

	client_cb->preset_availability(inst, net_buf_simple_pull_u8(buf), available, is_last);
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

static void subscribed(struct bt_gatt_subscribe_params *params)
{
	/* Clear the subscribe callback to not be called when subscription is removed */
	params->subscribe = NULL;
}

static void unsubscribed(struct bt_has_client *inst, struct bt_gatt_subscribe_params *params)
{
	params->notify = NULL;

	if (inst->state != STATE_DISCONNECTING) {
		return;
	}

	if (inst->conn != NULL) {
		/* Continue disconnecting instance */
		inst_disconnect(inst);
	}
}

static bool is_subscribed(const struct bt_gatt_subscribe_params *params)
{
	return params->notify != NULL;
}

static void control_point_handler(struct bt_has_client *inst, const void *data, uint16_t len)
{
	const struct bt_has_cp_hdr *hdr;
	struct net_buf_simple buf;

	LOG_DBG("conn %p data %p len %u", (void *)inst->conn, data, len);

	if (len < sizeof(*hdr)) {
		/* Ignore malformed notification */
		return;
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
}

static void control_point_write_cb(struct bt_conn *conn, uint8_t err,
				   struct bt_gatt_write_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, write_params);

	LOG_DBG("conn %p err 0x%02x param %p", (void *)conn, err, params);

	inst->params = NULL;

	if (client_cb && client_cb->cmd_status) {
		client_cb->cmd_status(inst, err);
	}
}

static int control_point_write(struct bt_has_client *inst, struct net_buf_simple *buf)
{
	int err;

	if (!is_chrc_discovered(&inst->control_point_sub)) {
		return -ENOTSUP;
	}

	if (inst->params != NULL) {
		return -EBUSY;
	}

	inst->write_params.func = control_point_write_cb;
	inst->write_params.handle = inst->control_point_sub.value_handle;
	inst->write_params.offset = 0U;
	inst->write_params.data = buf->data;
	inst->write_params.length = buf->len;
	inst->params = &inst->write_params;

	err = bt_gatt_write(inst->conn, &inst->write_params);
	if (err != 0) {
		LOG_ERR("bt_gatt_write err %d", err);
		inst->params = NULL;

		return err;
	}

	return 0;
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

	return control_point_write(inst, &buf);
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

	return control_point_write(inst, &buf);
}

static int preset_set_next_or_prev(struct bt_has_client *inst, uint8_t opcode)
{
	struct bt_has_cp_hdr *hdr;

	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*hdr));

	LOG_DBG("conn %p opcode 0x%02x", (void *)inst->conn, opcode);

	hdr = net_buf_simple_add(&buf, sizeof(*hdr));
	hdr->opcode = opcode;

	return control_point_write(inst, &buf);
}

static void active_index_changed(struct bt_has_client *inst, const void *data, uint16_t len)
{
	struct net_buf_simple buf;
	const uint8_t prev = inst->active_index;

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	inst->active_index = net_buf_simple_pull_u8(&buf);

	LOG_DBG("conn %p index 0x%02x", (void *)inst->conn, inst->active_index);

	/* Got notification during discovery process, postpone the active_index callback
	 * until discovery is complete.
	 */
	if (inst->state != STATE_CONNECTED) {
		return;
	}

	if (client_cb && client_cb->preset_switch && inst->active_index != prev) {
		client_cb->preset_switch(inst, inst->active_index);
	}
}

static uint8_t active_index_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				      const void *data, uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, active_index_sub);

	LOG_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (conn == NULL) {
		/* Unpaired */
		return BT_GATT_ITER_STOP;
	}

	if (data == NULL) {
		unsubscribed(inst, params);
		return BT_GATT_ITER_STOP;
	}

	if (len == 0) {
		/* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	active_index_changed(inst, data, len);

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t active_index_read_cb(struct bt_conn *conn, uint8_t att_err,
				    struct bt_gatt_read_params *params, const void *data,
				    uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, read_params);

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	inst->params = NULL;

	if (att_err != BT_ATT_ERR_SUCCESS || len == 0) {
		inst->err = len == 0 ? -EINVAL : att_err;
		inst_state_set(inst, STATE_DISCONNECTING);
	} else {
		active_index_changed(inst, data, len);
		inst_state_set(inst, STATE_CONNECTED);
	}

	return BT_GATT_ITER_STOP;
}

static int active_index_read(struct bt_has_client *inst)
{
	int err;

	LOG_DBG("conn %p handle 0x%04x", (void *)inst->conn, inst->active_index_sub.value_handle);

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->read_params;

	(void)memset(&inst->read_params, 0, sizeof(inst->read_params));
	inst->read_params.func = active_index_read_cb;
	inst->read_params.handle_count = 1u;
	inst->read_params.single.handle = inst->active_index_sub.value_handle;
	inst->read_params.single.offset = 0u;

	err = bt_gatt_read(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_read err %d", err);
		inst->params = NULL;

		return err;
	}

	return 0;
}

static void active_index_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				      struct bt_gatt_subscribe_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, active_index_sub);

	LOG_DBG("conn %p att_err 0x%02x params %p", (void *)conn, att_err, params);

	inst->params = NULL;

	if (att_err != BT_ATT_ERR_SUCCESS) {
		unsubscribed(inst, params);
		inst->err = att_err;
	} else {
		subscribed(params);
		inst->err = active_index_read(inst);
	}

	if (inst->err != 0) {
		inst_state_set(inst, STATE_DISCONNECTING);
	}
}

static int active_index_subscribe(struct bt_has_client *inst)
{
	int err;

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->active_index_sub;

	if (is_subscribed(&inst->active_index_sub)) {
		LOG_DBG("Subscribed already");
		active_index_subscribe_cb(inst->conn, 0, &inst->active_index_sub);

		return 0;
	}

	inst->active_index_sub.notify = active_index_notify_cb;
	inst->active_index_sub.subscribe = active_index_subscribe_cb;

	err = bt_gatt_subscribe(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_subscribe err %d", err);
		inst->active_index_sub.notify = NULL;
		inst->params = NULL;

		return err;
	}

	return 0;
}

static void control_point_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				       struct bt_gatt_subscribe_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, control_point_sub);

	LOG_DBG("conn %p att_err 0x%02x params %p", (void *)conn, att_err, params);

	inst->params = NULL;

	if (att_err != BT_ATT_ERR_SUCCESS) {
		unsubscribed(inst, params);
		inst->err = att_err;
	} else {
		subscribed(params);
		inst->err = active_index_subscribe(inst);
	}

	if (inst->err != 0) {
		inst_state_set(inst, STATE_DISCONNECTING);
	}
}

static uint8_t control_point_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, control_point_sub);

	LOG_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (conn == NULL) {
		/* Unpaired */
		return BT_GATT_ITER_STOP;
	}

	if (data == NULL) {
		unsubscribed(inst, params);
		return BT_GATT_ITER_STOP;
	}

	control_point_handler(inst, data, len);

	return BT_GATT_ITER_CONTINUE;
}

static int control_point_subscribe(struct bt_has_client *inst)
{
	int err;

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->control_point_sub;

	if (is_subscribed(&inst->control_point_sub)) {
		LOG_DBG("Subscribed already");
		control_point_subscribe_cb(inst->conn, 0, &inst->control_point_sub);

		return 0;
	}

	inst->control_point_sub.notify = control_point_notify_cb;
	inst->control_point_sub.subscribe = control_point_subscribe_cb;

	err = bt_gatt_subscribe(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_subscribe err %d", err);
		inst->control_point_sub.notify = NULL;
		inst->params = NULL;

		return err;
	}

	return 0;
}

static bool is_control_point_supported(const struct bt_has_client *inst)
{
	return (inst->control_point_sub.value != 0 && inst->control_point_sub.ccc_handle != 0 &&
		inst->active_index_sub.value != 0 && inst->active_index_sub.ccc_handle != 0);
}

static void features_changed(struct bt_has_client *inst, const void *data, uint16_t len)
{
	enum bt_has_features features;
	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	features = net_buf_simple_pull_u8(&buf);

	LOG_DBG("conn %p features 0x%02x", (void *)inst->conn, features);

	if (features == inst->features) {
		/* Not updated */
		return;
	}

	inst->features = features;

	/* Omit if not connected yet */
	if (inst->state != STATE_CONNECTED) {
		return;
	}

	if (client_cb != NULL && client_cb->features != NULL) {
		client_cb->features(inst, inst->features);
	}
}

static uint8_t features_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, read_params);

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	inst->params = NULL;

	if (att_err != BT_ATT_ERR_SUCCESS || len == 0) {
		inst->err = len == 0 ? -EINVAL : att_err;
		inst_state_set(inst, STATE_DISCONNECTING);
	} else {
		features_changed(inst, data, len);

		if (!is_control_point_supported(inst)) {
			inst_state_set(inst, STATE_CONNECTED);

			return BT_GATT_ITER_STOP;
		}

		if (inst->state == STATE_CONNECTING) {
			inst->err = control_point_subscribe(inst);
		} else if (inst->state == STATE_RECONNECTING) {
			inst->err = active_index_read(inst);
		}
	}

	if (inst->err != 0) {
		inst_state_set(inst, STATE_DISCONNECTING);
	}

	return BT_GATT_ITER_STOP;
}

static int features_read(struct bt_has_client *inst)
{
	int err;

	LOG_DBG("conn %p handle 0x%04x", (void *)inst->conn, inst->features_sub.value_handle);

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->read_params;

	(void)memset(&inst->read_params, 0, sizeof(inst->read_params));

	inst->read_params.func = features_read_cb;
	inst->read_params.handle_count = 1u;
	inst->read_params.single.handle = inst->features_sub.value_handle;
	inst->read_params.single.offset = 0u;

	err = bt_gatt_read(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_read err %d", err);
		inst->params = NULL;

		return err;
	}

	return 0;
}

static void features_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				  struct bt_gatt_subscribe_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, features_sub);

	LOG_DBG("conn %p att_err 0x%02x params %p", (void *)conn, att_err, params);

	inst->params = NULL;

	if (att_err != BT_ATT_ERR_SUCCESS) {
		unsubscribed(inst, params);
		inst->err = att_err;
	} else {
		subscribed(params);
		inst->err = features_read(inst);
	}

	if (inst->err != 0) {
		inst_state_set(inst, STATE_DISCONNECTING);
	}
}

static uint8_t features_notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t len)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, features_sub);

	LOG_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (conn == NULL) {
		/* Unpaired */
		return BT_GATT_ITER_STOP;
	}

	if (data == NULL) {
		unsubscribed(inst, params);
		return BT_GATT_ITER_STOP;
	}

	if (len == 0) {
		/* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	features_changed(inst, data, len);

	return BT_GATT_ITER_CONTINUE;
}

static int features_subscribe(struct bt_has_client *inst)
{
	int err;

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->features_sub;

	if (is_subscribed(&inst->features_sub)) {
		LOG_DBG("Subscribed already");
		features_subscribe_cb(inst->conn, 0, &inst->features_sub);

		return 0;
	}

	inst->features_sub.notify = features_notify_cb;
	inst->features_sub.subscribe = features_subscribe_cb;

	err = bt_gatt_subscribe(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_subscribe err %d", err);
		inst->features_sub.notify = NULL;
		inst->params = NULL;

		return err;
	}

	return 0;
}

static bool is_chrc_notifiable(const struct bt_gatt_subscribe_params *params)
{
	return params->value != 0 && params->ccc_handle != 0;
}

static bool check_ccc_discovery_results(const struct bt_has_client *inst)
{
	/* Check whether all the expected CCC dexcriptors have been found */
	return !!inst->features_sub.value == !!inst->features_sub.ccc_handle &&
	       !!inst->control_point_sub.value == !!inst->control_point_sub.ccc_handle &&
	       !!inst->active_index_sub.value == !!inst->active_index_sub.ccc_handle;
}

static uint8_t ccc_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       struct bt_gatt_discover_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, discover_params);

	if (!attr) {
		inst->params = NULL;

		if (!check_ccc_discovery_results(inst)) {
			inst->err = -ENOENT;
		} else if (is_chrc_notifiable(&inst->features_sub)) {
			inst->err = features_subscribe(inst);
		} else {
			inst->err = features_read(inst);
		}

		if (inst->err != 0) {
			inst_state_set(inst, STATE_DISCONNECTING);
		}

		return BT_GATT_ITER_STOP;
	}

	if (bt_uuid_cmp(attr->uuid, BT_UUID_HAS_HEARING_AID_FEATURES) == 0 ||
	    bt_uuid_cmp(attr->uuid, BT_UUID_HAS_PRESET_CONTROL_POINT) == 0 ||
	    bt_uuid_cmp(attr->uuid, BT_UUID_HAS_ACTIVE_PRESET_INDEX) == 0) {
		/* Store the characteristic value UUID to know which characteristic the next CCC
		 * belongs to.
		 */
		(void)memcpy(&inst->uuid, attr->uuid, sizeof(inst->uuid));
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) == 0) {
		/* Clear the characteristic value UUID stored */
		(void)memset(&inst->uuid, 0, sizeof(inst->uuid));
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
		/* Based on characteristic value UUID stored, assign the CCC value handle */
		if (bt_uuid_cmp(&inst->uuid.uuid, BT_UUID_HAS_HEARING_AID_FEATURES) == 0) {
			LOG_DBG("features ccc_handle 0x%04x", attr->handle);

			inst->features_sub.ccc_handle = attr->handle;
		} else if (bt_uuid_cmp(&inst->uuid.uuid, BT_UUID_HAS_PRESET_CONTROL_POINT) == 0) {
			LOG_DBG("control point ccc_handle 0x%04x", attr->handle);

			inst->control_point_sub.ccc_handle = attr->handle;
		} else if (bt_uuid_cmp(&inst->uuid.uuid, BT_UUID_HAS_ACTIVE_PRESET_INDEX) == 0) {
			LOG_DBG("active_index ccc_handle 0x%04x", attr->handle);

			inst->active_index_sub.ccc_handle = attr->handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static int ccc_discover(struct bt_has_client *inst, uint16_t start_handle, uint16_t end_handle)
{
	int err;

	LOG_DBG("conn %p start_handle 0x%04x end_handle 0x%04x",
		(void *)inst->conn, start_handle, end_handle);

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->discover_params;

	(void)memset(&inst->discover_params, 0, sizeof(inst->discover_params));
	inst->discover_params.func = ccc_discover_cb;
	inst->discover_params.start_handle = start_handle;
	inst->discover_params.end_handle = end_handle;
	inst->discover_params.type = BT_GATT_DISCOVER_ATTRIBUTE;

	err = bt_gatt_discover(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_discover err %d", err);
		inst->params = NULL;

		return err;
	}

	return 0;
}

static uint16_t get_first_value_handle(const struct bt_has_client *inst)
{
	uint16_t start_handle;

	start_handle = inst->features_sub.value_handle;

	if (inst->control_point_sub.value_handle > 0) {
		start_handle = MIN(start_handle, inst->control_point_sub.value_handle);
	}

	if (inst->active_index_sub.value_handle > 0) {
		start_handle = MIN(start_handle, inst->active_index_sub.value_handle);
	}

	return start_handle;
}

static bool check_chrc_discovery_results(const struct bt_has_client *inst)
{
	bool features_found = is_chrc_discovered(&inst->features_sub);
	bool control_point_found = is_chrc_discovered(&inst->control_point_sub);
	bool active_index_found = is_chrc_discovered(&inst->active_index_sub);

	/* Mandatory to support Hearing Aid Features. Preset Control Point is optional.
	 * Active Index is Mandatory is Preset Control Point is supported.
	 */
	return features_found && (control_point_found == active_index_found);
}

static uint8_t chrc_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				struct bt_gatt_discover_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, discover_params);
	struct bt_gatt_chrc *chrc;

	LOG_DBG("conn %p attr %p", (void *)conn, attr);

	if (!attr) {
		/* Characteristic discovery complete */
		inst->params = NULL;

		/* Check whether handles discovered are valid */
		if (check_chrc_discovery_results(inst)) {
			uint16_t start_handle;

			start_handle = get_first_value_handle(inst);
			__ASSERT_NO_MSG(start_handle > 0);

			inst->err = ccc_discover(inst, start_handle, params->end_handle);
		} else {
			inst->err = -EINVAL;
		}

		if (inst->err != 0) {
			inst_state_set(inst, STATE_DISCONNECTING);
		}

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if (bt_uuid_cmp(chrc->uuid, BT_UUID_HAS_HEARING_AID_FEATURES) == 0) {
		if ((chrc->properties & BT_GATT_CHRC_READ) == 0) {
			LOG_DBG("features invalid properties 0x%02x", chrc->properties);

			inst->err = -EINVAL;
		} else {
			LOG_DBG("features handle 0x%04x", chrc->value_handle);

			inst->features_sub.value_handle = chrc->value_handle;
			if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
				inst->features_sub.value = BT_GATT_CCC_NOTIFY;
			}
		}
	} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HAS_PRESET_CONTROL_POINT) == 0) {
		if ((chrc->properties & (BT_GATT_CHRC_WRITE | BT_GATT_CCC_INDICATE)) == 0) {
			LOG_DBG("control point invalid properties 0x%02x", chrc->properties);

			inst->err = -EINVAL;
		} else {
			LOG_DBG("control point handle 0x%04x", chrc->value_handle);

			inst->control_point_sub.value_handle = chrc->value_handle;
			if (IS_ENABLED(CONFIG_BT_EATT) && chrc->properties & BT_GATT_CHRC_NOTIFY) {
				inst->control_point_sub.value = BT_GATT_CCC_INDICATE |
								BT_GATT_CCC_NOTIFY;
			} else {
				inst->control_point_sub.value = BT_GATT_CCC_INDICATE;
			}
		}
	} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_HAS_ACTIVE_PRESET_INDEX) == 0) {
		if ((chrc->properties & (BT_GATT_CHRC_READ | BT_GATT_CCC_NOTIFY)) == 0) {
			LOG_DBG("active index invalid properties 0x%02x", chrc->properties);

			inst->err = -EINVAL;
		} else {
			LOG_DBG("active index handle 0x%04x", chrc->value_handle);

			inst->active_index_sub.value_handle = chrc->value_handle;
			inst->active_index_sub.value = BT_GATT_CCC_NOTIFY;
		}
	}

	if (inst->err != 0) {
		inst_state_set(inst, STATE_DISCONNECTING);

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int chrc_discover(struct bt_has_client *inst, uint16_t start_handle, uint16_t end_handle)
{
	int err;

	LOG_DBG("conn %p start_handle 0x%04x end_handle 0x%04x",
		(void *)inst->conn, start_handle, end_handle);

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->discover_params;

	(void)memset(&inst->discover_params, 0, sizeof(inst->discover_params));
	inst->discover_params.func = chrc_discover_cb;
	inst->discover_params.start_handle = start_handle;
	inst->discover_params.end_handle = end_handle;
	inst->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_discover err %d", err);
		inst->params = NULL;

		return err;
	}

	return 0;
}

static uint8_t primary_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   struct bt_gatt_discover_params *params)
{
	struct bt_has_client *inst = CONTAINER_OF(params, struct bt_has_client, discover_params);
	const struct bt_gatt_service_val *service_val;

	inst->params = NULL;

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (attr == NULL) {
		inst->err = -ENOENT;
		inst_state_set(inst, STATE_DISCONNECTING);

		return BT_GATT_ITER_STOP;
	}

	service_val = attr->user_data;

	inst->err = chrc_discover(inst, attr->handle + 1, service_val->end_handle);
	if (inst->err != 0) {
		inst_state_set(inst, STATE_DISCONNECTING);
	}

	return BT_GATT_ITER_STOP;
}

static int primary_discover(struct bt_has_client *inst)
{
	int err;

	__ASSERT_NO_MSG(inst->params == NULL);
	inst->params = &inst->discover_params;

	(void)memset(&inst->discover_params, 0, sizeof(inst->discover_params));
	(void)memcpy(&inst->uuid, BT_UUID_HAS, sizeof(inst->uuid));
	inst->discover_params.uuid = &inst->uuid.uuid;
	inst->discover_params.func = primary_discover_cb;
	inst->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	inst->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(inst->conn, inst->params);
	if (err != 0) {
		LOG_ERR("bt_gatt_discover err %d", err);
		inst->params = NULL;

		return err;
	}

	return 0;
}

static void bond_deleted_cb(uint8_t id, const bt_addr_le_t *peer)
{
	struct bt_has_client *inst;

	LOG_DBG("");

	inst = inst_find_by_addr(peer);
	if (inst == NULL) {
		return;
	}

	if (inst->state == STATE_DISCONNECTED) {
		inst_state_set(inst, STATE_IDLE);
	} else {
		inst_state_set(inst, STATE_DISCONNECTED);
	}
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.bond_deleted = bond_deleted_cb,
};

int bt_has_client_init(const struct bt_has_client_cb *cb)
{
	int err;

	CHECKIF(!cb) {
		return -EINVAL;
	}

	CHECKIF(client_cb) {
		return -EALREADY;
	}

	err = bt_conn_auth_info_cb_register(&auth_info_cb);
	if (err != 0) {
		LOG_ERR("Failed to register auth_info_cb %d", err);
		return err;
	}

	client_cb = cb;

	return 0;
}

static int inst_connect(struct bt_has_client *inst)
{
	int err;

	if (inst->err == 0 && inst->state == STATE_DISCONNECTED) {
		inst_state_set(inst, STATE_RECONNECTING);

		/* Read the characteristic values on reconnection */
		err = features_read(inst);
	} else {
		inst_state_set(inst, STATE_CONNECTING);

		/* Start with Hearing Aid Service declaration discovery */
		err = primary_discover(inst);
	}

	if (err != 0) {
		inst_state_set(inst, STATE_DISCONNECTING);

		return err;
	}

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
int bt_has_client_bind(struct bt_conn *conn, struct bt_has_client **inst)
{
	struct bt_conn_info info = { 0 };
	int err;

	LOG_DBG("conn %p", (void *)conn);

	CHECKIF(conn == NULL) {
		return -EINVAL;
	}

	err = bt_conn_get_info(conn, &info);
	__ASSERT_NO_MSG(err == 0);

	*inst = inst_find_by_addr(info.le.dst);
	if (*inst == NULL) {
		*inst = inst_alloc(info.le.dst);
		__ASSERT_NO_MSG(*inst != NULL);
	}

	switch ((*inst)->state) {
	case STATE_CONNECTING:
	case STATE_RECONNECTING:
		return -EINPROGRESS;
	case STATE_CONNECTED:
		return -EALREADY;
	case STATE_IDLE:
	case STATE_DISCONNECTED:
		__ASSERT_NO_MSG((*inst)->conn == NULL);
		(*inst)->conn = bt_conn_ref(conn);
		break;
	default:
		break;
	}

	return inst_connect(*inst);
}

int bt_has_client_unbind(struct bt_has_client *inst)
{
	LOG_DBG("inst %p", inst);

	switch (inst->state) {
	case STATE_CONNECTING:
	case STATE_RECONNECTING:
		inst->err = -ECANCELED;
		/* fallthrough */
	case STATE_CONNECTED:
	case STATE_DISCONNECTING:
		break;
	default:
		return -ENOTCONN;
	}

	inst->unbind = true;

	inst_state_set(inst, STATE_DISCONNECTING);

	return 0;
}

int bt_has_client_cmd_presets_read(struct bt_has_client *inst, uint8_t start_index, uint8_t count)
{
	LOG_DBG("inst %p start_index 0x%02x count %d", inst, start_index, count);

	CHECKIF(inst == NULL) {
		return -ENOTCONN;
	}

	if (inst->state != STATE_CONNECTED) {
		return -ENOTCONN;
	}

	CHECKIF(start_index == BT_HAS_PRESET_INDEX_NONE) {
		return -EINVAL;
	}

	CHECKIF(count == 0u) {
		return -EINVAL;
	}

	return read_presets_req(inst, start_index, count);
}

int bt_has_client_cmd_preset_set(struct bt_has_client *inst, uint8_t index, bool sync)
{
	uint8_t opcode;

	LOG_DBG("inst %p index 0x%02x", inst, index);

	CHECKIF(inst == NULL) {
		return -ENOTCONN;
	}

	if (inst->state != STATE_CONNECTED) {
		return -ENOTCONN;
	}

	CHECKIF(index == BT_HAS_PRESET_INDEX_NONE) {
		return -EINVAL;
	}

	if (sync && !bt_has_features_check_preset_sync_supp(inst->features)) {
		return -EOPNOTSUPP;
	}

	opcode = sync ? BT_HAS_OP_SET_ACTIVE_PRESET_SYNC : BT_HAS_OP_SET_ACTIVE_PRESET;

	return preset_set(inst, opcode, index);
}

int bt_has_client_cmd_preset_next(struct bt_has_client *inst, bool sync)
{
	uint8_t opcode;

	LOG_DBG("inst %p sync %d", inst, sync);

	CHECKIF(inst == NULL) {
		return -ENOTCONN;
	}

	if (inst->state != STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (sync && !bt_has_features_check_preset_sync_supp(inst->features)) {
		return -EOPNOTSUPP;
	}

	opcode = sync ? BT_HAS_OP_SET_NEXT_PRESET_SYNC : BT_HAS_OP_SET_NEXT_PRESET;

	return preset_set_next_or_prev(inst, opcode);
}

int bt_has_client_cmd_preset_prev(struct bt_has_client *inst, bool sync)
{
	uint8_t opcode;

	LOG_DBG("inst %p sync %d", inst, sync);

	CHECKIF(inst == NULL) {
		return -ENOTCONN;
	}

	if (inst->state != STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if (sync && !bt_has_features_check_preset_sync_supp(inst->features)) {
		return -EOPNOTSUPP;
	}

	opcode = sync ? BT_HAS_OP_SET_PREV_PRESET_SYNC : BT_HAS_OP_SET_PREV_PRESET;

	return preset_set_next_or_prev(inst, opcode);
}

int bt_has_client_info_get(const struct bt_has_client *inst, struct bt_has_client_info *info)
{
	CHECKIF(inst == NULL) {
		LOG_ERR("NULL inst");

		return -EINVAL;
	}

	CHECKIF(info == NULL) {
		LOG_ERR("NULL info");

		return -EINVAL;
	}

	info->addr = &inst->addr;
	info->id = inst->id;
	info->type = bt_has_features_get_type(inst->features);
	info->features = inst->features;
	info->caps = get_capabilities(inst);
	info->active_index = inst->active_index;

	return 0;
}

int bt_has_client_conn_get(const struct bt_has_client *inst, struct bt_conn **conn)
{
	CHECKIF(inst == NULL) {
		LOG_ERR("NULL inst");

		return -EINVAL;
	}

	CHECKIF(conn == NULL) {
		LOG_ERR("NULL conn");

		return -EINVAL;
	}

	if (inst->conn == NULL) {
		return -ENOTCONN;
	}

	*conn = inst->conn;

	return 0;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info = { 0 };
	struct bt_has_client *inst;

	LOG_DBG("conn %p err %d", (void *)conn, err);

	if (err != 0) {
		return;
	}

	err = bt_conn_get_info(conn, &info);
	__ASSERT_NO_MSG(err == 0);

	inst = inst_find_by_addr(info.le.dst);
	if (inst == NULL) {
		return;
	}

	__ASSERT_NO_MSG(inst->conn == NULL);
	inst->conn = bt_conn_ref(conn);

	inst->err = inst_connect(inst);
	if (inst->err != 0) {
		LOG_ERR("Failed to connect profile (err %d)", inst->err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_has_client *inst;

	LOG_DBG("conn %p reason %d", (void *)conn, reason);

	inst = inst_find_by_conn(conn);
	if (inst == NULL) {
		return;
	}

	if (inst->state == STATE_CONNECTING ||
	    inst->state == STATE_RECONNECTING) {
		inst->err = -ECONNABORTED;
	}

	inst_state_set(inst, STATE_DISCONNECTED);
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	struct bt_has_client *inst;

	LOG_DBG("conn %p %s -> %s", (void *)conn, bt_addr_le_str(rpa), bt_addr_le_str(identity));

	inst = inst_find_by_conn(conn);
	if (inst != NULL) {
		bt_addr_le_copy(&inst->addr, identity);
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
	.identity_resolved = identity_resolved,
};
