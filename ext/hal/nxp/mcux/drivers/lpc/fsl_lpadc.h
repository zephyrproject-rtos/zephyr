/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_LPADC_H_
#define _FSL_LPADC_H_

#include "fsl_common.h"

/*!
 * @addtogroup lpadc
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief LPADC driver version 2.0.3. */
#define FSL_LPADC_DRIVER_VERSION (MAKE_VERSION(2, 0, 3))
/*@}*/

/*!
 * @brief Define the MACRO function to get command status from status value.
 *
 * The statusVal is the return value from LPADC_GetStatusFlags().
 */
#define LPADC_GET_ACTIVE_COMMAND_STATUS(statusVal) ((statusVal & ADC_STAT_CMDACT_MASK) >> ADC_STAT_CMDACT_SHIFT)

/*!
 * @brief Define the MACRO function to get trigger status from status value.
 *
 * The statusVal is the return value from LPADC_GetStatusFlags().
 */
#define LPADC_GET_ACTIVE_TRIGGER_STATUE(statusVal) ((statusVal & ADC_STAT_TRGACT_MASK) >> ADC_STAT_TRGACT_SHIFT)

#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) && (FSL_FEATURE_LPADC_FIFO_COUNT == 2))
/*!
 * @brief Define hardware flags of the module.
 */
enum _lpadc_status_flags
{
    kLPADC_ResultFIFO0OverflowFlag = ADC_STAT_FOF0_MASK, /*!< Indicates that more data has been written to the Result
                                                               FIFO 0 than it can hold. */
    kLPADC_ResultFIFO0ReadyFlag = ADC_STAT_RDY0_MASK,    /*!< Indicates when the number of valid datawords in the result
                                                               FIFO 0 is greater than the setting watermark level. */
    kLPADC_ResultFIFO1OverflowFlag = ADC_STAT_FOF1_MASK, /*!< Indicates that more data has been written to the Result
                                                              FIFO 1 than it can hold. */
    kLPADC_ResultFIFO1ReadyFlag = ADC_STAT_RDY1_MASK,    /*!< Indicates when the number of valid datawords in the result
                                                              FIFO 1 is greater than the setting watermark level. */
};

/*!
 * @brief Define interrupt switchers of the module.
 */
enum _lpadc_interrupt_enable
{
    kLPADC_ResultFIFO0OverflowInterruptEnable = ADC_IE_FOFIE0_MASK, /*!< Configures ADC to generate overflow interrupt
                                                                         requests when FOF0 flag is asserted. */
    kLPADC_FIFO0WatermarkInterruptEnable = ADC_IE_FWMIE0_MASK,      /*!< Configures ADC to generate watermark interrupt
                                                                         requests when RDY0 flag is asserted. */
    kLPADC_ResultFIFO1OverflowInterruptEnable = ADC_IE_FOFIE1_MASK, /*!< Configures ADC to generate overflow interrupt
                                                                         requests when FOF1 flag is asserted. */
    kLPADC_FIFO1WatermarkInterruptEnable = ADC_IE_FWMIE1_MASK,      /*!< Configures ADC to generate watermark interrupt
                                                                         requests when RDY1 flag is asserted. */
};
#else
/*!
 * @brief Define hardware flags of the module.
 */
enum _lpadc_status_flags
{
    kLPADC_ResultFIFOOverflowFlag = ADC_STAT_FOF_MASK, /*!< Indicates that more data has been written to the Result FIFO
                                                            than it can hold. */
    kLPADC_ResultFIFOReadyFlag = ADC_STAT_RDY_MASK, /*!< Indicates when the number of valid datawords in the result FIFO
                                                         is greater than the setting watermark level. */
};

/*!
 * @brief Define interrupt switchers of the module.
 */
enum _lpadc_interrupt_enable
{
    kLPADC_ResultFIFOOverflowInterruptEnable = ADC_IE_FOFIE_MASK, /*!< Configures ADC to generate overflow interrupt
                                                                       requests when FOF flag is asserted. */
    kLPADC_FIFOWatermarkInterruptEnable = ADC_IE_FWMIE_MASK,      /*!< Configures ADC to generate watermark interrupt
                                                                       requests when RDY flag is asserted. */
};
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */

/*!
 * @brief Define enumeration of sample scale mode.
 *
 * The sample scale mode is used to reduce the selected ADC analog channel input voltage level by a factor. The maximum
 * possible voltage on the ADC channel input should be considered when selecting a scale mode to ensure that the
 * reducing factor always results voltage level at or below the VREFH reference. This reducing capability allows
 * conversion of analog inputs higher than VREFH. A-side and B-side channel inputs are both scaled using the scale mode.
 */
typedef enum _lpadc_sample_scale_mode
{
    kLPADC_SamplePartScale = 0U, /*!< Use divided input voltage signal. (Factor of 30/64). */
    kLPADC_SampleFullScale = 1U, /*!< Full scale (Factor of 1). */
} lpadc_sample_scale_mode_t;

/*!
 * @brief Define enumeration of channel sample mode.
 *
 * The channel sample mode configures the channel with single-end/differential/dual-single-end, side A/B.
 */
typedef enum _lpadc_sample_channel_mode
{
    kLPADC_SampleChannelSingleEndSideA = 0U, /*!< Single end mode, using side A. */
    kLPADC_SampleChannelSingleEndSideB = 1U, /*!< Single end mode, using side B. */
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_DIFF) && FSL_FEATURE_LPADC_HAS_CMDL_DIFF
    kLPADC_SampleChannelDiffBothSideAB = 2U, /*!< Differential mode, using A as plus side and B as minue side. */
    kLPADC_SampleChannelDiffBothSideBA = 3U, /*!< Differential mode, using B as plus side and A as minue side. */
#elif defined(FSL_FEATURE_LPADC_HAS_CMDL_CTYPE) && FSL_FEATURE_LPADC_HAS_CMDL_CTYPE
    kLPADC_SampleChannelDiffBothSide = 2U, /*!< Differential mode, using A and B. */
    kLPADC_SampleChannelDualSingleEndBothSide =
        3U, /*!< Dual-Single-Ended Mode. Both A side and B side channels are converted independently. */
#endif
} lpadc_sample_channel_mode_t;

/*!
 * @brief Define enumeration of hardware average selection.
 *
 * It Selects how many ADC conversions are averaged to create the ADC result. An internal storage buffer is used to
 * capture temporary results while the averaging iterations are executed.
 */
typedef enum _lpadc_hardware_average_mode
{
    kLPADC_HardwareAverageCount1 = 0U,   /*!< Single conversion. */
    kLPADC_HardwareAverageCount2 = 1U,   /*!< 2 conversions averaged. */
    kLPADC_HardwareAverageCount4 = 2U,   /*!< 4 conversions averaged. */
    kLPADC_HardwareAverageCount8 = 3U,   /*!< 8 conversions averaged. */
    kLPADC_HardwareAverageCount16 = 4U,  /*!< 16 conversions averaged. */
    kLPADC_HardwareAverageCount32 = 5U,  /*!< 32 conversions averaged. */
    kLPADC_HardwareAverageCount64 = 6U,  /*!< 64 conversions averaged. */
    kLPADC_HardwareAverageCount128 = 7U, /*!< 128 conversions averaged. */
} lpadc_hardware_average_mode_t;

/*!
 * @brief Define enumeration of sample time selection.
 *
 * The shortest sample time maximizes conversion speed for lower impedance inputs. Extending sample time allows higher
 * impedance inputs to be accurately sampled. Longer sample times can also be used to lower overall power consumption
 * when command looping and sequencing is configured and high conversion rates are not required.
 */
typedef enum _lpadc_sample_time_mode
{
    kLPADC_SampleTimeADCK3 = 0U,   /*!< 3 ADCK cycles total sample time. */
    kLPADC_SampleTimeADCK5 = 1U,   /*!< 5 ADCK cycles total sample time. */
    kLPADC_SampleTimeADCK7 = 2U,   /*!< 7 ADCK cycles total sample time. */
    kLPADC_SampleTimeADCK11 = 3U,  /*!< 11 ADCK cycles total sample time. */
    kLPADC_SampleTimeADCK19 = 4U,  /*!< 19 ADCK cycles total sample time. */
    kLPADC_SampleTimeADCK35 = 5U,  /*!< 35 ADCK cycles total sample time. */
    kLPADC_SampleTimeADCK67 = 6U,  /*!< 69 ADCK cycles total sample time. */
    kLPADC_SampleTimeADCK131 = 7U, /*!< 131 ADCK cycles total sample time. */
} lpadc_sample_time_mode_t;

/*!
 * @brief Define enumeration of hardware compare mode.
 *
 * After an ADC channel input is sampled and converted and any averaging iterations are performed, this mode setting
 * guides operation of the automatic compare function to optionally only store when the compare operation is true.
 * When compare is enabled, the conversion result is compared to the compare values.
 */
typedef enum _lpadc_hardware_compare_mode
{
    kLPADC_HardwareCompareDisabled = 0U,        /*!< Compare disabled. */
    kLPADC_HardwareCompareStoreOnTrue = 2U,     /*!< Compare enabled. Store on true. */
    kLPADC_HardwareCompareRepeatUntilTrue = 3U, /*!< Compare enabled. Repeat channel acquisition until true. */
} lpadc_hardware_compare_mode_t;

/*!
 * @brief Define enumeration of conversion resolution mode.
 *
 * Configure the resolution bit in specific conversion type. For detailed resolution accuracy, see to
 * #_lpadc_sample_channel_mode
 */
typedef enum _lpadc_conversion_resolution_mode
{
    kLPADC_ConversionResolutionStandard = 0U, /*!< Standard resolution. Single-ended 12-bit conversion, Differential
                                                   13-bit conversion with 2’s complement output. */
    kLPADC_ConversionResolutionHigh = 1U,     /*!< High resolution. Single-ended 16-bit conversion; Differential 16-bit
                                                   conversion with 2’s complement output. */
} lpadc_conversion_resolution_mode_t;

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS) && FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS
/*!
 * @brief Define enumeration of conversion averages mode.
 *
 * Configure the converion average number for auto-calibration.
 */
typedef enum _lpadc_conversion_average_mode
{
    kLPADC_ConversionAverage1 = 0U,   /*!< Single conversion. */
    kLPADC_ConversionAverage2 = 1U,   /*!< 2 conversions averaged. */
    kLPADC_ConversionAverage4 = 2U,   /*!< 4 conversions averaged. */
    kLPADC_ConversionAverage8 = 3U,   /*!< 8 conversions averaged. */
    kLPADC_ConversionAverage16 = 4U,  /*!< 16 conversions averaged. */
    kLPADC_ConversionAverage32 = 5U,  /*!< 32 conversions averaged. */
    kLPADC_ConversionAverage64 = 6U,  /*!< 64 conversions averaged. */
    kLPADC_ConversionAverage128 = 7U, /*!< 128 conversions averaged. */
} lpadc_conversion_average_mode_t;
#endif /* FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS */

/*!
 * @brief Define enumeration of reference voltage source.
 *
 * For detail information, need to check the SoC's specification.
 */
typedef enum _lpadc_reference_voltage_mode
{
    kLPADC_ReferenceVoltageAlt1 = 0U, /*!< Option 1 setting. */
    kLPADC_ReferenceVoltageAlt2 = 1U, /*!< Option 2 setting. */
    kLPADC_ReferenceVoltageAlt3 = 2U, /*!< Option 3 setting. */
} lpadc_reference_voltage_source_t;

/*!
 * @brief Define enumeration of power configuration.
 *
 * Configures the ADC for power and performance. In the highest power setting the highest conversion rates will be
 * possible. Refer to the device data sheet for power and performance capabilities for each setting.
 */
typedef enum _lpadc_power_level_mode
{
    kLPADC_PowerLevelAlt1 = 0U, /*!< Lowest power setting. */
    kLPADC_PowerLevelAlt2 = 1U, /*!< Next lowest power setting. */
    kLPADC_PowerLevelAlt3 = 2U, /*!< ... */
    kLPADC_PowerLevelAlt4 = 3U, /*!< Highest power setting. */
} lpadc_power_level_mode_t;

/*!
 * @brief Define enumeration of trigger priority policy.
 *
 * This selection controls how higher priority triggers are handled.
 */
typedef enum _lpadc_trigger_priority_policy
{
    kLPADC_TriggerPriorityPreemptImmediately = 0U, /*!< If a higher priority trigger is detected during command
                                                        processing, the current conversion is aborted and the new
                                                        command specified by the trigger is started. */
    kLPADC_TriggerPriorityPreemptSoftly = 1U, /*!< If a higher priority trigger is received during command processing,
                                                    the current conversion is completed (including averaging iterations
                                                    and compare function if enabled) and stored to the result FIFO
                                                    before the higher priority trigger/command is initiated. */
    kLPADC_TriggerPriorityPreemptSubsequently =
        2U, /*!< If a higher priority trigger is received during command processing, the current
            command will be completed (averaging, looping, compare) before servicing the
            higher priority trigger. */
} lpadc_trigger_priority_policy_t;

/*!
 * @beief LPADC global configuration.
 *
 * This structure would used to keep the settings for initialization.
 */
typedef struct
{
#if defined(FSL_FEATURE_LPADC_HAS_CFG_ADCKEN) && FSL_FEATURE_LPADC_HAS_CFG_ADCKEN
    bool enableInternalClock; /*!< Enables the internally generated clock source. The clock source is used in clock
                                   selection logic at the chip level and is optionally used for the ADC clock source. */
#endif                        /* FSL_FEATURE_LPADC_HAS_CFG_ADCKEN */
#if defined(FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG) && FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG
    bool enableVref1LowVoltage; /*!< If voltage reference option1 input is below 1.8V, it should be "true".
                                     If voltage reference option1 input is above 1.8V, it should be "false". */
#endif                          /* FSL_FEATURE_LPADC_HAS_CFG_VREF1RNG */
    bool enableInDozeMode; /*!< Control system transition to Stop and Wait power modes while ADC is converting. When
                                enabled in Doze mode, immediate entries to Wait or Stop are allowed. When disabled, the
                                ADC will wait for the current averaging iteration/FIFO storage to complete before
                                acknowledging stop or wait mode entry. */
#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS) && FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS
    lpadc_conversion_average_mode_t conversionAverageMode; /*!< Auto-Calibration Averages. */
#endif                                                     /* FSL_FEATURE_LPADC_HAS_CTRL_CAL_AVGS */
    bool enableAnalogPreliminary; /*!< ADC analog circuits are pre-enabled and ready to execute conversions without
                                       startup delays(at the cost of higher DC current consumption). */
    uint32_t powerUpDelay; /*!< When the analog circuits are not pre-enabled, the ADC analog circuits are only powered
                                while the ADC is active and there is a counted delay defined by this field after an
                                initial trigger transitions the ADC from its Idle state to allow time for the analog
                                circuits to stabilize. The startup delay count of (powerUpDelay * 4) ADCK cycles must
                                result in a longer delay than the analog startup time. */
    lpadc_reference_voltage_source_t referenceVoltageSource; /*!< Selects the voltage reference high used for
                                                                  conversions.*/
    lpadc_power_level_mode_t powerLevelMode;                 /*!< Power Configuration Selection. */
    lpadc_trigger_priority_policy_t triggerPrioirtyPolicy; /*!< Control how higher priority triggers are handled, see to
                                                                #lpadc_trigger_priority_policy_mode_t. */
    bool enableConvPause; /*!< Enables the ADC pausing function. When enabled, a programmable delay is inserted during
                               command execution sequencing between LOOP iterations, between commands in a sequence, and
                               between conversions when command is executing in "Compare Until True" configuration. */
    uint32_t convPauseDelay; /*!< Controls the duration of pausing during command execution sequencing. The pause delay
                                  is a count of (convPauseDelay*4) ADCK cycles. Only available when ADC pausing
                                  function is enabled. The available value range is in 9-bit. */
#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) && (FSL_FEATURE_LPADC_FIFO_COUNT == 2))
    /* for FIFO0. */
    uint32_t
        FIFO0Watermark; /*!< FIFO0Watermark is a programmable threshold setting. When the number of datawords stored
                            in the ADC Result FIFO0 is greater than the value in this field, the ready flag would be
                            asserted to indicate stored data has reached the programmable threshold. */
    /* for FIFO1. */
    uint32_t
        FIFO1Watermark; /*!< FIFO1Watermark is a programmable threshold setting. When the number of datawords stored
                            in the ADC Result FIFO1 is greater than the value in this field, the ready flag would be
                            asserted to indicate stored data has reached the programmable threshold. */
#else
    /* for FIFO. */
    uint32_t FIFOWatermark; /*!< FIFOWatermark is a programmable threshold setting. When the number of datawords stored
                                 in the ADC Result FIFO is greater than the value in this field, the ready flag would be
                                 asserted to indicate stored data has reached the programmable threshold. */
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */
} lpadc_config_t;

/*!
 * @brief Define structure to keep the configuration for conversion command.
 */
typedef struct
{
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_CSCALE) && FSL_FEATURE_LPADC_HAS_CMDL_CSCALE
    lpadc_sample_scale_mode_t sampleScaleMode;     /*!< Sample scale mode. */
#endif                                             /* FSL_FEATURE_LPADC_HAS_CMDL_CSCALE */
    lpadc_sample_channel_mode_t sampleChannelMode; /*!< Channel sample mode. */
    uint32_t channelNumber;                        /*!< Channel number, select the channel or channel pair. */
    uint32_t chainedNextCommandNumber; /*!< Selects the next command to be executed after this command completes.
                                            1-15 is available, 0 is to terminate the chain after this command. */
    bool enableAutoChannelIncrement;   /*!< Loop with increment: when disabled, the "loopCount" field selects the number
                                            of times the selected channel is converted consecutively; when enabled, the
                                            "loopCount" field defines how many consecutive channels are converted as part
                                            of the command execution. */
    uint32_t loopCount; /*!< Selects how many times this command executes before finish and transition to the next
                             command or Idle state. Command executes LOOP+1 times.  0-15 is available. */
    lpadc_hardware_average_mode_t hardwareAverageMode; /*!< Hardware average selection. */
    lpadc_sample_time_mode_t sampleTimeMode;           /*!< Sample time selection. */

    lpadc_hardware_compare_mode_t hardwareCompareMode; /*!< Hardware compare selection. */
    uint32_t hardwareCompareValueHigh; /*!< Compare Value High. The available value range is in 16-bit. */
    uint32_t hardwareCompareValueLow;  /*!< Compare Value Low. The available value range is in 16-bit. */
#if defined(FSL_FEATURE_LPADC_HAS_CMDL_MODE) && FSL_FEATURE_LPADC_HAS_CMDL_MODE
    lpadc_conversion_resolution_mode_t conversionResoultuionMode; /*!< Conversion resolution mode. */
#endif                                                            /* FSL_FEATURE_LPADC_HAS_CMDL_MODE */
#if defined(FSL_FEATURE_LPADC_HAS_CMDH_WAIT_TRIG) && FSL_FEATURE_LPADC_HAS_CMDH_WAIT_TRIG
    bool enableWaitTrigger; /*!< Wait for trigger assertion before execution: when disabled, this command will be
                                 automatically executed; when enabled, the active trigger must be asserted again before
                                 executing this command. */
#endif                      /* FSL_FEATURE_LPADC_HAS_CMDH_WAIT_TRIG */
} lpadc_conv_command_config_t;

/*!
 * @brief Define structure to keep the configuration for conversion trigger.
 */
typedef struct
{
    uint32_t targetCommandId; /*!< Select the command from command buffer to execute upon detect of the associated
                                   trigger event. */
    uint32_t delayPower;      /*!< Select the trigger delay duration to wait at the start of servicing a trigger event.
                                   When this field is clear, then no delay is incurred. When this field is set to a non-zero
                                   value, the duration for the delay is 2^delayPower ADCK cycles. The available value range
                                   is 4-bit. */
    uint32_t priority; /*!< Sets the priority of the associated trigger source. If two or more triggers have the same
                            priority level setting, the lower order trigger event has the higher priority. The lower
                            value for this field is for the higher priority, the available value range is 1-bit. */
#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) && (FSL_FEATURE_LPADC_FIFO_COUNT == 2))
    uint8_t channelAFIFOSelect; /* SAR Result Destination For Channel A. */
    uint8_t channelBFIFOSelect; /* SAR Result Destination For Channel B. */
#endif                          /* FSL_FEATURE_LPADC_FIFO_COUNT */
    bool enableHardwareTrigger; /*!< Enable hardware trigger source to initiate conversion on the rising edge of the
                                     input trigger source or not. THe software trigger is always available. */
} lpadc_conv_trigger_config_t;

/*!
 * @brief Define the structure to keep the conversion result.
 */
typedef struct
{
    uint32_t commandIdSource; /*!< Indicate the command buffer being executed that generated this result. */
    uint32_t loopCountIndex;  /*!< Indicate the loop count value during command execution that generated this result. */
    uint32_t triggerIdSource; /*!< Indicate the trigger source that initiated a conversion and generated this result. */
    uint16_t convValue;       /*!< Data result. */
} lpadc_conv_result_t;

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/
/*!
 * @name Initialization & de-initialization.
 * @{
 */

/*!
 * @brief Initializes the LPADC module.
 *
 * @param base   LPADC peripheral base address.
 * @param config Pointer to configuration structure. See "lpadc_config_t".
 */
void LPADC_Init(ADC_Type *base, const lpadc_config_t *config);

/*!
 * @brief Gets an available pre-defined settings for initial configuration.
 *
 * This function initializes the converter configuration structure with an available settings. The default values are:
 * @code
 *   config->enableInDozeMode        = true;
 *   config->enableAnalogPreliminary = false;
 *   config->powerUpDelay            = 0x80;
 *   config->referenceVoltageSource  = kLPADC_ReferenceVoltageAlt1;
 *   config->powerLevelMode          = kLPADC_PowerLevelAlt1;
 *   config->triggerPrioirtyPolicy   = kLPADC_TriggerPriorityPreemptImmediately;
 *   config->enableConvPause         = false;
 *   config->convPauseDelay          = 0U;
 *   config->FIFOWatermark           = 0U;
 * @endcode
 * @param config Pointer to configuration structure.
 */
void LPADC_GetDefaultConfig(lpadc_config_t *config);

/*!
 * @brief De-initializes the LPADC module.
 *
 * @param base LPADC peripheral base address.
 */
void LPADC_Deinit(ADC_Type *base);

/*!
 * @brief Switch on/off the LPADC module.
 *
 * @param base LPADC peripheral base address.
 * @param enable switcher to the module.
 */
static inline void LPADC_Enable(ADC_Type *base, bool enable)
{
    if (enable)
    {
        base->CTRL |= ADC_CTRL_ADCEN_MASK;
    }
    else
    {
        base->CTRL &= ~ADC_CTRL_ADCEN_MASK;
    }
}

#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) && (FSL_FEATURE_LPADC_FIFO_COUNT == 2))
/*!
 * @brief Do reset the conversion FIFO0.
 *
 * @param base LPADC peripheral base address.
 */
static inline void LPADC_DoResetFIFO0(ADC_Type *base)
{
    base->CTRL |= ADC_CTRL_RSTFIFO0_MASK;
}

/*!
 * @brief Do reset the conversion FIFO1.
 *
 * @param base LPADC peripheral base address.
 */
static inline void LPADC_DoResetFIFO1(ADC_Type *base)
{
    base->CTRL |= ADC_CTRL_RSTFIFO1_MASK;
}
#else
/*!
 * @brief Do reset the conversion FIFO.
 *
 * @param base LPADC peripheral base address.
 */
static inline void LPADC_DoResetFIFO(ADC_Type *base)
{
    base->CTRL |= ADC_CTRL_RSTFIFO_MASK;
}
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */

/*!
 * @brief Do reset the module's configuration.
 *
 * Reset all ADC internal logic and registers, except the Control Register (ADCx_CTRL).
 *
 * @param base LPADC peripheral base address.
 */
static inline void LPADC_DoResetConfig(ADC_Type *base)
{
    base->CTRL |= ADC_CTRL_RST_MASK;
    base->CTRL &= ~ADC_CTRL_RST_MASK;
}

/* @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Get status flags.
 *
 * @param base LPADC peripheral base address.
 * @return status flags' mask. See to #_lpadc_status_flags.
 */
static inline uint32_t LPADC_GetStatusFlags(ADC_Type *base)
{
    return base->STAT;
}

/*!
 * @brief Clear status flags.
 *
 * Only the flags can be cleared by writing ADCx_STATUS register would be cleared by this API.
 *
 * @param base LPADC peripheral base address.
 * @param mask Mask value for flags to be cleared. See to #_lpadc_status_flags.
 */
static inline void LPADC_ClearStatusFlags(ADC_Type *base, uint32_t mask)
{
    base->STAT = mask;
}

/* @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enable interrupts.
 *
 * @param base LPADC peripheral base address.
 * @mask Mask value for interrupt events. See to #_lpadc_interrupt_enable.
 */
static inline void LPADC_EnableInterrupts(ADC_Type *base, uint32_t mask)
{
    base->IE |= mask;
}

/*!
 * @brief Disable interrupts.
 *
 * @param base LPADC peripheral base address.
 * @param mask Mask value for interrupt events. See to #_lpadc_interrupt_enable.
 */
static inline void LPADC_DisableInterrupts(ADC_Type *base, uint32_t mask)
{
    base->IE &= ~mask;
}

/*!
 * @name DMA Control
 * @{
 */

#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) && (FSL_FEATURE_LPADC_FIFO_COUNT == 2))
/*!
 * @brief Switch on/off the DMA trigger for FIFO0 watermark event.
 *
 * @param base LPADC peripheral base address.
 * @param enable Switcher to the event.
 */
static inline void LPADC_EnableFIFO0WatermarkDMA(ADC_Type *base, bool enable)
{
    if (enable)
    {
        base->DE |= ADC_DE_FWMDE0_MASK;
    }
    else
    {
        base->DE &= ~ADC_DE_FWMDE0_MASK;
    }
}

/*!
 * @brief Switch on/off the DMA trigger for FIFO1 watermark event.
 *
 * @param base LPADC peripheral base address.
 * @param enable Switcher to the event.
 */
static inline void LPADC_EnableFIFO1WatermarkDMA(ADC_Type *base, bool enable)
{
    if (enable)
    {
        base->DE |= ADC_DE_FWMDE1_MASK;
    }
    else
    {
        base->DE &= ~ADC_DE_FWMDE1_MASK;
    }
}
#else
/*!
 * @brief Switch on/off the DMA trigger for FIFO watermark event.
 *
 * @param base LPADC peripheral base address.
 * @param enable Switcher to the event.
 */
static inline void LPADC_EnableFIFOWatermarkDMA(ADC_Type *base, bool enable)
{
    if (enable)
    {
        base->DE |= ADC_DE_FWMDE_MASK;
    }
    else
    {
        base->DE &= ~ADC_DE_FWMDE_MASK;
    }
}
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */
       /* @} */

/*!
 * @name Trigger and conversion with FIFO.
 * @{
 */

#if (defined(FSL_FEATURE_LPADC_FIFO_COUNT) && (FSL_FEATURE_LPADC_FIFO_COUNT == 2))
/*!
 * @brief Get the count of result kept in conversion FIFOn.
 *
 * @param base LPADC peripheral base address.
 * @param index Result FIFO index.
 * @return The count of result kept in conversion FIFOn.
 */
static inline uint32_t LPADC_GetConvResultCount(ADC_Type *base, uint8_t index)
{
    return (ADC_FCTRL_FCOUNT_MASK & base->FCTRL[index]) >> ADC_FCTRL_FCOUNT_SHIFT;
}

/*!
 * brief Get the result in conversion FIFOn.
 *
 * param base LPADC peripheral base address.
 * param result Pointer to structure variable that keeps the conversion result in conversion FIFOn.
 * param index Result FIFO index.
 *
 * return Status whether FIFOn entry is valid.
 */
bool LPADC_GetConvResult(ADC_Type *base, lpadc_conv_result_t *result, uint8_t index);
#else
/*!
 * @brief Get the count of result kept in conversion FIFO.
 *
 * @param base LPADC peripheral base address.
 * @return The count of result kept in conversion FIFO.
 */
static inline uint32_t LPADC_GetConvResultCount(ADC_Type *base)
{
    return (ADC_FCTRL_FCOUNT_MASK & base->FCTRL) >> ADC_FCTRL_FCOUNT_SHIFT;
}

/*!
 * @brief Get the result in conversion FIFO.
 *
 * @param base LPADC peripheral base address.
 * @param result Pointer to structure variable that keeps the conversion result in conversion FIFO.
 *
 * @return Status whether FIFO entry is valid.
 */
bool LPADC_GetConvResult(ADC_Type *base, lpadc_conv_result_t *result);
#endif /* FSL_FEATURE_LPADC_FIFO_COUNT */

/*!
 * @brief Configure the conversion trigger source.
 *
 * Each programmable trigger can launch the conversion command in command buffer.
 *
 * @param base LPADC peripheral base address.
 * @param triggerId ID for each trigger. Typically, the available value range is from 0.
 * @param config Pointer to configuration structure. See to #lpadc_conv_trigger_config_t.
 */
void LPADC_SetConvTriggerConfig(ADC_Type *base, uint32_t triggerId, const lpadc_conv_trigger_config_t *config);

/*!
 * @brief Gets an available pre-defined settings for trigger's configuration.
 *
 * This function initializes the trigger's configuration structure with an available settings. The default values are:
 * @code
 *   config->commandIdSource       = 0U;
 *   config->loopCountIndex        = 0U;
 *   config->triggerIdSource       = 0U;
 *   config->enableHardwareTrigger = false;
 * @endcode
 * @param config Pointer to configuration structure.
 */
void LPADC_GetDefaultConvTriggerConfig(lpadc_conv_trigger_config_t *config);

/*!
 * @brief Do software trigger to conversion command.
 *
 * @param base LPADC peripheral base address.
 * @param triggerIdMask Mask value for software trigger indexes, which count from zero.
 */
static inline void LPADC_DoSoftwareTrigger(ADC_Type *base, uint32_t triggerIdMask)
{
    /* Writes to ADCx_SWTRIG register are ignored while ADCx_CTRL[ADCEN] is clear. */
    base->SWTRIG = triggerIdMask;
}

/*!
 * @brief Configure conversion command.
 *
 * @param base LPADC peripheral base address.
 * @param commandId ID for command in command buffer. Typically, the available value range is 1 - 15.
 * @param config Pointer to configuration structure. See to #lpadc_conv_command_config_t.
 */
void LPADC_SetConvCommandConfig(ADC_Type *base, uint32_t commandId, const lpadc_conv_command_config_t *config);

/*!
 * @brief Gets an available pre-defined settings for conversion command's configuration.
 *
 * This function initializes the conversion command's configuration structure with an available settings. The default
 * values are:
 * @code
 *   config->sampleScaleMode            = kLPADC_SampleFullScale;
 *   config->channelSampleMode          = kLPADC_SampleChannelSingleEndSideA;
 *   config->channelNumber              = 0U;
 *   config->chainedNextCmdNumber       = 0U;
 *   config->enableAutoChannelIncrement = false;
 *   config->loopCount                  = 0U;
 *   config->hardwareAverageMode        = kLPADC_HardwareAverageCount1;
 *   config->sampleTimeMode             = kLPADC_SampleTimeADCK3;
 *   config->hardwareCompareMode        = kLPADC_HardwareCompareDisabled;
 *   config->hardwareCompareValueHigh   = 0U;
 *   config->hardwareCompareValueLow    = 0U;
 *   config->conversionResoultuionMode  = kLPADC_ConversionResolutionStandard;
 *   config->enableWaitTrigger          = false;
 * @endcode
 * @param config Pointer to configuration structure.
 */
void LPADC_GetDefaultConvCommandConfig(lpadc_conv_command_config_t *config);

#if defined(FSL_FEATURE_LPADC_HAS_CFG_CALOFS) && FSL_FEATURE_LPADC_HAS_CFG_CALOFS
/*!
 * @brief Enable the calibration function.
 *
 * When CALOFS is set, the ADC is configured to perform a calibration function anytime the ADC executes
 * a conversion. Any channel selected is ignored and the value returned in the RESFIFO is a signed value
 * between -31 and 31. -32 is not a valid and is never a returned value. Software should copy the lower 6-
 * bits of the conversion result stored in the RESFIFO after a completed calibration conversion to the
 * OFSTRIM field. The OFSTRIM field is used in normal operation for offset correction.
 *
 * @param base LPADC peripheral base address.
 * @bool enable switcher to the calibration function.
 */
void LPADC_EnableCalibration(ADC_Type *base, bool enable);
#if defined(FSL_FEATURE_LPADC_HAS_OFSTRIM) && FSL_FEATURE_LPADC_HAS_OFSTRIM
/*!
 * @brief Set proper offset value to trim ADC.
 *
 * To minimize the offset during normal operation, software should read the conversion result from
 * the RESFIFO calibration operation and write the lower 6 bits to the OFSTRIM register.
 *
 * @param base  LPADC peripheral base address.
 * @param value Setting offset value.
 */
static inline void LPADC_SetOffsetValue(ADC_Type *base, uint32_t value)
{
    base->OFSTRIM = (value & ADC_OFSTRIM_OFSTRIM_MASK) >> ADC_OFSTRIM_OFSTRIM_SHIFT;
}

/*!
* @brief Do auto calibration.
*
* Calibration function should be executed before using converter in application. It used the software trigger and a
* dummy conversion, get the offset and write them into the OFSTRIM register. It called some of functional API including:
*   -LPADC_EnableCalibration(...)
*   -LPADC_LPADC_SetOffsetValue(...)
*   -LPADC_SetConvCommandConfig(...)
*   -LPADC_SetConvTriggerConfig(...)
*
* @param base  LPADC peripheral base address.
*/
void LPADC_DoAutoCalibration(ADC_Type *base);
#endif /* FSL_FEATURE_LPADC_HAS_OFSTRIM */
#endif /* FSL_FEATURE_LPADC_HAS_CFG_CALOFS */

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CALOFS) && FSL_FEATURE_LPADC_HAS_CTRL_CALOFS
#if defined(FSL_FEATURE_LPADC_HAS_OFSTRIM) && FSL_FEATURE_LPADC_HAS_OFSTRIM
/*!
 * @brief Set proper offset value to trim ADC.
 *
 * Set the offset trim value for offset calibration manually.
 *
 * @param base  LPADC peripheral base address.
 * @param valueA Setting offset value A.
 * @param valueB Setting offset value B.
 * @note In normal adc sequence, the values are automatically calculated by LPADC_EnableOffsetCalibration.
 */
static inline void LPADC_SetOffsetValue(ADC_Type *base, uint32_t valueA, uint32_t valueB)
{
    base->OFSTRIM = ADC_OFSTRIM_OFSTRIM_A(valueA) | ADC_OFSTRIM_OFSTRIM_B(valueB);
}
#endif /* FSL_FEATURE_LPADC_HAS_OFSTRIM */

/*!
 * @brief Enable the offset calibration function.
 *
 * @param base LPADC peripheral base address.
 * @bool enable switcher to the calibration function.
 */
static inline void LPADC_EnableOffsetCalibration(ADC_Type *base, bool enable)
{
    if (enable)
    {
        base->CTRL |= ADC_CTRL_CALOFS_MASK;
    }
    else
    {
        base->CTRL &= ~ADC_CTRL_CALOFS_MASK;
    }
}

/*!
 * @brief Do offset calibration.
 *
 * @param base LPADC peripheral base address.
 */
void LPADC_DoOffsetCalibration(ADC_Type *base);

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ) && FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ
/*!
 * brief Do auto calibration.
 *
 * param base  LPADC peripheral base address.
 */
void LPADC_DoAutoCalibration(ADC_Type *base);
#endif /* FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ */
#endif /* FSL_FEATURE_LPADC_HAS_CTRL_CALOFS */
/* @} */

#if defined(__cplusplus)
}
#endif
/*!
 * @}
 */
#endif /* _FSL_LPADC_H_ */
