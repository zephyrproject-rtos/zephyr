/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME common

LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

void test_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test failed (not passed after %i seconds)\n", WAIT_TIME);
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(WAIT_TIME);
	bst_result = In_progress;
}

/* Call in init functions*/
void device_sync_init(uint device_nbr)
{
	uint peer;

	if (device_nbr == CENTRAL_ID) {
		peer = PERIPHERAL_ID;
	} else {
		peer = CENTRAL_ID;
	}

	uint dev_nbrs[BACK_CHANNELS] = { peer };
	uint channel_nbrs[BACK_CHANNELS] = { 0 };
	const uint *ch = bs_open_back_channel(device_nbr, dev_nbrs, channel_nbrs, BACK_CHANNELS);

	if (!ch) {
		LOG_ERR("bs_open_back_channel failed!");
	}
}

/* Call it to make peer to proceed.*/
void device_sync_send(void)
{
	uint8_t msg[1] = "S";

	bs_bc_send_msg(0, msg, sizeof(msg));
}

/* Wait until peer send sync*/
void device_sync_wait(void)
{
	int size_msg_received = 0;
	uint8_t msg;

	while (!size_msg_received) {
		size_msg_received = bs_bc_is_msg_received(0);
		k_sleep(K_MSEC(1));
	}

	bs_bc_receive_msg(0, &msg, size_msg_received);
}
