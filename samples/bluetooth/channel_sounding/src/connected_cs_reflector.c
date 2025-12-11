/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include "common.h"

#define CS_CONFIG_ID     0
#define NUM_MODE_0_STEPS 1

static K_SEM_DEFINE(sem_remote_capabilities_obtained, 0, 1);
static K_SEM_DEFINE(sem_config_created, 0, 1);
static K_SEM_DEFINE(sem_cs_security_enabled, 0, 1);
static K_SEM_DEFINE(sem_procedure_done, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_discovered, 0, 1);
static K_SEM_DEFINE(sem_written, 0, 1);

static uint16_t step_data_attr_handle;
static struct bt_conn *connection;
static uint16_t latest_procedure_counter = UINT16_MAX;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];

static const char sample_str[] = "CS Sample";
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "CS Sample", sizeof(sample_str) - 1),
};

static void subevent_result_cb(struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result)
{
	if (result->header.procedure_counter == latest_procedure_counter) {
		printk("The sample does not handle CS procedures with multiple CS subevents.\n");
		latest_procedure_counter = result->header.procedure_counter;
		return;
	}
	latest_procedure_counter = result->header.procedure_counter;

	if (result->step_data_buf) {
		if (result->step_data_buf->len <= STEP_DATA_BUF_LEN) {
			memcpy(latest_local_steps, result->step_data_buf->data,
			       result->step_data_buf->len);
		} else {
			printk("Not enough memory to store step data. (%d > %d)\n",
			       result->step_data_buf->len, STEP_DATA_BUF_LEN);
		}
	}

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE) {
		k_sem_give(&sem_procedure_done);
	}
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
}

static void remote_capabilities_cb(struct bt_conn *conn,
				   uint8_t status,
				   struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(params);

	if (status == BT_HCI_ERR_SUCCESS) {
		printk("CS capability exchange completed.\n");
		k_sem_give(&sem_remote_capabilities_obtained);
	} else {
		printk("CS capability exchange failed. (HCI status 0x%02x)\n", status);
	}
}

static void config_create_cb(struct bt_conn *conn,
			     uint8_t status,
			     struct bt_conn_le_cs_config *config)
{
	if (status == BT_HCI_ERR_SUCCESS) {
		printk("CS config creation complete. ID: %d\n", config->id);
		k_sem_give(&sem_config_created);
	} else {
		printk("CS config creation failed. (HCI status 0x%02x)\n", status);
	}
}

static void security_enable_cb(struct bt_conn *conn, uint8_t status)
{
	if (status == BT_HCI_ERR_SUCCESS) {
		printk("CS security enabled.\n");
		k_sem_give(&sem_cs_security_enabled);
	} else {
		printk("CS security enable failed. (HCI status 0x%02x)\n", status);
	}
}

static void procedure_enable_cb(struct bt_conn *conn,
				uint8_t status,
				struct bt_conn_le_cs_procedure_enable_complete *params)
{
	if (status == BT_HCI_ERR_SUCCESS) {
		if (params->state == 1) {
			printk("CS procedures enabled.\n");
		} else {
			printk("CS procedures disabled.\n");
		}
	} else {
		printk("CS procedures enable failed. (HCI status 0x%02x)\n", status);
	}
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
	.le_cs_read_remote_capabilities_complete = remote_capabilities_cb,
	.le_cs_config_complete = config_create_cb,
	.le_cs_security_enable_complete = security_enable_cb,
	.le_cs_procedure_enable_complete = procedure_enable_cb,
	.le_cs_subevent_data_available = subevent_result_cb,
};

int main(void)
{
	int err;
	struct bt_gatt_discover_params discover_params;
	struct bt_gatt_write_params write_params;

	printk("Starting Channel Sounding Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
					      BT_GAP_ADV_FAST_INT_MAX_1, NULL),
			      ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	k_sem_take(&sem_connected, K_FOREVER);

	const struct bt_le_cs_set_default_settings_param default_settings = {
		.enable_initiator_role = false,
		.enable_reflector_role = true,
		.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
		.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
	};

	err = bt_le_cs_set_default_settings(connection, &default_settings);
	if (err) {
		printk("Failed to configure default CS settings (err %d)\n", err);
	}

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

	while (true) {
		k_sem_take(&sem_procedure_done, K_FOREVER);

		write_params.func = write_func;
		write_params.handle = step_data_attr_handle;
		write_params.length = STEP_DATA_BUF_LEN;
		write_params.data = &latest_local_steps[0];
		write_params.offset = 0;

		err = bt_gatt_write(connection, &write_params);
		if (err) {
			printk("Write failed (err %d)\n", err);
			return 0;
		}

		err = k_sem_take(&sem_written, K_SECONDS(10));
		if (err) {
			printk("Timed out during GATT write\n");
			return 0;
		}
	}

	return 0;
}
