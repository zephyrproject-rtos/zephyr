/*
 * Copyright (c) 2023 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#define ADDR_TRANSLATE_RAT_BASE_ADDR		(0x044200000u)

static struct address_trans_region_config region_config[] = {
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

static int am62x_m4_init(void)
{
	sys_mm_drv_ti_rat_init(
		region_config, ADDR_TRANSLATE_RAT_BASE_ADDR, ARRAY_SIZE(region_config));
	return 0;
}

SYS_INIT(am62x_m4_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
