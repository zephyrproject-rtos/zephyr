/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_CCP

#include "common.h"
#include "../../../../../subsys/bluetooth/host/audio/ccp.h"
#include "../../../../../subsys/bluetooth/host/audio/tbs.h"

static struct bt_conn_cb conn_callbacks;
extern enum bst_result_t bst_result;
static uint8_t expected_passes = 1;
static uint8_t passes;
static volatile bool bt_init;
static volatile bool is_connected;
static volatile bool discovery_complete;
static volatile bool is_gtbs_found;
static volatile bool read_complete;
static volatile bool call_placed;
static volatile uint8_t call_state;
static volatile uint8_t call_index;
static struct bt_conn *tbs_conn;

static void ccp_call_states_cb(struct bt_conn *conn, int err, uint8_t index,
			       uint8_t call_count,
			       const struct bt_ccp_call_state_t *call_states)
{
	if (index != 0) {
		return;
	}

	printk("%s\n", __func__);
	printk("Index %u\n", index);
	if (err) {
		FAIL("Call could not read call states (%d)\n", err);
		return;
	}

	call_index = call_states[0].index;
	call_state = call_states[0].state;
	printk("call index %u - state %u\n", call_index, call_state);
}

static void ccp_read_bearer_provider_name(struct bt_conn *conn, int err,
					  uint8_t index, const char *value)
{
	if (err) {
		FAIL("Call could not read bearer name (%d)\n", err);
		return;
	}

	printk("Index %u\n", index);
	printk("Bearer name pointer: %p\n", value);
	printk("Bearer name: %s\n", value);
	read_complete = true;
}

static void ccp_discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count,
			    bool gtbs_found)
{
	printk("%s\n", __func__);
	if (err) {
		FAIL("CCP could not be discovered (%d)\n", err);
		return;
	}

	is_gtbs_found = true;
	discovery_complete = true;
}

static struct bt_ccp_cb_t ccp_cbs = {
	.discover = ccp_discover_cb,
	.originate_call = NULL,
	.terminate_call = NULL,
	.hold_call = NULL,
	.accept_call = NULL,
	.retrieve_call = NULL,
	.join_calls = NULL,
	.bearer_provider_name = ccp_read_bearer_provider_name,
	.bearer_uci = NULL,
	.technology = NULL,
	.uri_list = NULL,
	.signal_strength = NULL,
	.signal_interval = NULL,
	.current_calls = NULL,
	.ccid = NULL,
	.status_flags = NULL,
	.call_uri = NULL,
	.call_state = ccp_call_states_cb,
	.termination_reason = NULL
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}
	printk("Connected to %s\n", addr);
	tbs_conn = conn;
	is_connected = true;
}

static void bt_ready(int err)
{
	if (err) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	bt_init = true;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void test_main(void)
{
	int err;
	int index = 0;
	int ccp_err;

	err = bt_enable(bt_ready);

	if (err) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_ccp_register_cb(&ccp_cbs);

	WAIT_FOR(bt_init);

	printk("Audio Server: Bluetooth discovered\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, AD_SIZE, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR(is_connected);

	ccp_err = bt_ccp_discover(tbs_conn, true);
	if (ccp_err) {
		FAIL("Failed to discover CCP for connection %d", ccp_err);
	}

	WAIT_FOR(discovery_complete);

	printk("GTBS %sfound\n", is_gtbs_found ? "" : "not ");

	printk("Placing call\n");
	err = bt_ccp_originate_call(tbs_conn, 0, "tel:123456789012");
	if (err) {
		FAIL("Originate call failed (%d)\n", err);
	}

	/* Call transitions:
	 * 1) Dialing
	 * 2) Alerting
	 * 3) Active
	 * 4) Remotely Held
	 */
	printk("Waiting for remotely held\n");
	WAIT_FOR(call_state == BT_CCP_CALL_STATE_REMOTELY_HELD);

	printk("Holding call\n");
	err = bt_ccp_hold_call(tbs_conn, index, call_index);
	if (err) {
		FAIL("Hold call failed (%d)\n", err);
	}

	/* Call transitions:
	 * 1) Locally and remotely held
	 * 2) Locally held
	 */
	WAIT_FOR(call_state == BT_CCP_CALL_STATE_LOCALLY_HELD);

	printk("Retrieving call\n");
	err = bt_ccp_retrieve_call(tbs_conn, index, call_index);
	if (err) {
		FAIL("Retrieve call failed (%d)\n", err);
	}

	WAIT_FOR(call_state == BT_CCP_CALL_STATE_ACTIVE);

	printk("Reading bearer provider name\n");
	err = bt_ccp_read_bearer_provider_name(tbs_conn, index);
	if (err) {
		FAIL("Read bearer provider name failed (%d)\n", err);
	}

	WAIT_FOR(read_complete);
	PASS("CCP Passed");
}

static const struct bst_test_instance test_ccp[] = {
	{
		.test_id = "ccp",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ccp_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ccp);
}

#else

struct bst_test_list *test_ccp_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CCP */
