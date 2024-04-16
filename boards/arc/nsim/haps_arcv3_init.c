/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arc/v2/aux_regs.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>

#define ARC_CLN_MST_NOC_0_0_ADDR	292
#define ARC_CLN_MST_NOC_0_0_SIZE	293

#define ARC_CLN_MST_NOC_0_1_ADDR	2560
#define ARC_CLN_MST_NOC_0_1_SIZE	2561

#define ARC_CLN_MST_NOC_0_2_ADDR	2562
#define ARC_CLN_MST_NOC_0_2_SIZE	2563

#define ARC_CLN_MST_NOC_0_3_ADDR	2564
#define ARC_CLN_MST_NOC_0_3_SIZE	2565

#define ARC_CLN_MST_NOC_0_4_ADDR	2566
#define ARC_CLN_MST_NOC_0_4_SIZE	2567

#define ARC_CLN_PER0_BASE		2688
#define ARC_CLN_PER0_SIZE		2689

#define AUX_CLN_ADDR			0x640
#define AUX_CLN_DATA			0x641


static int haps_arcv3_init(void)
{

	z_arc_v2_aux_reg_write(AUX_CLN_ADDR, ARC_CLN_PER0_BASE);
	z_arc_v2_aux_reg_write(AUX_CLN_DATA, 0xF00);
	z_arc_v2_aux_reg_write(AUX_CLN_ADDR, ARC_CLN_PER0_SIZE);
	z_arc_v2_aux_reg_write(AUX_CLN_DATA, 1);

	z_arc_v2_aux_reg_write(AUX_CLN_ADDR, ARC_CLN_MST_NOC_0_0_ADDR);
	z_arc_v2_aux_reg_write(AUX_CLN_DATA, (DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) / (1024 * 1024)));
	z_arc_v2_aux_reg_write(AUX_CLN_ADDR, ARC_CLN_MST_NOC_0_0_SIZE);
	z_arc_v2_aux_reg_write(AUX_CLN_DATA, (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) / (1024 * 1024)));


	return 0;
}


SYS_INIT(haps_arcv3_init, PRE_KERNEL_1, 0);
