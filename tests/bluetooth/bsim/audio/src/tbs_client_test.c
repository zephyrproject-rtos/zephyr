/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_TBS_CLIENT

#include <zephyr/bluetooth/audio/tbs.h>

#include "common.h"

static struct bt_conn_cb conn_callbacks;
extern enum bst_result_t bst_result;
static volatile bool bt_init;
static volatile bool is_connected;
static volatile bool discovery_complete;
static volatile bool is_gtbs_found;
static volatile bool read_complete;
static volatile bool call_placed;
static volatile uint8_t call_state;
static volatile uint8_t call_index;
static volatile uint8_t tbs_count;

CREATE_FLAG(ccid_read_flag);

static void tbs_client_call_states_cb(struct bt_conn *conn, int err,
				      uint8_t index, uint8_t call_count,
				      const struct bt_tbs_client_call_state *call_states)
{
	if (index != 0) {
		return;
	}

	printk("%s\n", __func__);
	printk("Index %u\n", index);
	if (err != 0) {
		FAIL("Call could not read call states (%d)\n", err);
		return;
	}

	call_index = call_states[0].index;
	call_state = call_states[0].state;
	printk("call index %u - state %u\n", call_index, call_state);
}

static void tbs_client_read_bearer_provider_name(struct bt_conn *conn, int err,
						 uint8_t index,
						 const char *value)
{
	if (err != 0) {
		FAIL("Call could not read bearer name (%d)\n", err);
		return;
	}

	printk("Index %u\n", index);
	printk("Bearer name pointer: %p\n", value);
	printk("Bearer name: %s\n", value);
	read_complete = true;
}

static void tbs_client_discover_cb(struct bt_conn *conn, int err,
				   uint8_t count, bool gtbs_found)
{
	printk("%s\n", __func__);
	if (err != 0) {
		FAIL("TBS_CLIENT could not be discovered (%d)\n", err);
		return;
	}

	tbs_count = count;
	is_gtbs_found = true;
	discovery_complete = true;
}

static void tbs_client_read_ccid_cb(struct bt_conn *conn, int err,
				    uint8_t inst_index, uint32_t value)
{
	struct bt_tbs_instance *inst;

	if (value > UINT8_MAX) {
		FAIL("Invalid CCID: %u", value);
		return;
	}

	printk("Read CCID %u on index %u\n", value, inst_index);

	inst = bt_tbs_client_get_by_ccid(conn, (uint8_t)value);
	if (inst == NULL) {
		FAIL("Could not get instance by CCID: %u", value);
		return;
	}

	SET_FLAG(ccid_read_flag);
}

static const struct bt_tbs_client_cb tbs_client_cbs = {
	.discover = tbs_client_discover_cb,
	.originate_call = NULL,
	.terminate_call = NULL,
	.hold_call = NULL,
	.accept_call = NULL,
	.retrieve_call = NULL,
	.bearer_provider_name = tbs_client_read_bearer_provider_name,
	.bearer_uci = NULL,
	.technology = NULL,
	.uri_list = NULL,
	.signal_strength = NULL,
	.signal_interval = NULL,
	.current_calls = NULL,
	.ccid = tbs_client_read_ccid_cb,
	.status_flags = NULL,
	.call_uri = NULL,
	.call_state = tbs_client_call_states_cb,
	.termination_reason = NULL
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		bt_conn_unref(default_conn);
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	is_connected = true;
}

static void bt_ready(int err)
{
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	bt_init = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_ccid(void)
{
	if (is_gtbs_found) {
		int err;

		UNSET_FLAG(ccid_read_flag);
		printk("Reading GTBS CCID\n");

		err = bt_tbs_client_read_ccid(default_conn, BT_TBS_GTBS_INDEX);
		if (err != 0) {
			FAIL("Read GTBS CCID failed (%d)\n", err);
			return;
		}

		WAIT_FOR_FLAG(ccid_read_flag);
	}

	for (uint8_t i = 0; i < tbs_count; i++) {
		int err;

		UNSET_FLAG(ccid_read_flag);
		printk("Reading bearer CCID on index %u\n", i);

		err = bt_tbs_client_read_ccid(default_conn, i);
		if (err != 0) {
			FAIL("Read bearer CCID failed (%d)\n", err);
			return;
		}

		WAIT_FOR_FLAG(ccid_read_flag);
	}
}

static void test_main(void)
{
	int err;
	int index = 0;
	int tbs_client_err;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_tbs_client_register_cb(&tbs_client_cbs);

	WAIT_FOR_COND(bt_init);

	printk("Audio Server: Bluetooth discovered\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_COND(is_connected);

	tbs_client_err = bt_tbs_client_discover(default_conn, true);
	if (tbs_client_err) {
		FAIL("Failed to discover TBS_CLIENT for connection %d", tbs_client_err);
	}

	WAIT_FOR_COND(discovery_complete);

	printk("GTBS %sfound\n", is_gtbs_found ? "" : "not ");

	printk("Placing call\n");
	err = bt_tbs_client_originate_call(default_conn, 0, "tel:123456789012");
	if (err != 0) {
		FAIL("Originate call failed (%d)\n", err);
	}

	/* Call transitions:
	 * 1) Dialing
	 * 2) Alerting
	 * 3) Active
	 * 4) Remotely Held
	 */
	printk("Waiting for remotely held\n");
	WAIT_FOR_COND(call_state == BT_TBS_CALL_STATE_REMOTELY_HELD);

	printk("Holding call\n");
	err = bt_tbs_client_hold_call(default_conn, index, call_index);
	if (err != 0) {
		FAIL("Hold call failed (%d)\n", err);
	}

	/* Call transitions:
	 * 1) Locally and remotely held
	 * 2) Locally held
	 */
	WAIT_FOR_COND(call_state == BT_TBS_CALL_STATE_LOCALLY_HELD);

	printk("Retrieving call\n");
	err = bt_tbs_client_retrieve_call(default_conn, index, call_index);
	if (err != 0) {
		FAIL("Retrieve call failed (%d)\n", err);
	}

	WAIT_FOR_COND(call_state == BT_TBS_CALL_STATE_ACTIVE);

	printk("Reading bearer provider name\n");
	err = bt_tbs_client_read_bearer_provider_name(default_conn, index);
	if (err != 0) {
		FAIL("Read bearer provider name failed (%d)\n", err);
	}

	test_ccid();

	WAIT_FOR_COND(read_complete);
	PASS("TBS_CLIENT Passed\n");
}

static const struct bst_test_instance test_tbs_client[] = {
	{
		.test_id = "tbs_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_tbs_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_tbs_client);
}

#else

struct bst_test_list *test_tbs_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_TBS_CLIENT */
