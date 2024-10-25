/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include "distance_estimation.h"
#include "common.h"
#include "cs_test_params.h"

static K_SEM_DEFINE(sem_results_available, 0, 1);
static K_SEM_DEFINE(sem_test_complete, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);
static K_SEM_DEFINE(sem_data_received, 0, 1);

static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static struct bt_conn *connection;
static uint8_t n_ap;
static uint8_t latest_num_steps_reported;
static uint16_t latest_step_data_len;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];
static uint8_t latest_peer_steps[STEP_DATA_BUF_LEN];

static struct bt_gatt_attr gatt_attributes[] = {
	BT_GATT_PRIMARY_SERVICE(&step_data_svc_uuid),
	BT_GATT_CHARACTERISTIC(&step_data_char_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ, NULL, on_attr_write_cb,
			       NULL),
};
static struct bt_gatt_service step_data_gatt_service = BT_GATT_SERVICE(gatt_attributes);
static const char sample_str[] = "CS Test Sample";

static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(latest_local_steps)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (flags & BT_GATT_WRITE_FLAG_EXECUTE) {
		uint8_t *data = (uint8_t *)buf;

		memcpy(latest_peer_steps, &data[offset], len);
		k_sem_give(&sem_data_received);
	}

	return len;
}

static void subevent_result_cb(struct bt_conn_le_cs_subevent_result *result)
{
	latest_num_steps_reported = result->header.num_steps_reported;
	n_ap = result->header.num_antenna_paths;

	if (result->step_data_buf) {
		if (result->step_data_buf->len <= STEP_DATA_BUF_LEN) {
			memcpy(latest_local_steps, result->step_data_buf->data,
			       result->step_data_buf->len);
			latest_step_data_len = result->step_data_buf->len;
		} else {
			printk("Not enough memory to store step data. (%d > %d)\n",
			       result->step_data_buf->len, STEP_DATA_BUF_LEN);
			latest_num_steps_reported = 0;
		}
	}

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE ||
	    result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_ABORTED) {
		k_sem_give(&sem_results_available);
	}
}

static void end_cb(void)
{
	k_sem_give(&sem_test_complete);
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	printk("MTU exchange %s (%u)\n", err == 0U ? "success" : "failed", bt_gatt_get_mtu(conn));
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected to %s (err 0x%02X)\n", addr, err);

	__ASSERT(connection == conn, "Unexpected connected callback");

	if (err) {
		bt_conn_unref(conn);
		connection = NULL;
	}

	static struct bt_gatt_exchange_params mtu_exchange_params = {.func = mtu_exchange_cb};

	err = bt_gatt_exchange_mtu(connection, &mtu_exchange_params);
	if (err) {
		printk("%s: MTU exchange failed (err %d)\n", __func__, err);
	}

	k_sem_give(&sem_connected);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02X)\n", reason);

	bt_conn_unref(conn);
	connection = NULL;

	k_sem_give(&sem_disconnected);
}

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN] = {};
	int err;

	if (connection) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_data_parse(ad, data_cb, name);

	if (strcmp(name, sample_str)) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	printk("Found device with name %s, connecting...\n", name);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&connection);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
	}
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

int main(void)
{
	int err;
	struct bt_le_cs_test_param test_params;

	printk("Starting Channel Sounding Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	struct bt_le_cs_test_cb cs_test_cb = {
		.le_cs_test_subevent_data_available = subevent_result_cb,
		.le_cs_test_end_complete = end_cb,
	};

	err = bt_le_cs_test_cb_register(cs_test_cb);
	if (err) {
		printk("Failed to register callbacks (err %d)\n", err);
		return 0;
	}

	err = bt_gatt_service_register(&step_data_gatt_service);
	if (err) {
		printk("bt_gatt_service_register() returned err %d\n", err);
		return 0;
	}

	while (true) {
		while (true) {
			k_sleep(K_SECONDS(2));

			test_params = test_params_get(BT_CONN_LE_CS_ROLE_INITIATOR);

			err = bt_le_cs_start_test(&test_params);
			if (err) {
				printk("Failed to start CS test (err %d)\n", err);
				return 0;
			}

			k_sem_take(&sem_results_available, K_SECONDS(5));

			err = bt_le_cs_stop_test();
			if (err) {
				printk("Failed to stop CS test (err %d)\n", err);
				return 0;
			}

			k_sem_take(&sem_test_complete, K_FOREVER);

			if (latest_num_steps_reported > NUM_MODE_0_STEPS) {
				break;
			}
		}

		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, device_found);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return 0;
		}

		k_sem_take(&sem_connected, K_FOREVER);

		k_sem_take(&sem_data_received, K_FOREVER);

		estimate_distance(
			latest_local_steps, latest_step_data_len, latest_peer_steps,
			latest_step_data_len -
				NUM_MODE_0_STEPS *
					(sizeof(struct bt_hci_le_cs_step_data_mode_0_initiator) -
					 sizeof(struct bt_hci_le_cs_step_data_mode_0_reflector)),
			n_ap, BT_CONN_LE_CS_ROLE_INITIATOR);

		bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

		k_sem_take(&sem_disconnected, K_FOREVER);

		printk("Re-running CS test...\n");
	}

	return 0;
}
