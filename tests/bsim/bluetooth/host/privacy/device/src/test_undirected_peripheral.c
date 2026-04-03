/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_bsim_privacy, LOG_LEVEL_INF);

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "bs_cmd_line.h"

DEFINE_FLAG_STATIC(paired_flag);
DEFINE_FLAG_STATIC(connected_flag);
DEFINE_FLAG_STATIC(wait_disconnection);
DEFINE_FLAG_STATIC(wait_scanned);

static struct bt_conn *default_conn;

enum adv_param_t {
	CONN_SCAN,
	CONN_NSCAN,
	NCONN_SCAN,
	NCONN_NSCAN,
};

enum addr_type_t {
	RPA,
	IDENTITY_ADDR,
};

static enum addr_type_t test_addr_type;

static bool use_ext_adv;

static bool scannable_test;
static bool connectable_test;
static enum adv_param_t adv_param;

static int sim_id;

void peripheral_test_args_parse(int argc, char *argv[])
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
			.name = "{identity, rsa}",
			.option = "addr-type",
			.descript = "Address type to test",
		},
		{
			.dest = &use_ext_adv,
			.type = 'b',
			.name = "{0, 1}",
			.option = "use-ext-adv",
			.descript = "Use Extended Advertising",
		},
		{
			.dest = &scannable_test,
			.type = 'b',
			.name = "{0, 1}",
			.option = "scannable",
			.descript = "Use a scannable advertiser for the test",
		},
		{
			.dest = &connectable_test,
			.type = 'b',
			.name = "{0, 1}",
			.option = "connectable",
			.descript = "Use a connectable advertiser for the test",
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

	if (connectable_test && scannable_test) {
		if (!use_ext_adv) {
			adv_param = CONN_SCAN;
		}
	} else if (connectable_test) {
		adv_param = CONN_NSCAN;
	} else if (scannable_test) {
		adv_param = NCONN_SCAN;
	} else {
		adv_param = NCONN_NSCAN;
	}
}

static void wait_for_scanned(void)
{
	LOG_DBG("Waiting for scan request");
	WAIT_FOR_FLAG(wait_scanned);
	UNSET_FLAG(wait_scanned);
}

static void adv_scanned_cb(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_scanned_info *info)
{
	LOG_DBG("Scan request received");
	SET_FLAG(wait_scanned);
}

static struct bt_le_ext_adv_cb adv_cb = {
	.scanned = adv_scanned_cb,
};

static void create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_CONN;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	params.interval_max = BT_GAP_ADV_SLOW_INT_MAX;

	err = bt_le_ext_adv_create(&params, &adv_cb, adv);
	if (err) {
		LOG_ERR("Failed to create advertiser (%d)", err);
		return;
	}

	LOG_DBG("Advertiser created");
}

static void update_adv_params(struct bt_le_ext_adv *adv, enum adv_param_t adv_params,
			      enum addr_type_t addr_type)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	if (adv_params == CONN_SCAN) {
		params.options |= BT_LE_ADV_OPT_CONN;
		params.options |= BT_LE_ADV_OPT_SCANNABLE;
		LOG_DBG("Advertiser params: CONN_SCAN");
	} else if (adv_params == CONN_NSCAN) {
		params.options |= BT_LE_ADV_OPT_CONN;
		LOG_DBG("Advertiser params: CONN_NSCAN");
	} else if (adv_params == NCONN_SCAN) {
		params.options |= BT_LE_ADV_OPT_SCANNABLE;
		LOG_DBG("Advertiser params: NCONN_SCAN");
	} else if (adv_params == NCONN_NSCAN) {
		LOG_DBG("Advertiser params: NCONN_NSCAN");
	}

	if (use_ext_adv) {
		params.options |= BT_LE_ADV_OPT_EXT_ADV;
		LOG_DBG("Advertiser params: EXT_ADV");
		params.options |= BT_LE_ADV_OPT_NOTIFY_SCAN_REQ;
		LOG_DBG("Advertiser params: NOTIFY_SCAN_REQ");
	} else {
		LOG_DBG("Advertiser params: LEGACY_ADV");
	}

	if (addr_type == IDENTITY_ADDR) {
		LOG_DBG("Advertiser params: USE_IDENTITY");
		params.options |= BT_LE_ADV_OPT_USE_IDENTITY;
	} else if (addr_type == RPA) {
		LOG_DBG("Advertiser params: USE_RPA");
	}

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	params.interval_max = BT_GAP_ADV_SLOW_INT_MAX;

	err = bt_le_ext_adv_update_param(adv, &params);
	if (err) {
		LOG_ERR("Failed to update advertiser set (%d)", err);
		return;
	}

	if (use_ext_adv && adv_params == NCONN_SCAN) {
		struct bt_data sd =
			BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, 'z', 'e', 'p', 'h', 'y', 'r');
		size_t sd_len = 1;

		err = bt_le_ext_adv_set_data(adv, NULL, 0, &sd, sd_len);
		if (err) {
			LOG_ERR("Failed to set advertising data (%d)", err);
		}

		LOG_DBG("Advertiser data set");
	}

	LOG_DBG("Advertiser params updated");
}

static void start_adv(struct bt_le_ext_adv *adv)
{
	int err;
	int32_t timeout = 0;
	uint8_t num_events = 0;

	struct bt_le_ext_adv_start_param start_params;

	start_params.timeout = timeout;
	start_params.num_events = num_events;

	err = bt_le_ext_adv_start(adv, &start_params);

	if (err) {
		LOG_ERR("Failed to start advertiser (%d)", err);
		return;
	}

	LOG_DBG("Advertiser started");
}

static void stop_adv(struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		LOG_WRN("Failed to stop advertiser (%d)", err);
		return;
	}

	LOG_DBG("Advertiser stopped");
}

static void disconnect(void)
{
	LOG_DBG("Starting disconnection");
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		TEST_FAIL("Disconnection failed (err %d)", err);
	}

	WAIT_FOR_FLAG(wait_disconnection);
	UNSET_FLAG(wait_disconnection);
}

static void wait_for_connection(void)
{
	WAIT_FOR_FLAG(connected_flag);
	UNSET_FLAG(connected_flag);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	LOG_DBG("Peripheral Connected function");

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_WRN("Failed to connect to %s (%u)", addr, err);
		return;
	}

	LOG_DBG("Connected: %s", addr);

	default_conn = bt_conn_ref(conn);

	if (!IS_FLAG_SET(paired_flag)) {
		if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
			TEST_FAIL("Failed to set security");
		}
	} else {
		SET_FLAG(connected_flag);
	}
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

	LOG_DBG("Disconnected");
	SET_FLAG(wait_disconnection);
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	LOG_DBG("Identity resolved %s -> %s", addr_rpa, addr_identity);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_DBG("Security changed: %s level %u", addr, level);
	} else {
		LOG_ERR("Security failed: %s level %u err %d", addr, level);
	}
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_DBG("Pairing complete");
	SET_FLAG(paired_flag);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	LOG_WRN("Pairing failed (%d). Disconnecting.", reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static struct bt_conn_cb peri_cb;

static struct bt_conn_auth_info_cb auth_cb_info = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static void test_peripheral_main(void)
{
	LOG_DBG("Peripheral device");

	int err;
	struct bt_le_ext_adv *adv = NULL;

	peri_cb.connected = connected;
	peri_cb.disconnected = disconnected;
	peri_cb.security_changed = security_changed;
	peri_cb.identity_resolved = identity_resolved;

	bt_conn_cb_register(&peri_cb);

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);
	}

	LOG_DBG("Bluetooth initialized");

	bt_conn_auth_info_cb_register(&auth_cb_info);

	create_adv(&adv);

	update_adv_params(adv, CONN_NSCAN, RPA);
	start_adv(adv);

	WAIT_FOR_FLAG(paired_flag);

	disconnect();
	stop_adv(adv);

	update_adv_params(adv, adv_param, test_addr_type);
	start_adv(adv);

	/* (the connection with identity should fail with privacy network mode) */
	if (connectable_test) {
		wait_for_connection();
		disconnect();
	} else if (scannable_test && use_ext_adv) {
		wait_for_scanned();
	}

	/* It is up to the controller to decide if it should send an
	 * AUX_SCAN_RSP or not when it gets ordered to stop advertising right
	 * after receiving the AUX_SCAN_REQ.
	 *
	 * Some test cases depend on receiving AUX_SCAN_RSP, so don't stop the
	 * advertiser. This ensures we will always get it.
	 */
}

void test_peripheral(void)
{
	char *addr_tested = "";

	if (test_addr_type == RPA) {
		addr_tested = "RPA";
	} else if (test_addr_type == IDENTITY_ADDR) {
		addr_tested = "identity address";
	}

	LOG_INF("Peripheral test START (id: %d: %s advertiser, "
		"%sconnectable %sscannable, testing %s)\n",
		sim_id, use_ext_adv ? "extended" : "legacy", connectable_test ? "" : "non-",
		scannable_test ? "" : "non-", addr_tested);

	test_peripheral_main();

	TEST_PASS("passed");
}
