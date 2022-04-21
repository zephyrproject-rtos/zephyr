/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/has.h>
#include <sys/check.h>

#include "has_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HAS_CLIENT)
#define LOG_MODULE_NAME bt_has_client
#include "../common/log.h"

#define HAS_INST(_has) CONTAINER_OF(_has, struct has_inst, has)
#define HANDLE_IS_VALID(handle) ((handle) != 0x0000)

enum {
	HAS_DISCOVER_IN_PROGRESS,

	HAS_NUM_FLAGS, /* keep as last */
};

static struct has_inst {
	/** Common profile reference object */
	struct bt_has has;

	/** Profile connection reference */
	struct bt_conn *conn;

	/** Internal flags */
	ATOMIC_DEFINE(flags, HAS_NUM_FLAGS);

	/* GATT procedure parameters */
	struct {
		struct bt_uuid_16 uuid;
		union {
			struct bt_gatt_read_params read;
			struct bt_gatt_discover_params discover;
		};
	} params;

	struct bt_gatt_subscribe_params features_subscription;
	struct bt_gatt_subscribe_params control_point_subscription;
	struct bt_gatt_subscribe_params active_index_subscription;
} has_insts[CONFIG_BT_MAX_CONN];

static const struct bt_has_client_cb *client_cb;

static struct has_inst *inst_by_conn(struct bt_conn *conn)
{
	struct has_inst *inst = &has_insts[bt_conn_index(conn)];

	if (inst->conn == conn) {
		return inst;
	}

	return NULL;
}

static void inst_cleanup(struct has_inst *inst)
{
	bt_conn_unref(inst->conn);

	(void)memset(inst, 0, sizeof(*inst));
}

static enum bt_has_capabilities get_capabilities(const struct has_inst *inst)
{
	enum bt_has_capabilities caps = 0;

	/* The Control Point support is optional, as the server might have no presets support */
	if (HANDLE_IS_VALID(inst->control_point_subscription.value_handle)) {
		caps |= BT_HAS_PRESET_SUPPORT;
	}

	return caps;
}

static uint8_t control_point_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t len)
{
	/* TODO: Handle Control Point PDU */

	return BT_GATT_ITER_CONTINUE;
}

static void discover_complete(struct has_inst *inst)
{
	BT_DBG("conn %p", (void *)inst->conn);

	atomic_clear_bit(inst->flags, HAS_DISCOVER_IN_PROGRESS);

	client_cb->discover(inst->conn, 0, &inst->has,
			    inst->has.features & BT_HAS_FEAT_HEARING_AID_TYPE_MASK,
			    get_capabilities(inst));

	/* If Active Preset Index supported, notify it's value */
	if (client_cb->preset_switch &&
	    HANDLE_IS_VALID(inst->active_index_subscription.value_handle)) {
		client_cb->preset_switch(&inst->has, inst->has.active_index);
	}
}

static void discover_failed(struct bt_conn *conn, int err)
{
	BT_DBG("conn %p", (void *)conn);

	client_cb->discover(conn, err, NULL, 0, 0);
}

static uint8_t active_index_update(struct has_inst *inst, const void *data, uint16_t len)
{
	struct net_buf_simple buf;
	const uint8_t prev = inst->has.active_index;

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	inst->has.active_index = net_buf_simple_pull_u8(&buf);

	BT_DBG("conn %p index 0x%02x", (void *)inst->conn, inst->has.active_index);

	return prev;
}

static uint8_t active_preset_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t len)
{
	struct has_inst *inst;
	uint8_t prev;

	BT_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (!conn) {
		/* Unpaired, stop receiving notifications from device */
		return BT_GATT_ITER_STOP;
	}

	if (!data) {
		/* Unsubscribed */
		params->value_handle = 0u;

		return BT_GATT_ITER_STOP;
	}

	inst = inst_by_conn(conn);
	if (!inst) {
		/* Ignore notification from unknown instance */
		return BT_GATT_ITER_STOP;
	}

	if (len == 0) {
		/* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	prev = active_index_update(inst, data, len);

	if (atomic_test_bit(inst->flags, HAS_DISCOVER_IN_PROGRESS)) {
		/* Got notification during discovery process, postpone the active_index callback
		 * until discovery is complete.
		 */
		return BT_GATT_ITER_CONTINUE;
	}

	if (client_cb && client_cb->preset_switch && inst->has.active_index != prev) {
		client_cb->preset_switch(&inst->has, inst->has.active_index);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void active_index_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				      struct bt_gatt_write_params *params)
{
	struct has_inst *inst = inst_by_conn(conn);

	__ASSERT(inst, "no instance for conn %p", (void *)conn);

	BT_DBG("conn %p att_err 0x%02x params %p", (void *)inst->conn, att_err, params);

	if (att_err != BT_ATT_ERR_SUCCESS) {
		/* Cleanup instance so that it can be reused */
		inst_cleanup(inst);

		discover_failed(conn, att_err);
	} else {
		discover_complete(inst);
	}
}

static int active_index_subscribe(struct has_inst *inst, uint16_t value_handle)
{
	BT_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	inst->active_index_subscription.notify = active_preset_notify_cb;
	inst->active_index_subscription.write = active_index_subscribe_cb;
	inst->active_index_subscription.value_handle = value_handle;
	inst->active_index_subscription.ccc_handle = 0x0000;
	inst->active_index_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->active_index_subscription.disc_params = &inst->params.discover;
	inst->active_index_subscription.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(inst->active_index_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(inst->conn, &inst->active_index_subscription);
}

static uint8_t active_index_read_cb(struct bt_conn *conn, uint8_t att_err,
				    struct bt_gatt_read_params *params, const void *data,
				    uint16_t len)
{
	struct has_inst *inst = inst_by_conn(conn);
	int err = att_err;

	__ASSERT(inst, "no instance for conn %p", (void *)conn);

	BT_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
	       data, len);

	if (att_err != BT_ATT_ERR_SUCCESS || len == 0) {
		goto fail;
	}

	active_index_update(inst, data, len);

	err = active_index_subscribe(inst, params->by_uuid.start_handle);
	if (err) {
		BT_ERR("Subscribe failed (err %d)", err);
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);

	return BT_GATT_ITER_STOP;
}

static int active_index_read(struct has_inst *inst)
{
	BT_DBG("conn %p", (void *)inst->conn);

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
				       struct bt_gatt_write_params *write)
{
	struct has_inst *inst = inst_by_conn(conn);
	int err = att_err;

	__ASSERT(inst, "no instance for conn %p", (void *)conn);

	BT_DBG("conn %p att_err 0x%02x", (void *)inst->conn, att_err);

	if (att_err != BT_ATT_ERR_SUCCESS) {
		goto fail;
	}

	err = active_index_read(inst);
	if (err) {
		BT_ERR("Active Preset Index read failed (err %d)", err);
		goto fail;
	}

	return;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);
}

static int control_point_subscribe(struct has_inst *inst, uint16_t value_handle,
				   uint8_t properties)
{
	BT_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	inst->control_point_subscription.notify = control_point_notify_cb;
	inst->control_point_subscription.write = control_point_subscribe_cb;
	inst->control_point_subscription.value_handle = value_handle;
	inst->control_point_subscription.ccc_handle = 0x0000;
	inst->control_point_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->control_point_subscription.disc_params = &inst->params.discover;
	atomic_set_bit(inst->control_point_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	if (IS_ENABLED(CONFIG_BT_EATT) && properties & BT_GATT_CHRC_NOTIFY) {
		inst->control_point_subscription.value = BT_GATT_CCC_INDICATE | BT_GATT_CCC_NOTIFY;
	} else {
		inst->control_point_subscription.value = BT_GATT_CCC_INDICATE;
	}

	return bt_gatt_subscribe(inst->conn, &inst->control_point_subscription);
}

static uint8_t control_point_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					 struct bt_gatt_discover_params *params)
{
	struct has_inst *inst = inst_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	__ASSERT(inst, "no instance for conn %p", (void *)conn);

	BT_DBG("conn %p attr %p params %p", (void *)inst->conn, attr, params);

	if (!attr) {
		BT_INFO("Control Point not found");
		discover_complete(inst);
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	err = control_point_subscribe(inst, chrc->value_handle, chrc->properties);
	if (err) {
		BT_ERR("Subscribe failed (err %d)", err);

		/* Cleanup instance so that it can be reused */
		inst_cleanup(inst);

		discover_failed(conn, err);
	}

	return BT_GATT_ITER_STOP;
}

static int control_point_discover(struct has_inst *inst)
{
	BT_DBG("conn %p", (void *)inst->conn);

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

static void features_update(struct has_inst *inst, const void *data, uint16_t len)
{
	struct net_buf_simple buf;

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	inst->has.features = net_buf_simple_pull_u8(&buf);

	BT_DBG("conn %p features 0x%02x", (void *)inst->conn, inst->has.features);
}

static uint8_t features_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct has_inst *inst = inst_by_conn(conn);
	int err = att_err;

	__ASSERT(inst, "no instance for conn %p", (void *)conn);

	BT_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
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
		BT_ERR("Control Point discover failed (err %d)", err);
		goto fail;
	}

	return BT_GATT_ITER_STOP;

fail:
	/* Cleanup instance so that it can be reused */
	inst_cleanup(inst);

	discover_failed(conn, err);

	return BT_GATT_ITER_STOP;
}

static int features_read(struct has_inst *inst, uint16_t value_handle)
{
	BT_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	inst->params.read.func = features_read_cb;
	inst->params.read.handle_count = 1u;
	inst->params.read.single.handle = value_handle;
	inst->params.read.single.offset = 0u;

	return bt_gatt_read(inst->conn, &inst->params.read);
}

static void features_subscribe_cb(struct bt_conn *conn, uint8_t att_err,
				  struct bt_gatt_write_params *params)
{
	struct has_inst *inst = inst_by_conn(conn);
	int err = att_err;

	__ASSERT(inst, "no instance for conn %p", (void *)conn);

	BT_DBG("conn %p att_err 0x%02x params %p", (void *)inst->conn, att_err, params);

	if (att_err != BT_ATT_ERR_SUCCESS) {
		goto fail;
	}

	err = features_read(inst, inst->features_subscription.value_handle);
	if (err) {
		BT_ERR("Read failed (err %d)", err);
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
	struct has_inst *inst;

	BT_DBG("conn %p params %p data %p len %u", (void *)conn, params, data, len);

	if (!conn) {
		/* Unpaired, stop receiving notifications from device */
		return BT_GATT_ITER_STOP;
	}

	if (!data) {
		/* Unsubscribed */
		params->value_handle = 0u;

		return BT_GATT_ITER_STOP;
	}

	inst = inst_by_conn(conn);
	if (!inst) {
		/* Ignore notification from unknown instance */
		return BT_GATT_ITER_STOP;
	}

	if (len == 0) {
		/* Ignore empty notification */
		return BT_GATT_ITER_CONTINUE;
	}

	features_update(inst, data, len);

	return BT_GATT_ITER_CONTINUE;
}

static int features_subscribe(struct has_inst *inst, uint16_t value_handle)
{
	BT_DBG("conn %p handle 0x%04x", (void *)inst->conn, value_handle);

	inst->features_subscription.notify = features_notify_cb;
	inst->features_subscription.write = features_subscribe_cb;
	inst->features_subscription.value_handle = value_handle;
	inst->features_subscription.ccc_handle = 0x0000;
	inst->features_subscription.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	inst->features_subscription.disc_params = &inst->params.discover;
	inst->features_subscription.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(inst->features_subscription.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	return bt_gatt_subscribe(inst->conn, &inst->features_subscription);
}

static uint8_t features_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    struct bt_gatt_discover_params *params)
{
	struct has_inst *inst = inst_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	__ASSERT(inst, "no instance for conn %p", (void *)conn);

	BT_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (!attr) {
		err = -ENOENT;
		goto fail;
	}

	chrc = attr->user_data;

	/* Subscribe first if notifications are supported, otherwise read the features */
	if (chrc->properties & BT_GATT_CHRC_NOTIFY) {
		err = features_subscribe(inst, chrc->value_handle);
		if (err) {
			BT_ERR("Subscribe failed (err %d)", err);
			goto fail;
		}
	} else {
		err = features_read(inst, chrc->value_handle);
		if (err) {
			BT_ERR("Read failed (err %d)", err);
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

static int features_discover(struct has_inst *inst)
{
	BT_DBG("conn %p", (void *)inst->conn);

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
	struct has_inst *inst;
	int err;

	BT_DBG("conn %p", (void *)conn);

	CHECKIF(!conn || !client_cb || !client_cb->discover) {
		return -EINVAL;
	}

	inst = &has_insts[bt_conn_index(conn)];

	if (atomic_test_and_set_bit(inst->flags, HAS_DISCOVER_IN_PROGRESS)) {
		return -EBUSY;
	}

	if (inst->conn) {
		return -EALREADY;
	}

	inst->conn = bt_conn_ref(conn);

	err = features_discover(inst);
	if (err) {
		atomic_clear_bit(inst->flags, HAS_DISCOVER_IN_PROGRESS);
	}

	return err;
}

int bt_has_client_conn_get(const struct bt_has *has, struct bt_conn **conn)
{
	struct has_inst *inst = HAS_INST(has);

	*conn = bt_conn_ref(inst->conn);

	return 0;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct has_inst *inst = inst_by_conn(conn);

	if (!inst) {
		return;
	}

	if (atomic_test_bit(inst->flags, HAS_DISCOVER_IN_PROGRESS)) {
		discover_failed(conn, -ECONNABORTED);
	}

	inst_cleanup(inst);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.disconnected = disconnected,
};
