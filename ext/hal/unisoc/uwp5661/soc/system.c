/*
 * Copyright (c) 2018, UNISOC Incorporated
 * All rights reserved.
 *
 */
#include <zephyr.h>
#include <uwp_hal.h>

void uwp_glb_init(void)
{
	/* Set system clock to 416M */
	/* 0x40088018 */
	sci_write32(REG_APB_EB, 0xffffffff); /* FIXME */
	/* 0x4083c050 */
	sci_glb_set(REG_AON_CLK_CTRL4, BIT_AON_APB_CGM_RTC_EN);
	/* 0x4083c024 */
	sci_write32(REG_AON_GLB_EB, 0xf7ffffff); /* FIXME */
	/* 0x40130004 */
	sci_reg_or(REG_AHB_EB0, BIT(0) | BIT(1));

	/* 0x4083c168 */
	sci_reg_and(REG_AON_PLL_CTRL_CFG, ~0xFFFF);
	sci_reg_or(REG_AON_PLL_CTRL_CFG, CLK_416M);
	/* 0x40844220 */
	sci_write32(REG_AON_CLK_RF_CGM_ARM_CFG,
			BIT_AON_CLK_RF_CGM_ARM_SEL(CLK_SRC5));

	/* 0x40130114 */
	sci_reg_or(REG_AHB_MTX_CTL1, BIT(22));
	/* 0x4083c038 */
	//sci_reg_and(REG_AON_PD_AT_RESET, ~BIT(0));
	/* 0x40844024 */
	sci_reg_and(REG_AON_CLK_PRE_DIV_PLL_WAIT_SEL0_CFG, ~BIT(2));
}


