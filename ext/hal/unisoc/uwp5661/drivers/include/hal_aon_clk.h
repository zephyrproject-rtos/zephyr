/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_AON_CLK_H
#define __HAL_AON_CLK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define REG_AON_CLK_PRE_DIV_PLL_WAIT_SEL0_CFG	(BASE_AON_CLK_BASE + 0x24)

#define REG_AON_CLK_RF_CGM_ARM_CFG           (BASE_AON_CLK_RF +0x0020)
#define REG_AON_CLK_RF_CGM_BTWF_MTX_CFG      (BASE_AON_CLK_RF +0x0024)
#define REG_AON_CLK_RF_CGM_AHB_BT_CFG        (BASE_AON_CLK_RF +0x0028)
#define REG_AON_CLK_RF_CGM_WIFI_MAC_APB_CFG  (BASE_AON_CLK_RF +0x002C)
#define REG_AON_CLK_RF_CGM_SFC_2X_CFG        (BASE_AON_CLK_RF +0x0030)
#define REG_AON_CLK_RF_CGM_SFC_1X_CFG		 (BASE_AON_CLK_RF +0x0034)
#define REG_AON_CLK_RF_CGM_IIC_CFG           (BASE_AON_CLK_RF +0x006C)

	/*------------------------------------------------------------------
	//Register Name :REG_AON_CLK_RF_CGM_ARM_CFG
	//Register Offset:0x0020
	-------------------------------------------------------------------*/
#define BIT_AON_CLK_RF_CGM_ARM_SEL(x)       (((x)&0x7))

	typedef enum _arm_clk_sel
	{
		CLK_SRC0 = 0,
		CLK_SRC1,
		CLK_SRC2,
		CLK_SRC3,
		CLK_SRC4,
		CLK_SRC5,
	} ARM_CLK_SEL_E;

#ifdef __cplusplus
}
#endif

#endif
