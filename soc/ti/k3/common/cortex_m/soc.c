/*
 * Copyright (c) 2023 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ctrl_partitions.h"
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#define ADDR_TRANSLATE_RAT_BASE_ADDR (0x44200000u)

static struct address_trans_region_config am6x_region_config[] = {
	{
		.system_addr = 0x00000000u,
		.local_addr = 0x60000000u,
		.size = address_trans_region_size_256M,
	},
	{
		.system_addr = 0x20000000u,
		.local_addr = 0xc0000000u,
		.size = address_trans_region_size_512M,
	},
	{
		.system_addr = 0x40000000u,
		.local_addr = 0x70000000u,
		.size = address_trans_region_size_256M,
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

#if defined(CONFIG_SOC_SERIES_AM62X_M4)

/*
 * MCU_CTRL_MMR0 as addressed by the M4F. The first RAT region above maps system
 * address 0x00000000 to local 0x60000000, so the addresses documented in the
 * AM62x TRM (SPRUIV7) are reached at that offset.
 */
#define MCU_CTRL_MMR_BASE        0x64500000U
#define MCU_CTRL_MMR_RST_CTRL    (MCU_CTRL_MMR_BASE + 0x18170U)
#define MCU_CTRL_MMR_LOCK6_KICK0 (MCU_CTRL_MMR_BASE + 0x19008U)
#define MCU_CTRL_MMR_LOCK6_KICK1 (MCU_CTRL_MMR_BASE + 0x1900CU)

/*
 * RST_CTRL field SW_MCU_WARMRST. Writing 0x6 requests the reset; the field is
 * fault tolerant and returns to 0xF by itself.
 */
#define RST_CTRL_SW_MCU_WARMRST_MSK GENMASK(11, 8)
#define RST_CTRL_SW_MCU_WARMRST_REQ 0x6U

/*
 * SYSRESETREQ, which the weak Cortex-M implementation asserts, is not routed
 * for this core; resets are requested through MCU_CTRL_MMR0 instead. The
 * partition holding RST_CTRL is not among those unlocked at boot, so it is
 * kicked here.
 *
 * The reset covers the MAIN domain as well as the MCU domain.
 */
void sys_arch_reboot(int type)
{
	uint32_t rst_ctrl;

	ARG_UNUSED(type);

	sys_write32(KICK0_UNLOCK_VAL, MCU_CTRL_MMR_LOCK6_KICK0);
	sys_write32(KICK1_UNLOCK_VAL, MCU_CTRL_MMR_LOCK6_KICK1);

	rst_ctrl = sys_read32(MCU_CTRL_MMR_RST_CTRL);
	rst_ctrl &= ~RST_CTRL_SW_MCU_WARMRST_MSK;
	rst_ctrl |= FIELD_PREP(RST_CTRL_SW_MCU_WARMRST_MSK, RST_CTRL_SW_MCU_WARMRST_REQ);
	sys_write32(rst_ctrl, MCU_CTRL_MMR_RST_CTRL);
}

#endif /* CONFIG_SOC_SERIES_AM62X_M4 */
