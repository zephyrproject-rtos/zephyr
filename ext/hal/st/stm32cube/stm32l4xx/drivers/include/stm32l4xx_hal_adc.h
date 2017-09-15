/**
  ******************************************************************************
  * @file    stm32l4xx_hal_adc.h
  * @author  MCD Application Team
  * @brief   Header file of ADC HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L4xx_HAL_ADC_H
#define __STM32L4xx_HAL_ADC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal_def.h"

/* Include low level driver */
#include "stm32l4xx_ll_adc.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

/** @addtogroup ADC
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup ADC_Exported_Types ADC Exported Types
  * @{
  */

/** 
  * @brief  ADC group regular oversampling structure definition
  */
typedef struct
{
  uint32_t Ratio;                         /*!< Configures the oversampling ratio.
                                               This parameter can be a value of @ref ADC_Oversampling_Ratio */

  uint32_t RightBitShift;                 /*!< Configures the division coefficient for the Oversampler.
                                               This parameter can be a value of @ref ADC_Right_Bit_Shift */

  uint32_t TriggeredMode;                 /*!< Selects the regular triggered oversampling mode.
                                               This parameter can be a value of @ref ADC_Triggered_Oversampling_Mode */

  uint32_t OversamplingStopReset;         /*!< Selects the regular oversampling mode.
                                               The oversampling is either temporary stopped or reset upon an injected
                                               sequence interruption. 
                                               If oversampling is enabled on both regular and injected groups, this parameter 
                                               is discarded and forced to setting "ADC_REGOVERSAMPLING_RESUMED_MODE" 
                                               (the oversampling buffer is zeroed during injection sequence).   
                                               This parameter can be a value of @ref ADC_Regular_Oversampling_Mode */                                               

}ADC_OversamplingTypeDef;

/**
  * @brief  Structure definition of ADC instance and ADC group regular.
  * @note   Parameters of this structure are shared within 2 scopes:
  *          - Scope entire ADC (affects ADC groups regular and injected): ClockPrescaler, Resolution, DataAlign,
  *            ScanConvMode, EOCSelection, LowPowerAutoWait.
  *          - Scope ADC group regular: ContinuousConvMode, NbrOfConversion, DiscontinuousConvMode, NbrOfDiscConversion,
  *            ExternalTrigConv, ExternalTrigConvEdge, DMAContinuousRequests, Overrun, OversamplingMode, Oversampling.
  * @note   The setting of these parameters by function HAL_ADC_Init() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled
  *          - For all parameters except 'LowPowerAutoWait', 'DMAContinuousRequests' and 'Oversampling': ADC enabled without conversion on going on group regular.
  *          - For parameters 'LowPowerAutoWait' and 'DMAContinuousRequests': ADC enabled without conversion on going on groups regular and injected.
  *         If ADC is not in the appropriate state to modify some parameters, these parameters setting is bypassed
  *         without error reporting (as it can be the expected behavior in case of intended action to update another parameter 
  *         (which fulfills the ADC state condition) on the fly).
  */
typedef struct
{
  uint32_t ClockPrescaler;        /*!< Select ADC clock source (synchronous clock derived from APB clock or asynchronous clock derived from system clock or PLL (Refer to reference manual for list of clocks available)) and clock prescaler.
                                       This parameter can be a value of @ref ADC_ClockPrescaler.
                                       Note: The ADC clock configuration is common to all ADC instances.
                                       Note: In case of usage of channels on injected group, ADC frequency should be lower than AHB clock frequency /4 for resolution 12 or 10 bits, 
                                             AHB clock frequency /3 for resolution 8 bits, AHB clock frequency /2 for resolution 6 bits.
                                       Note: In case of synchronous clock mode based on HCLK/1, the configuration must be enabled only
                                             if the system clock has a 50% duty clock cycle (APB prescaler configured inside RCC 
                                             must be bypassed and PCLK clock must have 50% duty cycle). Refer to reference manual for details.
                                       Note: In case of usage of asynchronous clock, the selected clock must be preliminarily enabled at RCC top level.
                                       Note: This parameter can be modified only if all ADC instances are disabled. */

  uint32_t Resolution;            /*!< Configure the ADC resolution. 
                                       This parameter can be a value of @ref ADC_Resolution */

  uint32_t DataAlign;             /*!< Specify ADC data alignment in conversion data register (right or left).
                                       Refer to reference manual for alignments formats versus resolutions.
                                       This parameter can be a value of @ref ADC_Data_align */

  uint32_t ScanConvMode;          /*!< Configure the sequencer of ADC groups regular and injected.
                                       This parameter can be associated to parameter 'DiscontinuousConvMode' to have main sequence subdivided in successive parts.
                                       If disabled: Conversion is performed in single mode (one channel converted, the one defined in rank 1).
                                                    Parameters 'NbrOfConversion' and 'InjectedNbrOfConversion' are discarded (equivalent to set to 1).
                                       If enabled:  Conversions are performed in sequence mode (multiple ranks defined by 'NbrOfConversion' or 'InjectedNbrOfConversion' and rank of each channel in sequencer).
                                                    Scan direction is upward: from rank 1 to rank 'n'.
                                       This parameter can be a value of @ref ADC_Scan_mode */

  uint32_t EOCSelection;          /*!< Specify which EOC (End Of Conversion) flag is used for conversion by polling and interruption: end of unitary conversion or end of sequence conversions.
                                       This parameter can be a value of @ref ADC_EOCSelection. */

  uint32_t LowPowerAutoWait;      /*!< Select the dynamic low power Auto Delay: new conversion start only when the previous
                                       conversion (for ADC group regular) or previous sequence (for ADC group injected) has been retrieved by user software,
                                       using function HAL_ADC_GetValue() or HAL_ADCEx_InjectedGetValue().
                                       This feature automatically adapts the frequency of ADC conversions triggers to the speed of the system that reads the data. Moreover, this avoids risk of overrun
                                       for low frequency applications. 
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: Do not use with interruption or DMA (HAL_ADC_Start_IT(), HAL_ADC_Start_DMA()) since they clear immediately the EOC flag
                                             to free the IRQ vector sequencer.
                                             Do use with polling: 1. Start conversion with HAL_ADC_Start(), 2. Later on, when ADC conversion data is needed:
                                             use HAL_ADC_PollForConversion() to ensure that conversion is completed and HAL_ADC_GetValue() to retrieve conversion result and trig another conversion start.
                                             (in case of usage of ADC group injected, use the equivalent functions HAL_ADCExInjected_Start(), HAL_ADCEx_InjectedGetValue(), ...). */

  uint32_t ContinuousConvMode;    /*!< Specify whether the conversion is performed in single mode (one conversion) or continuous mode for ADC group regular,
                                       after the first ADC conversion start trigger occurred (software start or external trigger).
                                       This parameter can be set to ENABLE or DISABLE. */

  uint32_t NbrOfConversion;       /*!< Specify the number of ranks that will be converted within the regular group sequencer.
                                       To use the regular group sequencer and convert several ranks, parameter 'ScanConvMode' must be enabled.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 16.
                                       Note: This parameter must be modified when no conversion is on going on regular group (ADC disabled, or ADC enabled without 
                                       continuous mode or external trigger that could launch a conversion). */

  uint32_t DiscontinuousConvMode; /*!< Specify whether the conversions sequence of ADC group regular is performed in Complete-sequence/Discontinuous-sequence
                                       (main sequence subdivided in successive parts).
                                       Discontinuous mode is used only if sequencer is enabled (parameter 'ScanConvMode'). If sequencer is disabled, this parameter is discarded.
                                       Discontinuous mode can be enabled only if continuous mode is disabled. If continuous mode is enabled, this parameter setting is discarded.
                                       This parameter can be set to ENABLE or DISABLE. */

  uint32_t NbrOfDiscConversion;   /*!< Specifies the number of discontinuous conversions in which the main sequence of ADC group regular (parameter NbrOfConversion) will be subdivided.
                                       If parameter 'DiscontinuousConvMode' is disabled, this parameter is discarded.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 8. */

  uint32_t ExternalTrigConv;      /*!< Select the external event source used to trigger ADC group regular conversion start.
                                       If set to ADC_SOFTWARE_START, external triggers are disabled and software trigger is used instead.
                                       This parameter can be a value of @ref ADC_regular_external_trigger_source.
                                       Caution: external trigger source is common to all ADC instances. */
                                                                                                        
  uint32_t ExternalTrigConvEdge;  /*!< Select the external event edge used to trigger ADC group regular conversion start.
                                       If trigger source is set to ADC_SOFTWARE_START, this parameter is discarded.
                                       This parameter can be a value of @ref ADC_regular_external_trigger_edge */

  uint32_t DMAContinuousRequests; /*!< Specify whether the DMA requests are performed in one shot mode (DMA transfer stops when number of conversions is reached)
                                       or in continuous mode (DMA transfer unlimited, whatever number of conversions).
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: In continuous mode, DMA must be configured in circular mode. Otherwise an overrun will be triggered when DMA buffer maximum pointer is reached. */

  uint32_t Overrun;               /*!< Select the behavior in case of overrun: data overwritten or preserved (default).
                                       This parameter applies to ADC group regular only.
                                       This parameter can be a value of @ref ADC_Overrun.
                                       Note: In case of overrun set to data preserved and usage with programming model with interruption (HAL_Start_IT()): ADC IRQ handler has to clear 
                                       end of conversion flags, this induces the release of the preserved data. If needed, this data can be saved in function 
                                       HAL_ADC_ConvCpltCallback(), placed in user program code (called before end of conversion flags clear).
                                       Note: Error reporting with respect to the conversion mode:
                                             - Usage with ADC conversion by polling for event or interruption: Error is reported only if overrun is set to data preserved. If overrun is set to data 
                                               overwritten, user can willingly not read all the converted data, this is not considered as an erroneous case.
                                             - Usage with ADC conversion by DMA: Error is reported whatever overrun setting (DMA is expected to process all data from data register). */

  uint32_t OversamplingMode;              /*!< Specify whether the oversampling feature is enabled or disabled.
                                               This parameter can be set to ENABLE or DISABLE.
                                               Note: This parameter can be modified only if there is no conversion is ongoing on ADC groups regular and injected */

  ADC_OversamplingTypeDef Oversampling;   /*!< Specify the Oversampling parameters.
                                               Caution: this setting overwrites the previous oversampling configuration if oversampling is already enabled. */

#if defined(ADC_CFGR_DFSDMCFG) &&defined(DFSDM1_Channel0)
  uint32_t DFSDMConfig;           /*!< Specify whether ADC conversion data is sent directly to DFSDM.
                                       This parameter can be a value of @ref ADCEx_DFSDM_Mode_Configuration.
                                       Note: This parameter can be modified only if there is no conversion is ongoing (both ADSTART and JADSTART cleared). */

#endif
}ADC_InitTypeDef;

/**
  * @brief  Structure definition of ADC channel for regular group
  * @note   The setting of these parameters by function HAL_ADC_ConfigChannel() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled (this is the only possible ADC state to modify parameter 'SingleDiff')
  *          - For all except parameters 'SamplingTime', 'Offset', 'OffsetNumber': ADC enabled without conversion on going on regular group.
  *          - For parameters 'SamplingTime', 'Offset', 'OffsetNumber': ADC enabled without conversion on going on regular and injected groups.
  *         If ADC is not in the appropriate state to modify some parameters, these parameters setting is bypassed
  *         without error reporting (as it can be the expected behavior in case of intended action to update another parameter (which fulfills the ADC state condition)
  *         on the fly).
  */
typedef struct
{
  uint32_t Channel;                /*!< Specify the channel to configure into ADC regular group.
                                        This parameter can be a value of @ref ADC_channels
                                        Note: Depending on devices and ADC instances, some channels may not be available on device package pins. Refer to device datasheet for channels availability. */

  uint32_t Rank;                   /*!< Specify the rank in the regular group sequencer.
                                        This parameter can be a value of @ref ADC_regular_rank
                                        Note: to disable a channel or change order of conversion sequencer, rank containing a previous channel setting can be overwritten by 
                                        the new channel setting (or parameter number of conversions adjusted) */

  uint32_t SamplingTime;           /*!< Sampling time value to be set for the selected channel.
                                        Unit: ADC clock cycles
                                        Conversion time is the addition of sampling time and processing time
                                        (12.5 ADC clock cycles at ADC resolution 12 bits, 10.5 cycles at 10 bits, 8.5 cycles at 8 bits, 6.5 cycles at 6 bits).
                                        This parameter can be a value of @ref ADC_HAL_EC_CHANNEL_SAMPLINGTIME
                                        Caution: This parameter applies to a channel that can be used into regular and/or injected group.
                                                 It overwrites the last setting.
                                        Note: In case of usage of internal measurement channels (VrefInt/Vbat/TempSensor),
                                              sampling time constraints must be respected (sampling time can be adjusted in function of ADC clock frequency and sampling time setting)
                                              Refer to device datasheet for timings values. */

  uint32_t SingleDiff;             /*!< Select single-ended or differential input.
                                        In differential mode: Differential measurement is carried out between the selected channel 'i' (positive input) and channel 'i+1' (negative input).
                                                              Only channel 'i' has to be configured, channel 'i+1' is configured automatically.
                                        This parameter must be a value of @ref ADCEx_SingleDifferential
                                        Caution: This parameter applies to a channel that can be used in a regular and/or injected group.
                                                 It overwrites the last setting.
                                        Note: Refer to Reference Manual to ensure the selected channel is available in differential mode.
                                        Note: When configuring a channel 'i' in differential mode, the channel 'i+1' is not usable separately.
                                        Note: This parameter must be modified when ADC is disabled (before ADC start conversion or after ADC stop conversion).
                                              If ADC is enabled, this parameter setting is bypassed without error reporting (as it can be the expected behavior in case 
                                        of another parameter update on the fly) */

  uint32_t OffsetNumber;           /*!< Select the offset number
                                        This parameter can be a value of @ref ADCEx_OffsetNumber
                                        Caution: Only one offset is allowed per channel. This parameter overwrites the last setting. */

  uint32_t Offset;                 /*!< Define the offset to be subtracted from the raw converted data.
                                        Offset value must be a positive number.
                                        Depending of ADC resolution selected (12, 10, 8 or 6 bits), this parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF, 
                                        0x3FF, 0xFF or 0x3F respectively.
                                        Note: This parameter must be modified when no conversion is on going on both regular and injected groups (ADC disabled, or ADC enabled 
                                              without continuous mode or external trigger that could launch a conversion). */

}ADC_ChannelConfTypeDef;

/**
  * @brief  Structure definition of ADC analog watchdog
  * @note   The setting of these parameters by function HAL_ADC_AnalogWDGConfig() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled or ADC enabled without conversion on going on ADC groups regular and injected.
  */
typedef struct
{
  uint32_t WatchdogNumber;    /*!< Select which ADC analog watchdog is monitoring the selected channel.
                                   For Analog Watchdog 1: Only 1 channel can be monitored (or overall group of channels by setting parameter 'WatchdogMode')
                                   For Analog Watchdog 2 and 3: Several channels can be monitored (by successive calls of 'HAL_ADC_AnalogWDGConfig()' for each channel)
                                   This parameter can be a value of @ref ADC_analog_watchdog_number. */

  uint32_t WatchdogMode;      /*!< Configure the ADC analog watchdog mode: single/all/none channels.
                                   For Analog Watchdog 1: Configure the ADC analog watchdog mode: single channel or all channels, ADC groups regular and-or injected.
                                   
                                   For Analog Watchdog 2 and 3: There is no configuration for all channels as AWD1. Set value 'ADC_ANALOGWATCHDOG_NONE' to reset 
                                   channels group programmed with parameter 'Channel', set any other value to program the channel(s) to be monitored.
                                   This parameter can be a value of @ref ADC_analog_watchdog_mode. */

  uint32_t Channel;           /*!< Select which ADC channel to monitor by analog watchdog.
                                   For Analog Watchdog 1: this parameter has an effect only if parameter 'WatchdogMode' is configured on single channel (only 1 channel can be monitored).
                                   For Analog Watchdog 2 and 3: Several channels can be monitored. To use this feature, call successively the function HAL_ADC_AnalogWDGConfig() for each channel to be added (or removed with value 'ADC_ANALOGWATCHDOG_NONE').
                                   This parameter can be a value of @ref ADC_channels. */

  uint32_t ITMode;            /*!< Specify whether the analog watchdog is configured in interrupt or polling mode.
                                   This parameter can be set to ENABLE or DISABLE */

  uint32_t HighThreshold;     /*!< Configure the ADC analog watchdog High threshold value.
                                   Depending of ADC resolution selected (12, 10, 8 or 6 bits), this parameter must be a number
                                   between Min_Data = 0x000 and Max_Data = 0xFFF, 0x3FF, 0xFF or 0x3F respectively.
                                   Note: Analog watchdog 2 and 3 are limited to a resolution of 8 bits: if ADC resolution is 12 bits 
                                         the 4 LSB are ignored, if ADC resolution is 10 bits the 2 LSB are ignored. */

  uint32_t LowThreshold;      /*!< Configures the ADC analog watchdog Low threshold value.
                                   Depending of ADC resolution selected (12, 10, 8 or 6 bits), this parameter must be a number
                                   between Min_Data = 0x000 and Max_Data = 0xFFF, 0x3FF, 0xFF or 0x3F respectively.
                                   Note: Analog watchdog 2 and 3 are limited to a resolution of 8 bits: if ADC resolution is 12 bits 
                                         the 4 LSB are ignored, if ADC resolution is 10 bits the 2 LSB are ignored. */
}ADC_AnalogWDGConfTypeDef;

/**
  * @brief  ADC group injected contexts queue configuration
  * @note   Structure intended to be used only through structure "ADC_HandleTypeDef"
  */
typedef struct
{
  uint32_t ContextQueue;                 /*!< Injected channel configuration context: build-up over each 
                                              HAL_ADCEx_InjectedConfigChannel() call to finally initialize
                                              JSQR register at HAL_ADCEx_InjectedConfigChannel() last call */
                                               
  uint32_t ChannelCount;                 /*!< Number of channels in the injected sequence */
}ADC_InjectionConfigTypeDef;  

/** @defgroup ADC_States ADC States
  * @{
  */

/**
  * @brief  HAL ADC state machine: ADC states definition (bitfields)
  * @note   ADC state machine is managed by bitfields, state must be compared
  *         with bit by bit.
  *         For example:                                                         
  *           " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_REG_BUSY)) "
  *           " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_AWD1)    ) "
  */
/* States of ADC global scope */
#define HAL_ADC_STATE_RESET             (0x00000000U)    /*!< ADC not yet initialized or disabled */
#define HAL_ADC_STATE_READY             (0x00000001U)    /*!< ADC peripheral ready for use */
#define HAL_ADC_STATE_BUSY_INTERNAL     (0x00000002U)    /*!< ADC is busy due to an internal process (initialization, calibration) */
#define HAL_ADC_STATE_TIMEOUT           (0x00000004U)    /*!< TimeOut occurrence */

/* States of ADC errors */
#define HAL_ADC_STATE_ERROR_INTERNAL    (0x00000010U)    /*!< Internal error occurrence */
#define HAL_ADC_STATE_ERROR_CONFIG      (0x00000020U)    /*!< Configuration error occurrence */
#define HAL_ADC_STATE_ERROR_DMA         (0x00000040U)    /*!< DMA error occurrence */

/* States of ADC group regular */
#define HAL_ADC_STATE_REG_BUSY          (0x00000100U)    /*!< A conversion on ADC group regular is ongoing or can occur (either by continuous mode,
                                                              external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available)) */
#define HAL_ADC_STATE_REG_EOC           (0x00000200U)    /*!< Conversion data available on group regular */
#define HAL_ADC_STATE_REG_OVR           (0x00000400U)    /*!< Overrun occurrence */
#define HAL_ADC_STATE_REG_EOSMP         (0x00000800U)    /*!< Not available on this STM32 serie: End Of Sampling flag raised  */

/* States of ADC group injected */
#define HAL_ADC_STATE_INJ_BUSY          (0x00001000U)    /*!< A conversion on ADC group injected is ongoing or can occur (either by auto-injection mode,
                                                              external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available)) */
#define HAL_ADC_STATE_INJ_EOC           (0x00002000U)    /*!< Conversion data available on group injected */
#define HAL_ADC_STATE_INJ_JQOVF         (0x00004000U)    /*!< Injected queue overflow occurrence */

/* States of ADC analog watchdogs */
#define HAL_ADC_STATE_AWD1              (0x00010000U)    /*!< Out-of-window occurrence of ADC analog watchdog 1 */
#define HAL_ADC_STATE_AWD2              (0x00020000U)    /*!< Out-of-window occurrence of ADC analog watchdog 2 */
#define HAL_ADC_STATE_AWD3              (0x00040000U)    /*!< Out-of-window occurrence of ADC analog watchdog 3 */

/* States of ADC multi-mode */
#define HAL_ADC_STATE_MULTIMODE_SLAVE   (0x00100000U)    /*!< ADC in multimode slave state, controlled by another ADC master (when feature available) */

/**
  * @}
  */

/** 
  * @brief  ADC handle Structure definition
  */
typedef struct
{
  ADC_TypeDef                   *Instance;              /*!< Register base address */

  ADC_InitTypeDef               Init;                   /*!< ADC initialization parameters and regular conversions setting */

  DMA_HandleTypeDef             *DMA_Handle;            /*!< Pointer DMA Handler */

  HAL_LockTypeDef               Lock;                   /*!< ADC locking object */

  __IO uint32_t                 State;                  /*!< ADC communication state (bitmap of ADC states) */

  __IO uint32_t                 ErrorCode;              /*!< ADC Error code */

  ADC_InjectionConfigTypeDef    InjectionConfig ;       /*!< ADC injected channel configuration build-up structure */
}ADC_HandleTypeDef;

/**
  * @}
  */


/* Exported constants --------------------------------------------------------*/

/** @defgroup ADC_Exported_Constants ADC Exported Constants
  * @{
  */

/** @defgroup ADC_Error_Code ADC Error Code
  * @{
  */
#define HAL_ADC_ERROR_NONE        (0x00U)   /*!< No error                                    */
#define HAL_ADC_ERROR_INTERNAL    (0x01U)   /*!< ADC IP internal error (problem of clocking,
                                                 enable/disable, erroneous state, ...)       */
#define HAL_ADC_ERROR_OVR         (0x02U)   /*!< Overrun error                               */
#define HAL_ADC_ERROR_DMA         (0x04U)   /*!< DMA transfer error                          */
#define HAL_ADC_ERROR_JQOVF       (0x08U)   /*!< Injected context queue overflow error       */
/**
  * @}
  */

/** @defgroup ADC_ClockPrescaler ADC clock source and clock prescaler
  * @{
  */
#define ADC_CLOCK_SYNC_PCLK_DIV1      ((uint32_t)ADC_CCR_CKMODE_0) /*!< ADC synchronous clock derived from AHB clock not divided  */
#define ADC_CLOCK_SYNC_PCLK_DIV2      ((uint32_t)ADC_CCR_CKMODE_1) /*!< ADC synchronous clock derived from AHB clock divided a prescaler of 2 */
#define ADC_CLOCK_SYNC_PCLK_DIV4      ((uint32_t)ADC_CCR_CKMODE)   /*!< ADC synchronous clock derived from AHB clock divided a prescaler of 4 */

#define ADC_CLOCKPRESCALER_PCLK_DIV1   ADC_CLOCK_SYNC_PCLK_DIV1    /*!< Obsolete naming, kept for compatibility with some other devices */
#define ADC_CLOCKPRESCALER_PCLK_DIV2   ADC_CLOCK_SYNC_PCLK_DIV2    /*!< Obsolete naming, kept for compatibility with some other devices */
#define ADC_CLOCKPRESCALER_PCLK_DIV4   ADC_CLOCK_SYNC_PCLK_DIV4    /*!< Obsolete naming, kept for compatibility with some other devices */

#define ADC_CLOCK_ASYNC_DIV1       ((uint32_t)0x00000000)                                        /*!< ADC asynchronous clock not divided    */
#define ADC_CLOCK_ASYNC_DIV2       ((uint32_t)ADC_CCR_PRESC_0)                                   /*!< ADC asynchronous clock divided by 2   */
#define ADC_CLOCK_ASYNC_DIV4       ((uint32_t)ADC_CCR_PRESC_1)                                   /*!< ADC asynchronous clock divided by 4   */
#define ADC_CLOCK_ASYNC_DIV6       ((uint32_t)(ADC_CCR_PRESC_1|ADC_CCR_PRESC_0))                 /*!< ADC asynchronous clock divided by 6   */
#define ADC_CLOCK_ASYNC_DIV8       ((uint32_t)(ADC_CCR_PRESC_2))                                 /*!< ADC asynchronous clock divided by 8   */
#define ADC_CLOCK_ASYNC_DIV10      ((uint32_t)(ADC_CCR_PRESC_2|ADC_CCR_PRESC_0))                 /*!< ADC asynchronous clock divided by 10  */
#define ADC_CLOCK_ASYNC_DIV12      ((uint32_t)(ADC_CCR_PRESC_2|ADC_CCR_PRESC_1))                 /*!< ADC asynchronous clock divided by 12  */
#define ADC_CLOCK_ASYNC_DIV16      ((uint32_t)(ADC_CCR_PRESC_2|ADC_CCR_PRESC_1|ADC_CCR_PRESC_0)) /*!< ADC asynchronous clock divided by 16  */
#define ADC_CLOCK_ASYNC_DIV32      ((uint32_t)(ADC_CCR_PRESC_3))                                 /*!< ADC asynchronous clock divided by 32  */
#define ADC_CLOCK_ASYNC_DIV64      ((uint32_t)(ADC_CCR_PRESC_3|ADC_CCR_PRESC_0))                 /*!< ADC asynchronous clock divided by 64  */
#define ADC_CLOCK_ASYNC_DIV128     ((uint32_t)(ADC_CCR_PRESC_3|ADC_CCR_PRESC_1))                 /*!< ADC asynchronous clock divided by 128 */
#define ADC_CLOCK_ASYNC_DIV256     ((uint32_t)(ADC_CCR_PRESC_3|ADC_CCR_PRESC_1|ADC_CCR_PRESC_0)) /*!< ADC asynchronous clock divided by 256 */
/**
  * @}
  */

/** @defgroup ADC_Resolution ADC Resolution
  * @{
  */
#define ADC_RESOLUTION_12B      ((uint32_t)0x00000000)          /*!< ADC 12-bit resolution */
#define ADC_RESOLUTION_10B      ((uint32_t)ADC_CFGR_RES_0)      /*!< ADC 10-bit resolution */
#define ADC_RESOLUTION_8B       ((uint32_t)ADC_CFGR_RES_1)      /*!< ADC 8-bit resolution */
#define ADC_RESOLUTION_6B       ((uint32_t)ADC_CFGR_RES)        /*!< ADC 6-bit resolution */
/**
  * @}
  */

/** @defgroup ADC_Data_align ADC conversion data alignment
  * @{
  */
#define ADC_DATAALIGN_RIGHT      ((uint32_t)0x00000000)         /*!< Data right alignment */
#define ADC_DATAALIGN_LEFT       ((uint32_t)ADC_CFGR_ALIGN)     /*!< Data left alignment  */
/**
  * @}
  */

/** @defgroup ADC_Scan_mode ADC sequencer scan mode
  * @{
  */
#define ADC_SCAN_DISABLE         ((uint32_t)0x00000000)        /*!< Scan mode disabled */
#define ADC_SCAN_ENABLE          ((uint32_t)0x00000001)        /*!< Scan mode enabled  */
/**
  * @}
  */

/** @defgroup ADC_regular_external_trigger_source ADC group regular trigger source
  * @{
  */
/* ADC group regular trigger sources for all ADC instances */
#define ADC_EXTERNALTRIG_T1_CC1           ((uint32_t)0x00000000)                                                                      /*!< Event 0 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T1_CC2           ((uint32_t)ADC_CFGR_EXTSEL_0)                                                               /*!< Event 1 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T1_CC3           ((uint32_t)ADC_CFGR_EXTSEL_1)                                                               /*!< Event 2 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T2_CC2           ((uint32_t)(ADC_CFGR_EXTSEL_1 | ADC_CFGR_EXTSEL_0))                                         /*!< Event 3 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T3_TRGO          ((uint32_t)ADC_CFGR_EXTSEL_2)                                                               /*!< Event 4 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T4_CC4           ((uint32_t)(ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_0))                                         /*!< Event 5 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_EXT_IT11         ((uint32_t)(ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_1))                                         /*!< Event 6 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T8_TRGO          ((uint32_t)(ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_1 | ADC_CFGR_EXTSEL_0))                     /*!< Event 7 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T8_TRGO2         ((uint32_t) ADC_CFGR_EXTSEL_3)                                                              /*!< Event 8 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T1_TRGO          ((uint32_t)(ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_0))                                         /*!< Event 9 triggers regular group conversion start  */
#define ADC_EXTERNALTRIG_T1_TRGO2         ((uint32_t)(ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_1))                                         /*!< Event 10 triggers regular group conversion start */
#define ADC_EXTERNALTRIG_T2_TRGO          ((uint32_t)(ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_1 | ADC_CFGR_EXTSEL_0))                     /*!< Event 11 triggers regular group conversion start */
#define ADC_EXTERNALTRIG_T4_TRGO          ((uint32_t)(ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_2))                                         /*!< Event 12 triggers regular group conversion start */
#define ADC_EXTERNALTRIG_T6_TRGO          ((uint32_t)(ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_0))                     /*!< Event 13 triggers regular group conversion start */
#define ADC_EXTERNALTRIG_T15_TRGO         ((uint32_t)(ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_1))                     /*!< Event 14 triggers regular group conversion start */
#define ADC_EXTERNALTRIG_T3_CC4           ((uint32_t)(ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_1 | ADC_CFGR_EXTSEL_0)) /*!< Event 15 triggers regular group conversion start */
#define ADC_SOFTWARE_START                ((uint32_t)0x00000001)                                                  /*!< Software triggers regular group conversion start */
/**
  * @}
  */

/** @defgroup ADC_regular_external_trigger_edge ADC group regular trigger edge (when external trigger is selected)
  * @{
  */
#define ADC_EXTERNALTRIGCONVEDGE_NONE           ((uint32_t)0x00000000)        /*!< Regular conversions hardware trigger detection disabled */
#define ADC_EXTERNALTRIGCONVEDGE_RISING         ((uint32_t)ADC_CFGR_EXTEN_0)  /*!< Regular conversions hardware trigger detection on the rising edge */
#define ADC_EXTERNALTRIGCONVEDGE_FALLING        ((uint32_t)ADC_CFGR_EXTEN_1)  /*!< Regular conversions hardware trigger detection on the falling edge */
#define ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING  ((uint32_t)ADC_CFGR_EXTEN)    /*!< Regular conversions hardware trigger detection on both the rising and falling edges */
/**
  * @}
  */

/** @defgroup ADC_EOCSelection ADC sequencer end of unitary conversion or sequence conversions
  * @{
  */
#define ADC_EOC_SINGLE_CONV         ((uint32_t) ADC_ISR_EOC)                 /*!< End of unitary conversion flag  */
#define ADC_EOC_SEQ_CONV            ((uint32_t) ADC_ISR_EOS)                 /*!< End of sequence conversions flag    */
/**
  * @}
  */

/** @defgroup ADC_Overrun ADC overrun
  * @{
  */
#define ADC_OVR_DATA_PRESERVED             ((uint32_t)0x00000000)           /*!< Data preserved in case of overrun   */
#define ADC_OVR_DATA_OVERWRITTEN           ((uint32_t)ADC_CFGR_OVRMOD)      /*!< Data overwritten in case of overrun */
/**
  * @}
  */

/** @defgroup ADC_regular_rank ADC group regular sequencer rank
  * @{
  */
#define ADC_REGULAR_RANK_1    ((uint32_t)0x00000001)       /*!< ADC regular conversion rank 1  */
#define ADC_REGULAR_RANK_2    ((uint32_t)0x00000002)       /*!< ADC regular conversion rank 2  */
#define ADC_REGULAR_RANK_3    ((uint32_t)0x00000003)       /*!< ADC regular conversion rank 3  */
#define ADC_REGULAR_RANK_4    ((uint32_t)0x00000004)       /*!< ADC regular conversion rank 4  */
#define ADC_REGULAR_RANK_5    ((uint32_t)0x00000005)       /*!< ADC regular conversion rank 5  */
#define ADC_REGULAR_RANK_6    ((uint32_t)0x00000006)       /*!< ADC regular conversion rank 6  */
#define ADC_REGULAR_RANK_7    ((uint32_t)0x00000007)       /*!< ADC regular conversion rank 7  */
#define ADC_REGULAR_RANK_8    ((uint32_t)0x00000008)       /*!< ADC regular conversion rank 8  */
#define ADC_REGULAR_RANK_9    ((uint32_t)0x00000009)       /*!< ADC regular conversion rank 9  */
#define ADC_REGULAR_RANK_10   ((uint32_t)0x0000000A)       /*!< ADC regular conversion rank 10 */
#define ADC_REGULAR_RANK_11   ((uint32_t)0x0000000B)       /*!< ADC regular conversion rank 11 */
#define ADC_REGULAR_RANK_12   ((uint32_t)0x0000000C)       /*!< ADC regular conversion rank 12 */
#define ADC_REGULAR_RANK_13   ((uint32_t)0x0000000D)       /*!< ADC regular conversion rank 13 */
#define ADC_REGULAR_RANK_14   ((uint32_t)0x0000000E)       /*!< ADC regular conversion rank 14 */
#define ADC_REGULAR_RANK_15   ((uint32_t)0x0000000F)       /*!< ADC regular conversion rank 15 */
#define ADC_REGULAR_RANK_16   ((uint32_t)0x00000010)       /*!< ADC regular conversion rank 16 */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_CHANNEL_SAMPLINGTIME  Channel - Sampling time
  * @{
  */
#define ADC_SAMPLETIME_2CYCLES_5        (0x00000000U)                                  /*!< Sampling time 2.5 ADC clock cycles */
#if defined(ADC_SMPR1_SMPPLUS)
#define ADC_SAMPLETIME_3CYCLES_5        (ADC_SMPR1_SMPPLUS)                       /*!< Sampling time 3.5 ADC clock cycles. If selected, this sampling time replaces all sampling time 2.5 ADC clock cycles. These 2 sampling times cannot be used simultaneously. */
#endif
#define ADC_SAMPLETIME_6CYCLES_5        (ADC_SMPR2_SMP10_0)                       /*!< Sampling time 6.5 ADC clock cycles   */
#define ADC_SAMPLETIME_12CYCLES_5       (ADC_SMPR2_SMP10_1)                       /*!< Sampling time 12.5 ADC clock cycles  */
#define ADC_SAMPLETIME_24CYCLES_5       ((ADC_SMPR2_SMP10_1 | ADC_SMPR2_SMP10_0)) /*!< Sampling time 24.5 ADC clock cycles  */
#define ADC_SAMPLETIME_47CYCLES_5       (ADC_SMPR2_SMP10_2)                       /*!< Sampling time 47.5 ADC clock cycles  */
#define ADC_SAMPLETIME_92CYCLES_5       ((ADC_SMPR2_SMP10_2 | ADC_SMPR2_SMP10_0)) /*!< Sampling time 92.5 ADC clock cycles  */
#define ADC_SAMPLETIME_247CYCLES_5      ((ADC_SMPR2_SMP10_2 | ADC_SMPR2_SMP10_1)) /*!< Sampling time 247.5 ADC clock cycles */
#define ADC_SAMPLETIME_640CYCLES_5      (ADC_SMPR2_SMP10)                         /*!< Sampling time 640.5 ADC clock cycles */
/**
  * @}
  */

/** @defgroup ADC_channels ADC channels
  * @{
  */
#define ADC_CHANNEL_0           ((uint32_t)(0x00000000))                                                            /*!< ADC channel 0  */
#define ADC_CHANNEL_1           ((uint32_t)(ADC_SQR3_SQ10_0))                                                       /*!< ADC channel 1  */
#define ADC_CHANNEL_2           ((uint32_t)(ADC_SQR3_SQ10_1))                                                       /*!< ADC channel 2  */
#define ADC_CHANNEL_3           ((uint32_t)(ADC_SQR3_SQ10_1 | ADC_SQR3_SQ10_0))                                     /*!< ADC channel 3  */
#define ADC_CHANNEL_4           ((uint32_t)(ADC_SQR3_SQ10_2))                                                       /*!< ADC channel 4  */
#define ADC_CHANNEL_5           ((uint32_t)(ADC_SQR3_SQ10_2 | ADC_SQR3_SQ10_0))                                     /*!< ADC channel 5  */
#define ADC_CHANNEL_6           ((uint32_t)(ADC_SQR3_SQ10_2 | ADC_SQR3_SQ10_1))                                     /*!< ADC channel 6  */
#define ADC_CHANNEL_7           ((uint32_t)(ADC_SQR3_SQ10_2 | ADC_SQR3_SQ10_1 | ADC_SQR3_SQ10_0))                   /*!< ADC channel 7  */
#define ADC_CHANNEL_8           ((uint32_t)(ADC_SQR3_SQ10_3))                                                       /*!< ADC channel 8  */
#define ADC_CHANNEL_9           ((uint32_t)(ADC_SQR3_SQ10_3 | ADC_SQR3_SQ10_0))                                     /*!< ADC channel 9  */
#define ADC_CHANNEL_10          ((uint32_t)(ADC_SQR3_SQ10_3 | ADC_SQR3_SQ10_1))                                     /*!< ADC channel 10 */
#define ADC_CHANNEL_11          ((uint32_t)(ADC_SQR3_SQ10_3 | ADC_SQR3_SQ10_1 | ADC_SQR3_SQ10_0))                   /*!< ADC channel 11 */
#define ADC_CHANNEL_12          ((uint32_t)(ADC_SQR3_SQ10_3 | ADC_SQR3_SQ10_2))                                     /*!< ADC channel 12 */
#define ADC_CHANNEL_13          ((uint32_t)(ADC_SQR3_SQ10_3 | ADC_SQR3_SQ10_2 | ADC_SQR3_SQ10_0))                   /*!< ADC channel 13 */
#define ADC_CHANNEL_14          ((uint32_t)(ADC_SQR3_SQ10_3 | ADC_SQR3_SQ10_2 | ADC_SQR3_SQ10_1))                   /*!< ADC channel 14 */
#define ADC_CHANNEL_15          ((uint32_t)(ADC_SQR3_SQ10_3 | ADC_SQR3_SQ10_2 | ADC_SQR3_SQ10_1 | ADC_SQR3_SQ10_0)) /*!< ADC channel 15 */
#define ADC_CHANNEL_16          ((uint32_t)(ADC_SQR3_SQ10_4))                                                       /*!< ADC channel 16 */
#define ADC_CHANNEL_17          ((uint32_t)(ADC_SQR3_SQ10_4 | ADC_SQR3_SQ10_0))                                     /*!< ADC channel 17 */
#define ADC_CHANNEL_18          ((uint32_t)(ADC_SQR3_SQ10_4 | ADC_SQR3_SQ10_1))                                     /*!< ADC channel 18 */

/* Note: VrefInt, TempSensor and Vbat internal channels are not available on  */
/*        all ADC instances (refer to Reference Manual).                      */
#define ADC_CHANNEL_TEMPSENSOR  ADC_CHANNEL_17                                                                      /*!< ADC temperature sensor channel */
#define ADC_CHANNEL_VBAT        ADC_CHANNEL_18                                                                      /*!< ADC Vbat channel               */
#define ADC_CHANNEL_VREFINT     ADC_CHANNEL_0                                                                       /*!< ADC Vrefint channel            */

#if defined(ADC1) && !defined(ADC2)
#define ADC_CHANNEL_DAC1CH1             (ADC_CHANNEL_17) /*!< ADC internal channel connected to DAC1 channel 1, channel specific to ADC1. This channel is shared with ADC internal channel connected to temperature sensor, they cannot be used both simultenaeously. */
#define ADC_CHANNEL_DAC1CH2             (ADC_CHANNEL_18) /*!< ADC internal channel connected to DAC1 channel 2, channel specific to ADC1. This channel is shared with ADC internal channel connected to Vbat, they cannot be used both simultenaeously. */
#elif defined(ADC2)
#define ADC_CHANNEL_DAC1CH1_ADC2        (ADC_CHANNEL_17) /*!< ADC internal channel connected to DAC1 channel 1, channel specific to ADC2 */
#define ADC_CHANNEL_DAC1CH2_ADC2        (ADC_CHANNEL_18) /*!< ADC internal channel connected to DAC1 channel 2, channel specific to ADC2 */
#if defined(ADC3)
#define ADC_CHANNEL_DAC1CH1_ADC3        (ADC_CHANNEL_14) /*!< ADC internal channel connected to DAC1 channel 1, channel specific to ADC3 */
#define ADC_CHANNEL_DAC1CH2_ADC3        (ADC_CHANNEL_15) /*!< ADC internal channel connected to DAC1 channel 2, channel specific to ADC3 */
#endif
#endif
/**
  * @}
  */

/** @defgroup ADC_analog_watchdog_number ADC Analog Watchdog Selection
  * @{
  */
#define ADC_ANALOGWATCHDOG_1                    ((uint32_t)0x00000001)   /*!< Analog watchdog 1 selection */ 
#define ADC_ANALOGWATCHDOG_2                    ((uint32_t)0x00000002)   /*!< Analog watchdog 2 selection */ 
#define ADC_ANALOGWATCHDOG_3                    ((uint32_t)0x00000003)   /*!< Analog watchdog 3 selection */ 
/**
  * @}
  */

/** @defgroup ADC_analog_watchdog_mode ADC Analog Watchdog Mode
  * @{
  */
#define ADC_ANALOGWATCHDOG_NONE                 ((uint32_t) 0x00000000)                                             /*!< No analog watchdog selected                                             */
#define ADC_ANALOGWATCHDOG_SINGLE_REG           ((uint32_t)(ADC_CFGR_AWD1SGL | ADC_CFGR_AWD1EN))                    /*!< Analog watchdog applied to a regular group single channel               */
#define ADC_ANALOGWATCHDOG_SINGLE_INJEC         ((uint32_t)(ADC_CFGR_AWD1SGL | ADC_CFGR_JAWD1EN))                   /*!< Analog watchdog applied to an injected group single channel             */
#define ADC_ANALOGWATCHDOG_SINGLE_REGINJEC      ((uint32_t)(ADC_CFGR_AWD1SGL | ADC_CFGR_AWD1EN | ADC_CFGR_JAWD1EN)) /*!< Analog watchdog applied to a regular and injected groups single channel */
#define ADC_ANALOGWATCHDOG_ALL_REG              ((uint32_t) ADC_CFGR_AWD1EN)                                        /*!< Analog watchdog applied to regular group all channels                   */
#define ADC_ANALOGWATCHDOG_ALL_INJEC            ((uint32_t) ADC_CFGR_JAWD1EN)                                       /*!< Analog watchdog applied to injected group all channels                  */
#define ADC_ANALOGWATCHDOG_ALL_REGINJEC         ((uint32_t)(ADC_CFGR_AWD1EN | ADC_CFGR_JAWD1EN))                    /*!< Analog watchdog applied to regular and injected groups all channels     */
/**
  * @}
  */

/** @defgroup ADC_Oversampling_Ratio ADC Oversampling Ratio
  * @{
  */
#define ADC_OVERSAMPLING_RATIO_2      ((uint32_t)0x00000000)                            /*!<  ADC Oversampling ratio 2x   */
#define ADC_OVERSAMPLING_RATIO_4      ((uint32_t)ADC_CFGR2_OVSR_0)                      /*!<  ADC Oversampling ratio 4x   */
#define ADC_OVERSAMPLING_RATIO_8      ((uint32_t)ADC_CFGR2_OVSR_1)                      /*!<  ADC Oversampling ratio 8x   */
#define ADC_OVERSAMPLING_RATIO_16     ((uint32_t)(ADC_CFGR2_OVSR_1 | ADC_CFGR2_OVSR_0)) /*!<  ADC Oversampling ratio 16x  */
#define ADC_OVERSAMPLING_RATIO_32     ((uint32_t)ADC_CFGR2_OVSR_2)                      /*!<  ADC Oversampling ratio 32x  */
#define ADC_OVERSAMPLING_RATIO_64     ((uint32_t)(ADC_CFGR2_OVSR_2 | ADC_CFGR2_OVSR_0)) /*!<  ADC Oversampling ratio 64x  */
#define ADC_OVERSAMPLING_RATIO_128    ((uint32_t)(ADC_CFGR2_OVSR_2 | ADC_CFGR2_OVSR_1)) /*!<  ADC Oversampling ratio 128x */
#define ADC_OVERSAMPLING_RATIO_256    ((uint32_t)(ADC_CFGR2_OVSR))                      /*!<  ADC Oversampling ratio 256x */
/**
  * @}
  */

/** @defgroup ADC_Right_Bit_Shift ADC Oversampling Right Shift
  * @{
  */
#define ADC_RIGHTBITSHIFT_NONE  ((uint32_t)0x00000000)                                               /*!<  ADC No bit shift for oversampling */
#define ADC_RIGHTBITSHIFT_1     ((uint32_t)ADC_CFGR2_OVSS_0)                                         /*!<  ADC 1 bit shift for oversampling  */
#define ADC_RIGHTBITSHIFT_2     ((uint32_t)ADC_CFGR2_OVSS_1)                                         /*!<  ADC 2 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_3     ((uint32_t)(ADC_CFGR2_OVSS_1 | ADC_CFGR2_OVSS_0))                    /*!<  ADC 3 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_4     ((uint32_t)ADC_CFGR2_OVSS_2)                                         /*!<  ADC 4 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_5     ((uint32_t)(ADC_CFGR2_OVSS_2 | ADC_CFGR2_OVSS_0))                    /*!<  ADC 5 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_6     ((uint32_t)(ADC_CFGR2_OVSS_2 | ADC_CFGR2_OVSS_1))                    /*!<  ADC 6 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_7     ((uint32_t)(ADC_CFGR2_OVSS_2 | ADC_CFGR2_OVSS_1 | ADC_CFGR2_OVSS_0)) /*!<  ADC 7 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_8     ((uint32_t)ADC_CFGR2_OVSS_3)                                         /*!<  ADC 8 bits shift for oversampling */
/**
  * @}
  */

/** @defgroup ADC_Triggered_Oversampling_Mode   ADC Triggered Regular Oversampling
  * @{
  */
#define ADC_TRIGGEREDMODE_SINGLE_TRIGGER      ((uint32_t)0x00000000)      /*!<  A single trigger for all channel oversampled conversions */
#define ADC_TRIGGEREDMODE_MULTI_TRIGGER       ((uint32_t)ADC_CFGR2_TROVS) /*!<  A trigger for each oversampled conversion                */
/**
  * @}
  */

/** @defgroup ADC_Regular_Oversampling_Mode   ADC Regular Oversampling Continued or Resumed Mode
  * @{
  */
#define ADC_REGOVERSAMPLING_CONTINUED_MODE    ((uint32_t)0x00000000)      /*!<  Oversampling buffer maintained during injection sequence */
#define ADC_REGOVERSAMPLING_RESUMED_MODE      ((uint32_t)ADC_CFGR2_ROVSM) /*!<  Oversampling buffer zeroed during injection sequence     */
/**
  * @}
  */


/** @defgroup ADC_Event_type ADC Event Type
  * @{
  */
#define ADC_EOSMP_EVENT          ((uint32_t)ADC_FLAG_EOSMP) /*!< ADC End of Sampling event */
#define ADC_AWD1_EVENT           ((uint32_t)ADC_FLAG_AWD1)  /*!< ADC Analog watchdog 1 event (main analog watchdog, present on all STM32 series) */
#define ADC_AWD2_EVENT           ((uint32_t)ADC_FLAG_AWD2)  /*!< ADC Analog watchdog 2 event (additional analog watchdog, not present on all STM32 series) */
#define ADC_AWD3_EVENT           ((uint32_t)ADC_FLAG_AWD3)  /*!< ADC Analog watchdog 3 event (additional analog watchdog, not present on all STM32 series) */
#define ADC_OVR_EVENT            ((uint32_t)ADC_FLAG_OVR)   /*!< ADC overrun event */
#define ADC_JQOVF_EVENT          ((uint32_t)ADC_FLAG_JQOVF) /*!< ADC Injected Context Queue Overflow event */
/**
  * @}
  */
#define ADC_AWD_EVENT            ADC_AWD1_EVENT      /*!< ADC Analog watchdog 1 event: Naming for compatibility with other STM32 devices having only one analog watchdog */

/** @defgroup ADC_interrupts_definition ADC Interrupts Definition
  * @{
  */
#define ADC_IT_RDY           ADC_IER_ADRDY      /*!< ADC Ready (ADRDY) interrupt source */
#define ADC_IT_EOSMP         ADC_IER_EOSMP      /*!< ADC End of sampling interrupt source */
#define ADC_IT_EOC           ADC_IER_EOC        /*!< ADC End of regular conversion interrupt source */
#define ADC_IT_EOS           ADC_IER_EOS        /*!< ADC End of regular sequence of conversions interrupt source */
#define ADC_IT_OVR           ADC_IER_OVR        /*!< ADC overrun interrupt source */
#define ADC_IT_JEOC          ADC_IER_JEOC       /*!< ADC End of injected conversion interrupt source */
#define ADC_IT_JEOS          ADC_IER_JEOS       /*!< ADC End of injected sequence of conversions interrupt source */
#define ADC_IT_AWD1          ADC_IER_AWD1       /*!< ADC Analog watchdog 1 interrupt source (main analog watchdog) */
#define ADC_IT_AWD2          ADC_IER_AWD2       /*!< ADC Analog watchdog 2 interrupt source (additional analog watchdog) */
#define ADC_IT_AWD3          ADC_IER_AWD3       /*!< ADC Analog watchdog 3 interrupt source (additional analog watchdog) */
#define ADC_IT_JQOVF         ADC_IER_JQOVF      /*!< ADC Injected Context Queue Overflow interrupt source */

#define ADC_IT_AWD           ADC_IT_AWD1        /*!< ADC Analog watchdog 1 interrupt source: naming for compatibility with other STM32 devices having only one analog watchdog */

/**
  * @}
  */

/** @defgroup ADC_flags_definition ADC Flags Definition
  * @{
  */
#define ADC_FLAG_RDY           ADC_ISR_ADRDY    /*!< ADC Ready flag */
#define ADC_FLAG_EOSMP         ADC_ISR_EOSMP    /*!< ADC End of Sampling flag */
#define ADC_FLAG_EOC           ADC_ISR_EOC      /*!< ADC End of Regular Conversion flag */
#define ADC_FLAG_EOS           ADC_ISR_EOS      /*!< ADC End of Regular sequence of Conversions flag */
#define ADC_FLAG_OVR           ADC_ISR_OVR      /*!< ADC overrun flag */
#define ADC_FLAG_JEOC          ADC_ISR_JEOC     /*!< ADC End of Injected Conversion flag */
#define ADC_FLAG_JEOS          ADC_ISR_JEOS     /*!< ADC End of Injected sequence of Conversions flag */
#define ADC_FLAG_AWD1          ADC_ISR_AWD1     /*!< ADC Analog watchdog 1 flag (main analog watchdog) */
#define ADC_FLAG_AWD2          ADC_ISR_AWD2     /*!< ADC Analog watchdog 2 flag (additional analog watchdog) */
#define ADC_FLAG_AWD3          ADC_ISR_AWD3     /*!< ADC Analog watchdog 3 flag (additional analog watchdog) */
#define ADC_FLAG_JQOVF         ADC_ISR_JQOVF    /*!< ADC Injected Context Queue Overflow flag */

#define ADC_FLAG_AWD           ADC_FLAG_AWD1    /*!< ADC Analog watchdog 1 flag: Naming for compatibility with other STM32 devices having only one analog watchdog */

#define ADC_FLAG_ALL    (ADC_FLAG_RDY | ADC_FLAG_EOSMP | ADC_FLAG_EOC | ADC_FLAG_EOS |  \
                         ADC_FLAG_JEOC | ADC_FLAG_JEOS | ADC_FLAG_OVR | ADC_FLAG_AWD1 | \
                         ADC_FLAG_AWD2 | ADC_FLAG_AWD3 | ADC_FLAG_JQOVF)   /*!< ADC all flags */

/* Combination of all post-conversion flags bits: EOC/EOS, JEOC/JEOS, OVR, AWDx, JQOVF */
#define ADC_FLAG_POSTCONV_ALL (ADC_FLAG_EOC | ADC_FLAG_EOS  | ADC_FLAG_JEOC | ADC_FLAG_JEOS | \
                               ADC_FLAG_OVR | ADC_FLAG_AWD1 | ADC_FLAG_AWD2 | ADC_FLAG_AWD3 | \
                               ADC_FLAG_JQOVF)                             /*!< ADC post-conversion all flags */

/**
  * @}
  */

/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/

/** @defgroup ADC_Private_Macros ADC Private Macros
  * @{
  */
/* Macro reserved for internal HAL driver usage, not intended to be used in   */
/* code of final user.                                                        */

/**
  * @brief Test if conversion trigger of regular group is software start
  *        or external trigger.
  * @param __HANDLE__ ADC handle
  * @retval SET (software start) or RESET (external trigger)
  */
#define ADC_IS_SOFTWARE_START_REGULAR(__HANDLE__)                        \
       (((__HANDLE__)->Instance->CFGR & ADC_CFGR_EXTEN) == RESET)

/**
  * @brief Return resolution bits in CFGR register RES[1:0] field.
  * @param __HANDLE__ ADC handle
  * @retval 2-bit field RES of CFGR register.
  */
#define ADC_GET_RESOLUTION(__HANDLE__) (((__HANDLE__)->Instance->CFGR) & ADC_CFGR_RES)

/**
  * @brief Clear ADC error code (set it to no error code "HAL_ADC_ERROR_NONE").
  * @param __HANDLE__ ADC handle
  * @retval None
  */
#define ADC_CLEAR_ERRORCODE(__HANDLE__) ((__HANDLE__)->ErrorCode = HAL_ADC_ERROR_NONE) 

/**
  * @brief Verification of ADC state: enabled or disabled.
  * @param __HANDLE__ ADC handle
  * @retval SET (ADC enabled) or RESET (ADC disabled)
  */
#define ADC_IS_ENABLE(__HANDLE__)                                                    \
       (( ((((__HANDLE__)->Instance->CR) & (ADC_CR_ADEN | ADC_CR_ADDIS)) == ADC_CR_ADEN) && \
          ((((__HANDLE__)->Instance->ISR) & ADC_FLAG_RDY) == ADC_FLAG_RDY)                  \
        ) ? SET : RESET)

/**
  * @brief Check if conversion is on going on regular group.
  * @param __HANDLE__ ADC handle
  * @retval SET (conversion is on going) or RESET (no conversion is on going)
  */
#define ADC_IS_CONVERSION_ONGOING_REGULAR(__HANDLE__)                          \
  (( (((__HANDLE__)->Instance->CR) & ADC_CR_ADSTART) == RESET                  \
  ) ? RESET : SET)

/**
  * @brief Simultaneously clear and set specific bits of the handle State.
  * @note  ADC_STATE_CLR_SET() macro is merely aliased to generic macro MODIFY_REG(),
  *        the first parameter is the ADC handle State, the second parameter is the
  *        bit field to clear, the third and last parameter is the bit field to set.
  * @retval None
  */
#define ADC_STATE_CLR_SET MODIFY_REG

/**
  * @brief Verify that a given value is aligned with the ADC resolution range.
  * @param __RESOLUTION__ ADC resolution (12, 10, 8 or 6 bits).
  * @param __ADC_VALUE__ value checked against the resolution.     
  * @retval SET (__ADC_VALUE__ in line with __RESOLUTION__) or RESET (__ADC_VALUE__ not in line with __RESOLUTION__)
  */
#define IS_ADC_RANGE(__RESOLUTION__, __ADC_VALUE__)                                         \
   ((((__RESOLUTION__) == ADC_RESOLUTION_12B) && ((__ADC_VALUE__) <= ((uint32_t)0x0FFF))) || \
    (((__RESOLUTION__) == ADC_RESOLUTION_10B) && ((__ADC_VALUE__) <= ((uint32_t)0x03FF))) || \
    (((__RESOLUTION__) == ADC_RESOLUTION_8B)  && ((__ADC_VALUE__) <= ((uint32_t)0x00FF))) || \
    (((__RESOLUTION__) == ADC_RESOLUTION_6B)  && ((__ADC_VALUE__) <= ((uint32_t)0x003F)))   )

/**
  * @brief Verify the length of the scheduled regular conversions group.
  * @param __LENGTH__ number of programmed conversions.   
  * @retval SET (__LENGTH__ is within the maximum number of possible programmable regular conversions) or RESET (__LENGTH__ is null or too large)
  */
#define IS_ADC_REGULAR_NB_CONV(__LENGTH__) (((__LENGTH__) >= ((uint32_t)1)) && ((__LENGTH__) <= ((uint32_t)16)))


/**
  * @brief Verify the number of scheduled regular conversions in discontinuous mode.
  * @param NUMBER number of scheduled regular conversions in discontinuous mode.  
  * @retval SET (NUMBER is within the maximum number of regular conversions in discontinous mode) or RESET (NUMBER is null or too large)
  */
#define IS_ADC_REGULAR_DISCONT_NUMBER(NUMBER) (((NUMBER) >= ((uint32_t)1)) && ((NUMBER) <= ((uint32_t)8)))


/**
  * @brief Verify the ADC clock setting.
  * @param __ADC_CLOCK__ programmed ADC clock. 
  * @retval SET (__ADC_CLOCK__ is a valid value) or RESET (__ADC_CLOCK__ is invalid)
  */
#define IS_ADC_CLOCKPRESCALER(__ADC_CLOCK__) (((__ADC_CLOCK__) == ADC_CLOCK_SYNC_PCLK_DIV1) || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_SYNC_PCLK_DIV2) || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_SYNC_PCLK_DIV4) || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV1)     || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV2)     || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV4)     || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV6)     || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV8)     || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV10)    || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV12)    || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV16)    || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV32)    || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV64)    || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV128)   || \
                                              ((__ADC_CLOCK__) == ADC_CLOCK_ASYNC_DIV256) )  

/**
  * @brief Verify the ADC resolution setting.
  * @param __RESOLUTION__ programmed ADC resolution. 
  * @retval SET (__RESOLUTION__ is a valid value) or RESET (__RESOLUTION__ is invalid)
  */
#define IS_ADC_RESOLUTION(__RESOLUTION__) (((__RESOLUTION__) == ADC_RESOLUTION_12B) || \
                                           ((__RESOLUTION__) == ADC_RESOLUTION_10B) || \
                                           ((__RESOLUTION__) == ADC_RESOLUTION_8B)  || \
                                           ((__RESOLUTION__) == ADC_RESOLUTION_6B)    )
                             
/**                          
  * @brief Verify the ADC resolution setting when limited to 6 or 8 bits.
  * @param __RESOLUTION__ programmed ADC resolution when limited to 6 or 8 bits. 
  * @retval SET (__RESOLUTION__ is a valid value) or RESET (__RESOLUTION__ is invalid)
  */ 
#define IS_ADC_RESOLUTION_8_6_BITS(__RESOLUTION__) (((__RESOLUTION__) == ADC_RESOLUTION_8B) || \
                                                    ((__RESOLUTION__) == ADC_RESOLUTION_6B)   )

/**
  * @brief Verify the ADC converted data alignment.
  * @param __ALIGN__ programmed ADC converted data alignment. 
  * @retval SET (__ALIGN__ is a valid value) or RESET (__ALIGN__ is invalid)
  */
#define IS_ADC_DATA_ALIGN(__ALIGN__) (((__ALIGN__) == ADC_DATAALIGN_RIGHT) || \
                                      ((__ALIGN__) == ADC_DATAALIGN_LEFT)    )

/**
  * @brief Verify the ADC scan mode.
  * @param __SCAN_MODE__ programmed ADC scan mode.
  * @retval SET (__SCAN_MODE__ is valid) or RESET (__SCAN_MODE__ is invalid)
  */
#define IS_ADC_SCAN_MODE(__SCAN_MODE__) (((__SCAN_MODE__) == ADC_SCAN_DISABLE) || \
                                         ((__SCAN_MODE__) == ADC_SCAN_ENABLE)    )

/**
  * @brief Verify the ADC edge trigger setting for regular group.
  * @param __EDGE__ programmed ADC edge trigger setting.
  * @retval SET (__EDGE__ is a valid value) or RESET (__EDGE__ is invalid)
  */
#define IS_ADC_EXTTRIG_EDGE(__EDGE__) (((__EDGE__) == ADC_EXTERNALTRIGCONVEDGE_NONE)         || \
                                       ((__EDGE__) == ADC_EXTERNALTRIGCONVEDGE_RISING)       || \
                                       ((__EDGE__) == ADC_EXTERNALTRIGCONVEDGE_FALLING)      || \
                                       ((__EDGE__) == ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING)  )

/**
  * @brief Verify the ADC regular conversions external trigger.
  * @param __HANDLE__ ADC handle
  * @param __REGTRIG__ programmed ADC regular conversions external trigger.
  * @retval SET (__REGTRIG__ is a valid value) or RESET (__REGTRIG__ is invalid)
  */
#define IS_ADC_EXTTRIG(__HANDLE__, __REGTRIG__) (((__REGTRIG__) == ADC_EXTERNALTRIG_T1_CC1)   || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T1_CC2)   || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T1_CC3)   || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T2_CC2)   || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T3_TRGO)  || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T4_CC4)   || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_EXT_IT11) || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T8_TRGO)  || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T8_TRGO2) || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T1_TRGO)  || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T1_TRGO2) || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T2_TRGO)  || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T4_TRGO)  || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T6_TRGO)  || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T15_TRGO) || \
                                                 ((__REGTRIG__) == ADC_EXTERNALTRIG_T3_CC4)   || \
                                                                                             \
                                                 ((__REGTRIG__) == ADC_SOFTWARE_START)           )

/**
  * @brief Verify the ADC regular conversions check for converted data availability.
  * @param __EOC_SELECTION__ converted data availability check.
  * @retval SET (__EOC_SELECTION__ is a valid value) or RESET (__EOC_SELECTION__ is invalid)
  */
#define IS_ADC_EOC_SELECTION(__EOC_SELECTION__) (((__EOC_SELECTION__) == ADC_EOC_SINGLE_CONV)    || \
                                                 ((__EOC_SELECTION__) == ADC_EOC_SEQ_CONV)  )

/**
  * @brief Verify the ADC regular conversions overrun handling.
  * @param __OVR__ ADC regular conversions overrun handling.
  * @retval SET (__OVR__ is a valid value) or RESET (__OVR__ is invalid)
  */
#define IS_ADC_OVERRUN(__OVR__) (((__OVR__) == ADC_OVR_DATA_PRESERVED)  || \
                                 ((__OVR__) == ADC_OVR_DATA_OVERWRITTEN)  )

/**
  * @brief Verify the ADC conversions sampling time.
  * @param __TIME__ ADC conversions sampling time.
  * @retval SET (__TIME__ is a valid value) or RESET (__TIME__ is invalid)
  */
#if defined(ADC_SMPR1_SMPPLUS)
#define IS_ADC_SAMPLE_TIME(__TIME__) (((__TIME__) == ADC_SAMPLETIME_2CYCLES_5)   || \
                                      ((__TIME__) == ADC_SAMPLETIME_3CYCLES_5)   || \
                                      ((__TIME__) == ADC_SAMPLETIME_6CYCLES_5)   || \
                                      ((__TIME__) == ADC_SAMPLETIME_12CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_24CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_47CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_92CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_247CYCLES_5) || \
                                      ((__TIME__) == ADC_SAMPLETIME_640CYCLES_5)   )
#else
#define IS_ADC_SAMPLE_TIME(__TIME__) (((__TIME__) == ADC_SAMPLETIME_2CYCLES_5)   || \
                                      ((__TIME__) == ADC_SAMPLETIME_6CYCLES_5)   || \
                                      ((__TIME__) == ADC_SAMPLETIME_12CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_24CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_47CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_92CYCLES_5)  || \
                                      ((__TIME__) == ADC_SAMPLETIME_247CYCLES_5) || \
                                      ((__TIME__) == ADC_SAMPLETIME_640CYCLES_5)   )
#endif

/**
  * @brief Verify the ADC regular channel setting.
  * @param  __CHANNEL__ programmed ADC regular channel. 
  * @retval SET (__CHANNEL__ is valid) or RESET (__CHANNEL__ is invalid)
  */
#define IS_ADC_REGULAR_RANK(__CHANNEL__) (((__CHANNEL__) == ADC_REGULAR_RANK_1 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_2 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_3 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_4 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_5 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_6 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_7 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_8 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_9 ) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_10) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_11) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_12) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_13) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_14) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_15) || \
                                          ((__CHANNEL__) == ADC_REGULAR_RANK_16)   )

/**
  * @}
  */


/* Private constants ---------------------------------------------------------*/

/** @defgroup ADC_Private_Constants ADC Private Constants
  * @{
  */

/* Fixed timeout values for ADC conversion (including sampling time)        */
/* Maximum sampling time is 640.5 ADC clock cycle (SMPx[2:0] = 0b111        */
/* Maximum conversion time is 12.5 + Maximum sampling time                  */
/*                       or 12.5  + 640.5 = 653 ADC clock cycles            */
/* Minimum ADC Clock frequency is 0.14 MHz                                  */
/* Maximum conversion time is                                               */
/*              653 / 0.14 MHz = 4.66 ms                                    */
#define ADC_STOP_CONVERSION_TIMEOUT     ((uint32_t) 5)      /*!< ADC stop time-out value */ 

/* Delay for temperature sensor stabilization time.                         */
/* Maximum delay is 120us (refer device datasheet, parameter tSTART).       */
/* Unit: us                                                                 */
#define ADC_TEMPSENSOR_DELAY_US         ((uint32_t) 120)

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/** @defgroup ADC_Exported_Macros ADC Exported Macros
  * @{
  */
/* Macro for internal HAL driver usage, and possibly can be used into code of */
/* final user.                                                                */

/** @defgroup ADC_HAL_EM_HANDLE_IT_FLAG HAL ADC macro to manage HAL ADC handle, IT and flags.
  * @{
  */

/** @brief  Reset ADC handle state.
  * @param __HANDLE__ ADC handle
  * @retval None
  */
#define __HAL_ADC_RESET_HANDLE_STATE(__HANDLE__)                               \
  ((__HANDLE__)->State = HAL_ADC_STATE_RESET)

/**
  * @brief Enable ADC interrupt.
  * @param __HANDLE__ ADC handle
  * @param __INTERRUPT__ ADC Interrupt
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_IT_RDY,    ADC Ready (ADRDY) interrupt source
  *            @arg @ref ADC_IT_EOSMP,  ADC End of Sampling interrupt source
  *            @arg @ref ADC_IT_EOC,    ADC End of Regular Conversion interrupt source
  *            @arg @ref ADC_IT_EOS,    ADC End of Regular sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_OVR,    ADC overrun interrupt source
  *            @arg @ref ADC_IT_JEOC,   ADC End of Injected Conversion interrupt source
  *            @arg @ref ADC_IT_JEOS,   ADC End of Injected sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_AWD1,   ADC Analog watchdog 1 interrupt source (main analog watchdog)
  *            @arg @ref ADC_IT_AWD2,   ADC Analog watchdog 2 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_AWD3,   ADC Analog watchdog 3 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_JQOVF,  ADC Injected Context Queue Overflow interrupt source. 
  * @retval None
  */
#define __HAL_ADC_ENABLE_IT(__HANDLE__, __INTERRUPT__)                         \
  (((__HANDLE__)->Instance->IER) |= (__INTERRUPT__))

/**
  * @brief Disable ADC interrupt.
  * @param __HANDLE__ ADC handle
  * @param __INTERRUPT__ ADC Interrupt
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_IT_RDY,    ADC Ready (ADRDY) interrupt source
  *            @arg @ref ADC_IT_EOSMP,  ADC End of Sampling interrupt source
  *            @arg @ref ADC_IT_EOC,    ADC End of Regular Conversion interrupt source
  *            @arg @ref ADC_IT_EOS,    ADC End of Regular sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_OVR,    ADC overrun interrupt source
  *            @arg @ref ADC_IT_JEOC,   ADC End of Injected Conversion interrupt source
  *            @arg @ref ADC_IT_JEOS,   ADC End of Injected sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_AWD1,   ADC Analog watchdog 1 interrupt source (main analog watchdog)
  *            @arg @ref ADC_IT_AWD2,   ADC Analog watchdog 2 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_AWD3,   ADC Analog watchdog 3 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_JQOVF,  ADC Injected Context Queue Overflow interrupt source. 
  * @retval None
  */
#define __HAL_ADC_DISABLE_IT(__HANDLE__, __INTERRUPT__)                        \
  (((__HANDLE__)->Instance->IER) &= ~(__INTERRUPT__))

/** @brief  Checks if the specified ADC interrupt source is enabled or disabled.
  * @param __HANDLE__ ADC handle
  * @param __INTERRUPT__ ADC interrupt source to check
  *          This parameter can be one of the following values:
  *            @arg @ref ADC_IT_RDY,    ADC Ready (ADRDY) interrupt source
  *            @arg @ref ADC_IT_EOSMP,  ADC End of Sampling interrupt source
  *            @arg @ref ADC_IT_EOC,    ADC End of Regular Conversion interrupt source
  *            @arg @ref ADC_IT_EOS,    ADC End of Regular sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_OVR,    ADC overrun interrupt source
  *            @arg @ref ADC_IT_JEOC,   ADC End of Injected Conversion interrupt source
  *            @arg @ref ADC_IT_JEOS,   ADC End of Injected sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_AWD1,   ADC Analog watchdog 1 interrupt source (main analog watchdog)
  *            @arg @ref ADC_IT_AWD2,   ADC Analog watchdog 2 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_AWD3,   ADC Analog watchdog 3 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_JQOVF,  ADC Injected Context Queue Overflow interrupt source.  
  * @retval State of interruption (SET or RESET)
  */
#define __HAL_ADC_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)                     \
  (((__HANDLE__)->Instance->IER & (__INTERRUPT__)) == (__INTERRUPT__))
    
/**
  * @brief Check whether the specified ADC flag is set or not.
  * @param __HANDLE__ ADC handle
  * @param __FLAG__ ADC flag
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_FLAG_RDY,     ADC Ready (ADRDY) flag                              
  *            @arg @ref ADC_FLAG_EOSMP,   ADC End of Sampling flag                            
  *            @arg @ref ADC_FLAG_EOC,     ADC End of Regular Conversion flag                  
  *            @arg @ref ADC_FLAG_EOS,     ADC End of Regular sequence of Conversions flag     
  *            @arg @ref ADC_FLAG_OVR,     ADC overrun flag        
  *            @arg @ref ADC_FLAG_JEOC,    ADC End of Injected Conversion flag                 
  *            @arg @ref ADC_FLAG_JEOS,    ADC End of Injected sequence of Conversions flag    
  *            @arg @ref ADC_FLAG_AWD1,    ADC Analog watchdog 1 flag (main analog watchdog)
  *            @arg @ref ADC_FLAG_AWD2,    ADC Analog watchdog 2 flag (additional analog watchdog)
  *            @arg @ref ADC_FLAG_AWD3,    ADC Analog watchdog 3 flag (additional analog watchdog)
  *            @arg @ref ADC_FLAG_JQOVF,   ADC Injected Context Queue Overflow flag.            
  * @retval State of flag (TRUE or FALSE).
  */
#define __HAL_ADC_GET_FLAG(__HANDLE__, __FLAG__)                               \
  ((((__HANDLE__)->Instance->ISR) & (__FLAG__)) == (__FLAG__))

/**
  * @brief Clear the specified ADC flag.
  * @param __HANDLE__ ADC handle
  * @param __FLAG__ ADC flag
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_FLAG_RDY,     ADC Ready (ADRDY) flag                              
  *            @arg @ref ADC_FLAG_EOSMP,   ADC End of Sampling flag                            
  *            @arg @ref ADC_FLAG_EOC,     ADC End of Regular Conversion flag                  
  *            @arg @ref ADC_FLAG_EOS,     ADC End of Regular sequence of Conversions flag     
  *            @arg @ref ADC_FLAG_OVR,     ADC overrun flag        
  *            @arg @ref ADC_FLAG_JEOC,    ADC End of Injected Conversion flag                 
  *            @arg @ref ADC_FLAG_JEOS,    ADC End of Injected sequence of Conversions flag    
  *            @arg @ref ADC_FLAG_AWD1,    ADC Analog watchdog 1 flag (main analog watchdog)
  *            @arg @ref ADC_FLAG_AWD2,    ADC Analog watchdog 2 flag (additional analog watchdog)
  *            @arg @ref ADC_FLAG_AWD3,    ADC Analog watchdog 3 flag (additional analog watchdog)
  *            @arg @ref ADC_FLAG_JQOVF,   ADC Injected Context Queue Overflow flag.   
  * @note  Bit cleared bit by writing 1 (writing 0 has no effect on any bit of register ISR).
  * @retval None
  */
/* Note: bit cleared bit by writing 1 (writing 0 has no effect on any bit of register ISR) */
#define __HAL_ADC_CLEAR_FLAG(__HANDLE__, __FLAG__)                             \
  (((__HANDLE__)->Instance->ISR) = (__FLAG__))

/**
  * @}
  */

/** @defgroup ADC_HAL_EM_HELPER_MACRO HAL ADC helper macro
  * @{
  */

/**
  * @brief  Helper macro to get ADC channel number in decimal format
  *         from literals ADC_CHANNEL_x.
  * @note   Example:
  *           __HAL_ADC_CHANNEL_TO_DECIMAL_NB(ADC_CHANNEL_4)
  *           will return decimal number "4".
  * @note   The input can be a value from functions where a channel
  *         number is returned, either defined with number
  *         or with bitfield (only one bit must be set).
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref ADC_CHANNEL_0
  *         @arg @ref ADC_CHANNEL_1            (7)
  *         @arg @ref ADC_CHANNEL_2            (7)
  *         @arg @ref ADC_CHANNEL_3            (7)
  *         @arg @ref ADC_CHANNEL_4            (7)
  *         @arg @ref ADC_CHANNEL_5            (7)
  *         @arg @ref ADC_CHANNEL_6
  *         @arg @ref ADC_CHANNEL_7
  *         @arg @ref ADC_CHANNEL_8
  *         @arg @ref ADC_CHANNEL_9
  *         @arg @ref ADC_CHANNEL_10
  *         @arg @ref ADC_CHANNEL_11
  *         @arg @ref ADC_CHANNEL_12
  *         @arg @ref ADC_CHANNEL_13
  *         @arg @ref ADC_CHANNEL_14
  *         @arg @ref ADC_CHANNEL_15
  *         @arg @ref ADC_CHANNEL_16
  *         @arg @ref ADC_CHANNEL_17
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT      (1)
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR   (4)
  *         @arg @ref ADC_CHANNEL_VBAT         (4)
  *         @arg @ref ADC_CHANNEL_DAC1CH1         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH2         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC3 (3)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC3 (3)(6)
  *         
  *         (1) On STM32L4, parameter available only on ADC instance: ADC1.\n
  *         (2) On STM32L4, parameter available only on ADC instance: ADC2.\n
  *         (3) On STM32L4, parameter available only on ADC instance: ADC3.\n
  *         (4) On STM32L4, parameter available only on ADC instances: ADC1, ADC3.\n
  *         (5) On STM32L4, parameter available on devices with only 1 ADC instance.\n
  *         (6) On STM32L4, parameter available on devices with several ADC instances.\n
  *         (7) On STM32L4, fast channel (0.188 us for 12-bit resolution (ADC conversion rate up to 5.33 Ms/s)).
  *             Other channels are slow channels (0.238 us for 12-bit resolution (ADC conversion rate up to 4.21 Ms/s)).
  * @retval Value between Min_Data=0 and Max_Data=18
  */
#define __HAL_ADC_CHANNEL_TO_DECIMAL_NB(__CHANNEL__)                           \
         __LL_ADC_CHANNEL_TO_DECIMAL_NB(__CHANNEL__)

/**
  * @brief  Helper macro to get ADC channel in literal format ADC_CHANNEL_x
  *         from number in decimal format.
  * @note   Example:
  *           __HAL_ADC_DECIMAL_NB_TO_CHANNEL(4)
  *           will return a data equivalent to "ADC_CHANNEL_4".
  * @param  __DECIMAL_NB__ Value between Min_Data=0 and Max_Data=18
  * @retval Returned value can be one of the following values:
  *         @arg @ref ADC_CHANNEL_0
  *         @arg @ref ADC_CHANNEL_1            (7)
  *         @arg @ref ADC_CHANNEL_2            (7)
  *         @arg @ref ADC_CHANNEL_3            (7)
  *         @arg @ref ADC_CHANNEL_4            (7)
  *         @arg @ref ADC_CHANNEL_5            (7)
  *         @arg @ref ADC_CHANNEL_6
  *         @arg @ref ADC_CHANNEL_7
  *         @arg @ref ADC_CHANNEL_8
  *         @arg @ref ADC_CHANNEL_9
  *         @arg @ref ADC_CHANNEL_10
  *         @arg @ref ADC_CHANNEL_11
  *         @arg @ref ADC_CHANNEL_12
  *         @arg @ref ADC_CHANNEL_13
  *         @arg @ref ADC_CHANNEL_14
  *         @arg @ref ADC_CHANNEL_15
  *         @arg @ref ADC_CHANNEL_16
  *         @arg @ref ADC_CHANNEL_17
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT      (1)
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR   (4)
  *         @arg @ref ADC_CHANNEL_VBAT         (4)
  *         @arg @ref ADC_CHANNEL_DAC1CH1         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH2         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC3 (3)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC3 (3)(6)
  *         
  *         (1) On STM32L4, parameter available only on ADC instance: ADC1.\n
  *         (2) On STM32L4, parameter available only on ADC instance: ADC2.\n
  *         (3) On STM32L4, parameter available only on ADC instance: ADC3.\n
  *         (4) On STM32L4, parameter available only on ADC instances: ADC1, ADC3.\n
  *         (5) On STM32L4, parameter available on devices with only 1 ADC instance.\n
  *         (6) On STM32L4, parameter available on devices with several ADC instances.\n
  *         (7) On STM32L4, fast channel (0.188 us for 12-bit resolution (ADC conversion rate up to 5.33 Ms/s)).
  *             Other channels are slow channels (0.238 us for 12-bit resolution (ADC conversion rate up to 4.21 Ms/s)).\n
  *         (1, 2, 3, 4) For ADC channel read back from ADC register,
  *                      comparison with internal channel parameter to be done
  *                      using helper macro @ref __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL().
  */
#define __HAL_ADC_DECIMAL_NB_TO_CHANNEL(__DECIMAL_NB__)                        \
         __LL_ADC_DECIMAL_NB_TO_CHANNEL(__DECIMAL_NB__)

/**
  * @brief  Helper macro to determine whether the selected channel
  *         corresponds to literal definitions of driver.
  * @note   The different literal definitions of ADC channels are:
  *         - ADC internal channel:
  *           ADC_CHANNEL_VREFINT, ADC_CHANNEL_TEMPSENSOR, ...
  *         - ADC external channel (channel connected to a GPIO pin):
  *           ADC_CHANNEL_1, ADC_CHANNEL_2, ...
  * @note   The channel parameter must be a value defined from literal
  *         definition of a ADC internal channel (ADC_CHANNEL_VREFINT,
  *         ADC_CHANNEL_TEMPSENSOR, ...),
  *         ADC external channel (ADC_CHANNEL_1, ADC_CHANNEL_2, ...),
  *         must not be a value from functions where a channel number is
  *         returned from ADC registers,
  *         because internal and external channels share the same channel
  *         number in ADC registers. The differentiation is made only with
  *         parameters definitions of driver.
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref ADC_CHANNEL_0
  *         @arg @ref ADC_CHANNEL_1            (7)
  *         @arg @ref ADC_CHANNEL_2            (7)
  *         @arg @ref ADC_CHANNEL_3            (7)
  *         @arg @ref ADC_CHANNEL_4            (7)
  *         @arg @ref ADC_CHANNEL_5            (7)
  *         @arg @ref ADC_CHANNEL_6
  *         @arg @ref ADC_CHANNEL_7
  *         @arg @ref ADC_CHANNEL_8
  *         @arg @ref ADC_CHANNEL_9
  *         @arg @ref ADC_CHANNEL_10
  *         @arg @ref ADC_CHANNEL_11
  *         @arg @ref ADC_CHANNEL_12
  *         @arg @ref ADC_CHANNEL_13
  *         @arg @ref ADC_CHANNEL_14
  *         @arg @ref ADC_CHANNEL_15
  *         @arg @ref ADC_CHANNEL_16
  *         @arg @ref ADC_CHANNEL_17
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT      (1)
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR   (4)
  *         @arg @ref ADC_CHANNEL_VBAT         (4)
  *         @arg @ref ADC_CHANNEL_DAC1CH1         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH2         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC3 (3)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC3 (3)(6)
  *         
  *         (1) On STM32L4, parameter available only on ADC instance: ADC1.\n
  *         (2) On STM32L4, parameter available only on ADC instance: ADC2.\n
  *         (3) On STM32L4, parameter available only on ADC instance: ADC3.\n
  *         (4) On STM32L4, parameter available only on ADC instances: ADC1, ADC3.\n
  *         (5) On STM32L4, parameter available on devices with only 1 ADC instance.\n
  *         (6) On STM32L4, parameter available on devices with several ADC instances.\n
  *         (7) On STM32L4, fast channel (0.188 us for 12-bit resolution (ADC conversion rate up to 5.33 Ms/s)).
  *             Other channels are slow channels (0.238 us for 12-bit resolution (ADC conversion rate up to 4.21 Ms/s)).
  * @retval Value "0" if the channel corresponds to a parameter definition of a ADC external channel (channel connected to a GPIO pin).
  *         Value "1" if the channel corresponds to a parameter definition of a ADC internal channel.
  */
#define __HAL_ADC_IS_CHANNEL_INTERNAL(__CHANNEL__)                             \
         __LL_ADC_IS_CHANNEL_INTERNAL(__CHANNEL__)

/**
  * @brief  Helper macro to convert a channel defined from parameter
  *         definition of a ADC internal channel (ADC_CHANNEL_VREFINT,
  *         ADC_CHANNEL_TEMPSENSOR, ...),
  *         to its equivalent parameter definition of a ADC external channel
  *         (ADC_CHANNEL_1, ADC_CHANNEL_2, ...).
  * @note   The channel parameter can be, additionally to a value
  *         defined from parameter definition of a ADC internal channel
  *         (ADC_CHANNEL_VREFINT, ADC_CHANNEL_TEMPSENSOR, ...),
  *         a value defined from parameter definition of
  *         ADC external channel (ADC_CHANNEL_1, ADC_CHANNEL_2, ...)
  *         or a value from functions where a channel number is returned
  *         from ADC registers.
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref ADC_CHANNEL_0
  *         @arg @ref ADC_CHANNEL_1            (7)
  *         @arg @ref ADC_CHANNEL_2            (7)
  *         @arg @ref ADC_CHANNEL_3            (7)
  *         @arg @ref ADC_CHANNEL_4            (7)
  *         @arg @ref ADC_CHANNEL_5            (7)
  *         @arg @ref ADC_CHANNEL_6
  *         @arg @ref ADC_CHANNEL_7
  *         @arg @ref ADC_CHANNEL_8
  *         @arg @ref ADC_CHANNEL_9
  *         @arg @ref ADC_CHANNEL_10
  *         @arg @ref ADC_CHANNEL_11
  *         @arg @ref ADC_CHANNEL_12
  *         @arg @ref ADC_CHANNEL_13
  *         @arg @ref ADC_CHANNEL_14
  *         @arg @ref ADC_CHANNEL_15
  *         @arg @ref ADC_CHANNEL_16
  *         @arg @ref ADC_CHANNEL_17
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT      (1)
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR   (4)
  *         @arg @ref ADC_CHANNEL_VBAT         (4)
  *         @arg @ref ADC_CHANNEL_DAC1CH1         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH2         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC3 (3)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC3 (3)(6)
  *         
  *         (1) On STM32L4, parameter available only on ADC instance: ADC1.\n
  *         (2) On STM32L4, parameter available only on ADC instance: ADC2.\n
  *         (3) On STM32L4, parameter available only on ADC instance: ADC3.\n
  *         (4) On STM32L4, parameter available only on ADC instances: ADC1, ADC3.\n
  *         (5) On STM32L4, parameter available on devices with only 1 ADC instance.\n
  *         (6) On STM32L4, parameter available on devices with several ADC instances.\n
  *         (7) On STM32L4, fast channel (0.188 us for 12-bit resolution (ADC conversion rate up to 5.33 Ms/s)).
  *             Other channels are slow channels (0.238 us for 12-bit resolution (ADC conversion rate up to 4.21 Ms/s)).
  * @retval Returned value can be one of the following values:
  *         @arg @ref ADC_CHANNEL_0
  *         @arg @ref ADC_CHANNEL_1
  *         @arg @ref ADC_CHANNEL_2
  *         @arg @ref ADC_CHANNEL_3
  *         @arg @ref ADC_CHANNEL_4
  *         @arg @ref ADC_CHANNEL_5
  *         @arg @ref ADC_CHANNEL_6
  *         @arg @ref ADC_CHANNEL_7
  *         @arg @ref ADC_CHANNEL_8
  *         @arg @ref ADC_CHANNEL_9
  *         @arg @ref ADC_CHANNEL_10
  *         @arg @ref ADC_CHANNEL_11
  *         @arg @ref ADC_CHANNEL_12
  *         @arg @ref ADC_CHANNEL_13
  *         @arg @ref ADC_CHANNEL_14
  *         @arg @ref ADC_CHANNEL_15
  *         @arg @ref ADC_CHANNEL_16
  *         @arg @ref ADC_CHANNEL_17
  *         @arg @ref ADC_CHANNEL_18
  */
#define __HAL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL(__CHANNEL__)                    \
         __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL(__CHANNEL__)

/**
  * @brief  Helper macro to determine whether the internal channel
  *         selected is available on the ADC instance selected.
  * @note   The channel parameter must be a value defined from parameter
  *         definition of a ADC internal channel (ADC_CHANNEL_VREFINT,
  *         ADC_CHANNEL_TEMPSENSOR, ...),
  *         must not be a value defined from parameter definition of
  *         ADC external channel (ADC_CHANNEL_1, ADC_CHANNEL_2, ...)
  *         or a value from functions where a channel number is
  *         returned from ADC registers,
  *         because internal and external channels share the same channel
  *         number in ADC registers. The differentiation is made only with
  *         parameters definitions of driver.
  * @param  __ADC_INSTANCE__ ADC instance
  * @param  __CHANNEL__ This parameter can be one of the following values:
  *         @arg @ref ADC_CHANNEL_VREFINT      (1)
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR   (4)
  *         @arg @ref ADC_CHANNEL_VBAT         (4)
  *         @arg @ref ADC_CHANNEL_DAC1CH1         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH2         (5)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC2 (2)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH1_ADC3 (3)(6)
  *         @arg @ref ADC_CHANNEL_DAC1CH2_ADC3 (3)(6)
  *         
  *         (1) On STM32L4, parameter available only on ADC instance: ADC1.\n
  *         (2) On STM32L4, parameter available only on ADC instance: ADC2.\n
  *         (3) On STM32L4, parameter available only on ADC instance: ADC3.\n
  *         (4) On STM32L4, parameter available only on ADC instances: ADC1, ADC3.\n
  *         (5) On STM32L4, parameter available on devices with only 1 ADC instance.\n
  *         (6) On STM32L4, parameter available on devices with several ADC instances.
  * @retval Value "0" if the internal channel selected is not available on the ADC instance selected.
  *         Value "1" if the internal channel selected is available on the ADC instance selected.
  */
#define __HAL_ADC_IS_CHANNEL_INTERNAL_AVAILABLE(__ADC_INSTANCE__, __CHANNEL__)  \
         __LL_ADC_IS_CHANNEL_INTERNAL_AVAILABLE(__ADC_INSTANCE__, __CHANNEL__)

#if defined(ADC_MULTIMODE_SUPPORT)
/**
  * @brief  Helper macro to get the ADC multimode conversion data of ADC master
  *         or ADC slave from raw value with both ADC conversion data concatenated.
  * @note   This macro is intended to be used when multimode transfer by DMA
  *         is enabled: refer to function @ref LL_ADC_SetMultiDMATransfer().
  *         In this case the transferred data need to processed with this macro
  *         to separate the conversion data of ADC master and ADC slave.
  * @param  __ADC_MULTI_MASTER_SLAVE__ This parameter can be one of the following values:
  *         @arg @ref LL_ADC_MULTI_MASTER
  *         @arg @ref LL_ADC_MULTI_SLAVE
  * @param  __ADC_MULTI_CONV_DATA__ Value between Min_Data=0x000 and Max_Data=0xFFF
  * @retval Value between Min_Data=0x000 and Max_Data=0xFFF
  */
#define __HAL_ADC_MULTI_CONV_DATA_MASTER_SLAVE(__ADC_MULTI_MASTER_SLAVE__, __ADC_MULTI_CONV_DATA__)  \
         __LL_ADC_MULTI_CONV_DATA_MASTER_SLAVE(__ADC_MULTI_MASTER_SLAVE__, __ADC_MULTI_CONV_DATA__)
#endif

/**
  * @brief  Helper macro to select the ADC common instance
  *         to which is belonging the selected ADC instance.
  * @note   ADC common register instance can be used for:
  *         - Set parameters common to several ADC instances
  *         - Multimode (for devices with several ADC instances)
  *         Refer to functions having argument "ADCxy_COMMON" as parameter.
  * @param  __ADCx__ ADC instance
  * @retval ADC common register instance
  */
#define __HAL_ADC_COMMON_INSTANCE(__ADCx__)                                    \
         __LL_ADC_COMMON_INSTANCE(__ADCx__)

/**
  * @brief  Helper macro to check if all ADC instances sharing the same
  *         ADC common instance are disabled.
  * @note   This check is required by functions with setting conditioned to
  *         ADC state:
  *         All ADC instances of the ADC common group must be disabled.
  *         Refer to functions having argument "ADCxy_COMMON" as parameter.
  * @note   On devices with only 1 ADC common instance, parameter of this macro
  *         is useless and can be ignored (parameter kept for compatibility
  *         with devices featuring several ADC common instances).
  * @param  __ADCXY_COMMON__ ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @retval Value "0" if all ADC instances sharing the same ADC common instance
  *         are disabled.
  *         Value "1" if at least one ADC instance sharing the same ADC common instance
  *         is enabled.
  */
#define __HAL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE(__ADCXY_COMMON__)              \
         __LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE(__ADCXY_COMMON__)

/**
  * @brief  Helper macro to define the ADC conversion data full-scale digital
  *         value corresponding to the selected ADC resolution.
  * @note   ADC conversion data full-scale corresponds to voltage range
  *         determined by analog voltage references Vref+ and Vref-
  *         (refer to reference manual).
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref ADC_RESOLUTION_12B
  *         @arg @ref ADC_RESOLUTION_10B
  *         @arg @ref ADC_RESOLUTION_8B
  *         @arg @ref ADC_RESOLUTION_6B
  * @retval ADC conversion data equivalent voltage value (unit: mVolt)
  */
#define __HAL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__)                             \
         __LL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__)

/**
  * @brief  Helper macro to convert the ADC conversion data from
  *         a resolution to another resolution.
  * @param  __DATA__ ADC conversion data to be converted 
  * @param  __ADC_RESOLUTION_CURRENT__ Resolution of to the data to be converted
  *         This parameter can be one of the following values:
  *         @arg @ref ADC_RESOLUTION_12B
  *         @arg @ref ADC_RESOLUTION_10B
  *         @arg @ref ADC_RESOLUTION_8B
  *         @arg @ref ADC_RESOLUTION_6B
  * @param  __ADC_RESOLUTION_TARGET__ Resolution of the data after conversion
  *         This parameter can be one of the following values:
  *         @arg @ref ADC_RESOLUTION_12B
  *         @arg @ref ADC_RESOLUTION_10B
  *         @arg @ref ADC_RESOLUTION_8B
  *         @arg @ref ADC_RESOLUTION_6B
  * @retval ADC conversion data to the requested resolution
  */
#define __HAL_ADC_CONVERT_DATA_RESOLUTION(__DATA__,\
                                          __ADC_RESOLUTION_CURRENT__,\
                                          __ADC_RESOLUTION_TARGET__)            \
         __LL_ADC_CONVERT_DATA_RESOLUTION(__DATA__,\
                                          __ADC_RESOLUTION_CURRENT__,\
                                          __ADC_RESOLUTION_TARGET__)

/**
  * @brief  Helper macro to calculate the voltage (unit: mVolt)
  *         corresponding to a ADC conversion data (unit: digital value).
  * @note   Analog reference voltage (Vref+) must be either known from
  *         user board environment or can be calculated using ADC measurement
  *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
  * @param  __VREFANALOG_VOLTAGE__ Analog reference voltage (unit: mV)
  * @param  __ADC_DATA__ ADC conversion data (resolution 12 bits)
  *                       (unit: digital value).
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref ADC_RESOLUTION_12B
  *         @arg @ref ADC_RESOLUTION_10B
  *         @arg @ref ADC_RESOLUTION_8B
  *         @arg @ref ADC_RESOLUTION_6B
  * @retval ADC conversion data equivalent voltage value (unit: mVolt)
  */
#define __HAL_ADC_CALC_DATA_TO_VOLTAGE(__VREFANALOG_VOLTAGE__,\
                                       __ADC_DATA__,\
                                       __ADC_RESOLUTION__)                     \
         __LL_ADC_CALC_DATA_TO_VOLTAGE(__VREFANALOG_VOLTAGE__,\
                                       __ADC_DATA__,\
                                       __ADC_RESOLUTION__)

/**
  * @brief  Helper macro to calculate analog reference voltage (Vref+)
  *         (unit: mVolt) from ADC conversion data of internal voltage
  *         reference VrefInt.
  * @note   Computation is using VrefInt calibration value
  *         stored in system memory for each device during production.
  * @note   This voltage depends on user board environment: voltage level
  *         connected to pin Vref+.
  *         On devices with small package, the pin Vref+ is not present
  *         and internally bonded to pin Vdda.
  * @note   On this STM32 serie, calibration data of internal voltage reference
  *         VrefInt corresponds to a resolution of 12 bits,
  *         this is the recommended ADC resolution to convert voltage of
  *         internal voltage reference VrefInt.
  *         Otherwise, this macro performs the processing to scale
  *         ADC conversion data to 12 bits.
  * @param  __VREFINT_ADC_DATA__ ADC conversion data (resolution 12 bits)
  *         of internal voltage reference VrefInt (unit: digital value).
  * @param  __ADC_RESOLUTION__ This parameter can be one of the following values:
  *         @arg @ref ADC_RESOLUTION_12B
  *         @arg @ref ADC_RESOLUTION_10B
  *         @arg @ref ADC_RESOLUTION_8B
  *         @arg @ref ADC_RESOLUTION_6B
  * @retval Analog reference voltage (unit: mV)
  */
#define __HAL_ADC_CALC_VREFANALOG_VOLTAGE(__VREFINT_ADC_DATA__,\
                                          __ADC_RESOLUTION__)                  \
         __LL_ADC_CALC_VREFANALOG_VOLTAGE(__VREFINT_ADC_DATA__,\
                                          __ADC_RESOLUTION__)

/**
  * @brief  Helper macro to calculate the temperature (unit: degree Celsius)
  *         from ADC conversion data of internal temperature sensor.
  * @note   Computation is using temperature sensor calibration values
  *         stored in system memory for each device during production.
  * @note   Calculation formula:
  *           Temperature = ((TS_ADC_DATA - TS_CAL1)
  *                           * (TS_CAL2_TEMP - TS_CAL1_TEMP))
  *                         / (TS_CAL2 - TS_CAL1) + TS_CAL1_TEMP
  *           with TS_ADC_DATA = temperature sensor raw data measured by ADC
  *                Avg_Slope = (TS_CAL2 - TS_CAL1)
  *                            / (TS_CAL2_TEMP - TS_CAL1_TEMP)
  *                TS_CAL1   = equivalent TS_ADC_DATA at temperature
  *                            TEMP_DEGC_CAL1 (calibrated in factory)
  *                TS_CAL2   = equivalent TS_ADC_DATA at temperature
  *                            TEMP_DEGC_CAL2 (calibrated in factory)
  *         Caution: Calculation relevancy under reserve that calibration
  *                  parameters are correct (address and data).
  *                  To calculate temperature using temperature sensor
  *                  datasheet typical values (generic values less, therefore
  *                  less accurate than calibrated values),
  *                  use helper macro @ref __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS().
  * @note   As calculation input, the analog reference voltage (Vref+) must be
  *         defined as it impacts the ADC LSB equivalent voltage.
  * @note   Analog reference voltage (Vref+) must be either known from
  *         user board environment or can be calculated using ADC measurement
  *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
  * @note   On this STM32 serie, calibration data of temperature sensor
  *         corresponds to a resolution of 12 bits,
  *         this is the recommended ADC resolution to convert voltage of
  *         temperature sensor.
  *         Otherwise, this macro performs the processing to scale
  *         ADC conversion data to 12 bits.
  * @param  __VREFANALOG_VOLTAGE__  Analog reference voltage (unit: mV)
  * @param  __TEMPSENSOR_ADC_DATA__ ADC conversion data of internal
  *                                 temperature sensor (unit: digital value).
  * @param  __ADC_RESOLUTION__      ADC resolution at which internal temperature
  *                                 sensor voltage has been measured.
  *         This parameter can be one of the following values:
  *         @arg @ref ADC_RESOLUTION_12B
  *         @arg @ref ADC_RESOLUTION_10B
  *         @arg @ref ADC_RESOLUTION_8B
  *         @arg @ref ADC_RESOLUTION_6B
  * @retval Temperature (unit: degree Celsius)
  */
#define __HAL_ADC_CALC_TEMPERATURE(__VREFANALOG_VOLTAGE__,\
                                   __TEMPSENSOR_ADC_DATA__,\
                                   __ADC_RESOLUTION__)                         \
         __LL_ADC_CALC_TEMPERATURE(__VREFANALOG_VOLTAGE__,\
                                   __TEMPSENSOR_ADC_DATA__,\
                                   __ADC_RESOLUTION__)

/**
  * @brief  Helper macro to calculate the temperature (unit: degree Celsius)
  *         from ADC conversion data of internal temperature sensor.
  * @note   Computation is using temperature sensor typical values
  *         (refer to device datasheet).
  * @note   Calculation formula:
  *           Temperature = (TS_TYP_CALx_VOLT(uV) - TS_ADC_DATA * Conversion_uV)
  *                         / Avg_Slope + CALx_TEMP
  *           with TS_ADC_DATA      = temperature sensor raw data measured by ADC
  *                                   (unit: digital value)
  *                Avg_Slope        = temperature sensor slope
  *                                   (unit: uV/Degree Celsius)
  *                TS_TYP_CALx_VOLT = temperature sensor digital value at
  *                                   temperature CALx_TEMP (unit: mV)
  *         Caution: Calculation relevancy under reserve the temperature sensor
  *                  of the current device has characteristics in line with
  *                  datasheet typical values.
  *                  If temperature sensor calibration values are available on
  *                  on this device (presence of macro __LL_ADC_CALC_TEMPERATURE()),
  *                  temperature calculation will be more accurate using
  *                  helper macro @ref __LL_ADC_CALC_TEMPERATURE().
  * @note   As calculation input, the analog reference voltage (Vref+) must be
  *         defined as it impacts the ADC LSB equivalent voltage.
  * @note   Analog reference voltage (Vref+) must be either known from
  *         user board environment or can be calculated using ADC measurement
  *         and ADC helper macro @ref __LL_ADC_CALC_VREFANALOG_VOLTAGE().
  * @note   ADC measurement data must correspond to a resolution of 12bits
  *         (full scale digital value 4095). If not the case, the data must be
  *         preliminarily rescaled to an equivalent resolution of 12 bits.
  * @param  __TEMPSENSOR_TYP_AVGSLOPE__   Device datasheet data: Temperature sensor slope typical value (unit: uV/DegCelsius).
  *                                       On STM32L4, refer to device datasheet parameter "Avg_Slope".
  * @param  __TEMPSENSOR_TYP_CALX_V__     Device datasheet data: Temperature sensor voltage typical value (at temperature and Vref+ defined in parameters below) (unit: mV).
  *                                       On STM32L4, refer to device datasheet parameter "V30" (corresponding to TS_CAL1).
  * @param  __TEMPSENSOR_CALX_TEMP__      Device datasheet data: Temperature at which temperature sensor voltage (see parameter above) is corresponding (unit: mV)
  * @param  __VREFANALOG_VOLTAGE__        Analog voltage reference (Vref+) voltage (unit: mV)
  * @param  __TEMPSENSOR_ADC_DATA__       ADC conversion data of internal temperature sensor (unit: digital value).
  * @param  __ADC_RESOLUTION__            ADC resolution at which internal temperature sensor voltage has been measured.
  *         This parameter can be one of the following values:
  *         @arg @ref ADC_RESOLUTION_12B
  *         @arg @ref ADC_RESOLUTION_10B
  *         @arg @ref ADC_RESOLUTION_8B
  *         @arg @ref ADC_RESOLUTION_6B
  * @retval Temperature (unit: degree Celsius)
  */
#define __HAL_ADC_CALC_TEMPERATURE_TYP_PARAMS(__TEMPSENSOR_TYP_AVGSLOPE__,\
                                              __TEMPSENSOR_TYP_CALX_V__,\
                                              __TEMPSENSOR_CALX_TEMP__,\
                                              __VREFANALOG_VOLTAGE__,\
                                              __TEMPSENSOR_ADC_DATA__,\
                                              __ADC_RESOLUTION__)              \
         __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(__TEMPSENSOR_TYP_AVGSLOPE__,\
                                              __TEMPSENSOR_TYP_CALX_V__,\
                                              __TEMPSENSOR_CALX_TEMP__,\
                                              __VREFANALOG_VOLTAGE__,\
                                              __TEMPSENSOR_ADC_DATA__,\
                                              __ADC_RESOLUTION__)

/**
  * @}
  */

/**
  * @}
  */

/* Include ADC HAL Extended module */
#include "stm32l4xx_hal_adc_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup ADC_Exported_Functions
  * @{
  */

/** @addtogroup ADC_Exported_Functions_Group1
  * @brief    Initialization and Configuration functions
  * @{
  */
/* Initialization and de-initialization functions  ****************************/
HAL_StatusTypeDef       HAL_ADC_Init(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef       HAL_ADC_DeInit(ADC_HandleTypeDef *hadc);
void                    HAL_ADC_MspInit(ADC_HandleTypeDef* hadc);
void                    HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc);
/**
  * @}
  */

/** @addtogroup ADC_Exported_Functions_Group2
  * @brief    IO operation functions
  * @{
  */
/* IO operation functions  *****************************************************/

/* Blocking mode: Polling */
HAL_StatusTypeDef       HAL_ADC_Start(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef       HAL_ADC_Stop(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef       HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);
HAL_StatusTypeDef       HAL_ADC_PollForEvent(ADC_HandleTypeDef* hadc, uint32_t EventType, uint32_t Timeout);

/* Non-blocking mode: Interruption */
HAL_StatusTypeDef       HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef       HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc);

/* Non-blocking mode: DMA */
HAL_StatusTypeDef       HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length);
HAL_StatusTypeDef       HAL_ADC_Stop_DMA(ADC_HandleTypeDef* hadc);

/* ADC retrieve conversion value intended to be used with polling or interruption */
uint32_t                HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);

/* ADC IRQHandler and Callbacks used in non-blocking modes (Interruption and DMA) */
void                    HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc);
void                    HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void                    HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);
void                    HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc);
void                    HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc);
/**
  * @}
  */

/** @addtogroup ADC_Exported_Functions_Group3 Peripheral Control functions
 *  @brief    Peripheral Control functions 
 * @{
 */
/* Peripheral Control functions ***********************************************/
HAL_StatusTypeDef       HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig);
HAL_StatusTypeDef       HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig);

/**
  * @}
  */

/* Peripheral State functions *************************************************/
/** @addtogroup ADC_Exported_Functions_Group4
  * @{
  */
uint32_t                HAL_ADC_GetState(ADC_HandleTypeDef* hadc);
uint32_t                HAL_ADC_GetError(ADC_HandleTypeDef *hadc);

/**
  * @}
  */

/**
  * @}
  */

/* Private functions -----------------------------------------------------------*/
/** @addtogroup ADC_Private_Functions ADC Private Functions
  * @{
  */
HAL_StatusTypeDef ADC_ConversionStop(ADC_HandleTypeDef* hadc, uint32_t ConversionGroup);
HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef ADC_Disable(ADC_HandleTypeDef* hadc);
void ADC_DMAConvCplt(DMA_HandleTypeDef *hdma);
void ADC_DMAHalfConvCplt(DMA_HandleTypeDef *hdma);
void ADC_DMAError(DMA_HandleTypeDef *hdma);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif


#endif /* __STM32L4xx_HAL_ADC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
