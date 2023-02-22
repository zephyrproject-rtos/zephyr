/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include "argparse.h"

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

#define CHANNEL_ID 0
#define MSG_SIZE 1

void backchannel_init(void)
{
	uint device_number = get_device_nbr();
	uint peer_number = device_number ^ 1;
	uint device_numbers[] = { peer_number };
	uint channel_numbers[] = { CHANNEL_ID };
	uint *ch;

	ch = bs_open_back_channel(device_number, device_numbers, channel_numbers,
				  ARRAY_SIZE(channel_numbers));
	if (!ch) {
		FAIL("Unable to open backchannel\n");
	}
}

void backchannel_sync_send(void)
{
	uint8_t sync_msg[MSG_SIZE] = { get_device_nbr() };

	printk("Sending sync\n");
	bs_bc_send_msg(CHANNEL_ID, sync_msg, ARRAY_SIZE(sync_msg));
}

void backchannel_sync_wait(void)
{
	uint8_t sync_msg[MSG_SIZE];

	while (true) {
		if (bs_bc_is_msg_received(CHANNEL_ID) > 0) {
			bs_bc_receive_msg(CHANNEL_ID, sync_msg, ARRAY_SIZE(sync_msg));
			if (sync_msg[0] != get_device_nbr()) {
				/* Received a message from another device, exit */
				break;
			}
		}

		k_sleep(K_MSEC(1));
	}

	printk("Sync received\n");
}
