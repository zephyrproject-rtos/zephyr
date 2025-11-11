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
static K_SEM_DEFINE(sem_discovered, 0, 1);
static K_SEM_DEFINE(sem_written, 0, 1);

static uint16_t step_data_attr_handle;
static struct bt_conn *connection;
static uint8_t latest_num_steps_reported;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];

static const char sample_str[] = "CS Test Sample";
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "CS Test Sample", sizeof(sample_str) - 1),
};

static void subevent_result_cb(struct bt_conn_le_cs_subevent_result *result)
{
	latest_num_steps_reported = result->header.num_steps_reported;

	if (result->step_data_buf) {
		if (result->step_data_buf->len <= STEP_DATA_BUF_LEN) {
			memcpy(latest_local_steps, result->step_data_buf->data,
			       result->step_data_buf->len);
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

	connection = bt_conn_ref(conn);

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

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_chrc *chrc;
	char str[BT_UUID_STR_LEN];

	printk("Discovery: attr %p\n", attr);

	if (!attr) {
		return BT_GATT_ITER_STOP;
	}

	chrc = (struct bt_gatt_chrc *)attr->user_data;

	bt_uuid_to_str(chrc->uuid, str, sizeof(str));
	printk("UUID %s\n", str);

	if (!bt_uuid_cmp(chrc->uuid, &step_data_char_uuid.uuid)) {
		step_data_attr_handle = chrc->value_handle;

		printk("Found expected UUID\n");

		k_sem_give(&sem_discovered);
	}

	return BT_GATT_ITER_STOP;
}

static void write_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err) {
		printk("Write failed (err %d)\n", err);

		return;
	}

	k_sem_give(&sem_written);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

int main(void)
{
	int err;
	struct bt_le_cs_test_param test_params;
	struct bt_gatt_discover_params discover_params;
	struct bt_gatt_write_params write_params;

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

	while (true) {
		while (true) {
			k_sleep(K_SECONDS(2));

			test_params = test_params_get(BT_CONN_LE_CS_ROLE_REFLECTOR);

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

		err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
						      BT_GAP_ADV_FAST_INT_MAX_1, NULL),
				      ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return 0;
		}

		k_sem_take(&sem_connected, K_FOREVER);

		discover_params.uuid = &step_data_char_uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(connection, &discover_params);
		if (err) {
			printk("Discovery failed (err %d)\n", err);
			return 0;
		}

		err = k_sem_take(&sem_discovered, K_SECONDS(10));
		if (err) {
			printk("Timed out during GATT discovery\n");
			return 0;
		}

		write_params.func = write_func;
		write_params.handle = step_data_attr_handle;
		write_params.length = STEP_DATA_BUF_LEN;
		write_params.data = latest_local_steps;
		write_params.offset = 0;

		err = bt_gatt_write(connection, &write_params);
		if (err) {
			printk("Write failed (err %d)\n", err);
			return 0;
		}

		k_sem_take(&sem_disconnected, K_FOREVER);

		printk("Re-running CS test...\n");
	}

	return 0;
}
