/***************************************************************************//**
* \file cy_device.h
* \version 2.10
*
* This file specifies the structure for core and peripheral block HW base
* addresses, versions, and parameters.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef CY_DEVICE_H_
#define CY_DEVICE_H_

#include <stdint.h>
#include <stddef.h>

#include "ip/cyip_cpuss.h"
#include "ip/cyip_cpuss_v2.h"
#include "ip/cyip_flashc.h"
#include "ip/cyip_flashc_v2.h"
#include "ip/cyip_gpio.h"
#include "ip/cyip_gpio_v2.h"
#include "ip/cyip_hsiom.h"
#include "ip/cyip_hsiom_v2.h"
#include "ip/cyip_sflash.h"
#include "ip/cyip_srss.h"
#include "ip/cyip_backup.h"
#include "ip/cyip_peri.h"
#include "ip/cyip_peri_v2.h"
#include "ip/cyip_peri_ms_v2.h"
#include "ip/cyip_prot.h"
#include "ip/cyip_prot_v2.h"
#include "ip/cyip_ipc.h"
#include "ip/cyip_ipc_v2.h"
#include "ip/cyip_udb.h"
#include "ip/cyip_dw.h"
#include "ip/cyip_dw_v2.h"
#include "ip/cyip_dmac_v2.h"
#include "ip/cyip_i2s.h"
#include "ip/cyip_pdm.h"
#include "ip/cyip_lcd.h"
#include "ip/cyip_crypto.h"
#include "ip/cyip_crypto_v2.h"
#include "ip/cyip_sdhc.h"
#include "ip/cyip_smartio.h"

/* Device descriptor type */
typedef struct
{
    /* Base HW addresses */
    uint32_t sflashBase; /* TODO: Remove */
    uint32_t cpussBase;
    uint32_t flashcBase;
    uint32_t srssBase;   /* TODO: Remove */
    uint32_t backupBase; /* TODO: Remove */
    uint32_t periBase;
    uint32_t udbBase;
    uint32_t protBase;
    uint32_t profileBase; /* TODO: Remove */
    uint32_t hsiomBase;
    uint32_t gpioBase;
    uint32_t passBase;

    /* Versions */
    uint8_t cpussVersion;
    uint8_t cryptoVersion;
    uint8_t dwVersion;
    uint8_t flashcVersion;
    uint8_t gpioVersion;
    uint8_t hsiomVersion;
    uint8_t ipcVersion;
    uint8_t periVersion;
    uint8_t protVersion;

    /* Parameters */
    uint8_t  cpussIpcNr;
    uint8_t  cpussIpcIrqNr;
    uint8_t  cpussDwChNr;
    uint8_t  cpussFlashPaSize;
    int8_t   cpussInterruptFmIrq;
    uint8_t  srssNumClkpath;
    uint8_t  srssNumPll;
    uint8_t  srssNumHfroot;
    uint8_t  periClockNr;
    uint8_t  smifDeviceNr;
    uint8_t  passSarChannels;
    uint8_t  iossGpioPortNr;


    /* register structure specific numbers */
    /* DW registers */
    uint16_t dwChOffset;
    uint16_t dwChSize;

    /* PERI registers */
    uint16_t periTrCmdOffset;
    uint16_t periTrCmdGrSelMsk;
    uint16_t periTrGrOffset;
    uint16_t periTrGrSize;

    uint8_t periDivCmdDivSelMsk;
    uint8_t periDivCmdTypeSelPos;
    uint8_t periDivCmdPaDivSelPos;
    uint8_t periDivCmdPaTypeSelPos;

    uint16_t periDiv8CtlOffset;
    uint16_t periDiv16CtlOffset;
    uint16_t periDiv16_5CtlOffset;
    uint16_t periDiv24_5CtlOffset;
    
    /* GPIO registers */
    uint8_t gpioPrtIntrCfgOffset;
    uint8_t gpioPrtCfgOffset;
    uint8_t gpioPrtCfgInOffset;
    uint8_t gpioPrtCfgOutOffset;
    uint8_t gpioPrtCfgSioOffset;

    /* CPUSS registers */
    uint32_t cpussCm0ClockCtlOffset;
    uint32_t cpussCm4ClockCtlOffset;
    uint32_t cpussCm4StatusOffset;
    uint32_t cpussCm0StatusOffset;
    uint32_t cpussCm4PwrCtlOffset;
    uint32_t cpussTrimRamCtlOffset;
    uint32_t cpussTrimRomCtlOffset;
    uint16_t cpussCm0NmiCtlOffset;
    uint16_t cpussCm4NmiCtlOffset;

    /* IPC register */
    uint32_t ipcLockStatusOffset;

    /* Flash Parameters */
    uint32_t flashcSize;
    uint8_t flashRwwSupport;
    uint8_t flashPipeEnable;
    uint8_t flashWriteDelay;
    uint8_t flashProgramDelay;
    uint8_t flashEraseDelay;
    uint8_t flashCtlMainWs0Freq;
    uint8_t flashCtlMainWs1Freq;
    uint8_t flashCtlMainWs2Freq;
    uint8_t flashCtlMainWs3Freq;
    uint8_t flashCtlMainWs4Freq;

    /* SysPm parameters */
    uint8_t sysPmMmioUdbGroupNr;
    uint8_t sysPmMmioUdbSlaveNr;
    uint8_t udbPresent;
    uint8_t sysPmSimoPresent;

    /* CRYPTO register offsets */
    uint32_t cryptoStatusOffset;            // STATUS
    uint32_t cryptoIstrFfCtlOffset;         // INSTR_FF_CTL
    uint32_t cryptoInstrFfStatusOffset;     // INSTR_FF_STATUS
    uint32_t cryptoInstrFfWrOffset;         // INSTR_FF_WR
    uint32_t cryptoVuRfDataOffset;
    uint32_t cryptoAesCtlOffset;            // AES_CTL
    uint32_t cryptoPrResultOffset;          // PR_RESULT
    uint32_t cryptoTrResultOffset;          // TR_RESULT
    uint32_t cryptoCrcCtlOffset;            // CRC_CTL
    uint32_t cryptoCrcDataCtlOffset;        // CRC_DATA_CTL
    uint32_t cryptoCrcPolCtlOffset;         // CRC_POL_CTL
    uint32_t cryptoCrcRemCtlOffset;         // CRC_REM_CTL
    uint32_t cryptoCrcRemResultOffset;      // CRC_REM_RESULT
    uint32_t cryptoVuCtl0Offset;            // VU_CTL0
    uint32_t cryptoVuCtl1Offset;            // VU_CTL1
    uint32_t cryptoVuStatusOffset;          // VU_STATUS
    uint32_t cryptoIntrOffset;
    uint32_t cryptoIntrSetOffset;
    uint32_t cryptoIntrMaskOffset;
    uint32_t cryptoIntrMaskedOffset;
    uint32_t cryptoMemBufOffset;

} cy_stc_device_t;

extern const cy_stc_device_t   cy_deviceIpBlockCfg1;
extern const cy_stc_device_t   cy_deviceIpBlockCfg2;
extern const cy_stc_device_t * cy_device;


/*******************************************************************************
*                Register Access Helper Macros
*******************************************************************************/

#define CY_SRSS_NUM_CLKPATH         ((uint32_t)(cy_device->srssNumClkpath))
#define CY_SRSS_NUM_PLL             ((uint32_t)(cy_device->srssNumPll))
#define CY_SRSS_NUM_HFROOT          ((uint32_t)(cy_device->srssNumHfroot))

#define SRSS_PWR_CTL                (((SRSS_V1_Type *) SRSS)->PWR_CTL)
#define SRSS_PWR_HIBERNATE          (((SRSS_V1_Type *) SRSS)->PWR_HIBERNATE)
#define SRSS_PWR_TRIM_PWRSYS_CTL    (((SRSS_V1_Type *) SRSS)->PWR_TRIM_PWRSYS_CTL)
#define SRSS_PWR_BUCK_CTL           (((SRSS_V1_Type *) SRSS)->PWR_BUCK_CTL)
#define SRSS_PWR_BUCK_CTL2          (((SRSS_V1_Type *) SRSS)->PWR_BUCK_CTL2)
#define SRSS_PWR_TRIM_WAKE_CTL      (((SRSS_V1_Type *) SRSS)->PWR_TRIM_WAKE_CTL)
#define SRSS_PWR_LVD_CTL            (((SRSS_V1_Type *) SRSS)->PWR_LVD_CTL)
#define SRSS_PWR_LVD_STATUS         (((SRSS_V1_Type *) SRSS)->PWR_LVD_STATUS)
#define SRSS_WDT_CTL                (((SRSS_V1_Type *) SRSS)->WDT_CTL)
#define SRSS_WDT_CNT                (((SRSS_V1_Type *) SRSS)->WDT_CNT)
#define SRSS_WDT_MATCH              (((SRSS_V1_Type *) SRSS)->WDT_MATCH)
#define SRSS_CLK_DSI_SELECT         (((SRSS_V1_Type *) SRSS)->CLK_DSI_SELECT)
#define SRSS_CLK_PATH_SELECT        (((SRSS_V1_Type *) SRSS)->CLK_PATH_SELECT)
#define SRSS_CLK_ROOT_SELECT        (((SRSS_V1_Type *) SRSS)->CLK_ROOT_SELECT)
#define SRSS_CLK_CSV_HF_LIMIT(clk)  (((SRSS_V1_Type *) SRSS)->CLK_CSV[(clk)].HF_LIMIT)
#define SRSS_CLK_CSV_HF_CTL(clk)    (((SRSS_V1_Type *) SRSS)->CLK_CSV[(clk)].HF_CTL)
#define SRSS_CLK_SELECT             (((SRSS_V1_Type *) SRSS)->CLK_SELECT)
#define SRSS_CLK_TIMER_CTL          (((SRSS_V1_Type *) SRSS)->CLK_TIMER_CTL)
#define SRSS_CLK_CSV_WCO_CTL        (((SRSS_V1_Type *) SRSS)->CLK_CSV_WCO_CTL)
#define SRSS_CLK_ILO_CONFIG         (((SRSS_V1_Type *) SRSS)->CLK_ILO_CONFIG)
#define SRSS_CLK_OUTPUT_SLOW        (((SRSS_V1_Type *) SRSS)->CLK_OUTPUT_SLOW)
#define SRSS_CLK_OUTPUT_FAST        (((SRSS_V1_Type *) SRSS)->CLK_OUTPUT_FAST)
#define SRSS_CLK_CAL_CNT1           (((SRSS_V1_Type *) SRSS)->CLK_CAL_CNT1)
#define SRSS_CLK_CAL_CNT2           (((SRSS_V1_Type *) SRSS)->CLK_CAL_CNT2)
#define SRSS_CLK_ECO_CONFIG         (((SRSS_V1_Type *) SRSS)->CLK_ECO_CONFIG)
#define SRSS_CLK_ECO_STATUS         (((SRSS_V1_Type *) SRSS)->CLK_ECO_STATUS)
#define SRSS_CLK_PILO_CONFIG        (((SRSS_V1_Type *) SRSS)->CLK_PILO_CONFIG)
#define SRSS_CLK_FLL_CONFIG         (((SRSS_V1_Type *) SRSS)->CLK_FLL_CONFIG)
#define SRSS_CLK_FLL_CONFIG2        (((SRSS_V1_Type *) SRSS)->CLK_FLL_CONFIG2)
#define SRSS_CLK_FLL_CONFIG3        (((SRSS_V1_Type *) SRSS)->CLK_FLL_CONFIG3)
#define SRSS_CLK_FLL_CONFIG4        (((SRSS_V1_Type *) SRSS)->CLK_FLL_CONFIG4)
#define SRSS_CLK_FLL_STATUS         (((SRSS_V1_Type *) SRSS)->CLK_FLL_STATUS)
#define SRSS_CLK_PLL_CONFIG         (((SRSS_V1_Type *) SRSS)->CLK_PLL_CONFIG)
#define SRSS_CLK_PLL_STATUS         (((SRSS_V1_Type *) SRSS)->CLK_PLL_STATUS)
#define SRSS_SRSS_INTR              (((SRSS_V1_Type *) SRSS)->SRSS_INTR)
#define SRSS_SRSS_INTR_SET          (((SRSS_V1_Type *) SRSS)->SRSS_INTR_SET)
#define SRSS_SRSS_INTR_CFG          (((SRSS_V1_Type *) SRSS)->SRSS_INTR_CFG)
#define SRSS_SRSS_INTR_MASK         (((SRSS_V1_Type *) SRSS)->SRSS_INTR_MASK)
#define SRSS_SRSS_INTR_MASKED       (((SRSS_V1_Type *) SRSS)->SRSS_INTR_MASKED)
#define SRSS_CLK_TRIM_ILO_CTL       (((SRSS_V1_Type *) SRSS)->CLK_TRIM_ILO_CTL)
#define SRSS_CLK_TRIM_ECO_CTL       (((SRSS_V1_Type *) SRSS)->CLK_TRIM_ECO_CTL)


/*******************************************************************************
*                BACKUP
*******************************************************************************/

#define BACKUP_PMIC_CTL             (((BACKUP_V1_Type *) BACKUP)->PMIC_CTL)
#define BACKUP_CTL                  (((BACKUP_V1_Type *) BACKUP)->CTL)
#define BACKUP_RTC_TIME             (((BACKUP_V1_Type *) BACKUP)->RTC_TIME)
#define BACKUP_RTC_DATE             (((BACKUP_V1_Type *) BACKUP)->RTC_DATE)
#define BACKUP_RTC_RW               (((BACKUP_V1_Type *) BACKUP)->RTC_RW)
#define BACKUP_ALM1_TIME            (((BACKUP_V1_Type *) BACKUP)->ALM1_TIME)
#define BACKUP_ALM1_DATE            (((BACKUP_V1_Type *) BACKUP)->ALM1_DATE)
#define BACKUP_ALM2_TIME            (((BACKUP_V1_Type *) BACKUP)->ALM2_TIME)
#define BACKUP_ALM2_DATE            (((BACKUP_V1_Type *) BACKUP)->ALM2_DATE)
#define BACKUP_STATUS               (((BACKUP_V1_Type *) BACKUP)->STATUS)
#define BACKUP_INTR                 (((BACKUP_V1_Type *) BACKUP)->INTR)
#define BACKUP_INTR_SET             (((BACKUP_V1_Type *) BACKUP)->INTR_SET)
#define BACKUP_INTR_MASK            (((BACKUP_V1_Type *) BACKUP)->INTR_MASK)
#define BACKUP_INTR_MASKED          (((BACKUP_V1_Type *) BACKUP)->INTR_MASKED)

#define FLASHC_FM_CTL_ANA_CTL0         (((FLASHC_V1_Type *) cy_device->flashcBase)->FM_CTL.ANA_CTL0)
#define DELAY_DONE_FLAG                (((FLASHC_V1_Type *) cy_device->flashcBase)->BIST_DATA[0U])
#define FLASHC_ACQUIRE(base)           (((FLASHC_V2_Type *)(base))->ACQUIRE)

#define SFLASH_PWR_TRIM_WAKE_CTL       (((SFLASH_V1_Type *) SFLASH)->PWR_TRIM_WAKE_CTL)
#define SFLASH_LDO_0P9V_TRIM           (((SFLASH_V1_Type *) SFLASH)->LDO_0P9V_TRIM)
#define SFLASH_LDO_1P1V_TRIM           (((SFLASH_V1_Type *) SFLASH)->LDO_1P1V_TRIM)
#define SFLASH_TRIM_ROM_CTL_LP         (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_ROM_CTL_LP)
#define SFLASH_TRIM_RAM_CTL_LP         (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_RAM_CTL_LP)
#define SFLASH_TRIM_ROM_CTL_ULP        (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_ROM_CTL_ULP)
#define SFLASH_TRIM_RAM_CTL_ULP        (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_RAM_CTL_ULP)
#define SFLASH_TRIM_ROM_CTL_HALF_LP    (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_ROM_CTL_HALF_LP)
#define SFLASH_TRIM_ROM_CTL_HALF_LP    (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_ROM_CTL_HALF_LP)
#define SFLASH_TRIM_RAM_CTL_HALF_ULP   (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_RAM_CTL_HALF_ULP)
#define SFLASH_TRIM_ROM_CTL_HALF_ULP   (((SFLASH_V1_Type *) SFLASH)->CPUSS_TRIM_ROM_CTL_HALF_ULP)


/*******************************************************************************
*                CPUSS
*******************************************************************************/

#define CY_CPUSS_V1            (1U == cy_device->cpussVersion)

#define CPUSS_CM0_CLOCK_CTL    (*(volatile uint32_t *) (cy_device->cpussBase + cy_device->cpussCm0ClockCtlOffset))
#define CPUSS_CM4_CLOCK_CTL    (*(volatile uint32_t *) (cy_device->cpussBase + cy_device->cpussCm4ClockCtlOffset))
#define CPUSS_CM4_STATUS       (*(volatile uint32_t *) (cy_device->cpussBase + cy_device->cpussCm4StatusOffset))
#define CPUSS_CM0_STATUS       (*(volatile uint32_t *) (cy_device->cpussBase + cy_device->cpussCm0StatusOffset))
#define CPUSS_CM4_PWR_CTL      (*(volatile uint32_t *) (cy_device->cpussBase + cy_device->cpussCm4PwrCtlOffset))
#define CPUSS_TRIM_RAM_CTL     (*(volatile uint32_t *) (cy_device->cpussBase + cy_device->cpussTrimRamCtlOffset))
#define CPUSS_TRIM_ROM_CTL     (*(volatile uint32_t *) (cy_device->cpussBase + cy_device->cpussTrimRomCtlOffset))

#define CY_CPUSS_NMI_CTL(nmi)  ((cy_device->cpussBase + ((nmi) * sizeof(uint32_t) * (cy_device->cpussVersion - 1U))))
#define CPUSS_CM0_NMI_CTL(nmi) (*(volatile uint32_t *) (CY_CPUSS_NMI_CTL(nmi) + (uint32_t)cy_device->cpussCm0NmiCtlOffset))
#define CPUSS_CM4_NMI_CTL(nmi) (*(volatile uint32_t *) (CY_CPUSS_NMI_CTL(nmi) + (uint32_t)cy_device->cpussCm4NmiCtlOffset))

/* used in V1 code only */
#define CPUSS_CM0_INT_CTL       ((volatile uint32_t *) &(((CPUSS_V1_Type *)(cy_device->cpussBase))->CM0_INT_CTL0))

/* used in V2 code only */
#define CPUSS_CM0_SYSTEM_INT_CTL (((CPUSS_V2_Type *)(cy_device->cpussBase))->CM0_SYSTEM_INT_CTL)
#define CPUSS_CM0_INT_STATUS   ((volatile uint32_t *) &(((CPUSS_V2_Type *)(cy_device->cpussBase))->CM0_INT0_STATUS))

/* ARM core address which should not be changed */
#define SYSTICK_CTRL     (((SysTick_Type *)SysTick)->CTRL)
#define SYSTICK_LOAD     (((SysTick_Type *)SysTick)->LOAD)
#define SYSTICK_VAL      (((SysTick_Type *)SysTick)->VAL)
#define SCB_SCR          (((SCB_Type *)SCB)->SCR)

#define SYSPM_IPC_STC_DATA       (((IPC_V1_Type *) IPC)->STRUCT[7].DATA)
#define SYSPM_IPC_STC_ACQUIRE    (((IPC_V1_Type *) IPC)->STRUCT[7].ACQUIRE)
#define SYSPM_IPC_STC_RELEASE    (((IPC_V1_Type *) IPC)->STRUCT[7].RELEASE)

#define UDB_UDBIF_BANK_CTL      (((UDB_V1_Type *) cy_device->udbBase)->UDBIF.BANK_CTL)
#define UDB_BCTL_MDCLK_EN       (((UDB_V1_Type *) cy_device->udbBase)->BCTL.MDCLK_EN)
#define UDB_BCTL_MBCLK_EN       (((UDB_V1_Type *) cy_device->udbBase)->BCTL.MBCLK_EN)
#define UDB_BCTL_BOTSEL_L       (((UDB_V1_Type *) cy_device->udbBase)->BCTL.BOTSEL_L)
#define UDB_BCTL_BOTSEL_U       (((UDB_V1_Type *) cy_device->udbBase)->BCTL.BOTSEL_U)
#define UDB_BCTL_QCLK_EN_0      (((UDB_V1_Type *) cy_device->udbBase)->BCTL.QCLK_EN[0U])
#define UDB_BCTL_QCLK_EN_1      (((UDB_V1_Type *) cy_device->udbBase)->BCTL.QCLK_EN[1U])
#define UDB_BCTL_QCLK_EN_2      (((UDB_V1_Type *) cy_device->udbBase)->BCTL.QCLK_EN[2U])


/*******************************************************************************
*                LPCOMP
*******************************************************************************/

#define LPCOMP_CMP0_CTRL(base)         (((LPCOMP_V1_Type *)(base))->CMP0_CTRL)
#define LPCOMP_CMP1_CTRL(base)         (((LPCOMP_V1_Type *)(base))->CMP1_CTRL)
#define LPCOMP_CMP0_SW_CLEAR(base)     (((LPCOMP_V1_Type *)(base))->CMP0_SW_CLEAR)
#define LPCOMP_CMP1_SW_CLEAR(base)     (((LPCOMP_V1_Type *)(base))->CMP1_SW_CLEAR)
#define LPCOMP_CMP0_SW(base)           (((LPCOMP_V1_Type *)(base))->CMP0_SW)
#define LPCOMP_CMP1_SW(base)           (((LPCOMP_V1_Type *)(base))->CMP1_SW)
#define LPCOMP_STATUS(base)            (((LPCOMP_V1_Type *)(base))->STATUS)
#define LPCOMP_CONFIG(base)            (((LPCOMP_V1_Type *)(base))->CONFIG)
#define LPCOMP_INTR(base)              (((LPCOMP_V1_Type *)(base))->INTR)
#define LPCOMP_INTR_SET(base)          (((LPCOMP_V1_Type *)(base))->INTR_SET)
#define LPCOMP_INTR_MASK(base)         (((LPCOMP_V1_Type *)(base))->INTR_MASK)
#define LPCOMP_INTR_MASKED(base)       (((LPCOMP_V1_Type *)(base))->INTR_MASKED)

/* Has two HSIOM versions */
#define HSIOM_AMUX_SPLIT_CTL_3         (((HSIOM_V1_Type *) cy_device->hsiomBase)->AMUX_SPLIT_CTL[3])


/*******************************************************************************
*                MCWDT
*******************************************************************************/

#define MCWDT_STRUCT_MCWDT_CNTLOW(base)         (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_CNTLOW)
#define MCWDT_STRUCT_MCWDT_CNTHIGH(base)        (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_CNTHIGH)
#define MCWDT_STRUCT_MCWDT_MATCH(base)          (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_MATCH)
#define MCWDT_STRUCT_MCWDT_CONFIG(base)         (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_CONFIG)
#define MCWDT_STRUCT_MCWDT_LOCK(base)           (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_LOCK)
#define MCWDT_STRUCT_MCWDT_CTL(base)            (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_CTL)
#define MCWDT_STRUCT_MCWDT_INTR(base)           (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_INTR)
#define MCWDT_STRUCT_MCWDT_INTR_SET(base)       (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_INTR_SET)
#define MCWDT_STRUCT_MCWDT_INTR_MASK(base)      (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_INTR_MASK)
#define MCWDT_STRUCT_MCWDT_INTR_MASKED(base)    (((MCWDT_STRUCT_V1_Type *)(base))->MCWDT_INTR_MASKED)


/*******************************************************************************
*                TCPWM
*******************************************************************************/

#define TCPWM_CTRL_SET(base)                (((TCPWM_V1_Type *)(base))->CTRL_SET)
#define TCPWM_CTRL_CLR(base)                (((TCPWM_V1_Type *)(base))->CTRL_CLR)
#define TCPWM_CMD_START(base)               (((TCPWM_V1_Type *)(base))->CMD_START)
#define TCPWM_CMD_RELOAD(base)              (((TCPWM_V1_Type *)(base))->CMD_RELOAD)
#define TCPWM_CMD_STOP(base)                (((TCPWM_V1_Type *)(base))->CMD_STOP)
#define TCPWM_CMD_CAPTURE(base)             (((TCPWM_V1_Type *)(base))->CMD_CAPTURE)

#define TCPWM_CNT_CTRL(base, cntNum)         (((TCPWM_V1_Type *)(base))->CNT[cntNum].CTRL)
#define TCPWM_CNT_CC(base, cntNum)           (((TCPWM_V1_Type *)(base))->CNT[cntNum].CC)
#define TCPWM_CNT_CC_BUFF(base, cntNum)      (((TCPWM_V1_Type *)(base))->CNT[cntNum].CC_BUFF)
#define TCPWM_CNT_COUNTER(base, cntNum)      (((TCPWM_V1_Type *)(base))->CNT[cntNum].COUNTER)
#define TCPWM_CNT_PERIOD(base, cntNum)       (((TCPWM_V1_Type *)(base))->CNT[cntNum].PERIOD)
#define TCPWM_CNT_PERIOD_BUFF(base, cntNum)  (((TCPWM_V1_Type *)(base))->CNT[cntNum].PERIOD_BUFF)
#define TCPWM_CNT_STATUS(base, cntNum)       (((TCPWM_V1_Type *)(base))->CNT[cntNum].STATUS)
#define TCPWM_CNT_INTR(base, cntNum)         (((TCPWM_V1_Type *)(base))->CNT[cntNum].INTR)
#define TCPWM_CNT_INTR_SET(base, cntNum)     (((TCPWM_V1_Type *)(base))->CNT[cntNum].INTR_SET)
#define TCPWM_CNT_INTR_MASK(base, cntNum)    (((TCPWM_V1_Type *)(base))->CNT[cntNum].INTR_MASK)
#define TCPWM_CNT_INTR_MASKED(base, cntNum)  (((TCPWM_V1_Type *)(base))->CNT[cntNum].INTR_MASKED)
#define TCPWM_CNT_TR_CTRL0(base, cntNum)     (((TCPWM_V1_Type *)(base))->CNT[cntNum].TR_CTRL0)
#define TCPWM_CNT_TR_CTRL1(base, cntNum)     (((TCPWM_V1_Type *)(base))->CNT[cntNum].TR_CTRL1)
#define TCPWM_CNT_TR_CTRL2(base, cntNum)     (((TCPWM_V1_Type *)(base))->CNT[cntNum].TR_CTRL2)


/*******************************************************************************
*                SDHC
*******************************************************************************/

#define SDHC_WRAP_CTL(base)                     (((SDHC_V1_Type *)(base))->WRAP.CTL)
#define SDHC_CORE_SDMASA_R(base)                (((SDHC_V1_Type *)(base))->CORE.SDMASA_R)
#define SDHC_CORE_BLOCKSIZE_R(base)             (((SDHC_V1_Type *)(base))->CORE.BLOCKSIZE_R)
#define SDHC_CORE_BLOCKCOUNT_R(base)            (((SDHC_V1_Type *)(base))->CORE.BLOCKCOUNT_R)
#define SDHC_CORE_ARGUMENT_R(base)              (((SDHC_V1_Type *)(base))->CORE.ARGUMENT_R)
#define SDHC_CORE_XFER_MODE_R(base)             (((SDHC_V1_Type *)(base))->CORE.XFER_MODE_R)
#define SDHC_CORE_CMD_R(base)                   (((SDHC_V1_Type *)(base))->CORE.CMD_R)
#define SDHC_CORE_RESP01_R(base)                (((SDHC_V1_Type *)(base))->CORE.RESP01_R)
#define SDHC_CORE_RESP23_R(base)                (((SDHC_V1_Type *)(base))->CORE.RESP23_R)
#define SDHC_CORE_RESP45_R(base)                (((SDHC_V1_Type *)(base))->CORE.RESP45_R)
#define SDHC_CORE_RESP67_R(base)                (((SDHC_V1_Type *)(base))->CORE.RESP67_R)
#define SDHC_CORE_BUF_DATA_R(base)              (((SDHC_V1_Type *)(base))->CORE.BUF_DATA_R)
#define SDHC_CORE_PSTATE_REG(base)              (((SDHC_V1_Type *)(base))->CORE.PSTATE_REG)
#define SDHC_CORE_HOST_CTRL1_R(base)            (((SDHC_V1_Type *)(base))->CORE.HOST_CTRL1_R)
#define SDHC_CORE_PWR_CTRL_R(base)              (((SDHC_V1_Type *)(base))->CORE.PWR_CTRL_R)
#define SDHC_CORE_BGAP_CTRL_R(base)             (((SDHC_V1_Type *)(base))->CORE.BGAP_CTRL_R)
#define SDHC_CORE_WUP_CTRL_R(base)              (((SDHC_V1_Type *)(base))->CORE.WUP_CTRL_R)
#define SDHC_CORE_CLK_CTRL_R(base)              (((SDHC_V1_Type *)(base))->CORE.CLK_CTRL_R)
#define SDHC_CORE_TOUT_CTRL_R(base)             (((SDHC_V1_Type *)(base))->CORE.TOUT_CTRL_R)
#define SDHC_CORE_SW_RST_R(base)                (((SDHC_V1_Type *)(base))->CORE.SW_RST_R)
#define SDHC_CORE_NORMAL_INT_STAT_R(base)       (((SDHC_V1_Type *)(base))->CORE.NORMAL_INT_STAT_R)
#define SDHC_CORE_ERROR_INT_STAT_R(base)        (((SDHC_V1_Type *)(base))->CORE.ERROR_INT_STAT_R)
#define SDHC_CORE_NORMAL_INT_STAT_EN_R(base)    (((SDHC_V1_Type *)(base))->CORE.NORMAL_INT_STAT_EN_R)
#define SDHC_CORE_ERROR_INT_STAT_EN_R(base)     (((SDHC_V1_Type *)(base))->CORE.ERROR_INT_STAT_EN_R)
#define SDHC_CORE_NORMAL_INT_SIGNAL_EN_R(base)  (((SDHC_V1_Type *)(base))->CORE.NORMAL_INT_SIGNAL_EN_R)
#define SDHC_CORE_ERROR_INT_SIGNAL_EN_R(base)   (((SDHC_V1_Type *)(base))->CORE.ERROR_INT_SIGNAL_EN_R)
#define SDHC_CORE_AUTO_CMD_STAT_R(base)         (((SDHC_V1_Type *)(base))->CORE.AUTO_CMD_STAT_R)
#define SDHC_CORE_HOST_CTRL2_R(base)            (((SDHC_V1_Type *)(base))->CORE.HOST_CTRL2_R)
#define SDHC_CORE_CAPABILITIES1_R(base)         (((SDHC_V1_Type *)(base))->CORE.CAPABILITIES1_R)
#define SDHC_CORE_CAPABILITIES2_R(base)         (((SDHC_V1_Type *)(base))->CORE.CAPABILITIES2_R)
#define SDHC_CORE_CURR_CAPABILITIES1_R(base)    (((SDHC_V1_Type *)(base))->CORE.CURR_CAPABILITIES1_R)
#define SDHC_CORE_CURR_CAPABILITIES2_R(base)    (((SDHC_V1_Type *)(base))->CORE.CURR_CAPABILITIES2_R)
#define SDHC_CORE_ADMA_ERR_STAT_R(base)         (((SDHC_V1_Type *)(base))->CORE.ADMA_ERR_STAT_R)
#define SDHC_CORE_ADMA_SA_LOW_R(base)           (((SDHC_V1_Type *)(base))->CORE.ADMA_SA_LOW_R)
#define SDHC_CORE_ADMA_ID_LOW_R(base)           (((SDHC_V1_Type *)(base))->CORE.ADMA_ID_LOW_R)
#define SDHC_CORE_EMMC_CTRL_R(base)             (((SDHC_V1_Type *)(base))->CORE.EMMC_CTRL_R)
#define SDHC_CORE_GP_OUT_R(base)                (((SDHC_V1_Type *)(base))->CORE.GP_OUT_R)


/*******************************************************************************
*                SMARTIO
*******************************************************************************/
#define SMARTIO_PRT_CTL(base)             (((SMARTIO_PRT_V1_Type *)(base))->CTL)
#define SMARTIO_PRT_SYNC_CTL(base)        (((SMARTIO_PRT_V1_Type *)(base))->SYNC_CTL)
#define SMARTIO_PRT_LUT_SEL(base, idx)    (((SMARTIO_PRT_V1_Type *)(base))->LUT_SEL[idx])
#define SMARTIO_PRT_LUT_CTL(base, idx)    (((SMARTIO_PRT_V1_Type *)(base))->LUT_CTL[idx])
#define SMARTIO_PRT_DU_SEL(base)          (((SMARTIO_PRT_V1_Type *)(base))->DU_SEL)
#define SMARTIO_PRT_DU_CTL(base)          (((SMARTIO_PRT_V1_Type *)(base))->DU_CTL)
#define SMARTIO_PRT_DATA(base)            (((SMARTIO_PRT_V1_Type *)(base))->DATA)


/*******************************************************************************
*                SMIF
*******************************************************************************/

#define SMIF_DEVICE_CTL(base)             (((SMIF_DEVICE_V1_Type *)(base))->CTL)
#define SMIF_DEVICE_ADDR(base)            (((SMIF_DEVICE_V1_Type *)(base))->ADDR)
#define SMIF_DEVICE_ADDR_CTL(base)        (((SMIF_DEVICE_V1_Type *)(base))->ADDR_CTL)
#define SMIF_DEVICE_MASK(base)            (((SMIF_DEVICE_V1_Type *)(base))->MASK)
#define SMIF_DEVICE_RD_CMD_CTL(base)      (((SMIF_DEVICE_V1_Type *)(base))->RD_CMD_CTL)
#define SMIF_DEVICE_RD_ADDR_CTL(base)     (((SMIF_DEVICE_V1_Type *)(base))->RD_ADDR_CTL)
#define SMIF_DEVICE_RD_MODE_CTL(base)     (((SMIF_DEVICE_V1_Type *)(base))->RD_MODE_CTL)
#define SMIF_DEVICE_RD_DUMMY_CTL(base)    (((SMIF_DEVICE_V1_Type *)(base))->RD_DUMMY_CTL)
#define SMIF_DEVICE_RD_DATA_CTL(base)     (((SMIF_DEVICE_V1_Type *)(base))->RD_DATA_CTL)
#define SMIF_DEVICE_WR_CMD_CTL(base)      (((SMIF_DEVICE_V1_Type *)(base))->WR_CMD_CTL)
#define SMIF_DEVICE_WR_ADDR_CTL(base)     (((SMIF_DEVICE_V1_Type *)(base))->WR_ADDR_CTL)
#define SMIF_DEVICE_WR_MODE_CTL(base)     (((SMIF_DEVICE_V1_Type *)(base))->WR_MODE_CTL)
#define SMIF_DEVICE_WR_DUMMY_CTL(base)    (((SMIF_DEVICE_V1_Type *)(base))->WR_DUMMY_CTL)
#define SMIF_DEVICE_WR_DATA_CTL(base)     (((SMIF_DEVICE_V1_Type *)(base))->WR_DATA_CTL)

#define SMIF_DEVICE_IDX(base, deviceIndex)                 (((SMIF_V1_Type *)(base))->DEVICE[deviceIndex])

#define SMIF_DEVICE_IDX_CTL(base, deviceIndex)             (SMIF_DEVICE_IDX(base, deviceIndex).CTL)
#define SMIF_DEVICE_IDX_ADDR(base, deviceIndex)            (SMIF_DEVICE_IDX(base, deviceIndex).ADDR)
#define SMIF_DEVICE_IDX_ADDR_CTL(base, deviceIndex)        (SMIF_DEVICE_IDX(base, deviceIndex).ADDR_CTL)
#define SMIF_DEVICE_IDX_MASK(base, deviceIndex)            (SMIF_DEVICE_IDX(base, deviceIndex).MASK)
#define SMIF_DEVICE_IDX_RD_CMD_CTL(base, deviceIndex)      (SMIF_DEVICE_IDX(base, deviceIndex).RD_CMD_CTL)
#define SMIF_DEVICE_IDX_RD_ADDR_CTL(base, deviceIndex)     (SMIF_DEVICE_IDX(base, deviceIndex).RD_ADDR_CTL)
#define SMIF_DEVICE_IDX_RD_MODE_CTL(base, deviceIndex)     (SMIF_DEVICE_IDX(base, deviceIndex).RD_MODE_CTL)
#define SMIF_DEVICE_IDX_RD_DUMMY_CTL(base, deviceIndex)    (SMIF_DEVICE_IDX(base, deviceIndex).RD_DUMMY_CTL)
#define SMIF_DEVICE_IDX_RD_DATA_CTL(base, deviceIndex)     (SMIF_DEVICE_IDX(base, deviceIndex).RD_DATA_CTL)
#define SMIF_DEVICE_IDX_WR_CMD_CTL(base, deviceIndex)      (SMIF_DEVICE_IDX(base, deviceIndex).WR_CMD_CTL)
#define SMIF_DEVICE_IDX_WR_ADDR_CTL(base, deviceIndex)     (SMIF_DEVICE_IDX(base, deviceIndex).WR_ADDR_CTL)
#define SMIF_DEVICE_IDX_WR_MODE_CTL(base, deviceIndex)     (SMIF_DEVICE_IDX(base, deviceIndex).WR_MODE_CTL)
#define SMIF_DEVICE_IDX_WR_DUMMY_CTL(base, deviceIndex)    (SMIF_DEVICE_IDX(base, deviceIndex).WR_DUMMY_CTL)
#define SMIF_DEVICE_IDX_WR_DATA_CTL(base, deviceIndex)     (SMIF_DEVICE_IDX(base, deviceIndex).WR_DATA_CTL)

#define SMIF_CTL(base)                    (((SMIF_V1_Type *)(base))->CTL)
#define SMIF_STATUS(base)                 (((SMIF_V1_Type *)(base))->STATUS)
#define SMIF_TX_DATA_FIFO_CTL(base)       (((SMIF_V1_Type *)(base))->TX_DATA_FIFO_CTL)
#define SMIF_RX_DATA_FIFO_CTL(base)       (((SMIF_V1_Type *)(base))->RX_DATA_FIFO_CTL)
#define SMIF_TX_DATA_FIFO_WR1(base)       (((SMIF_V1_Type *)(base))->TX_DATA_FIFO_WR1)
#define SMIF_TX_DATA_FIFO_WR2(base)       (((SMIF_V1_Type *)(base))->TX_DATA_FIFO_WR2)
#define SMIF_TX_DATA_FIFO_WR4(base)       (((SMIF_V1_Type *)(base))->TX_DATA_FIFO_WR4)
#define SMIF_RX_DATA_FIFO_RD1(base)       (((SMIF_V1_Type *)(base))->RX_DATA_FIFO_RD1)
#define SMIF_RX_DATA_FIFO_RD2(base)       (((SMIF_V1_Type *)(base))->RX_DATA_FIFO_RD2)
#define SMIF_RX_DATA_FIFO_RD4(base)       (((SMIF_V1_Type *)(base))->RX_DATA_FIFO_RD4)
#define SMIF_TX_CMD_FIFO_WR(base)         (((SMIF_V1_Type *)(base))->TX_CMD_FIFO_WR)
#define SMIF_TX_CMD_FIFO_STATUS(base)     (((SMIF_V1_Type *)(base))->TX_CMD_FIFO_STATUS)
#define SMIF_RX_DATA_FIFO_STATUS(base)    (((SMIF_V1_Type *)(base))->RX_DATA_FIFO_STATUS)
#define SMIF_TX_DATA_FIFO_STATUS(base)    (((SMIF_V1_Type *)(base))->TX_DATA_FIFO_STATUS)
#define SMIF_INTR(base)                   (((SMIF_V1_Type *)(base))->INTR)
#define SMIF_INTR_SET(base)               (((SMIF_V1_Type *)(base))->INTR_SET)
#define SMIF_INTR_MASK(base)              (((SMIF_V1_Type *)(base))->INTR_MASK)
#define SMIF_INTR_MASKED(base)            (((SMIF_V1_Type *)(base))->INTR_MASKED)
#define SMIF_CRYPTO_INPUT0(base)          (((SMIF_V1_Type *)(base))->CRYPTO_INPUT0)
#define SMIF_CRYPTO_OUTPUT0(base)         (((SMIF_V1_Type *)(base))->CRYPTO_OUTPUT0)
#define SMIF_CRYPTO_OUTPUT1(base)         (((SMIF_V1_Type *)(base))->CRYPTO_OUTPUT1)
#define SMIF_CRYPTO_OUTPUT2(base)         (((SMIF_V1_Type *)(base))->CRYPTO_OUTPUT2)
#define SMIF_CRYPTO_OUTPUT3(base)         (((SMIF_V1_Type *)(base))->CRYPTO_OUTPUT3)
#define SMIF_CRYPTO_CMD(base)             (((SMIF_V1_Type *)(base))->CRYPTO_CMD)
#define SMIF_SLOW_CA_CTL(base)            (((SMIF_V1_Type *)(base))->SLOW_CA_CTL)
#define SMIF_FAST_CA_CTL(base)            (((SMIF_V1_Type *)(base))->FAST_CA_CTL)
#define SMIF_SLOW_CA_CMD(base)            (((SMIF_V1_Type *)(base))->SLOW_CA_CMD)
#define SMIF_FAST_CA_CMD(base)            (((SMIF_V1_Type *)(base))->FAST_CA_CMD)


/*******************************************************************************
*                DW
*******************************************************************************/

#define CY_DW_V1                      (1U == cy_device->dwVersion)
#define CY_DW_CRC                     (1U < cy_device->dwVersion)
#define CY_DW_CH_NR                   (cy_device->cpussDwChNr)

#define DW_CTL(base)                  (((DW_V1_Type*)(base))->CTL)
#define DW_STATUS(base)               (((DW_V1_Type*)(base))->STATUS)
#define DW_DESCR_SRC(base)            (((DW_V1_Type*)(base))->ACT_DESCR_SRC)
#define DW_DESCR_DST(base)            (((DW_V1_Type*)(base))->ACT_DESCR_DST)

#define DW_CRC_CTL(base)              (((DW_V2_Type*)(base))->CRC_CTL)
#define DW_CRC_DATA_CTL(base)         (((DW_V2_Type*)(base))->CRC_DATA_CTL)
#define DW_CRC_REM_CTL(base)          (((DW_V2_Type*)(base))->CRC_REM_CTL)
#define DW_CRC_POL_CTL(base)          (((DW_V2_Type*)(base))->CRC_POL_CTL)

#define DW_CH(base, chan)             ((DW_CH_STRUCT_V2_Type *)((uint32_t)(base) + cy_device->dwChOffset + ((chan) * cy_device->dwChSize)))
#define DW_CH_CTL(base, chan)         (DW_CH(base, chan)->CH_CTL)
#define DW_CH_STATUS(base, chan)      (DW_CH(base, chan)->CH_STATUS)
#define DW_CH_IDX(base, chan)         (DW_CH(base, chan)->CH_IDX)
#define DW_CH_CURR_PTR(base, chan)    (DW_CH(base, chan)->CH_CURR_PTR)

#define DW_CH_INTR(base, chan)        (DW_CH(base, chan)->INTR)
#define DW_CH_INTR_SET(base, chan)    (DW_CH(base, chan)->INTR_SET)
#define DW_CH_INTR_MASK(base, chan)   (DW_CH(base, chan)->INTR_MASK)
#define DW_CH_INTR_MASKED(base, chan) (DW_CH(base, chan)->INTR_MASKED)


/*******************************************************************************
*                DMAC
*******************************************************************************/

#define CY_DMAC_CH_NR                   (4UL)
#define DMAC_CTL(base)                  (((DMAC_V2_Type*)(base))->CTL)
#define DMAC_ACTIVE(base)               (((DMAC_V2_Type*)(base))->ACTIVE)
#define DMAC_CH(base, chan)             (((DMAC_V2_Type*)(base))->CH[(chan)])
#define DMAC_CH_CTL(base, chan)         (DMAC_CH(base, chan).CTL)
#define DMAC_CH_CURR(base, chan)        (DMAC_CH(base, chan).CURR)
#define DMAC_CH_INTR(base, chan)        (DMAC_CH(base, chan).INTR)
#define DMAC_CH_INTR_SET(base, chan)    (DMAC_CH(base, chan).INTR_SET)
#define DMAC_CH_INTR_MASK(base, chan)   (DMAC_CH(base, chan).INTR_MASK)
#define DMAC_CH_INTR_MASKED(base, chan) (DMAC_CH(base, chan).INTR_MASKED)


/*******************************************************************************
*                PERI
*******************************************************************************/

#define CY_PERI_V1                          (1U == cy_device->periVersion) /* true if the mxperi version is 1 */
#define CY_PERI_V2_TR_GR_SIZE               (sizeof(PERI_TR_GR_V2_Type))
#define CY_PERI_TR_CTL_NUM                  (cy_device->periTrGrSize / sizeof(uint32_t))
#define CY_PERI_TR_CTL_SEL_Pos              (0UL)
#define CY_PERI_TR_CTL_SEL_Msk              ((uint32_t)CY_PERI_TR_CTL_NUM - 1UL)
#define CY_PERI_TR_CMD_GROUP_SEL_Pos        (PERI_TR_CMD_GROUP_SEL_Pos)
#define CY_PERI_TR_CMD_GROUP_Msk            (PERI_TR_CMD_GROUP_SEL_Msk >> CY_PERI_TR_CMD_GROUP_SEL_Pos)
#define CY_PERI_TR_CMD_GROUP_SEL_Msk        ((uint32_t)cy_device->periTrCmdGrSelMsk)
#define CY_PERI_1_TO_1_TR_CMD_GROUP_SEL_Msk (CY_PERI_TR_CMD_GROUP_SEL_Msk & ((uint32_t)~PERI_TR_CMD_GROUP_SEL_Msk))
#define CY_PERI_1_TO_1_TR_CMD_GROUP_Msk     (CY_PERI_1_TO_1_TR_CMD_GROUP_SEL_Msk >> CY_PERI_TR_CMD_GROUP_SEL_Pos)
#define PERI_TR_CMD                         (*(volatile uint32_t*)((uint32_t)cy_device->periBase + \
                                                                   (uint32_t)cy_device->periTrCmdOffset))
#define PERI_TR_GR_TR_CTL(group, trCtl)     (*(volatile uint32_t*)((uint32_t)cy_device->periBase + \
                                                                   (uint32_t)cy_device->periTrGrOffset + \
                                                          (trCtl * (uint32_t)sizeof(uint32_t)) + \
                             ((CY_PERI_TR_CMD_GROUP_Msk & group) * (uint32_t)cy_device->periTrGrSize) + \
                      ((CY_PERI_1_TO_1_TR_CMD_GROUP_Msk & group) * (uint32_t)sizeof(PERI_TR_GR_V2_Type))))


#define CY_PERI_CLOCK_NR                    ((uint32_t)(cy_device->periClockNr))

#define PERI_DIV_CMD                        (((PERI_V1_Type*)(cy_device->periBase))->DIV_CMD)

#define CY_PERI_DIV_CMD_DIV_SEL_Pos         (PERI_DIV_CMD_DIV_SEL_Pos)
#define CY_PERI_DIV_CMD_DIV_SEL_Msk         ((uint32_t)(cy_device->periDivCmdDivSelMsk))
#define CY_PERI_DIV_CMD_TYPE_SEL_Pos        ((uint32_t)(cy_device->periDivCmdTypeSelPos))
#define CY_PERI_DIV_CMD_TYPE_SEL_Msk        ((uint32_t)(0x3UL << CY_PERI_DIV_CMD_TYPE_SEL_Pos))
#define CY_PERI_DIV_CMD_PA_DIV_SEL_Pos      ((uint32_t)(cy_device->periDivCmdPaDivSelPos))
#define CY_PERI_DIV_CMD_PA_DIV_SEL_Msk      ((uint32_t)(CY_PERI_DIV_CMD_DIV_SEL_Msk << CY_PERI_DIV_CMD_PA_DIV_SEL_Pos))
#define CY_PERI_DIV_CMD_PA_TYPE_SEL_Pos     ((uint32_t)(cy_device->periDivCmdPaTypeSelPos))
#define CY_PERI_DIV_CMD_PA_TYPE_SEL_Msk     ((uint32_t)(0x3UL << CY_PERI_DIV_CMD_PA_TYPE_SEL_Pos))

#define PERI_CLOCK_CTL                      (((PERI_V1_Type*)(cy_device->periBase))->CLOCK_CTL)

#define CY_PERI_CLOCK_CTL_DIV_SEL_Pos       (PERI_CLOCK_CTL_DIV_SEL_Pos)
#define CY_PERI_CLOCK_CTL_DIV_SEL_Msk       (CY_PERI_DIV_CMD_DIV_SEL_Msk)
#define CY_PERI_CLOCK_CTL_TYPE_SEL_Pos      (CY_PERI_DIV_CMD_TYPE_SEL_Pos)
#define CY_PERI_CLOCK_CTL_TYPE_SEL_Msk      (CY_PERI_DIV_CMD_TYPE_SEL_Msk)

#define PERI_DIV_8_CTL                      ((volatile uint32_t *)((uint32_t)(cy_device->periBase) + (uint32_t)(cy_device->periDiv8CtlOffset)))
#define PERI_DIV_16_CTL                     ((volatile uint32_t *)((uint32_t)(cy_device->periBase) + (uint32_t)(cy_device->periDiv16CtlOffset)))
#define PERI_DIV_16_5_CTL                   ((volatile uint32_t *)((uint32_t)(cy_device->periBase) + (uint32_t)(cy_device->periDiv16_5CtlOffset)))
#define PERI_DIV_24_5_CTL                   ((volatile uint32_t *)((uint32_t)(cy_device->periBase) + (uint32_t)(cy_device->periDiv24_5CtlOffset)))

#define PERI_GR_SL_CTL(udbGroupNr)          (((PERI_V1_Type*) cy_device->periBase)->GR[udbGroupNr].SL_CTL)

#define PERI_PPU_PR_ADDR0(base)             (((PERI_PPU_PR_V1_Type *) (base))->ADDR0)
#define PERI_PPU_PR_ATT0(base)              (((PERI_PPU_PR_V1_Type *) (base))->ATT0)
#define PERI_PPU_PR_ATT1(base)              (((PERI_PPU_PR_V1_Type *) (base))->ATT1)

#define PERI_PPU_GR_ADDR0(base)             (((PERI_PPU_GR_V1_Type *) (base))->ADDR0)
#define PERI_PPU_GR_ATT0(base)              (((PERI_PPU_GR_V1_Type *) (base))->ATT0)
#define PERI_PPU_GR_ATT1(base)              (((PERI_PPU_GR_V1_Type *) (base))->ATT1)

#define PERI_GR_PPU_SL_ADDR0(base)          (((PERI_GR_PPU_SL_V1_Type *) (base))->ADDR0)
#define PERI_GR_PPU_SL_ATT0(base)           (((PERI_GR_PPU_SL_V1_Type *) (base))->ATT0)
#define PERI_GR_PPU_SL_ATT1(base)           (((PERI_GR_PPU_SL_V1_Type *) (base))->ATT1)

#define PERI_GR_PPU_RG_ADDR0(base)          (((PERI_GR_PPU_RG_V1_Type *) (base))->ADDR0)
#define PERI_GR_PPU_RG_ATT0(base)           (((PERI_GR_PPU_RG_V1_Type *) (base))->ATT0)
#define PERI_GR_PPU_RG_ATT1(base)           (((PERI_GR_PPU_RG_V1_Type *) (base))->ATT1)

#define PERI_MS_PPU_PR_SL_ADDR(base)        (((PERI_MS_PPU_PR_V2_Type *) (base))->SL_ADDR)
#define PERI_MS_PPU_PR_SL_SIZE(base)        (((PERI_MS_PPU_PR_V2_Type *) (base))->SL_SIZE)
#define PERI_MS_PPU_PR_MS_ATT(base)         ((volatile uint32_t *) &(((PERI_MS_PPU_PR_V2_Type *)(base))->MS_ATT0))
#define PERI_MS_PPU_PR_SL_ATT(base)         ((volatile uint32_t *) &(((PERI_MS_PPU_PR_V2_Type *)(base))->SL_ATT0))
#define PERI_MS_PPU_FX_MS_ATT(base)         ((volatile uint32_t *) &(((PERI_MS_PPU_FX_V2_Type *)(base))->MS_ATT0))
#define PERI_MS_PPU_FX_SL_ATT(base)         ((volatile uint32_t *) &(((PERI_MS_PPU_FX_V2_Type *)(base))->SL_ATT0))


/*******************************************************************************
*                PROT
*******************************************************************************/

#define CY_PROT_BASE                        (cy_device->protBase)
#define PROT_MPU_MS_CTL(mpu)                (((PROT_V1_Type*)CY_PROT_BASE)->CYMPU[(mpu)].MS_CTL)
#define PROT_MPU_MPU_STRUCT_ADDR(base)      (((PROT_MPU_MPU_STRUCT_V1_Type *) (base))->ADDR)
#define PROT_MPU_MPU_STRUCT_ATT(base)       (((PROT_MPU_MPU_STRUCT_V1_Type *) (base))->ATT)

#define PROT_SMPU_SMPU_STRUCT_ADDR0(base)   (((PROT_SMPU_SMPU_STRUCT_V1_Type *) (base))->ADDR0)
#define PROT_SMPU_SMPU_STRUCT_ADDR1(base)   (((PROT_SMPU_SMPU_STRUCT_V1_Type *) (base))->ADDR1)
#define PROT_SMPU_SMPU_STRUCT_ATT0(base)    (((PROT_SMPU_SMPU_STRUCT_V1_Type *) (base))->ATT0)
#define PROT_SMPU_SMPU_STRUCT_ATT1(base)    (((PROT_SMPU_SMPU_STRUCT_V1_Type *) (base))->ATT1)


/*******************************************************************************
*                IOSS
*******************************************************************************/

#define  CY_GPIO_BASE                       ((uint32_t)(cy_device->gpioBase))

#define  GPIO_INTR_CAUSE0                   (((GPIO_V1_Type*)(cy_device->gpioBase))->INTR_CAUSE0)
#define  GPIO_INTR_CAUSE1                   (((GPIO_V1_Type*)(cy_device->gpioBase))->INTR_CAUSE1)
#define  GPIO_INTR_CAUSE2                   (((GPIO_V1_Type*)(cy_device->gpioBase))->INTR_CAUSE2)
#define  GPIO_INTR_CAUSE3                   (((GPIO_V1_Type*)(cy_device->gpioBase))->INTR_CAUSE3)

#define  GPIO_PRT_OUT(base)                 (((GPIO_PRT_V1_Type*)(base))->OUT)
#define  GPIO_PRT_OUT_CLR(base)             (((GPIO_PRT_V1_Type*)(base))->OUT_CLR)
#define  GPIO_PRT_OUT_SET(base)             (((GPIO_PRT_V1_Type*)(base))->OUT_SET)
#define  GPIO_PRT_OUT_INV(base)             (((GPIO_PRT_V1_Type*)(base))->OUT_INV)
#define  GPIO_PRT_IN(base)                  (((GPIO_PRT_V1_Type*)(base))->IN)
#define  GPIO_PRT_INTR(base)                (((GPIO_PRT_V1_Type*)(base))->INTR)
#define  GPIO_PRT_INTR_MASK(base)           (((GPIO_PRT_V1_Type*)(base))->INTR_MASK)
#define  GPIO_PRT_INTR_MASKED(base)         (((GPIO_PRT_V1_Type*)(base))->INTR_MASKED)
#define  GPIO_PRT_INTR_SET(base)            (((GPIO_PRT_V1_Type*)(base))->INTR_SET)

#define  GPIO_PRT_INTR_CFG(base)            (*(volatile uint32_t *)((uint32_t)(base) + (uint32_t)(cy_device->gpioPrtIntrCfgOffset)))
#define  GPIO_PRT_CFG(base)                 (*(volatile uint32_t *)((uint32_t)(base) + (uint32_t)(cy_device->gpioPrtCfgOffset)))
#define  GPIO_PRT_CFG_IN(base)              (*(volatile uint32_t *)((uint32_t)(base) + (uint32_t)(cy_device->gpioPrtCfgInOffset)))
#define  GPIO_PRT_CFG_OUT(base)             (*(volatile uint32_t *)((uint32_t)(base) + (uint32_t)(cy_device->gpioPrtCfgOutOffset)))
#define  GPIO_PRT_CFG_SIO(base)             (*(volatile uint32_t *)((uint32_t)(base) + (uint32_t)(cy_device->gpioPrtCfgSioOffset)))

#define  CY_HSIOM_BASE                      ((uint32_t)(cy_device->hsiomBase))

#define  HSIOM_PRT_PORT_SEL0(base)          (((HSIOM_PRT_V1_Type *)(base))->PORT_SEL0)
#define  HSIOM_PRT_PORT_SEL1(base)          (((HSIOM_PRT_V1_Type *)(base))->PORT_SEL1)


/*******************************************************************************
*                I2S
*******************************************************************************/

#define I2S_CTL(base)                       (((I2S_V1_Type*)(base))->CTL)
#define I2S_CMD(base)                       (((I2S_V1_Type*)(base))->CMD)
#define I2S_CLOCK_CTL(base)                 (((I2S_V1_Type*)(base))->CLOCK_CTL)
#define I2S_TR_CTL(base)                    (((I2S_V1_Type*)(base))->TR_CTL)
#define I2S_TX_CTL(base)                    (((I2S_V1_Type*)(base))->TX_CTL)
#define I2S_TX_FIFO_CTL(base)               (((I2S_V1_Type*)(base))->TX_FIFO_CTL)
#define I2S_TX_FIFO_STATUS(base)            (((I2S_V1_Type*)(base))->TX_FIFO_STATUS)
#define I2S_TX_FIFO_WR(base)                (((I2S_V1_Type*)(base))->TX_FIFO_WR)
#define I2S_TX_WATCHDOG(base)               (((I2S_V1_Type*)(base))->TX_WATCHDOG)
#define I2S_RX_CTL(base)                    (((I2S_V1_Type*)(base))->RX_CTL)
#define I2S_RX_FIFO_CTL(base)               (((I2S_V1_Type*)(base))->RX_FIFO_CTL)
#define I2S_RX_FIFO_STATUS(base)            (((I2S_V1_Type*)(base))->RX_FIFO_STATUS)
#define I2S_RX_FIFO_RD(base)                (((I2S_V1_Type*)(base))->RX_FIFO_RD)
#define I2S_RX_FIFO_RD_SILENT(base)         (((I2S_V1_Type*)(base))->RX_FIFO_RD_SILENT)
#define I2S_RX_WATCHDOG(base)               (((I2S_V1_Type*)(base))->RX_WATCHDOG)
#define I2S_INTR(base)                      (((I2S_V1_Type*)(base))->INTR)
#define I2S_INTR_SET(base)                  (((I2S_V1_Type*)(base))->INTR_SET)
#define I2S_INTR_MASK(base)                 (((I2S_V1_Type*)(base))->INTR_MASK)
#define I2S_INTR_MASKED(base)               (((I2S_V1_Type*)(base))->INTR_MASKED)


/*******************************************************************************
*                PDM
*******************************************************************************/

#define PDM_PCM_CTL(base)                   (((PDM_V1_Type*)(base))->CTL)
#define PDM_PCM_CMD(base)                   (((PDM_V1_Type*)(base))->CMD)
#define PDM_PCM_CLOCK_CTL(base)             (((PDM_V1_Type*)(base))->CLOCK_CTL)
#define PDM_PCM_MODE_CTL(base)              (((PDM_V1_Type*)(base))->MODE_CTL)
#define PDM_PCM_DATA_CTL(base)              (((PDM_V1_Type*)(base))->DATA_CTL)
#define PDM_PCM_TR_CTL(base)                (((PDM_V1_Type*)(base))->TR_CTL)
#define PDM_PCM_INTR_MASK(base)             (((PDM_V1_Type*)(base))->INTR_MASK)
#define PDM_PCM_INTR_MASKED(base)           (((PDM_V1_Type*)(base))->INTR_MASKED)
#define PDM_PCM_INTR(base)                  (((PDM_V1_Type*)(base))->INTR)
#define PDM_PCM_INTR_SET(base)              (((PDM_V1_Type*)(base))->INTR_SET)
#define PDM_PCM_RX_FIFO_STATUS(base)        (((PDM_V1_Type*)(base))->RX_FIFO_STATUS)
#define PDM_PCM_RX_FIFO_CTL(base)           (((PDM_V1_Type*)(base))->RX_FIFO_CTL)
#define PDM_PCM_RX_FIFO_RD(base)            (((PDM_V1_Type*)(base))->RX_FIFO_RD)
#define PDM_PCM_RX_FIFO_RD_SILENT(base)     (((PDM_V1_Type*)(base))->RX_FIFO_RD_SILENT)


/*******************************************************************************
*                LCD
*******************************************************************************/

#define LCD_OCTET_NUM                       (8U) /* number of octets */
#define LCD_COM_NUM                         (8U) /* maximum number of commons */

#define LCD_ID(base)                        (((LCD_V1_Type*)(base))->ID)
#define LCD_CONTROL(base)                   (((LCD_V1_Type*)(base))->CONTROL)
#define LCD_DIVIDER(base)                   (((LCD_V1_Type*)(base))->DIVIDER)
#define LCD_DATA0(base)                     (((LCD_V1_Type*)(base))->DATA0)
#define LCD_DATA1(base)                     (((LCD_V1_Type*)(base))->DATA1)
#define LCD_DATA2(base)                     (((LCD_V1_Type*)(base))->DATA2)
#define LCD_DATA3(base)                     (((LCD_V1_Type*)(base))->DATA3)


/*******************************************************************************
*                CRYPTO
*******************************************************************************/
/* Non-changed registers */
#define REG_CRYPTO_CTL(base)               (((CRYPTO_V1_Type*)(base))->CTL)
#define REG_CRYPTO_ERROR_STATUS0(base)     (((CRYPTO_V1_Type*)(base))->ERROR_STATUS0)
#define REG_CRYPTO_ERROR_STATUS1(base)     (((CRYPTO_V1_Type*)(base))->ERROR_STATUS1)
#define REG_CRYPTO_PR_LFSR_CTL0(base)      (((CRYPTO_V1_Type*)(base))->PR_LFSR_CTL0)
#define REG_CRYPTO_PR_LFSR_CTL1(base)      (((CRYPTO_V1_Type*)(base))->PR_LFSR_CTL1)
#define REG_CRYPTO_PR_LFSR_CTL2(base)      (((CRYPTO_V1_Type*)(base))->PR_LFSR_CTL2)
#define REG_CRYPTO_TR_CTL0(base)           (((CRYPTO_V1_Type*)(base))->TR_CTL0)
#define REG_CRYPTO_TR_CTL1(base)           (((CRYPTO_V1_Type*)(base))->TR_CTL1)
#define REG_CRYPTO_TR_GARO_CTL(base)       (((CRYPTO_V1_Type*)(base))->TR_GARO_CTL)
#define REG_CRYPTO_TR_FIRO_CTL(base)       (((CRYPTO_V1_Type*)(base))->TR_FIRO_CTL)
#define REG_CRYPTO_TR_MON_CTL(base)        (((CRYPTO_V1_Type*)(base))->TR_MON_CTL)
#define REG_CRYPTO_TR_MON_CMD(base)        (((CRYPTO_V1_Type*)(base))->TR_MON_CMD)
#define REG_CRYPTO_TR_MON_RC_CTL(base)     (((CRYPTO_V1_Type*)(base))->TR_MON_RC_CTL)
#define REG_CRYPTO_TR_MON_AP_CTL(base)     (((CRYPTO_V1_Type*)(base))->TR_MON_AP_CTL)

/* Changed registers in the regmap */
#define REG_CRYPTO_STATUS(base)            (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoStatusOffset))
#define REG_CRYPTO_INSTR_FF_CTL(base)      (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoIstrFfCtlOffset))
#define REG_CRYPTO_INSTR_FF_STATUS(base)   (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoInstrFfStatusOffset))
#define REG_CRYPTO_INSTR_FF_WR(base)       (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoInstrFfWrOffset))
#define REG_CRYPTO_AES_CTL(base)           (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoAesCtlOffset))
#define REG_CRYPTO_PR_RESULT(base)         (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoPrResultOffset))
#define REG_CRYPTO_TR_RESULT(base)         (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoTrResultOffset))
#define REG_CRYPTO_CRC_CTL(base)           (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoCrcCtlOffset))
#define REG_CRYPTO_CRC_DATA_CTL(base)      (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoCrcDataCtlOffset))
#define REG_CRYPTO_CRC_POL_CTL(base)       (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoCrcPolCtlOffset))
#define REG_CRYPTO_CRC_REM_CTL(base)       (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoCrcRemCtlOffset))
#define REG_CRYPTO_CRC_REM_RESULT(base)    (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoCrcRemResultOffset))
#define REG_CRYPTO_VU_CTL0(base)           (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoVuCtl0Offset))
#define REG_CRYPTO_VU_CTL1(base)           (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoVuCtl1Offset))
#define REG_CRYPTO_VU_STATUS(base)         (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoVuStatusOffset))
#define REG_CRYPTO_INTR(base)              (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoIntrOffset))
#define REG_CRYPTO_INTR_SET(base)          (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoIntrSetOffset))
#define REG_CRYPTO_INTR_MASK(base)         (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoIntrMaskOffset))
#define REG_CRYPTO_INTR_MASKED(base)       (*(volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoIntrMaskedOffset))
#define REG_CRYPTO_VU_RF_DATA(base, reg)   ((volatile uint32_t*)((uint32_t)(base) + cy_device->cryptoVuRfDataOffset))[(reg)]
#define REG_CRYPTO_MEM_BUFF(base)          ((uint32_t*)((uint32_t)(base) + cy_device->cryptoMemBufOffset))

/* Old V1 registers in the regmap */
#define REG_CRYPTO_RAM_PWRUP_DELAY(base)   (((CRYPTO_V1_Type*)(base))->RAM_PWRUP_DELAY)
#define REG_CRYPTO_STR_RESULT(base)        (((CRYPTO_V1_Type*)(base))->STR_RESULT)
#define REG_CRYPTO_SHA_CTL(base)           (((CRYPTO_V1_Type*)(base))->SHA_CTL)
#define REG_CRYPTO_CRC_LFSR_CTL(base)      (((CRYPTO_V1_Type*)(base))->CRC_LFSR_CTL)

/* New V2 registers in the regmap */
#define REG_CRYPTO_RAM_PWR_CTL(base)       (((CRYPTO_V2_Type*)(base))->RAM_PWR_CTL)
#define REG_CRYPTO_RAM_PWR_DELAY_CTL(base) (((CRYPTO_V2_Type*)(base))->RAM_PWR_DELAY_CTL)
#define REG_CRYPTO_ECC_CTL(base)           (((CRYPTO_V2_Type*)(base))->ECC_CTL)
#define REG_CRYPTO_PR_MAX_CTL(base)        (((CRYPTO_V2_Type*)(base))->PR_MAX_CTL)
#define REG_CRYPTO_PR_CMD(base)            (((CRYPTO_V2_Type*)(base))->PR_CMD)
#define REG_CRYPTO_TR2_CTL(base)           (((CRYPTO_V2_Type*)(base))->TR2_CTL)
#define REG_CRYPTO_TR_STATUS(base)         (((CRYPTO_V2_Type*)(base))->TR_STATUS)
#define REG_CRYPTO_TR_CMD(base)            (((CRYPTO_V2_Type*)(base))->TR_CMD)
#define REG_CRYPTO_LOAD0_FF_STATUS(base)   (((CRYPTO_V2_Type*)(base))->LOAD0_FF_STATUS)
#define REG_CRYPTO_LOAD1_FF_STATUS(base)   (((CRYPTO_V2_Type*)(base))->LOAD1_FF_STATUS)
#define REG_CRYPTO_STORE_FF_STATUS(base)   (((CRYPTO_V2_Type*)(base))->STORE_FF_STATUS)
#define REG_CRYPTO_RESULT(base)            (((CRYPTO_V2_Type*)(base))->RESULT)
#define REG_CRYPTO_VU_CTL2(base)           (((CRYPTO_V2_Type*)(base))->VU_CTL2)
#define REG_CRYPTO_DEV_KEY_ADDR0_CTL(base) (((CRYPTO_V2_Type*)(base))->DEV_KEY_ADDR0_CTL)
#define REG_CRYPTO_DEV_KEY_ADDR0(base)     (((CRYPTO_V2_Type*)(base))->DEV_KEY_ADDR0)
#define REG_CRYPTO_DEV_KEY_ADDR1_CTL(base) (((CRYPTO_V2_Type*)(base))->DEV_KEY_ADDR1_CTL)
#define REG_CRYPTO_DEV_KEY_ADDR1(base)     (((CRYPTO_V2_Type*)(base))->DEV_KEY_ADDR1)
#define REG_CRYPTO_DEV_KEY_STATUS(base)    (((CRYPTO_V2_Type*)(base))->DEV_KEY_STATUS)
#define REG_CRYPTO_DEV_KEY_CTL0(base)      (((CRYPTO_V2_Type*)(base))->DEV_KEY_CTL0)
#define REG_CRYPTO_DEV_KEY_CTL1(base)      (((CRYPTO_V2_Type*)(base))->DEV_KEY_CTL1)


/*******************************************************************************
*                IPC
*******************************************************************************/

#define REG_IPC_STRUCT_ACQUIRE(base)           (((IPC_STRUCT_V1_Type*)(base))->ACQUIRE)
#define REG_IPC_STRUCT_RELEASE(base)           (((IPC_STRUCT_V1_Type*)(base))->RELEASE)
#define REG_IPC_STRUCT_NOTIFY(base)            (((IPC_STRUCT_V1_Type*)(base))->NOTIFY)
#define REG_IPC_STRUCT_DATA(base)              (((IPC_STRUCT_V1_Type*)(base))->DATA)
#define REG_IPC_STRUCT_DATA1(base)             (((IPC_STRUCT_V2_Type*)(base))->DATA1)
#define REG_IPC_STRUCT_LOCK_STATUS(base)       (*(volatile uint32_t*)((uint32_t)(base) + cy_device->ipcLockStatusOffset))

#define REG_IPC_INTR_STRUCT_INTR(base)         (((IPC_INTR_STRUCT_V1_Type*)(base))->INTR)
#define REG_IPC_INTR_STRUCT_INTR_SET(base)     (((IPC_INTR_STRUCT_V1_Type*)(base))->INTR_SET)
#define REG_IPC_INTR_STRUCT_INTR_MASK(base)    (((IPC_INTR_STRUCT_V1_Type*)(base))->INTR_MASK)
#define REG_IPC_INTR_STRUCT_INTR_MASKED(base)  (((IPC_INTR_STRUCT_V1_Type*)(base))->INTR_MASKED)


/*******************************************************************************
*                CTB
*******************************************************************************/

#define CTBM_CTB_CTRL(base)                 (((CTBM_V1_Type *) (base))->CTB_CTRL)
#define CTBM_CTB_SW_DS_CTRL(base)           (((CTBM_V1_Type *) (base))->CTB_SW_DS_CTRL)
#define CTBM_CTB_SW_SQ_CTRL(base)           (((CTBM_V1_Type *) (base))->CTB_SW_SQ_CTRL)
#define CTBM_CTD_SW(base)                   (((CTBM_V1_Type *) (base))->CTD_SW)
#define CTBM_CTD_SW_CLEAR(base)             (((CTBM_V1_Type *) (base))->CTD_SW_CLEAR)
#define CTBM_COMP_STAT(base)                (((CTBM_V1_Type *) (base))->COMP_STAT)
#define CTBM_OA0_SW_CLEAR(base)             (((CTBM_V1_Type *) (base))->OA0_SW_CLEAR)
#define CTBM_OA1_SW_CLEAR(base)             (((CTBM_V1_Type *) (base))->OA1_SW_CLEAR)
#define CTBM_OA0_SW(base)                   (((CTBM_V1_Type *) (base))->OA0_SW)
#define CTBM_OA1_SW(base)                   (((CTBM_V1_Type *) (base))->OA1_SW)
#define CTBM_OA_RES0_CTRL(base)             (((CTBM_V1_Type *) (base))->OA_RES0_CTRL)
#define CTBM_OA_RES1_CTRL(base)             (((CTBM_V1_Type *) (base))->OA_RES1_CTRL)
#define CTBM_OA0_COMP_TRIM(base)            (((CTBM_V1_Type *) (base))->OA0_COMP_TRIM)
#define CTBM_OA1_COMP_TRIM(base)            (((CTBM_V1_Type *) (base))->OA1_COMP_TRIM)
#define CTBM_OA0_OFFSET_TRIM(base)          (((CTBM_V1_Type *) (base))->OA0_OFFSET_TRIM)
#define CTBM_OA1_OFFSET_TRIM(base)          (((CTBM_V1_Type *) (base))->OA1_OFFSET_TRIM)
#define CTBM_OA0_SLOPE_OFFSET_TRIM(base)    (((CTBM_V1_Type *) (base))->OA0_SLOPE_OFFSET_TRIM)
#define CTBM_OA1_SLOPE_OFFSET_TRIM(base)    (((CTBM_V1_Type *) (base))->OA1_SLOPE_OFFSET_TRIM)
#define CTBM_INTR(base)                     (((CTBM_V1_Type *) (base))->INTR)
#define CTBM_INTR_SET(base)                 (((CTBM_V1_Type *) (base))->INTR_SET)
#define CTBM_INTR_MASK(base)                (((CTBM_V1_Type *) (base))->INTR_MASK)
#define CTBM_INTR_MASKED(base)              (((CTBM_V1_Type *) (base))->INTR_MASKED)


/*******************************************************************************
*                CTDAC
*******************************************************************************/

#define CTDAC_CTDAC_CTRL(base)              (((CTDAC_V1_Type *) (base))->CTDAC_CTRL)
#define CTDAC_CTDAC_SW(base)                (((CTDAC_V1_Type *) (base))->CTDAC_SW)
#define CTDAC_CTDAC_SW_CLEAR(base)          (((CTDAC_V1_Type *) (base))->CTDAC_SW_CLEAR)
#define CTDAC_CTDAC_VAL(base)               (((CTDAC_V1_Type *) (base))->CTDAC_VAL)
#define CTDAC_CTDAC_VAL_NXT(base)           (((CTDAC_V1_Type *) (base))->CTDAC_VAL_NXT)
#define CTDAC_INTR(base)                    (((CTDAC_V1_Type *) (base))->INTR)
#define CTDAC_INTR_SET(base)                (((CTDAC_V1_Type *) (base))->INTR_SET)
#define CTDAC_INTR_MASK(base)               (((CTDAC_V1_Type *) (base))->INTR_MASK)
#define CTDAC_INTR_MASKED(base)             (((CTDAC_V1_Type *) (base))->INTR_MASKED)


/*******************************************************************************
*                SYSANALOG
*******************************************************************************/

#define PASS_AREF_AREF_CTRL     (((PASS_V1_Type*) cy_device->passBase)->AREF.AREF_CTRL)
#define PASS_INTR_CAUSE         (((PASS_V1_Type*) cy_device->passBase)->INTR_CAUSE)


#endif /* CY_DEVICE_H_ */

/* [] END OF FILE */

