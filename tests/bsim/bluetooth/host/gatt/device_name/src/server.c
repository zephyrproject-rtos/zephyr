/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdbool.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gap/device_name.h>

#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

#include <zephyr/sys/util.h>

#include "testlib/conn.h"
#include "testlib/scan.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

LOG_MODULE_REGISTER(server, LOG_LEVEL_DBG);

DEFINE_FLAG_STATIC(security_changed_flag);

static struct bt_conn_cb server_conn_cb;

static inline bool memeq(const void *m1, size_t len1, const void *m2, size_t len2)
{
	int ret;

	if (len1 != len2) {
		return false;
	}

	ret = memcmp(m1, m2, len1);

	return ret == 0;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	TEST_ASSERT(err == 0, "Security update failed: %s level %u err %d", addr_str, level, err);

	LOG_DBG("Security changed: %s level %u", addr_str, level);
	SET_FLAG(security_changed_flag);
}

static void init_server_conn_callbacks(void)
{
	int err;

	server_conn_cb.connected = NULL;
	server_conn_cb.disconnected = NULL;
	server_conn_cb.security_changed = security_changed;
	server_conn_cb.identity_resolved = NULL;

	err = bt_conn_cb_register(&server_conn_cb);
	TEST_ASSERT(err == 0, "Failed to set server conn callbacks (err %d)", err);
}

static void connect_and_set_security(struct bt_conn **conn)
{
	int err;
	bt_addr_le_t client = {};

	err = bt_testlib_scan_find_name(&client, ADVERTISER_NAME);
	TEST_ASSERT(err == 0, "Failed to start scan (err %d)", err);

	err = bt_testlib_connect(&client, conn);
	TEST_ASSERT(err == 0, "Failed to initiate connection (err %d)", err);

	err = bt_conn_set_security(*conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "Failed to set security (err %d)", err);

	WAIT_FOR_FLAG(security_changed_flag);
}

void server_procedure(void)
{
	/* Test purpose:
	 *
	 * Verifies that writing to the GAP Device Name characteristic correctly
	 * update the device name.
	 *
	 * Two devices:
	 * - `server`: GATT server, connect and elevate security
	 * - `client`: GATT client, when connected will look for the GAP Device
	 *   Name characteristic handle and then will send a GATT write with a
	 *   new name
	 *
	 * [verdict]
	 * - the server device name has been updated by the client
	 */

	int err;
	struct bt_conn *conn = NULL;

	bool names_are_matching;

	char *name = "Server Super Name";

	uint8_t expected_name[CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC_MAX];

	/* add one for the null character */
	uint8_t original_name[CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC_MAX];
	size_t original_name_size;
	uint8_t new_name[CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC_MAX];
	size_t new_name_size;

	generate_name(expected_name, CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC_MAX);

	TEST_START("server");

	bk_sync_init();

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Cannot enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	err = bt_gap_set_device_name(name, sizeof(name));
	TEST_ASSERT(err == 0, "Failed to set the name (err %d)", err);

	original_name_size = bt_gap_get_device_name(original_name, sizeof(original_name));
        TEST_ASSERT(original_name_size >= 0,
                    "Failed to get device name (err %d)", original_name_size);

	init_server_conn_callbacks();

	connect_and_set_security(&conn);

	/* wait for client to do gatt write */
	bk_sync_wait();

	new_name_size = bt_gap_get_device_name(new_name, sizeof(new_name));
        TEST_ASSERT(new_name_size >= 0, "Failed to get device name (err %d)",
                    new_name_size);

        LOG_DBG("Original Device Name: %.*s", original_name_size, original_name);
	LOG_DBG("New Device Name: %.*s", new_name_size, new_name);

	names_are_matching =
		memeq(expected_name, sizeof(expected_name), new_name, strlen(new_name));
	TEST_ASSERT(names_are_matching,
		    "The name of the server doesn't match the one set by the client (server name: "
		    "`%.*s`, expected name: `%.*s`)",
		    new_name_size, new_name, sizeof(expected_name), expected_name);

	TEST_PASS("server");
}
