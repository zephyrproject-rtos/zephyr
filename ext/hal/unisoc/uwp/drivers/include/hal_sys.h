/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_SYS_H
#define __HAL_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define CTL_APB_BASE			BASE_GLB

#define REG_APB_RST				(CTL_APB_BASE + 0x0)
#define REG_MCU_SOFT_RST		(CTL_APB_BASE + 0x4)
#define REG_PWR_ON_RSTN_INDEX	(CTL_APB_BASE + 0x8)
#define REG_SYS_SOFT_RST		(CTL_APB_BASE + 0xC)

#define REG_APB_EB				(CTL_APB_BASE + 0x18)
#define REG_APB_EB1				(CTL_APB_BASE + 0x1C)
#define REG_WDG_INT_EN			(CTL_APB_BASE + 0x20)
#define REG_TRIM_LDO_DIGCORE	(CTL_APB_BASE + 0x24)
#define REG_WFRX_MODE_SEL		(CTL_APB_BASE + 0x28)
#define REG_FM_RF_CTRL			(CTL_APB_BASE + 0x2C)
#define REG_APB_SLP_CTL0		(CTL_APB_BASE + 0x30)
#define REG_APB_INT_STS0		(CTL_APB_BASE + 0x34)
#define REG_WIFI_SYS0			(CTL_APB_BASE + 0x70)
#define REG_WIFI_SYS1			(CTL_APB_BASE + 0x74)

#define REG_AHB_RST0			(BASE_AHB+0X0000)
#define REG_AHB_EB0				(BASE_AHB+0X0004)

#define REG_AHB_CM4_AHB_CTRL    (BASE_AHB+0X017C)
#define REG_AHB_MTX_CTL1		(BASE_AHB+0x0114)

#define BIT_AHB_WIFI_AXI_FORCE_EB           BIT(12)
#define BIT_AHB_PCIE_EB                     BIT(11)
#define BIT_AHB_DAP_EB                      BIT(1)
#define BIT_AHB_WIFI_MAC_EB                 BIT(9)
#define BIT_AHB_CACHE_CFG_EB                BIT(8)
#define BIT_AHB_EDMA_EB                     BIT(7)
#define BIT_AHB_SDIO_AXI_FORCE_EB           BIT(6)
#define BIT_AHB_WIFI_EB                     BIT(5)
#define BIT_AHB_BT_EB                       BIT(4)
#define BIT_AHB_BUSMON1_EB                  BIT(3)
#define BIT_AHB_BUSMON0_EN                  BIT(2)
#define BIT_AHB_SDIO_EB                     BIT(1)
#define BIT_AHB_DMA_EB                      BIT(0)

#define BIT_AHB_BRG_BYPASS_ACK_CM4I         BIT(4)
#define BIT_AHB_BRG_BYPASS_ACK_CM4D         BIT(3)
#define BIT_AHB_BRG_BYPASS_ACK_CM4S         BIT(2)
#define BIT_AHB_BRG_BYPASS_MODE             BIT(1)
#define BIT_AHB_BRG_BYPASS_SWEN             BIT(0)

#define APB_MCU_SOFT_RST	0
#define APB_PWR_ON_RST		0
#define APB_SYS_SOFT_RST	0

	enum {
		APB_EB_TMR3_RTC = 0,
		APB_EB_TMR4_RTC,
		APB_EB_UART0,
		APB_EB_UART1,
		APB_EB_SYST,
		APB_EB_TMR0,
		APB_EB_TMR1,
		APB_EB_WDG,
		APB_EB_DJTAG,
		APB_EB_WDG_RTC,
		APB_EB_SYST_RTC,
		APB_EB_TMR0_RTC,
		APB_EB_TMR1_RTC,
		APB_EB_BT_RTC,
		APB_EB_EIC,
		APB_EB_EIC_RTC,

		APB_EB_IIS = 16,
		APB_EB_INTC,
		APB_EB_PIN,
		APB_EB_FM,
		APB_EB_EFUSE,
		APB_EB_TMR2,
		APB_EB_TMR3,
		APB_EB_TMR4,
		APB_EB_GPIO,
		APB_EB_IPI,
		APB_EB_COM_TMR_ = 30,
		APB_EB_WCI2,
	};

	struct uwp_sys {
		u32_t rst;
		u32_t mcu_soft_rst;
		u32_t rstn_index;
		u32_t sys_soft_rst;
		u32_t eb;
		u32_t eb1;
	};

	static inline void uwp_sys_enable(u32_t bits) {
		sci_glb_set(REG_APB_EB, bits);
	}

	static inline void uwp_sys_disable(u32_t bits) {
		sci_glb_clr(REG_APB_EB, bits);
	}

	static inline void uwp_sys_reset(u32_t bits) {
		u32_t wait = 50;

		sci_glb_set(REG_APB_RST, bits);
		while(wait--){}
		sci_glb_clr(REG_APB_RST, bits);
	}

#ifdef __cplusplus
}
#endif

#endif
