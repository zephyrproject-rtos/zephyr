/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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
#ifndef _FSL_TPM_H_
#define _FSL_TPM_H_

#include "fsl_common.h"

/*!
 * @addtogroup tpm
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_TPM_DRIVER_VERSION (MAKE_VERSION(2, 0, 2)) /*!< Version 2.0.2 */
/*@}*/

/*!
 * @brief List of TPM channels.
 * @note Actual number of available channels is SoC dependent
 */
typedef enum _tpm_chnl
{
    kTPM_Chnl_0 = 0U, /*!< TPM channel number 0*/
    kTPM_Chnl_1,      /*!< TPM channel number 1 */
    kTPM_Chnl_2,      /*!< TPM channel number 2 */
    kTPM_Chnl_3,      /*!< TPM channel number 3 */
    kTPM_Chnl_4,      /*!< TPM channel number 4 */
    kTPM_Chnl_5,      /*!< TPM channel number 5 */
    kTPM_Chnl_6,      /*!< TPM channel number 6 */
    kTPM_Chnl_7       /*!< TPM channel number 7 */
} tpm_chnl_t;

/*! @brief TPM PWM operation modes */
typedef enum _tpm_pwm_mode
{
    kTPM_EdgeAlignedPwm = 0U, /*!< Edge aligned PWM */
    kTPM_CenterAlignedPwm,    /*!< Center aligned PWM */
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
    kTPM_CombinedPwm /*!< Combined PWM */
#endif
} tpm_pwm_mode_t;

/*! @brief TPM PWM output pulse mode: high-true, low-true or no output */
typedef enum _tpm_pwm_level_select
{
    kTPM_NoPwmSignal = 0U, /*!< No PWM output on pin */
    kTPM_LowTrue,          /*!< Low true pulses */
    kTPM_HighTrue          /*!< High true pulses */
} tpm_pwm_level_select_t;

/*! @brief Options to configure a TPM channel's PWM signal */
typedef struct _tpm_chnl_pwm_signal_param
{
    tpm_chnl_t chnlNumber;        /*!< TPM channel to configure.
                                       In combined mode (available in some SoC's, this represents the
                                       channel pair number */
    tpm_pwm_level_select_t level; /*!< PWM output active level select */
    uint8_t dutyCyclePercent;     /*!< PWM pulse width, value should be between 0 to 100
                                       0=inactive signal(0% duty cycle)...
                                       100=always active signal (100% duty cycle)*/
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
    uint8_t firstEdgeDelayPercent; /*!< Used only in combined PWM mode to generate asymmetrical PWM.
                                        Specifies the delay to the first edge in a PWM period.
                                        If unsure, leave as 0; Should be specified as
                                        percentage of the PWM period */
#endif
} tpm_chnl_pwm_signal_param_t;

/*!
 * @brief Trigger options available.
 *
 * This is used for both internal & external trigger sources (external option available in certain SoC's)
 *
 * @note The actual trigger options available is SoC-specific.
 */
typedef enum _tpm_trigger_select
{
    kTPM_Trigger_Select_0 = 0U,
    kTPM_Trigger_Select_1,
    kTPM_Trigger_Select_2,
    kTPM_Trigger_Select_3,
    kTPM_Trigger_Select_4,
    kTPM_Trigger_Select_5,
    kTPM_Trigger_Select_6,
    kTPM_Trigger_Select_7,
    kTPM_Trigger_Select_8,
    kTPM_Trigger_Select_9,
    kTPM_Trigger_Select_10,
    kTPM_Trigger_Select_11,
    kTPM_Trigger_Select_12,
    kTPM_Trigger_Select_13,
    kTPM_Trigger_Select_14,
    kTPM_Trigger_Select_15
} tpm_trigger_select_t;

#if defined(FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION) && FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION
/*!
 * @brief Trigger source options available
 *
 * @note This selection is available only on some SoC's. For SoC's without this selection, the only
 * trigger source available is internal triger.
 */
typedef enum _tpm_trigger_source
{
    kTPM_TriggerSource_External = 0U, /*!< Use external trigger input */
    kTPM_TriggerSource_Internal       /*!< Use internal trigger */
} tpm_trigger_source_t;
#endif

/*! @brief TPM output compare modes */
typedef enum _tpm_output_compare_mode
{
    kTPM_NoOutputSignal = (1U << TPM_CnSC_MSA_SHIFT), /*!< No channel output when counter reaches CnV  */
    kTPM_ToggleOnMatch = ((1U << TPM_CnSC_MSA_SHIFT) | (1U << TPM_CnSC_ELSA_SHIFT)),   /*!< Toggle output */
    kTPM_ClearOnMatch = ((1U << TPM_CnSC_MSA_SHIFT) | (2U << TPM_CnSC_ELSA_SHIFT)),    /*!< Clear output */
    kTPM_SetOnMatch = ((1U << TPM_CnSC_MSA_SHIFT) | (3U << TPM_CnSC_ELSA_SHIFT)),      /*!< Set output */
    kTPM_HighPulseOutput = ((3U << TPM_CnSC_MSA_SHIFT) | (1U << TPM_CnSC_ELSA_SHIFT)), /*!< Pulse output high */
    kTPM_LowPulseOutput = ((3U << TPM_CnSC_MSA_SHIFT) | (2U << TPM_CnSC_ELSA_SHIFT))   /*!< Pulse output low */
} tpm_output_compare_mode_t;

/*! @brief TPM input capture edge */
typedef enum _tpm_input_capture_edge
{
    kTPM_RisingEdge = (1U << TPM_CnSC_ELSA_SHIFT),     /*!< Capture on rising edge only */
    kTPM_FallingEdge = (2U << TPM_CnSC_ELSA_SHIFT),    /*!< Capture on falling edge only */
    kTPM_RiseAndFallEdge = (3U << TPM_CnSC_ELSA_SHIFT) /*!< Capture on rising or falling edge */
} tpm_input_capture_edge_t;

#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
/*!
 * @brief TPM dual edge capture parameters
 *
 * @note This mode is available only on some SoC's.
 */
typedef struct _tpm_dual_edge_capture_param
{
    bool enableSwap;                           /*!< true: Use channel n+1 input, channel n input is ignored;
                                                    false: Use channel n input, channel n+1 input is ignored */
    tpm_input_capture_edge_t currChanEdgeMode; /*!< Input capture edge select for channel n */
    tpm_input_capture_edge_t nextChanEdgeMode; /*!< Input capture edge select for channel n+1 */
} tpm_dual_edge_capture_param_t;
#endif

#if defined(FSL_FEATURE_TPM_HAS_QDCTRL) && FSL_FEATURE_TPM_HAS_QDCTRL
/*!
 * @brief TPM quadrature decode modes
 *
 * @note This mode is available only on some SoC's.
 */
typedef enum _tpm_quad_decode_mode
{
    kTPM_QuadPhaseEncode = 0U, /*!< Phase A and Phase B encoding mode */
    kTPM_QuadCountAndDir       /*!< Count and direction encoding mode */
} tpm_quad_decode_mode_t;

/*! @brief TPM quadrature phase polarities */
typedef enum _tpm_phase_polarity
{
    kTPM_QuadPhaseNormal = 0U, /*!< Phase input signal is not inverted */
    kTPM_QuadPhaseInvert       /*!< Phase input signal is inverted */
} tpm_phase_polarity_t;

/*! @brief TPM quadrature decode phase parameters */
typedef struct _tpm_phase_param
{
    uint32_t phaseFilterVal;            /*!< Filter value, filter is disabled when the value is zero */
    tpm_phase_polarity_t phasePolarity; /*!< Phase polarity */
} tpm_phase_params_t;
#endif

/*! @brief TPM clock source selection*/
typedef enum _tpm_clock_source
{
    kTPM_SystemClock = 1U, /*!< System clock */
    kTPM_ExternalClock     /*!< External clock */
} tpm_clock_source_t;

/*! @brief TPM prescale value selection for the clock source*/
typedef enum _tpm_clock_prescale
{
    kTPM_Prescale_Divide_1 = 0U, /*!< Divide by 1 */
    kTPM_Prescale_Divide_2,      /*!< Divide by 2 */
    kTPM_Prescale_Divide_4,      /*!< Divide by 4 */
    kTPM_Prescale_Divide_8,      /*!< Divide by 8 */
    kTPM_Prescale_Divide_16,     /*!< Divide by 16 */
    kTPM_Prescale_Divide_32,     /*!< Divide by 32 */
    kTPM_Prescale_Divide_64,     /*!< Divide by 64 */
    kTPM_Prescale_Divide_128     /*!< Divide by 128 */
} tpm_clock_prescale_t;

/*!
 * @brief TPM config structure
 *
 * This structure holds the configuration settings for the TPM peripheral. To initialize this
 * structure to reasonable defaults, call the TPM_GetDefaultConfig() function and pass a
 * pointer to your config structure instance.
 *
 * The config struct can be made const so it resides in flash
 */
typedef struct _tpm_config
{
    tpm_clock_prescale_t prescale;      /*!< Select TPM clock prescale value */
    bool useGlobalTimeBase;             /*!< true: Use of an external global time base is enabled;
                                             false: disabled */
    tpm_trigger_select_t triggerSelect; /*!< Input trigger to use for controlling the counter operation */
#if defined(FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION) && FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION
    tpm_trigger_source_t triggerSource; /*!< Decides if we use external or internal trigger. */
#endif
    bool enableDoze;            /*!< true: TPM counter is paused in doze mode;
                                     false: TPM counter continues in doze mode */
    bool enableDebugMode;       /*!< true: TPM counter continues in debug mode;
                                     false: TPM counter is paused in debug mode */
    bool enableReloadOnTrigger; /*!< true: TPM counter is reloaded on trigger;
                                     false: TPM counter not reloaded */
    bool enableStopOnOverflow;  /*!< true: TPM counter stops after overflow;
                                     false: TPM counter continues running after overflow */
    bool enableStartOnTrigger;  /*!< true: TPM counter only starts when a trigger is detected;
                                     false: TPM counter starts immediately */
#if defined(FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER) && FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER
    bool enablePauseOnTrigger; /*!< true: TPM counter will pause while trigger remains asserted;
                                    false: TPM counter continues running */
#endif
} tpm_config_t;

/*! @brief List of TPM interrupts */
typedef enum _tpm_interrupt_enable
{
    kTPM_Chnl0InterruptEnable = (1U << 0),       /*!< Channel 0 interrupt.*/
    kTPM_Chnl1InterruptEnable = (1U << 1),       /*!< Channel 1 interrupt.*/
    kTPM_Chnl2InterruptEnable = (1U << 2),       /*!< Channel 2 interrupt.*/
    kTPM_Chnl3InterruptEnable = (1U << 3),       /*!< Channel 3 interrupt.*/
    kTPM_Chnl4InterruptEnable = (1U << 4),       /*!< Channel 4 interrupt.*/
    kTPM_Chnl5InterruptEnable = (1U << 5),       /*!< Channel 5 interrupt.*/
    kTPM_Chnl6InterruptEnable = (1U << 6),       /*!< Channel 6 interrupt.*/
    kTPM_Chnl7InterruptEnable = (1U << 7),       /*!< Channel 7 interrupt.*/
    kTPM_TimeOverflowInterruptEnable = (1U << 8) /*!< Time overflow interrupt.*/
} tpm_interrupt_enable_t;

/*! @brief List of TPM flags */
typedef enum _tpm_status_flags
{
    kTPM_Chnl0Flag = (1U << 0),       /*!< Channel 0 flag */
    kTPM_Chnl1Flag = (1U << 1),       /*!< Channel 1 flag */
    kTPM_Chnl2Flag = (1U << 2),       /*!< Channel 2 flag */
    kTPM_Chnl3Flag = (1U << 3),       /*!< Channel 3 flag */
    kTPM_Chnl4Flag = (1U << 4),       /*!< Channel 4 flag */
    kTPM_Chnl5Flag = (1U << 5),       /*!< Channel 5 flag */
    kTPM_Chnl6Flag = (1U << 6),       /*!< Channel 6 flag */
    kTPM_Chnl7Flag = (1U << 7),       /*!< Channel 7 flag */
    kTPM_TimeOverflowFlag = (1U << 8) /*!< Time overflow flag */
} tpm_status_flags_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Ungates the TPM clock and configures the peripheral for basic operation.
 *
 * @note This API should be called at the beginning of the application using the TPM driver.
 *
 * @param base   TPM peripheral base address
 * @param config Pointer to user's TPM config structure.
 */
void TPM_Init(TPM_Type *base, const tpm_config_t *config);

/*!
 * @brief Stops the counter and gates the TPM clock
 *
 * @param base TPM peripheral base address
 */
void TPM_Deinit(TPM_Type *base);

/*!
 * @brief  Fill in the TPM config struct with the default settings
 *
 * The default values are:
 * @code
 *     config->prescale = kTPM_Prescale_Divide_1;
 *     config->useGlobalTimeBase = false;
 *     config->dozeEnable = false;
 *     config->dbgMode = false;
 *     config->enableReloadOnTrigger = false;
 *     config->enableStopOnOverflow = false;
 *     config->enableStartOnTrigger = false;
 *#if FSL_FEATURE_TPM_HAS_PAUSE_COUNTER_ON_TRIGGER
 *     config->enablePauseOnTrigger = false;
 *#endif
 *     config->triggerSelect = kTPM_Trigger_Select_0;
 *#if FSL_FEATURE_TPM_HAS_EXTERNAL_TRIGGER_SELECTION
 *     config->triggerSource = kTPM_TriggerSource_External;
 *#endif
 * @endcode
 * @param config Pointer to user's TPM config structure.
 */
void TPM_GetDefaultConfig(tpm_config_t *config);

/*! @}*/

/*!
 * @name Channel mode operations
 * @{
 */

/*!
 * @brief Configures the PWM signal parameters
 *
 * User calls this function to configure the PWM signals period, mode, dutycycle and edge. Use this
 * function to configure all the TPM channels that will be used to output a PWM signal
 *
 * @param base        TPM peripheral base address
 * @param chnlParams  Array of PWM channel parameters to configure the channel(s)
 * @param numOfChnls  Number of channels to configure, this should be the size of the array passed in
 * @param mode        PWM operation mode, options available in enumeration ::tpm_pwm_mode_t
 * @param pwmFreq_Hz  PWM signal frequency in Hz
 * @param srcClock_Hz TPM counter clock in Hz
 *
 * @return kStatus_Success if the PWM setup was successful,
 *         kStatus_Error on failure
 */
status_t TPM_SetupPwm(TPM_Type *base,
                      const tpm_chnl_pwm_signal_param_t *chnlParams,
                      uint8_t numOfChnls,
                      tpm_pwm_mode_t mode,
                      uint32_t pwmFreq_Hz,
                      uint32_t srcClock_Hz);

/*!
 * @brief Update the duty cycle of an active PWM signal
 *
 * @param base              TPM peripheral base address
 * @param chnlNumber        The channel number. In combined mode, this represents
 *                          the channel pair number
 * @param currentPwmMode    The current PWM mode set during PWM setup
 * @param dutyCyclePercent  New PWM pulse width, value should be between 0 to 100
 *                          0=inactive signal(0% duty cycle)...
 *                          100=active signal (100% duty cycle)
 */
void TPM_UpdatePwmDutycycle(TPM_Type *base,
                            tpm_chnl_t chnlNumber,
                            tpm_pwm_mode_t currentPwmMode,
                            uint8_t dutyCyclePercent);

/*!
 * @brief Update the edge level selection for a channel
 *
 * @param base       TPM peripheral base address
 * @param chnlNumber The channel number
 * @param level      The level to be set to the ELSnB:ELSnA field; valid values are 00, 01, 10, 11.
 *                   See the appropriate SoC reference manual for details about this field.
 */
void TPM_UpdateChnlEdgeLevelSelect(TPM_Type *base, tpm_chnl_t chnlNumber, uint8_t level);

/*!
 * @brief Enables capturing an input signal on the channel using the function parameters.
 *
 * When the edge specified in the captureMode argument occurs on the channel, the TPM counter is captured into
 * the CnV register. The user has to read the CnV register separately to get this value.
 *
 * @param base        TPM peripheral base address
 * @param chnlNumber  The channel number
 * @param captureMode Specifies which edge to capture
 */
void TPM_SetupInputCapture(TPM_Type *base, tpm_chnl_t chnlNumber, tpm_input_capture_edge_t captureMode);

/*!
 * @brief Configures the TPM to generate timed pulses.
 *
 * When the TPM counter matches the value of compareVal argument (this is written into CnV reg), the channel
 * output is changed based on what is specified in the compareMode argument.
 *
 * @param base         TPM peripheral base address
 * @param chnlNumber   The channel number
 * @param compareMode  Action to take on the channel output when the compare condition is met
 * @param compareValue Value to be programmed in the CnV register.
 */
void TPM_SetupOutputCompare(TPM_Type *base,
                            tpm_chnl_t chnlNumber,
                            tpm_output_compare_mode_t compareMode,
                            uint32_t compareValue);

#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
/*!
 * @brief Configures the dual edge capture mode of the TPM.
 *
 * This function allows to measure a pulse width of the signal on the input of channel of a
 * channel pair. The filter function is disabled if the filterVal argument passed is zero.
 *
 * @param base           TPM peripheral base address
 * @param chnlPairNumber The TPM channel pair number; options are 0, 1, 2, 3
 * @param edgeParam      Sets up the dual edge capture function
 * @param filterValue    Filter value, specify 0 to disable filter.
 */
void TPM_SetupDualEdgeCapture(TPM_Type *base,
                              tpm_chnl_t chnlPairNumber,
                              const tpm_dual_edge_capture_param_t *edgeParam,
                              uint32_t filterValue);
#endif

#if defined(FSL_FEATURE_TPM_HAS_QDCTRL) && FSL_FEATURE_TPM_HAS_QDCTRL
/*!
 * @brief Configures the parameters and activates the quadrature decode mode.
 *
 * @param base         TPM peripheral base address
 * @param phaseAParams Phase A configuration parameters
 * @param phaseBParams Phase B configuration parameters
 * @param quadMode     Selects encoding mode used in quadrature decoder mode
 */
void TPM_SetupQuadDecode(TPM_Type *base,
                         const tpm_phase_params_t *phaseAParams,
                         const tpm_phase_params_t *phaseBParams,
                         tpm_quad_decode_mode_t quadMode);
#endif

/*! @}*/

/*!
 * @name Interrupt Interface
 * @{
 */

/*!
 * @brief Enables the selected TPM interrupts.
 *
 * @param base TPM peripheral base address
 * @param mask The interrupts to enable. This is a logical OR of members of the
 *             enumeration ::tpm_interrupt_enable_t
 */
void TPM_EnableInterrupts(TPM_Type *base, uint32_t mask);

/*!
 * @brief Disables the selected TPM interrupts.
 *
 * @param base TPM peripheral base address
 * @param mask The interrupts to disable. This is a logical OR of members of the
 *             enumeration ::tpm_interrupt_enable_t
 */
void TPM_DisableInterrupts(TPM_Type *base, uint32_t mask);

/*!
 * @brief Gets the enabled TPM interrupts.
 *
 * @param base TPM peripheral base address
 *
 * @return The enabled interrupts. This is the logical OR of members of the
 *         enumeration ::tpm_interrupt_enable_t
 */
uint32_t TPM_GetEnabledInterrupts(TPM_Type *base);

/*! @}*/

/*!
 * @name Status Interface
 * @{
 */

/*!
 * @brief Gets the TPM status flags
 *
 * @param base TPM peripheral base address
 *
 * @return The status flags. This is the logical OR of members of the
 *         enumeration ::tpm_status_flags_t
 */
static inline uint32_t TPM_GetStatusFlags(TPM_Type *base)
{
    return base->STATUS;
}

/*!
 * @brief Clears the TPM status flags
 *
 * @param base TPM peripheral base address
 * @param mask The status flags to clear. This is a logical OR of members of the
 *             enumeration ::tpm_status_flags_t
 */
static inline void TPM_ClearStatusFlags(TPM_Type *base, uint32_t mask)
{
    /* Clear the status flags */
    base->STATUS = mask;
}

/*! @}*/

/*!
 * @name Read and write the timer period
 * @{
 */

/*!
 * @brief Sets the timer period in units of ticks.
 *
 * Timers counts from 0 until it equals the count value set here. The count value is written to
 * the MOD register.
 *
 * @note
 * 1. This API allows the user to use the TPM module as a timer. Do not mix usage
 *    of this API with TPM's PWM setup API's.
 * 2. Call the utility macros provided in the fsl_common.h to convert usec or msec to ticks.
 *
 * @param base TPM peripheral base address
 * @param ticks A timer period in units of ticks, which should be equal or greater than 1.
 */
static inline void TPM_SetTimerPeriod(TPM_Type *base, uint32_t ticks)
{
    base->MOD = ticks;
}

/*!
 * @brief Reads the current timer counting value.
 *
 * This function returns the real-time timer counting value in a range from 0 to a
 * timer period.
 *
 * @note Call the utility macros provided in the fsl_common.h to convert ticks to usec or msec.
 *
 * @param base TPM peripheral base address
 *
 * @return The current counter value in ticks
 */
static inline uint32_t TPM_GetCurrentTimerCount(TPM_Type *base)
{
    return (uint32_t)((base->CNT & TPM_CNT_COUNT_MASK) >> TPM_CNT_COUNT_SHIFT);
}

/*!
 * @name Timer Start and Stop
 * @{
 */

/*!
 * @brief Starts the TPM counter.
 *
 *
 * @param base        TPM peripheral base address
 * @param clockSource TPM clock source; once clock source is set the counter will start running
 */
static inline void TPM_StartTimer(TPM_Type *base, tpm_clock_source_t clockSource)
{
    uint32_t reg = base->SC;

    reg &= ~(TPM_SC_CMOD_MASK);
    reg |= TPM_SC_CMOD(clockSource);
    base->SC = reg;
}

/*!
 * @brief Stops the TPM counter.
 *
 * @param base TPM peripheral base address
 */
static inline void TPM_StopTimer(TPM_Type *base)
{
    /* Set clock source to none to disable counter */
    base->SC &= ~(TPM_SC_CMOD_MASK);

    /* Wait till this reads as zero acknowledging the counter is disabled */
    while (base->SC & TPM_SC_CMOD_MASK)
    {
    }
}

/*! @}*/

#if defined(FSL_FEATURE_TPM_HAS_GLOBAL) && FSL_FEATURE_TPM_HAS_GLOBAL
/*!
 * @brief Performs a software reset on the TPM module.
 *
 * Reset all internal logic and registers, except the Global Register. Remains set until cleared by software..
 *
 * @note TPM software reset is available on certain SoC's only
 *
 * @param base TPM peripheral base address
 */
static inline void TPM_Reset(TPM_Type *base)
{
    base->GLOBAL |= TPM_GLOBAL_RST_MASK;
    base->GLOBAL &= ~TPM_GLOBAL_RST_MASK;
}
#endif

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_TPM_H_ */
