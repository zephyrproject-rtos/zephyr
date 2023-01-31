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
