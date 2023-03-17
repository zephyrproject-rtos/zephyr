/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "argparse.h"
#include "bs_pc_backchannel.h"

BUILD_ASSERT(CONFIG_BT_MAX_PAIRED >= 2, "CONFIG_BT_MAX_PAIRED is too small.");
BUILD_ASSERT(CONFIG_BT_ID_MAX == 2, "CONFIG_BT_ID_MAX should be 2.");

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * USEC_PER_SEC)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS(60)

DEFINE_FLAG(flag_is_connected);
DEFINE_FLAG(flag_test_end);

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

void wait_connected(void)
{
	UNSET_FLAG(flag_is_connected);
	WAIT_FOR_FLAG(flag_is_connected);
	printk("connected\n");
}

void wait_disconnected(void)
{
	SET_FLAG(flag_is_connected);
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
	printk("disconnected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	UNSET_FLAG(flag_is_connected);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		return;
	}

	SET_FLAG(flag_is_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void bs_bt_utils_setup(void)
{
	int err;

	UNSET_FLAG(flag_test_end);

	err = bt_enable(NULL);
	ASSERT(!err, "bt_enable failed.\n");
}

static bt_addr_le_t last_scanned_addr;

static void scan_connect_to_first_result_device_found(const bt_addr_le_t *addr, int8_t rssi,
						      uint8_t type, struct net_buf_simple *ad)
{
	struct bt_conn *conn;
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		FAIL("Unexpected advertisement type.");
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Got scan result, connecting.. dst %s, RSSI %d\n",
	       addr_str, rssi);

	err = bt_le_scan_stop();
	ASSERT(!err, "Err bt_le_scan_stop %d", err);

	err = bt_conn_le_create(addr,
				BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				&conn);
	ASSERT(!err, "Err bt_conn_le_create %d", err);

	/* Save address for later comparison */
	memcpy(&last_scanned_addr, addr, sizeof(last_scanned_addr));
}

void scan_connect_to_first_result(void)
{
	int err;

	printk("start scanner\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE,
			       scan_connect_to_first_result_device_found);
	ASSERT(!err, "Err bt_le_scan_start %d", err);
}

static void scan_expect_same_address_device_found(const bt_addr_le_t *addr, int8_t rssi,
						  uint8_t type, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	char expected_addr_str[BT_ADDR_LE_STR_LEN];

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		FAIL("Unexpected advertisement type.");
	}

	if (!bt_addr_le_eq(&last_scanned_addr, addr)) {
		bt_addr_le_to_str(&last_scanned_addr,
				  expected_addr_str,
				  sizeof(expected_addr_str));
		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

		FAIL("Expected advertiser with addr %s, got %s\n",
		     expected_addr_str, addr_str);
	}

	PASS("Advertiser used correct address on resume\n");
}

void scan_expect_same_address(void)
{
	int err;

	printk("start scanner\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE,
			       scan_expect_same_address_device_found);
	ASSERT(!err, "Err bt_le_scan_start %d", err);
}

static void disconnect_device(struct bt_conn *conn, void *data)
{
	int err;

	/* We only use a single flag to indicate connections. Since this
	 * function will be called multiple times in a row, we have to set it
	 * back after it has been unset (in the `disconnected` callback).
	 */
	SET_FLAG(flag_is_connected);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	ASSERT(!err, "Failed to initate disconnect (err %d)", err);

	printk("Waiting for disconnection...\n");
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
}

void disconnect(void)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_device, NULL);
}

void advertise_connectable(int id, bool persist)
{
	printk("start advertiser\n");
	int err;
	struct bt_le_adv_param param = {};

	param.id = id;
	param.interval_min = 0x0020;
	param.interval_max = 0x4000;
	param.options |= persist ? 0 : BT_LE_ADV_OPT_ONE_TIME;
	param.options |= BT_LE_ADV_OPT_CONNECTABLE;

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	ASSERT(err == 0, "Advertising failed to start (err %d)\n", err);
}

#define CHANNEL_ID 0
#define MSG_SIZE 1

void backchannel_init(uint peer)
{
	uint device_number = get_device_nbr();
	uint device_numbers[] = { peer };
	uint channel_numbers[] = { CHANNEL_ID };
	uint *ch;

	ch = bs_open_back_channel(device_number, device_numbers,
				  channel_numbers, ARRAY_SIZE(channel_numbers));
	if (!ch) {
		FAIL("Unable to open backchannel\n");
	}
}

void backchannel_sync_send(void)
{
	uint8_t sync_msg[MSG_SIZE] = { get_device_nbr() };

	printk("Sending sync\n");
	bs_bc_send_msg(CHANNEL_ID, sync_msg, ARRAY_SIZE(sync_msg));
}

void backchannel_sync_wait(void)
{
	uint8_t sync_msg[MSG_SIZE];

	while (true) {
		if (bs_bc_is_msg_received(CHANNEL_ID) > 0) {
			bs_bc_receive_msg(CHANNEL_ID, sync_msg,
					  ARRAY_SIZE(sync_msg));
			if (sync_msg[0] != get_device_nbr()) {
				/* Received a message from another device, exit */
				break;
			}
		}

		k_sleep(K_MSEC(1));
	}

	printk("Sync received\n");
}
