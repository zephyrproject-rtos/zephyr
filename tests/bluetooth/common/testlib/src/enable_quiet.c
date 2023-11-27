/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <testlib/log_utils.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_testlib_enable_quiet, LOG_LEVEL_DBG);

#include <testlib/enable_quiet.h>

int bt_testlib_enable_quiet(void)
{
	int err;

	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_ERR);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_ERR);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt_enable failed (err %d)", err);
	}

	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_INF);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_INF);

	return 0;
}
