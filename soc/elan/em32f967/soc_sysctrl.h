/*
 * Copyright (c) 2025 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ELAN_EM32F967_SOC_SYSCTRL_H_
#define ZEPHYR_SOC_ELAN_EM32F967_SOC_SYSCTRL_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/* Devicetree node label */
#define SYSCTRL_DT_NODE DT_NODELABEL(sysctrl)

/* Register Offsets */
#define SYSCTRL_SYS_REG_CTRL_OFF    0x0000
#define SYSCTRL_SYS_STATUS_CTRL_OFF 0x0004
#define SYSCTRL_MISC_REG_CTRL_OFF   0x0008
#define SYSCTRL_CLK_GATE_REG_OFF    0x0100
#define SYSCTRL_CLK_GATE_REG2_OFF   0x0104

/* Field Masks for SYS_REG_CTRL */
#define SYSCTRL_XTAL_HIRC_SEL    BIT(0)          /* [0]   XTALHIRCSEL */
#define SYSCTRL_XTAL_LJIRC_SEL   BIT(1)          /* [1]   XTALLJIRCSEL */
#define SYSCTRL_HCLK_SEL_MASK    GENMASK(3, 2)   /* [3:2] HCLKSEL */
#define SYSCTRL_USB_CLK_SEL      BIT(4)          /* [4]   USBCLKSEL */
#define SYSCTRL_HCLK_DIV_MASK    GENMASK(7, 5)   /* [7:5] HCLKDIV */
#define SYSCTRL_QSPI_CLK_SEL     BIT(8)          /* [8]   QSPICLK_SEL */
#define SYSCTRL_ACC1_CLK_SEL     BIT(9)          /* [9]   ACC1CLK_SEL */
#define SYSCTRL_ENCRYPT_SEL      BIT(10)         /* [10]  ENCRYPT_SEL */
#define SYSCTRL_TIMER1_SEL       BIT(11)         /* [11]  Timer1_SEL */
#define SYSCTRL_TIMER2_SEL       BIT(12)         /* [12]  Timer2_SEL */
#define SYSCTRL_TIMER3_SEL       BIT(13)         /* [13]  Timer3_SEL */
#define SYSCTRL_TIMER4_SEL       BIT(14)         /* [14]  Timer4_SEL */
#define SYSCTRL_QSPI_CLK_DIV     BIT(15)         /* [15]  QSPICLK_DIV */
#define SYSCTRL_ACC1_CLK_DIV     BIT(16)         /* [16]  ACC1CLK_DIV */
#define SYSCTRL_ENCRYPT_CLK_DIV  BIT(17)         /* [17]  EncryptCLK_DIV */
#define SYSCTRL_RTC_SEL          BIT(18)         /* [18]  RTC_SEL */
#define SYSCTRL_I2C1_RESET_SEL   BIT(19)         /* [19]  I2C1Reset_SEL */
#define SYSCTRL_USB_RESET_SEL    BIT(20)         /* [20]  USBReset_SEL */
#define SYSCTRL_HIRC_TESTV       BIT(21)         /* [21]  HIRC_TESTV */
#define SYSCTRL_SW_RESTN         BIT(22)         /* [22]  SWRESTN */
#define SYSCTRL_DEEP_SLP_CLK_OFF BIT(23)         /* [23]  DEEPSLPCLKOFF */
#define SYSCTRL_CLEAR_ECC_KEY    BIT(24)         /* [24]  ClearECCKey */
#define SYSCTRL_POW_EN           BIT(25)         /* [25]  POWEN */
#define SYSCTRL_RESET_OP         BIT(26)         /* [26]  RESETOP */
#define SYSCTRL_PMU_CTRL         BIT(27)         /* [27]  PMUCTRL */
#define SYSCTRL_REAMP_MODE_MASK  GENMASK(31, 28) /* [31:28] REAMPMODE */

/* Field Masks for SYS_STATUS_CTRL */
#define SYSCTRL_LEVEL_RSTS        BIT(0)        /* [0]   LEVELRSTS */
#define SYSCTRL_WDT_RESETS        BIT(1)        /* [1]   WDTRESETS */
#define SYSCTRL_SW_RESETS         BIT(2)        /* [2]   SWRESETS */
#define SYSCTRL_SYSREQ_RST_STATUS BIT(3)        /* [3]   SYSREQ_RST_STATUS */
#define SYSCTRL_LOCKUP_RST_STATUS BIT(4)        /* [4]   LOCKUP_RST_STATUS */
#define SYSCTRL_FLASHW_COUNT_MASK GENMASK(7, 5) /* [7:5] FLASHWCOUNTS */
#define SYSCTRL_BOR_RESETS        BIT(8)        /* [8]   BORRESETS */
#define SYSCTRL_LVD_FLASH_RSTS    BIT(9)        /* [9]   LVDFlashRSTS */

/* Field Masks for MISC_REG_CTRL */
#define SYSCTRL_WAIT_COUNT_MASK       GENMASK(2, 0) /* [2:0]   WaitCount */
#define SYSCTRL_WAIT_COUNT_SET        BIT(3)        /* [3]     WaitCountSet */
#define SYSCTRL_WAIT_COUNT_PASS_MASK  GENMASK(7, 4) /* [7:4]   WaitCountPass */
#define SYSCTRL_REMAP_SYSR_BOOT       BIT(26)       /* [26]    Remap_SYSR_Boot */
#define SYSCTRL_REMAP_IDR_BOOT        BIT(27)       /* [27]    Remap_IDR_Boot */
#define SYSCTRL_GATING_CPU_CLK        BIT(28)       /* [28]    Gating_CPU_CLK */
#define SYSCTRL_REMAP_SWITCH          BIT(29)       /* [29]    Remap_Switch */
#define SYSCTRL_CPUREADY_SKIP_ARBITER BIT(30)       /* [30]    CPUReady_SkipArbiter */
#define SYSCTRL_SW_RESTEN             BIT(31)       /* [31]    SWRESTEN */

#endif /* ZEPHYR_SOC_ELAN_EM32F967_SOC_SYSCTRL_H_ */
