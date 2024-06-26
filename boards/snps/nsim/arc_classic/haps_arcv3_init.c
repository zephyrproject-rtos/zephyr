/*
 * Copyright (c) 2022-2023 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arc/cluster.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>

#define DT_SRAM_NODE_ADDR		(DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) / (1024 * 1024))
#define DT_SRAM_NODE_SIZE		(DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) / (1024 * 1024))

static int haps_arcv3_init(void)
{
	arc_cln_write_reg_nolock(ARC_CLN_PER0_BASE, 0xF00);
	arc_cln_write_reg_nolock(ARC_CLN_PER0_SIZE, 1);

	arc_cln_write_reg_nolock(ARC_CLN_MST_NOC_0_0_ADDR, DT_SRAM_NODE_ADDR);
	arc_cln_write_reg_nolock(ARC_CLN_MST_NOC_0_0_SIZE, DT_SRAM_NODE_SIZE);

	return 0;
}


SYS_INIT(haps_arcv3_init, PRE_KERNEL_1, 0);
