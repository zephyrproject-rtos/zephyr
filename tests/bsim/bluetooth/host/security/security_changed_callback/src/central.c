/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "bs_bt_utils.h"
#include "babblekit/testcase.h"
#include "babblekit/flags.h"

LOG_MODULE_REGISTER(test_central, LOG_LEVEL_DBG);

BUILD_ASSERT(CONFIG_BT_BONDABLE, "CONFIG_BT_BONDABLE must be enabled by default.");

void central(void)
{
	LOG_DBG("===== Central =====");

	bs_bt_utils_setup();

	scan_connect_to_first_result();
	wait_connected();
	set_security(BT_SECURITY_L2);

	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);

	LOG_DBG("Wait for disconnection...");
	wait_disconnected();

	clear_g_conn();

	TEST_PASS("PASS");
}
