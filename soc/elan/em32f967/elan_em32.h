/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_SOC_ELAN_EM32F967_ELAN_EM32_H__
#define __ZEPHYR_SOC_ELAN_EM32F967_ELAN_EM32_H__

#include <stdint.h>
#include <cmsis_core_m_defaults.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Enumeration
 */

typedef enum {
	HCLKG_DMA = 0x00,
	HCLKG_GPIOA = 0x01,
	HCLKG_GPIOB = 0x02,
	PCLKG_LPC = 0x03,
	HCLKG_7816_1 = 0x04,
	HCLKG_7816_2 = 0x05,
	HCLKG_ENCRYPT = 0x06,
	PCLKG_USART = 0x07,
	PCLKG_TMR1 = 0x08,
	PCLKG_TMR2 = 0x09,
	PCLKG_TMR3 = 0x0a,
	PCLKG_TMR4 = 0x0b,
	PCLKG_UART1 = 0x0c,
	PCLKG_UART2 = 0x0d,
	PCLKG_RVD1 = 0x0e,
	HCLKG_ESPI1 = 0x0f,
	PCLKG_SSP2 = 0x10,
	PCLKG_I2C1 = 0x11,
	PCLKG_I2C2 = 0x12,
	PCLKG_PWM = 0x13,
	PCLKG_RVD2 = 0x14,
	PCLKG_UDC = 0x15,
	PCLKG_ATRIM = 0x16,
	PCLKG_RTC = 0x17,
	PCLKG_BKP = 0x18,
	PCLKG_DWG = 0x19,
	PCLKG_PWR = 0x1a,
	PCLKG_CACHE = 0x1b,
	PCLKG_AIP = 0x1c,
	PCLKG_ECC = 0x1d,
	PCLKG_TRNG = 0x1e,
	HCLKG_EXTSPI = 0x1f,
	HCLKG_GHM_ACC1 = 0x20,
	HCLKG_GHM_ACC2 = 0x21,
	HCLKG_GHM_ACC3 = 0x22,
	HCLKF_GHM_IP = 0x23,
	HCLKF_FLASH_BIST = 0x24,
	HCLKF_GHM_RANSAC = 0x25,
	HCLKF_SWSPI = 0x26,
	HCLKF_GHM_DOUBLE = 0x27,
	HCLKF_GHM_DISTINGUISH = 0x28,
	HCLKF_GHM_LSE = 0x29,
	HCLKF_GHM_SAD = 0x2a,
	HCLKF_GHM_M2D = 0x2b,
	PCLKG_SSP1 = 0x30,
	PCLKG_ALL = 0xffff
} CLKGatingSwitch;

/*
 * Data Structure
 */

typedef struct {
	__IO uint32_t MIRCPD: 1;
	__IO uint32_t HIRC_TESTV: 1;
	__IO uint32_t MIRCRCM: 3;
	__IO uint32_t MIRCCA: 6;
	__IO uint32_t MIRCTBG: 2;
	__IO uint32_t MIRCTCF: 2;
	__IO uint32_t MIRCTV12: 3;
	__IO uint32_t Reserved1: 14;
} MIRC_Type;

typedef struct {
	__IO uint32_t MIRCPD: 1;
	__IO uint32_t HIRC_TESTV: 1;
	__IO uint32_t MIRCRCM: 3;
	__IO uint32_t MIRCTall: 10;
	__IO uint32_t MIRCTV12: 3;
	__IO uint32_t Reserved1: 14;
} MIRC_Type2;

typedef struct {
	__IO uint32_t WaitCount: 3;
	__IO uint32_t WaitCountSet: 1;
	__IO uint32_t WaitCountPass: 4;
	__IO uint32_t Reserved: 18;
	__IO uint32_t Remap_SYSR_Boot: 1;
	__IO uint32_t Remap_IDR_Boot: 1;
	__IO uint32_t Gating_CPU_CLK: 1;
	__IO uint32_t Remap_Switch: 1;
	__IO uint32_t CPUReady_SkipArbiter: 1;
	__IO uint32_t SWRESTEN: 1;
} MISCReg_Type;

typedef struct {
	__IO uint32_t XTALHIRCSEL: 1;    // [0]
	__IO uint32_t XTALLJIRCSEL: 1;   // [1]
	__IO uint32_t HCLKSEL: 2;        // [3:2]
	__IO uint32_t USBCLKSEL: 1;      // [4]
	__IO uint32_t HCLKDIV: 3;        // [7:5]
	__IO uint32_t QSPICLK_SEL: 1;    // [8]
	__IO uint32_t ACC1CLK_SEL: 1;    // [9]
	__IO uint32_t ENCRYPT_SEL: 1;    // [10]
	__IO uint32_t Timer1_SEL: 1;     // [11]
	__IO uint32_t Timer2_SEL: 1;     // [12]
	__IO uint32_t Timer3_SEL: 1;     // [13]
	__IO uint32_t Timer4_SEL: 1;     // [14]
	__IO uint32_t QSPICLK_DIV: 1;    // [15]
	__IO uint32_t ACC1CLK_DIV: 1;    // [16]
	__IO uint32_t EncryptCLK_DIV: 1; // [17]
	__IO uint32_t RTC_SEL: 1;        // [18]
	__IO uint32_t I2C1Reset_SEL: 1;  // [19]
	__IO uint32_t USBReset_SEL: 1;   // [20]
	__IO uint32_t HIRC_TESTV: 1;     // [21]
	__IO uint32_t SWRESTN: 1;        // [22]
	__IO uint32_t DEEPSLPCLKOFF: 1;  // [23]
	__IO uint32_t ClearECCKey: 1;    // [24]
	__IO uint32_t POWEN: 1;          // [25]
	__IO uint32_t RESETOP: 1;        // [26]
	__IO uint32_t PMUCTRL: 1;        // [27]
	__IO uint32_t REAMPMODE: 4;      // [31:28]
} SysReg_Type;

typedef struct {
	__IO uint32_t SYSPLLPD: 1;
	__IO uint32_t SYSPLLPSET: 2;
	__IO uint32_t SYSPLLFSET: 4;
	__I uint32_t SYSPLLSTABLECNT: 2;
	__I uint32_t SYSPLLSTABLE: 1;
	__IO uint32_t Reserved: 22;
} SYSPLL_Type;

typedef struct {
	__IO uint32_t PLLLDO_PD: 1;
	__IO uint32_t PLLLDO_VP_SEL: 1;
	__IO uint32_t PLLLDO_VS: 3;
	__IO uint32_t PLLLDO_TV12: 4;
	__IO uint32_t Reserved: 23;
} LDOPLL_Type;

typedef struct {
	__IO uint32_t LEVELRSTS: 1;
	__IO uint32_t WDTRESETS: 1;
	__IO uint32_t SWRESETS: 1;
	__IO uint32_t SYSREQ_RST_STATUS: 1;
	__IO uint32_t LOCKUP_RST_STATUS: 1;
	__I uint32_t FLASHWCOUNTS: 3;
	__IO uint32_t BORRESETS: 1;
	__IO uint32_t LVDFlashRSTS: 1;
	__IO uint32_t Reserved: 22;
} SysStatus_Type;

typedef struct {
	__IO uint32_t POWERSW: 3;
	__IO uint32_t WARMUPCNT: 3;
	__IO uint32_t PD_SW_ACK_EN: 1;
	__IO uint32_t StandBy1_S: 1;
	__IO uint32_t StandBy2_S: 1;
	__IO uint32_t SIPPDEnable: 1;
	__IO uint32_t LDOIdle: 1;
	__IO uint32_t HIRCPD: 1;
	__IO uint32_t SIRC32PD: 1;
	__IO uint32_t BORPD: 1;
	__IO uint32_t LDO2PD: 1;
	__IO uint32_t RAMPDEnable: 1;
	__IO uint32_t Reserved: 16;
} PowerSW_Type;

// Flash Control Register
typedef struct 
{
                                    
    __IO uint32_t Reserved0         : 1;  // [0]
    __IO uint32_t Flash_Idle        : 1;  // [1]
    __IO uint32_t Flash_BurstWrite  : 1;  // [2]
    __IO uint32_t Flash_PageErase   : 1;  // [3]
    __IO uint32_t Flash_Verify      : 1;  // [4]
    __IO uint32_t Reserved1         : 2;  // [6:5]
    __IO uint32_t XYADDR            : 9;  // [15:7] 512 bytes address
    //__IO uint32_t YADDR           : 5;  // [11:7] 32 * 128 bits buffer start addr
    //__IO uint32_t XADDR0          : 4;  // [15:12] 1 page = 8K = 16 * buffers' start
    __IO uint32_t PageAddr          : 7;  // [22:16] total 86 pages
    __IO uint32_t EraseEnable       : 1;  // [23]
    __IO uint32_t BurstW_Length     : 5;  // [28:24] 32 (*128 bits )
    __IO uint32_t Reserved2         : 1;  // [29]
    __IO uint32_t FW_load           : 1;  // [30]
    __IO uint32_t BurstW_Enable     : 1;  // [31]
} FLASHSR0_Type;

/*
 * Definition
 */

#define CLKGATEREG  (*(__IO uint32_t *)0x40030100)
#define CLKGATEREG2 (*(__IO uint32_t *)0x40030104)

#define MIRCCTRL    ((MIRC_Type *)0x40036000)
#define MIRCCTRL_2  ((MIRC_Type2 *)0x40036000)
#define MISCREGCTRL ((MISCReg_Type *)0x40030008)
#define SYSREGCTRL  ((SysReg_Type *)0x40030000)
#define SYSPLLCTRL  ((SYSPLL_Type *)0x40036404)
#define LDOPLL      ((LDOPLL_Type *)0x4003630c)

#define SYSSTATUSCTRL ((SysStatus_Type *)0x40030004)
#define POWERSWCTRL   ((PowerSW_Type *)0x40031000)

#define FLASHSR0_Ctrl   ((FLASHSR0_Type *)0x40034010)
#define FLASH_SR0       (*( __IO uint32_t *)0x40034010)
#define FLASHKEY1       (*( __IO uint32_t *)0x40034000)
#define FLASHKEY2       (*( __IO uint32_t *)0x40034004)

#ifdef __cplusplus
}
#endif

#endif //__ZEPHYR_SOC_ELAN_EM32F967_ELAN_EM32_H__
