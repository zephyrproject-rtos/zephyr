/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_bt_utils.h"
#include "bs_pc_backchannel.h"
#include "argparse.h"

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * 1000000)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS(60)
#define CHANNEL_ID	       0
#define MSG_SIZE	       1

void test_tick(bs_time_t HW_device_time)
{
	bs_trace_debug_time(0, "Simulation ends now.\n");
	if (bst_result != Passed) {
		bst_result = Failed;
		bs_trace_error("Test did not pass before simulation ended.\n");
	}
}

void test_init(void)
{
	bst_ticker_set_next_tick_absolute(TEST_TIMEOUT_SIMULATED);
	bst_result = In_progress;
}

void backchannel_init(uint peer)
{
	uint device_number = get_device_nbr();
	uint device_numbers[] = {peer};
	uint channel_numbers[] = {CHANNEL_ID};
	uint *ch;

	ch = bs_open_back_channel(device_number, device_numbers, channel_numbers,
				  ARRAY_SIZE(channel_numbers));
	if (!ch) {
		FAIL("Unable to open backchannel\n");
	}
}

void backchannel_sync_send(void)
{
	uint8_t sync_msg[MSG_SIZE] = {get_device_nbr()};

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
}

void print_address(bt_addr_le_t *addr)
{
	char array[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, array, sizeof(array));
	printk("Address : %s\n", array);
}
