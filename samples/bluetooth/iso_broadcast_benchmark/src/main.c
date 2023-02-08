/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>

#include <zephyr/console/console.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>

#include "common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(iso_broadcast_main, LOG_LEVEL_DBG);

enum benchmark_role {
	ROLE_RECEIVER,
	ROLE_BROADCASTER,
	ROLE_QUIT
};

static enum benchmark_role device_role_select(void)
{
	char role;

	while (true) {
		printk("Choose device role - type r (receiver role) or b "
		       "(broadcaster role), or q to quit: ");

		role = tolower(console_getchar());

		printk("%c\n", role);

		if (role == 'r') {
			printk("Receiver role\n");
			return ROLE_RECEIVER;
		} else if (role == 'b') {
			printk("Broadcaster role\n");
			return ROLE_BROADCASTER;
		} else if (role == 'q') {
			printk("Quitting\n");
			return ROLE_QUIT;
		} else if (role == '\n') {
			continue;
		}

		printk("Invalid role: %c\n", role);
	}
}

int main(void)
{
	int err;
	enum benchmark_role role;

	LOG_INF("Starting Bluetooth Throughput example");

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_INF("Bluetooth init failed (err %d)", err);
		return 0;
	}

	err = console_init();
	if (err != 0) {
		LOG_INF("Console init failed (err %d)", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");


	while (true) {
		role = device_role_select();

		if (role == ROLE_RECEIVER) {
			err = test_run_receiver();
		} else if (role == ROLE_BROADCASTER) {
			err = test_run_broadcaster();
		} else {
			if (role != ROLE_QUIT) {
				LOG_INF("Invalid role %u", role);
				continue;
			} else {
				err = 0;
				break;
			}
		}

		LOG_INF("Test complete %d", err);
	}

	LOG_INF("Exiting");
	return 0;
}
