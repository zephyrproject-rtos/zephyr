/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_TBS
#include <zephyr/bluetooth/audio/tbs.h>
#include "common.h"

extern enum bst_result_t bst_result;
static uint8_t call_index;
static volatile uint8_t call_state;

CREATE_FLAG(is_connected);
CREATE_FLAG(call_placed);
CREATE_FLAG(call_held);
CREATE_FLAG(call_id);
CREATE_FLAG(call_terminated);
CREATE_FLAG(call_accepted);
CREATE_FLAG(call_retrieved);
CREATE_FLAG(call_joined);

static void tbs_hold_call_cb(struct bt_conn *conn, uint8_t call_index)
{
	if (call_index == call_id) {
		SET_FLAG(call_held);
	}
}

static bool tbs_originate_call_cb(struct bt_conn *conn, uint8_t call_index,
				  const char *caller_id)
{
	printk("Placing call to remote with id %u to %s\n", call_index, caller_id);
	call_id = call_index;
	SET_FLAG(call_placed);
	return true;
}

static bool tbs_authorize_cb(struct bt_conn *conn)
{
	return conn == default_conn;
}

static void tbs_terminate_call_cb(struct bt_conn *conn, uint8_t call_index,
				  uint8_t reason)
{
	printk("Terminating call with id %u reason: %u", call_index, reason);
	SET_FLAG(call_terminated);
	UNSET_FLAG(call_placed);
}

static void tbs_accept_call_cb(struct bt_conn *conn, uint8_t call_index)
{
	printk("Accepting call with index %u\n", call_index);
	SET_FLAG(call_accepted);
}

static void tbs_retrieve_call_cb(struct bt_conn *conn, uint8_t call_index)
{
	printk("Retrieve call with index %u\n", call_index);
	SET_FLAG(call_retrieved);
}

static void tbs_join_calls_cb(struct bt_conn *conn,
			      uint8_t call_index_count,
			      const uint8_t *call_indexes)
{
	for (size_t i = 0; i < sizeof(call_indexes); i++) {
		printk("Call index: %u joined\n", call_indexes[i]);
	}
	SET_FLAG(call_joined);
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
	SET_FLAG(is_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static int test_provider_name(void)
{
	int err;

	printk("%s\n", __func__);
	err = bt_tbs_set_bearer_provider_name(0, "BabblesimTBS");
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not set bearer provider name: %d\n", err);
		return err;
	}

	printk("Set bearer provider name test success\n");

	return err;
}

static int test_set_signal_strength(void)
{
	int err;

	printk("%s\n", __func__);
	err = bt_tbs_set_signal_strength(0, 6);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not set bearer provider name: %d\n", err);
		return err;
	}

	printk("Set signal strength test success\n");

	return err;
}

static int test_set_bearer_technology(void)
{
	int err;

	printk("%s\n", __func__);
	err = bt_tbs_set_bearer_technology(0, BT_TBS_TECHNOLOGY_GSM);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not set bearer technology: %d\n", err);
		return err;
	}

	printk("Set bearer technology test success\n");

	return err;
}

static int test_set_status_flags(void)
{
	int err;

	printk("%s\n", __func__);
	err = bt_tbs_set_status_flags(0, 3);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not set status flags: %d\n", err);
		return err;
	}

	printk("Set status flags test success\n");

	return err;
}

static int test_answer_terminate(void)
{
	int err;

	printk("%s\n", __func__);
	printk("Placing call\n");
	err = bt_tbs_originate(0, "tel:000000000001", &call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not originate call: %d\n", err);
		return err;
	}

	printk("Answering call\n");
	err = bt_tbs_remote_answer(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not accept call: %d\n", err);
		return err;
	}

	printk("Terminating call\n");
	err = bt_tbs_terminate(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not terminate call: %d\n", err);
		return err;
	}

	printk("Test answer & terminate successful\n");

	return err;
}

static int test_hold_retrieve(void)
{
	int err;

	printk("%s\n", __func__);
	err = bt_tbs_originate(0, "tel:000000000001", &call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not originate call: %d\n", err);
		return err;
	}

	err = bt_tbs_remote_answer(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not accept call: %d\n", err);
		return err;
	}

	printk("Holding call\n");
	err = bt_tbs_hold(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not terminate call: %d\n", err);
		return err;
	}

	printk("Retrieving call\n");
	err = bt_tbs_retrieve(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not retrieve call: %d\n", err);
		return err;
	}

	printk("Terminating call\n");
	err = bt_tbs_terminate(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not terminate call: %d\n", err);
		return err;
	}

	printk("Hold & retrieve test sucessfull\n");

	return err;
}

static int test_join(void)
{
	int err;
	uint8_t call_indexes[2];

	printk("%s\n", __func__);
	printk("Placing first call\n");
	err = bt_tbs_originate(0, "tel:000000000001", &call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not originate first call: %d\n", err);
		return err;
	}

	printk("Answering first call\n");
	err = bt_tbs_remote_answer(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not answer first call: %d\n", err);
		return err;
	}
	printk("First call answered\n");

	call_indexes[0] = (uint8_t)call_index;

	printk("Placing second call\n");
	err = bt_tbs_originate(0, "tel:000000000002", &call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not originate second call: %d\n", err);
		return err;
	}

	printk("Answering second call\n");
	err = bt_tbs_remote_answer(call_index);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not answer second call: %d\n", err);
		return err;
	}
	printk("Second call answered\n");

	call_indexes[1] = (uint8_t)call_index;

	printk("Joining calls\n");
	err = bt_tbs_join(2, call_indexes);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not join calls: %d\n", err);
		return err;
	}

	err = bt_tbs_terminate(call_indexes[0]);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not terminate first call: %d\n", err);
		return err;
	}

	err = bt_tbs_terminate(call_indexes[1]);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Could not terminate second call: %d\n", err);
		return err;
	}

	printk("Join calls test succesfull\n");

	return err;
}

static void test_tbs_server_only(void)
{
	test_answer_terminate();
	test_hold_retrieve();
	test_join();
	test_provider_name();
	test_set_signal_strength();
	test_set_bearer_technology();
	test_set_status_flags();
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

	err = bt_tbs_remote_answer(call_id);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Remote could not answer call: %d\n", err);
		return;
	}
	printk("Remote answered %u\n", call_id);

	err = bt_tbs_remote_hold(call_id);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Remote could not hold call: %d\n", err);
	}
	printk("Remote held %u\n", call_id);

	WAIT_FOR_COND(call_held);

	err = bt_tbs_remote_retrieve(call_id);
	if (err != BT_TBS_RESULT_CODE_SUCCESS) {
		FAIL("Remote could not answer call: %d\n", err);
		return;
	}
	printk("Remote retrieved %u\n", call_id);

	PASS("TBS Passed\n");
}

static void tbs_test_server_only(void)
{
	int err;

	err = bt_enable(NULL);

	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	test_tbs_server_only();

	PASS("TBS server tests passed\n");
}

static const struct bst_test_instance test_tbs[] = {
	{
		.test_id = "tbs_test_server_only",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = tbs_test_server_only
	},
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
#else
struct bst_test_list *test_tbs_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_TBS */
