/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* A server whose GTBS and CSIS each declare a characteristic twice, first with Notify and then
 * without, and a client that discovers them with the TBS client and the CSIP set coordinator.
 * Both declarations arrive in one Read By Type response, so the client handles the second one
 * while the CCC discovery of the first is pending.
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(chrc_redeclared_test, LOG_LEVEL_DBG);

#if defined(CONFIG_BT_TBS_CLIENT_GTBS) && defined(CONFIG_BT_CSIP_SET_COORDINATOR)
extern enum bst_result_t bst_result;

/* Call index 1 in the Incoming state, with no flags */
static const uint8_t call_state[] = {0x01, BT_TBS_CALL_STATE_INCOMING, 0x00};
/* A plain SIRK */
static const uint8_t sirk[] = {0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
			       0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00};
static const uint8_t ccid = 0x01;

CREATE_FLAG(flag_call_state_subscribed);
CREATE_FLAG(flag_sirk_subscribed);
CREATE_FLAG(flag_tbs_discovered);
CREATE_FLAG(flag_call_state_notified);
CREATE_FLAG(flag_csip_discovered);

static ssize_t read_value(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset, const void *value, uint16_t value_len)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t read_call_state(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return read_value(conn, attr, buf, len, offset, call_state, sizeof(call_state));
}

static ssize_t read_ccid(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	return read_value(conn, attr, buf, len, offset, &ccid, sizeof(ccid));
}

static ssize_t read_sirk(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	return read_value(conn, attr, buf, len, offset, sirk, sizeof(sirk));
}

static void call_state_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (value == BT_GATT_CCC_NOTIFY) {
		SET_FLAG(flag_call_state_subscribed);
	}
}

static void sirk_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	if (value == BT_GATT_CCC_NOTIFY) {
		SET_FLAG(flag_sirk_subscribed);
	}
}

static struct bt_gatt_attr gtbs_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GTBS),
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_CALL_STATE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_call_state, NULL, NULL),
	BT_GATT_CCC(call_state_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_TBS_CALL_STATE, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_call_state, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_CCID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_ccid, NULL,
			       NULL),
};

static struct bt_gatt_attr csis_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CSIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_CSIS_SIRK, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_sirk, NULL, NULL),
	BT_GATT_CCC(sirk_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_CSIS_SIRK, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_sirk,
			       NULL, NULL),
};

static struct bt_gatt_service gtbs_svc = BT_GATT_SERVICE(gtbs_attrs);
static struct bt_gatt_service csis_svc = BT_GATT_SERVICE(csis_attrs);

static void test_server(void)
{
	struct bt_le_ext_adv *ext_adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	err = bt_gatt_service_register(&gtbs_svc);
	if (err != 0) {
		FAIL("Failed to register GTBS (err %d)\n", err);
		return;
	}

	err = bt_gatt_service_register(&csis_svc);
	if (err != 0) {
		FAIL("Failed to register CSIS (err %d)\n", err);
		return;
	}

	setup_connectable_adv(&ext_adv);

	WAIT_FOR_FLAG(flag_connected);

	/* The client must have subscribed through the first declaration */
	WAIT_FOR_FLAG(flag_call_state_subscribed);

	err = bt_gatt_notify(default_conn, &gtbs_attrs[1], call_state, sizeof(call_state));
	if (err != 0) {
		FAIL("Failed to notify Call State (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_sirk_subscribed);

	PASS("Server with repeated characteristic declarations passed\n");
}

static void tbs_client_discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count,
				   bool gtbs_found)
{
	if (err != 0 || !gtbs_found) {
		FAIL("TBS discovery failed (err %d, GTBS %sfound)\n", err,
		     gtbs_found ? "" : "not ");
		return;
	}

	SET_FLAG(flag_tbs_discovered);
}

static void tbs_client_call_states_cb(struct bt_conn *conn, int err, uint8_t inst_index,
				      uint8_t call_count,
				      const struct bt_tbs_client_call_state *call_states)
{
	if (err != 0 || call_count != 1U || call_states[0].index != call_state[0]) {
		FAIL("Unexpected call states (err %d, count %u)\n", err, call_count);
		return;
	}

	SET_FLAG(flag_call_state_notified);
}

static struct bt_tbs_client_cb tbs_client_cbs = {
	.discover = tbs_client_discover_cb,
	.call_state = tbs_client_call_states_cb,
};

static void csip_discover_cb(struct bt_conn *conn,
			     const struct bt_csip_set_coordinator_set_member *member, int err,
			     size_t set_count)
{
	if (err != 0 || set_count != 1U) {
		FAIL("CSIP discovery failed (err %d, %zu sets)\n", err, set_count);
		return;
	}

	SET_FLAG(flag_csip_discovered);
}

static struct bt_csip_set_coordinator_cb csip_cbs = {
	.discover = csip_discover_cb,
};

static void test_client(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	err = bt_tbs_client_register_cb(&tbs_client_cbs);
	if (err != 0) {
		FAIL("Failed to register TBS client callbacks (err %d)\n", err);
		return;
	}

	err = bt_csip_set_coordinator_register_cb(&csip_cbs);
	if (err != 0) {
		FAIL("Failed to register CSIP callbacks (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scanning (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	err = bt_tbs_client_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to discover TBS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_tbs_discovered);

	/* Only the subscription of the first declaration is made, and it still works */
	WAIT_FOR_FLAG(flag_call_state_notified);

	err = bt_csip_set_coordinator_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to discover CSIS (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_csip_discovered);

	PASS("Client of repeated characteristic declarations passed\n");
}

static const struct bst_test_instance test_chrc_redeclared[] = {
	{
		.test_id = "chrc_redeclared_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_server,
	},
	{
		.test_id = "chrc_redeclared_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_client,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_chrc_redeclared_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_chrc_redeclared);
}
#else
struct bst_test_list *test_chrc_redeclared_install(struct bst_test_list *tests)
{
	return tests;
}
#endif /* CONFIG_BT_TBS_CLIENT_GTBS && CONFIG_BT_CSIP_SET_COORDINATOR */
