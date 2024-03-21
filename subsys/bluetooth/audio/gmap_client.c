/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/gmap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

#include "audio_internal.h"

LOG_MODULE_REGISTER(bt_gmap_client, CONFIG_BT_GMAP_LOG_LEVEL);

static const struct bt_uuid *gmas_uuid = BT_UUID_GMAS;
static const struct bt_uuid *gmap_role_uuid = BT_UUID_GMAP_ROLE;
static const struct bt_uuid *gmap_ugg_feat_uuid = BT_UUID_GMAP_UGG_FEAT;
static const struct bt_uuid *gmap_ugt_feat_uuid = BT_UUID_GMAP_UGT_FEAT;
static const struct bt_uuid *gmap_bgs_feat_uuid = BT_UUID_GMAP_BGS_FEAT;
static const struct bt_uuid *gmap_bgr_feat_uuid = BT_UUID_GMAP_BGR_FEAT;

static const struct bt_gmap_cb *gmap_cb;

static struct bt_gmap_client {
	/** Profile connection reference */
	struct bt_conn *conn;

	/* Remote role and features */
	enum bt_gmap_role role;
	struct bt_gmap_feat feat;

	uint16_t svc_start_handle;
	uint16_t svc_end_handle;

	bool busy;

	/* GATT procedure parameters */
	union {
		struct bt_gatt_read_params read;
		struct bt_gatt_discover_params discover;
	} params;
} gmap_insts[CONFIG_BT_MAX_CONN];

static void gmap_reset(struct bt_gmap_client *gmap_cli)
{
	if (gmap_cli->conn != NULL) {
		bt_conn_unref(gmap_cli->conn);
	}

	memset(gmap_cli, 0, sizeof(*gmap_cli));
}

static struct bt_gmap_client *client_by_conn(struct bt_conn *conn)
{
	struct bt_gmap_client *gmap_cli = &gmap_insts[bt_conn_index(conn)];

	if (gmap_cli->conn == conn) {
		return gmap_cli;
	}

	return NULL;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);

	if (gmap_cli != NULL) {
		bt_conn_unref(gmap_cli->conn);
		gmap_cli->conn = NULL;
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static void discover_complete(struct bt_gmap_client *gmap_cli)
{
	LOG_DBG("conn %p", (void *)gmap_cli->conn);

	gmap_cli->busy = false;

	if (gmap_cb->discover != NULL) {
		gmap_cb->discover(gmap_cli->conn, 0, gmap_cli->role, gmap_cli->feat);
	}
}

static void discover_failed(struct bt_gmap_client *gmap_cli, int err)
{
	struct bt_conn *conn = gmap_cli->conn;

	gmap_reset(gmap_cli);

	LOG_DBG("conn %p err %d", (void *)conn, err);

	gmap_cb->discover(conn, err, 0, (struct bt_gmap_feat){0});
}

static uint8_t bgr_feat_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	struct net_buf_simple buf;
	int err = att_err;

	__ASSERT(gmap_cli, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	if (data == NULL || att_err != BT_ATT_ERR_SUCCESS || len != sizeof(uint8_t)) {
		if (att_err == BT_ATT_ERR_SUCCESS) {
			att_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}

		discover_failed(gmap_cli, err);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	gmap_cli->feat.bgr_feat = net_buf_simple_pull_u8(&buf);
	LOG_DBG("bgr_feat 0x%02x", gmap_cli->feat.bgr_feat);

	discover_complete(gmap_cli);

	return BT_GATT_ITER_STOP;
}

static int gmap_read_bgr_feat(struct bt_gmap_client *gmap_cli, uint16_t handle)
{
	LOG_DBG("conn %p handle 0x%04x", (void *)gmap_cli->conn, handle);

	memset(&gmap_cli->params.read, 0, sizeof(gmap_cli->params.read));

	gmap_cli->params.read.func = bgr_feat_read_cb;
	gmap_cli->params.read.handle_count = 1u;
	gmap_cli->params.read.single.handle = handle;
	gmap_cli->params.read.single.offset = 0u;

	return bt_gatt_read(gmap_cli->conn, &gmap_cli->params.read);
}

static uint8_t bgr_feat_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	__ASSERT(gmap_cli != NULL, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (attr == NULL) {
		discover_failed(gmap_cli, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	/* Read features */
	err = gmap_read_bgr_feat(gmap_cli, chrc->value_handle);
	if (err != 0) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_discover_bgr_feat(struct bt_gmap_client *gmap_cli)
{
	LOG_DBG("conn %p", (void *)gmap_cli->conn);

	memset(&gmap_cli->params.discover, 0, sizeof(gmap_cli->params.discover));

	gmap_cli->params.discover.func = bgr_feat_discover_func;
	gmap_cli->params.discover.uuid = gmap_bgr_feat_uuid;
	gmap_cli->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	gmap_cli->params.discover.start_handle = gmap_cli->svc_start_handle;
	gmap_cli->params.discover.end_handle = gmap_cli->svc_end_handle;

	return bt_gatt_discover(gmap_cli->conn, &gmap_cli->params.discover);
}

static uint8_t bgs_feat_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	struct net_buf_simple buf;
	int err = att_err;

	__ASSERT(gmap_cli, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	if (data == NULL || att_err != BT_ATT_ERR_SUCCESS || len != sizeof(uint8_t)) {
		if (att_err == BT_ATT_ERR_SUCCESS) {
			att_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}

		discover_failed(gmap_cli, err);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	gmap_cli->feat.bgs_feat = net_buf_simple_pull_u8(&buf);
	LOG_DBG("bgs_feat 0x%02x", gmap_cli->feat.bgs_feat);

	if ((gmap_cli->role & BT_GMAP_ROLE_BGR) != 0) {
		err = gmap_discover_bgr_feat(gmap_cli);
	} else {
		discover_complete(gmap_cli);

		return BT_GATT_ITER_STOP;
	}

	if (err) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_read_bgs_feat(struct bt_gmap_client *gmap_cli, uint16_t handle)
{
	LOG_DBG("conn %p handle 0x%04x", (void *)gmap_cli->conn, handle);

	memset(&gmap_cli->params.read, 0, sizeof(gmap_cli->params.read));

	gmap_cli->params.read.func = bgs_feat_read_cb;
	gmap_cli->params.read.handle_count = 1u;
	gmap_cli->params.read.single.handle = handle;
	gmap_cli->params.read.single.offset = 0u;

	return bt_gatt_read(gmap_cli->conn, &gmap_cli->params.read);
}

static uint8_t bgs_feat_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	__ASSERT(gmap_cli != NULL, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (attr == NULL) {
		discover_failed(gmap_cli, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	/* Read features */
	err = gmap_read_bgs_feat(gmap_cli, chrc->value_handle);
	if (err != 0) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_discover_bgs_feat(struct bt_gmap_client *gmap_cli)
{
	LOG_DBG("conn %p", (void *)gmap_cli->conn);

	memset(&gmap_cli->params.discover, 0, sizeof(gmap_cli->params.discover));

	gmap_cli->params.discover.func = bgs_feat_discover_func;
	gmap_cli->params.discover.uuid = gmap_bgs_feat_uuid;
	gmap_cli->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	gmap_cli->params.discover.start_handle = gmap_cli->svc_start_handle;
	gmap_cli->params.discover.end_handle = gmap_cli->svc_end_handle;

	return bt_gatt_discover(gmap_cli->conn, &gmap_cli->params.discover);
}

static uint8_t ugt_feat_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	struct net_buf_simple buf;
	int err = att_err;

	__ASSERT(gmap_cli, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	if (data == NULL || att_err != BT_ATT_ERR_SUCCESS || len != sizeof(uint8_t)) {
		if (att_err == BT_ATT_ERR_SUCCESS) {
			att_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}

		discover_failed(gmap_cli, err);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	gmap_cli->feat.ugt_feat = net_buf_simple_pull_u8(&buf);
	LOG_DBG("ugt_feat 0x%02x", gmap_cli->feat.ugt_feat);

	if ((gmap_cli->role & BT_GMAP_ROLE_BGS) != 0) {
		err = gmap_discover_bgs_feat(gmap_cli);
	} else if ((gmap_cli->role & BT_GMAP_ROLE_BGR) != 0) {
		err = gmap_discover_bgr_feat(gmap_cli);
	} else {
		discover_complete(gmap_cli);

		return BT_GATT_ITER_STOP;
	}

	if (err) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_read_ugt_feat(struct bt_gmap_client *gmap_cli, uint16_t handle)
{
	LOG_DBG("conn %p handle 0x%04x", (void *)gmap_cli->conn, handle);

	memset(&gmap_cli->params.read, 0, sizeof(gmap_cli->params.read));

	gmap_cli->params.read.func = ugt_feat_read_cb;
	gmap_cli->params.read.handle_count = 1u;
	gmap_cli->params.read.single.handle = handle;
	gmap_cli->params.read.single.offset = 0u;

	return bt_gatt_read(gmap_cli->conn, &gmap_cli->params.read);
}

static uint8_t ugt_feat_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	__ASSERT(gmap_cli != NULL, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (attr == NULL) {
		discover_failed(gmap_cli, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	/* Read features */
	err = gmap_read_ugt_feat(gmap_cli, chrc->value_handle);
	if (err != 0) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_discover_ugt_feat(struct bt_gmap_client *gmap_cli)
{
	LOG_DBG("conn %p", (void *)gmap_cli->conn);

	memset(&gmap_cli->params.discover, 0, sizeof(gmap_cli->params.discover));

	gmap_cli->params.discover.func = ugt_feat_discover_func;
	gmap_cli->params.discover.uuid = gmap_ugt_feat_uuid;
	gmap_cli->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	gmap_cli->params.discover.start_handle = gmap_cli->svc_start_handle;
	gmap_cli->params.discover.end_handle = gmap_cli->svc_end_handle;

	return bt_gatt_discover(gmap_cli->conn, &gmap_cli->params.discover);
}

static uint8_t ugg_feat_read_cb(struct bt_conn *conn, uint8_t att_err,
				struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	struct net_buf_simple buf;
	int err = att_err;

	__ASSERT(gmap_cli, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	if (data == NULL || att_err != BT_ATT_ERR_SUCCESS || len != sizeof(uint8_t)) {
		if (att_err == BT_ATT_ERR_SUCCESS) {
			att_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}

		discover_failed(gmap_cli, err);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	gmap_cli->feat.ugg_feat = net_buf_simple_pull_u8(&buf);
	LOG_DBG("ugg_feat 0x%02x", gmap_cli->feat.ugg_feat);

	if ((gmap_cli->role & BT_GMAP_ROLE_UGT) != 0) {
		err = gmap_discover_ugt_feat(gmap_cli);
	} else if ((gmap_cli->role & BT_GMAP_ROLE_BGS) != 0) {
		err = gmap_discover_bgs_feat(gmap_cli);
	} else if ((gmap_cli->role & BT_GMAP_ROLE_BGR) != 0) {
		err = gmap_discover_bgr_feat(gmap_cli);
	} else {
		discover_complete(gmap_cli);

		return BT_GATT_ITER_STOP;
	}

	if (err) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_read_ugg_feat(struct bt_gmap_client *gmap_cli, uint16_t handle)
{
	LOG_DBG("conn %p handle 0x%04x", (void *)gmap_cli->conn, handle);

	memset(&gmap_cli->params.read, 0, sizeof(gmap_cli->params.read));

	gmap_cli->params.read.func = ugg_feat_read_cb;
	gmap_cli->params.read.handle_count = 1u;
	gmap_cli->params.read.single.handle = handle;
	gmap_cli->params.read.single.offset = 0u;

	return bt_gatt_read(gmap_cli->conn, &gmap_cli->params.read);
}

static uint8_t ugg_feat_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	__ASSERT(gmap_cli != NULL, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (attr == NULL) {
		discover_failed(gmap_cli, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	/* Read features */
	err = gmap_read_ugg_feat(gmap_cli, chrc->value_handle);
	if (err != 0) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_discover_ugg_feat(struct bt_gmap_client *gmap_cli)
{
	LOG_DBG("conn %p", (void *)gmap_cli->conn);

	memset(&gmap_cli->params.discover, 0, sizeof(gmap_cli->params.discover));

	gmap_cli->params.discover.func = ugg_feat_discover_func;
	gmap_cli->params.discover.uuid = gmap_ugg_feat_uuid;
	gmap_cli->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	gmap_cli->params.discover.start_handle = gmap_cli->svc_start_handle;
	gmap_cli->params.discover.end_handle = gmap_cli->svc_end_handle;

	return bt_gatt_discover(gmap_cli->conn, &gmap_cli->params.discover);
}

static uint8_t role_read_cb(struct bt_conn *conn, uint8_t att_err,
			    struct bt_gatt_read_params *params, const void *data, uint16_t len)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	struct net_buf_simple buf;
	int err = att_err;

	__ASSERT(gmap_cli, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p att_err 0x%02x params %p data %p len %u", (void *)conn, att_err, params,
		data, len);

	if (data == NULL || att_err != BT_ATT_ERR_SUCCESS || len != sizeof(uint8_t)) {
		if (att_err == BT_ATT_ERR_SUCCESS) {
			att_err = BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
		}

		discover_failed(gmap_cli, err);

		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	gmap_cli->role = net_buf_simple_pull_u8(&buf);
	LOG_DBG("role 0x%02x", gmap_cli->role);

	if ((gmap_cli->role & BT_GMAP_ROLE_UGG) != 0) {
		err = gmap_discover_ugg_feat(gmap_cli);
	} else if ((gmap_cli->role & BT_GMAP_ROLE_UGT) != 0) {
		err = gmap_discover_ugt_feat(gmap_cli);
	} else if ((gmap_cli->role & BT_GMAP_ROLE_BGS) != 0) {
		err = gmap_discover_bgs_feat(gmap_cli);
	} else if ((gmap_cli->role & BT_GMAP_ROLE_BGR) != 0) {
		err = gmap_discover_bgr_feat(gmap_cli);
	} else {
		LOG_DBG("Remote device does not support any known roles");
		err = -ECANCELED;
	}

	if (err) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_read_role(struct bt_gmap_client *gmap_cli, uint16_t handle)
{
	LOG_DBG("conn %p handle 0x%04x", (void *)gmap_cli->conn, handle);

	memset(&gmap_cli->params.read, 0, sizeof(gmap_cli->params.read));

	gmap_cli->params.read.func = role_read_cb;
	gmap_cli->params.read.handle_count = 1u;
	gmap_cli->params.read.single.handle = handle;
	gmap_cli->params.read.single.offset = 0u;

	return bt_gatt_read(gmap_cli->conn, &gmap_cli->params.read);
}

static uint8_t role_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	const struct bt_gatt_chrc *chrc;
	int err;

	__ASSERT(gmap_cli != NULL, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (attr == NULL) {
		discover_failed(gmap_cli, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	/* Read features */
	err = gmap_read_role(gmap_cli, chrc->value_handle);
	if (err != 0) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

static int gmap_discover_role(struct bt_gmap_client *gmap_cli)
{
	LOG_DBG("conn %p", (void *)gmap_cli->conn);

	memset(&gmap_cli->params.discover, 0, sizeof(gmap_cli->params.discover));

	gmap_cli->params.discover.func = role_discover_func;
	gmap_cli->params.discover.uuid = gmap_role_uuid;
	gmap_cli->params.discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	gmap_cli->params.discover.start_handle = gmap_cli->svc_start_handle;
	gmap_cli->params.discover.end_handle = gmap_cli->svc_end_handle;

	return bt_gatt_discover(gmap_cli->conn, &gmap_cli->params.discover);
}

static uint8_t gmas_discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	struct bt_gmap_client *gmap_cli = client_by_conn(conn);
	const struct bt_gatt_service_val *svc;
	int err;

	__ASSERT(gmap_cli != NULL, "no instance for conn %p", (void *)conn);

	LOG_DBG("conn %p attr %p params %p", (void *)conn, attr, params);

	if (attr == NULL) {
		discover_failed(gmap_cli, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	svc = (struct bt_gatt_service_val *)attr->user_data;
	gmap_cli->svc_start_handle = attr->handle;
	gmap_cli->svc_end_handle = svc->end_handle;

	err = gmap_discover_role(gmap_cli);
	if (err != 0) {
		discover_failed(gmap_cli, err);
	}

	return BT_GATT_ITER_STOP;
}

int bt_gmap_discover(struct bt_conn *conn)
{
	struct bt_gmap_client *gmap_cli;
	int err;

	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");

		return -EINVAL;
	}

	gmap_cli = &gmap_insts[bt_conn_index(conn)];

	if (gmap_cli->busy) {
		LOG_DBG("Busy");

		return -EBUSY;
	}

	gmap_reset(gmap_cli);

	gmap_cli->params.discover.func = gmas_discover_func;
	gmap_cli->params.discover.uuid = gmas_uuid;
	gmap_cli->params.discover.type = BT_GATT_DISCOVER_PRIMARY;
	gmap_cli->params.discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	gmap_cli->params.discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_discover(conn, &gmap_cli->params.discover);
	if (err != 0) {
		LOG_DBG("Failed to initiate discovery: %d", err);

		return -ENOEXEC;
	}

	gmap_cli->conn = bt_conn_ref(conn);

	return 0;
}

int bt_gmap_cb_register(const struct bt_gmap_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");

		return -EINVAL;
	}

	if (gmap_cb != NULL) {
		LOG_DBG("GMAP callbacks already registered");

		return -EALREADY;
	}

	gmap_cb = cb;

	return 0;
}
