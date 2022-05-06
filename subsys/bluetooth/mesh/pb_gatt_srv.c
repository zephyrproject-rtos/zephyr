/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2021 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_PROV)
#define LOG_MODULE_NAME bt_mesh_pb_gatt_srv
#include "common/log.h"

#include "mesh.h"
#include "adv.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "host/ecc.h"
#include "prov.h"
#include "pb_gatt.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "proxy_msg.h"
#include "pb_gatt_srv.h"

#if defined(CONFIG_BT_MESH_PB_GATT_USE_DEVICE_NAME)
#define ADV_OPT_USE_NAME BT_LE_ADV_OPT_USE_NAME
#else
#define ADV_OPT_USE_NAME 0
#endif

#define ADV_OPT_PROV                                                           \
	(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_SCANNABLE |                 \
	 BT_LE_ADV_OPT_ONE_TIME | ADV_OPT_USE_IDENTITY |                       \
	 ADV_OPT_USE_NAME)

static bool prov_fast_adv;

static int gatt_send(struct bt_conn *conn,
		     const void *data, uint16_t len,
		     bt_gatt_complete_func_t end, void *user_data);

static struct bt_mesh_proxy_role *cli;
static bool service_registered;

static void proxy_msg_recv(struct bt_mesh_proxy_role *role)
{
	switch (role->msg_type) {
	case BT_MESH_PROXY_PROV:
		BT_DBG("Mesh Provisioning PDU");
		bt_mesh_pb_gatt_recv(role->conn, &role->buf);
		break;

	default:
		BT_WARN("Unhandled Message Type 0x%02x", role->msg_type);
		break;
	}
}

static ssize_t gatt_recv(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	if (cli->conn != conn) {
		BT_ERR("No PB-GATT Client found");
		return -ENOTCONN;
	}

	if (len < 1) {
		BT_WARN("Too small Proxy PDU");
		return -EINVAL;
	}

	if (PDU_TYPE(data) != BT_MESH_PROXY_PROV) {
		BT_WARN("Proxy PDU type doesn't match GATT service");
		return -EINVAL;
	}

	return bt_mesh_proxy_msg_recv(conn, buf, len);
}

static void gatt_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);
	if (info.role != BT_CONN_ROLE_PERIPHERAL ||
	    !service_registered || bt_mesh_is_provisioned()) {
		return;
	}

	cli = bt_mesh_proxy_role_setup(conn, gatt_send, proxy_msg_recv);

	BT_DBG("conn %p err 0x%02x", (void *)conn, err);
}

static void gatt_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);
	if (info.role != BT_CONN_ROLE_PERIPHERAL ||
	    !service_registered) {
		return;
	}

	if (cli) {
		bt_mesh_proxy_role_cleanup(cli);
		cli = NULL;
	}

	bt_mesh_pb_gatt_close(conn);

	if (bt_mesh_is_provisioned()) {
		(void)bt_mesh_pb_gatt_srv_disable();
	}
}

static void prov_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t prov_ccc_write(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, uint16_t value)
{
	if (cli->conn != conn) {
		BT_ERR("No PB-GATT Client found");
		return -ENOTCONN;
	}

	BT_DBG("value 0x%04x", value);

	if (value != BT_GATT_CCC_NOTIFY) {
		BT_WARN("Client wrote 0x%04x instead enabling notify", value);
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
	BT_DBG("");

	if (bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	if (service_registered) {
		return -EBUSY;
	}

	(void)bt_gatt_service_register(&prov_svc);
	service_registered = true;
	prov_fast_adv = true;

	return 0;
}

int bt_mesh_pb_gatt_srv_disable(void)
{
	BT_DBG("");

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

static size_t gatt_prov_adv_create(struct bt_data prov_sd[1])
{
	const struct bt_mesh_prov *prov = bt_mesh_prov_get();
	size_t uri_len;

	memcpy(prov_svc_data + 2, prov->uuid, 16);
	sys_put_be16(prov->oob_info, prov_svc_data + 18);

	if (!prov->uri) {
		return 0;
	}

	uri_len = strlen(prov->uri);
	if (uri_len > 29) {
		/* There's no way to shorten an URI */
		BT_WARN("Too long URI to fit advertising packet");
		return 0;
	}

	prov_sd[0].type = BT_DATA_URI;
	prov_sd[0].data_len = uri_len;
	prov_sd[0].data = (const uint8_t *)prov->uri;

	return 1;
}

static int gatt_send(struct bt_conn *conn,
		     const void *data, uint16_t len,
		     bt_gatt_complete_func_t end, void *user_data)
{
	BT_DBG("%u bytes: %s", len, bt_hex(data, len));

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
	BT_DBG("");

	if (!service_registered || bt_mesh_is_provisioned()) {
		return -ENOTSUP;
	}

	struct bt_le_adv_param fast_adv_param = {
		.options = ADV_OPT_PROV,
		ADV_FAST_INT,
	};
	struct bt_data prov_sd[1];
	size_t prov_sd_len;
	int err;

	prov_sd_len = gatt_prov_adv_create(prov_sd);

	if (!prov_fast_adv) {
		struct bt_le_adv_param slow_adv_param = {
			.options = ADV_OPT_PROV,
			ADV_SLOW_INT,
		};

		return bt_mesh_adv_gatt_start(&slow_adv_param, SYS_FOREVER_MS, prov_ad,
					      ARRAY_SIZE(prov_ad), prov_sd, prov_sd_len);
	}

	/* Advertise 60 seconds using fast interval */
	err = bt_mesh_adv_gatt_start(&fast_adv_param, (60 * MSEC_PER_SEC),
				     prov_ad, ARRAY_SIZE(prov_ad),
				     prov_sd, prov_sd_len);
	if (!err) {
		prov_fast_adv = false;
	}

	return err;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = gatt_connected,
	.disconnected = gatt_disconnected,
};
