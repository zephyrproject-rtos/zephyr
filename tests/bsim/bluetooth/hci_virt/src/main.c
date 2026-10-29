/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "babblekit/testcase.h"

#define TEST_SERVICE_UUID BT_UUID_DECLARE_16(0xfff0)
#define TEST_VALUE_UUID BT_UUID_DECLARE_16(0xfff1)
#define NOTIFICATION_COUNT 5U

static const uint8_t initial_value[] = "Unix socket GATT";
static const uint8_t test_value[] = {
	0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U,
	0x88U, 0x99U, 0xaaU, 0xbbU, 0xccU, 0xddU, 0xeeU, 0xffU,
};
static const bt_addr_le_t peripheral_addr = {
	.type = BT_ADDR_LE_PUBLIC,
	.a.val = {0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U},
};
static const struct bt_data advertising_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xf0, 0xff),
};

static struct bt_conn *connection;
static bool central_role;
static uint8_t value[sizeof(initial_value)];
static uint16_t value_len = sizeof(initial_value);
static uint16_t value_handle;
static uint8_t read_value[sizeof(value)];
static size_t read_len;
static uint32_t notification_count;
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_discover_params ccc_discover_params;
static struct bt_gatt_read_params read_params;
static struct bt_gatt_write_params write_params;
static struct bt_gatt_subscribe_params subscribe_params;

static K_SEM_DEFINE(peer_found, 0, 1);
static K_SEM_DEFINE(connected, 0, 1);
static K_SEM_DEFINE(disconnected, 0, 1);
static K_SEM_DEFINE(discovered, 0, 1);
static K_SEM_DEFINE(read_complete, 0, 1);
static K_SEM_DEFINE(write_complete, 0, 1);
static K_SEM_DEFINE(value_written, 0, 1);
static K_SEM_DEFINE(subscribed, 0, 1);
static K_SEM_DEFINE(notifications_received, 0, 1);

static void wait_for(struct k_sem *sem, const char *operation)
{
	int err = k_sem_take(sem, K_SECONDS(1));

	TEST_ASSERT(err == 0, "Timed out waiting for %s (%d)", operation, err);
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;
	int ret;

	TEST_ASSERT(err == 0U, "Connection failed (0x%02x)", err);
	ret = bt_conn_get_info(conn, &info);
	TEST_ASSERT(ret == 0, "Cannot get connection info (%d)", ret);
	TEST_ASSERT(info.role == (central_role ? BT_CONN_ROLE_CENTRAL : BT_CONN_ROLE_PERIPHERAL),
		    "Unexpected connection role %u", info.role);

	if (!central_role) {
		connection = bt_conn_ref(conn);
	}
	k_sem_give(&connected);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	TEST_ASSERT(conn == connection, "Unexpected disconnected connection");
	TEST_ASSERT(reason == BT_HCI_ERR_REMOTE_USER_TERM_CONN ||
			    reason == BT_HCI_ERR_LOCALHOST_TERM_CONN,
		    "Unexpected disconnect reason 0x%02x", reason);
	k_sem_give(&disconnected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

static ssize_t read_characteristic(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t write_characteristic(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);

	TEST_ASSERT(offset == 0U && flags == 0U, "Unexpected GATT write offset or flags");
	TEST_ASSERT(len == sizeof(test_value), "Unexpected GATT write length %u", len);
	TEST_ASSERT(memcmp(buf, test_value, len) == 0, "GATT write payload mismatch");
	memcpy(value, buf, len);
	value_len = len;
	k_sem_give(&value_written);

	return len;
}

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t ccc_value)
{
	ARG_UNUSED(attr);

	if (ccc_value == BT_GATT_CCC_NOTIFY) {
		k_sem_give(&subscribed);
	}
}

BT_GATT_SERVICE_DEFINE(test_service, BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
		       BT_GATT_CHARACTERISTIC(TEST_VALUE_UUID,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
						      BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					      read_characteristic, write_characteristic, NULL),
		       BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	ARG_UNUSED(rssi);
	ARG_UNUSED(ad);

	if (type == BT_GAP_ADV_TYPE_ADV_IND && bt_addr_le_eq(addr, &peripheral_addr)) {
		k_sem_give(&peer_found);
	}
}

static uint8_t characteristic_found(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	TEST_ASSERT(attr != NULL, "Test characteristic not found");
	chrc = attr->user_data;
	TEST_ASSERT(bt_uuid_cmp(chrc->uuid, TEST_VALUE_UUID) == 0, "Unexpected UUID");
	value_handle = chrc->value_handle;
	k_sem_give(&discovered);

	return BT_GATT_ITER_STOP;
}

static uint8_t on_read(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
		       const void *data, uint16_t len)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	TEST_ASSERT(err == 0U, "GATT read failed (0x%02x)", err);
	if (data == NULL) {
		k_sem_give(&read_complete);
		return BT_GATT_ITER_STOP;
	}

	TEST_ASSERT(read_len + len <= sizeof(read_value), "GATT read exceeds buffer");
	memcpy(read_value + read_len, data, len);
	read_len += len;

	return BT_GATT_ITER_CONTINUE;
}

static void on_write(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	TEST_ASSERT(err == 0U, "GATT write failed (0x%02x)", err);
	k_sem_give(&write_complete);
}

static uint8_t on_notification(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			       const void *data, uint16_t len)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	if (data == NULL) {
		return BT_GATT_ITER_STOP;
	}

	TEST_ASSERT(len == sizeof(test_value), "Unexpected notification length %u", len);
	TEST_ASSERT(memcmp(data, test_value, len) == 0, "Notification payload mismatch");
	notification_count++;
	if (notification_count == NOTIFICATION_COUNT) {
		k_sem_give(&notifications_received);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void enable_host(bool central)
{
	int err;

	TEST_START("%s Host over Unix socket", central ? "central" : "peripheral");
	central_role = central;
	memcpy(value, initial_value, sizeof(initial_value));

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth initialization failed (%d)", err);
}

static void connect_central(void)
{
	int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);

	TEST_ASSERT(err == 0, "Cannot start scanning (%d)", err);
	wait_for(&peer_found, "peripheral advertisement");
	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Cannot stop scanning (%d)", err);
	err = bt_conn_le_create(&peripheral_addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&connection);
	TEST_ASSERT(err == 0, "Cannot initiate connection (%d)", err);
	wait_for(&connected, "central connection");
}

static void discover_characteristic(void)
{
	int err;

	discover_params.uuid = TEST_VALUE_UUID;
	discover_params.func = characteristic_found;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	err = bt_gatt_discover(connection, &discover_params);
	TEST_ASSERT(err == 0, "Cannot start GATT discovery (%d)", err);
	wait_for(&discovered, "GATT characteristic discovery");
}

static void check_read(const uint8_t *expected, size_t expected_len)
{
	int err;

	read_len = 0U;
	memset(&read_params, 0, sizeof(read_params));
	read_params.func = on_read;
	read_params.handle_count = 1U;
	read_params.single.handle = value_handle;
	err = bt_gatt_read(connection, &read_params);
	TEST_ASSERT(err == 0, "Cannot start GATT read (%d)", err);
	wait_for(&read_complete, "GATT read response");
	TEST_ASSERT(read_len == expected_len, "GATT read length mismatch (%zu)", read_len);
	TEST_ASSERT(memcmp(read_value, expected, read_len) == 0, "GATT read payload mismatch");
}

static void central_test(void)
{
	int err;

	enable_host(true);
	connect_central();

	discover_characteristic();
	check_read(initial_value, sizeof(initial_value));
	write_params.func = on_write;
	write_params.handle = value_handle;
	write_params.data = test_value;
	write_params.length = sizeof(test_value);
	err = bt_gatt_write(connection, &write_params);
	TEST_ASSERT(err == 0, "Cannot start GATT write (%d)", err);
	wait_for(&write_complete, "GATT write response");
	check_read(test_value, sizeof(test_value));

	subscribe_params.notify = on_notification;
	subscribe_params.value_handle = value_handle;
	subscribe_params.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
	subscribe_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	subscribe_params.disc_params = &ccc_discover_params;
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	err = bt_gatt_subscribe(connection, &subscribe_params);
	TEST_ASSERT(err == 0, "Cannot subscribe (%d)", err);
	wait_for(&notifications_received, "GATT notifications");
	TEST_ASSERT(notification_count == NOTIFICATION_COUNT, "Notification count mismatch");

	err = bt_conn_disconnect(connection, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "Cannot disconnect (%d)", err);
	wait_for(&disconnected, "central disconnection");
	bt_conn_unref(connection);
	connection = NULL;
	TEST_PASS("central GATT over Unix socket passed");
}

static void peripheral_test(void)
{
	int err;

	enable_host(false);
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, advertising_data, ARRAY_SIZE(advertising_data),
			      NULL, 0);
	TEST_ASSERT(err == 0, "Cannot start advertising (%d)", err);
	wait_for(&connected, "peripheral connection");

	wait_for(&value_written, "GATT write request");
	wait_for(&subscribed, "CCC subscription");
	for (uint32_t i = 0U; i < NOTIFICATION_COUNT; i++) {
		err = bt_gatt_notify(connection, &test_service.attrs[2], value, value_len);
		TEST_ASSERT(err == 0, "Cannot send notification (%d)", err);
	}

	wait_for(&disconnected, "peripheral disconnection");
	bt_conn_unref(connection);
	connection = NULL;
	TEST_PASS("peripheral GATT over Unix socket passed");
}

static const struct bst_test_instance test_cases[] = {
	{
		.test_id = "central",
		.test_descr = "Central exercises GATT over a Unix HCI socket",
		.test_main_f = central_test,
	},
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral serves GATT over a Unix HCI socket",
		.test_main_f = peripheral_test,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install_tests(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cases);
}

bst_test_install_t test_installers[] = {
	install_tests,
	NULL,
};

int main(void)
{
	bst_main();
	return 0;
}
