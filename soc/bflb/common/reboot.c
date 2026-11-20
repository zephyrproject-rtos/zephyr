/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <pds_reg.h>

/* Reboot to bootrom to make reset state-less */
#if defined(CONFIG_SOC_SERIES_BL61X) || defined(CONFIG_SOC_SERIES_BL616CL) \
	|| defined(CONFIG_SOC_SERIES_BL808)
#define RESET_BOOTROM_ADDR	0x90000000U
#define RESET_ADDR_ADDR		(PDS_BASE + PDS_CPU_CORE_CFG14_OFFSET)
#endif

void sys_arch_reboot(int type)
{
	uint32_t tmp;

	/* "basically deprecated" */
	ARG_UNUSED(type);

#ifdef RESET_BOOTROM_ADDR
	sys_write32(RESET_BOOTROM_ADDR, RESET_ADDR_ADDR);
#endif

	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG2_OFFSET);
	tmp &= ~(GLB_REG_CTRL_SYS_RESET_MSK | GLB_REG_CTRL_CPU_RESET_MSK
		 | GLB_REG_CTRL_PWRON_RST_MSK);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG2_OFFSET);

	tmp |= GLB_REG_CTRL_CPU_RESET_MSK | GLB_REG_CTRL_SYS_RESET_MSK | GLB_REG_CTRL_PWRON_RST_MSK;
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG2_OFFSET);

	k_busy_wait(USEC_PER_SEC);
}
