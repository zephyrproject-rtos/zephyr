/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_bsim_privacy, LOG_LEVEL_INF);

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "bs_cmd_line.h"

DEFINE_FLAG_STATIC(paired);
DEFINE_FLAG_STATIC(rpa_tested);
DEFINE_FLAG_STATIC(identity_tested);

static void start_scan(void);

static struct bt_conn *default_conn;

static bt_addr_le_t peer_rpa;
static bt_addr_le_t peer_identity;

enum addr_type_t {
	RPA,
	IDENTITY_ADDR,
};
static enum addr_type_t test_addr_type;

static bool use_active_scan;
static bool connection_test;

static int sim_id;

void central_test_args_parse(int argc, char *argv[])
{
	char *addr_type_arg = NULL;

	bs_args_struct_t args_struct[] = {
		{
			.dest = &sim_id,
			.type = 'i',
			.name = "{positive integer}",
			.option = "sim-id",
			.descript = "Simulation ID counter",
		},
		{
			.dest = &addr_type_arg,
			.type = 's',
			.name = "{identity, rpa}",
			.option = "addr-type",
			.descript = "Address type to test",
		},
		{
			.dest = &use_active_scan,
			.type = 'b',
			.name = "{0, 1}",
			.option = "active-scan",
			.descript = "",
		},
		{
			.dest = &connection_test,
			.type = 'b',
			.name = "{0, 1}",
			.option = "connection-test",
			.descript = "",
		},
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);

	if (addr_type_arg != NULL) {
		if (!strcmp(addr_type_arg, "identity")) {
			test_addr_type = IDENTITY_ADDR;
		} else if (!strcmp(addr_type_arg, "rpa")) {
			test_addr_type = RPA;
		}
	}
}

static void wait_check_result(void)
{
	if (test_addr_type == IDENTITY_ADDR) {
		WAIT_FOR_FLAG(identity_tested);
		LOG_INF("Identity address tested");
	} else if (test_addr_type == RPA) {
		WAIT_FOR_FLAG(rpa_tested);
		LOG_INF("Resolvable Private Address tested");
	}
}

static void check_addresses(const bt_addr_le_t *peer_addr)
{
	LOG_DBG("Check addresses");

	bool addr_equal;

	if (test_addr_type == IDENTITY_ADDR) {
		SET_FLAG(identity_tested);
		addr_equal = bt_addr_le_eq(&peer_identity, peer_addr);
		if (!addr_equal) {
			TEST_FAIL(
				"The peer address is not the same as the peer previously paired.");
		}
	} else if (test_addr_type == RPA) {
		SET_FLAG(rpa_tested);
		addr_equal = bt_addr_le_eq(&peer_identity, peer_addr);
		if (!addr_equal) {
			TEST_FAIL("The resolved address is not the same as the peer previously "
				  "paired.");
		}
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		return;
	}

	bt_addr_le_to_str(info->addr, addr_str, sizeof(addr_str));
	LOG_INF("Device found: %s (RSSI %d)", addr_str, info->rssi);

	/* In the case of extended advertising and active scanning, this
	 * callback will be called twice: once for the AUX_ADV_IND and
	 * another time for the AUX_SCAN_RSP.
	 *
	 * We have to be careful not to stop the scanner before we have gotten
	 * the second one, as the peripheral side waits until it gets an
	 * AUX_SCAN_REQ to end the test.
	 *
	 * There is a catch though, since we have to bond, in order to exchange
	 * the address resolving keys, then this check should only apply after
	 * the pairing is done.
	 */
	if (IS_FLAG_SET(paired) &&
	    info->adv_props == (BT_GAP_ADV_PROP_EXT_ADV | BT_GAP_ADV_PROP_SCANNABLE)) {
		LOG_DBG("skipping AUX_ADV_IND report, waiting for AUX_SCAN_REQ "
			"(props: 0x%x)", info->adv_props);
		return;
	}

	if (IS_FLAG_SET(paired)) {
		check_addresses(info->addr);
	}

	if (connection_test || !IS_FLAG_SET(paired)) {
		if (bt_le_scan_stop()) {
			LOG_DBG("Failed to stop scanner");
			return;
		}
		LOG_DBG("Scanner stopped: conn %d paired %d", connection_test, IS_FLAG_SET(paired));

		err = bt_conn_le_create(info->addr,
					BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT,
					&default_conn);
		if (err) {
			LOG_DBG("Create conn to %s failed (%u)", addr_str, err);
			start_scan();
		}
	}
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv,
};

static void start_scan(void)
{
	int err;

	LOG_DBG("Using %s scan", use_active_scan ? "active" : "passive");

	err = bt_le_scan_start(use_active_scan ? BT_LE_SCAN_ACTIVE : BT_LE_SCAN_PASSIVE,
			       NULL);

	if (err) {
		TEST_FAIL("Scanning failed to start (err %d)", err);
	}

	LOG_DBG("Scanning successfully started");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Connected: %s", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	LOG_DBG("Identity resolved %s -> %s", addr_rpa, addr_identity);

	bt_addr_le_copy(&peer_rpa, rpa);
	bt_addr_le_copy(&peer_identity, identity);

	SET_FLAG(paired);
}

static struct bt_conn_cb central_cb;

void test_central(void)
{
	int err;

	LOG_DBG("Central device");

	central_cb.connected = connected;
	central_cb.disconnected = disconnected;
	central_cb.identity_resolved = identity_resolved;

	bt_conn_cb_register(&central_cb);
	bt_le_scan_cb_register(&scan_cb);

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	UNSET_FLAG(identity_tested);
	UNSET_FLAG(rpa_tested);

	start_scan();

	wait_check_result();
}

void test_central_main(void)
{
	char *addr_tested = "";

	if (test_addr_type == RPA) {
		addr_tested = "RPA";
	} else if (test_addr_type == IDENTITY_ADDR) {
		addr_tested = "identity address";
	}

	LOG_INF("Central test START (id: %d: params: %s scan, %sconnectable test, testing %s)\n",
		sim_id, use_active_scan ? "active" : "passive", connection_test ? "" : "non-",
		addr_tested);

	test_central();

	TEST_PASS("passed");
}
