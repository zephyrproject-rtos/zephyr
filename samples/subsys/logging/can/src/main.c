/*
 * Copyright (c) 2025 Paul Wedeck <paulwedeck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend_can.h>

LOG_MODULE_REGISTER(sample_main, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("These log messages are sent under CAN ID 0x7ff (std CAN ID)");
	LOG_INF("Listen to these messages using candump -a or scripts/logging/can/recv_can_log.c");
	k_sleep(K_MSEC(1000));

	for (int i = 1; i <= 3; i++) {
		LOG_INF("example message %d", i);
		k_sleep(K_MSEC(1000));
	}

	LOG_INF("Switching to CAN ID 0x1234abcd (extended CAN ID)");
	log_backend_can_set_frameopts(0x1234abcd, CAN_FRAME_IDE);
	k_sleep(K_MSEC(3000));

	for (int i = 4; i <= 6; i++) {
		LOG_INF("example message %d", i);
		k_sleep(K_MSEC(1000));
	}

	LOG_INF("Switching to CAN FD with baud rate switching (same CAN ID)");
	log_backend_can_set_frameopts(0x1234abcd, CAN_FRAME_IDE | CAN_FRAME_FDF | CAN_FRAME_BRS);
	k_sleep(K_MSEC(3000));

	for (int i = 7; i <= 9; i++) {
		LOG_INF("example message %d", i);
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
