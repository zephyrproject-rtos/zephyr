/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_msmc.h"

#if defined(__riscv)
#define CONFIG_NORMAL_SLEEP EVENT_UNIT->SLPCTRL = (EVENT_UNIT->SLPCTRL & ~0x03) | (1 << 0)
#define CONFIG_DEEP_SLEEP EVENT_UNIT->SLPCTRL |= 0x03;
#else
#define CONFIG_NORMAL_SLEEP SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk
#define CONFIG_DEEP_SLEEP SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk
#endif

status_t SMC_SetPowerModeRun(SMC_Type *base)
{
    uint32_t reg;

    reg = base->PMCTRL;
    /* configure Normal RUN mode */
    reg &= ~SMC_PMCTRL_RUNM_MASK;
    reg |= (kSMC_RunNormal << SMC_PMCTRL_RUNM_SHIFT);
    base->PMCTRL = reg;

    return kStatus_Success;
}

status_t SMC_SetPowerModeHsrun(SMC_Type *base)
{
    uint32_t reg;

    reg = base->PMCTRL;
    /* configure High Speed RUN mode */
    reg &= ~SMC_PMCTRL_RUNM_MASK;
    reg |= (kSMC_Hsrun << SMC_PMCTRL_RUNM_SHIFT);
    base->PMCTRL = reg;

    return kStatus_Success;
}

status_t SMC_SetPowerModeWait(SMC_Type *base)
{
    /* configure Normal Wait mode */
    CONFIG_NORMAL_SLEEP;

    __DSB();
    __WFI();
    __ISB();

    return kStatus_Success;
}

status_t SMC_SetPowerModeStop(SMC_Type *base, smc_partial_stop_option_t option)
{
    uint32_t reg;

    /* configure the Partial Stop mode in Noraml Stop mode */
    reg = base->PMCTRL;
    reg &= ~(SMC_PMCTRL_PSTOPO_MASK | SMC_PMCTRL_STOPM_MASK);
    reg |= ((uint32_t)option << SMC_PMCTRL_PSTOPO_SHIFT) | (kSMC_StopNormal << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

    /* Set the SLEEPDEEP bit to enable deep sleep mode (stop mode) */
    CONFIG_DEEP_SLEEP;

    /* read back to make sure the configuration valid before entering stop mode */
    (void)base->PMCTRL;
    __DSB();
    __WFI();
    __ISB();

#if (defined(FSL_FEATURE_SMC_HAS_PMCTRL_STOPA) && FSL_FEATURE_SMC_HAS_PMCTRL_STOPA)
    /* check whether the power mode enter Stop mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
#else
    return kStatus_Success;
#endif /* FSL_FEATURE_SMC_HAS_PMCTRL_STOPA */
}

status_t SMC_SetPowerModeVlpr(SMC_Type *base)
{
    uint32_t reg;

    reg = base->PMCTRL;
    /* configure VLPR mode */
    reg &= ~SMC_PMCTRL_RUNM_MASK;
    reg |= (kSMC_RunVlpr << SMC_PMCTRL_RUNM_SHIFT);
    base->PMCTRL = reg;

    return kStatus_Success;
}

status_t SMC_SetPowerModeVlpw(SMC_Type *base)
{
    /* configure VLPW mode */
    /* Clear the SLEEPDEEP bit to disable deep sleep mode */
    CONFIG_NORMAL_SLEEP;

    __DSB();
    __WFI();
    __ISB();

    return kStatus_Success;
}

status_t SMC_SetPowerModeVlps(SMC_Type *base)
{
    uint32_t reg;

    /* configure VLPS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopVlps << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    CONFIG_DEEP_SLEEP;

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    __DSB();
    __WFI();
    __ISB();

#if (defined(FSL_FEATURE_SMC_HAS_PMCTRL_STOPA) && FSL_FEATURE_SMC_HAS_PMCTRL_STOPA)
    /* check whether the power mode enter Stop mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
#else
    return kStatus_Success;
#endif /* FSL_FEATURE_SMC_HAS_PMCTRL_STOPA */
}

status_t SMC_SetPowerModeLls(SMC_Type *base)
{
    uint32_t reg;

    /* configure to LLS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopLls << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    CONFIG_DEEP_SLEEP;

    /* read back to make sure the configuration valid before entering stop mode */
    (void)base->PMCTRL;
    __DSB();
    __WFI();
    __ISB();

#if (defined(FSL_FEATURE_SMC_HAS_PMCTRL_STOPA) && FSL_FEATURE_SMC_HAS_PMCTRL_STOPA)
    /* check whether the power mode enter Stop mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
#else
    return kStatus_Success;
#endif /* FSL_FEATURE_SMC_HAS_PMCTRL_STOPA */
}

#if (defined(FSL_FEATURE_SMC_HAS_SUB_STOP_MODE) && FSL_FEATURE_SMC_HAS_SUB_STOP_MODE)

#if (defined(FSL_FEATURE_SMC_HAS_STOP_SUBMODE0) && FSL_FEATURE_SMC_HAS_STOP_SUBMODE0)
status_t SMC_SetPowerModeVlls0(SMC_Type *base)
{
    uint32_t reg;

    /* configure to VLLS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopVlls0 << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    CONFIG_DEEP_SLEEP;

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    __DSB();
    __WFI();
    __ISB();

    return kStatus_Success;
}
#endif /* FSL_FEATURE_SMC_HAS_STOP_SUBMODE0 */

#if (defined(FSL_FEATURE_SMC_HAS_STOP_SUBMODE2) && FSL_FEATURE_SMC_HAS_STOP_SUBMODE2)
status_t SMC_SetPowerModeVlls2(SMC_Type *base)
{
    uint32_t reg;

    /* configure to VLLS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopVlls2 << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    CONFIG_DEEP_SLEEP;

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    __DSB();
    __WFI();
    __ISB();

    return kStatus_Success;
}
#endif /* FSL_FEATURE_SMC_HAS_STOP_SUBMODE0 */

#else /* FSL_FEATURE_SMC_HAS_SUB_STOP_MODE */
status_t SMC_SetPowerModeVlls(SMC_Type *base)
{
    uint32_t reg;

    /* configure to VLLS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopVlls << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

#if defined(__riscv)
    EVENT->SCR = (EVENT->SCR & ~0x03) | (1 << 1);
#else
    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    CONFIG_DEEP_SLEEP;
#endif

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    __DSB();
    __WFI();
    __ISB();

#if (defined(FSL_FEATURE_SMC_HAS_PMCTRL_STOPA) && FSL_FEATURE_SMC_HAS_PMCTRL_STOPA)
    /* check whether the power mode enter Stop mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
#else
    return kStatus_Success;
#endif /* FSL_FEATURE_SMC_HAS_PMCTRL_STOPA */
}
#endif /* FSL_FEATURE_SMC_HAS_SUB_STOP_MODE */

void SMC_ConfigureResetPinFilter(SMC_Type *base, const smc_reset_pin_filter_config_t *config)
{
    assert(config);

    uint32_t reg;

    reg = SMC_RPC_FILTCFG(config->slowClockFilterCount) | SMC_RPC_FILTEN(config->enableFilter);
#if (defined(FSL_FEATURE_SMC_HAS_RPC_LPOFEN) && FSL_FEATURE_SMC_HAS_RPC_LPOFEN)
    if (config->enableLpoFilter)
    {
        reg |= SMC_RPC_LPOFEN_MASK;
    }
#endif /* FSL_FEATURE_SMC_HAS_RPC_LPOFEN */

    base->RPC = reg;
}
