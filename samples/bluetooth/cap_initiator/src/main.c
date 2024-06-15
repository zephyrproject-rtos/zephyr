/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <stddef.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/util_macro.h>

#include "cap_initiator.h"

LOG_MODULE_REGISTER(cap_initiator, LOG_LEVEL_INF);

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth enable failed: %d", err);

		return 0;
	}

	LOG_INF("Bluetooth initialized");

	/* TODO: Add CAP initiator broadcast support*/

	/* If the CONFIG_BT_CAP_INITIATOR_UNICAST is enabled we call the cap_initiator_unicast
	 * function that runs the application as a CAP Initiator for unicast. This will attempt to
	 * scan for and connect to a CAP acceptor to set up a stream
	 *
	 * Since cap_initiator_unicast runs as a while (true) loop, this shall be done as the last
	 * thing in this function
	 */
	if (IS_ENABLED(CONFIG_BT_CAP_INITIATOR_UNICAST)) {
		err = cap_initiator_unicast();
	}

	if (err != 0) {
		LOG_ERR("Failed to run CAP Initiator: %d", err);
	}

	return 0;
}
