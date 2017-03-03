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

#include "fsl_ftm.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address
 *
 * @param base FTM peripheral base address
 *
 * @return The FTM instance
 */
static uint32_t FTM_GetInstance(FTM_Type *base);

/*!
 * @brief Sets the FTM register PWM synchronization method
 *
 * This function will set the necessary bits for the PWM synchronization mode that
 * user wishes to use.
 *
 * @param base       FTM peripheral base address
 * @param syncMethod Syncronization methods to use to update buffered registers. This is a logical
 *                   OR of members of the enumeration ::ftm_pwm_sync_method_t
 */
static void FTM_SetPwmSync(FTM_Type *base, uint32_t syncMethod);

/*!
 * @brief Sets the reload points used as loading points for register update
 *
 * This function will set the necessary bits based on what the user wishes to use as loading
 * points for FTM register update. When using this it is not required to use PWM synchnronization.
 *
 * @param base         FTM peripheral base address
 * @param reloadPoints FTM reload points. This is a logical OR of members of the
 *                     enumeration ::ftm_reload_point_t
 */
static void FTM_SetReloadPoints(FTM_Type *base, uint32_t reloadPoints);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to FTM bases for each instance. */
static FTM_Type *const s_ftmBases[] = FTM_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to FTM clocks for each instance. */
static const clock_ip_name_t s_ftmClocks[] = FTM_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t FTM_GetInstance(FTM_Type *base)
{
    uint32_t instance;
    uint32_t ftmArrayCount = (sizeof(s_ftmBases) / sizeof(s_ftmBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ftmArrayCount; instance++)
    {
        if (s_ftmBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ftmArrayCount);

    return instance;
}

static void FTM_SetPwmSync(FTM_Type *base, uint32_t syncMethod)
{
    uint8_t chnlNumber = 0;
    uint32_t reg = 0, syncReg = 0;

    syncReg = base->SYNC;
    /* Enable PWM synchronization of output mask register */
    syncReg |= FTM_SYNC_SYNCHOM_MASK;

    reg = base->COMBINE;
    for (chnlNumber = 0; chnlNumber < (FSL_FEATURE_FTM_CHANNEL_COUNTn(base) / 2); chnlNumber++)
    {
        /* Enable PWM synchronization of registers C(n)V and C(n+1)V */
        reg |= (1U << (FTM_COMBINE_SYNCEN0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * chnlNumber)));
    }
    base->COMBINE = reg;

    reg = base->SYNCONF;

    /* Use enhanced PWM synchronization method. Use PWM sync to update register values */
    reg |= (FTM_SYNCONF_SYNCMODE_MASK | FTM_SYNCONF_CNTINC_MASK | FTM_SYNCONF_INVC_MASK | FTM_SYNCONF_SWOC_MASK);

    if (syncMethod & FTM_SYNC_SWSYNC_MASK)
    {
        /* Enable needed bits for software trigger to update registers with its buffer value */
        reg |= (FTM_SYNCONF_SWRSTCNT_MASK | FTM_SYNCONF_SWWRBUF_MASK | FTM_SYNCONF_SWINVC_MASK |
                FTM_SYNCONF_SWSOC_MASK | FTM_SYNCONF_SWOM_MASK);
    }

    if (syncMethod & (FTM_SYNC_TRIG0_MASK | FTM_SYNC_TRIG1_MASK | FTM_SYNC_TRIG2_MASK))
    {
        /* Enable needed bits for hardware trigger to update registers with its buffer value */
        reg |= (FTM_SYNCONF_HWRSTCNT_MASK | FTM_SYNCONF_HWWRBUF_MASK | FTM_SYNCONF_HWINVC_MASK |
                FTM_SYNCONF_HWSOC_MASK | FTM_SYNCONF_HWOM_MASK);

        /* Enable the appropriate hardware trigger that is used for PWM sync */
        if (syncMethod & FTM_SYNC_TRIG0_MASK)
        {
            syncReg |= FTM_SYNC_TRIG0_MASK;
        }
        if (syncMethod & FTM_SYNC_TRIG1_MASK)
        {
            syncReg |= FTM_SYNC_TRIG1_MASK;
        }
        if (syncMethod & FTM_SYNC_TRIG2_MASK)
        {
            syncReg |= FTM_SYNC_TRIG2_MASK;
        }
    }

    /* Write back values to the SYNC register */
    base->SYNC = syncReg;

    /* Write the PWM synch values to the SYNCONF register */
    base->SYNCONF = reg;
}

static void FTM_SetReloadPoints(FTM_Type *base, uint32_t reloadPoints)
{
    uint32_t chnlNumber = 0;
    uint32_t reg = 0;

    /* Need CNTINC bit to be 1 for CNTIN register to update with its buffer value on reload  */
    base->SYNCONF |= FTM_SYNCONF_CNTINC_MASK;

    reg = base->COMBINE;
    for (chnlNumber = 0; chnlNumber < (FSL_FEATURE_FTM_CHANNEL_COUNTn(base) / 2); chnlNumber++)
    {
        /* Need SYNCEN bit to be 1 for CnV reg to update with its buffer value on reload  */
        reg |= (1U << (FTM_COMBINE_SYNCEN0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * chnlNumber)));
    }
    base->COMBINE = reg;

    /* Set the reload points */
    reg = base->PWMLOAD;

    /* Enable the selected channel match reload points */
    reg &= ~((1U << FSL_FEATURE_FTM_CHANNEL_COUNTn(base)) - 1);
    reg |= (reloadPoints & ((1U << FSL_FEATURE_FTM_CHANNEL_COUNTn(base)) - 1));

#if defined(FSL_FEATURE_FTM_HAS_HALFCYCLE_RELOAD) && (FSL_FEATURE_FTM_HAS_HALFCYCLE_RELOAD)
    /* Enable half cycle match as a reload point */
    if (reloadPoints & kFTM_HalfCycMatch)
    {
        reg |= FTM_PWMLOAD_HCSEL_MASK;
    }
    else
    {
        reg &= ~FTM_PWMLOAD_HCSEL_MASK;
    }
#endif /* FSL_FEATURE_FTM_HAS_HALFCYCLE_RELOAD */

    base->PWMLOAD = reg;

    /* These reload points are used when counter is in up-down counting mode */
    reg = base->SYNC;
    if (reloadPoints & kFTM_CntMax)
    {
        /* Reload when counter turns from up to down */
        reg |= FTM_SYNC_CNTMAX_MASK;
    }
    else
    {
        reg &= ~FTM_SYNC_CNTMAX_MASK;
    }

    if (reloadPoints & kFTM_CntMin)
    {
        /* Reload when counter turns from down to up */
        reg |= FTM_SYNC_CNTMIN_MASK;
    }
    else
    {
        reg &= ~FTM_SYNC_CNTMIN_MASK;
    }
    base->SYNC = reg;
}

status_t FTM_Init(FTM_Type *base, const ftm_config_t *config)
{
    assert(config);

    uint32_t reg;

    if (!(config->pwmSyncMode &
          (FTM_SYNC_TRIG0_MASK | FTM_SYNC_TRIG1_MASK | FTM_SYNC_TRIG2_MASK | FTM_SYNC_SWSYNC_MASK)))
    {
        /* Invalid PWM sync mode */
        return kStatus_Fail;
    }

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate the FTM clock*/
    CLOCK_EnableClock(s_ftmClocks[FTM_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Configure the fault mode, enable FTM mode and disable write protection */
    base->MODE = FTM_MODE_FAULTM(config->faultMode) | FTM_MODE_FTMEN_MASK | FTM_MODE_WPDIS_MASK;

    /* Configure the update mechanism for buffered registers */
    FTM_SetPwmSync(base, config->pwmSyncMode);

    /* Setup intermediate register reload points */
    FTM_SetReloadPoints(base, config->reloadPoints);

    /* Set the clock prescale factor */
    base->SC = FTM_SC_PS(config->prescale);

    /* Setup the counter operation */
    base->CONF = (FTM_CONF_BDMMODE(config->bdmMode) | FTM_CONF_GTBEEN(config->useGlobalTimeBase));

    /* Initial state of channel output */
    base->OUTINIT = config->chnlInitState;

    /* Channel polarity */
    base->POL = config->chnlPolarity;

    /* Set the external trigger sources */
    base->EXTTRIG = config->extTriggers;
#if defined(FSL_FEATURE_FTM_HAS_RELOAD_INITIALIZATION_TRIGGER) && (FSL_FEATURE_FTM_HAS_RELOAD_INITIALIZATION_TRIGGER)
    if (config->extTriggers & kFTM_ReloadInitTrigger)
    {
        base->CONF |= FTM_CONF_ITRIGR_MASK;
    }
    else
    {
        base->CONF &= ~FTM_CONF_ITRIGR_MASK;
    }
#endif /* FSL_FEATURE_FTM_HAS_RELOAD_INITIALIZATION_TRIGGER */

    /* FTM deadtime insertion control */
    base->DEADTIME = (0u | 
#if defined(FSL_FEATURE_FTM_HAS_EXTENDED_DEADTIME_VALUE) && (FSL_FEATURE_FTM_HAS_EXTENDED_DEADTIME_VALUE)
                        /* Has extended deadtime value register) */
                        FTM_DEADTIME_DTVALEX(config->deadTimeValue >> 6) | 
#endif /* FSL_FEATURE_FTM_HAS_EXTENDED_DEADTIME_VALUE */
                        FTM_DEADTIME_DTPS(config->deadTimePrescale) | 
                        FTM_DEADTIME_DTVAL(config->deadTimeValue));

    /* FTM fault filter value */
    reg = base->FLTCTRL;
    reg &= ~FTM_FLTCTRL_FFVAL_MASK;
    reg |= FTM_FLTCTRL_FFVAL(config->faultFilterValue);
    base->FLTCTRL = reg;

    return kStatus_Success;
}

void FTM_Deinit(FTM_Type *base)
{
    /* Set clock source to none to disable counter */
    base->SC &= ~(FTM_SC_CLKS_MASK);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the FTM clock */
    CLOCK_DisableClock(s_ftmClocks[FTM_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void FTM_GetDefaultConfig(ftm_config_t *config)
{
    assert(config);

    /* Divide FTM clock by 1 */
    config->prescale = kFTM_Prescale_Divide_1;
    /* FTM behavior in BDM mode */
    config->bdmMode = kFTM_BdmMode_0;
    /* Software trigger will be used to update registers */
    config->pwmSyncMode = kFTM_SoftwareTrigger;
    /* No intermediate register load */
    config->reloadPoints = 0;
    /* Fault control disabled for all channels */
    config->faultMode = kFTM_Fault_Disable;
    /* Disable the fault filter */
    config->faultFilterValue = 0;
    /* Divide the system clock by 1 */
    config->deadTimePrescale = kFTM_Deadtime_Prescale_1;
    /* No counts are inserted */
    config->deadTimeValue = 0;
    /* No external trigger */
    config->extTriggers = 0;
    /* Initialization value is 0 for all channels */
    config->chnlInitState = 0;
    /* Active high polarity for all channels */
    config->chnlPolarity = 0;
    /* Use internal FTM counter as timebase */
    config->useGlobalTimeBase = false;
}

status_t FTM_SetupPwm(FTM_Type *base,
                      const ftm_chnl_pwm_signal_param_t *chnlParams,
                      uint8_t numOfChnls,
                      ftm_pwm_mode_t mode,
                      uint32_t pwmFreq_Hz,
                      uint32_t srcClock_Hz)
{
    assert(chnlParams);
    assert(srcClock_Hz);
    assert(pwmFreq_Hz);
    assert(numOfChnls);

    uint32_t mod, reg;
    uint32_t ftmClock = (srcClock_Hz / (1U << (base->SC & FTM_SC_PS_MASK)));
    uint16_t cnv, cnvFirstEdge;
    uint8_t i;

    switch (mode)
    {
        case kFTM_EdgeAlignedPwm:
        case kFTM_CombinedPwm:
            base->SC &= ~FTM_SC_CPWMS_MASK;
            mod = (ftmClock / pwmFreq_Hz) - 1;
            break;
        case kFTM_CenterAlignedPwm:
            base->SC |= FTM_SC_CPWMS_MASK;
            mod = ftmClock / (pwmFreq_Hz * 2);
            break;
        default:
            return kStatus_Fail;
    }

    /* Return an error in case we overflow the registers, probably would require changing
     * clock source to get the desired frequency */
    if (mod > 65535U)
    {
        return kStatus_Fail;
    }
    /* Set the PWM period */
    base->MOD = mod;

    /* Setup each FTM channel */
    for (i = 0; i < numOfChnls; i++)
    {
        /* Return error if requested dutycycle is greater than the max allowed */
        if (chnlParams->dutyCyclePercent > 100)
        {
            return kStatus_Fail;
        }

        if ((mode == kFTM_EdgeAlignedPwm) || (mode == kFTM_CenterAlignedPwm))
        {
            /* Clear the current mode and edge level bits */
            reg = base->CONTROLS[chnlParams->chnlNumber].CnSC;
            reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);

            /* Setup the active level */
            reg |= (uint32_t)(chnlParams->level << FTM_CnSC_ELSA_SHIFT);

            /* Edge-aligned mode needs MSB to be 1, don't care for Center-aligned mode */
            reg |= FTM_CnSC_MSB(1U);

            /* Update the mode and edge level */
            base->CONTROLS[chnlParams->chnlNumber].CnSC = reg;

            if (chnlParams->dutyCyclePercent == 0)
            {
                /* Signal stays low */
                cnv = 0;
            }
            else
            {
                cnv = (mod * chnlParams->dutyCyclePercent) / 100;
                /* For 100% duty cycle */
                if (cnv >= mod)
                {
                    cnv = mod + 1;
                }
            }

            base->CONTROLS[chnlParams->chnlNumber].CnV = cnv;
#if defined(FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT) && (FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT)
            /* Set to output mode */
            FTM_SetPwmOutputEnable(base, chnlParams->chnlNumber, true);
#endif
        }
        else
        {
            /* This check is added for combined mode as the channel number should be the pair number */
            if (chnlParams->chnlNumber >= (FSL_FEATURE_FTM_CHANNEL_COUNTn(base) / 2))
            {
                return kStatus_Fail;
            }

            /* Return error if requested value is greater than the max allowed */
            if (chnlParams->firstEdgeDelayPercent > 100)
            {
                return kStatus_Fail;
            }

            /* Configure delay of the first edge */
            if (chnlParams->firstEdgeDelayPercent == 0)
            {
                /* No delay for the first edge */
                cnvFirstEdge = 0;
            }
            else
            {
                cnvFirstEdge = (mod * chnlParams->firstEdgeDelayPercent) / 100;
            }

            /* Configure dutycycle */
            if (chnlParams->dutyCyclePercent == 0)
            {
                /* Signal stays low */
                cnv = 0;
                cnvFirstEdge = 0;
            }
            else
            {
                cnv = (mod * chnlParams->dutyCyclePercent) / 100;
                /* For 100% duty cycle */
                if (cnv >= mod)
                {
                    cnv = mod + 1;
                }
            }

            /* Clear the current mode and edge level bits for channel n */
            reg = base->CONTROLS[chnlParams->chnlNumber * 2].CnSC;
            reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);

            /* Setup the active level for channel n */
            reg |= (uint32_t)(chnlParams->level << FTM_CnSC_ELSA_SHIFT);

            /* Update the mode and edge level for channel n */
            base->CONTROLS[chnlParams->chnlNumber * 2].CnSC = reg;

            /* Clear the current mode and edge level bits for channel n + 1 */
            reg = base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnSC;
            reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);

            /* Setup the active level for channel n + 1 */
            reg |= (uint32_t)(chnlParams->level << FTM_CnSC_ELSA_SHIFT);

            /* Update the mode and edge level for channel n + 1*/
            base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnSC = reg;

            /* Set the combine bit for the channel pair */
            base->COMBINE |=
                (1U << (FTM_COMBINE_COMBINE0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * chnlParams->chnlNumber)));

            /* Set the channel pair values */
            base->CONTROLS[chnlParams->chnlNumber * 2].CnV = cnvFirstEdge;
            base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnV = cnvFirstEdge + cnv;

#if defined(FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT) && (FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT)
            /* Set to output mode */
            FTM_SetPwmOutputEnable(base, (ftm_chnl_t)((uint8_t)chnlParams->chnlNumber * 2), true);
            FTM_SetPwmOutputEnable(base, (ftm_chnl_t)((uint8_t)chnlParams->chnlNumber * 2 + 1), true);
#endif
        }
        chnlParams++;
    }

    return kStatus_Success;
}

void FTM_UpdatePwmDutycycle(FTM_Type *base,
                            ftm_chnl_t chnlNumber,
                            ftm_pwm_mode_t currentPwmMode,
                            uint8_t dutyCyclePercent)
{
    uint16_t cnv, cnvFirstEdge = 0, mod;

    mod = base->MOD;
    if ((currentPwmMode == kFTM_EdgeAlignedPwm) || (currentPwmMode == kFTM_CenterAlignedPwm))
    {
        cnv = (mod * dutyCyclePercent) / 100;
        /* For 100% duty cycle */
        if (cnv >= mod)
        {
            cnv = mod + 1;
        }
        base->CONTROLS[chnlNumber].CnV = cnv;
    }
    else
    {
        /* This check is added for combined mode as the channel number should be the pair number */
        if (chnlNumber >= (FSL_FEATURE_FTM_CHANNEL_COUNTn(base) / 2))
        {
            return;
        }

        cnv = (mod * dutyCyclePercent) / 100;
        cnvFirstEdge = base->CONTROLS[chnlNumber * 2].CnV;
        /* For 100% duty cycle */
        if (cnv >= mod)
        {
            cnv = mod + 1;
        }
        base->CONTROLS[(chnlNumber * 2) + 1].CnV = cnvFirstEdge + cnv;
    }
}

void FTM_UpdateChnlEdgeLevelSelect(FTM_Type *base, ftm_chnl_t chnlNumber, uint8_t level)
{
    uint32_t reg = base->CONTROLS[chnlNumber].CnSC;

    /* Clear the field and write the new level value */
    reg &= ~(FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);
    reg |= ((uint32_t)level << FTM_CnSC_ELSA_SHIFT) & (FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);

    base->CONTROLS[chnlNumber].CnSC = reg;
}

void FTM_SetupInputCapture(FTM_Type *base,
                           ftm_chnl_t chnlNumber,
                           ftm_input_capture_edge_t captureMode,
                           uint32_t filterValue)
{
    uint32_t reg;

    /* Clear the combine bit for the channel pair */
    base->COMBINE &= ~(1U << (FTM_COMBINE_COMBINE0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * (chnlNumber >> 1))));
    /* Clear the dual edge capture mode because it's it's higher priority */
    base->COMBINE &= ~(1U << (FTM_COMBINE_DECAPEN0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * (chnlNumber >> 1))));
    /* Clear the quadrature decoder mode beacause it's higher priority */
    base->QDCTRL &= ~FTM_QDCTRL_QUADEN_MASK;

    reg = base->CONTROLS[chnlNumber].CnSC;
    reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);
    reg |= captureMode;

    /* Set the requested input capture mode */
    base->CONTROLS[chnlNumber].CnSC = reg;
    /* Input filter available only for channels 0, 1, 2, 3 */
    if (chnlNumber < kFTM_Chnl_4)
    {
        reg = base->FILTER;
        reg &= ~(FTM_FILTER_CH0FVAL_MASK << (FTM_FILTER_CH1FVAL_SHIFT * chnlNumber));
        reg |= (filterValue << (FTM_FILTER_CH1FVAL_SHIFT * chnlNumber));
        base->FILTER = reg;
    }
#if defined(FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT) && (FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT)
    /* Set to input mode */
    FTM_SetPwmOutputEnable(base, chnlNumber, false);
#endif
}

void FTM_SetupOutputCompare(FTM_Type *base,
                            ftm_chnl_t chnlNumber,
                            ftm_output_compare_mode_t compareMode,
                            uint32_t compareValue)
{
    uint32_t reg;

    /* Clear the combine bit for the channel pair */
    base->COMBINE &= ~(1U << (FTM_COMBINE_COMBINE0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * (chnlNumber >> 1))));
    /* Clear the dual edge capture mode because it's it's higher priority */
    base->COMBINE &= ~(1U << (FTM_COMBINE_DECAPEN0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * (chnlNumber >> 1))));
    /* Clear the quadrature decoder mode beacause it's higher priority */
    base->QDCTRL &= ~FTM_QDCTRL_QUADEN_MASK;

    reg = base->CONTROLS[chnlNumber].CnSC;
    reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);
    reg |= compareMode;
    /* Setup the channel output behaviour when a match occurs with the compare value */
    base->CONTROLS[chnlNumber].CnSC = reg;

    /* Set output on match to the requested level */
    base->CONTROLS[chnlNumber].CnV = compareValue;

#if defined(FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT) && (FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT)
    /* Set to output mode */
    FTM_SetPwmOutputEnable(base, chnlNumber, true);
#endif
}

void FTM_SetupDualEdgeCapture(FTM_Type *base,
                              ftm_chnl_t chnlPairNumber,
                              const ftm_dual_edge_capture_param_t *edgeParam,
                              uint32_t filterValue)
{
    assert(edgeParam);

    uint32_t reg;

    reg = base->COMBINE;
    /* Clear the combine bit for the channel pair */
    reg &= ~(1U << (FTM_COMBINE_COMBINE0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * chnlPairNumber)));
    /* Enable the DECAPEN bit */
    reg |= (1U << (FTM_COMBINE_DECAPEN0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * chnlPairNumber)));
    reg |= (1U << (FTM_COMBINE_DECAP0_SHIFT + (FTM_COMBINE_COMBINE1_SHIFT * chnlPairNumber)));
    base->COMBINE = reg;

    /* Setup the edge detection from channel n and n + 1 */
    reg = base->CONTROLS[chnlPairNumber * 2].CnSC;
    reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);
    reg |= ((uint32_t)edgeParam->mode | (uint32_t)edgeParam->currChanEdgeMode);
    base->CONTROLS[chnlPairNumber * 2].CnSC = reg;

    reg = base->CONTROLS[(chnlPairNumber * 2) + 1].CnSC;
    reg &= ~(FTM_CnSC_MSA_MASK | FTM_CnSC_MSB_MASK | FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK);
    reg |= ((uint32_t)edgeParam->mode | (uint32_t)edgeParam->nextChanEdgeMode);
    base->CONTROLS[(chnlPairNumber * 2) + 1].CnSC = reg;

    /* Input filter available only for channels 0, 1, 2, 3 */
    if (chnlPairNumber < kFTM_Chnl_4)
    {
        reg = base->FILTER;
        reg &= ~(FTM_FILTER_CH0FVAL_MASK << (FTM_FILTER_CH1FVAL_SHIFT * chnlPairNumber));
        reg |= (filterValue << (FTM_FILTER_CH1FVAL_SHIFT * chnlPairNumber));
        base->FILTER = reg;
    }

#if defined(FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT) && (FSL_FEATURE_FTM_HAS_ENABLE_PWM_OUTPUT)
    /* Set to input mode */
    FTM_SetPwmOutputEnable(base, chnlPairNumber, false);
#endif
}

void FTM_SetupQuadDecode(FTM_Type *base,
                         const ftm_phase_params_t *phaseAParams,
                         const ftm_phase_params_t *phaseBParams,
                         ftm_quad_decode_mode_t quadMode)
{
    assert(phaseAParams);
    assert(phaseBParams);

    uint32_t reg;

    /* Set Phase A filter value if phase filter is enabled */
    if (phaseAParams->enablePhaseFilter)
    {
        reg = base->FILTER;
        reg &= ~(FTM_FILTER_CH0FVAL_MASK);
        reg |= FTM_FILTER_CH0FVAL(phaseAParams->phaseFilterVal);
        base->FILTER = reg;
    }

    /* Set Phase B filter value if phase filter is enabled */
    if (phaseBParams->enablePhaseFilter)
    {
        reg = base->FILTER;
        reg &= ~(FTM_FILTER_CH1FVAL_MASK);
        reg |= FTM_FILTER_CH1FVAL(phaseBParams->phaseFilterVal);
        base->FILTER = reg;
    }

    /* Set Quadrature decode properties */
    reg = base->QDCTRL;
    reg &= ~(FTM_QDCTRL_QUADMODE_MASK | FTM_QDCTRL_PHAFLTREN_MASK | FTM_QDCTRL_PHBFLTREN_MASK | FTM_QDCTRL_PHAPOL_MASK |
             FTM_QDCTRL_PHBPOL_MASK);
    reg |= (FTM_QDCTRL_QUADMODE(quadMode) | FTM_QDCTRL_PHAFLTREN(phaseAParams->enablePhaseFilter) |
            FTM_QDCTRL_PHBFLTREN(phaseBParams->enablePhaseFilter) | FTM_QDCTRL_PHAPOL(phaseAParams->phasePolarity) |
            FTM_QDCTRL_PHBPOL(phaseBParams->phasePolarity));
    base->QDCTRL = reg;
    /* Enable Quad decode */
    base->QDCTRL |= FTM_QDCTRL_QUADEN_MASK;
}

void FTM_SetupFault(FTM_Type *base, ftm_fault_input_t faultNumber, const ftm_fault_param_t *faultParams)
{
    assert(faultParams);

    uint32_t reg;

    reg = base->FLTCTRL;
    if (faultParams->enableFaultInput)
    {
        /* Enable the fault input */
        reg |= (FTM_FLTCTRL_FAULT0EN_MASK << faultNumber);
    }
    else
    {
        /* Disable the fault input */
        reg &= ~(FTM_FLTCTRL_FAULT0EN_MASK << faultNumber);
    }

    if (faultParams->useFaultFilter)
    {
        /* Enable the fault filter */
        reg |= (FTM_FLTCTRL_FFLTR0EN_MASK << (FTM_FLTCTRL_FFLTR0EN_SHIFT + faultNumber));
    }
    else
    {
        /* Disable the fault filter */
        reg &= ~(FTM_FLTCTRL_FFLTR0EN_MASK << (FTM_FLTCTRL_FFLTR0EN_SHIFT + faultNumber));
    }
    base->FLTCTRL = reg;

    if (faultParams->faultLevel)
    {
        /* Active low polarity for the fault input pin */
        base->FLTPOL |= (1U << faultNumber);
    }
    else
    {
        /* Active high polarity for the fault input pin */
        base->FLTPOL &= ~(1U << faultNumber);
    }
}

void FTM_EnableInterrupts(FTM_Type *base, uint32_t mask)
{
    uint32_t chnlInts = (mask & 0xFFU);
    uint8_t chnlNumber = 0;

    /* Enable the timer overflow interrupt */
    if (mask & kFTM_TimeOverflowInterruptEnable)
    {
        base->SC |= FTM_SC_TOIE_MASK;
    }

    /* Enable the fault interrupt */
    if (mask & kFTM_FaultInterruptEnable)
    {
        base->MODE |= FTM_MODE_FAULTIE_MASK;
    }

#if defined(FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT) && (FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT)
    /* Enable the reload interrupt available only on certain SoC's */
    if (mask & kFTM_ReloadInterruptEnable)
    {
        base->SC |= FTM_SC_RIE_MASK;
    }
#endif

    /* Enable the channel interrupts */
    while (chnlInts)
    {
        if (chnlInts & 0x1)
        {
            base->CONTROLS[chnlNumber].CnSC |= FTM_CnSC_CHIE_MASK;
        }
        chnlNumber++;
        chnlInts = chnlInts >> 1U;
    }
}

void FTM_DisableInterrupts(FTM_Type *base, uint32_t mask)
{
    uint32_t chnlInts = (mask & 0xFF);
    uint8_t chnlNumber = 0;

    /* Disable the timer overflow interrupt */
    if (mask & kFTM_TimeOverflowInterruptEnable)
    {
        base->SC &= ~FTM_SC_TOIE_MASK;
    }
    /* Disable the fault interrupt */
    if (mask & kFTM_FaultInterruptEnable)
    {
        base->MODE &= ~FTM_MODE_FAULTIE_MASK;
    }

#if defined(FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT) && (FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT)
    /* Disable the reload interrupt available only on certain SoC's */
    if (mask & kFTM_ReloadInterruptEnable)
    {
        base->SC &= ~FTM_SC_RIE_MASK;
    }
#endif

    /* Disable the channel interrupts */
    while (chnlInts)
    {
        if (chnlInts & 0x1)
        {
            base->CONTROLS[chnlNumber].CnSC &= ~FTM_CnSC_CHIE_MASK;
        }
        chnlNumber++;
        chnlInts = chnlInts >> 1U;
    }
}

uint32_t FTM_GetEnabledInterrupts(FTM_Type *base)
{
    uint32_t enabledInterrupts = 0;
    int8_t chnlCount = FSL_FEATURE_FTM_CHANNEL_COUNTn(base);

    /* The CHANNEL_COUNT macro returns -1 if it cannot match the FTM instance */
    assert(chnlCount != -1);

    /* Check if timer overflow interrupt is enabled */
    if (base->SC & FTM_SC_TOIE_MASK)
    {
        enabledInterrupts |= kFTM_TimeOverflowInterruptEnable;
    }
    /* Check if fault interrupt is enabled */
    if (base->MODE & FTM_MODE_FAULTIE_MASK)
    {
        enabledInterrupts |= kFTM_FaultInterruptEnable;
    }

#if defined(FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT) && (FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT)
    /* Check if the reload interrupt is enabled */
    if (base->SC & FTM_SC_RIE_MASK)
    {
        enabledInterrupts |= kFTM_ReloadInterruptEnable;
    }
#endif

    /* Check if the channel interrupts are enabled */
    while (chnlCount > 0)
    {
        chnlCount--;
        if (base->CONTROLS[chnlCount].CnSC & FTM_CnSC_CHIE_MASK)
        {
            enabledInterrupts |= (1U << chnlCount);
        }
    }

    return enabledInterrupts;
}

uint32_t FTM_GetStatusFlags(FTM_Type *base)
{
    uint32_t statusFlags = 0;

    /* Check the timer flag */
    if (base->SC & FTM_SC_TOF_MASK)
    {
        statusFlags |= kFTM_TimeOverflowFlag;
    }
    /* Check fault flag */
    if (base->FMS & FTM_FMS_FAULTF_MASK)
    {
        statusFlags |= kFTM_FaultFlag;
    }
    /* Check channel trigger flag */
    if (base->EXTTRIG & FTM_EXTTRIG_TRIGF_MASK)
    {
        statusFlags |= kFTM_ChnlTriggerFlag;
    }
#if defined(FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT) && (FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT)
    /* Check reload flag */
    if (base->SC & FTM_SC_RF_MASK)
    {
        statusFlags |= kFTM_ReloadFlag;
    }
#endif

    /* Lower 8 bits contain the channel status flags */
    statusFlags |= (base->STATUS & 0xFFU);

    return statusFlags;
}

void FTM_ClearStatusFlags(FTM_Type *base, uint32_t mask)
{
    /* Clear the timer overflow flag by writing a 0 to the bit while it is set */
    if (mask & kFTM_TimeOverflowFlag)
    {
        base->SC &= ~FTM_SC_TOF_MASK;
    }
    /* Clear fault flag by writing a 0 to the bit while it is set */
    if (mask & kFTM_FaultFlag)
    {
        base->FMS &= ~FTM_FMS_FAULTF_MASK;
    }
    /* Clear channel trigger flag */
    if (mask & kFTM_ChnlTriggerFlag)
    {
        base->EXTTRIG &= ~FTM_EXTTRIG_TRIGF_MASK;
    }

#if defined(FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT) && (FSL_FEATURE_FTM_HAS_RELOAD_INTERRUPT)
    /* Check reload flag by writing a 0 to the bit while it is set */
    if (mask & kFTM_ReloadFlag)
    {
        base->SC &= ~FTM_SC_RF_MASK;
    }
#endif
    /* Clear the channel status flags by writing a 0 to the bit */
    base->STATUS &= ~(mask & 0xFFU);
}
