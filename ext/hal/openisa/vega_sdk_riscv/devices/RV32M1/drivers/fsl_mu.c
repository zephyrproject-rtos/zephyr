/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_mu.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to mu clocks for each instance. */
static const clock_ip_name_t s_muClocks[] = MU_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
/*! @brief Pointers to mu bases for each instance. */
static MU_Type *const s_muBases[] = MU_BASE_PTRS;

/******************************************************************************
 * Code
 *****************************************************************************/
static uint32_t MU_GetInstance(MU_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < (sizeof(s_muBases)/sizeof(s_muBases[0])); instance++)
    {
        if (s_muBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < (sizeof(s_muBases)/sizeof(s_muBases[0])));

    return instance;
}

void MU_Init(MU_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_EnableClock(s_muClocks[MU_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void MU_Deinit(MU_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    CLOCK_DisableClock(s_muClocks[MU_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void MU_SendMsg(MU_Type *base, uint32_t regIndex, uint32_t msg)
{
    assert(regIndex < MU_TR_COUNT);

    /* Wait TX register to be empty. */
    while (!(base->SR & (kMU_Tx0EmptyFlag >> regIndex)))
    {
    }

    base->TR[regIndex] = msg;
}

uint32_t MU_ReceiveMsg(MU_Type *base, uint32_t regIndex)
{
    assert(regIndex < MU_TR_COUNT);

    /* Wait RX register to be full. */
    while (!(base->SR & (kMU_Rx0FullFlag >> regIndex)))
    {
    }

    return base->RR[regIndex];
}

void MU_SetFlags(MU_Type *base, uint32_t flags)
{
    /* Wait for update finished. */
    while (base->SR & MU_SR_FUP_MASK)
    {
    }

    MU_SetFlagsNonBlocking(base, flags);
}

status_t MU_TriggerInterrupts(MU_Type *base, uint32_t mask)
{
    uint32_t reg = base->CR;

    /* Previous interrupt has been accepted. */
    if (!(reg & mask))
    {
        /* All interrupts have been accepted, trigger now. */
        reg = (reg & ~(MU_CR_GIRn_MASK | MU_CR_NMI_MASK)) | mask;
        base->CR = reg;
        return kStatus_Success;
    }
    else
    {
        return kStatus_Fail;
    }
}

void MU_BootCoreB(MU_Type *base, mu_core_boot_mode_t mode)
{
#if (defined(FSL_FEATURE_MU_HAS_RESET_INT) && FSL_FEATURE_MU_HAS_RESET_INT)
    /* Clean the reset de-assert pending flag. */
    base->SR = MU_SR_RDIP_MASK;
#endif

#if (defined(FSL_FEATURE_MU_HAS_CCR) && FSL_FEATURE_MU_HAS_CCR)
    uint32_t reg = base->CCR;

    reg = (reg & ~(MU_CCR_HR_MASK | MU_CCR_RSTH_MASK | MU_CCR_BOOT_MASK)) | MU_CCR_BOOT(mode);

    base->CCR = reg;
#else
    uint32_t reg = base->CR;

    reg = (reg & ~((MU_CR_GIRn_MASK | MU_CR_NMI_MASK) | MU_CR_HR_MASK | MU_CR_RSTH_MASK | MU_CR_BBOOT_MASK)) | MU_CR_BBOOT(mode);

    base->CR = reg;
#endif

#if (defined(FSL_FEATURE_MU_HAS_RESET_INT) && FSL_FEATURE_MU_HAS_RESET_INT)
    /* Wait for coming out of reset. */
    while (!(base->SR & MU_SR_RDIP_MASK))
    {
    }
#endif
}

void MU_BootOtherCore(MU_Type *base, mu_core_boot_mode_t mode)
{
    /*
     * MU_BootOtherCore and MU_BootCoreB are the same, MU_BootCoreB is kept
     * for compatible with older platforms.
     */
    MU_BootCoreB(base, mode);
}

#if (defined(FSL_FEATURE_MU_HAS_CCR) && FSL_FEATURE_MU_HAS_CCR)
void MU_HardwareResetOtherCore(MU_Type *base, bool waitReset, bool holdReset, mu_core_boot_mode_t bootMode)
{
    volatile uint32_t sr = 0; 
    uint32_t ccr = base->CCR & ~(MU_CCR_HR_MASK | MU_CCR_RSTH_MASK | MU_CCR_BOOT_MASK);

    ccr |= MU_CCR_BOOT(bootMode);

    if (holdReset)
    {
        ccr |= MU_CCR_RSTH_MASK;
    }

    /* Clean the reset assert pending flag. */
    sr = (MU_SR_RAIP_MASK | MU_SR_RDIP_MASK);
    base->SR = sr;

    /* Set CCR[HR] to trigger hardware reset. */
    base->CCR = ccr | MU_CCR_HR_MASK;

    /* If don't wait the other core enters reset, return directly. */
    if (!waitReset)
    {
        return;
    }

    /* Wait for the other core go to reset. */
    while (!(base->SR & MU_SR_RAIP_MASK))
    {
    }

    if (!holdReset)
    {
        /* Clear CCR[HR]. */
        base->CCR = ccr;

        /* Wait for the other core out of reset. */
        while (!(base->SR & MU_SR_RDIP_MASK))
        {
        }
    }
}
#else
void MU_HardwareResetOtherCore(MU_Type *base, bool waitReset, bool holdReset, mu_core_boot_mode_t bootMode)
{
    volatile uint32_t sr = 0; 
    uint32_t cr = base->CR & ~(MU_CR_HR_MASK | MU_CR_RSTH_MASK | MU_CR_BOOT_MASK | MU_CR_GIRn_MASK | MU_CR_NMI_MASK);

    cr |= MU_CR_BOOT(bootMode);

    if (holdReset)
    {
        cr |= MU_CR_RSTH_MASK;
    }

    /* Clean the reset assert pending flag. */
    sr = (MU_SR_RAIP_MASK | MU_SR_RDIP_MASK);
    base->SR = sr;

    /* Set CR[HR] to trigger hardware reset. */
    base->CR = cr | MU_CR_HR_MASK;

    /* If don't wait the other core enters reset, return directly. */
    if (!waitReset)
    {
        return;
    }

    /* Wait for the other core go to reset. */
    while (!(base->SR & MU_SR_RAIP_MASK))
    {
    }

    if (!holdReset)
    {
        /* Clear CR[HR]. */
        base->CR = cr;

        /* Wait for the other core out of reset. */
        while (!(base->SR & MU_SR_RDIP_MASK))
        {
        }
    }
}
#endif
