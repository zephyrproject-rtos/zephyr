/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/mcc.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/tmap.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "common.h"

#if defined(CONFIG_BT_AICS_CLIENT) && defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT) &&                \
	defined(CONFIG_BT_MCC) && defined(CONFIG_BT_MICP_MIC_CTLR) &&                              \
	defined(CONFIG_BT_TBS_CLIENT_GTBS) && defined(CONFIG_BT_TBS_CLIENT_TBS) &&                 \
	defined(CONFIG_BT_TMAP) && defined(CONFIG_BT_VCP_VOL_CTLR)

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_discovered);

static int discover_err;
static uint8_t aics_count;

static void ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
}

/* The UUID compound literals are constant only at file scope */
static struct bt_gatt_attr empty_mics[] = { BT_GATT_PRIMARY_SERVICE(BT_UUID_MICS) };
static struct bt_gatt_attr empty_vcs[] = { BT_GATT_PRIMARY_SERVICE(BT_UUID_VCS) };
static struct bt_gatt_attr empty_gmcs[] = { BT_GATT_PRIMARY_SERVICE(BT_UUID_GMCS) };
static struct bt_gatt_attr empty_bass[] = { BT_GATT_PRIMARY_SERVICE(BT_UUID_BASS) };
static struct bt_gatt_attr empty_tbs[] = { BT_GATT_PRIMARY_SERVICE(BT_UUID_TBS) };
static struct bt_gatt_attr empty_gtbs[] = { BT_GATT_PRIMARY_SERVICE(BT_UUID_GTBS) };
static struct bt_gatt_attr empty_tmas[] = { BT_GATT_PRIMARY_SERVICE(BT_UUID_TMAS) };

/* The State characteristic goes last, at the last handles, without room for a CCC */
static struct bt_gatt_attr complete_aics[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_AICS),
	BT_GATT_CHARACTERISTIC(BT_UUID_AICS_GAIN_SETTINGS, BT_GATT_CHRC_READ, BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_AICS_INPUT_TYPE, BT_GATT_CHRC_READ, BT_GATT_PERM_NONE, NULL,
			       NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_AICS_INPUT_STATUS, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_AICS_CONTROL, BT_GATT_CHRC_WRITE, BT_GATT_PERM_NONE, NULL,
			       NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_AICS_DESCRIPTION, BT_GATT_CHRC_READ, BT_GATT_PERM_NONE, NULL,
			       NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_AICS_STATE, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL,
			       NULL, NULL),
};
static struct bt_gatt_attr mics_last_include[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MICS),
	BT_GATT_INCLUDE_SERVICE(complete_aics),
};
static struct bt_gatt_attr vcs_last_include[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_VCS),
	BT_GATT_INCLUDE_SERVICE(complete_aics),
};

static void server_common(struct bt_gatt_service *services, size_t count)
{
	struct bt_le_ext_adv *ext_adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	for (size_t i = 0U; i < count; i++) {
		err = bt_gatt_service_register(&services[i]);
		if (err != 0) {
			FAIL("Failed to register service %zu (err %d)\n", i, err);
			return;
		}
	}

	setup_connectable_adv(&ext_adv);

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("GATT invalid services server\n");
}

static void test_empty_services_server(void)
{
	static struct bt_gatt_service services[] = {
		BT_GATT_SERVICE(empty_mics), BT_GATT_SERVICE(empty_vcs),
		BT_GATT_SERVICE(empty_gmcs), BT_GATT_SERVICE(empty_bass),
		BT_GATT_SERVICE(empty_tbs),  BT_GATT_SERVICE(empty_gtbs),
		BT_GATT_SERVICE(empty_tmas),
	};

	server_common(services, ARRAY_SIZE(services));
}

static void test_last_include_server(void)
{
	static struct bt_gatt_service services[] = {
		BT_GATT_SERVICE(mics_last_include),
		BT_GATT_SERVICE(vcs_last_include),
		BT_GATT_SERVICE(complete_aics),
	};

	complete_aics[ARRAY_SIZE(complete_aics) - 2U].handle = BT_ATT_LAST_ATTRIBUTE_HANDLE - 1U;
	complete_aics[ARRAY_SIZE(complete_aics) - 1U].handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	server_common(services, ARRAY_SIZE(services));
}

static void discovered(int err)
{
	discover_err = err;
	SET_FLAG(flag_discovered);
}

static void check_discovered(const char *what, int expected_err)
{
	WAIT_FOR_AND_CLEAR_FLAG(flag_discovered);

	if (discover_err != expected_err) {
		FAIL("%s discovery ended with %d instead of %d\n", what, discover_err,
		     expected_err);
	}
}

static void micp_discover_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err, uint8_t count)
{
	aics_count = count;
	discovered(err);
}

static void vcp_discover_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err, uint8_t vocs_count,
			    uint8_t count)
{
	aics_count = count;
	discovered(err);
}

static void client_connect(void)
{
	static struct bt_micp_mic_ctlr_cb micp_cb = {
		.discover = micp_discover_cb,
	};
	static struct bt_vcp_vol_ctlr_cb vcp_cb = {
		.discover = vcp_discover_cb,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	err = bt_micp_mic_ctlr_cb_register(&micp_cb);
	if (err != 0) {
		FAIL("Failed to register the MICP callbacks (err %d)\n", err);
		return;
	}

	err = bt_vcp_vol_ctlr_cb_register(&vcp_cb);
	if (err != 0) {
		FAIL("Failed to register the VCP callbacks (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);
}

static void client_disconnect(void)
{
	int err;

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		FAIL("Failed to disconnect (err %d)\n", err);
		return;
	}

	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("GATT invalid services client\n");
}

static void discover_micp(int expected_err)
{
	struct bt_micp_mic_ctlr *mic_ctlr;
	int err;

	err = bt_micp_mic_ctlr_discover(default_conn, &mic_ctlr);
	if (err != 0) {
		FAIL("Failed to start the MICP discovery (err %d)\n", err);
		return;
	}

	check_discovered("MICP", expected_err);
}

static void discover_vcp(int expected_err)
{
	struct bt_vcp_vol_ctlr *vol_ctlr;
	int err;

	err = bt_vcp_vol_ctlr_discover(default_conn, &vol_ctlr);
	if (err != 0) {
		FAIL("Failed to start the VCP discovery (err %d)\n", err);
		return;
	}

	check_discovered("VCP", expected_err);
}

static void mcc_discover_cb(struct bt_conn *conn, int err)
{
	discovered(err);
}

static void discover_mcc(void)
{
	static struct bt_mcc_cb cb = {
		.discover_mcs = mcc_discover_cb,
	};
	int err;

	err = bt_mcc_init(&cb);
	if (err != 0) {
		FAIL("Failed to initialize MCC (err %d)\n", err);
		return;
	}

	err = bt_mcc_discover_mcs(default_conn, true);
	if (err != 0) {
		FAIL("Failed to start the MCC discovery (err %d)\n", err);
		return;
	}

	check_discovered("MCC", -ENODATA);
}

static void bass_discover_cb(struct bt_conn *conn, int err, uint8_t count)
{
	discovered(err);
}

static void discover_bass(void)
{
	static struct bt_bap_broadcast_assistant_cb cb = {
		.discover = bass_discover_cb,
	};
	int err;

	err = bt_bap_broadcast_assistant_register_cb(&cb);
	if (err != 0) {
		FAIL("Failed to register the Broadcast Assistant callbacks (err %d)\n", err);
		return;
	}

	err = bt_bap_broadcast_assistant_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to start the Broadcast Assistant discovery (err %d)\n", err);
		return;
	}

	check_discovered("Broadcast Assistant", BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED));
}

static void tbs_discover_cb(struct bt_conn *conn, int err, uint8_t tbs_count, bool gtbs_found)
{
	if (tbs_count != 0U || gtbs_found) {
		FAIL("Found %u TBS instances and GTBS %d\n", tbs_count, gtbs_found);
		return;
	}

	discovered(err);
}

static void discover_tbs(void)
{
	static struct bt_tbs_client_cb cb = {
		.discover = tbs_discover_cb,
	};
	int err;

	err = bt_tbs_client_register_cb(&cb);
	if (err != 0) {
		FAIL("Failed to register the TBS callbacks (err %d)\n", err);
		return;
	}

	err = bt_tbs_client_discover(default_conn);
	if (err != 0) {
		FAIL("Failed to start the TBS discovery (err %d)\n", err);
		return;
	}

	check_discovered("TBS", 0);
}

static void tmap_discover_cb(enum bt_tmap_role role, struct bt_conn *conn, int err)
{
	discovered(err);
}

static void discover_tmap(void)
{
	static const struct bt_tmap_cb cb = {
		.discovery_complete = tmap_discover_cb,
	};
	int err;

	err = bt_tmap_discover(default_conn, &cb);
	if (err != 0) {
		FAIL("Failed to start the TMAP discovery (err %d)\n", err);
		return;
	}

	check_discovered("TMAP", BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
}

static void test_empty_services_client(void)
{
	client_connect();
	update_security(default_conn);

	discover_micp(-ENODATA);
	discover_vcp(-ENODATA);
	discover_mcc();
	discover_bass();
	discover_tbs();
	discover_tmap();

	client_disconnect();
}

static void test_last_include_client(void)
{
	client_connect();

	discover_micp(0);
	if (aics_count != 1U) {
		FAIL("MICP found %u AICS instances\n", aics_count);
		return;
	}

	discover_vcp(0);
	if (aics_count != 1U) {
		FAIL("VCP found %u AICS instances\n", aics_count);
		return;
	}

	client_disconnect();
}

static const struct bst_test_instance test_gatt_invalid_services[] = {
	{
		.test_id = "gatt_empty_services_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_empty_services_server,
	},
	{
		.test_id = "gatt_empty_services_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_empty_services_client,
	},
	{
		.test_id = "gatt_last_include_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_last_include_server,
	},
	{
		.test_id = "gatt_last_include_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_last_include_client,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_invalid_services_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_invalid_services);
}

#else

struct bst_test_list *test_gatt_invalid_services_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* All the clients under test */
