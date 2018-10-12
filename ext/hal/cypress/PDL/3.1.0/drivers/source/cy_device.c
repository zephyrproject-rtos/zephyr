/***************************************************************************//**
* \file cy_device.c
* \version 2.10
*
* This file provides the definitions for core and peripheral block HW base
* addresses, versions, and parameters.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_device.h"

/* This is set in Cy_PDL_Init() to the device information relevant
 * for the current target.
 */
const cy_stc_device_t * cy_device;

const cy_stc_device_t cy_deviceIpBlockCfg1 =
{
    /* Base HW addresses */
    .sflashBase             =   0x16000000UL,  /* TODO: Remove */
    .cpussBase              =   0x40210000UL,
    .flashcBase             =   0x40250000UL,  /* TODO: Remove */
    .srssBase               =   0x40260000UL,  /* TODO: Remove */
    .backupBase             =   0x40270000UL,  /* TODO: Remove */
    .periBase               =   0x40010000UL,
    .udbBase                =   0x40340000UL,
    .protBase               =   0x40240000UL,
    .profileBase            =   0x402D0000UL,  /* TODO: Remove */
    .hsiomBase              =   0x40310000UL,
    .gpioBase               =   0x40320000UL,
    .passBase               =   0x411F0000UL,

    /* Versions */
    .cpussVersion           =   1U,
    .cryptoVersion          =   1U,
    .dwVersion              =   1U,
    .flashcVersion          =   1U,
    .gpioVersion            =   1U,
    .hsiomVersion           =   1U,
    .ipcVersion             =   1U,
    .periVersion            =   1U,
    .protVersion            =   1U,

    /* Parameters */
    .cpussIpcNr             =   16U,
    .cpussIpcIrqNr          =   16U,
    .cpussDwChNr            =   16U,
    .cpussFlashPaSize       =   128U,
    .cpussInterruptFmIrq    =   85,
    .srssNumClkpath         =   5U,
    .srssNumPll             =   1U,
    .srssNumHfroot          =   5U,
    .periClockNr            =   59U,
    .smifDeviceNr           =   4U,
    .passSarChannels        =   16U,
    .iossGpioPortNr         =   15U,
    .udbPresent             =   1U,
    .flashcSize             =   0x100000U,
    .flashRwwSupport        =   1U,
    .flashPipeEnable        =   1U,
    .flashWriteDelay        =   1U,
    .flashProgramDelay      =   1U,
    .flashEraseDelay        =   1U,
    .flashCtlMainWs0Freq    =   29U,
    .flashCtlMainWs1Freq    =   58U,
    .flashCtlMainWs2Freq    =   87U,
    .flashCtlMainWs3Freq    =   120U,
    .flashCtlMainWs4Freq    =   150U,
    .sysPmMmioUdbGroupNr    =   4U,
    .sysPmMmioUdbSlaveNr    =   3U,
    .sysPmSimoPresent       =   1U,

    /* register structure specific numbers */
    /* DW registers */
    .dwChOffset             =   offsetof(DW_V1_Type, CH_STRUCT),
    .dwChSize               =   sizeof(DW_CH_STRUCT_V1_Type),
    
    /* PERI registers */
    .periTrCmdOffset        =   offsetof(PERI_V1_Type, TR_CMD),
    .periTrCmdGrSelMsk      =   PERI_TR_CMD_GROUP_SEL_Msk,
    .periTrGrOffset         =   offsetof(PERI_V1_Type, TR_GR),
    .periTrGrSize           =   sizeof(PERI_TR_GR_V1_Type),
    
    .periDivCmdDivSelMsk    =   PERI_DIV_CMD_DIV_SEL_Msk,
    .periDivCmdTypeSelPos   =   PERI_DIV_CMD_TYPE_SEL_Pos,
    .periDivCmdPaDivSelPos  =   PERI_DIV_CMD_PA_DIV_SEL_Pos,
    .periDivCmdPaTypeSelPos =   PERI_DIV_CMD_PA_TYPE_SEL_Pos,

    .periDiv8CtlOffset      =   offsetof(PERI_V1_Type, DIV_8_CTL),
    .periDiv16CtlOffset     =   offsetof(PERI_V1_Type, DIV_16_CTL),
    .periDiv16_5CtlOffset   =   offsetof(PERI_V1_Type, DIV_16_5_CTL),
    .periDiv24_5CtlOffset   =   offsetof(PERI_V1_Type, DIV_24_5_CTL),
    
    /* GPIO registers */
    .gpioPrtIntrCfgOffset   =   offsetof(GPIO_PRT_V1_Type, INTR_CFG),
    .gpioPrtCfgOffset       =   offsetof(GPIO_PRT_V1_Type, CFG),
    .gpioPrtCfgInOffset     =   offsetof(GPIO_PRT_V1_Type, CFG_IN),
    .gpioPrtCfgOutOffset    =   offsetof(GPIO_PRT_V1_Type, CFG_OUT),
    .gpioPrtCfgSioOffset    =   offsetof(GPIO_PRT_V1_Type, CFG_SIO),

    /* CPUSS registers */
    .cpussCm0ClockCtlOffset =   offsetof(CPUSS_V1_Type, CM0_CLOCK_CTL),
    .cpussCm4ClockCtlOffset =   offsetof(CPUSS_V1_Type, CM4_CLOCK_CTL),
    .cpussCm4StatusOffset   =   offsetof(CPUSS_V1_Type, CM4_STATUS),
    .cpussCm0StatusOffset   =   offsetof(CPUSS_V1_Type, CM0_STATUS),
    .cpussCm4PwrCtlOffset   =   offsetof(CPUSS_V1_Type, CM4_PWR_CTL),
    .cpussTrimRamCtlOffset  =   offsetof(CPUSS_V1_Type, TRIM_RAM_CTL),
    .cpussTrimRomCtlOffset  =   offsetof(CPUSS_V1_Type, TRIM_ROM_CTL),
    .cpussCm0NmiCtlOffset   =   (uint16_t)offsetof(CPUSS_V1_Type, CM0_NMI_CTL),
    .cpussCm4NmiCtlOffset   =   (uint16_t)offsetof(CPUSS_V1_Type, CM4_NMI_CTL),

    /* IPC register */
    .ipcLockStatusOffset    =   offsetof(IPC_STRUCT_V1_Type, LOCK_STATUS),

    .cryptoStatusOffset      = offsetof(CRYPTO_V1_Type, STATUS),
    .cryptoIstrFfCtlOffset   = offsetof(CRYPTO_V1_Type, INSTR_FF_CTL),
    .cryptoInstrFfStatusOffset = offsetof(CRYPTO_V1_Type, INSTR_FF_STATUS),
    .cryptoInstrFfWrOffset   = offsetof(CRYPTO_V1_Type, INSTR_FF_WR),
    .cryptoVuRfDataOffset    = offsetof(CRYPTO_V1_Type, RF_DATA),
    .cryptoAesCtlOffset      = offsetof(CRYPTO_V1_Type, AES_CTL),
    .cryptoPrResultOffset    = offsetof(CRYPTO_V1_Type, PR_RESULT),
    .cryptoTrResultOffset    = offsetof(CRYPTO_V1_Type, TR_RESULT),
    .cryptoCrcCtlOffset      = offsetof(CRYPTO_V1_Type, CRC_CTL),
    .cryptoCrcDataCtlOffset  = offsetof(CRYPTO_V1_Type, CRC_DATA_CTL),
    .cryptoCrcPolCtlOffset   = offsetof(CRYPTO_V1_Type, CRC_POL_CTL),
    .cryptoCrcRemCtlOffset   = offsetof(CRYPTO_V1_Type, CRC_REM_CTL),
    .cryptoCrcRemResultOffset = offsetof(CRYPTO_V1_Type, CRC_REM_RESULT),
    .cryptoVuCtl0Offset      = offsetof(CRYPTO_V1_Type, VU_CTL0),
    .cryptoVuCtl1Offset      = offsetof(CRYPTO_V1_Type, VU_CTL1),
    .cryptoVuStatusOffset    = offsetof(CRYPTO_V1_Type, VU_STATUS),
    .cryptoIntrOffset        = offsetof(CRYPTO_V1_Type, INTR),
    .cryptoIntrSetOffset     = offsetof(CRYPTO_V1_Type, INTR_SET),
    .cryptoIntrMaskOffset    = offsetof(CRYPTO_V1_Type, INTR_MASK),
    .cryptoIntrMaskedOffset  = offsetof(CRYPTO_V1_Type, INTR_MASKED),
    .cryptoMemBufOffset      = offsetof(CRYPTO_V1_Type, MEM_BUFF),
};

const cy_stc_device_t cy_deviceIpBlockCfg2 =
{
    /* Base addresses */
    .sflashBase             =   0x16000000UL,  /* TODO: Remove */
    .cpussBase              =   0x40200000UL,
    .flashcBase             =   0x40240000UL,  /* TODO: Remove */
    .srssBase               =   0x40260000UL,  /* TODO: Remove */
    .backupBase             =   0x40270000UL,  /* TODO: Remove */
    .periBase               =   0x40000000UL,
    .udbBase                =   0UL,
    .protBase               =   0x40230000UL,
    .profileBase            =   0x402D0000UL,  /* TODO: Remove */
    .hsiomBase              =   0x40300000UL,
    .gpioBase               =   0x40310000UL,
    .passBase               =   0x409F0000UL,

    /* IP block versions */
    .cpussVersion           =   2U,
    .cryptoVersion          =   2U,
    .dwVersion              =   2U,
    .flashcVersion          =   2U,
    .gpioVersion            =   2U,
    .hsiomVersion           =   2U,
    .ipcVersion             =   2U,
    .periVersion            =   2U,
    .protVersion            =   2U,

    /* Parameters */
    .cpussIpcNr             =   16U,
    .cpussIpcIrqNr          =   16U,
    .cpussDwChNr            =   29U,
    .cpussFlashPaSize       =   128U,
    .cpussInterruptFmIrq    =   107U,
    .srssNumClkpath         =   6U,
    .srssNumPll             =   2U,
    .srssNumHfroot          =   6U,
    .periClockNr            =   54U,
    .smifDeviceNr           =   4,
    .passSarChannels        =   16U,
    .iossGpioPortNr         =   15U,
    .udbPresent             =   0U,
    .flashcSize             =   0x200000U,
    .flashRwwSupport        =   0U,
    .flashPipeEnable        =   1U,
    .flashWriteDelay        =   0U,
    .flashProgramDelay      =   0U,
    .flashEraseDelay        =   0U,
    .flashCtlMainWs0Freq    =   25U,
    .flashCtlMainWs1Freq    =   50U,
    .flashCtlMainWs2Freq    =   75U,
    .flashCtlMainWs3Freq    =   100U,
    .flashCtlMainWs4Freq    =   125U,
    .sysPmMmioUdbGroupNr    =   4U,
    .sysPmMmioUdbSlaveNr    =   3U,
    .sysPmSimoPresent       =   0U,

    /* register structure specific numbers */
    /* DW registers */
    .dwChOffset             =   offsetof(DW_V2_Type, CH_STRUCT),
    .dwChSize               =   sizeof(DW_CH_STRUCT_V2_Type),
    
    /* PERI registers */
    .periTrCmdOffset        =   offsetof(PERI_V2_Type, TR_CMD),
    .periTrCmdGrSelMsk      =   PERI_V2_TR_CMD_GROUP_SEL_Msk,
    .periTrGrOffset         =   offsetof(PERI_V2_Type, TR_GR),
    .periTrGrSize           =   sizeof(PERI_TR_GR_V2_Type),
    
    .periDivCmdDivSelMsk    =   (uint8_t)PERI_V2_DIV_CMD_DIV_SEL_Msk,
    .periDivCmdTypeSelPos   =   (uint8_t)PERI_V2_DIV_CMD_TYPE_SEL_Pos,
    .periDivCmdPaDivSelPos  =   (uint8_t)PERI_V2_DIV_CMD_PA_DIV_SEL_Pos,
    .periDivCmdPaTypeSelPos =   (uint8_t)PERI_V2_DIV_CMD_PA_TYPE_SEL_Pos,

    .periDiv8CtlOffset      =   offsetof(PERI_V2_Type, DIV_8_CTL),
    .periDiv16CtlOffset     =   offsetof(PERI_V2_Type, DIV_16_CTL),
    .periDiv16_5CtlOffset   =   offsetof(PERI_V2_Type, DIV_16_5_CTL),
    .periDiv24_5CtlOffset   =   offsetof(PERI_V2_Type, DIV_24_5_CTL),
    
    /* GPIO registers */
    .gpioPrtIntrCfgOffset   =   offsetof(GPIO_PRT_V2_Type, INTR_CFG),
    .gpioPrtCfgOffset       =   offsetof(GPIO_PRT_V2_Type, CFG),
    .gpioPrtCfgInOffset     =   offsetof(GPIO_PRT_V2_Type, CFG_IN),
    .gpioPrtCfgOutOffset    =   offsetof(GPIO_PRT_V2_Type, CFG_OUT),
    .gpioPrtCfgSioOffset    =   offsetof(GPIO_PRT_V2_Type, CFG_SIO),

    /* CPUSS registers */
    .cpussCm0ClockCtlOffset =   offsetof(CPUSS_V2_Type, CM0_CLOCK_CTL),
    .cpussCm4ClockCtlOffset =   offsetof(CPUSS_V2_Type, CM4_CLOCK_CTL),
    .cpussCm4StatusOffset   =   offsetof(CPUSS_V2_Type, CM4_STATUS),
    .cpussCm0StatusOffset   =   offsetof(CPUSS_V2_Type, CM0_STATUS),
    .cpussCm4PwrCtlOffset   =   offsetof(CPUSS_V2_Type, CM4_PWR_CTL),
    .cpussTrimRamCtlOffset  =   offsetof(CPUSS_V2_Type, TRIM_RAM_CTL),
    .cpussTrimRomCtlOffset  =   offsetof(CPUSS_V2_Type, TRIM_ROM_CTL),
    .cpussCm0NmiCtlOffset   =   (uint16_t)offsetof(CPUSS_V2_Type, CM0_NMI_CTL),
    .cpussCm4NmiCtlOffset   =   (uint16_t)offsetof(CPUSS_V2_Type, CM4_NMI_CTL),

    /* IPC register */
    .ipcLockStatusOffset    =   offsetof(IPC_STRUCT_V2_Type, LOCK_STATUS),

    .cryptoStatusOffset      = offsetof(CRYPTO_V2_Type, STATUS),
    .cryptoIstrFfCtlOffset   = offsetof(CRYPTO_V2_Type, INSTR_FF_CTL),
    .cryptoInstrFfStatusOffset = offsetof(CRYPTO_V2_Type, INSTR_FF_STATUS),
    .cryptoInstrFfWrOffset   = offsetof(CRYPTO_V2_Type, INSTR_FF_WR),
    .cryptoVuRfDataOffset    = offsetof(CRYPTO_V2_Type, VU_RF_DATA),
    .cryptoAesCtlOffset      = offsetof(CRYPTO_V2_Type, AES_CTL),
    .cryptoPrResultOffset    = offsetof(CRYPTO_V2_Type, PR_RESULT),
    .cryptoTrResultOffset    = offsetof(CRYPTO_V2_Type, TR_RESULT),
    .cryptoCrcCtlOffset      = offsetof(CRYPTO_V2_Type, CRC_CTL),
    .cryptoCrcDataCtlOffset  = offsetof(CRYPTO_V2_Type, CRC_DATA_CTL),
    .cryptoCrcPolCtlOffset   = offsetof(CRYPTO_V2_Type, CRC_POL_CTL),
    .cryptoCrcRemCtlOffset   = offsetof(CRYPTO_V2_Type, CRC_REM_CTL),
    .cryptoCrcRemResultOffset = offsetof(CRYPTO_V2_Type, CRC_REM_RESULT),
    .cryptoVuCtl0Offset      = offsetof(CRYPTO_V2_Type, VU_CTL0),
    .cryptoVuCtl1Offset      = offsetof(CRYPTO_V2_Type, VU_CTL1),
    .cryptoVuStatusOffset    = offsetof(CRYPTO_V2_Type, VU_STATUS),
    .cryptoIntrOffset        = offsetof(CRYPTO_V2_Type, INTR),
    .cryptoIntrSetOffset     = offsetof(CRYPTO_V2_Type, INTR_SET),
    .cryptoIntrMaskOffset    = offsetof(CRYPTO_V2_Type, INTR_MASK),
    .cryptoIntrMaskedOffset  = offsetof(CRYPTO_V2_Type, INTR_MASKED),
    .cryptoMemBufOffset      = offsetof(CRYPTO_V2_Type, MEM_BUFF),
};


/* [] END OF FILE */
