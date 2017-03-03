/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_smc.h"
#include "fsl_flash.h"

#if (defined(FSL_FEATURE_SMC_HAS_PARAM) && FSL_FEATURE_SMC_HAS_PARAM)
void SMC_GetParam(SMC_Type *base, smc_param_t *param)
{
    uint32_t reg = base->PARAM;
    param->hsrunEnable = (bool)(reg & SMC_PARAM_EHSRUN_MASK);
    param->llsEnable = (bool)(reg & SMC_PARAM_ELLS_MASK);
    param->lls2Enable = (bool)(reg & SMC_PARAM_ELLS2_MASK);
    param->vlls0Enable = (bool)(reg & SMC_PARAM_EVLLS0_MASK);
}
#endif /* FSL_FEATURE_SMC_HAS_PARAM */

void SMC_PreEnterStopModes(void)
{
    flash_prefetch_speculation_status_t speculationStatus =
    {
        kFLASH_prefetchSpeculationOptionDisable, /* Disable instruction speculation.*/
        kFLASH_prefetchSpeculationOptionDisable, /* Disable data speculation.*/
    };

    __disable_irq();
    __ISB();

    /*
     * Before enter stop modes, the flash cache prefetch should be disabled.
     * Otherwise the prefetch might be interrupted by stop, then the data and
     * and instruction from flash are wrong.
     */
    FLASH_PflashSetPrefetchSpeculation(&speculationStatus);
}

void SMC_PostExitStopModes(void)
{
    flash_prefetch_speculation_status_t speculationStatus =
    {
        kFLASH_prefetchSpeculationOptionEnable, /* Enable instruction speculation.*/
        kFLASH_prefetchSpeculationOptionEnable, /* Enable data speculation.*/
    };

    FLASH_PflashSetPrefetchSpeculation(&speculationStatus);

    __enable_irq();
    __ISB();
}

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

status_t SMC_SetPowerModeWait(SMC_Type *base)
{
    /* configure Normal Wait mode */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __WFI();
    __ISB();

    return kStatus_Success;
}

status_t SMC_SetPowerModeStop(SMC_Type *base, smc_partial_stop_option_t option)
{
    uint8_t reg;

#if (defined(FSL_FEATURE_SMC_HAS_PSTOPO) && FSL_FEATURE_SMC_HAS_PSTOPO)
    /* configure the Partial Stop mode in Noraml Stop mode */
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
    __DSB();
    __WFI();
    __ISB();

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
    __DSB();
    __WFI();
    __ISB();

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
    __DSB();
    __WFI();
    __ISB();

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
    __DSB();
    __WFI();
    __ISB();

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
