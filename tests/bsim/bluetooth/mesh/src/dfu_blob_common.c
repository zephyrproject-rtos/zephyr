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

	/* Reconfigure SAR Transmitter state to change compile-time configuration to default
	 * configuration.
	 */
	struct bt_mesh_sar_tx tx_set = {
		.seg_int_step = 1,
		.unicast_retrans_count = 3,
		.unicast_retrans_without_prog_count = 2,
		.unicast_retrans_int_step = 7,
		.unicast_retrans_int_inc = 1,
		.multicast_retrans_count = 2,
		.multicast_retrans_int = 3,
	};
	struct bt_mesh_sar_tx tx_rsp;

	err = bt_mesh_sar_cfg_cli_transmitter_set(0, addr, &tx_set, &tx_rsp);
	if (err) {
		FAIL("Failed to configure SAR Transmitter state (err %d)", err);
	}

	/* Reconfigure SAR Receiver state so that the transport layer doesn't generate SegAcks too
	 * frequently.
	 */
	struct bt_mesh_sar_rx rx_set = {
		.seg_thresh = 0x1f,
		.ack_delay_inc = 7,
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
