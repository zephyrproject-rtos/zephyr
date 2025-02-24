/*
 * Copyright (c) 2023 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <common/k3_ctrl_partition.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#define ADDR_TRANSLATE_RAT_BASE_ADDR (0x044200000u)

static struct address_trans_region_config am6x_region_config[] = {
	{
		.system_addr = 0x0u,
		.local_addr = 0x80000000u,
		.size = address_trans_region_size_512M,
	},
	{
		.local_addr = 0xA0000000u,
		.system_addr = 0x20000000u,
		.size = address_trans_region_size_512M,
	},
	{
		.local_addr = 0xC0000000u,
		.system_addr = 0x40000000u,
		.size = address_trans_region_size_512M,
	},
	{
		.local_addr = 0x60000000u,
		.system_addr = 0x60000000u,
		.size = address_trans_region_size_512M,
	},

/*
 * Add regions here if you want to map more memory.
 */
};

void soc_early_init_hook(void)
{
	sys_mm_drv_ti_rat_init(am6x_region_config, ADDR_TRANSLATE_RAT_BASE_ADDR,
			       ARRAY_SIZE(am6x_region_config));

	k3_unlock_all_ctrl_partitions();
}

void soc_late_init_hook(void)
{
	k3_lock_all_ctrl_partitions();
}
