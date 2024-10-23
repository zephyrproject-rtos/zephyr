/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/printk.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_TBS_CLIENT
static struct bt_conn_cb conn_callbacks;
extern enum bst_result_t bst_result;

static volatile uint8_t call_state;
static volatile uint8_t call_index;
static volatile uint8_t tbs_count;

CREATE_FLAG(bt_init);
CREATE_FLAG(is_connected);
CREATE_FLAG(discovery_complete);
CREATE_FLAG(is_gtbs_found);
CREATE_FLAG(read_complete);
CREATE_FLAG(call_placed);
CREATE_FLAG(call_terminated);
CREATE_FLAG(provider_name);
CREATE_FLAG(ccid_read_flag);
CREATE_FLAG(signal_strength);
CREATE_FLAG(technology);
CREATE_FLAG(status_flags);
CREATE_FLAG(signal_interval);
CREATE_FLAG(call_accepted);
CREATE_FLAG(bearer_uci);
CREATE_FLAG(uri_list);
CREATE_FLAG(current_calls);
CREATE_FLAG(uri_inc);
CREATE_FLAG(term_reason);

static void tbs_client_call_states_cb(struct bt_conn *conn, int err,
				      uint8_t index, uint8_t call_count,
				      const struct bt_tbs_client_call_state *call_states)
{
	if (index != 0) {
		return;
	}

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
	SET_FLAG(provider_name);
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

static void tbs_client_originate_call_cb(struct bt_conn *conn, int err,
					 uint8_t inst_index,
					 uint8_t call_index)
{
	printk("%s %u:\n", __func__, call_index);
	call_placed = true;
}

static void tbs_client_hold_call_cb(struct bt_conn *conn, int err,
				    uint8_t inst_index,
				    uint8_t call_index)
{
	if (err != 0) {
		FAIL("Client hold call error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Call index: %u\n", __func__, inst_index,
						  call_index);
}

static void tbs_client_retrieve_call_cb(struct bt_conn *conn, int err,
				    uint8_t inst_index,
				    uint8_t call_index)
{
	if (err != 0) {
		FAIL("Client retrieve call error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Call index: %u\n", __func__, inst_index,
						  call_index);
}

static void tbs_client_technology_cb(struct bt_conn *conn, int err,
				    uint8_t inst_index,
				    uint32_t value)
{
	if (err != 0) {
		FAIL("Client bearer technology error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Technology: %u\n", __func__, inst_index, value);

	SET_FLAG(technology);
}

static void tbs_client_signal_strength_cb(struct bt_conn *conn, int err,
					  uint8_t inst_index,
					  uint32_t value)
{
	if (err != 0) {
		FAIL("Client signal strength error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u, Strength: %u\n", __func__, inst_index, value);

	SET_FLAG(signal_strength);
}

static void tbs_client_signal_interval_cb(struct bt_conn *conn, int err,
					  uint8_t inst_index,
					  uint32_t value)
{
	if (err != 0) {
		FAIL("Client signal interval error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Interval: %u\n", __func__, inst_index, value);

	SET_FLAG(signal_interval);
}

static void tbs_client_status_flags_cb(struct bt_conn *conn, int err,
				       uint8_t inst_index,
				       uint32_t value)
{
	if (err != 0) {
		FAIL("Status flags error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Flags: %u\n", __func__, inst_index, value);

	SET_FLAG(status_flags);
}

static void tbs_client_terminate_call_cb(struct bt_conn *conn, int err,
					 uint8_t inst_index, uint8_t call_index)
{
	if (err != 0) {
		FAIL("Terminate call error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Call index: %u\n", __func__, inst_index,
						  call_index);

	SET_FLAG(call_terminated);
}

static void tbs_client_accept_call_cb(struct bt_conn *conn, int err,
				      uint8_t inst_index, uint8_t call_index)
{
	if (err != 0) {
		FAIL("Accept call error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Call index: %u\n", __func__, inst_index,
						  call_index);

	SET_FLAG(call_accepted);
}

static void tbs_client_bearer_uci_cb(struct bt_conn *conn, int err,
				     uint8_t inst_index,
				     const char *value)
{
	if (err != 0) {
		FAIL("Bearer UCI error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u UCI: %s\n", __func__, inst_index, value);

	SET_FLAG(bearer_uci);
}

static void tbs_client_uri_list_cb(struct bt_conn *conn, int err,
				    uint8_t inst_index, const char *value)
{
	if (err != 0) {
		FAIL("URI list error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u URI list: %s\n", __func__, inst_index, value);

	SET_FLAG(uri_list);
}

static void tbs_client_current_calls_cb(struct bt_conn *conn, int err,
					uint8_t inst_index,
					uint8_t call_count,
					const struct bt_tbs_client_call *calls)
{
	if (err != 0) {
		FAIL("Current calls error: (%d)\n", err);
		return;
	}

	printk("%s Instance: %u Call count: %u\n", __func__, inst_index,
						  call_count);

	SET_FLAG(current_calls);
}

static void tbs_client_call_uri_cb(struct bt_conn *conn, int err,
				   uint8_t inst_index,
				   const char *value)
{
	if (err != 0) {
		FAIL("Incoming URI error: (%d)\n", err);
		return;
	}

	printk("Incoming URI callback\n");
	printk("%s Instance: %u URI: %s\n", __func__, inst_index, value);

	SET_FLAG(uri_inc);
}

static void tbs_client_term_reason_cb(struct bt_conn *conn,
				      int err, uint8_t inst_index,
				      uint8_t call_index,
				      uint8_t reason)
{
	printk("%s Instance: %u Reason: %u\n", __func__, inst_index, reason);

	SET_FLAG(term_reason);
}

static struct bt_tbs_client_cb tbs_client_cbs = {
	.discover = tbs_client_discover_cb,
	.originate_call = tbs_client_originate_call_cb,
	.terminate_call = tbs_client_terminate_call_cb,
	.hold_call = tbs_client_hold_call_cb,
	.accept_call = tbs_client_accept_call_cb,
	.retrieve_call = tbs_client_retrieve_call_cb,
	.bearer_provider_name = tbs_client_read_bearer_provider_name,
	.bearer_uci = tbs_client_bearer_uci_cb,
	.technology = tbs_client_technology_cb,
	.uri_list = tbs_client_uri_list_cb,
	.signal_strength = tbs_client_signal_strength_cb,
	.signal_interval = tbs_client_signal_interval_cb,
	.current_calls = tbs_client_current_calls_cb,
	.ccid = tbs_client_read_ccid_cb,
	.status_flags = tbs_client_status_flags_cb,
	.call_uri = tbs_client_call_uri_cb,
	.call_state = tbs_client_call_states_cb,
	.termination_reason = tbs_client_term_reason_cb
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

static void test_signal_strength(uint8_t index)
{
	int err;

	UNSET_FLAG(signal_strength);

	printk("%s\n", __func__);

	err = bt_tbs_client_read_signal_strength(default_conn, index);
	if (err != 0) {
		FAIL("Read signal strength failed (%d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(signal_strength);

	printk("Client read signal strength test success\n");
}

static void test_technology(uint8_t index)
{
	int err;

	UNSET_FLAG(technology);

	printk("%s\n", __func__);

	err = bt_tbs_client_read_technology(default_conn, index);
	if (err != 0) {
		FAIL("Read technology failed (%d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(technology);

	printk("Client read technology test success\n");
}

static void test_status_flags(uint8_t index)
{
	int err;

	UNSET_FLAG(status_flags);

	printk("%s\n", __func__);

	err = bt_tbs_client_read_status_flags(default_conn, index);
	if (err != 0) {
		FAIL("Read status flags failed (%d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(status_flags);

	printk("Client read status flags test success\n");
}

static void test_signal_interval(uint8_t index)
{
	int err;

	UNSET_FLAG(signal_interval);

	printk("%s\n", __func__);

	err = bt_tbs_client_read_signal_interval(default_conn, index);
	if (err != 0) {
		FAIL("Read signal interval failed (%d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(signal_interval);

	printk("Client signal interval test success\n");
}

static void discover_tbs(void)
{
	int err;

	discovery_complete = false;

	err = bt_tbs_client_discover(default_conn);
	if (err) {
		FAIL("Failed to discover TBS: %d", err);
		return;
	}

	WAIT_FOR_COND(discovery_complete);
}

static void test_main(void)
{
	int err;
	int index = 0;

	err = bt_enable(bt_ready);

	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	err = bt_tbs_client_register_cb(&tbs_client_cbs);
	if (err != 0) {
		FAIL("Failed to register TBS client cbs (err %d)\n", err);
		return;
	}

	WAIT_FOR_COND(bt_init);

	printk("Audio Server: Bluetooth discovered\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, AD_SIZE, NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_COND(is_connected);

	discover_tbs();
	discover_tbs(); /* test that we can discover twice */

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
	UNSET_FLAG(provider_name);
	err = bt_tbs_client_read_bearer_provider_name(default_conn, index);
	if (err != 0) {
		FAIL("Read bearer provider name failed (%d)\n", err);
	}

	test_ccid();
	WAIT_FOR_COND(read_complete);

	test_signal_strength(index);
	test_technology(index);
	test_status_flags(index);
	test_signal_interval(index);

	PASS("TBS_CLIENT Passed\n");
}

static const struct bst_test_instance test_tbs_client[] = {
	{
		.test_id = "tbs_client",
		.test_pre_init_f = test_init,
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
