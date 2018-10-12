/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flexram.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flexram"
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address to be used to gate or ungate the module clock
 *
 * @param base FLEXRAM base address
 *
 * @return The FLEXRAM instance
 */
static uint32_t FLEXRAM_GetInstance(FLEXRAM_Type *base);

/*!
 * @brief FLEXRAM map TCM size to register value
 *
 * @param tcmBankNum tcm banknumber
 * @retval register value correspond to the tcm size
  */
static uint8_t FLEXRAM_MapTcmSizeToRegister(uint8_t tcmBankNum);

/*!
 * @brief FLEXRAM configure TCM size
 * This function  is used to set the TCM to the actual size.When access to the TCM memory boundary ,hardfault will
 * raised by core.
 * @param itcmBankNum itcm bank number to allocate
 * @param dtcmBankNum dtcm bank number to allocate
  */
static status_t FLEXRAM_SetTCMSize(uint8_t itcmBankNum, uint8_t dtcmBankNum);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to FLEXRAM bases for each instance. */
static FLEXRAM_Type *const s_flexramBases[] = FLEXRAM_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to FLEXRAM clocks for each instance. */
static const clock_ip_name_t s_flexramClocks[] = FLEXRAM_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t FLEXRAM_GetInstance(FLEXRAM_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_flexramBases); instance++)
    {
        if (s_flexramBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_flexramBases));

    return instance;
}

void FLEXRAM_Init(FLEXRAM_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate ENET clock. */
    CLOCK_EnableClock(s_flexramClocks[FLEXRAM_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* enable all the interrupt status */
    base->INT_STAT_EN |= kFLEXRAM_InterruptStatusAll;
    /* clear all the interrupt status */
    base->INT_STATUS |= kFLEXRAM_InterruptStatusAll;
    /* disable all the interrpt */
    base->INT_SIG_EN = 0U;
}

void FLEXRAN_Deinit(FLEXRAM_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate ENET clock. */
    CLOCK_DisableClock(s_flexramClocks[FLEXRAM_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

static uint8_t FLEXRAM_MapTcmSizeToRegister(uint8_t tcmBankNum)
{
    uint8_t tcmSizeConfig = 0U;

    switch (tcmBankNum * FSL_FEATURE_FLEXRAM_INTERNAL_RAM_BANK_SIZE)
    {
        case kFLEXRAM_TCMSize32KB:
            tcmSizeConfig = 6U;
            break;

        case kFLEXRAM_TCMSize64KB:
            tcmSizeConfig = 7U;
            break;

        case kFLEXRAM_TCMSize128KB:
            tcmSizeConfig = 8U;
            break;

        case kFLEXRAM_TCMSize256KB:
            tcmSizeConfig = 9U;
            break;

        case kFLEXRAM_TCMSize512KB:
            tcmSizeConfig = 10U;
            break;

        default:
            break;
    }

    return tcmSizeConfig;
}

static status_t FLEXRAM_SetTCMSize(uint8_t itcmBankNum, uint8_t dtcmBankNum)
{
    /* dtcm configuration */
    if (dtcmBankNum != 0U)
    {
        IOMUXC_GPR->GPR14 &= ~IOMUXC_GPR_GPR14_CM7_CFGDTCMSZ_MASK;
        IOMUXC_GPR->GPR14 |= IOMUXC_GPR_GPR14_CM7_CFGDTCMSZ(FLEXRAM_MapTcmSizeToRegister(dtcmBankNum));
        IOMUXC_GPR->GPR16 |= IOMUXC_GPR_GPR16_INIT_DTCM_EN_MASK;
    }
    else
    {
        IOMUXC_GPR->GPR16 &= ~IOMUXC_GPR_GPR16_INIT_DTCM_EN_MASK;
    }
    /* itcm configuration */
    if (itcmBankNum != 0U)
    {
        IOMUXC_GPR->GPR14 &= ~IOMUXC_GPR_GPR14_CM7_CFGITCMSZ_MASK;
        IOMUXC_GPR->GPR14 |= IOMUXC_GPR_GPR14_CM7_CFGITCMSZ(FLEXRAM_MapTcmSizeToRegister(itcmBankNum));
        IOMUXC_GPR->GPR16 |= IOMUXC_GPR_GPR16_INIT_ITCM_EN_MASK;
    }
    else
    {
        IOMUXC_GPR->GPR16 &= ~IOMUXC_GPR_GPR16_INIT_ITCM_EN_MASK;
    }

    return kStatus_Success;
}

status_t FLEXRAM_AllocateRam(flexram_allocate_ram_t *config)
{
    uint8_t dtcmBankNum = config->dtcmBankNum;
    uint8_t itcmBankNum = config->itcmBankNum;
    uint8_t ocramBankNum = config->ocramBankNum;
    uint32_t bankCfg = 0U, i = 0U;

    /* check the arguments */
    if ((FSL_FEATURE_FLEXRAM_INTERNAL_RAM_TOTAL_BANK_NUMBERS < (dtcmBankNum + itcmBankNum + ocramBankNum)) ||
        ((dtcmBankNum != 0U) && ((dtcmBankNum & (dtcmBankNum - 1u)) != 0U)) ||
        ((itcmBankNum != 0U) && ((itcmBankNum & (itcmBankNum - 1u)) != 0U)))
    {
        return kStatus_InvalidArgument;
    }
    /* flexram bank config value */
    for (i = 0U; i < FSL_FEATURE_FLEXRAM_INTERNAL_RAM_TOTAL_BANK_NUMBERS; i++)
    {
        if (i < ocramBankNum)
        {
            bankCfg |= ((uint32_t)kFLEXRAM_BankOCRAM) << (i * 2);
            continue;
        }

        if (i < (dtcmBankNum + ocramBankNum))
        {
            bankCfg |= ((uint32_t)kFLEXRAM_BankDTCM) << (i * 2);
            continue;
        }

        if (i < (dtcmBankNum + ocramBankNum + itcmBankNum))
        {
            bankCfg |= ((uint32_t)kFLEXRAM_BankITCM) << (i * 2);
            continue;
        }
    }
    IOMUXC_GPR->GPR17 = bankCfg;
    /* set TCM size */
    FLEXRAM_SetTCMSize(itcmBankNum, dtcmBankNum);
    /* select ram allocate source from FLEXRAM_BANK_CFG */
    FLEXRAM_SetAllocateRamSrc(kFLEXRAM_BankAllocateThroughBankCfg);

    return kStatus_Success;
}
