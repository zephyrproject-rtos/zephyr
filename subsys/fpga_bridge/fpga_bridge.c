/*
 * Copyright (c) 2024, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fpga_bridge/fpga_bridge.h>

__weak int32_t do_bridge_reset_plat(uint32_t action, uint32_t mask)
{
	ARG_UNUSED(action);
	ARG_UNUSED(mask);
	return -ENOSYS;
}

int32_t do_bridge_reset(uint32_t action, uint32_t mask)
{
	return do_bridge_reset_plat(action, mask);
}
