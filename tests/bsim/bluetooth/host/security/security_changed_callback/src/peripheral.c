/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "bs_bt_utils.h"

LOG_MODULE_REGISTER(test_peripheral, LOG_LEVEL_DBG);

BUILD_ASSERT(CONFIG_BT_BONDABLE, "CONFIG_BT_BONDABLE must be enabled by default.");

static void pairing_complete_unpair(struct bt_conn *conn, bool bonded)
{
	FAIL("Pairing succeed\n");
}

static void peripheral_security_changed_unpair(struct bt_conn *conn,
					bt_security_t level,
					enum bt_security_err err)
{
	/* Try to trigger fault here */
	k_msleep(2000);
	LOG_INF("remove pairing...");
	bt_unpair(BT_ID_DEFAULT, bt_conn_get_dst(conn));
	LOG_DBG("unpaired");
}

void peripheral_unpair_in_sec_cb(void)
{
	LOG_DBG("===== Peripheral (will trigger unpair in sec changed cb) =====");

	int err;
	struct bt_conn_cb peripheral_cb = {};
	struct bt_conn_auth_info_cb peripheral_auth_info_cb = {};

	/* Call `bt_unpair` in security changed callback */

	peripheral_cb.security_changed = peripheral_security_changed_unpair;
	peripheral_auth_info_cb.pairing_complete = pairing_complete_unpair;

	bs_bt_utils_setup();

	bt_conn_cb_register(&peripheral_cb);
	err = bt_conn_auth_info_cb_register(&peripheral_auth_info_cb);
	ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	advertise_connectable(BT_ID_DEFAULT, NULL);
	wait_connected();

	wait_disconnected();

	clear_g_conn();

	PASS("PASS\n");
}

static void pairing_failed_disconnect(struct bt_conn *conn, enum bt_security_err err)
{
	FAIL("Pairing failed\n");
}

static void peripheral_security_changed_disconnect(struct bt_conn *conn,
						   bt_security_t level,
						   enum bt_security_err err)
{
	/* Try to trigger fault here */
	k_msleep(2000);
	LOG_INF("disconnecting...");
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

void peripheral_disconnect_in_sec_cb(void)
{
	LOG_DBG("===== Peripheral (will trigger unpair in sec changed cb) =====");

	int err;
	struct bt_conn_cb peripheral_cb = {};
	struct bt_conn_auth_info_cb peripheral_auth_info_cb = {};

	/* Disconnect in security changed callback */

	peripheral_cb.security_changed = peripheral_security_changed_disconnect;
	peripheral_auth_info_cb.pairing_failed = pairing_failed_disconnect;

	bs_bt_utils_setup();

	bt_conn_cb_register(&peripheral_cb);
	err = bt_conn_auth_info_cb_register(&peripheral_auth_info_cb);
	ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	advertise_connectable(BT_ID_DEFAULT, NULL);
	wait_connected();

	wait_disconnected();

	clear_g_conn();

	PASS("PASS\n");
}
