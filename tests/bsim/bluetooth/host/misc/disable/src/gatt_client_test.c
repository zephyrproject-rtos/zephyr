/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#include "common.h"

CREATE_FLAG(flag_is_connected);
CREATE_FLAG(flag_discover_complete);
CREATE_FLAG(flag_write_complete);
CREATE_FLAG(flag_read_complete);

static struct bt_conn *g_conn;
static uint16_t chrc_handle;
static uint16_t long_chrc_handle;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

#define NUM_ITERATIONS 10

#define ARRAY_ITEM(i, _) i
static uint8_t chrc_data[] = { LISTIFY(CHRC_SIZE, ARRAY_ITEM, (,)) }; /* 1, 2, 3 ... */
static uint8_t long_chrc_data[] = { LISTIFY(LONG_CHRC_SIZE, ARRAY_ITEM, (,)) }; /* 1, 2, 3 ... */

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	g_conn = conn;
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

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
		  struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (g_conn != NULL) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	printk("Stopping scan\n");
	err = bt_le_scan_stop();
	if (err != 0) {
		FAIL("Could not stop scan: %d");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		FAIL("Could not connect to peer: %d", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn,
		const struct bt_gatt_attr *attr,
		struct bt_gatt_discover_params *params,
		int err)
{
	if (attr == NULL) {
		if (chrc_handle == 0 || long_chrc_handle == 0) {
			FAIL("Did not discover chrc (%x) or long_chrc (%x)",
			     chrc_handle, long_chrc_handle);
		}

		(void)memset(params, 0, sizeof(*params));

		SET_FLAG(flag_discover_complete);

		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (params->type == BT_GATT_DISCOVER_PRIMARY &&
	    bt_uuid_cmp(params->uuid, TEST_SERVICE_UUID) == 0) {
		printk("Found test service\n");
		params->uuid = NULL;
		params->start_handle = attr->handle + 1;
		params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, params);
		if (err != 0) {
			FAIL("Discover failed (err %d)\n", err);
		}

		return BT_GATT_ITER_STOP;
	} else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, TEST_CHRC_UUID) == 0) {
			printk("Found chrc\n");
			chrc_handle = chrc->value_handle;
		} else if (bt_uuid_cmp(chrc->uuid, TEST_LONG_CHRC_UUID) == 0) {
			printk("Found long_chrc\n");
			long_chrc_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(void)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	printk("Discovering services and characteristics\n");

	discover_params.uuid = test_svc_uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		FAIL("Discover failed(err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_discover_complete);
	printk("Discover complete\n");
}

static void gatt_write_cb(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_write_params *params)
{
	if (err != BT_ATT_ERR_SUCCESS) {
		FAIL("Write failed: 0x%02X\n", err);
	}

	(void)memset(params, 0, sizeof(*params));

	SET_FLAG(flag_write_complete);
}

static void gatt_write(uint16_t handle)
{
	static struct bt_gatt_write_params write_params;
	int err;

	if (handle == chrc_handle) {
		printk("Writing to chrc\n");
		write_params.data = chrc_data;
		write_params.length = sizeof(chrc_data);
	} else if (handle) {
		printk("Writing to long_chrc\n");
		write_params.data = long_chrc_data;
		write_params.length = sizeof(long_chrc_data);
	}

	write_params.func = gatt_write_cb;
	write_params.handle = handle;

	UNSET_FLAG(flag_write_complete);

	err = bt_gatt_write(g_conn, &write_params);
	if (err != 0) {
		FAIL("bt_gatt_write failed: %d\n", err);
	}

	WAIT_FOR_FLAG(flag_write_complete);
	printk("success\n");
}

static uint8_t gatt_read_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_read_params *params,
			    const void *data, uint16_t length)
{
	if (err != BT_ATT_ERR_SUCCESS) {
		FAIL("Read failed: 0x%02X\n", err);
	}

	if (params->single.handle == chrc_handle) {
		if (length != CHRC_SIZE ||
		    memcmp(data, chrc_data, length) != 0) {
			FAIL("chrc data different than expected", err);
		}
	} else if (params->single.handle == chrc_handle) {
		if (length != LONG_CHRC_SIZE ||
		    memcmp(data, long_chrc_data, length) != 0) {
			FAIL("long_chrc data different than expected", err);
		}
	}

	(void)memset(params, 0, sizeof(*params));

	SET_FLAG(flag_read_complete);

	return 0;
}

static void gatt_read(uint16_t handle)
{
	static struct bt_gatt_read_params read_params;
	int err;

	printk("Reading chrc\n");

	read_params.func = gatt_read_cb;
	read_params.handle_count = 1;
	read_params.single.handle = chrc_handle;
	read_params.single.offset = 0;

	UNSET_FLAG(flag_read_complete);

	err = bt_gatt_read(g_conn, &read_params);
	if (err != 0) {
		FAIL("bt_gatt_read failed: %d\n", err);
	}

	WAIT_FOR_FLAG(flag_read_complete);
	printk("success\n");
}

static void test_main(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	for (int i = 0; i < NUM_ITERATIONS; i++) {

		err = bt_enable(NULL);
		if (err != 0) {
			FAIL("Bluetooth discover failed (err %d)\n", err);
		}
		printk("Bluetooth initialized\n");

		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
		if (err != 0) {
			FAIL("Scanning failed to start (err %d)\n", err);
		}

		printk("Scanning successfully started\n");

		WAIT_FOR_FLAG(flag_is_connected);

		gatt_discover();

		/* Write and read a few times to ensure stateless behavior */
		for (size_t i = 0; i < 3; i++) {
			gatt_write(chrc_handle);
			gatt_read(chrc_handle);
			gatt_write(long_chrc_handle);
			gatt_read(long_chrc_handle);
		}

		err = bt_conn_disconnect(g_conn, 0x13);
		if (err != 0) {
			FAIL("Disconnect failed (err %d)\n", err);
			return;
		}

		WAIT_FOR_FLAG_UNSET(flag_is_connected);

		err = bt_disable();
		if (err != 0) {
			FAIL("Bluetooth disable failed (err %d)\n", err);
		}
		printk("Bluetooth successfully disabled\n");
	}

	PASS("GATT client Passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "gatt_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_gatt_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
