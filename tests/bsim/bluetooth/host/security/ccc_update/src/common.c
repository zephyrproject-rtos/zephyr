/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "common.h"

#include "argparse.h"
#include "bs_pc_backchannel.h"

void backchannel_sync_send(uint channel, uint device_nbr)
{
	uint8_t sync_msg[BC_MSG_SIZE] = {get_device_nbr(), device_nbr}; /* src, dst */

	bs_bc_send_msg(channel, sync_msg, ARRAY_SIZE(sync_msg));
}

void backchannel_sync_wait(uint channel, uint device_nbr)
{
	uint8_t sync_msg[BC_MSG_SIZE];

	LOG_DBG("Wait for %d on channel %d", device_nbr, channel);

	while (true) {
		if (bs_bc_is_msg_received(channel) > 0) {
			bs_bc_receive_msg(channel, sync_msg, ARRAY_SIZE(sync_msg));

			if (sync_msg[0] == device_nbr && sync_msg[1] == get_device_nbr()) {
				break;
			}
		}

		k_msleep(1);
	}

	LOG_DBG("Sync received");
}
