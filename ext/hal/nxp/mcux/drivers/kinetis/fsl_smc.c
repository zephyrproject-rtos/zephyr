/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_smc.h"
#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.smc"
#endif

typedef void (*smc_stop_ram_func_t)(void);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void SMC_EnterStopRamFunc(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint32_t g_savedPrimask;

/*
 * The ram function code is:
 *
 *  uint32_t i;
 *  for (i=0; i<0x8; i++)
 *  {
 *      __NOP();
 *  }
 *  __DSB();
 *  __WFI();
 *  __ISB();
 *
 * When entring the stop modes, the flash prefetch might be interrupted, thus
 * the prefetched code or data might be broken. To make sure the flash is idle
 * when entring the stop modes, the code is moved to ram. And delay for a while
 * before WFI to make sure previous flash prefetch is finished.
 *
 * Only need to do like this when code is in flash, if code is in rom or ram,
 * this is not necessary.
 */
static uint16_t s_stopRamFuncArray[] = {
    0x2000,         /* MOVS R0, #0 */
    0x2808,         /* CMP R0, #8 */
    0xD202,         /* BCS.N */
    0xBF00,         /* NOP */
    0x1C40,         /* ADDS R0, R0, #1 */
    0xE7FA,         /* B.N */
    0xF3BF, 0x8F4F, /* DSB */
    0xBF30,         /* WFI */
    0xF3BF, 0x8F6F, /* ISB */
    0x4770,         /* BX LR */
};

/*******************************************************************************
 * Code
 ******************************************************************************/
static void SMC_EnterStopRamFunc(void)
{
    uint32_t ramFuncEntry = ((uint32_t)(s_stopRamFuncArray)) + 1U;
    smc_stop_ram_func_t stopRamFunc = (smc_stop_ram_func_t)ramFuncEntry;
    stopRamFunc();
}

#if (defined(FSL_FEATURE_SMC_HAS_PARAM) && FSL_FEATURE_SMC_HAS_PARAM)
/*!
 * brief Gets the SMC parameter.
 *
 * This function gets the SMC parameter including the enabled power mdoes.
 *
 * param base SMC peripheral base address.
 * param param         Pointer to the SMC param structure.
 */
void SMC_GetParam(SMC_Type *base, smc_param_t *param)
{
    uint32_t reg = base->PARAM;
    param->hsrunEnable = (bool)(reg & SMC_PARAM_EHSRUN_MASK);
    param->llsEnable = (bool)(reg & SMC_PARAM_ELLS_MASK);
    param->lls2Enable = (bool)(reg & SMC_PARAM_ELLS2_MASK);
    param->vlls0Enable = (bool)(reg & SMC_PARAM_EVLLS0_MASK);
}
#endif /* FSL_FEATURE_SMC_HAS_PARAM */

/*!
 * brief Prepares to enter stop modes.
 *
 * This function should be called before entering STOP/VLPS/LLS/VLLS modes.
 */
void SMC_PreEnterStopModes(void)
{
    g_savedPrimask = DisableGlobalIRQ();
    __ISB();
}

/*!
 * brief Recovers after wake up from stop modes.
 *
 * This function should be called after wake up from STOP/VLPS/LLS/VLLS modes.
 * It is used with ref SMC_PreEnterStopModes.
 */
void SMC_PostExitStopModes(void)
{
    EnableGlobalIRQ(g_savedPrimask);
    __ISB();
}

/*!
 * brief Prepares to enter wait modes.
 *
 * This function should be called before entering WAIT/VLPW modes.
 */
void SMC_PreEnterWaitModes(void)
{
    g_savedPrimask = DisableGlobalIRQ();
    __ISB();
}

/*!
 * brief Recovers after wake up from stop modes.
 *
 * This function should be called after wake up from WAIT/VLPW modes.
 * It is used with ref SMC_PreEnterWaitModes.
 */
void SMC_PostExitWaitModes(void)
{
    EnableGlobalIRQ(g_savedPrimask);
    __ISB();
}

/*!
 * brief Configures the system to RUN power mode.
 *
 * param base SMC peripheral base address.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeRun(SMC_Type *base)
{
    uint8_t reg;

    reg = base->PMCTRL;
    /* configure Normal RUN mode */
    reg &= ~SMC_PMCTRL_RUNM_MASK;
    reg |= (kSMC_RunNormal << SMC_PMCTRL_RUNM_SHIFT);
    base->PMCTRL = reg;

    return kStatus_Success;
}

#if (defined(FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE) && FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE)
/*!
 * brief Configures the system to HSRUN power mode.
 *
 * param base SMC peripheral base address.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeHsrun(SMC_Type *base)
{
    uint8_t reg;

    reg = base->PMCTRL;
    /* configure High Speed RUN mode */
    reg &= ~SMC_PMCTRL_RUNM_MASK;
    reg |= (kSMC_Hsrun << SMC_PMCTRL_RUNM_SHIFT);
    base->PMCTRL = reg;

    return kStatus_Success;
}
#endif /* FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE */

/*!
 * brief Configures the system to WAIT power mode.
 *
 * param base SMC peripheral base address.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeWait(SMC_Type *base)
{
    /* configure Normal Wait mode */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    __ISB();

    return kStatus_Success;
}

/*!
 * brief Configures the system to Stop power mode.
 *
 * param base SMC peripheral base address.
 * param  option Partial Stop mode option.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeStop(SMC_Type *base, smc_partial_stop_option_t option)
{
    uint8_t reg;

#if (defined(FSL_FEATURE_SMC_HAS_PSTOPO) && FSL_FEATURE_SMC_HAS_PSTOPO)
    /* configure the Partial Stop mode in Normal Stop mode */
    reg = base->STOPCTRL;
    reg &= ~SMC_STOPCTRL_PSTOPO_MASK;
    reg |= ((uint32_t)option << SMC_STOPCTRL_PSTOPO_SHIFT);
    base->STOPCTRL = reg;
#endif

    /* configure Normal Stop mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopNormal << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

    /* Set the SLEEPDEEP bit to enable deep sleep mode (stop mode) */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    SMC_EnterStopRamFunc();

    /* check whether the power mode enter Stop mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
}

/*!
 * brief Configures the system to VLPR power mode.
 *
 * param base SMC peripheral base address.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlpr(SMC_Type *base
#if (defined(FSL_FEATURE_SMC_HAS_LPWUI) && FSL_FEATURE_SMC_HAS_LPWUI)
                              ,
                              bool wakeupMode
#endif
                              )
{
    uint8_t reg;

    reg = base->PMCTRL;
#if (defined(FSL_FEATURE_SMC_HAS_LPWUI) && FSL_FEATURE_SMC_HAS_LPWUI)
    /* configure whether the system remains in VLP mode on an interrupt */
    if (wakeupMode)
    {
        /* exits to RUN mode on an interrupt */
        reg |= SMC_PMCTRL_LPWUI_MASK;
    }
    else
    {
        /* remains in VLP mode on an interrupt */
        reg &= ~SMC_PMCTRL_LPWUI_MASK;
    }
#endif /* FSL_FEATURE_SMC_HAS_LPWUI */

    /* configure VLPR mode */
    reg &= ~SMC_PMCTRL_RUNM_MASK;
    reg |= (kSMC_RunVlpr << SMC_PMCTRL_RUNM_SHIFT);
    base->PMCTRL = reg;

    return kStatus_Success;
}

/*!
 * brief Configures the system to VLPW power mode.
 *
 * param base SMC peripheral base address.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlpw(SMC_Type *base)
{
    /* configure VLPW mode */
    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    __ISB();

    return kStatus_Success;
}

/*!
 * brief Configures the system to VLPS power mode.
 *
 * param base SMC peripheral base address.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlps(SMC_Type *base)
{
    uint8_t reg;

    /* configure VLPS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopVlps << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    SMC_EnterStopRamFunc();

    /* check whether the power mode enter VLPS mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
}

#if (defined(FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE)
/*!
 * brief Configures the system to LLS power mode.
 *
 * param base SMC peripheral base address.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeLls(SMC_Type *base
#if ((defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE) || \
     (defined(FSL_FEATURE_SMC_HAS_LPOPO) && FSL_FEATURE_SMC_HAS_LPOPO))
                             ,
                             const smc_power_mode_lls_config_t *config
#endif
                             )
{
    uint8_t reg;

    /* configure to LLS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopLls << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

/* configure LLS sub-mode*/
#if (defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE)
    reg = base->STOPCTRL;
    reg &= ~SMC_STOPCTRL_LLSM_MASK;
    reg |= ((uint32_t)config->subMode << SMC_STOPCTRL_LLSM_SHIFT);
    base->STOPCTRL = reg;
#endif /* FSL_FEATURE_SMC_HAS_LLS_SUBMODE */

#if (defined(FSL_FEATURE_SMC_HAS_LPOPO) && FSL_FEATURE_SMC_HAS_LPOPO)
    if (config->enableLpoClock)
    {
        base->STOPCTRL &= ~SMC_STOPCTRL_LPOPO_MASK;
    }
    else
    {
        base->STOPCTRL |= SMC_STOPCTRL_LPOPO_MASK;
    }
#endif /* FSL_FEATURE_SMC_HAS_LPOPO */

    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    SMC_EnterStopRamFunc();

    /* check whether the power mode enter LLS mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
}
#endif /* FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE */

#if (defined(FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE)
/*!
 * brief Configures the system to VLLS power mode.
 *
 * param base SMC peripheral base address.
 * param  config The VLLS power mode configuration structure.
 * return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlls(SMC_Type *base, const smc_power_mode_vlls_config_t *config)
{
    uint8_t reg;

#if (defined(FSL_FEATURE_SMC_HAS_PORPO) && FSL_FEATURE_SMC_HAS_PORPO)
#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG) ||     \
    (defined(FSL_FEATURE_SMC_USE_STOPCTRL_VLLSM) && FSL_FEATURE_SMC_USE_STOPCTRL_VLLSM) || \
    (defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE)
    if (config->subMode == kSMC_StopSub0)
#endif
    {
        /* configure whether the Por Detect work in Vlls0 mode */
        if (config->enablePorDetectInVlls0)
        {
#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG)
            base->VLLSCTRL &= ~SMC_VLLSCTRL_PORPO_MASK;
#else
            base->STOPCTRL &= ~SMC_STOPCTRL_PORPO_MASK;
#endif
        }
        else
        {
#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG)
            base->VLLSCTRL |= SMC_VLLSCTRL_PORPO_MASK;
#else
            base->STOPCTRL |= SMC_STOPCTRL_PORPO_MASK;
#endif
        }
    }
#endif /* FSL_FEATURE_SMC_HAS_PORPO */

#if (defined(FSL_FEATURE_SMC_HAS_RAM2_POWER_OPTION) && FSL_FEATURE_SMC_HAS_RAM2_POWER_OPTION)
    else if (config->subMode == kSMC_StopSub2)
    {
        /* configure whether the Por Detect work in Vlls0 mode */
        if (config->enableRam2InVlls2)
        {
#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG)
            base->VLLSCTRL |= SMC_VLLSCTRL_RAM2PO_MASK;
#else
            base->STOPCTRL |= SMC_STOPCTRL_RAM2PO_MASK;
#endif
        }
        else
        {
#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG)
            base->VLLSCTRL &= ~SMC_VLLSCTRL_RAM2PO_MASK;
#else
            base->STOPCTRL &= ~SMC_STOPCTRL_RAM2PO_MASK;
#endif
        }
    }
    else
    {
    }
#endif /* FSL_FEATURE_SMC_HAS_RAM2_POWER_OPTION */

    /* configure to VLLS mode */
    reg = base->PMCTRL;
    reg &= ~SMC_PMCTRL_STOPM_MASK;
    reg |= (kSMC_StopVlls << SMC_PMCTRL_STOPM_SHIFT);
    base->PMCTRL = reg;

/* configure the VLLS sub-mode */
#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG)
    reg = base->VLLSCTRL;
    reg &= ~SMC_VLLSCTRL_VLLSM_MASK;
    reg |= ((uint32_t)config->subMode << SMC_VLLSCTRL_VLLSM_SHIFT);
    base->VLLSCTRL = reg;
#else
#if (defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE)
    reg = base->STOPCTRL;
    reg &= ~SMC_STOPCTRL_LLSM_MASK;
    reg |= ((uint32_t)config->subMode << SMC_STOPCTRL_LLSM_SHIFT);
    base->STOPCTRL = reg;
#else
    reg = base->STOPCTRL;
    reg &= ~SMC_STOPCTRL_VLLSM_MASK;
    reg |= ((uint32_t)config->subMode << SMC_STOPCTRL_VLLSM_SHIFT);
    base->STOPCTRL = reg;
#endif /* FSL_FEATURE_SMC_HAS_LLS_SUBMODE */
#endif

#if (defined(FSL_FEATURE_SMC_HAS_LPOPO) && FSL_FEATURE_SMC_HAS_LPOPO)
    if (config->enableLpoClock)
    {
        base->STOPCTRL &= ~SMC_STOPCTRL_LPOPO_MASK;
    }
    else
    {
        base->STOPCTRL |= SMC_STOPCTRL_LPOPO_MASK;
    }
#endif /* FSL_FEATURE_SMC_HAS_LPOPO */

    /* Set the SLEEPDEEP bit to enable deep sleep mode */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* read back to make sure the configuration valid before enter stop mode */
    (void)base->PMCTRL;
    SMC_EnterStopRamFunc();

    /* check whether the power mode enter LLS mode succeed */
    if (base->PMCTRL & SMC_PMCTRL_STOPA_MASK)
    {
        return kStatus_SMC_StopAbort;
    }
    else
    {
        return kStatus_Success;
    }
}
#endif /* FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE */
