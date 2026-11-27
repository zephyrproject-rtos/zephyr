/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* A server whose BASS has a characteristic that the Broadcast Assistant does not know, between
 * the Broadcast Audio Scan Control Point and the Broadcast Receive State, as a later revision of
 * the service or a vendor could add, and a Broadcast Assistant that discovers it.
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(bass_unknown_chrc_test, LOG_LEVEL_DBG);

#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
extern enum bst_result_t bst_result;

/* A vendor characteristic, unknown to the Broadcast Assistant */
#define VENDOR_CHRC_UUID                                                                           \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0))

static const uint8_t vendor_value;

CREATE_FLAG(flag_recv_state_subscribed);
CREATE_FLAG(flag_discovered);

static ssize_t read_recv_state(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	/* No source: an empty Broadcast Receive State */
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &vendor_value, 0U);
}

static ssize_t read_vendor(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &vendor_value, sizeof(vendor_value));
}

static ssize_t write_cp(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			uint16_t len, uint16_t offset, uint8_t flags)
{
	return len;
}

static void recv_state_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (value == BT_GATT_CCC_NOTIFY) {
		SET_FLAG(flag_recv_state_subscribed);
	}
}

static struct bt_gatt_attr bass_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BASS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BASS_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE_ENCRYPT, NULL, write_cp, NULL),
	BT_GATT_CHARACTERISTIC(VENDOR_CHRC_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       read_vendor, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_BASS_RECV_STATE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT, read_recv_state, NULL, NULL),
	BT_GATT_CCC(recv_state_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
};

static struct bt_gatt_service bass_svc = BT_GATT_SERVICE(bass_attrs);

static void test_server(void)
{
	struct bt_le_ext_adv *ext_adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	err = bt_gatt_service_register(&bass_svc);
	if (err != 0) {
		FAIL("Failed to register BASS (err %d)\n", err);
		return;
	}

	setup_connectable_adv(&ext_adv);

	WAIT_FOR_FLAG(flag_connected);

	/* The client subscribes to the Broadcast Receive State that follows the unknown
	 * characteristic
	 */
	WAIT_FOR_FLAG(flag_recv_state_subscribed);

	PASS("BASS server with an unknown characteristic passed\n");
}

static void discover_cb(struct bt_conn *conn, int err, uint8_t recv_state_count)
{
	if (err != 0 || recv_state_count != 1U) {
		FAIL("BASS discovery failed (err %d, %u receive states)\n", err, recv_state_count);
		return;
	}

	SET_FLAG(flag_discovered);
}

static struct bt_bap_broadcast_assistant_cb broadcast_assistant_cbs = {
	.discover = discover_cb,
};

static void test_client(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	err = bt_bap_broadcast_assistant_register_cb(&broadcast_assistant_cbs);
	if (err != 0) {
		FAIL("Failed to register Broadcast Assistant callbacks (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scanning (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	update_security(default_conn);

	err = bt_bap_broadcast_assistant_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to discover BASS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovered);

	PASS("Broadcast Assistant with an unknown BASS characteristic passed\n");
}

static const struct bst_test_instance test_bass_unknown_chrc[] = {
	{
		.test_id = "bass_unknown_chrc_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_server,
	},
	{
		.test_id = "bass_unknown_chrc_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_bass_unknown_chrc_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_bass_unknown_chrc);
}
#else
struct bst_test_list *test_bass_unknown_chrc_install(struct bst_test_list *tests)
{
	return tests;
}
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */
