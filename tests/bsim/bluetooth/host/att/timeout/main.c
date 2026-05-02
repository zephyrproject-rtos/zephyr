/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <argparse.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "host/att_internal.h"

#include "testlib/adv.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "bs_macro.h"
#include "bs_sync.h"
#include <testlib/conn.h>
#include "testlib/log_utils.h"
#include "testlib/scan.h"
#include "testlib/security.h"

/* This test uses system asserts to fail tests. */
BUILD_ASSERT(__ASSERT_ON);

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define CENTRAL_DEVICE_NBR    0
#define PERIPHERAL_DEVICE_NBR 1

#define UUID_1                                                                                     \
	BT_UUID_DECLARE_128(0xdb, 0x1f, 0xe2, 0x52, 0xf3, 0xc6, 0x43, 0x66, 0xb3, 0x92, 0x5d,      \
			    0xc6, 0xe7, 0xc9, 0x59, 0x9d)

#define UUID_2                                                                                     \
	BT_UUID_DECLARE_128(0x3f, 0xa4, 0x7f, 0x44, 0x2e, 0x2a, 0x43, 0x05, 0xab, 0x38, 0x07,      \
			    0x8d, 0x16, 0xbf, 0x99, 0xf1)

static bool trigger_att_timeout;
static K_SEM_DEFINE(disconnected_sem, 0, 1);

static ssize_t read_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t buf_len, uint16_t offset)
{
	ssize_t read_len;

	LOG_INF("ATT timeout will %sbe triggered", trigger_att_timeout ? "" : "not ");

	if (trigger_att_timeout) {
		/* Sleep longer than ATT Timeout (section 3.3.3). */
		k_sleep(K_SECONDS(BT_ATT_TIMEOUT_SEC + 1));
	}

	__ASSERT_NO_MSG(offset == 0);
	read_len = buf_len;

	__ASSERT_NO_MSG(read_len >= 2);
	sys_put_le16(read_len, buf);

	return read_len;
}

static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(UUID_1),
	BT_GATT_CHARACTERISTIC(UUID_2, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_chrc, NULL, NULL),
};

static struct bt_gatt_service svc = {
	.attrs = attrs,
	.attr_count = ARRAY_SIZE(attrs),
};

static void bs_sync_all_log(char *log_msg)
{
	/* Everyone meets here. */
	bt_testlib_bs_sync_all();

	if (get_device_nbr() == 0) {
		LOG_WRN("Sync point: %s", log_msg);
	}

	/* Everyone waits for d0 to finish logging. */
	bt_testlib_bs_sync_all();
}

static inline void bt_enable_quiet(void)
{
	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_ERR);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_ERR);

	EXPECT_ZERO(bt_enable(NULL));

	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_INF);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_INF);
}

static struct bt_conn *peripheral_setup(enum bt_att_chan_opt bearer, bool timeout)
{
	struct bt_conn *conn = NULL;

	EXPECT_ZERO(bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, bt_get_name()));

	trigger_att_timeout = timeout;

	return conn;
}

static struct bt_conn *central_setup(enum bt_att_chan_opt bearer, bool timeout)
{
	bt_addr_le_t adva;
	struct bt_conn *conn = NULL;

	EXPECT_ZERO(bt_testlib_scan_find_name(&adva, "peripheral"));
	EXPECT_ZERO(bt_testlib_connect(&adva, &conn));

	/* Establish EATT bearers. */
	EXPECT_ZERO(bt_testlib_secure(conn, BT_SECURITY_L2));

	while (bt_eatt_count(conn) == 0) {
		k_msleep(100);
	};

	return conn;
}

static void central_read(struct bt_conn *conn, enum bt_att_chan_opt bearer, bool timeout)
{
	uint16_t actual_read_len;
	uint16_t remote_read_send_len;
	uint16_t handle = 0;
	int err;

	NET_BUF_SIMPLE_DEFINE(attr_value, sizeof(remote_read_send_len));

	err = bt_testlib_att_read_by_type_sync(&attr_value, &actual_read_len, &handle, NULL, conn,
					       bearer, UUID_2, BT_ATT_FIRST_ATTRIBUTE_HANDLE,
					       BT_ATT_LAST_ATTRIBUTE_HANDLE);

	if (timeout) {
		__ASSERT(err == BT_ATT_ERR_UNLIKELY, "Unexpected error %d", err);
	} else {
		__ASSERT(!err, "Unexpected error %d", err);
		__ASSERT(attr_value.len >= sizeof(remote_read_send_len),
			 "Remote sent too little data.");
		remote_read_send_len = net_buf_simple_pull_le16(&attr_value);
		__ASSERT(remote_read_send_len == actual_read_len, "Length mismatch. %u %u",
			 remote_read_send_len, actual_read_len);
	}
}

/**
 * Test procedure:
 *
 * Central:
 * 1. Connect to the peripheral.
 * 2. Try to read a characteristic value.
 * 3. Expect BT_ATT_ERR_UNLIKELY error.
 * 4. Expect the peripheral to disconnect.
 * 5. Reconnect to the peripheral.
 * 6. Try to read a characteristic value.
 * 7. Expect the peripheral to respond with the characteristic value.
 * 8. Ensure that connection stays alive after a delay equal to ATT timeout.
 * 9. Disconnect from the peripheral.
 *
 * Peripheral:
 * 1. Start advertising.
 * 2. Make the read callback sleep for more than ATT Timeout when the central tries to read.
 * 3. Expect the disconnected callback to be called.
 * 4. Start advertising again.
 * 5. Make the read callback respond with the characteristic value when the central tries to read.
 * 6. Expect the connection stay alive after a delay equal to ATT timeout.
 * 7. Expect the central to disconnect.
 */
static void test_timeout(enum bt_att_chan_opt bearer)
{
	bool central = (get_device_nbr() == CENTRAL_DEVICE_NBR);
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);
	struct bt_conn *conn;
	int err;

	/* Test ATT timeout. */
	if (peripheral) {
		conn = peripheral_setup(bearer, true);
	}

	if (central) {
		conn = central_setup(bearer, true);
	}

	bs_sync_all_log("Ready to test ATT timeout");

	if (central) {
		central_read(conn, bearer, true);
	}

	err = k_sem_take(&disconnected_sem, K_SECONDS(BT_ATT_TIMEOUT_SEC + 2));
	/* Here disconnect is triggered by the Central host due to ATT timeout. */
	__ASSERT(!err, "Unexpected error %d", err);
	bt_testlib_conn_unref(&conn);

	/* Test successful read. */
	if (peripheral) {
		conn = peripheral_setup(bearer, false);
	}

	if (central) {
		conn = central_setup(bearer, false);
	}

	bs_sync_all_log("Ready to test successful read");

	if (central) {
		central_read(conn, bearer, false);
	}

	err = k_sem_take(&disconnected_sem, K_SECONDS(BT_ATT_TIMEOUT_SEC + 2));
	/* Check that disconnect doesn't happen during time > ATT timeout. */
	__ASSERT(err == -EAGAIN, "Unexpected error %d", err);

	if (central) {
		/* This time disconnect from the peripheral. */
		EXPECT_ZERO(bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN));
	}

	if (peripheral) {
		/* Wait for the central to disconnect. */
		bt_testlib_wait_disconnected(conn);
		bt_testlib_conn_unref(&conn);
	}

	/* Clear the semaphore. */
	err = k_sem_take(&disconnected_sem, K_SECONDS(1));
	__ASSERT_NO_MSG(!err);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	LOG_INF("Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	bool central = (get_device_nbr() == CENTRAL_DEVICE_NBR);
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);
	uint8_t expected_reason;

	LOG_INF("Disconnected: %u", reason);

	if (central) {
		expected_reason = BT_HCI_ERR_LOCALHOST_TERM_CONN;
	}

	if (peripheral) {
		expected_reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	}

	__ASSERT(expected_reason == reason, "Unexpected reason %u", reason);

	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void the_test(void)
{
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);

	if (peripheral) {
		EXPECT_ZERO(bt_gatt_service_register(&svc));
	}

	bt_enable_quiet();

	if (peripheral) {
		EXPECT_ZERO(bt_set_name("peripheral"));
	}

	bs_sync_all_log("Testing UATT");
	test_timeout(BT_ATT_CHAN_OPT_UNENHANCED_ONLY);

	bs_sync_all_log("Testing EATT");
	test_timeout(BT_ATT_CHAN_OPT_ENHANCED_ONLY);

	bs_sync_all_log("Test Complete");

	PASS("Test complete\n");
}
