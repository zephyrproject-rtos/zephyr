/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dfu_blob_common.h"
#include <zephyr/sys/util.h>
#include "mesh_test.h"

static struct {
	uint16_t addrs[6];
	int rem_cnt;
} lost_targets;

bool lost_target_find_and_remove(uint16_t addr)
{
	for (int i = 0; i < ARRAY_SIZE(lost_targets.addrs); i++) {
		if (addr == lost_targets.addrs[i]) {
			lost_targets.addrs[i] = 0;
			ASSERT_TRUE(lost_targets.rem_cnt > 0);
			lost_targets.rem_cnt--;
			return true;
		}
	}

	return false;
}

void lost_target_add(uint16_t addr)
{
	if (lost_targets.rem_cnt >= ARRAY_SIZE(lost_targets.addrs)) {
		FAIL("No more room in lost target list");
		return;
	}

	lost_targets.addrs[lost_targets.rem_cnt] = addr;
	lost_targets.rem_cnt++;
}

int lost_targets_rem(void)
{
	return lost_targets.rem_cnt;
}

void common_sar_conf(uint16_t addr)
{
	int err;

	/* Reconfigure SAR Transmitter and Receiver states so BLOB and DFU transfers
	 * complete without segment retransmission storms in the bsim environment.
	 *
	 * Timing invariants that must hold to avoid false -ETIMEDOUT on TX and
	 * unnecessary retransmit bursts:
	 *   1. unicast retransmission timeout > max ACK_DELAY, so TX does not
	 *      retransmit before RX has a chance to send the segment ack.
	 *   2. unicast_retrans_count > 0 so the transport gets more than one
	 *      chance to deliver a segmented message; single-attempt delivery
	 *      is too fragile when multiple Targets respond concurrently to a
	 *      multicast Firmware Update Apply and their unicast responses
	 *      collide in bsim's 2.4 GHz airtime model.
	 *
	 * With the values below at ttl=1:
	 *   - RX_SEG_INT_MS      = (0xf + 1) * 10  = 160 ms
	 *   - max ACK_DELAY      = (0 * 2 + 3) * 160 / 2 = 240 ms
	 *   - RETRANS_TIMEOUT@1  = (0xf + 1) * 25 = 400 ms
	 *   - Total TX budget    = (2 + 1) * 400  = 1200 ms
	 * so the ack always arrives well before the first retransmission fires
	 * on the ideal path (no storm), but there are two extra attempts in
	 * reserve when a segment collision drops the initial burst. Only
	 * unacked segments are retransmitted (see seg_tx_send_unacked in
	 * transport.c), so the retry cost scales with actual loss, not with
	 * the retry budget.
	 * With seg_thresh = 0x1f (max), the RX side does not retransmit acks,
	 * so lowering ack_delay_inc only shortens ack latency without adding
	 * airtime.
	 */
	struct bt_mesh_sar_tx tx_set = {
		.seg_int_step = 1,
		.unicast_retrans_count = 2,
		.unicast_retrans_without_prog_count = 3,
		.unicast_retrans_int_step = 0xf,
		.unicast_retrans_int_inc = 1,
		.multicast_retrans_count = 2,
		.multicast_retrans_int = 3,
	};
	struct bt_mesh_sar_tx tx_rsp;

	err = bt_mesh_sar_cfg_cli_transmitter_set(0, addr, &tx_set, &tx_rsp);
	if (err) {
		FAIL("Failed to configure SAR Transmitter state (err %d)", err);
	}

	struct bt_mesh_sar_rx rx_set = {
		.seg_thresh = 0x1f,
		.ack_delay_inc = 0,
		.discard_timeout = 1,
		.rx_seg_int_step = 0xf,
		.ack_retrans_count = 1,
	};
	struct bt_mesh_sar_rx rx_rsp;

	err = bt_mesh_sar_cfg_cli_receiver_set(0, addr, &rx_set, &rx_rsp);
	if (err) {
		FAIL("Failed to configure SAR Receiver state (err %d)", err);
	}
}
