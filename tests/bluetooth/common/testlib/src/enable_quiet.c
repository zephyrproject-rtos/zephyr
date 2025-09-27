/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <testlib/log_utils.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log_core.h>

#include <testlib/enable_quiet.h>

int bt_testlib_silent_bt_enable(void)
{
	int err;

	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_ERR);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_ERR);

	err = bt_enable(NULL);

	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_INF);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_INF);

	return err;
}
