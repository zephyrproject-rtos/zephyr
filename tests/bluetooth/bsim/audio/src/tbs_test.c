/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#ifdef CONFIG_BT_TBS
#include <zephyr/bluetooth/audio/tbs.h>
#include "common.h"

extern enum bst_result_t bst_result;
static volatile bool is_connected;
static volatile bool call_placed;
static volatile bool call_held;
static volatile bool call_id;
static volatile bool call_terminated;
static volatile bool call_accepted;
static volatile bool call_retrieved;
static volatile bool test_end;
static volatile uint8_t call_state;
uint8_t call_index;

static volatile bool signal_str;
static volatile bool technology;
static volatile bool status_flags;
static volatile bool provider_name;

static void tbs_hold_call_cb(struct bt_conn *conn, uint8_t call_index)
{
	if (call_index == call_id) {
		call_held = true;
	}
}

static bool tbs_originate_call_cb(struct bt_conn *conn, uint8_t call_index,
				  const char *caller_id)
{
	printk("Placing call to remote with id %u to %s\n",
	       call_index, caller_id);
	call_id = call_index;
	call_placed = true;
	return true;
}

static void tbs_terminate_call_cb(struct bt_conn *conn, uint8_t call_index,
				  uint8_t reason)
{
	printk("Terminating call with id %u reason: %u", call_index, reason);
	call_terminated = true;
	call_id = 0;
	call_placed = false;
}

static void tbs_accept_call_cb(struct bt_conn *conn, uint8_t call_index)
{
	printk("Accepting call with index %u\n", call_index);
	call_accepted = true;
}

static void tbs_retrieve_call_cb(struct bt_conn *conn, uint8_t call_index)
{
	printk("Retrieve call with index %u\n", call_index);
	call_retrieved = true;
}

static void tbs_join_calls_cb(struct bt_conn *conn,
			      uint8_t call_index_count,
			      const uint8_t *call_indexes)
{
	printk("Joined calls with indexes: \n");
}

static bool tbs_authorize_cb(struct bt_conn *conn)
{
	return conn == default_conn;
}

static struct bt_tbs_cb tbs_cbs = {
	.originate_call = tbs_originate_call_cb,
	.terminate_call = tbs_terminate_call_cb,
	.hold_call = tbs_hold_call_cb,
	.accept_call = tbs_accept_call_cb,
	.retrieve_call = tbs_retrieve_call_cb,
	.join_calls = tbs_join_calls_cb,
	.authorize = tbs_authorize_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	default_conn = bt_conn_ref(conn);
	is_connected = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

//static void test_remote_answer(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	printk("%s\n", __func__);
//	err = bt_tbs_remote_answer(call_id);
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Remote could not answer call: %d\n", err);
//		return;
//	}
//	printk("Remote answer test success\n");
//}

//static void test_remote_hold(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	printk("%s\n", __func__);
//	err = bt_tbs_remote_hold(call_id);
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Remote could not hold call: %d\n", err);
//		return;
//	}
//
//	WAIT_FOR_COND(call_held);
//
//	printk("Remote held test success\n");
//}

//static void test_remote_retrieve(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	WAIT_FOR_COND(call_held);
//
//	printk("%s\n", __func__);
//	err = bt_tbs_remote_retrieve(call_id);
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Remote could not answer call: %d\n", err);
//		return;
//	}
//	printk("Remote retrieved test success\n");
//}

//static void test_remote_terminate(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	printk("%s\n", __func__);
//	err = bt_tbs_terminate(call_id);
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Remote could not terminate call: %d\n", err);
//		return;
//	}
//
//	WAIT_FOR_COND(call_terminated);
//	printk("Remote terminated call test success\n");
//}

//static void test_provider_name(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	printk("%s\n", __func__);
//	err = bt_tbs_set_bearer_provider_name(0, "BabblesimTBS");
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Could not set bearer provider name: %d\n", err);
//		return;
//	}
//
//	WAIT_FOR_COND(provider_name);
//	printk("Set bearer provider name test success\n");
//}

//static void test_set_signal_strength(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	printk("%s\n", __func__);
//	err = bt_tbs_set_signal_strength(0, 6);
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Could not set bearer provider name: %d\n", err);
//		return;
//	}
//
//	WAIT_FOR_COND(signal_str);
//	printk("Set signal strength test success\n");
//}

//static void test_set_bearer_technology(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	printk("%s\n", __func__);
//	err = bt_tbs_set_bearer_technology(0, 6);
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Could not set bearer technology: %d\n", err);
//		return;
//	}
//
//	WAIT_FOR_COND(technology);
//	printk("Set bearer technology test success\n");
//}

//static void test_set_status_flags(struct bt_conn *conn, uint8_t call_id)
//{
//	int err;
//
//	printk("%s\n", __func__);
//	err = bt_tbs_set_status_flags(0, 3);
//	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
//		FAIL("Could not set status flags: %d\n", err);
//		return;
//	}
//
//	WAIT_FOR_COND(status_flags);
//	printk("Set status flags test success\n");
//}

void test_accept()
{
	int err;

	printk("%s\n", __func__);
	err = bt_tbs_originate(1, "tel:000000000001", &call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not originate call: %d\n", err);
		return;
	}

	err = bt_tbs_accept(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not accept call: %d\n", err);
		return;
	}

	WAIT_FOR_COND(call_accepted);

	err = bt_tbs_terminate(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not terminate call: %d\n", err);
		return;
	}

	WAIT_FOR_COND(call_terminated);

	printk("Test accept successfull\n");
}

void test_hold_retrieve()
{
	int err;

	printk("%s\n", __func__);
	err = bt_tbs_originate(0, "tel:123456789012", &call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not originate call: %d\n", err);
		return;
	}

	err = bt_tbs_accept(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not accept call: %d\n", err);
		return;
	}

	err = bt_tbs_hold(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not terminate call: %d\n", err);
		return;
	}

	WAIT_FOR_COND(call_held);

	err = bt_tbs_retrieve(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not retrieve call: %d\n", err);
		return;
	}

	WAIT_FOR_COND(call_retrieved);

	printk("Hold & retrieve test sucessfull\n");
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Audio Client: Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);
	bt_tbs_register_cb(&tbs_cbs);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");

	WAIT_FOR_COND(is_connected);
	WAIT_FOR_COND(call_placed);

	//test_remote_answer(default_conn, call_id);
	//test_remote_hold(default_conn, call_id);
	//test_remote_retrieve(default_conn, call_id);
	//test_remote_terminate(default_conn, call_id);

	//test_provider_name(default_conn, call_id);
	//test_set_signal_strength(default_conn, call_id);
	//test_set_bearer_technology(default_conn, call_id);
	//test_set_status_flags(default_conn, call_id);
	test_accept();
	//test_hold_retrieve();
	//test_join();

	PASS("TBS Passed\n");
}

static const struct bst_test_instance test_tbs[] = {
	{
		.test_id = "tbs",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_tbs_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_tbs);
}
//#else
//struct bst_test_list *test_tbs_install(struct bst_test_list *tests)
//{
//	return tests;
//}

//#endif /* CONFIG_BT_TBS */
