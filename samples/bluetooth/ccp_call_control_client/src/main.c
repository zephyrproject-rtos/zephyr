/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(ccp_call_control_client, CONFIG_LOG_DEFAULT_LEVEL);

#define SEM_TIMEOUT K_SECONDS(10)

static struct bt_conn *peer_conn;
/* client is not static as it is used for testing purposes */
struct bt_ccp_call_control_client *client;
static struct bt_ccp_call_control_client_bearers client_bearers;

static K_SEM_DEFINE(sem_conn_state_change, 0, 1);
static K_SEM_DEFINE(sem_security_updated, 0, 1);
static K_SEM_DEFINE(sem_ccp_action_completed, 0, 1);

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);

	k_sem_give(&sem_conn_state_change);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != peer_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(peer_conn);
	peer_conn = NULL;
	client = NULL;
	memset(&client_bearers, 0, sizeof(client_bearers));
	k_sem_give(&sem_conn_state_change);
}

static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err == 0) {
		k_sem_give(&sem_security_updated);
	} else {
		LOG_ERR("Failed to set security level: %s(%u)", bt_security_err_to_str(err), err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,
};

static bool check_gtbs_support(struct bt_data *data, void *user_data)
{
	struct net_buf_simple svc_data;
	bool *connect = user_data;
	const struct bt_uuid *uuid;
	uint16_t uuid_val;

	if (data->type != BT_DATA_SVC_DATA16) {
		return true; /* Continue parsing to next AD data type */
	}

	if (data->data_len < sizeof(uuid_val)) {
		LOG_WRN("AD invalid size %u", data->data_len);
		return true; /* Continue parsing to next AD data type */
	}

	net_buf_simple_init_with_data(&svc_data, (void *)data->data, data->data_len);

	/* Pull the 16-bit service data and compare to what we are searching for */
	uuid_val = net_buf_simple_pull_le16(&svc_data);
	uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(uuid_val));
	if (bt_uuid_cmp(uuid, BT_UUID_GTBS) != 0) {
		/* We are looking for the GTBS service data */
		return true; /* Continue parsing to next AD data type */
	}

	*connect = true;

	return false; /* Stop parsing */
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bool connect = false;

	if (peer_conn != NULL) {
		/* Already connected */
		return;
	}

	/* CCP mandates that connectbale extended advertising is used by the peripherals so we
	 * ignore any scan report this is not that.
	 * We also ignore reports with poor RSSI
	 */
	if (info->adv_type != BT_GAP_ADV_TYPE_EXT_ADV ||
	    (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) == 0 ||
	    (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) == 0 || info->rssi < -70) {
		return;
	}

	(void)bt_addr_le_to_str(info->addr, addr_str, sizeof(addr_str));
	LOG_INF("Connectable device found: %s (RSSI %d)", addr_str, info->rssi);

	/* Iterate on the advertising data to see if claims GTBS support */
	bt_data_parse(ad, check_gtbs_support, &connect);

	if (connect) {
		int err;

		err = bt_le_scan_stop();
		if (err != 0) {
			LOG_ERR("Scanning failed to stop (err %d)", err);
			return;
		}

		LOG_INF("Connecting to found CCP server");
		err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT, &peer_conn);
		if (err != 0) {
			LOG_ERR("Conn create failed: %d", err);
		}
	}
}

static int scan_and_connect(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return err;
	}

	LOG_INF("Scanning successfully started");

	err = k_sem_take(&sem_conn_state_change, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_connected (err %d)", err);
		return err;
	}

	err = bt_conn_set_security(peer_conn, BT_SECURITY_L2);
	if (err != 0) {
		LOG_ERR("failed to set security (err %d)", err);
		return err;
	}

	err = k_sem_take(&sem_security_updated, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("failed to take sem_security_updated (err %d)", err);
		return err;
	}

	LOG_INF("Security successfully updated");

	return 0;
}

static void ccp_call_control_client_discover_cb(struct bt_ccp_call_control_client *client, int err,
						struct bt_ccp_call_control_client_bearers *bearers)
{
	if (err != 0) {
		LOG_ERR("Discovery failed: %d", err);
		return;
	}

	LOG_INF("Discovery completed with %s%u TBS bearers",
		bearers->gtbs_bearer != NULL ? "GTBS and " : "", bearers->tbs_count);

	memcpy(&client_bearers, bearers, sizeof(client_bearers));

	k_sem_give(&sem_ccp_action_completed);
}

static int reset_ccp_call_control_client(void)
{
	int err;

	LOG_INF("Resetting");

	if (peer_conn != NULL) {
		err = bt_conn_disconnect(peer_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			return err;
		}

		err = k_sem_take(&sem_conn_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Failed to take sem_conn_state_change: %d", err);
			return err;
		}
	}

	/* If scanning is already stopped it will still return `0` */
	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_ERR("Scanning failed to stop (err %d)", err);
		return err;
	}

	k_sem_reset(&sem_conn_state_change);

	return 0;
}

static int discover_services(void)
{
	int err;

	LOG_INF("Discovering GTBS and TBS");

	err = bt_ccp_call_control_client_discover(peer_conn, &client);
	if (err != 0) {
		LOG_ERR("Failed to discover: %d", err);
		return err;
	}

	err = k_sem_take(&sem_ccp_action_completed, SEM_TIMEOUT);
	if (err != 0) {
		LOG_ERR("Failed to take sem_ccp_action_completed: %d", err);
		return err;
	}

	return 0;
}

static int init_ccp_call_control_client(void)
{
	static struct bt_le_scan_cb scan_cbs = {
		.recv = scan_recv_cb,
	};
	static struct bt_ccp_call_control_client_cb ccp_call_control_client_cbs = {
		.discover = ccp_call_control_client_discover_cb,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth enable failed (err %d)", err);

		return err;
	}

	LOG_DBG("Bluetooth initialized");
	err = bt_le_scan_cb_register(&scan_cbs);
	if (err != 0) {
		LOG_ERR("Bluetooth enable failed (err %d)", err);

		return err;
	}

	err = bt_ccp_call_control_client_register_cb(&ccp_call_control_client_cbs);
	if (err != 0) {
		LOG_ERR("Bluetooth enable failed (err %d)", err);

		return err;
	}

	return 0;
}

int main(void)
{
	int err;

	err = init_ccp_call_control_client();
	if (err != 0) {
		return 0;
	}

	LOG_INF("CCP Call Control Client initialized");

	while (true) {
		err = reset_ccp_call_control_client();
		if (err != 0) {
			break;
		}

		/* Start scanning for CCP servers and connect to the first we find */
		err = scan_and_connect();
		if (err != 0) {
			continue;
		}

		/* Discover TBS and GTBS on the remove server */
		err = discover_services();
		if (err != 0) {
			continue;
		}

		/* Reset if disconnected */
		err = k_sem_take(&sem_conn_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Failed to take sem_conn_state_change: err %d", err);
			break;
		}
	}

	return 0;
}
