/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

void central(void)
{
	bs_bt_utils_setup();

	printk("== Bonding id a - global bondable mode ==\n");
	BUILD_ASSERT(CONFIG_BT_BONDABLE, "CONFIG_BT_BONDABLE must be enabled by default.");
	enable_bt_conn_set_bondable(false);
	scan_connect_to_first_result();
	wait_connected();
	set_security(BT_SECURITY_L2);
	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);
	disconnect();
	wait_disconnected();
	unpair(BT_ID_DEFAULT);
	clear_g_conn();

	printk("== Bonding id a - bond per-connection ==\n");
	enable_bt_conn_set_bondable(true);
	set_bondable(true);
	scan_connect_to_first_result();
	wait_connected();
	set_security(BT_SECURITY_L2);
	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);
	disconnect();
	wait_disconnected();
	clear_g_conn();

	printk("== Bonding id b - bond per-connection ==\n");
	scan_connect_to_first_result();
	wait_connected();
	set_security(BT_SECURITY_L2);
	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_not_bonded);
	disconnect();
	wait_disconnected();
	clear_g_conn();

	TEST_PASS("PASS");
}
