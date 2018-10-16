/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_AON_GLB_H
#define __HAL_AON_GLB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define REG_AON_RST				(BASE_AON_GLB + 0x0)
#define REG_AON_SOFT_RST		(BASE_AON_GLB + 0x4)
#define REG_AON_CORE_LD0		(BASE_AON_GLB + 0x8)
#define REG_AON_CHIP_SLP		(BASE_AON_GLB + 0xC)
#define REG_AON_REG_MEM_CTRL1	(BASE_AON_GLB + 0x1C)
#define REG_AON_REG_MEM_CTRL2	(BASE_AON_GLB + 0x20)
#define REG_AON_GLB_EB			(BASE_AON_GLB + 0x24)
#define REG_AON_GLB_EB1			(BASE_AON_GLB + 0x28)
#define REG_AON_MTX_CTRL		(BASE_AON_GLB + 0x2C)
#define REG_AON_WDG_INT_EN		(BASE_AON_GLB + 0x30)
#define REG_AON_PD_AT_RESET		(BASE_AON_GLB + 0x38)
#define REG_AON_CLK_CTRL0		(BASE_AON_GLB + 0x40)
#define REG_AON_CLK_CTRL1		(BASE_AON_GLB + 0x44)
#define REG_AON_CLK_CTRL2		(BASE_AON_GLB + 0x48)
#define REG_AON_CLK_CTRL3		(BASE_AON_GLB + 0x4C)
#define REG_AON_CLK_CTRL4		(BASE_AON_GLB + 0x50)
#define REG_AON_CLK_CTRL5		(BASE_AON_GLB + 0x54)
#define REG_AON_CLK_CTRL6		(BASE_AON_GLB + 0x58)
#define REG_AON_PERI_MODE		(BASE_AON_GLB + 0x5C)
#define REG_AON_FPGA_DBG_SIG	(BASE_AON_GLB + 0x60)
#define REG_AON_DJTAG_DAP_SEL	(BASE_AON_GLB + 0x64)
#define REG_AON_PLL_CTRL_CFG	(BASE_AON_GLB + 0x168)
#define REG_AON_CHIP_MEM_AUTO_EN (BASE_AON_GLB + 0x198)
#define REG_AON_PWR_CTRL1		(BASE_AON_GLB + 0x19C)
#define REG_AON_PWR_CTRL2		(BASE_AON_GLB + 0x200)
#define REG_AON_MCU_AP_RST		(BASE_AON_GLB + 0x204)
#define REG_AON_CHIP_ID			(BASE_AON_GLB + 0x208)
#define REG_AON_GPIO_MODE1		(BASE_AON_GLB + 0x20C)
#define REG_AON_GPIO_MODE2		(BASE_AON_GLB + 0x210)

	enum {
		AON_EB_SYST = 0,
		AON_EB_TMR0,
		AON_EB_TMR1,
		AON_EB_TMR2,
		AON_EB_DJTAG,
		AON_EB_EIC0,
		AON_EB_EIC_RTC,
		AON_EB_EIC1,
		AON_EB_I2C,
		AON_EB_INTC,
		AON_EB_PIN,
		AON_EB_EFUSE,
		AON_EB_GPIO0,
		AON_EB_GPIO1,
		AON_EB_GPIO2,
		AON_EB_GPIO3,
		AON_EB_GPIO4 = 16,
		AON_EB_GPIO5,
		AON_EB_GPIO6,
		AON_EB_GPIO7,
		AON_EB_RTC_BB,
		AON_EB_SPINLOCK,
		AON_EB_CLK_REG,
		AON_EB_PIN_REG,
		AON_EB_GLB_REG,
		AON_EB_IIS,
		AON_EB_PCIE_PHY,
		AON_EB_FUNTST,
		AON_EB_UART,
		AON_EB_SFC_2X,
		AON_EB_CM4_DAP0,
		AON_EB_CM4_DAP1,
	};

	enum {
		AON_RST_CLK_TOP = 0,
		AON_RST_PMU,
		AON_RST_RTC_BB,
		AON_RST_UART,
		AON_RST_SYST,
		AON_RST_TMR0,
		AON_RST_TMR1,
		AON_RST_TMR2,
		AON_RST_EIC0,
		AON_RST_EIC1,
		AON_RST_I2C,
		AON_RST_EFUSE,
		AON_RST_INTC,
		AON_RST_SPINLOCK,
		AON_RST_GPIO0,
		AON_RST_GPIO1,
		AON_RST_GPIO2 = 16,
		AON_RST_DCXO,
		AON_RST_GPIO4,
		AON_RST_GPIO5,
		AON_RST_GPIO6,
		AON_RST_FM_RF,
		AON_RST_CLK_REG,
		AON_RST_PIN_REG,
		AON_RST_GLB_REG,
	};

#define BIT_AON_APB_CGM_SDIO_FRUN_EN                BIT(26)
#define BIT_AON_APB_CGM_RTCDV5_EN                   BIT(25)
#define BIT_AON_APB_CGM_RTC_EN                      BIT(24)
#define BIT_AON_APB_CGM_IIC_AUTO_EN                 BIT(23)
#define BIT_AON_APB_CGM_IIC_EN                      BIT(22)
#define BIT_AON_APB_CGM_DJTAG_AUTO_EN               BIT(21)
#define BIT_AON_APB_CGM_DJTAG_EN                    BIT(20)
#define BIT_AON_APB_CGM_EFUSE_AUTO_EN               BIT(19)
#define BIT_AON_APB_CGM_EFUSE_EN                    BIT(18)
#define BIT_AON_APB_CGM_DAP_AUTO_EN                 BIT(17)
#define BIT_AON_APB_CGM_DAP_EN                      BIT(16)
#define BIT_AON_APB_CGM_AON_TMR2_AUTO_EN            BIT(15)
#define BIT_AON_APB_CGM_AON_TMR2_EN                 BIT(14)
#define BIT_AON_APB_CGM_AON_TMR1_AUTO_EN            BIT(13)
#define BIT_AON_APB_CGM_AON_TMR1_EN                 BIT(12)
#define BIT_AON_APB_CGM_AON_TMR0_AUTO_EN            BIT(11)
#define BIT_AON_APB_CGM_AON_TMR0_EN                 BIT(10)
#define BIT_AON_APB_CGM_SFC_1X_AUTO_EN              BIT(9)
#define BIT_AON_APB_CGM_SFC_2X_AUTO_EN              BIT(8)
#define BIT_AON_APB_CGM_SFC_1X_EN                   BIT(7)
#define BIT_AON_APB_CGM_SFC_2X_EN                   BIT(6)
#define BIT_AON_APB_CGM_IIS_AUTO_EN                 BIT(5)
#define BIT_AON_APB_CGM_IIS_EN                      BIT(4)
#define BIT_AON_APB_CGM_UART0_AON_AUTO_EN           BIT(3)
#define BIT_AON_APB_CGM_UART0_AON_EN                BIT(2)
#define BIT_AON_APB_CGM_PCIE_AUX_AUTO_EN            BIT(1)
#define BIT_AON_APB_CGM_PCIE_AUX_EN                 BIT(0)

	typedef enum _clk_arm_sel
	{
		CLK_26M   = 0x20,
		CLK_46M   = 0x10,
		CLK_52M   = 0xe,
		CLK_64M   = 0xa,
		CLK_83M   = 0x8,
		CLK_104M   = 0x6,
		CLK_139M   = 0x4,
		CLK_166M   = 0x3,
		CLK_208M   = 0x2,
		CLK_277M   = 0x1,
		CLK_416M   = 0x0,
	} CLK_ARM_SEL_E;

	static inline void uwp_aon_enable(u32_t bits) {
		sci_glb_set(REG_AON_GLB_EB, bits);
	}

	static inline void uwp_aon_disable(u32_t bits) {
		sci_glb_clr(REG_AON_GLB_EB, bits);
	}

	static inline void uwp_aon_reset(u32_t bits) {
		int i = 50;

		sci_glb_set(REG_AON_RST, bits);

		while(i--) ;

		sci_glb_clr(REG_AON_RST, bits);
	}

#ifdef __cplusplus
}
#endif

#endif
