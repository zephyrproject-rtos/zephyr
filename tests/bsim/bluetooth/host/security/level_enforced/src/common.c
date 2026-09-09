/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/logging/log.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

LOG_MODULE_REGISTER(common, LOG_LEVEL_DBG);

/* Long enough for a security request, a pairing exchange and an encryption
 * change to have travelled the link, so that a refusal that leaked onto the
 * link is observed rather than merely outrun.
 */
#define SETTLE_TIME K_MSEC(500)

/* All of the test's state lives here and is reached only through the calls
 * common.h declares, so neither role can read a flag or a reported level
 * without going through the check that is supposed to interpret it.
 */
DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_security_result);
DEFINE_FLAG_STATIC(flag_pairing_completed);

static struct bt_conn *g_conn;

/* The outcome of the elevation attempt, as reported to the application. */
static bt_security_t g_reported_level;
static enum bt_security_err g_reported_err;

static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(g_conn == NULL || conn == g_conn, "Unexpected new connection.");

	if (g_conn == NULL) {
		g_conn = bt_conn_ref(conn);
	}

	if (err != 0) {
		clear_g_conn();
		return;
	}

	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	UNSET_FLAG(flag_is_connected);
}

/* Both outcomes of an elevation attempt arrive here: a level that was reached,
 * and an error that says it was not. Recording the level alongside the error is
 * the whole point of the test, since a host that reports success at a lower
 * level than the one asked for is the failure being guarded against.
 */
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	LOG_INF("Security changed: level %d, err %d", level, err);

	g_reported_level = level;
	g_reported_err = err;

	SET_FLAG(flag_security_result);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_INF("Pairing complete, bonded %d", bonded);

	SET_FLAG(flag_pairing_completed);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err err)
{
	LOG_INF("Pairing failed, err %d", err);

	g_reported_err = err;

	SET_FLAG(flag_security_result);
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

void test_setup(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	err = bt_conn_auth_info_cb_register(&auth_info_cb);
	TEST_ASSERT(err == 0, "bt_conn_auth_info_cb_register failed (err %d)", err);
}

void wait_connected(void)
{
	WAIT_FOR_FLAG(flag_is_connected);
}

void wait_disconnected(void)
{
	WAIT_FOR_FLAG_UNSET(flag_is_connected);
}

void clear_g_conn(void)
{
	struct bt_conn *conn = bt_conn_take(&g_conn);

	TEST_ASSERT(conn, "Test error: no g_conn");
	bt_conn_unref(conn);
}

void disconnect_and_wait(void)
{
	int err;

	err = bt_conn_disconnect(g_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "bt_conn_disconnect failed (err %d)", err);

	wait_disconnected();
	clear_g_conn();
}

/* The second round has to start from an unpaired link. A stored key would let
 * the host satisfy the request from `smp_keys_check()` and never reach the
 * reachability guard the round is there to exercise.
 */
void forget_bonds(void)
{
	int err;

	err = bt_unpair(BT_ID_DEFAULT, NULL);
	TEST_ASSERT(err == 0, "bt_unpair failed (err %d)", err);
}

void forget_security_result(void)
{
	UNSET_FLAG(flag_security_result);
	UNSET_FLAG(flag_pairing_completed);

	g_reported_level = BT_SECURITY_L1;
	g_reported_err = BT_SECURITY_ERR_SUCCESS;
}

/* An unreachable level must be refused locally: the call fails, nothing is put
 * on the link, and the link stays where it was. Both roles owe this behaviour,
 * and each reaches it through a different SMP path.
 */
void expect_refused_elevation(bt_security_t level)
{
	int err;

	err = bt_conn_set_security(g_conn, level);
	TEST_ASSERT(err < 0, "Elevation to an unreachable L%d was accepted (err %d)", level, err);

	/* Nothing may have happened on the link as a result of the refusal. */
	k_sleep(SETTLE_TIME);
	TEST_ASSERT(!IS_FLAG_SET(flag_security_result),
		    "Refused elevation to L%d still produced a security result", level);
	TEST_ASSERT(!IS_FLAG_SET(flag_pairing_completed),
		    "Refused elevation to L%d still paired", level);
	TEST_ASSERT(bt_conn_get_security(g_conn) == BT_SECURITY_L1,
		    "Link reports level %d after a refused elevation to L%d",
		    bt_conn_get_security(g_conn), level);
}

/* The control for the refusals above: the same link does pair when asked for a
 * level the pairing method can actually deliver. Without it the refusals would
 * also hold on a build that cannot pair at all, which would make the whole test
 * a confident lie.
 */
void expect_granted_elevation(bt_security_t level)
{
	int err;

	err = bt_conn_set_security(g_conn, level);
	TEST_ASSERT(err == 0, "Elevation to a reachable L%d was refused (err %d)", level, err);

	expect_observed_elevation(level);
}

/* The same outcome seen from the peer that did not ask for it. The granted
 * round is only a two-sided fact if both ends observe the level being reached,
 * and this is the half a role cannot get from its own return value.
 */
void expect_observed_elevation(bt_security_t level)
{
	WAIT_FOR_FLAG(flag_security_result);
	TEST_ASSERT(g_reported_err == BT_SECURITY_ERR_SUCCESS, "Elevation to L%d failed (err %d)",
		    level, g_reported_err);
	TEST_ASSERT(g_reported_level >= level,
		    "Link settled at level %d after a successful elevation to L%d",
		    g_reported_level, level);
}

static void stop_scan_and_connect(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
				  struct net_buf_simple *ad)
{
	int err;

	if (g_conn != NULL) {
		return;
	}

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "bt_le_scan_stop failed (err %d)", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &g_conn);
	TEST_ASSERT(err == 0, "bt_conn_le_create failed (err %d)", err);
}

void scan_connect_to_first_result(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, stop_scan_and_connect);
	TEST_ASSERT(err == 0, "bt_le_scan_start failed (err %d)", err);
}

void advertise_connectable(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "bt_le_adv_start failed (err %d)", err);
}
