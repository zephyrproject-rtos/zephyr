/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <common/ctrl_partitions.h>

void soc_early_init_hook(void)
{
	k3_unlock_all_ctrl_partitions();
}
