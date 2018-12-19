/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_PDB_H_
#define _FSL_PDB_H_

#include "fsl_common.h"

/*!
 * @addtogroup pdb
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief PDB driver version 2.0.1. */
#define FSL_PDB_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*!
 * @brief PDB flags.
 */
enum _pdb_status_flags
{
    kPDB_LoadOKFlag = PDB_SC_LDOK_MASK,      /*!< This flag is automatically cleared when the values in buffers are
                                                  loaded into the internal registers after the LDOK bit is set or the
                                                  PDBEN is cleared. */
    kPDB_DelayEventFlag = PDB_SC_PDBIF_MASK, /*!< PDB timer delay event flag. */
};

/*!
 * @brief PDB ADC PreTrigger channel flags.
 */
enum _pdb_adc_pretrigger_flags
{
    /* PDB PreTrigger channel match flags. */
    kPDB_ADCPreTriggerChannel0Flag = PDB_S_CF(1U << 0), /*!< Pre-trigger 0 flag. */
    kPDB_ADCPreTriggerChannel1Flag = PDB_S_CF(1U << 1), /*!< Pre-trigger 1 flag. */
#if (PDB_DLY_COUNT2 > 2)
    kPDB_ADCPreTriggerChannel2Flag = PDB_S_CF(1U << 2), /*!< Pre-trigger 2 flag. */
    kPDB_ADCPreTriggerChannel3Flag = PDB_S_CF(1U << 3), /*!< Pre-trigger 3 flag. */
#endif                                                  /* PDB_DLY_COUNT2 > 2 */
#if (PDB_DLY_COUNT2 > 4)
    kPDB_ADCPreTriggerChannel4Flag = PDB_S_CF(1U << 4), /*!< Pre-trigger 4 flag. */
    kPDB_ADCPreTriggerChannel5Flag = PDB_S_CF(1U << 5), /*!< Pre-trigger 5 flag. */
    kPDB_ADCPreTriggerChannel6Flag = PDB_S_CF(1U << 6), /*!< Pre-trigger 6 flag. */
    kPDB_ADCPreTriggerChannel7Flag = PDB_S_CF(1U << 7), /*!< Pre-trigger 7 flag. */
#endif                                                  /* PDB_DLY_COUNT2 > 4 */

    /* PDB PreTrigger channel error flags. */
    kPDB_ADCPreTriggerChannel0ErrorFlag = PDB_S_ERR(1U << 0), /*!< Pre-trigger 0 Error. */
    kPDB_ADCPreTriggerChannel1ErrorFlag = PDB_S_ERR(1U << 1), /*!< Pre-trigger 1 Error. */
#if (PDB_DLY_COUNT2 > 2)
    kPDB_ADCPreTriggerChannel2ErrorFlag = PDB_S_ERR(1U << 2), /*!< Pre-trigger 2 Error. */
    kPDB_ADCPreTriggerChannel3ErrorFlag = PDB_S_ERR(1U << 3), /*!< Pre-trigger 3 Error. */
#endif                                                        /* PDB_DLY_COUNT2 > 2 */
#if (PDB_DLY_COUNT2 > 4)
    kPDB_ADCPreTriggerChannel4ErrorFlag = PDB_S_ERR(1U << 4), /*!< Pre-trigger 4 Error. */
    kPDB_ADCPreTriggerChannel5ErrorFlag = PDB_S_ERR(1U << 5), /*!< Pre-trigger 5 Error. */
    kPDB_ADCPreTriggerChannel6ErrorFlag = PDB_S_ERR(1U << 6), /*!< Pre-trigger 6 Error. */
    kPDB_ADCPreTriggerChannel7ErrorFlag = PDB_S_ERR(1U << 7), /*!< Pre-trigger 7 Error. */
#endif                                                        /* PDB_DLY_COUNT2 > 4 */
};

/*!
 * @brief PDB buffer interrupts.
 */
enum _pdb_interrupt_enable
{
    kPDB_SequenceErrorInterruptEnable = PDB_SC_PDBEIE_MASK, /*!< PDB sequence error interrupt enable. */
    kPDB_DelayInterruptEnable = PDB_SC_PDBIE_MASK,          /*!< PDB delay interrupt enable. */
};

/*!
 * @brief PDB load value mode.
 *
 * Selects the mode to load the internal values after doing the load operation (write 1 to PDBx_SC[LDOK]).
 * These values are for the following operations.
 *  - PDB counter (PDBx_MOD, PDBx_IDLY)
 *  - ADC trigger (PDBx_CHnDLYm)
 *  - DAC trigger (PDBx_DACINTx)
 *  - CMP trigger (PDBx_POyDLY)
 */
typedef enum _pdb_load_value_mode
{
    kPDB_LoadValueImmediately = 0U,                     /*!< Load immediately after 1 is written to LDOK. */
    kPDB_LoadValueOnCounterOverflow = 1U,               /*!< Load when the PDB counter overflows (reaches the MOD
                                                             register value). */
    kPDB_LoadValueOnTriggerInput = 2U,                  /*!< Load a trigger input event is detected. */
    kPDB_LoadValueOnCounterOverflowOrTriggerInput = 3U, /*!< Load either when the PDB counter overflows or a trigger
                                                             input is detected. */
} pdb_load_value_mode_t;

/*!
 * @brief Prescaler divider.
 *
 * Counting uses the peripheral clock divided by multiplication factor selected by times of MULT.
 */
typedef enum _pdb_prescaler_divider
{
    kPDB_PrescalerDivider1 = 0U,   /*!< Divider x1. */
    kPDB_PrescalerDivider2 = 1U,   /*!< Divider x2. */
    kPDB_PrescalerDivider4 = 2U,   /*!< Divider x4. */
    kPDB_PrescalerDivider8 = 3U,   /*!< Divider x8. */
    kPDB_PrescalerDivider16 = 4U,  /*!< Divider x16. */
    kPDB_PrescalerDivider32 = 5U,  /*!< Divider x32. */
    kPDB_PrescalerDivider64 = 6U,  /*!< Divider x64. */
    kPDB_PrescalerDivider128 = 7U, /*!< Divider x128. */
} pdb_prescaler_divider_t;

/*!
 * @brief Multiplication factor select for prescaler.
 *
 * Selects the multiplication factor of the prescaler divider for the counter clock.
 */
typedef enum _pdb_divider_multiplication_factor
{
    kPDB_DividerMultiplicationFactor1 = 0U,  /*!< Multiplication factor is 1. */
    kPDB_DividerMultiplicationFactor10 = 1U, /*!< Multiplication factor is 10. */
    kPDB_DividerMultiplicationFactor20 = 2U, /*!< Multiplication factor is 20. */
    kPDB_DividerMultiplicationFactor40 = 3U, /*!< Multiplication factor is 40. */
} pdb_divider_multiplication_factor_t;

/*!
 * @brief Trigger input source
 *
 * Selects the trigger input source for the PDB. The trigger input source can be internal or external (EXTRG pin), or
 * the software trigger. See chip configuration details for the actual PDB input trigger connections.
 */
typedef enum _pdb_trigger_input_source
{
    kPDB_TriggerInput0 = 0U,    /*!< Trigger-In 0. */
    kPDB_TriggerInput1 = 1U,    /*!< Trigger-In 1. */
    kPDB_TriggerInput2 = 2U,    /*!< Trigger-In 2. */
    kPDB_TriggerInput3 = 3U,    /*!< Trigger-In 3. */
    kPDB_TriggerInput4 = 4U,    /*!< Trigger-In 4. */
    kPDB_TriggerInput5 = 5U,    /*!< Trigger-In 5. */
    kPDB_TriggerInput6 = 6U,    /*!< Trigger-In 6. */
    kPDB_TriggerInput7 = 7U,    /*!< Trigger-In 7. */
    kPDB_TriggerInput8 = 8U,    /*!< Trigger-In 8. */
    kPDB_TriggerInput9 = 9U,    /*!< Trigger-In 9. */
    kPDB_TriggerInput10 = 10U,  /*!< Trigger-In 10. */
    kPDB_TriggerInput11 = 11U,  /*!< Trigger-In 11. */
    kPDB_TriggerInput12 = 12U,  /*!< Trigger-In 12. */
    kPDB_TriggerInput13 = 13U,  /*!< Trigger-In 13. */
    kPDB_TriggerInput14 = 14U,  /*!< Trigger-In 14. */
    kPDB_TriggerSoftware = 15U, /*!< Trigger-In 15, software trigger. */
} pdb_trigger_input_source_t;

/*!
 * @brief List of PDB ADC trigger channels
 * @note Actual number of available channels is SoC dependent
 */
typedef enum _pdb_adc_trigger_channel
{
    kPDB_ADCTriggerChannel0 = 0U, /*!< PDB ADC trigger channel number 0 */
    kPDB_ADCTriggerChannel1 = 1U, /*!< PDB ADC trigger channel number 1 */
    kPDB_ADCTriggerChannel2 = 2U, /*!< PDB ADC trigger channel number 2 */
    kPDB_ADCTriggerChannel3 = 3U, /*!< PDB ADC trigger channel number 3 */
} pdb_adc_trigger_channel_t;

/*!
 * @brief List of PDB ADC pretrigger
 * @note Actual number of available pretrigger channels is SoC dependent
 */
typedef enum _pdb_adc_pretrigger
{
    kPDB_ADCPreTrigger0 = 0U, /*!< PDB ADC pretrigger number 0 */
    kPDB_ADCPreTrigger1 = 1U, /*!< PDB ADC pretrigger number 1 */
    kPDB_ADCPreTrigger2 = 2U, /*!< PDB ADC pretrigger number 2 */
    kPDB_ADCPreTrigger3 = 3U, /*!< PDB ADC pretrigger number 3 */
    kPDB_ADCPreTrigger4 = 4U, /*!< PDB ADC pretrigger number 4 */
    kPDB_ADCPreTrigger5 = 5U, /*!< PDB ADC pretrigger number 5 */
    kPDB_ADCPreTrigger6 = 6U, /*!< PDB ADC pretrigger number 6 */
    kPDB_ADCPreTrigger7 = 7U, /*!< PDB ADC pretrigger number 7 */
} pdb_adc_pretrigger_t;

/*!
 * @brief List of PDB DAC trigger channels
 * @note Actual number of available channels is SoC dependent
 */
typedef enum _pdb_dac_trigger_channel
{
    kPDB_DACTriggerChannel0 = 0U, /*!< PDB DAC trigger channel number 0 */
    kPDB_DACTriggerChannel1 = 1U, /*!< PDB DAC trigger channel number 1 */
} pdb_dac_trigger_channel_t;

/*!
 * @brief List of PDB pulse out trigger channels
 * @note Actual number of available channels is SoC dependent
 */
typedef enum _pdb_pulse_out_trigger_channel
{
    kPDB_PulseOutTriggerChannel0 = 0U, /*!< PDB pulse out trigger channel number 0 */
    kPDB_PulseOutTriggerChannel1 = 1U, /*!< PDB pulse out trigger channel number 1 */
    kPDB_PulseOutTriggerChannel2 = 2U, /*!< PDB pulse out trigger channel number 2 */
    kPDB_PulseOutTriggerChannel3 = 3U, /*!< PDB pulse out trigger channel number 3 */
} pdb_pulse_out_trigger_channel_t;

/*!
 * @brief List of PDB pulse out trigger channels mask
 * @note Actual number of available channels mask is SoC dependent
 */
typedef enum _pdb_pulse_out_channel_mask
{
    kPDB_PulseOutChannel0Mask = (1U << 0U), /*!< PDB pulse out trigger channel number 0 mask */
    kPDB_PulseOutChannel1Mask = (1U << 1U), /*!< PDB pulse out trigger channel number 1 mask */
    kPDB_PulseOutChannel2Mask = (1U << 2U), /*!< PDB pulse out trigger channel number 2 mask */
    kPDB_PulseOutChannel3Mask = (1U << 3U), /*!< PDB pulse out trigger channel number 3 mask */
} pdb_pulse_out_channel_mask_t;

/*!
 * @brief PDB module configuration.
 */
typedef struct _pdb_config
{
    pdb_load_value_mode_t loadValueMode;                             /*!< Select the load value mode. */
    pdb_prescaler_divider_t prescalerDivider;                        /*!< Select the prescaler divider. */
    pdb_divider_multiplication_factor_t dividerMultiplicationFactor; /*!< Multiplication factor select for prescaler. */
    pdb_trigger_input_source_t triggerInputSource;                   /*!< Select the trigger input source. */
    bool enableContinuousMode;                                       /*!< Enable the PDB operation in Continuous mode.*/
} pdb_config_t;

/*!
 * @brief PDB ADC Pre-trigger configuration.
 */
typedef struct _pdb_adc_pretrigger_config
{
    uint32_t enablePreTriggerMask;          /*!< PDB Channel Pre-trigger Enable. */
    uint32_t enableOutputMask;              /*!< PDB Channel Pre-trigger Output Select.
                                                 PDB channel's corresponding pre-trigger asserts when the counter
                                                 reaches the channel delay register. */
    uint32_t enableBackToBackOperationMask; /*!< PDB Channel pre-trigger Back-to-Back Operation Enable.
                                                 Back-to-back operation enables the ADC conversions complete to trigger
                                                 the next PDB channel pre-trigger and trigger output, so that the ADC
                                                 conversions can be triggered on next set of configuration and results
                                                 registers.*/
} pdb_adc_pretrigger_config_t;

/*!
 * @brief PDB DAC trigger configuration.
 */
typedef struct _pdb_dac_trigger_config
{
    bool enableExternalTriggerInput; /*!< Enables the external trigger for DAC interval counter. */
    bool enableIntervalTrigger;      /*!< Enables the DAC interval trigger. */
} pdb_dac_trigger_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization
 * @{
 */

/*!
 * @brief Initializes the PDB module.
 *
 * This function initializes the PDB module. The operations included are as follows.
 *  - Enable the clock for PDB instance.
 *  - Configure the PDB module.
 *  - Enable the PDB module.
 *
 * @param base PDB peripheral base address.
 * @param config Pointer to the configuration structure. See "pdb_config_t".
 */
void PDB_Init(PDB_Type *base, const pdb_config_t *config);

/*!
 * @brief De-initializes the PDB module.
 *
 * @param base PDB peripheral base address.
 */
void PDB_Deinit(PDB_Type *base);

/*!
 * @brief Initializes the PDB user configuration structure.
 *
 * This function initializes the user configuration structure to a default value. The default values are as follows.
 * @code
 *   config->loadValueMode = kPDB_LoadValueImmediately;
 *   config->prescalerDivider = kPDB_PrescalerDivider1;
 *   config->dividerMultiplicationFactor = kPDB_DividerMultiplicationFactor1;
 *   config->triggerInputSource = kPDB_TriggerSoftware;
 *   config->enableContinuousMode = false;
 * @endcode
 * @param config Pointer to configuration structure. See "pdb_config_t".
 */
void PDB_GetDefaultConfig(pdb_config_t *config);

/*!
 * @brief Enables the PDB module.
 *
 * @param base PDB peripheral base address.
 * @param enable Enable the module or not.
 */
static inline void PDB_Enable(PDB_Type *base, bool enable)
{
    if (enable)
    {
        base->SC |= PDB_SC_PDBEN_MASK;
    }
    else
    {
        base->SC &= ~PDB_SC_PDBEN_MASK;
    }
}

/* @} */

/*!
 * @name Basic Counter
 * @{
 */

/*!
 * @brief Triggers the PDB counter by software.
 *
 * @param base PDB peripheral base address.
 */
static inline void PDB_DoSoftwareTrigger(PDB_Type *base)
{
    base->SC |= PDB_SC_SWTRIG_MASK;
}

/*!
 * @brief Loads the counter values.
 *
 * This function loads the counter values from the internal buffer.
 * See "pdb_load_value_mode_t" about PDB's load mode.
 *
 * @param base PDB peripheral base address.
 */
static inline void PDB_DoLoadValues(PDB_Type *base)
{
    base->SC |= PDB_SC_LDOK_MASK;
}

/*!
 * @brief Enables the DMA for the PDB module.
 *
 * @param base PDB peripheral base address.
 * @param enable Enable the feature or not.
 */
static inline void PDB_EnableDMA(PDB_Type *base, bool enable)
{
    if (enable)
    {
        base->SC |= PDB_SC_DMAEN_MASK;
    }
    else
    {
        base->SC &= ~PDB_SC_DMAEN_MASK;
    }
}

/*!
 * @brief Enables the interrupts for the PDB module.
 *
 * @param base PDB peripheral base address.
 * @param mask Mask value for interrupts. See "_pdb_interrupt_enable".
 */
static inline void PDB_EnableInterrupts(PDB_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~(PDB_SC_PDBEIE_MASK | PDB_SC_PDBIE_MASK)));

    base->SC |= mask;
}

/*!
 * @brief Disables the interrupts for the PDB module.
 *
 * @param base PDB peripheral base address.
 * @param mask Mask value for interrupts. See "_pdb_interrupt_enable".
 */
static inline void PDB_DisableInterrupts(PDB_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~(PDB_SC_PDBEIE_MASK | PDB_SC_PDBIE_MASK)));

    base->SC &= ~mask;
}

/*!
 * @brief  Gets the status flags of the PDB module.
 *
 * @param  base PDB peripheral base address.
 *
 * @return      Mask value for asserted flags. See "_pdb_status_flags".
 */
static inline uint32_t PDB_GetStatusFlags(PDB_Type *base)
{
    return base->SC & (PDB_SC_PDBIF_MASK | PDB_SC_LDOK_MASK);
}

/*!
 * @brief Clears the status flags of the PDB module.
 *
 * @param base PDB peripheral base address.
 * @param mask Mask value of flags. See "_pdb_status_flags".
 */
static inline void PDB_ClearStatusFlags(PDB_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~PDB_SC_PDBIF_MASK));

    base->SC &= ~mask;
}

/*!
 * @brief  Specifies the counter period.
 *
 * @param  base  PDB peripheral base address.
 * @param  value Setting value for the modulus. 16-bit is available.
 */
static inline void PDB_SetModulusValue(PDB_Type *base, uint32_t value)
{
    base->MOD = PDB_MOD_MOD(value);
}

/*!
 * @brief  Gets the PDB counter's current value.
 *
 * @param  base PDB peripheral base address.
 *
 * @return      PDB counter's current value.
 */
static inline uint32_t PDB_GetCounterValue(PDB_Type *base)
{
    return base->CNT;
}

/*!
 * @brief Sets the value for the PDB counter delay event.
 *
 * @param base  PDB peripheral base address.
 * @param value Setting value for PDB counter delay event. 16-bit is available.
 */
static inline void PDB_SetCounterDelayValue(PDB_Type *base, uint32_t value)
{
    base->IDLY = PDB_IDLY_IDLY(value);
}
/* @} */

/*!
 * @name ADC Pre-trigger
 * @{
 */

/*!
 * @brief Configures the ADC pre-trigger in the PDB module.
 *
 * @param base    PDB peripheral base address.
 * @param channel Channel index for ADC instance.
 * @param config  Pointer to the configuration structure. See "pdb_adc_pretrigger_config_t".
 */
static inline void PDB_SetADCPreTriggerConfig(PDB_Type *base, pdb_adc_trigger_channel_t channel, pdb_adc_pretrigger_config_t *config)
{
    assert(channel < PDB_C1_COUNT);
    assert(NULL != config);

    base->CH[channel].C1 = PDB_C1_BB(config->enableBackToBackOperationMask) | PDB_C1_TOS(config->enableOutputMask) |
                           PDB_C1_EN(config->enablePreTriggerMask);
}

/*!
 * @brief Sets the value for the ADC pre-trigger delay event.
 *
 * This function sets the value for ADC pre-trigger delay event. It specifies the delay value for the channel's
 * corresponding pre-trigger. The pre-trigger asserts when the PDB counter is equal to the set value.
 *
 * @param base             PDB peripheral base address.
 * @param channel          Channel index for ADC instance.
 * @param pretriggerNumber Channel group index for ADC instance.
 * @param value            Setting value for ADC pre-trigger delay event. 16-bit is available.
 */
static inline void PDB_SetADCPreTriggerDelayValue(PDB_Type *base, pdb_adc_trigger_channel_t channel, pdb_adc_pretrigger_t pretriggerNumber, uint32_t value)
{
    assert(channel < PDB_C1_COUNT);
    assert(pretriggerNumber < PDB_DLY_COUNT2);
    /* xx_COUNT2 is actually the count for pre-triggers in header file. xx_COUNT is used for the count of channels. */

    base->CH[channel].DLY[pretriggerNumber] = PDB_DLY_DLY(value);
}

/*!
 * @brief  Gets the ADC pre-trigger's status flags.
 *
 * @param  base    PDB peripheral base address.
 * @param  channel Channel index for ADC instance.
 *
 * @return         Mask value for asserted flags. See "_pdb_adc_pretrigger_flags".
 */
static inline uint32_t PDB_GetADCPreTriggerStatusFlags(PDB_Type *base, pdb_adc_trigger_channel_t channel)
{
    assert(channel < PDB_C1_COUNT);

    return base->CH[channel].S;
}

/*!
 * @brief Clears the ADC pre-trigger status flags.
 *
 * @param base    PDB peripheral base address.
 * @param channel Channel index for ADC instance.
 * @param mask    Mask value for flags. See "_pdb_adc_pretrigger_flags".
 */
static inline void PDB_ClearADCPreTriggerStatusFlags(PDB_Type *base, pdb_adc_trigger_channel_t channel, uint32_t mask)
{
    assert(channel < PDB_C1_COUNT);

    base->CH[channel].S &= ~mask;
}

/* @} */

#if defined(FSL_FEATURE_PDB_HAS_DAC) && FSL_FEATURE_PDB_HAS_DAC
/*!
 * @name DAC Interval Trigger
 * @{
 */

/*!
 * @brief Configures the DAC trigger in the PDB module.
 *
 * @param base    PDB peripheral base address.
 * @param channel Channel index for DAC instance.
 * @param config  Pointer to the configuration structure. See "pdb_dac_trigger_config_t".
 */
void PDB_SetDACTriggerConfig(PDB_Type *base, pdb_dac_trigger_channel_t channel, pdb_dac_trigger_config_t *config);

/*!
 * @brief Sets the value for the DAC interval event.
 *
 * This fucntion sets the value for DAC interval event. DAC interval trigger triggers the DAC module to update
 * the buffer when the DAC interval counter is equal to the set value.
 *
 * @param base    PDB peripheral base address.
 * @param channel Channel index for DAC instance.
 * @param value   Setting value for the DAC interval event.
 */
static inline void PDB_SetDACTriggerIntervalValue(PDB_Type *base, pdb_dac_trigger_channel_t channel, uint32_t value)
{
    assert(channel < PDB_INT_COUNT);

    base->DAC[channel].INT = PDB_INT_INT(value);
}

/* @} */
#endif /* FSL_FEATURE_PDB_HAS_DAC */

/*!
 * @name Pulse-Out Trigger
 * @{
 */

/*!
 * @brief Enables the pulse out trigger channels.
 *
 * @param base        PDB peripheral base address.
 * @param channelMask Channel mask value for multiple pulse out trigger channel.
 * @param enable Whether the feature is enabled or not.
 */
static inline void PDB_EnablePulseOutTrigger(PDB_Type *base, pdb_pulse_out_channel_mask_t channelMask, bool enable)
{
    assert(channelMask < (1 << PDB_PODLY_COUNT));

    if (enable)
    {
        base->POEN |= PDB_POEN_POEN(channelMask);
    }
    else
    {
        base->POEN &= ~(PDB_POEN_POEN(channelMask));
    }
}

/*!
 * @brief Sets event values for the pulse out trigger.
 *
 * This function is used to set event values for the pulse output trigger.
 * These pulse output trigger delay values specify the delay for the PDB Pulse-out. Pulse-out goes high when the PDB
 * counter is equal to the pulse output high value (value1). Pulse-out goes low when the PDB counter is equal to the
 * pulse output low value (value2).
 *
 * @param base    PDB peripheral base address.
 * @param channel Channel index for pulse out trigger channel.
 * @param value1  Setting value for pulse out high.
 * @param value2  Setting value for pulse out low.
 */
static inline void PDB_SetPulseOutTriggerDelayValue(PDB_Type *base, pdb_pulse_out_trigger_channel_t channel, uint32_t value1, uint32_t value2)
{
    assert(channel < PDB_PODLY_COUNT);

    base->PODLY[channel] = PDB_PODLY_DLY1(value1) | PDB_PODLY_DLY2(value2);
}

/* @} */
#if defined(__cplusplus)
}
#endif
/*!
 * @}
 */
#endif /* _FSL_PDB_H_ */
