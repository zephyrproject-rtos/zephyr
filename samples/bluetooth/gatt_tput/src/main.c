/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_DECLARE(gatt_tput, LOG_LEVEL_INF);

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (%d)", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_APP_ROLE_PERIPHERAL)) {
		app_peripheral_start();
	} else {
		app_central_start();
	}

	return 0;
}
