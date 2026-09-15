/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/fatal.h>

#include "soc.h"
#include "ctrl_partitions.h"
#include <zephyr/cache.h>

void soc_early_init_hook(void)
{
	sys_cache_data_enable();
	sys_cache_instr_enable();

	k3_unlock_all_ctrl_partitions();
}
