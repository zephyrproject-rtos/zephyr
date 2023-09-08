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
#define PINCTRL_BASE_ADDR			(0x4080000u)
#define KICK0_UNLOCK_VAL			(0x68EF3490U)
#define KICK1_UNLOCK_VAL			(0xD172BC5AU)
#define CSL_MCU_PADCONFIG_LOCK0_KICK0_OFFSET	(0x1008)
#define CSL_MCU_PADCONFIG_LOCK1_KICK0_OFFSET	(0x5008)

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

static void mmr_unlock(void)
{
	uint32_t baseAddr = PINCTRL_BASE_ADDR;
	uintptr_t kickAddr;

	/* Lock 0 */
	kickAddr = baseAddr + CSL_MCU_PADCONFIG_LOCK0_KICK0_OFFSET;
	sys_write32(KICK0_UNLOCK_VAL, kickAddr);   /* KICK 0 */
	kickAddr = kickAddr + sizeof(uint32_t *);
	sys_write32(KICK1_UNLOCK_VAL, kickAddr);   /* KICK 1 */

	/* Lock 1 */
	kickAddr = baseAddr + CSL_MCU_PADCONFIG_LOCK1_KICK0_OFFSET;
	sys_write32(KICK0_UNLOCK_VAL, kickAddr);   /* KICK 0 */
	kickAddr = kickAddr + sizeof(uint32_t *);
	sys_write32(KICK1_UNLOCK_VAL, kickAddr);   /* KICK 1 */
}

static int am62x_m4_init(void)
{
	sys_mm_drv_ti_rat_init(
		region_config, ADDR_TRANSLATE_RAT_BASE_ADDR, ARRAY_SIZE(region_config));
	mmr_unlock();
	return 0;
}

SYS_INIT(am62x_m4_init, EARLY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
