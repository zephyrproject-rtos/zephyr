/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/sys/check.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ias_client, CONFIG_BT_IAS_CLIENT_LOG_LEVEL);

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/services/ias.h>

enum {
	IAS_DISCOVER_IN_PROGRESS,

	IAS_NUM_FLAGS, /* keep as last */
};

struct bt_ias_client {
	/* Handle for alert writes */
	uint16_t alert_level_handle;

	/** Internal flags **/
	ATOMIC_DEFINE(flags, IAS_NUM_FLAGS);

	/* Gatt discover procedure parameters */
	struct bt_gatt_discover_params discover;
};

static const struct bt_uuid *alert_lvl_uuid = BT_UUID_ALERT_LEVEL;
static const struct bt_uuid *ias_uuid = BT_UUID_IAS;

static const struct bt_ias_client_cb *ias_client_cb;
static struct bt_ias_client client_list[CONFIG_BT_MAX_CONN];

static struct bt_ias_client *client_by_conn(struct bt_conn *conn)
{
	return &client_list[bt_conn_index(conn)];
}

static void client_cleanup(struct bt_ias_client *ias_client)
{
	(void)memset(ias_client, 0, sizeof(*ias_client));
}

static void discover_complete(struct bt_conn *conn, int err)
{
	LOG_DBG("conn %p", (void *)conn);

	if (err) {
		client_cleanup(client_by_conn(conn));
		LOG_DBG("Discover failed (err %d\n)", err);
	}

	if (ias_client_cb != NULL && ias_client_cb->discover != NULL) {
		ias_client_cb->discover(conn, err);
	}
}

int bt_ias_client_alert_write(struct bt_conn *conn, enum bt_ias_alert_lvl lvl)
{
	int err;
	uint8_t lvl_u8;

	CHECKIF(conn == NULL) {
		return -ENOTCONN;
	}

	if (client_by_conn(conn)->alert_level_handle == 0) {
		return -EINVAL;
	}

	lvl_u8 = (uint8_t)lvl;

	if (lvl_u8 != BT_IAS_ALERT_LVL_NO_ALERT &&
	    lvl_u8 != BT_IAS_ALERT_LVL_MILD_ALERT &&
	    lvl_u8 != BT_IAS_ALERT_LVL_HIGH_ALERT) {
		LOG_ERR("Invalid alert value: %u", lvl_u8);
		return -EINVAL;
	}

	err = bt_gatt_write_without_response(conn,
					     client_by_conn(conn)->alert_level_handle,
					     &lvl_u8, sizeof(lvl_u8), false);
	if (err < 0) {
		LOG_ERR("IAS client level %d write failed: %d", lvl, err);
	}

	return err;
}

static uint8_t bt_ias_alert_lvl_disc_cb(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					struct bt_gatt_discover_params *discover,
					int err)
{
	const struct bt_gatt_chrc *chrc;

	atomic_clear_bit(client_by_conn(conn)->flags, IAS_DISCOVER_IN_PROGRESS);

	if (attr == NULL) {
		discover_complete(conn, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	client_by_conn(conn)->alert_level_handle = chrc->value_handle;
	discover_complete(conn, 0);

	return BT_GATT_ITER_STOP;
}

static uint8_t bt_ias_prim_disc_cb(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   struct bt_gatt_discover_params *discover,
				   int err)
{
	const struct bt_gatt_service_val *data;
	struct bt_ias_client *client = client_by_conn(conn);

	if (!attr) {
		discover_complete(conn, -ENOENT);

		return BT_GATT_ITER_STOP;
	}

	data = attr->user_data;

	client->discover.uuid = alert_lvl_uuid;
	client->discover.start_handle = attr->handle + 1;
	client->discover.end_handle = data->end_handle;
	client->discover.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	client->discover.func = bt_ias_alert_lvl_disc_cb;

	err = bt_gatt_discover(conn, &client->discover);
	if (err) {
		discover_complete(conn, err);
	}

	return BT_GATT_ITER_STOP;
}

int bt_ias_discover(struct bt_conn *conn)
{
	int err;
	struct bt_ias_client *client = client_by_conn(conn);

	CHECKIF(!conn || !ias_client_cb || !ias_client_cb->discover) {
		return -EINVAL;
	}

	if (atomic_test_bit(client->flags, IAS_DISCOVER_IN_PROGRESS)) {
		return -EBUSY;
	}

	client_cleanup(client);
	atomic_set_bit(client->flags, IAS_DISCOVER_IN_PROGRESS);

	client->discover.uuid = ias_uuid;
	client->discover.func = bt_ias_prim_disc_cb;
	client->discover.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	client->discover.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	client->discover.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(conn, &client->discover);
	if (err < 0) {
		discover_complete(conn, err);
	}

	return err;
}

int bt_ias_client_cb_register(const struct bt_ias_client_cb *cb)
{
	CHECKIF(!cb) {
		return -EINVAL;
	}

	CHECKIF(cb->discover == NULL) {
		return -EINVAL;
	}

	CHECKIF(ias_client_cb) {
		return -EALREADY;
	}

	ias_client_cb = cb;

	return 0;
}
