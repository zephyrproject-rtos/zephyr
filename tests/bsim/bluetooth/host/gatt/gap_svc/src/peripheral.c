/** @file
 *  @brief Test local GATT Generic Access Service - peripheral role
 */
/*
 * Copyright (c) 2025 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

#include "babblekit/testcase.h"


LOG_MODULE_REGISTER(peripheral, LOG_LEVEL_DBG);

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE),
	     "This test requires BT_DEVICE_NAME_GATT_WRITABLE to be enabled");
BUILD_ASSERT(IS_ENABLED(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE),
	     "This test requires BT_DEVICE_APPEARANCE_GATT_WRITABLE to be enabled");
BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_GAP_SVC_DEFAULT_IMPL),
	     "This test requires BT_GAP_SVC_DEFAULT_IMPL to be disabled");
BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_PRIVACY),
	     "Simplified GAP implementation - BT_PRIVACY not implemented");
BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_GAP_PERIPHERAL_PREF_PARAMS),
	     "Simplified GAP implementation - BT_GAP_PERIPHERAL_PREF_PARAMS not implemented");

/* Wait time in microseconds for the name and appearance to be changed */
#define WAIT_TIME 10e6
/* Name changed called in the test */
bool gap_svc_name_changed;
/* Appearance changed called in the test */
bool gap_svc_appearance_changed;

/* Advertising data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL)),
};

/* -----------------------------------------------------------------------------
 * Local implementation of GAP service
 */

static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Name read called");

	const char *name = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Name changed called");
	/* adding one to fit the terminating null character */
	char value[CONFIG_BT_DEVICE_NAME_MAX + 1] = {};

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (offset + len > CONFIG_BT_DEVICE_NAME_MAX) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(value, buf, len);

	value[len] = '\0';

	bt_set_name(value);

	LOG_INF("Name changed to %s", value);
	gap_svc_name_changed = true;
	if (gap_svc_appearance_changed) {
		TEST_PASS("GAP service name and appearance changed successfully");
	}

	return len;
}

static ssize_t read_appearance(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(bt_get_appearance());

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance, sizeof(appearance));
}

static ssize_t write_appearance(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	LOG_DBG("Appearance write called");

	uint16_t appearance;
	int err;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(appearance)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	appearance = sys_get_le16(buf);

	err = bt_set_appearance(appearance);

	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	LOG_INF("Appearance changed to 0x%04x", appearance);
	gap_svc_appearance_changed = true;
	if (gap_svc_name_changed) {
		TEST_PASS("GAP service name and appearance changed successfully");
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(BT_GATT_GAP_SVC_DEFAULT_NAME,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GAP),
	/* Require pairing for writes to device name */
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_name, write_name, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_APPEARANCE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_appearance, write_appearance, NULL),
);

/* End of local implementation of GAP service
 * ---------------------------------------------------------------------------
 */

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		TEST_FAIL("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	gap_svc_name_changed = false;
	gap_svc_appearance_changed = false;

	LOG_INF("Connected: %s", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);
}

static int start_advertising(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		TEST_FAIL("Advertising failed to start (err %d)", err);
	}

	return err;
}

static void recycled(void)
{
	start_advertising();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

static void test_local_gap_svc_peripheral_main(void)
{
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);

	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Peripheral Bluetooth initialized");
	err = start_advertising();
	if (err) {
		return;
	}
	LOG_INF("Advertising successfully started");
}

static void test_local_gap_svc_peripheral_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	TEST_START("test_local_gap_svc_peripheral");
}

static void test_local_gap_svc_peripheral_tick(bs_time_t HW_device_time)

{
	/*
	 * If in WAIT_TIME seconds the testcase did not already pass
	 * (and finish) we consider it failed
	 */
	if (bst_result != Passed) {
		TEST_FAIL("test_local_gap_svc_peripheral failed (not passed after %d seconds)",
		     (int)(WAIT_TIME / 1e6));
	}
}

static const struct bst_test_instance test_peripheral[] = {
	{
		.test_id = "peripheral",
		.test_descr = "GAP service local reimplementation - peripheral role.",
		.test_main_f = test_local_gap_svc_peripheral_main,
		.test_pre_init_f = test_local_gap_svc_peripheral_init,
		.test_tick_f = test_local_gap_svc_peripheral_tick,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_local_gap_svc_peripheral_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_peripheral);
	return tests;
}
