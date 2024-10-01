/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/mesh.h>

#include "common/bt_str.h"

#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "prov.h"
#include "pb_gatt.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"
#include "pb_gatt_srv.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_pb_gatt_srv);

#define ADV_OPT_PROV                                                           \
	(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_SCANNABLE |                 \
	 BT_LE_ADV_OPT_ONE_TIME | ADV_OPT_USE_IDENTITY)

#define FAST_ADV_TIME (60LL * MSEC_PER_SEC)

static int64_t fast_adv_timestamp;

static int gatt_send(struct bt_conn *conn,
		     const void *data, uint16_t len,
		     bt_gatt_complete_func_t end, void *user_data);

static struct bt_mesh_proxy_role *cli;
static bool service_registered;

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_PROV:
		LOG_DBG("Mesh Provisioning PDU");
		bt_mesh_pb_gatt_recv(role->conn, &role->buf);
		break;

	default:
		LOG_WRN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static ssize_t gatt_recv(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	if (cli->conn != conn) {
		LOG_ERR("No PB-GATT Client found");
		return -ENOTCONN;
	}

	if (len < 1) {
		LOG_WRN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (PDU_TYPE(data) != BT_MESH_PROXY_PROV) {
		LOG_WRN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	return bt_mesh_proxy_msg_recv(conn, buf, len);
}

static void gatt_connected(struct bt_conn *conn, uint8_t conn_err)
{
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err || info.role != BT_CONN_ROLE_PERIPHERAL || !service_registered ||
	    bt_mesh_is_provisioned() || info.id != BT_ID_DEFAULT || cli)  {
		return;
	}

	cli = bt_mesh_proxy_role_setup(conn, gatt_send, proxy_msg_recv);

	LOG_DBG("conn %p err 0x%02x", (void *)conn, conn_err);
}

static void gatt_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;
	int err;

	err = bt_conn_get_info(conn, &info);
	if (err || info.role != BT_CONN_ROLE_PERIPHERAL || !service_registered ||
	    info.id != BT_ID_DEFAULT || !cli || cli->conn != conn) {
		return;
	}

	bt_mesh_proxy_role_cleanup(cli);
	cli = NULL;

	bt_mesh_pb_gatt_close(conn);

	if (bt_mesh_is_provisioned()) {
		(void)bt_mesh_pb_gatt_srv_disable();
	}
}

static void prov_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t prov_ccc_write(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, uint16_t value)
{
	if (cli->conn != conn) {
		LOG_ERR("No PB-GATT Client found");
		return -ENOTCONN;
	}

	LOG_DBG("value 0x%04x", value);

	if (value != BT_GATT_CCC_NOTIFY) {
		LOG_WRN("Client wrote 0x%04x instead enabling notify", value);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	bt_mesh_pb_gatt_start(conn);

	return sizeof(value);
}

/* Mesh Provisioning Service Declaration */
static struct _bt_gatt_ccc prov_ccc =
	BT_GATT_CCC_INITIALIZER(prov_ccc_changed, prov_ccc_write, NULL);

static struct bt_gatt_attr prov_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MESH_PROV),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROV_DATA_IN,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL, gatt_recv,
			       NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_MESH_PROV_DATA_OUT,
			       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC_MANAGED(&prov_ccc,
			    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service prov_svc = BT_GATT_SERVICE(prov_attrs);

int bt_mesh_pb_gatt_srv_enable(void)
{
	LOG_DBG("");

	if (bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	if (service_registered) {
		return -EBUSY;
	}

	(void)bt_gatt_service_register(&prov_svc);
	service_registered = true;
	fast_adv_timestamp = k_uptime_get();

	return 0;
}

int bt_mesh_pb_gatt_srv_disable(void)
{
	LOG_DBG("");

	if (!service_registered) {
		return -EALREADY;
	}

	bt_gatt_service_unregister(&prov_svc);
	service_registered = false;

	bt_mesh_adv_gatt_update();

	return 0;
}

static uint8_t prov_svc_data[20] = {
	BT_UUID_16_ENCODE(BT_UUID_MESH_PROV_VAL),
};

static const struct bt_data prov_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_MESH_PROV_VAL)),
	BT_DATA(BT_DATA_SVC_DATA16, prov_svc_data, sizeof(prov_svc_data)),
};

static size_t gatt_prov_adv_create(struct bt_data prov_sd[2])
{
	size_t prov_sd_len = 0;

	const struct bt_mesh_prov *prov = bt_mesh_prov_get();
	size_t uri_len;

	memcpy(prov_svc_data + 2, prov->uuid, 16);
	sys_put_be16(prov->oob_info, prov_svc_data + 18);

	if (!prov->uri) {
		goto dev_name;
	}

	uri_len = strlen(prov->uri);
	if (uri_len > 29) {
		/* There's no way to shorten an URI */
		LOG_WRN("Too long URI to fit advertising packet");
		goto dev_name;
	}

	prov_sd[prov_sd_len].type = BT_DATA_URI;
	prov_sd[prov_sd_len].data_len = uri_len;
	prov_sd[prov_sd_len].data = (const uint8_t *)prov->uri;

	prov_sd_len += 1;

dev_name:
#if defined(CONFIG_BT_MESH_PB_GATT_USE_DEVICE_NAME)
	prov_sd[prov_sd_len].type = BT_DATA_NAME_COMPLETE;
	prov_sd[prov_sd_len].data_len = sizeof(CONFIG_BT_DEVICE_NAME) - 1;
	prov_sd[prov_sd_len].data = CONFIG_BT_DEVICE_NAME;

	prov_sd_len += 1;
#endif

	return prov_sd_len;
}

static int gatt_send(struct bt_conn *conn,
		     const void *data, uint16_t len,
		     bt_gatt_complete_func_t end, void *user_data)
{
	LOG_DBG("%u bytes: %s", len, bt_hex(data, len));

	struct bt_gatt_notify_params params = {
		.data = data,
		.len = len,
		.attr = &prov_attrs[3],
		.user_data = user_data,
		.func = end,
	};

	return bt_gatt_notify_cb(conn, &params);
}

int bt_mesh_pb_gatt_srv_adv_start(void)
{
	LOG_DBG("");

	if (!service_registered || bt_mesh_is_provisioned() ||
	    !bt_mesh_proxy_has_avail_conn()) {
		return -ENOTSUP;
	}

	struct bt_le_adv_param fast_adv_param = {
		.id = BT_ID_DEFAULT,
		.options = ADV_OPT_PROV,
		ADV_FAST_INT,
	};
	struct bt_data prov_sd[2];
	size_t prov_sd_len;
	int64_t timestamp = fast_adv_timestamp;
	int64_t elapsed_time = k_uptime_delta(&timestamp);

	prov_sd_len = gatt_prov_adv_create(prov_sd);

	if (elapsed_time > FAST_ADV_TIME) {
		struct bt_le_adv_param slow_adv_param = {
			.id = BT_ID_DEFAULT,
			.options = ADV_OPT_PROV,
			ADV_SLOW_INT,
		};

		return bt_mesh_adv_gatt_start(&slow_adv_param, SYS_FOREVER_MS, prov_ad,
					      ARRAY_SIZE(prov_ad), prov_sd, prov_sd_len);
	}

	LOG_DBG("remaining fast adv time (%lld ms)", (FAST_ADV_TIME - elapsed_time));
	/* Advertise 60 seconds using fast interval */
	return bt_mesh_adv_gatt_start(&fast_adv_param, (FAST_ADV_TIME - elapsed_time),
				      prov_ad, ARRAY_SIZE(prov_ad),
				      prov_sd, prov_sd_len);

}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = gatt_connected,
	.disconnected = gatt_disconnected,
};
