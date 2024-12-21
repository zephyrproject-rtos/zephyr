/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_is_connected);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != g_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(g_conn);

	g_conn = NULL;
	UNSET_FLAG(flag_is_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static uint8_t chrc_data[CHRC_SIZE];
static uint8_t long_chrc_data[LONG_CHRC_SIZE];

static ssize_t read_test_chrc(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 (void *)chrc_data, sizeof(chrc_data));
}

static ssize_t write_test_chrc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       const void *buf, uint16_t len,
			       uint16_t offset, uint8_t flags)
{
	printk("chrc len %u offset %u\n", len, offset);

	if (len > sizeof(chrc_data)) {
		printk("Invalid chrc length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (offset + len > sizeof(chrc_data)) {
		printk("Invalid chrc offset and length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (flags != 0) {
		FAIL("Invalid flags %u\n", flags);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	(void)memcpy(chrc_data + offset, buf, len);

	return len;
}

static ssize_t read_long_test_chrc(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 (void *)long_chrc_data, sizeof(long_chrc_data));
}

static ssize_t write_long_test_chrc(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len,
				    uint16_t offset, uint8_t flags)
{
	static uint8_t prepare_count;

	printk("long_chrc len %u offset %u\n", len, offset);

	if (len > sizeof(long_chrc_data)) {
		printk("Invalid long_chrc length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (offset + len > sizeof(long_chrc_data)) {
		printk("Invalid long_chrc offset and length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		printk("prepare_count %u\n", prepare_count++);
		return BT_GATT_ERR(BT_ATT_ERR_SUCCESS);
	}

	(void)memcpy(long_chrc_data + offset, buf, len);
	prepare_count = 0;

	return len;
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
			       read_test_chrc, write_test_chrc, NULL),
	BT_GATT_CHARACTERISTIC(TEST_LONG_CHRC_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ | BT_GATT_PERM_PREPARE_WRITE,
			       read_long_test_chrc, write_long_test_chrc, NULL),
	BT_GATT_CHARACTERISTIC(TEST_ENC_CHRC_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
			       read_test_chrc, write_test_chrc, NULL),
	BT_GATT_CHARACTERISTIC(TEST_LESC_CHRC_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_LESC | BT_GATT_PERM_WRITE_LESC,
			       read_test_chrc, write_test_chrc, NULL),
);

static void test_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
	};

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	PASS("GATT server passed\n");
}

static const struct bst_test_instance test_gatt_server[] = {
	{
		.test_id = "gatt_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_gatt_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_server);
}
