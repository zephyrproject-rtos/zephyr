/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_ACMP_H_
#define _FSL_ACMP_H_

#include "fsl_common.h"

/*!
 * @addtogroup acmp
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief ACMP driver version 2.0.4. */
#define FSL_ACMP_DRIVER_VERSION (MAKE_VERSION(2U, 0U, 4U))
/*@}*/

/*! @brief The mask of status flags cleared by writing 1. */
#define CMP_C0_CFx_MASK (CMP_C0_CFR_MASK | CMP_C0_CFF_MASK)
#define CMP_C1_CHNn_MASK 0xFF0000U /* C1_CHN0 - C1_CHN7. */
#define CMP_C2_CHnF_MASK 0xFF0000U /* C2_CH0F - C2_CH7F. */

/*! @brief Interrupt enable/disable mask. */
enum _acmp_interrupt_enable
{
    kACMP_OutputRisingInterruptEnable = (1U << 0U),  /*!< Enable the interrupt when comparator outputs rising. */
    kACMP_OutputFallingInterruptEnable = (1U << 1U), /*!< Enable the interrupt when comparator outputs falling. */
    kACMP_RoundRobinInterruptEnable = (1U << 2U),    /*!< Enable the Round-Robin interrupt. */
};

/*! @brief Status flag mask. */
enum _acmp_status_flags
{
    kACMP_OutputRisingEventFlag = CMP_C0_CFR_MASK,  /*!< Rising-edge on compare output has occurred. */
    kACMP_OutputFallingEventFlag = CMP_C0_CFF_MASK, /*!< Falling-edge on compare output has occurred. */
    kACMP_OutputAssertEventFlag = CMP_C0_COUT_MASK, /*!< Return the current value of the analog comparator output. */
};

#if defined(FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT) && (FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT == 1U)
/*!
 * @brief Comparator hard block offset control.
 *
 * If OFFSET level is 1, then there is no hysteresis in the case of positive port input crossing negative port
 * input in the positive direction (or negative port input crossing positive port input in the negative direction).
 * Hysteresis still exists for positive port input crossing negative port input in the falling direction.
 * If OFFSET level is 0, then the hysteresis selected by acmp_hysteresis_mode_t is valid for both directions.
 */
typedef enum _acmp_offset_mode
{
    kACMP_OffsetLevel0 = 0U, /*!< The comparator hard block output has level 0 offset internally. */
    kACMP_OffsetLevel1 = 1U, /*!< The comparator hard block output has level 1 offset internally. */
} acmp_offset_mode_t;
#endif /* FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT */

/*!
 * @brief Comparator hard block hysteresis control.
 *
 * See chip data sheet to get the actual hysteresis value with each level.
 */
typedef enum _acmp_hysteresis_mode
{
    kACMP_HysteresisLevel0 = 0U, /*!< Offset is level 0 and Hysteresis is level 0. */
    kACMP_HysteresisLevel1 = 1U, /*!< Offset is level 0 and Hysteresis is level 1. */
    kACMP_HysteresisLevel2 = 2U, /*!< Offset is level 0 and Hysteresis is level 2. */
    kACMP_HysteresisLevel3 = 3U, /*!< Offset is level 0 and Hysteresis is level 3. */
} acmp_hysteresis_mode_t;

/*! @brief CMP Voltage Reference source. */
typedef enum _acmp_reference_voltage_source
{
    kACMP_VrefSourceVin1 = 0U, /*!< Vin1 is selected as resistor ladder network supply reference Vin. */
    kACMP_VrefSourceVin2 = 1U, /*!< Vin2 is selected as resistor ladder network supply reference Vin. */
} acmp_reference_voltage_source_t;

#if defined(FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT == 1U)
/*! @brief Port input source. */
typedef enum _acmp_port_input
{
    kACMP_PortInputFromDAC = 0U, /*!< Port input from the 8-bit DAC output. */
    kACMP_PortInputFromMux = 1U, /*!< Port input from the analog 8-1 mux. */
} acmp_port_input_t;
#endif /* FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT */

/*! @brief Fixed mux port. */
typedef enum _acmp_fixed_port
{
    kACMP_FixedPlusPort = 0U,  /*!< Only the inputs to the Minus port are swept in each round. */
    kACMP_FixedMinusPort = 1U, /*!< Only the inputs to the Plus port are swept in each round. */
} acmp_fixed_port_t;

#if defined(FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT) && (FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT == 1U)
/*! @brief Internal DAC's work mode. */
typedef enum _acmp_dac_work_mode
{
    kACMP_DACWorkLowSpeedMode = 0U,  /*!< DAC is selected to work in low speed and low power mode. */
    kACMP_DACWorkHighSpeedMode = 1U, /*!< DAC is selected to work in high speed high power mode. */
} acmp_dac_work_mode_t;
#endif /* FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT */

/*! @brief Configuration for ACMP. */
typedef struct _acmp_config
{
#if defined(FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT) && (FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT == 1U)
    acmp_offset_mode_t offsetMode;         /*!< Offset mode. */
#endif                                     /* FSL_FEATURE_ACMP_HAS_C0_OFFSET_BIT */
    acmp_hysteresis_mode_t hysteresisMode; /*!< Hysteresis mode. */
    bool enableHighSpeed;                  /*!< Enable High Speed (HS) comparison mode. */
    bool enableInvertOutput;               /*!< Enable inverted comparator output. */
    bool useUnfilteredOutput;              /*!< Set compare output(COUT) to equal COUTA(true) or COUT(false). */
    bool enablePinOut;                     /*!< The comparator output is available on the associated pin. */
} acmp_config_t;

/*!
 * @brief Configuration for channel.
 *
 * The comparator's port can be input from channel mux or DAC. If port input is from channel mux, detailed channel
 * number for the mux should be configured.
 */
typedef struct _acmp_channel_config
{
#if defined(FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT == 1U)
    acmp_port_input_t positivePortInput; /*!< Input source of the comparator's positive port. */
#endif                                   /* FSL_FEATURE_ACMP_HAS_C1_INPSEL_BIT */
    uint32_t plusMuxInput;               /*!< Plus mux input channel(0~7). */
#if defined(FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT) && (FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT == 1U)
    acmp_port_input_t negativePortInput; /*!< Input source of the comparator's negative port. */
#endif                                   /* FSL_FEATURE_ACMP_HAS_C1_INNSEL_BIT */
    uint32_t minusMuxInput;              /*!< Minus mux input channel(0~7). */
} acmp_channel_config_t;

/*! @brief Configuration for filter. */
typedef struct _acmp_filter_config
{
    bool enableSample;     /*!< Using external SAMPLE as sampling clock input, or using divided bus clock. */
    uint32_t filterCount;  /*!< Filter Sample Count. Available range is 1-7, 0 would cause the filter disabled. */
    uint32_t filterPeriod; /*!< Filter Sample Period. The divider to bus clock. Available range is 0-255. */
} acmp_filter_config_t;

/*! @brief Configuration for DAC. */
typedef struct _acmp_dac_config
{
    acmp_reference_voltage_source_t referenceVoltageSource; /*!< Supply voltage reference source. */
    uint32_t DACValue; /*!< Value for DAC Output Voltage. Available range is 0-63. */

#if defined(FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT) && (FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT == 1U)
    bool enableOutput; /*!< Enable the DAC output. */
#endif                 /* FSL_FEATURE_ACMP_HAS_C1_DACOE_BIT */

#if defined(FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT) && (FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT == 1U)
    acmp_dac_work_mode_t workMode;
#endif /* FSL_FEATURE_ACMP_HAS_C1_DMODE_BIT */
} acmp_dac_config_t;

/*! @brief Configuration for round robin mode. */
typedef struct _acmp_round_robin_config
{
    acmp_fixed_port_t fixedPort; /*!< Fixed mux port. */
    uint32_t fixedChannelNumber; /*!< Indicates which channel is fixed in the fixed mux port. */
    uint32_t checkerChannelMask; /*!< Mask of checker channel index. Available range is channel0:0x01 to channel7:0x80
                                    for round-robin checker. */
    uint32_t sampleClockCount;   /*!< Specifies how many round-robin clock cycles(0~3) later the sample takes place. */
    uint32_t delayModulus;       /*!< Comparator and DAC initialization delay modulus. */
} acmp_round_robin_config_t;

#if defined(FSL_FEATURE_ACMP_HAS_C3_REG) && (FSL_FEATURE_ACMP_HAS_C3_REG == 1U)

/*! @brief Discrete mode clock selection. */
typedef enum _acmp_discrete_clock_source
{
    kACMP_DiscreteClockSlow = 0U, /*!< Slow clock (32kHz) is used as the discrete mode clock. */
    kACMP_DiscreteClockFast = 1U, /*!< Fast clock (16-20MHz) is used as the discrete mode clock. */
} acmp_discrete_clock_source_t;

/*!
 * @brief ACMP discrete sample selection.
 * These values configures the analog comparator sampling timing (speicified by the discrete mode clock period T which
 * is selected by #acmp_discrete_clock_source_t) in discrete mode.
 */
typedef enum _acmp_discrete_sample_time
{
    kACMP_DiscreteSampleTimeAs1T = 0U,   /*!< The sampling time equals to 1xT. */
    kACMP_DiscreteSampleTimeAs2T = 1U,   /*!< The sampling time equals to 2xT. */
    kACMP_DiscreteSampleTimeAs4T = 2U,   /*!< The sampling time equals to 4xT. */
    kACMP_DiscreteSampleTimeAs8T = 3U,   /*!< The sampling time equals to 8xT. */
    kACMP_DiscreteSampleTimeAs16T = 4U,  /*!< The sampling time equals to 16xT. */
    kACMP_DiscreteSampleTimeAs32T = 5U,  /*!< The sampling time equals to 32xT. */
    kACMP_DiscreteSampleTimeAs64T = 6U,  /*!< The sampling time equals to 64xT. */
    kACMP_DiscreteSampleTimeAs256T = 7U, /*!< The sampling time equals to 256xT. */
} acmp_discrete_sample_time_t;

/*!
 * @brief ACMP discrete phase time selection.
 * There are two phases for sampling input signals, phase 1 and phase 2.
 */
typedef enum _acmp_discrete_phase_time
{
    kACMP_DiscretePhaseTimeAlt0 = 0U, /*!< The phase x active in one sampling selection 0. */
    kACMP_DiscretePhaseTimeAlt1 = 1U, /*!< The phase x active in one sampling selection 1. */
    kACMP_DiscretePhaseTimeAlt2 = 2U, /*!< The phase x active in one sampling selection 2. */
    kACMP_DiscretePhaseTimeAlt3 = 3U, /*!< The phase x active in one sampling selection 3. */
    kACMP_DiscretePhaseTimeAlt4 = 4U, /*!< The phase x active in one sampling selection 4. */
    kACMP_DiscretePhaseTimeAlt5 = 5U, /*!< The phase x active in one sampling selection 5. */
    kACMP_DiscretePhaseTimeAlt6 = 6U, /*!< The phase x active in one sampling selection 6. */
    kACMP_DiscretePhaseTimeAlt7 = 7U, /*!< The phase x active in one sampling selection 7. */
} acmp_discrete_phase_time_t;

/*! @brief Configuration for discrete mode. */
typedef struct _acmp_discrete_mode_config
{
    bool enablePositiveChannelDiscreteMode; /*!< Positive Channel Continuous Mode Enable. By default, the continuous
                                                 mode is used. */
    bool enableNegativeChannelDiscreteMode; /*!< Negative Channel Continuous Mode Enable. By default, the continuous
                                                 mode is used. */
    bool enableResistorDivider; /*!< Resistor Divider Enable is used to enable the resistor divider for the inputs when
                                     they come from 3v domain and their values are above 1.8v. */
    acmp_discrete_clock_source_t clockSource; /*!< Select the clock source in order to generate the requiried timing for
                                                   comparator to work in discrete mode.  */
    acmp_discrete_sample_time_t sampleTime;   /*!< Select the ACMP total sampling time period. */
    acmp_discrete_phase_time_t phase1Time;    /*!< Select the ACMP phase 1 sampling time. */
    acmp_discrete_phase_time_t phase2Time;    /*!< Select the ACMP phase 2 sampling time. */
} acmp_discrete_mode_config_t;

#endif /* FSL_FEATURE_ACMP_HAS_C3_REG */

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Initializes the ACMP.
 *
 * The default configuration can be got by calling ACMP_GetDefaultConfig().
 *
 * @param base ACMP peripheral base address.
 * @param config Pointer to ACMP configuration structure.
 */
void ACMP_Init(CMP_Type *base, const acmp_config_t *config);

/*!
 * @brief Deinitializes the ACMP.
 *
 * @param base ACMP peripheral base address.
 */
void ACMP_Deinit(CMP_Type *base);

/*!
 * @brief Gets the default configuration for ACMP.
 *
 * This function initializes the user configuration structure to default value. The default value are:
 *
 * Example:
   @code
   config->enableHighSpeed = false;
   config->enableInvertOutput = false;
   config->useUnfilteredOutput = false;
   config->enablePinOut = false;
   config->enableHysteresisBothDirections = false;
   config->hysteresisMode = kACMP_hysteresisMode0;
   @endcode
 *
 * @param config Pointer to ACMP configuration structure.
 */
void ACMP_GetDefaultConfig(acmp_config_t *config);

/* @} */

/*!
 * @name Basic Operations
 * @{
 */

/*!
 * @brief Enables or disables the ACMP.
 *
 * @param base ACMP peripheral base address.
 * @param enable True to enable the ACMP.
 */
void ACMP_Enable(CMP_Type *base, bool enable);

#if defined(FSL_FEATURE_ACMP_HAS_C0_LINKEN_BIT) && (FSL_FEATURE_ACMP_HAS_C0_LINKEN_BIT == 1U)
/*!
 * @brief Enables the link from CMP to DAC enable.
 *
 * When this bit is set, the DAC enable/disable is controlled by the bit CMP_C0[EN] instead of CMP_C1[DACEN].
 *
 * @param base ACMP peripheral base address.
 * @param enable Enable the feature or not.
 */
void ACMP_EnableLinkToDAC(CMP_Type *base, bool enable);
#endif /* FSL_FEATURE_ACMP_HAS_C0_LINKEN_BIT */

/*!
 * @brief Sets the channel configuration.
 *
 * Note that the plus/minus mux's setting is only valid when the positive/negative port's input isn't from DAC but
 * from channel mux.
 *
 * Example:
   @code
   acmp_channel_config_t configStruct = {0};
   configStruct.positivePortInput = kACMP_PortInputFromDAC;
   configStruct.negativePortInput = kACMP_PortInputFromMux;
   configStruct.minusMuxInput = 1U;
   ACMP_SetChannelConfig(CMP0, &configStruct);
   @endcode
 *
 * @param base ACMP peripheral base address.
 * @param config Pointer to channel configuration structure.
 */
void ACMP_SetChannelConfig(CMP_Type *base, const acmp_channel_config_t *config);

/* @} */

/*!
 * @name Advanced Operations
 * @{
 */

/*!
 * @brief Enables or disables DMA.
 *
 * @param base ACMP peripheral base address.
 * @param enable True to enable DMA.
 */
void ACMP_EnableDMA(CMP_Type *base, bool enable);

/*!
 * @brief Enables or disables window mode.
 *
 * @param base ACMP peripheral base address.
 * @param enable True to enable window mode.
 */
void ACMP_EnableWindowMode(CMP_Type *base, bool enable);

/*!
 * @brief Configures the filter.
 *
 * The filter can be enabled when the filter count is bigger than 1, the filter period is greater than 0 and the sample
 * clock is from divided bus clock or the filter is bigger than 1 and the sample clock is from external clock. Detailed
 * usage can be got from the reference manual.
 *
 * Example:
   @code
   acmp_filter_config_t configStruct = {0};
   configStruct.filterCount = 5U;
   configStruct.filterPeriod = 200U;
   configStruct.enableSample = false;
   ACMP_SetFilterConfig(CMP0, &configStruct);
   @endcode
 *
 * @param base ACMP peripheral base address.
 * @param config Pointer to filter configuration structure.
 */
void ACMP_SetFilterConfig(CMP_Type *base, const acmp_filter_config_t *config);

/*!
 * @brief Configures the internal DAC.
 *
 * Example:
   @code
   acmp_dac_config_t configStruct = {0};
   configStruct.referenceVoltageSource = kACMP_VrefSourceVin1;
   configStruct.DACValue = 20U;
   configStruct.enableOutput = false;
   configStruct.workMode = kACMP_DACWorkLowSpeedMode;
   ACMP_SetDACConfig(CMP0, &configStruct);
   @endcode
 *
 * @param base ACMP peripheral base address.
 * @param config Pointer to DAC configuration structure. "NULL" is for disabling the feature.
 */
void ACMP_SetDACConfig(CMP_Type *base, const acmp_dac_config_t *config);

/*!
 * @brief Configures the round robin mode.
 *
 * Example:
   @code
   acmp_round_robin_config_t configStruct = {0};
   configStruct.fixedPort = kACMP_FixedPlusPort;
   configStruct.fixedChannelNumber = 3U;
   configStruct.checkerChannelMask = 0xF7U;
   configStruct.sampleClockCount = 0U;
   configStruct.delayModulus = 0U;
   ACMP_SetRoundRobinConfig(CMP0, &configStruct);
   @endcode
 * @param base ACMP peripheral base address.
 * @param config Pointer to round robin mode configuration structure. "NULL" is for disabling the feature.
 */
void ACMP_SetRoundRobinConfig(CMP_Type *base, const acmp_round_robin_config_t *config);

/*!
 * @brief Defines the pre-set state of channels in round robin mode.
 *
 * Note: The pre-state has different circuit with get-round-robin-result in the SOC even though they are same bits.
 * So get-round-robin-result can't return the same value as the value are set by pre-state.
 *
 * @param base ACMP peripheral base address.
 * @param mask Mask of round robin channel index. Available range is channel0:0x01 to channel7:0x80.
 */
void ACMP_SetRoundRobinPreState(CMP_Type *base, uint32_t mask);

/*!
 * @brief Gets the channel input changed flags in round robin mode.
 *
 * @param base ACMP peripheral base address.
 * @return Mask of channel input changed asserted flags. Available range is channel0:0x01 to channel7:0x80.
 */
static inline uint32_t ACMP_GetRoundRobinStatusFlags(CMP_Type *base)
{
    return (((base->C2) & CMP_C2_CHnF_MASK) >> CMP_C2_CH0F_SHIFT);
}

/*!
 * @brief Clears the channel input changed flags in round robin mode.
 *
 * @param base ACMP peripheral base address.
 * @param mask Mask of channel index. Available range is channel0:0x01 to channel7:0x80.
 */
void ACMP_ClearRoundRobinStatusFlags(CMP_Type *base, uint32_t mask);

/*!
 * @brief Gets the round robin result.
 *
 * Note that the set-pre-state has different circuit with get-round-robin-result in the SOC even though they are same
 * bits. So ACMP_GetRoundRobinResult() can't return the same value as the value are set by ACMP_SetRoundRobinPreState.

 * @param base ACMP peripheral base address.
 * @return Mask of round robin channel result. Available range is channel0:0x01 to channel7:0x80.
 */
static inline uint32_t ACMP_GetRoundRobinResult(CMP_Type *base)
{
    return ((base->C2 & CMP_C2_ACOn_MASK) >> CMP_C2_ACOn_SHIFT);
}

/* @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enables interrupts.
 *
 * @param base ACMP peripheral base address.
 * @param mask Interrupts mask. See "_acmp_interrupt_enable".
 */
void ACMP_EnableInterrupts(CMP_Type *base, uint32_t mask);

/*!
 * @brief Disables interrupts.
 *
 * @param base ACMP peripheral base address.
 * @param mask Interrupts mask. See "_acmp_interrupt_enable".
 */
void ACMP_DisableInterrupts(CMP_Type *base, uint32_t mask);

/* @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Gets status flags.
 *
 * @param base ACMP peripheral base address.
 * @return Status flags asserted mask. See "_acmp_status_flags".
 */
uint32_t ACMP_GetStatusFlags(CMP_Type *base);

/*!
 * @brief Clears status flags.
 *
 * @param base ACMP peripheral base address.
 * @param mask Status flags mask. See "_acmp_status_flags".
 */
void ACMP_ClearStatusFlags(CMP_Type *base, uint32_t mask);

/* @} */

#if defined(FSL_FEATURE_ACMP_HAS_C3_REG) && (FSL_FEATURE_ACMP_HAS_C3_REG == 1U)
/*!
 * @name Discrete mode
 * @{
 */

/*!
 * @brief Configure the discrete mode.
 *
 * Configure the discrete mode when supporting 3V domain with 1.8V core.
 *
 * @param base ACMP peripheral base address.
 * @param config Pointer to configuration structure. See "acmp_discrete_mode_config_t".
 */
void ACMP_SetDiscreteModeConfig(CMP_Type *base, const acmp_discrete_mode_config_t *config);

/*!
 * @brief Get the default configuration for discrete mode setting.
 *
 * @param config Pointer to configuration structure to be restored with the setting values.
 */
void ACMP_GetDefaultDiscreteModeConfig(acmp_discrete_mode_config_t *config);

/* @} */
#endif /* FSL_FEATURE_ACMP_HAS_C3_REG */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_ACMP_H_ */
