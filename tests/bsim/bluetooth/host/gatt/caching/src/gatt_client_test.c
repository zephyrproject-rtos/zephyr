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
CREATE_FLAG(flag_chan_1_read);
CREATE_FLAG(flag_chan_2_read);
CREATE_FLAG(flag_db_hash_read);
CREATE_FLAG(flag_encrypted);

static struct bt_conn *g_conn;
static uint16_t chrc_handle;
static uint16_t csf_handle;
static const struct bt_uuid *test_svc_uuid = TEST_SERVICE_UUID;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);

		return;
	}

	printk("Connected to %s\n", addr);

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

void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err != BT_SECURITY_ERR_SUCCESS) {
		FAIL("Encryption failed\n");
	} else if (level < BT_SECURITY_L2) {
		FAIL("Insufficient security\n");
	} else {
		SET_FLAG(flag_encrypted);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
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
		FAIL("Could not stop scan (err %d)\n");

		return;
	}

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	if (err != 0) {
		FAIL("Could not connect to peer (err %d)", err);
	}
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params, int err)
{
	if (attr == NULL) {
		if (chrc_handle == 0) {
			FAIL("Did not discover chrc (%x)\n", chrc_handle);
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
		const struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, TEST_CHRC_UUID) == 0) {
			printk("Found chrc\n");
			chrc_handle = chrc->value_handle;

		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_GATT_CLIENT_FEATURES) == 0) {
			printk("Found csf\n");
			csf_handle = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(const struct bt_uuid *uuid, uint8_t type)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	printk("Discovering services and characteristics\n");

	discover_params.uuid = uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = type;
	discover_params.chan_opt = BT_ATT_CHAN_OPT_NONE;

	UNSET_FLAG(flag_discover_complete);

	err = bt_gatt_discover(g_conn, &discover_params);
	if (err != 0) {
		FAIL("Discover failed(err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_discover_complete);
	printk("Discover complete\n");
}
static struct bt_gatt_read_params chan_1_read = {
	.handle_count = 1,
	.single = {
		.handle = 0, /* Will be set later */
		.offset = 0,
	},
	.chan_opt = BT_ATT_CHAN_OPT_NONE,
};
static struct bt_gatt_read_params chan_2_read = {
	.handle_count = 1,
	.single = {
		.handle = 0, /* Will be set later */
		.offset = 0,
	},
	.chan_opt = BT_ATT_CHAN_OPT_NONE,
};
static struct bt_gatt_read_params db_hash_read = {
	.handle_count = 0,
	.by_uuid = {
		.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
		.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
		.uuid = BT_UUID_GATT_DB_HASH,
	},
	.chan_opt = BT_ATT_CHAN_OPT_NONE,
};

void expect_status(uint8_t err, uint8_t status)
{
	if (err != status) {
		FAIL("Unexpected status from read: 0x%02X, expected 0x%02X\n", err, status);
	}
}

static uint8_t gatt_read_expect_success_cb(struct bt_conn *conn, uint8_t err,
					   struct bt_gatt_read_params *params, const void *data,
					   uint16_t length)
{
	printk("GATT read cb: err 0x%02X\n", err);
	expect_status(err, BT_ATT_ERR_SUCCESS);

	if (params == &db_hash_read) {
		SET_FLAG(flag_db_hash_read);
	} else if (params == &chan_1_read) {
		SET_FLAG(flag_chan_1_read);
	} else if (params == &chan_2_read) {
		SET_FLAG(flag_chan_2_read);
	} else {
		FAIL("Unexpected params\n");
	}

	return 0;
}

static uint8_t gatt_read_expect_err_unlikely_cb(struct bt_conn *conn, uint8_t err,
						struct bt_gatt_read_params *params,
						const void *data, uint16_t length)
{
	printk("GATT read cb: err 0x%02X\n", err);
	expect_status(err, BT_ATT_ERR_UNLIKELY);

	if (params == &chan_1_read) {
		SET_FLAG(flag_chan_1_read);
	} else if (params == &chan_2_read) {
		SET_FLAG(flag_chan_2_read);
	} else {
		FAIL("Unexpected params\n");
	}

	return 0;
}

static uint8_t gatt_read_expect_err_out_of_sync_cb(struct bt_conn *conn, uint8_t err,
						   struct bt_gatt_read_params *params,
						   const void *data, uint16_t length)
{
	printk("GATT read cb: err 0x%02X\n", err);
	expect_status(err, BT_ATT_ERR_DB_OUT_OF_SYNC);

	if (params == &chan_1_read) {
		SET_FLAG(flag_chan_1_read);
	} else if (params == &chan_2_read) {
		SET_FLAG(flag_chan_2_read);
	} else {
		FAIL("Unexpected params\n");
	}

	return 0;
}

static void gatt_read(struct bt_gatt_read_params *read_params)
{
	int err;

	printk("Reading\n");

	err = bt_gatt_read(g_conn, read_params);
	if (err != 0) {
		FAIL("bt_gatt_read failed: %d\n", err);
	}
}

static void write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err != BT_ATT_ERR_SUCCESS) {
		FAIL("Write failed: 0x%02X\n", err);
	}

	SET_FLAG(flag_write_complete);
}

static void enable_robust_caching(void)
{
	/* Client Supported Features Characteristic Value
	 * Bit 0: Robust Caching
	 * Bit 1: EATT
	 */
	static const uint8_t csf[] = { BIT(0) | BIT(1) };
	static struct bt_gatt_write_params write_params = {
		.func = write_cb,
		.offset = 0,
		.data = csf,
		.length = sizeof(csf),
		.chan_opt = BT_ATT_CHAN_OPT_NONE,
	};
	int err;

	printk("Writing to Client Supported Features Characteristic\n");

	write_params.handle = csf_handle;
	UNSET_FLAG(flag_write_complete);

	err = bt_gatt_write(g_conn, &write_params);
	if (err) {
		FAIL("bt_gatt_write failed (err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_write_complete);
	printk("Success\n");
}

static void test_main_common(bool connect_eatt)
{
	int err;

	backchannel_init();

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

	err = bt_conn_set_security(g_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to start encryption procedure\n");
	}

	WAIT_FOR_FLAG(flag_encrypted);

	gatt_discover(test_svc_uuid, BT_GATT_DISCOVER_PRIMARY);
	gatt_discover(BT_UUID_GATT_CLIENT_FEATURES, BT_GATT_DISCOVER_CHARACTERISTIC);

	enable_robust_caching();

	if (connect_eatt) {
		while (bt_eatt_count(g_conn) < 1) {
			/* Wait for EATT channel to connect, in case it hasn't already */
			k_sleep(K_MSEC(10));
		}
	}

	/* Tell the server to register additional service */
	backchannel_sync_send();

	/* Wait for new service to be added by server */
	backchannel_sync_wait();

	chan_1_read.single.handle = chrc_handle;
	chan_2_read.single.handle = chrc_handle;
}

static void test_main_db_hash_read_eatt(void)
{
	test_main_common(true);

	/* Read the DB hash to become change-aware */
	db_hash_read.func = gatt_read_expect_success_cb;
	gatt_read(&db_hash_read);
	WAIT_FOR_FLAG(flag_db_hash_read);

	/* These shall now succeed */
	chan_1_read.func = gatt_read_expect_success_cb;
	chan_2_read.func = gatt_read_expect_success_cb;
	UNSET_FLAG(flag_chan_1_read);
	UNSET_FLAG(flag_chan_2_read);
	gatt_read(&chan_1_read);
	gatt_read(&chan_2_read);
	WAIT_FOR_FLAG(flag_chan_1_read);
	WAIT_FOR_FLAG(flag_chan_2_read);

	/* Signal to server that reads are done */
	backchannel_sync_send();

	PASS("GATT client Passed\n");
}

static void test_main_out_of_sync_eatt(void)
{
	test_main_common(true);

	chan_1_read.func = gatt_read_expect_err_out_of_sync_cb;
	chan_2_read.func = gatt_read_expect_err_out_of_sync_cb;
	gatt_read(&chan_1_read);
	gatt_read(&chan_2_read);

	/* Wait until received response on both reads. When robust caching is implemented
	 * on the client side, the waiting shall be done automatically by the host when
	 * reading the DB hash.
	 */
	WAIT_FOR_FLAG(flag_chan_1_read);
	WAIT_FOR_FLAG(flag_chan_2_read);

	/* Read the DB hash to become change-aware */
	db_hash_read.func = gatt_read_expect_success_cb;
	gatt_read(&db_hash_read);
	WAIT_FOR_FLAG(flag_db_hash_read);

	/* These shall now succeed */
	chan_1_read.func = gatt_read_expect_success_cb;
	chan_2_read.func = gatt_read_expect_success_cb;
	UNSET_FLAG(flag_chan_1_read);
	UNSET_FLAG(flag_chan_2_read);
	gatt_read(&chan_1_read);
	gatt_read(&chan_2_read);
	WAIT_FOR_FLAG(flag_chan_1_read);
	WAIT_FOR_FLAG(flag_chan_2_read);

	/* Signal to server that reads are done */
	backchannel_sync_send();

	PASS("GATT client Passed\n");
}

static void test_main_retry_reads_eatt(void)
{
	test_main_common(true);

	chan_1_read.func = gatt_read_expect_err_out_of_sync_cb;
	chan_2_read.func = gatt_read_expect_err_out_of_sync_cb;
	gatt_read(&chan_1_read);
	gatt_read(&chan_2_read);

	/* Wait until received response on both reads. When robust caching is implemented
	 * on the client side, the waiting shall be done automatically by the host when
	 * reading the DB hash.
	 */
	WAIT_FOR_FLAG(flag_chan_1_read);
	WAIT_FOR_FLAG(flag_chan_2_read);

	/* Retry the reads, these shall time out */
	chan_1_read.func = gatt_read_expect_err_unlikely_cb;
	chan_2_read.func = gatt_read_expect_err_unlikely_cb;
	UNSET_FLAG(flag_chan_1_read);
	UNSET_FLAG(flag_chan_2_read);
	gatt_read(&chan_1_read);
	gatt_read(&chan_2_read);
	WAIT_FOR_FLAG(flag_chan_1_read);
	WAIT_FOR_FLAG(flag_chan_2_read);

	/* Signal to server that reads are done */
	backchannel_sync_send();

	PASS("GATT client Passed\n");
}

static void test_main_db_hash_read_no_eatt(void)
{
	test_main_common(false);

	/* Read the DB hash to become change-aware */
	db_hash_read.func = gatt_read_expect_success_cb;
	gatt_read(&db_hash_read);
	WAIT_FOR_FLAG(flag_db_hash_read);

	/* Read shall now succeed */
	chan_1_read.func = gatt_read_expect_success_cb;
	UNSET_FLAG(flag_chan_1_read);
	gatt_read(&chan_1_read);
	WAIT_FOR_FLAG(flag_chan_1_read);

	/* Signal to server that reads are done */
	backchannel_sync_send();

	PASS("GATT client Passed\n");
}

static void test_main_out_of_sync_no_eatt(void)
{
	test_main_common(false);

	chan_1_read.func = gatt_read_expect_err_out_of_sync_cb;
	gatt_read(&chan_1_read);
	WAIT_FOR_FLAG(flag_chan_1_read);

	/* Read the DB hash to become change-aware */
	db_hash_read.func = gatt_read_expect_success_cb;
	gatt_read(&db_hash_read);
	WAIT_FOR_FLAG(flag_db_hash_read);

	/* Read shall now succeed */
	chan_1_read.func = gatt_read_expect_success_cb;
	UNSET_FLAG(flag_chan_1_read);
	gatt_read(&chan_1_read);
	WAIT_FOR_FLAG(flag_chan_1_read);

	/* Signal to server that reads are done */
	backchannel_sync_send();

	PASS("GATT client Passed\n");
}

static void test_main_retry_reads_no_eatt(void)
{
	test_main_common(false);

	chan_1_read.func = gatt_read_expect_err_out_of_sync_cb;
	gatt_read(&chan_1_read);
	WAIT_FOR_FLAG(flag_chan_1_read);

	/* Read again to become change-aware */
	chan_1_read.func = gatt_read_expect_success_cb;
	UNSET_FLAG(flag_chan_1_read);
	gatt_read(&chan_1_read);
	WAIT_FOR_FLAG(flag_chan_1_read);

	/* Signal to server that reads are done */
	backchannel_sync_send();

	PASS("GATT client Passed\n");
}

static const struct bst_test_instance test_vcs[] = {
	{
		.test_id = "gatt_client_db_hash_read_eatt",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_db_hash_read_eatt,
	},
	{
		.test_id = "gatt_client_out_of_sync_eatt",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_out_of_sync_eatt,
	},
	{
		.test_id = "gatt_client_retry_reads_eatt",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_retry_reads_eatt,
	},
	{
		.test_id = "gatt_client_db_hash_read_no_eatt",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_db_hash_read_no_eatt,
	},
	{
		.test_id = "gatt_client_out_of_sync_no_eatt",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_out_of_sync_no_eatt,
	},
	{
		.test_id = "gatt_client_retry_reads_no_eatt",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main_retry_reads_no_eatt,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_vcs);
}
