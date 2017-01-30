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

#include "fsl_tpm.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define TPM_COMBINE_SHIFT (8U)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Gets the instance from the base address
 *
 * @param base TPM peripheral base address
 *
 * @return The TPM instance
 */
static uint32_t TPM_GetInstance(TPM_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to TPM bases for each instance. */
static TPM_Type *const s_tpmBases[] = TPM_BASE_PTRS;

/*! @brief Pointers to TPM clocks for each instance. */
static const clock_ip_name_t s_tpmClocks[] = TPM_CLOCKS;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t TPM_GetInstance(TPM_Type *base)
{
    uint32_t instance;
    uint32_t tpmArrayCount = (sizeof(s_tpmBases) / sizeof(s_tpmBases[0]));

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < tpmArrayCount; instance++)
    {
        if (s_tpmBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < tpmArrayCount);

    return instance;
}

void TPM_Init(TPM_Type *base, const tpm_config_t *config)
{
    assert(config);

    /* Enable the module clock */
    CLOCK_EnableClock(s_tpmClocks[TPM_GetInstance(base)]);

#if defined(FSL_FEATURE_TPM_HAS_GLOBAL) && FSL_FEATURE_TPM_HAS_GLOBAL
    /* TPM reset is available on certain SoC's */
    TPM_Reset(base);
#endif

    /* Set the clock prescale factor */
    base->SC = TPM_SC_PS(config->prescale);

    /* Setup the counter operation */
    base->CONF = TPM_CONF_DOZEEN(config->enableDoze) | TPM_CONF_DBGMODE(config->enableDebugMode) |
                 TPM_CONF_GTBEEN(config->useGlobalTimeBase) | TPM_CONF_CROT(config->enableReloadOnTrigger) |
                 TPM_CONF_CSOT(config->enableStartOnTrigger) | TPM_CONF_CSOO(config->enableStopOnOverflow) |
#if defined(FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER) && FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER
                 TPM_CONF_CPOT(config->enablePauseOnTrigger) |
#endif
#if defined(FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION) && FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION
                 TPM_CONF_TRGSRC(config->triggerSource) |
#endif
                 TPM_CONF_TRGSEL(config->triggerSelect);
}

void TPM_Deinit(TPM_Type *base)
{
    /* Stop the counter */
    base->SC &= ~TPM_SC_CMOD_MASK;
    /* Gate the TPM clock */
    CLOCK_DisableClock(s_tpmClocks[TPM_GetInstance(base)]);
}

void TPM_GetDefaultConfig(tpm_config_t *config)
{
    assert(config);

    /* TPM clock divide by 1 */
    config->prescale = kTPM_Prescale_Divide_1;
    /* Use internal TPM counter as timebase */
    config->useGlobalTimeBase = false;
    /* TPM counter continues in doze mode */
    config->enableDoze = false;
    /* TPM counter pauses when in debug mode */
    config->enableDebugMode = false;
    /* TPM counter will not be reloaded on input trigger */
    config->enableReloadOnTrigger = false;
    /* TPM counter continues running after overflow */
    config->enableStopOnOverflow = false;
    /* TPM counter starts immediately once it is enabled */
    config->enableStartOnTrigger = false;
#if defined(FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER) && FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER
    config->enablePauseOnTrigger = false;
#endif
    /* Choose trigger select 0 as input trigger for controlling counter operation */
    config->triggerSelect = kTPM_Trigger_Select_0;
#if defined(FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION) && FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION
    /* Choose external trigger source to control counter operation */
    config->triggerSource = kTPM_TriggerSource_External;
#endif
}

status_t TPM_SetupPwm(TPM_Type *base,
                      const tpm_chnl_pwm_signal_param_t *chnlParams,
                      uint8_t numOfChnls,
                      tpm_pwm_mode_t mode,
                      uint32_t pwmFreq_Hz,
                      uint32_t srcClock_Hz)
{
    assert(chnlParams);

    uint32_t mod;
    uint32_t tpmClock = (srcClock_Hz / (1U << (base->SC & TPM_SC_PS_MASK)));
    uint16_t cnv;
    uint8_t i;

    switch (mode)
    {
        case kTPM_EdgeAlignedPwm:
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
        case kTPM_CombinedPwm:
#endif
            base->SC &= ~TPM_SC_CPWMS_MASK;
            mod = (tpmClock / pwmFreq_Hz) - 1;
            break;
        case kTPM_CenterAlignedPwm:
            base->SC |= TPM_SC_CPWMS_MASK;
            mod = tpmClock / (pwmFreq_Hz * 2);
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

    /* Setup each TPM channel */
    for (i = 0; i < numOfChnls; i++)
    {
        /* Return error if requested dutycycle is greater than the max allowed */
        if (chnlParams->dutyCyclePercent > 100)
        {
            return kStatus_Fail;
        }
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
        if (mode == kTPM_CombinedPwm)
        {
            uint16_t cnvFirstEdge;

            /* This check is added for combined mode as the channel number should be the pair number */
            if (chnlParams->chnlNumber >= (FSL_FEATURE_TPM_CHANNEL_COUNTn(base) / 2))
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
            /* Set the channel pair values */
            base->CONTROLS[chnlParams->chnlNumber * 2].CnV = cnvFirstEdge;
            base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnV = cnvFirstEdge + cnv;

            /* Set the combine bit for the channel pair */
            base->COMBINE |= (1U << (TPM_COMBINE_COMBINE0_SHIFT + (TPM_COMBINE_SHIFT * chnlParams->chnlNumber)));

            /* When switching mode, disable channel n first */
            base->CONTROLS[chnlParams->chnlNumber * 2].CnSC &=
                ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

            /* Wait till mode change to disable channel is acknowledged */
            while ((base->CONTROLS[chnlParams->chnlNumber * 2].CnSC &
                    (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
            {
            }

            /* Set the requested PWM mode for channel n, PWM output requires mode select to be set to 2 */
            base->CONTROLS[chnlParams->chnlNumber * 2].CnSC |=
                ((chnlParams->level << TPM_CnSC_ELSA_SHIFT) | (2U << TPM_CnSC_MSA_SHIFT));

            /* Wait till mode change is acknowledged */
            while (!(base->CONTROLS[chnlParams->chnlNumber * 2].CnSC &
                     (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
            {
            }

            /* When switching mode, disable channel n + 1 first */
            base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnSC &=
                ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

            /* Wait till mode change to disable channel is acknowledged */
            while ((base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnSC &
                    (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
            {
            }

            /* Set the requested PWM mode for channel n + 1, PWM output requires mode select to be set to 2 */
            base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnSC |=
                ((chnlParams->level << TPM_CnSC_ELSA_SHIFT) | (2U << TPM_CnSC_MSA_SHIFT));

            /* Wait till mode change is acknowledged */
            while (!(base->CONTROLS[(chnlParams->chnlNumber * 2) + 1].CnSC &
                     (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
            {
            }
        }
        else
        {
#endif
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

            /* When switching mode, disable channel first */
            base->CONTROLS[chnlParams->chnlNumber].CnSC &=
                ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

            /* Wait till mode change to disable channel is acknowledged */
            while ((base->CONTROLS[chnlParams->chnlNumber].CnSC &
                    (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
            {
            }

            /* Set the requested PWM mode, PWM output requires mode select to be set to 2 */
            base->CONTROLS[chnlParams->chnlNumber].CnSC |=
                ((chnlParams->level << TPM_CnSC_ELSA_SHIFT) | (2U << TPM_CnSC_MSA_SHIFT));

            /* Wait till mode change is acknowledged */
            while (!(base->CONTROLS[chnlParams->chnlNumber].CnSC &
                     (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
            {
            }
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
        }
#endif

        chnlParams++;
    }

    return kStatus_Success;
}

void TPM_UpdatePwmDutycycle(TPM_Type *base,
                            tpm_chnl_t chnlNumber,
                            tpm_pwm_mode_t currentPwmMode,
                            uint8_t dutyCyclePercent)
{
    assert(chnlNumber < FSL_FEATURE_TPM_CHANNEL_COUNTn(base));

    uint16_t cnv, mod;

    mod = base->MOD;
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
    if (currentPwmMode == kTPM_CombinedPwm)
    {
        uint16_t cnvFirstEdge;

        /* This check is added for combined mode as the channel number should be the pair number */
        if (chnlNumber >= (FSL_FEATURE_TPM_CHANNEL_COUNTn(base) / 2))
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
    else
    {
#endif
        cnv = (mod * dutyCyclePercent) / 100;
        /* For 100% duty cycle */
        if (cnv >= mod)
        {
            cnv = mod + 1;
        }
        base->CONTROLS[chnlNumber].CnV = cnv;
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
    }
#endif
}

void TPM_UpdateChnlEdgeLevelSelect(TPM_Type *base, tpm_chnl_t chnlNumber, uint8_t level)
{
    assert(chnlNumber < FSL_FEATURE_TPM_CHANNEL_COUNTn(base));

    uint32_t reg = base->CONTROLS[chnlNumber].CnSC & ~(TPM_CnSC_CHF_MASK);

    /* When switching mode, disable channel first  */
    base->CONTROLS[chnlNumber].CnSC &=
        ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

    /* Wait till mode change to disable channel is acknowledged */
    while ((base->CONTROLS[chnlNumber].CnSC &
            (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }

    /* Clear the field and write the new level value */
    reg &= ~(TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);
    reg |= ((uint32_t)level << TPM_CnSC_ELSA_SHIFT) & (TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

    base->CONTROLS[chnlNumber].CnSC = reg;

    /* Wait till mode change is acknowledged */
    reg &= (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);
    while (reg != (base->CONTROLS[chnlNumber].CnSC &
                   (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }
}

void TPM_SetupInputCapture(TPM_Type *base, tpm_chnl_t chnlNumber, tpm_input_capture_edge_t captureMode)
{
    assert(chnlNumber < FSL_FEATURE_TPM_CHANNEL_COUNTn(base));

    /* When switching mode, disable channel first  */
    base->CONTROLS[chnlNumber].CnSC &=
        ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

    /* Wait till mode change to disable channel is acknowledged */
    while ((base->CONTROLS[chnlNumber].CnSC &
            (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }

    /* Set the requested input capture mode */
    base->CONTROLS[chnlNumber].CnSC |= captureMode;

    /* Wait till mode change is acknowledged */
    while (!(base->CONTROLS[chnlNumber].CnSC &
             (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }
}

void TPM_SetupOutputCompare(TPM_Type *base,
                            tpm_chnl_t chnlNumber,
                            tpm_output_compare_mode_t compareMode,
                            uint32_t compareValue)
{
    assert(chnlNumber < FSL_FEATURE_TPM_CHANNEL_COUNTn(base));

    /* When switching mode, disable channel first  */
    base->CONTROLS[chnlNumber].CnSC &=
        ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

    /* Wait till mode change to disable channel is acknowledged */
    while ((base->CONTROLS[chnlNumber].CnSC &
            (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }

    /* Setup the channel output behaviour when a match occurs with the compare value */
    base->CONTROLS[chnlNumber].CnSC |= compareMode;

    /* Setup the compare value */
    base->CONTROLS[chnlNumber].CnV = compareValue;

    /* Wait till mode change is acknowledged */
    while (!(base->CONTROLS[chnlNumber].CnSC &
             (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }
}

#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
void TPM_SetupDualEdgeCapture(TPM_Type *base,
                              tpm_chnl_t chnlPairNumber,
                              const tpm_dual_edge_capture_param_t *edgeParam,
                              uint32_t filterValue)
{
    assert(edgeParam);

    uint32_t reg;

    /* Unlock: When switching mode, disable channel first */
    base->CONTROLS[chnlPairNumber * 2].CnSC &=
        ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

    /* Wait till mode change to disable channel is acknowledged */
    while ((base->CONTROLS[chnlPairNumber * 2].CnSC &
            (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }

    base->CONTROLS[chnlPairNumber * 2 + 1].CnSC &=
        ~(TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK);

    /* Wait till mode change to disable channel is acknowledged */
    while ((base->CONTROLS[chnlPairNumber * 2 + 1].CnSC &
            (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }

    /* Now, the registers for input mode can be operated. */
    if (edgeParam->enableSwap)
    {
        /* Set the combine and swap bits for the channel pair */
        base->COMBINE |= (TPM_COMBINE_COMBINE0_MASK | TPM_COMBINE_COMSWAP0_MASK)
                         << (TPM_COMBINE_SHIFT * chnlPairNumber);

        /* Input filter setup for channel n+1 input */
        reg = base->FILTER;
        reg &= ~(TPM_FILTER_CH0FVAL_MASK << (TPM_FILTER_CH1FVAL_SHIFT * (chnlPairNumber + 1)));
        reg |= (filterValue << (TPM_FILTER_CH1FVAL_SHIFT * (chnlPairNumber + 1)));
        base->FILTER = reg;
    }
    else
    {
        reg = base->COMBINE;
        /* Clear the swap bit for the channel pair */
        reg &= ~(TPM_COMBINE_COMSWAP0_MASK << (TPM_COMBINE_COMSWAP0_SHIFT * chnlPairNumber));

        /* Set the combine bit for the channel pair */
        reg |= TPM_COMBINE_COMBINE0_MASK << (TPM_COMBINE_SHIFT * chnlPairNumber);
        base->COMBINE = reg;

        /* Input filter setup for channel n input */
        reg = base->FILTER;
        reg &= ~(TPM_FILTER_CH0FVAL_MASK << (TPM_FILTER_CH1FVAL_SHIFT * chnlPairNumber));
        reg |= (filterValue << (TPM_FILTER_CH1FVAL_SHIFT * chnlPairNumber));
        base->FILTER = reg;
    }

    /* Setup the edge detection from channel n */
    base->CONTROLS[chnlPairNumber * 2].CnSC |= edgeParam->currChanEdgeMode;

    /* Wait till mode change is acknowledged */
    while (!(base->CONTROLS[chnlPairNumber * 2].CnSC &
             (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }

    /* Setup the edge detection from channel n+1 */
    base->CONTROLS[(chnlPairNumber * 2) + 1].CnSC |= edgeParam->nextChanEdgeMode;

    /* Wait till mode change is acknowledged */
    while (!(base->CONTROLS[(chnlPairNumber * 2) + 1].CnSC &
             (TPM_CnSC_MSA_MASK | TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK | TPM_CnSC_ELSB_MASK)))
    {
    }
}
#endif

#if defined(FSL_FEATURE_TPM_HAS_QDCTRL) && FSL_FEATURE_TPM_HAS_QDCTRL
void TPM_SetupQuadDecode(TPM_Type *base,
                         const tpm_phase_params_t *phaseAParams,
                         const tpm_phase_params_t *phaseBParams,
                         tpm_quad_decode_mode_t quadMode)
{
    assert(phaseAParams);
    assert(phaseBParams);

    uint32_t reg;

    /* Set Phase A filter value */
    reg = base->FILTER;
    reg &= ~(TPM_FILTER_CH0FVAL_MASK);
    reg |= TPM_FILTER_CH0FVAL(phaseAParams->phaseFilterVal);
    base->FILTER = reg;

    /* Set Phase A polarity */
    if (phaseAParams->phasePolarity)
    {
        base->POL |= TPM_POL_POL0_MASK;
    }
    else
    {
        base->POL &= ~TPM_POL_POL0_MASK;
    }

    /* Set Phase B filter value */
    reg = base->FILTER;
    reg &= ~(TPM_FILTER_CH1FVAL_MASK);
    reg |= TPM_FILTER_CH1FVAL(phaseBParams->phaseFilterVal);
    base->FILTER = reg;

    /* Set Phase B polarity */
    if (phaseBParams->phasePolarity)
    {
        base->POL |= TPM_POL_POL1_MASK;
    }
    else
    {
        base->POL &= ~TPM_POL_POL1_MASK;
    }

    /* Set Quadrature mode */
    reg = base->QDCTRL;
    reg &= ~(TPM_QDCTRL_QUADMODE_MASK);
    reg |= TPM_QDCTRL_QUADMODE(quadMode);
    base->QDCTRL = reg;

    /* Enable Quad decode */
    base->QDCTRL |= TPM_QDCTRL_QUADEN_MASK;
}

#endif

void TPM_EnableInterrupts(TPM_Type *base, uint32_t mask)
{
    uint32_t chnlInterrupts = (mask & 0xFF);
    uint8_t chnlNumber = 0;

    /* Enable the timer overflow interrupt */
    if (mask & kTPM_TimeOverflowInterruptEnable)
    {
        base->SC |= TPM_SC_TOIE_MASK;
    }

    /* Enable the channel interrupts */
    while (chnlInterrupts)
    {
        if (chnlInterrupts & 0x1)
        {
            base->CONTROLS[chnlNumber].CnSC |= TPM_CnSC_CHIE_MASK;
        }
        chnlNumber++;
        chnlInterrupts = chnlInterrupts >> 1U;
    }
}

void TPM_DisableInterrupts(TPM_Type *base, uint32_t mask)
{
    uint32_t chnlInterrupts = (mask & 0xFF);
    uint8_t chnlNumber = 0;

    /* Disable the timer overflow interrupt */
    if (mask & kTPM_TimeOverflowInterruptEnable)
    {
        base->SC &= ~TPM_SC_TOIE_MASK;
    }

    /* Disable the channel interrupts */
    while (chnlInterrupts)
    {
        if (chnlInterrupts & 0x1)
        {
            base->CONTROLS[chnlNumber].CnSC &= ~TPM_CnSC_CHIE_MASK;
        }
        chnlNumber++;
        chnlInterrupts = chnlInterrupts >> 1U;
    }
}

uint32_t TPM_GetEnabledInterrupts(TPM_Type *base)
{
    uint32_t enabledInterrupts = 0;
    int8_t chnlCount = FSL_FEATURE_TPM_CHANNEL_COUNTn(base);

    /* The CHANNEL_COUNT macro returns -1 if it cannot match the TPM instance */
    assert(chnlCount != -1);

    /* Check if timer overflow interrupt is enabled */
    if (base->SC & TPM_SC_TOIE_MASK)
    {
        enabledInterrupts |= kTPM_TimeOverflowInterruptEnable;
    }

    /* Check if the channel interrupts are enabled */
    while (chnlCount > 0)
    {
        chnlCount--;
        if (base->CONTROLS[chnlCount].CnSC & TPM_CnSC_CHIE_MASK)
        {
            enabledInterrupts |= (1U << chnlCount);
        }
    }

    return enabledInterrupts;
}
