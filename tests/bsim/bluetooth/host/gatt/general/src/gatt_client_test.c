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
CREATE_FLAG(flag_security_changed);
CREATE_FLAG(flag_write_complete);
CREATE_FLAG(flag_read_complete);

static struct bt_conn *g_conn;
static uint16_t chrc_handle;
static uint16_t long_chrc_handle;
static uint16_t enc_chrc_handle;
static uint16_t lesc_chrc_handle;
static uint8_t att_err;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

#define ARRAY_ITEM(i, _) i
static uint8_t chrc_data[] = { LISTIFY(CHRC_SIZE, ARRAY_ITEM, (,)) }; /* 1, 2, 3 ... */
static uint8_t long_chrc_data[] = { LISTIFY(LONG_CHRC_SIZE, ARRAY_ITEM, (,)) }; /* 1, 2, 3 ... */
static uint8_t data_received[LONG_CHRC_SIZE];
static uint16_t data_received_size;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	__ASSERT_NO_MSG(g_conn == conn);

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

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err != BT_SECURITY_ERR_SUCCESS) {
		FAIL("Security failed (err %d)\n", err);
	} else {
		SET_FLAG(flag_security_changed);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
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
		FAIL("Could not stop scan: %d\n");
		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		FAIL("Could not connect to peer: %d\n", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn,
		const struct bt_gatt_attr *attr,
		struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		if (chrc_handle == 0 || long_chrc_handle == 0) {
			FAIL("Did not discover chrc (%x) or long_chrc (%x)\n", chrc_handle,
			     long_chrc_handle);
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
			chrc_handle = bt_gatt_attr_value_handle(attr);
		} else if (bt_uuid_cmp(chrc->uuid, TEST_LONG_CHRC_UUID) == 0) {
			printk("Found long_chrc\n");
			long_chrc_handle = bt_gatt_attr_value_handle(attr);
		} else if (bt_uuid_cmp(chrc->uuid, TEST_ENC_CHRC_UUID) == 0) {
			printk("Found enc_chrc_handle\n");
			enc_chrc_handle = bt_gatt_attr_value_handle(attr);
		} else if (bt_uuid_cmp(chrc->uuid, TEST_LESC_CHRC_UUID) == 0) {
			printk("Found lesc_chrc_handle\n");
			lesc_chrc_handle = bt_gatt_attr_value_handle(attr);
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

static void update_security(void)
{
	int err;

	printk("Updating security\n");
	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err != 0) {
		FAIL("Set security failed (err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_security_changed);
	printk("Security changed\n");
}

static void gatt_write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	(void)memset(params, 0, sizeof(*params));
	att_err = err;

	SET_FLAG(flag_write_complete);
}

static void gatt_write(uint16_t handle, uint8_t expect_att_err)
{
	static struct bt_gatt_write_params write_params;
	int err;

	if (handle == chrc_handle) {
		printk("Writing to chrc and expecting 0x%02X\n", expect_att_err);
		write_params.data = chrc_data;
		write_params.length = sizeof(chrc_data);
	} else if (handle == long_chrc_handle) {
		printk("Writing to long_chrc and expecting 0x%02X\n", expect_att_err);
		write_params.data = long_chrc_data;
		write_params.length = sizeof(long_chrc_data);
	} else if (handle == enc_chrc_handle) {
		printk("Writing to enc_chrc and expecting 0x%02X\n", expect_att_err);
		write_params.data = chrc_data;
		write_params.length = sizeof(chrc_data);
	} else if (handle == lesc_chrc_handle) {
		printk("Writing to lesc_chrc and expecting 0x%02X\n", expect_att_err);
		write_params.data = chrc_data;
		write_params.length = sizeof(chrc_data);
	}

	write_params.func = gatt_write_cb;
	write_params.handle = handle;

	UNSET_FLAG(flag_write_complete);

	err = bt_gatt_write(g_conn, &write_params);
	if (err != 0) {
		FAIL("bt_gatt_write failed: %d\n", err);
	}

	WAIT_FOR_FLAG(flag_write_complete);

	if (att_err != expect_att_err) {
		FAIL("Write failed: 0x%02X\n", att_err);
	}

	printk("success\n");
}

static uint8_t gatt_read_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_read_params *params,
			    const void *data, uint16_t length)
{
	att_err = err;

	if (err != BT_ATT_ERR_SUCCESS) {
		printk("Read failed: 0x%02X\n", err);

		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_read_complete);

		return BT_GATT_ITER_STOP;
	}

	if (data != NULL) {
		if (data_received_size + length > sizeof(data_received)) {
			FAIL("Invalid amount of data received: %u\n", data_received_size + length);
		} else {
			memcpy(&data_received[data_received_size], data, length);
			data_received_size += length;
		}

		return BT_GATT_ITER_CONTINUE;
	}

	if (params->single.handle == chrc_handle) {
		if (data_received_size != CHRC_SIZE ||
		    memcmp(data_received, chrc_data, data_received_size) != 0) {
			FAIL("chrc data different than expected (%u %u)\n", length, CHRC_SIZE);
		}
	} else if (params->single.handle == long_chrc_handle) {
		if (data_received_size != LONG_CHRC_SIZE ||
		    memcmp(data_received, long_chrc_data, data_received_size) != 0) {
			FAIL("long_chrc data different than expected (%u %u)\n", length,
			     LONG_CHRC_SIZE);
		}
	} else if (params->single.handle == enc_chrc_handle) {
		if (data_received_size != CHRC_SIZE ||
		    memcmp(data_received, chrc_data, data_received_size) != 0) {
			FAIL("enc_chrc data different than expected (%u %u)\n", length, CHRC_SIZE);
		}
	} else if (params->single.handle == lesc_chrc_handle) {
		if (data_received_size != CHRC_SIZE ||
		    memcmp(data_received, chrc_data, data_received_size) != 0) {
			FAIL("lesc_chrc data different than expected (%u %u)\n", length, CHRC_SIZE);
		}
	}

	(void)memset(params, 0, sizeof(*params));
	SET_FLAG(flag_read_complete);

	return BT_GATT_ITER_STOP;
}

static void gatt_read(uint16_t handle, uint8_t expect_att_err)
{
	static struct bt_gatt_read_params read_params;
	int err;

	data_received_size = 0;
	memset(data_received, 0, sizeof(data_received));

	if (handle == chrc_handle) {
		printk("Reading chrc and expecting 0x%02X\n", expect_att_err);
	} else if (handle == long_chrc_handle) {
		printk("Reading long_chrc and expecting 0x%02X\n", expect_att_err);
	} else if (handle == enc_chrc_handle) {
		printk("Reading enc_chrc and expecting 0x%02X\n", expect_att_err);
	} else if (handle == lesc_chrc_handle) {
		printk("Reading lesc_chrc and expecting 0x%02X\n", expect_att_err);
	}

	read_params.func = gatt_read_cb;
	read_params.handle_count = 1;
	read_params.single.handle = handle;
	read_params.single.offset = 0;

	UNSET_FLAG(flag_read_complete);

	err = bt_gatt_read(g_conn, &read_params);
	if (err != 0) {
		FAIL("bt_gatt_read failed: %d\n", err);
	}

	WAIT_FOR_FLAG(flag_read_complete);

	if (att_err != expect_att_err) {
		FAIL("Read failed: 0x%02X\n", att_err);
	}

	printk("success\n");
}

static void test_main(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_FLAG(flag_is_connected);

	gatt_discover();

	/* Write and read a few times to ensure stateless behavior */
	for (size_t i = 0; i < 3; i++) {
		gatt_write(chrc_handle, BT_ATT_ERR_SUCCESS);
		gatt_read(chrc_handle, BT_ATT_ERR_SUCCESS);
		gatt_write(long_chrc_handle, BT_ATT_ERR_SUCCESS);
		gatt_read(long_chrc_handle, BT_ATT_ERR_SUCCESS);
	}

	gatt_write(enc_chrc_handle, BT_ATT_ERR_AUTHENTICATION);
	gatt_read(enc_chrc_handle, BT_ATT_ERR_AUTHENTICATION);
	gatt_write(lesc_chrc_handle, BT_ATT_ERR_AUTHENTICATION);
	gatt_read(lesc_chrc_handle, BT_ATT_ERR_AUTHENTICATION);

	update_security();

	gatt_write(enc_chrc_handle, BT_ATT_ERR_SUCCESS);
	gatt_read(enc_chrc_handle, BT_ATT_ERR_SUCCESS);
	gatt_write(lesc_chrc_handle, BT_ATT_ERR_SUCCESS);
	gatt_read(lesc_chrc_handle, BT_ATT_ERR_SUCCESS);

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
