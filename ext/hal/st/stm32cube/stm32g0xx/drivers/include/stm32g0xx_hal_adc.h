/**
  ******************************************************************************
  * @file    stm32g0xx_hal_adc.h
  * @author  MCD Application Team
  * @brief   Header file of ADC HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32G0xx_HAL_ADC_H
#define STM32G0xx_HAL_ADC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/* Include low level driver */
#include "stm32g0xx_ll_adc.h"

/** @addtogroup STM32G0xx_HAL_Driver
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
                                               This parameter can be a value of @ref ADC_HAL_EC_OVS_RATIO */

  uint32_t RightBitShift;                 /*!< Configures the division coefficient for the Oversampler.
                                               This parameter can be a value of @ref ADC_HAL_EC_OVS_SHIFT */

  uint32_t TriggeredMode;                 /*!< Selects the regular triggered oversampling mode.
                                               This parameter can be a value of @ref ADC_HAL_EC_OVS_DISCONT_MODE */

}ADC_OversamplingTypeDef;

/**
  * @brief  Structure definition of ADC instance and ADC group regular.
  * @note   Parameters of this structure are shared within 2 scopes:
  *          - Scope entire ADC (differentiation done for compatibility with some other STM32 series featuring ADC groups regular and injected): ClockPrescaler, Resolution, DataAlign,
  *            ScanConvMode, EOCSelection, LowPowerAutoWait.
  *          - Scope ADC group regular: ContinuousConvMode, NbrOfConversion, DiscontinuousConvMode,
  *            ExternalTrigConv, ExternalTrigConvEdge, DMAContinuousRequests, Overrun, OversamplingMode, Oversampling.
  * @note   The setting of these parameters by function HAL_ADC_Init() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled
  *          - For all parameters except 'ClockPrescaler' and 'Resolution': ADC enabled without conversion on going on group regular.
  *         If ADC is not in the appropriate state to modify some parameters, these parameters setting is bypassed
  *         without error reporting (as it can be the expected behavior in case of intended action to update another parameter 
  *         (which fulfills the ADC state condition) on the fly).
  */
typedef struct
{
  uint32_t ClockPrescaler;        /*!< Select ADC clock source (synchronous clock derived from APB clock or asynchronous clock derived from system clock or PLL (Refer to reference manual for list of clocks available)) and clock prescaler.
                                       This parameter can be a value of @ref ADC_HAL_EC_COMMON_CLOCK_SOURCE.
                                       Note: The ADC clock configuration is common to all ADC instances.
                                       Note: In case of synchronous clock mode based on HCLK/1, the configuration must be enabled only
                                             if the system clock has a 50% duty clock cycle (APB prescaler configured inside RCC 
                                             must be bypassed and PCLK clock must have 50% duty cycle). Refer to reference manual for details.
                                       Note: In case of usage of asynchronous clock, the selected clock must be preliminarily enabled at RCC top level.
                                       Note: This parameter can be modified only if all ADC instances are disabled. */

  uint32_t Resolution;            /*!< Configure the ADC resolution. 
                                       This parameter can be a value of @ref ADC_HAL_EC_RESOLUTION */

  uint32_t DataAlign;             /*!< Specify ADC data alignment in conversion data register (right or left).
                                       Refer to reference manual for alignments formats versus resolutions.
                                       This parameter can be a value of @ref ADC_HAL_EC_DATA_ALIGN */

  uint32_t ScanConvMode;          /*!< Configure the sequencer of ADC group regular.
                                       On this STM32 serie, ADC group regular sequencer both modes "fully configurable" or "not fully configurable" are
                                       available:
                                        - sequencer configured to fully configurable:
                                          sequencer length and each rank affectation to a channel are configurable.
                                           - Sequence length: Set number of ranks in the scan sequence.
                                           - Sequence direction: Unless specified in parameters, sequencer
                                             scan direction is forward (from rank 1 to rank n).
                                        - sequencer configured to not fully configurable:
                                          sequencer length and each rank affectation to a channel are fixed by channel HW number.
                                           - Sequence length: Number of ranks in the scan sequence is
                                             defined by number of channels set in the sequence,
                                             rank of each channel is fixed by channel HW number.
                                             (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
                                           - Sequence direction: Unless specified in parameters, sequencer
                                             scan direction is forward (from lowest channel number to
                                             highest channel number).
                                       This parameter can be associated to parameter 'DiscontinuousConvMode' to have main sequence subdivided in successive parts.
                                       Sequencer is automatically enabled if several channels are set (sequencer cannot be disabled, as it can be the case on other STM32 devices):
                                       If only 1 channel is set: Conversion is performed in single mode.
                                       If several channels are set:  Conversions are performed in sequence mode.
                                       This parameter can be a value of @ref ADC_Scan_mode */

  uint32_t EOCSelection;          /*!< Specify which EOC (End Of Conversion) flag is used for conversion by polling and interruption: end of unitary conversion or end of sequence conversions.
                                       This parameter can be a value of @ref ADC_EOCSelection. */

  FunctionalState LowPowerAutoWait; /*!< Select the dynamic low power Auto Delay: new conversion start only when the previous
                                       conversion (for ADC group regular) has been retrieved by user software,
                                       using function HAL_ADC_GetValue().
                                       This feature automatically adapts the frequency of ADC conversions triggers to the speed of the system that reads the data. Moreover, this avoids risk of overrun
                                       for low frequency applications. 
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: Do not use with interruption or DMA (HAL_ADC_Start_IT(), HAL_ADC_Start_DMA()) since they clear immediately the EOC flag
                                             to free the IRQ vector sequencer.
                                             Do use with polling: 1. Start conversion with HAL_ADC_Start(), 2. Later on, when ADC conversion data is needed:
                                             use HAL_ADC_PollForConversion() to ensure that conversion is completed and HAL_ADC_GetValue() to retrieve conversion result and trig another conversion start. */

  FunctionalState LowPowerAutoPowerOff; /*!< Select the auto-off mode: the ADC automatically powers-off after a conversion and automatically wakes-up when a new conversion is triggered (with startup time between trigger and start of sampling).
                                       This feature can be combined with automatic wait mode (parameter 'LowPowerAutoWait').
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: If enabled, this feature also turns off the ADC dedicated 14 MHz RC oscillator (HSI14) */

  FunctionalState ContinuousConvMode; /*!< Specify whether the conversion is performed in single mode (one conversion) or continuous mode for ADC group regular,
                                       after the first ADC conversion start trigger occurred (software start or external trigger).
                                       This parameter can be set to ENABLE or DISABLE. */

  uint32_t NbrOfConversion;       /*!< Specify the number of ranks that will be converted within the regular group sequencer.
                                       This parameter is dependent on ScanConvMode:
                                        - sequencer configured to fully configurable:
                                          Number of ranks in the scan sequence is configurable using this parameter.
                                          Note: After the first call of 'HAL_ADC_Init()', each rank corresponding to parameter "NbrOfConversion" must be set using 'HAL_ADC_ConfigChannel()'.
                                                Afterwards, when all needed sequencer ranks are set, parameter 'NbrOfConversion' can be updated without modifying configuration of sequencer ranks
                                                (sequencer ranks above 'NbrOfConversion' are discarded).
                                        - sequencer configured to not fully configurable:
                                          Number of ranks in the scan sequence is defined by number of channels set in the sequence. This parameter is discarded.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 8.
                                       Note: This parameter must be modified when no conversion is on going on regular group (ADC disabled, or ADC enabled without continuous mode or external trigger that could launch a conversion). */

  FunctionalState DiscontinuousConvMode; /*!< Specify whether the conversions sequence of ADC group regular is performed in Complete-sequence/Discontinuous-sequence
                                       (main sequence subdivided in successive parts).
                                       Discontinuous mode is used only if sequencer is enabled (parameter 'ScanConvMode'). If sequencer is disabled, this parameter is discarded.
                                       Discontinuous mode can be enabled only if continuous mode is disabled. If continuous mode is enabled, this parameter setting is discarded.
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: On this STM32 serie, ADC group regular number of discontinuous ranks increment is fixed to one-by-one. */

  uint32_t ExternalTrigConv;      /*!< Select the external event source used to trigger ADC group regular conversion start.
                                       If set to ADC_SOFTWARE_START, external triggers are disabled and software trigger is used instead.
                                       This parameter can be a value of @ref ADC_regular_external_trigger_source.
                                       Caution: external trigger source is common to all ADC instances. */
                                                                                                        
  uint32_t ExternalTrigConvEdge;  /*!< Select the external event edge used to trigger ADC group regular conversion start.
                                       If trigger source is set to ADC_SOFTWARE_START, this parameter is discarded.
                                       This parameter can be a value of @ref ADC_regular_external_trigger_edge */

  FunctionalState DMAContinuousRequests; /*!< Specify whether the DMA requests are performed in one shot mode (DMA transfer stops when number of conversions is reached)
                                       or in continuous mode (DMA transfer unlimited, whatever number of conversions).
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: In continuous mode, DMA must be configured in circular mode. Otherwise an overrun will be triggered when DMA buffer maximum pointer is reached. */

  uint32_t Overrun;               /*!< Select the behavior in case of overrun: data overwritten or preserved (default).
                                       This parameter can be a value of @ref ADC_HAL_EC_REG_OVR_DATA_BEHAVIOR.
                                       Note: In case of overrun set to data preserved and usage with programming model with interruption (HAL_Start_IT()): ADC IRQ handler has to clear 
                                       end of conversion flags, this induces the release of the preserved data. If needed, this data can be saved in function 
                                       HAL_ADC_ConvCpltCallback(), placed in user program code (called before end of conversion flags clear).
                                       Note: Error reporting with respect to the conversion mode:
                                             - Usage with ADC conversion by polling for event or interruption: Error is reported only if overrun is set to data preserved. If overrun is set to data 
                                               overwritten, user can willingly not read all the converted data, this is not considered as an erroneous case.
                                             - Usage with ADC conversion by DMA: Error is reported whatever overrun setting (DMA is expected to process all data from data register). */

  uint32_t SamplingTimeCommon1;   /*!< Set sampling time common to a group of channels.
                                       Unit: ADC clock cycles
                                       Conversion time is the addition of sampling time and processing time (12.5 ADC clock cycles at ADC resolution 12 bits, 10.5 cycles at 10 bits, 8.5 cycles at 8 bits, 6.5 cycles at 6 bits).
                                       Note: On this STM32 family, two different sampling time settings are available, each channel can use one of these two settings. On some other STM32 devices, this parameter in channel wise and is located into ADC channel initialization structure.
                                       This parameter can be a value of @ref ADC_HAL_EC_CHANNEL_SAMPLINGTIME
                                       Note: In case of usage of internal measurement channels (VrefInt/Vbat/TempSensor),
                                             sampling time constraints must be respected (sampling time can be adjusted in function of ADC clock frequency and sampling time setting)
                                             Refer to device datasheet for timings values, parameters TS_vrefint, TS_vbat, TS_temp (values rough order: few tens of microseconds). */

  uint32_t SamplingTimeCommon2;   /*!< Set sampling time common to a group of channels, second common setting possible.
                                       Unit: ADC clock cycles
                                       Conversion time is the addition of sampling time and processing time (12.5 ADC clock cycles at ADC resolution 12 bits, 10.5 cycles at 10 bits, 8.5 cycles at 8 bits, 6.5 cycles at 6 bits).
                                       Note: On this STM32 family, two different sampling time settings are available, each channel can use one of these two settings. On some other STM32 devices, this parameter in channel wise and is located into ADC channel initialization structure.
                                       This parameter can be a value of @ref ADC_HAL_EC_CHANNEL_SAMPLINGTIME
                                       Note: In case of usage of internal measurement channels (VrefInt/Vbat/TempSensor),
                                             sampling time constraints must be respected (sampling time can be adjusted in function of ADC clock frequency and sampling time setting)
                                             Refer to device datasheet for timings values, parameters TS_vrefint, TS_vbat, TS_temp (values rough order: few tens of microseconds). */

  FunctionalState OversamplingMode;       /*!< Specify whether the oversampling feature is enabled or disabled.
                                               This parameter can be set to ENABLE or DISABLE.
                                               Note: This parameter can be modified only if there is no conversion is ongoing on ADC group regular. */

  ADC_OversamplingTypeDef Oversampling;   /*!< Specify the Oversampling parameters.
                                               Caution: this setting overwrites the previous oversampling configuration if oversampling is already enabled. */

  uint32_t TriggerFrequencyMode;  /*!< Set ADC trigger frequency mode.
                                       This parameter can be a value of @ref ADC_HAL_EC_REG_TRIGGER_FREQ.
                                       Note: ADC trigger frequency mode must be set to low frequency when
                                             a duration is exceeded before ADC conversion start trigger event
                                             (between ADC enable and ADC conversion start trigger event
                                             or between two ADC conversion start trigger event).
                                             Duration value: Refer to device datasheet, parameter "tIdle".
                                       Note: When ADC trigger frequency mode is set to low frequency, 
                                             some rearm cycles are inserted before performing ADC conversion
                                             start, inducing a delay of 2 ADC clock cycles. */

}ADC_InitTypeDef;

/**
  * @brief  Structure definition of ADC channel for regular group
  * @note   The setting of these parameters by function HAL_ADC_ConfigChannel() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled or enabled without conversion on going on regular group.
  *         If ADC is not in the appropriate state to modify some parameters, these parameters setting is bypassed
  *         without error reporting (as it can be the expected behavior in case of intended action to update another parameter (which fulfills the ADC state condition)
  *         on the fly).
  */
typedef struct
{
  uint32_t Channel;                /*!< Specify the channel to configure into ADC regular group.
                                        This parameter can be a value of @ref ADC_HAL_EC_CHANNEL
                                        Note: Depending on devices and ADC instances, some channels may not be available on device package pins. Refer to device datasheet for channels availability. */

  uint32_t Rank;                   /*!< Add or remove the channel from ADC regular group sequencer and specify its conversion rank.
                                        This parameter is dependent on ScanConvMode:
                                        - sequencer configured to fully configurable:
                                          Channels ordering into each rank of scan sequence:
                                          whatever channel can be placed into whatever rank.
                                        - sequencer configured to not fully configurable:
                                          rank of each channel is fixed by channel HW number.
                                          (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
                                          Despite the channel rank is fixed, this parameter allow an additional possibility: to remove the selected rank (selected channel) from sequencer.
                                        This parameter can be a value of @ref ADC_HAL_EC_REG_SEQ_RANKS */

  uint32_t SamplingTime;           /*!< Sampling time value to be set for the selected channel.
                                        Unit: ADC clock cycles
                                        Conversion time is the addition of sampling time and processing time
                                        (12.5 ADC clock cycles at ADC resolution 12 bits, 10.5 cycles at 10 bits, 8.5 cycles at 8 bits, 6.5 cycles at 6 bits).
                                        This parameter can be a value of @ref ADC_HAL_EC_SAMPLINGTIME_COMMON
                                        Note: On this STM32 family, two different sampling time settings are available (refer to parameters "SamplingTimeCommon1" and "SamplingTimeCommon2"), each channel can use one of these two settings.

                                        Note: In case of usage of internal measurement channels (VrefInt/Vbat/TempSensor),
                                              sampling time constraints must be respected (sampling time can be adjusted in function of ADC clock frequency and sampling time setting)
                                              Refer to device datasheet for timings values. */

}ADC_ChannelConfTypeDef;

/**
  * @brief  Structure definition of ADC analog watchdog
  * @note   The setting of these parameters by function HAL_ADC_AnalogWDGConfig() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters except 'HighThreshold', 'LowThreshold': ADC disabled or ADC enabled without conversion on going on ADC groups regular.
  *          - For parameters 'HighThreshold', 'LowThreshold': ADC enabled with conversion on going on regular.
  */
typedef struct
{
  uint32_t WatchdogNumber;    /*!< Select which ADC analog watchdog is monitoring the selected channel.
                                   For Analog Watchdog 1: Only 1 channel can be monitored (or overall group of channels by setting parameter 'WatchdogMode')
                                   For Analog Watchdog 2 and 3: Several channels can be monitored (by successive calls of 'HAL_ADC_AnalogWDGConfig()' for each channel)
                                   This parameter can be a value of @ref ADC_HAL_EC_AWD_NUMBER. */

  uint32_t WatchdogMode;      /*!< Configure the ADC analog watchdog mode: single/all/none channels.
                                   For Analog Watchdog 1: Configure the ADC analog watchdog mode: single channel or all channels, ADC group regular.
                                   For Analog Watchdog 2 and 3: Several channels can be monitored by applying successively the AWD init structure.
                                   This parameter can be a value of @ref ADC_analog_watchdog_mode. */

  uint32_t Channel;           /*!< Select which ADC channel to monitor by analog watchdog.
                                   For Analog Watchdog 1: this parameter has an effect only if parameter 'WatchdogMode' is configured on single channel (only 1 channel can be monitored).
                                   For Analog Watchdog 2 and 3: Several channels can be monitored. To use this feature, call successively the function HAL_ADC_AnalogWDGConfig() for each channel to be added (or removed with value 'ADC_ANALOGWATCHDOG_NONE').
                                   This parameter can be a value of @ref ADC_HAL_EC_CHANNEL. */

  FunctionalState ITMode;     /*!< Specify whether the analog watchdog is configured in interrupt or polling mode.
                                   This parameter can be set to ENABLE or DISABLE */

  uint32_t HighThreshold;     /*!< Configure the ADC analog watchdog High threshold value.
                                   Depending of ADC resolution selected (12, 10, 8 or 6 bits), this parameter must be a number
                                   between Min_Data = 0x000 and Max_Data = 0xFFF, 0x3FF, 0xFF or 0x3F respectively.
                                   Note: Analog watchdog 2 and 3 are limited to a resolution of 8 bits: if ADC resolution is 12 bits 
                                         the 4 LSB are ignored, if ADC resolution is 10 bits the 2 LSB are ignored.
                                   Note: If ADC oversampling is enabled, ADC analog watchdog thresholds are
                                         impacted: the comparison of analog watchdog thresholds is done on
                                         oversampling final computation (after ratio and shift application):
                                         ADC data register bitfield [15:4] (12 most significant bits). */

  uint32_t LowThreshold;      /*!< Configures the ADC analog watchdog Low threshold value.
                                   Depending of ADC resolution selected (12, 10, 8 or 6 bits), this parameter must be a number
                                   between Min_Data = 0x000 and Max_Data = 0xFFF, 0x3FF, 0xFF or 0x3F respectively.
                                   Note: Analog watchdog 2 and 3 are limited to a resolution of 8 bits: if ADC resolution is 12 bits 
                                         the 4 LSB are ignored, if ADC resolution is 10 bits the 2 LSB are ignored.
                                   Note: If ADC oversampling is enabled, ADC analog watchdog thresholds are
                                         impacted: the comparison of analog watchdog thresholds is done on
                                         oversampling final computation (after ratio and shift application):
                                         ADC data register bitfield [15:4] (12 most significant bits). */
}ADC_AnalogWDGConfTypeDef;

/** @defgroup ADC_States ADC States
  * @{
  */

/**
  * @brief  HAL ADC state machine: ADC states definition (bitfields)
  * @note   ADC state machine is managed by bitfields, state must be compared
  *         with bit by bit.
  *         For example:                                                         
  *           " if ((HAL_ADC_GetState(hadc1) & HAL_ADC_STATE_REG_BUSY) != 0UL) "
  *           " if ((HAL_ADC_GetState(hadc1) & HAL_ADC_STATE_AWD1) != 0UL) "
  */
/* States of ADC global scope */
#define HAL_ADC_STATE_RESET             (0x00000000UL)   /*!< ADC not yet initialized or disabled */
#define HAL_ADC_STATE_READY             (0x00000001UL)   /*!< ADC peripheral ready for use */
#define HAL_ADC_STATE_BUSY_INTERNAL     (0x00000002UL)   /*!< ADC is busy due to an internal process (initialization, calibration) */
#define HAL_ADC_STATE_TIMEOUT           (0x00000004UL)   /*!< TimeOut occurrence */

/* States of ADC errors */
#define HAL_ADC_STATE_ERROR_INTERNAL    (0x00000010UL)   /*!< Internal error occurrence */
#define HAL_ADC_STATE_ERROR_CONFIG      (0x00000020UL)   /*!< Configuration error occurrence */
#define HAL_ADC_STATE_ERROR_DMA         (0x00000040UL)   /*!< DMA error occurrence */

/* States of ADC group regular */
#define HAL_ADC_STATE_REG_BUSY          (0x00000100UL)   /*!< A conversion on ADC group regular is ongoing or can occur (either by continuous mode,
                                                              external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available)) */
#define HAL_ADC_STATE_REG_EOC           (0x00000200UL)   /*!< Conversion data available on group regular */
#define HAL_ADC_STATE_REG_OVR           (0x00000400UL)   /*!< Overrun occurrence */
#define HAL_ADC_STATE_REG_EOSMP         (0x00000800UL)   /*!< Not available on this STM32 serie: End Of Sampling flag raised  */

/* States of ADC group injected */
#define HAL_ADC_STATE_INJ_BUSY          (0x00001000UL)  /*!< Not available on this STM32 serie: A conversion on group injected is ongoing or can occur (either by auto-injection mode,
                                                             external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available))*/
#define HAL_ADC_STATE_INJ_EOC           (0x00002000UL)  /*!< Not available on this STM32 serie: Conversion data available on group injected */
#define HAL_ADC_STATE_INJ_JQOVF         (0x00004000UL)  /*!< Not available on this STM32 serie: Injected queue overflow occurrence */

/* States of ADC analog watchdogs */
#define HAL_ADC_STATE_AWD1              (0x00010000UL)   /*!< Out-of-window occurrence of ADC analog watchdog 1 */
#define HAL_ADC_STATE_AWD2              (0x00020000UL)   /*!< Out-of-window occurrence of ADC analog watchdog 2 */
#define HAL_ADC_STATE_AWD3              (0x00040000UL)   /*!< Out-of-window occurrence of ADC analog watchdog 3 */

/* States of ADC multi-mode */
#define HAL_ADC_STATE_MULTIMODE_SLAVE   (0x00100000UL)   /*!< Not available on this STM32 serie: ADC in multimode slave state, controlled by another ADC master (when feature available) */


/**
  * @}
  */

/**
  * @brief  ADC handle Structure definition
  */
typedef struct __ADC_HandleTypeDef
{
  ADC_TypeDef                   *Instance;              /*!< Register base address */
  ADC_InitTypeDef               Init;                   /*!< ADC initialization parameters and regular conversions setting */
  DMA_HandleTypeDef             *DMA_Handle;            /*!< Pointer DMA Handler */
  HAL_LockTypeDef               Lock;                   /*!< ADC locking object */
  __IO uint32_t                 State;                  /*!< ADC communication state (bitmap of ADC states) */
  __IO uint32_t                 ErrorCode;              /*!< ADC Error code */

  uint32_t                      ADCGroupRegularSequencerRanks; /*!< ADC group regular sequencer memorization of ranks setting, used in mode "fully configurable" (refer to parameter 'ScanConvMode') */
#if (USE_HAL_ADC_REGISTER_CALLBACKS == 1)
  void (* ConvCpltCallback)(struct __ADC_HandleTypeDef *hadc);              /*!< ADC conversion complete callback */
  void (* ConvHalfCpltCallback)(struct __ADC_HandleTypeDef *hadc);          /*!< ADC conversion DMA half-transfer callback */
  void (* LevelOutOfWindowCallback)(struct __ADC_HandleTypeDef *hadc);      /*!< ADC analog watchdog 1 callback */
  void (* ErrorCallback)(struct __ADC_HandleTypeDef *hadc);                 /*!< ADC error callback */
  void (* LevelOutOfWindow2Callback)(struct __ADC_HandleTypeDef *hadc);     /*!< ADC analog watchdog 2 callback */
  void (* LevelOutOfWindow3Callback)(struct __ADC_HandleTypeDef *hadc);     /*!< ADC analog watchdog 3 callback */
  void (* EndOfSamplingCallback)(struct __ADC_HandleTypeDef *hadc);         /*!< ADC end of sampling callback */
  void (* MspInitCallback)(struct __ADC_HandleTypeDef *hadc);               /*!< ADC Msp Init callback */
  void (* MspDeInitCallback)(struct __ADC_HandleTypeDef *hadc);             /*!< ADC Msp DeInit callback */
#endif /* USE_HAL_ADC_REGISTER_CALLBACKS */
}ADC_HandleTypeDef;

#if (USE_HAL_ADC_REGISTER_CALLBACKS == 1)
/**
  * @brief  HAL ADC Callback ID enumeration definition
  */
typedef enum
{
  HAL_ADC_CONVERSION_COMPLETE_CB_ID     = 0x00U,  /*!< ADC conversion complete callback ID */
  HAL_ADC_CONVERSION_HALF_CB_ID         = 0x01U,  /*!< ADC conversion DMA half-transfer callback ID */
  HAL_ADC_LEVEL_OUT_OF_WINDOW_1_CB_ID   = 0x02U,  /*!< ADC analog watchdog 1 callback ID */
  HAL_ADC_ERROR_CB_ID                   = 0x03U,  /*!< ADC error callback ID */
  HAL_ADC_LEVEL_OUT_OF_WINDOW_2_CB_ID   = 0x06U,  /*!< ADC analog watchdog 2 callback ID */
  HAL_ADC_LEVEL_OUT_OF_WINDOW_3_CB_ID   = 0x07U,  /*!< ADC analog watchdog 3 callback ID */
  HAL_ADC_END_OF_SAMPLING_CB_ID         = 0x08U,  /*!< ADC end of sampling callback ID */
  HAL_ADC_MSPINIT_CB_ID                 = 0x09U,  /*!< ADC Msp Init callback ID          */
  HAL_ADC_MSPDEINIT_CB_ID               = 0x0AU   /*!< ADC Msp DeInit callback ID        */
} HAL_ADC_CallbackIDTypeDef;

/**
  * @brief  HAL ADC Callback pointer definition
  */
typedef  void (*pADC_CallbackTypeDef)(ADC_HandleTypeDef *hadc); /*!< pointer to a ADC callback function */

#endif /* USE_HAL_ADC_REGISTER_CALLBACKS */

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
#define HAL_ADC_ERROR_NONE              (0x00U)   /*!< No error                                    */
#define HAL_ADC_ERROR_INTERNAL          (0x01U)   /*!< ADC IP internal error (problem of clocking,
                                                       enable/disable, erroneous state, ...)       */
#define HAL_ADC_ERROR_OVR               (0x02U)   /*!< Overrun error                               */
#define HAL_ADC_ERROR_DMA               (0x04U)   /*!< DMA transfer error                          */
#if (USE_HAL_ADC_REGISTER_CALLBACKS == 1)
#define HAL_ADC_ERROR_INVALID_CALLBACK  (0x10U)   /*!< Invalid Callback error */
#endif /* USE_HAL_ADC_REGISTER_CALLBACKS */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_COMMON_CLOCK_SOURCE  ADC common - Clock source
  * @{
  */
#define ADC_CLOCK_SYNC_PCLK_DIV1           (LL_ADC_CLOCK_SYNC_PCLK_DIV1)  /*!< ADC synchronous clock derived from AHB clock without prescaler. This configuration must be enabled only if PCLK has a 50% duty clock cycle (APB prescaler configured inside the RCC must be bypassed and the system clock must by 50% duty cycle) */
#define ADC_CLOCK_SYNC_PCLK_DIV2           (LL_ADC_CLOCK_SYNC_PCLK_DIV2)  /*!< ADC synchronous clock derived from AHB clock with prescaler division by 2 */
#define ADC_CLOCK_SYNC_PCLK_DIV4           (LL_ADC_CLOCK_SYNC_PCLK_DIV4)  /*!< ADC synchronous clock derived from AHB clock with prescaler division by 4 */

#define ADC_CLOCK_ASYNC_DIV1               (LL_ADC_CLOCK_ASYNC_DIV1)      /*!< ADC asynchronous clock without prescaler */
#define ADC_CLOCK_ASYNC_DIV2               (LL_ADC_CLOCK_ASYNC_DIV2)      /*!< ADC asynchronous clock with prescaler division by 2   */
#define ADC_CLOCK_ASYNC_DIV4               (LL_ADC_CLOCK_ASYNC_DIV4)      /*!< ADC asynchronous clock with prescaler division by 4   */
#define ADC_CLOCK_ASYNC_DIV6               (LL_ADC_CLOCK_ASYNC_DIV6)      /*!< ADC asynchronous clock with prescaler division by 6   */
#define ADC_CLOCK_ASYNC_DIV8               (LL_ADC_CLOCK_ASYNC_DIV8)      /*!< ADC asynchronous clock with prescaler division by 8   */
#define ADC_CLOCK_ASYNC_DIV10              (LL_ADC_CLOCK_ASYNC_DIV10)     /*!< ADC asynchronous clock with prescaler division by 10  */
#define ADC_CLOCK_ASYNC_DIV12              (LL_ADC_CLOCK_ASYNC_DIV12)     /*!< ADC asynchronous clock with prescaler division by 12  */
#define ADC_CLOCK_ASYNC_DIV16              (LL_ADC_CLOCK_ASYNC_DIV16)     /*!< ADC asynchronous clock with prescaler division by 16  */
#define ADC_CLOCK_ASYNC_DIV32              (LL_ADC_CLOCK_ASYNC_DIV32)     /*!< ADC asynchronous clock with prescaler division by 32  */
#define ADC_CLOCK_ASYNC_DIV64              (LL_ADC_CLOCK_ASYNC_DIV64)     /*!< ADC asynchronous clock with prescaler division by 64  */
#define ADC_CLOCK_ASYNC_DIV128             (LL_ADC_CLOCK_ASYNC_DIV128)    /*!< ADC asynchronous clock with prescaler division by 128 */
#define ADC_CLOCK_ASYNC_DIV256             (LL_ADC_CLOCK_ASYNC_DIV256)    /*!< ADC asynchronous clock with prescaler division by 256 */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_RESOLUTION  ADC instance - Resolution
  * @{
  */
#define ADC_RESOLUTION_12B                 (LL_ADC_RESOLUTION_12B)  /*!< ADC resolution 12 bits */
#define ADC_RESOLUTION_10B                 (LL_ADC_RESOLUTION_10B)  /*!< ADC resolution 10 bits */
#define ADC_RESOLUTION_8B                  (LL_ADC_RESOLUTION_8B)   /*!< ADC resolution  8 bits */
#define ADC_RESOLUTION_6B                  (LL_ADC_RESOLUTION_6B)   /*!< ADC resolution  6 bits */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_DATA_ALIGN ADC conversion data alignment
  * @{
  */
#define ADC_DATAALIGN_RIGHT                (LL_ADC_DATA_ALIGN_RIGHT)/*!< ADC conversion data alignment: right aligned (alignment on data register LSB bit 0)*/
#define ADC_DATAALIGN_LEFT                 (LL_ADC_DATA_ALIGN_LEFT)       /*!< ADC conversion data alignment: left aligned (aligment on data register MSB bit 15)*/
/**
  * @}
  */

/** @defgroup ADC_Scan_mode ADC sequencer scan mode
  * @{
  */
/* Note: On this STM32 family, ADC group regular sequencer both modes         */
/*       "fully configurable" or "not fully configurable" are                 */
/*       available.                                                           */
/*       Scan mode values must be compatible with other STM32 devices having  */
/*       a configurable sequencer.                                            */
/*       Scan direction setting values are defined by taking in account       */
/*       already defined values for other STM32 devices:                      */
/*         ADC_SCAN_DISABLE         (0x00000000UL)                            */
/*         ADC_SCAN_ENABLE          (0x00000001UL)                            */
/*       Sequencer fully configurable with only rank 1 enabled is considered  */
/*       as default setting equivalent to scan enable.                        */
/*       In case of migration from another STM32 device, the user will be     */
/*       warned of change of setting choices with assert check.               */
#define ADC_SCAN_DISABLE                  (0x00000000UL)                               /*!< Sequencer set to fully configurable: only the rank 1 is enabled (no scan sequence on several ranks) */
#define ADC_SCAN_ENABLE                   (ADC_CFGR1_CHSELRMOD)                        /*!< Sequencer set to fully configurable: sequencer length and each rank affectation to a channel are configurable. */

#define ADC_SCAN_SEQ_FIXED                (ADC_SCAN_SEQ_FIXED_INT)                     /*!< Sequencer set to not fully configurable: sequencer length and each rank affectation to a channel are fixed by channel HW number (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...). Scan direction forward: from channel 0 to channel 18 */
#define ADC_SCAN_SEQ_FIXED_BACKWARD       (ADC_SCAN_SEQ_FIXED_INT | ADC_CFGR1_SCANDIR) /*!< Sequencer set to not fully configurable: sequencer length and each rank affectation to a channel are fixed by channel HW number (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...). Scan direction backward: from channel 18 to channel 0 */

#define ADC_SCAN_DIRECTION_FORWARD        (ADC_SCAN_SEQ_FIXED)                   /* For compatibility with other STM32 devices */
#define ADC_SCAN_DIRECTION_BACKWARD       (ADC_SCAN_SEQ_FIXED_BACKWARD)          /* For compatibility with other STM32 devices */
/**
  * @}
  */

/** @defgroup ADC_regular_external_trigger_source ADC group regular trigger source
  * @{
  */
/* ADC group regular trigger sources for all ADC instances */
#define ADC_SOFTWARE_START            (LL_ADC_REG_TRIG_SOFTWARE)                 /*!< ADC group regular conversion trigger internal: SW start. */
#define ADC_EXTERNALTRIG_T1_TRGO2     (LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)           /*!< ADC group regular conversion trigger from external IP: TIM1 TRGO. Trigger edge set to rising edge (default setting). */
#define ADC_EXTERNALTRIG_T1_CC4       (LL_ADC_REG_TRIG_EXT_TIM1_CH4)             /*!< ADC group regular conversion trigger from external IP: TIM1 channel 4 event (capture compare: input capture or output capture). Trigger edge set to rising edge (default setting). */
#if defined(TIM2)
#define ADC_EXTERNALTRIG_T2_TRGO      (LL_ADC_REG_TRIG_EXT_TIM2_TRGO)            /*!< ADC group regular conversion trigger from external IP: TIM2 TRGO. Trigger edge set to rising edge (default setting). */
#endif
#define ADC_EXTERNALTRIG_T3_TRGO      (LL_ADC_REG_TRIG_EXT_TIM3_TRGO)            /*!< ADC group regular conversion trigger from external IP: TIM3 TRGO. Trigger edge set to rising edge (default setting). */
#if defined(TIM6)
#define ADC_EXTERNALTRIG_T6_TRGO      (LL_ADC_REG_TRIG_EXT_TIM6_TRGO)            /*!< ADC group regular conversion trigger from external IP: TIM6 TRGO. Trigger edge set to rising edge (default setting). */
#endif
#if defined(TIM15)
#define ADC_EXTERNALTRIG_T15_TRGO     (LL_ADC_REG_TRIG_EXT_TIM15_TRGO)           /*!< ADC group regular conversion trigger from external IP: TIM15 TRGO. Trigger edge set to rising edge (default setting). */
#endif
#define ADC_EXTERNALTRIG_EXT_IT11     (LL_ADC_REG_TRIG_EXT_EXTI_LINE11)          /*!< ADC group regular conversion trigger from external IP: external interrupt line 11. Trigger edge set to rising edge (default setting). */
/**
  * @}
  */

/** @defgroup ADC_regular_external_trigger_edge ADC group regular trigger edge (when external trigger is selected)
  * @{
  */
#define ADC_EXTERNALTRIGCONVEDGE_NONE           (0x00000000UL)                      /*!< Regular conversions hardware trigger detection disabled */
#define ADC_EXTERNALTRIGCONVEDGE_RISING         (LL_ADC_REG_TRIG_EXT_RISING)        /*!< ADC group regular conversion trigger polarity set to rising edge */
#define ADC_EXTERNALTRIGCONVEDGE_FALLING        (LL_ADC_REG_TRIG_EXT_FALLING)       /*!< ADC group regular conversion trigger polarity set to falling edge */
#define ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING  (LL_ADC_REG_TRIG_EXT_RISINGFALLING) /*!< ADC group regular conversion trigger polarity set to both rising and falling edges */
/**
  * @}
  */

/** @defgroup ADC_EOCSelection ADC sequencer end of unitary conversion or sequence conversions
  * @{
  */
#define ADC_EOC_SINGLE_CONV         (ADC_ISR_EOC)                 /*!< End of unitary conversion flag  */
#define ADC_EOC_SEQ_CONV            (ADC_ISR_EOS)                 /*!< End of sequence conversions flag    */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_REG_OVR_DATA_BEHAVIOR  ADC group regular - Overrun behavior on conversion data
  * @{
  */
#define ADC_OVR_DATA_PRESERVED             (LL_ADC_REG_OVR_DATA_PRESERVED)    /*!< ADC group regular behavior in case of overrun: data preserved */
#define ADC_OVR_DATA_OVERWRITTEN           (LL_ADC_REG_OVR_DATA_OVERWRITTEN)  /*!< ADC group regular behavior in case of overrun: data overwritten */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_REG_SEQ_RANKS  ADC group regular - Sequencer ranks
  * @{
  */
#define ADC_RANK_CHANNEL_NUMBER            (0x00000001U)  /*!< Setting relevant if parameter "ScanConvMode" is set to sequencer not fully configurable: Enable the rank of the selected channels. Number of ranks in the sequence is defined by number of channels enabled, rank of each channel is defined by channel number (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...) */
#define ADC_RANK_NONE                      (0x00000002U)  /*!< Setting relevant if parameter "ScanConvMode" is set to sequencer not fully configurable: Disable the selected rank (selected channel) from sequencer */

#define ADC_REGULAR_RANK_1                 (LL_ADC_REG_RANK_1)  /*!< ADC group regular sequencer rank 1 */
#define ADC_REGULAR_RANK_2                 (LL_ADC_REG_RANK_2)  /*!< ADC group regular sequencer rank 2 */
#define ADC_REGULAR_RANK_3                 (LL_ADC_REG_RANK_3)  /*!< ADC group regular sequencer rank 3 */
#define ADC_REGULAR_RANK_4                 (LL_ADC_REG_RANK_4)  /*!< ADC group regular sequencer rank 4 */
#define ADC_REGULAR_RANK_5                 (LL_ADC_REG_RANK_5)  /*!< ADC group regular sequencer rank 5 */
#define ADC_REGULAR_RANK_6                 (LL_ADC_REG_RANK_6)  /*!< ADC group regular sequencer rank 6 */
#define ADC_REGULAR_RANK_7                 (LL_ADC_REG_RANK_7)  /*!< ADC group regular sequencer rank 7 */
#define ADC_REGULAR_RANK_8                 (LL_ADC_REG_RANK_8)  /*!< ADC group regular sequencer rank 8 */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_SAMPLINGTIME_COMMON  ADC instance - Sampling time common to a group of channels
  * @{
  */
#define ADC_SAMPLINGTIME_COMMON_1          (LL_ADC_SAMPLINGTIME_COMMON_1) /*!< Set sampling time common to a group of channels: sampling time nb 1 */
#define ADC_SAMPLINGTIME_COMMON_2          (LL_ADC_SAMPLINGTIME_COMMON_2) /*!< Set sampling time common to a group of channels: sampling time nb 2 */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_CHANNEL_SAMPLINGTIME  Channel - Sampling time
  * @{
  */
#define ADC_SAMPLETIME_1CYCLE_5            (LL_ADC_SAMPLINGTIME_1CYCLE_5)     /*!< Sampling time 1.5 ADC clock cycle */
#define ADC_SAMPLETIME_3CYCLES_5           (LL_ADC_SAMPLINGTIME_3CYCLES_5)    /*!< Sampling time 3.5 ADC clock cycles */
#define ADC_SAMPLETIME_7CYCLES_5           (LL_ADC_SAMPLINGTIME_7CYCLES_5)    /*!< Sampling time 7.5 ADC clock cycles */
#define ADC_SAMPLETIME_12CYCLES_5          (LL_ADC_SAMPLINGTIME_12CYCLES_5)   /*!< Sampling time 12.5 ADC clock cycles */
#define ADC_SAMPLETIME_19CYCLES_5          (LL_ADC_SAMPLINGTIME_19CYCLES_5)   /*!< Sampling time 19.5 ADC clock cycles */
#define ADC_SAMPLETIME_39CYCLES_5          (LL_ADC_SAMPLINGTIME_39CYCLES_5)   /*!< Sampling time 39.5 ADC clock cycles */
#define ADC_SAMPLETIME_79CYCLES_5          (LL_ADC_SAMPLINGTIME_79CYCLES_5)   /*!< Sampling time 79.5 ADC clock cycles */
#define ADC_SAMPLETIME_160CYCLES_5         (LL_ADC_SAMPLINGTIME_160CYCLES_5)  /*!< Sampling time 160.5 ADC clock cycles */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_CHANNEL  ADC instance - Channel number
  * @{
  */
#define ADC_CHANNEL_0                      (LL_ADC_CHANNEL_0)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN0  */
#define ADC_CHANNEL_1                      (LL_ADC_CHANNEL_1)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN1  */
#define ADC_CHANNEL_2                      (LL_ADC_CHANNEL_2)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN2  */
#define ADC_CHANNEL_3                      (LL_ADC_CHANNEL_3)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN3  */
#define ADC_CHANNEL_4                      (LL_ADC_CHANNEL_4)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN4  */
#define ADC_CHANNEL_5                      (LL_ADC_CHANNEL_5)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN5  */
#define ADC_CHANNEL_6                      (LL_ADC_CHANNEL_6)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN6  */
#define ADC_CHANNEL_7                      (LL_ADC_CHANNEL_7)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN7  */
#define ADC_CHANNEL_8                      (LL_ADC_CHANNEL_8)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN8  */
#define ADC_CHANNEL_9                      (LL_ADC_CHANNEL_9)               /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN9  */
#define ADC_CHANNEL_10                     (LL_ADC_CHANNEL_10)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN10 */
#define ADC_CHANNEL_11                     (LL_ADC_CHANNEL_11)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN11 */
#define ADC_CHANNEL_12                     (LL_ADC_CHANNEL_12)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN12 */
#define ADC_CHANNEL_13                     (LL_ADC_CHANNEL_13)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN13 */
#define ADC_CHANNEL_14                     (LL_ADC_CHANNEL_14)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN14 */
#define ADC_CHANNEL_15                     (LL_ADC_CHANNEL_15)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN15 */
#define ADC_CHANNEL_16                     (LL_ADC_CHANNEL_16)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN16 */
#define ADC_CHANNEL_17                     (LL_ADC_CHANNEL_17)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN17 */
#define ADC_CHANNEL_18                     (LL_ADC_CHANNEL_18)              /*!< ADC external channel (channel connected to GPIO pin) ADCx_IN18 */
#define ADC_CHANNEL_VREFINT                (LL_ADC_CHANNEL_VREFINT)         /*!< ADC internal channel connected to VrefInt: Internal voltage reference. */
#define ADC_CHANNEL_TEMPSENSOR             (LL_ADC_CHANNEL_TEMPSENSOR)      /*!< ADC internal channel connected to Temperature sensor. */
#define ADC_CHANNEL_VBAT                   (LL_ADC_CHANNEL_VBAT)            /*!< ADC internal channel connected to Vbat/3: Vbat voltage through a divider ladder of factor 1/3 to have Vbat always below Vdda. */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_AWD_NUMBER Analog watchdog - Analog watchdog number
  * @{
  */
#define ADC_ANALOGWATCHDOG_1               (LL_ADC_AWD1) /*!< ADC analog watchdog number 1 */
#define ADC_ANALOGWATCHDOG_2               (LL_ADC_AWD2) /*!< ADC analog watchdog number 2 */
#define ADC_ANALOGWATCHDOG_3               (LL_ADC_AWD3) /*!< ADC analog watchdog number 3 */
/**
  * @}
  */

/** @defgroup ADC_analog_watchdog_mode ADC Analog Watchdog Mode
  * @{
  */
#define ADC_ANALOGWATCHDOG_NONE                 (0x00000000UL)                                          /*!< No analog watchdog selected                                             */
#define ADC_ANALOGWATCHDOG_SINGLE_REG           (ADC_CFGR1_AWD1SGL | ADC_CFGR1_AWD1EN)                  /*!< Analog watchdog applied to a regular group single channel               */
#define ADC_ANALOGWATCHDOG_ALL_REG              (ADC_CFGR1_AWD1EN)                                      /*!< Analog watchdog applied to regular group all channels                   */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_OVS_RATIO  Oversampling - Ratio
  * @{
  */
#define ADC_OVERSAMPLING_RATIO_2           (LL_ADC_OVS_RATIO_2)   /*!< ADC oversampling ratio of 2 (2 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define ADC_OVERSAMPLING_RATIO_4           (LL_ADC_OVS_RATIO_4)   /*!< ADC oversampling ratio of 4 (4 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define ADC_OVERSAMPLING_RATIO_8           (LL_ADC_OVS_RATIO_8)   /*!< ADC oversampling ratio of 8 (8 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define ADC_OVERSAMPLING_RATIO_16          (LL_ADC_OVS_RATIO_16)  /*!< ADC oversampling ratio of 16 (16 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define ADC_OVERSAMPLING_RATIO_32          (LL_ADC_OVS_RATIO_32)  /*!< ADC oversampling ratio of 32 (32 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define ADC_OVERSAMPLING_RATIO_64          (LL_ADC_OVS_RATIO_64)  /*!< ADC oversampling ratio of 64 (64 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define ADC_OVERSAMPLING_RATIO_128         (LL_ADC_OVS_RATIO_128) /*!< ADC oversampling ratio of 128 (128 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
#define ADC_OVERSAMPLING_RATIO_256         (LL_ADC_OVS_RATIO_256) /*!< ADC oversampling ratio of 256 (256 ADC conversions are performed, sum of these conversions data is computed to result as the ADC oversampling conversion data (before potential shift) */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_OVS_SHIFT  Oversampling - Data shift
  * @{
  */
#define ADC_RIGHTBITSHIFT_NONE             (LL_ADC_OVS_SHIFT_NONE)    /*!< ADC oversampling no shift (sum of the ADC conversions data is not divided to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_1                (LL_ADC_OVS_SHIFT_RIGHT_1) /*!< ADC oversampling shift of 1 (sum of the ADC conversions data is divided by 2 to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_2                (LL_ADC_OVS_SHIFT_RIGHT_2) /*!< ADC oversampling shift of 2 (sum of the ADC conversions data is divided by 4 to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_3                (LL_ADC_OVS_SHIFT_RIGHT_3) /*!< ADC oversampling shift of 3 (sum of the ADC conversions data is divided by 8 to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_4                (LL_ADC_OVS_SHIFT_RIGHT_4) /*!< ADC oversampling shift of 4 (sum of the ADC conversions data is divided by 16 to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_5                (LL_ADC_OVS_SHIFT_RIGHT_5) /*!< ADC oversampling shift of 5 (sum of the ADC conversions data is divided by 32 to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_6                (LL_ADC_OVS_SHIFT_RIGHT_6) /*!< ADC oversampling shift of 6 (sum of the ADC conversions data is divided by 64 to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_7                (LL_ADC_OVS_SHIFT_RIGHT_7) /*!< ADC oversampling shift of 7 (sum of the ADC conversions data is divided by 128 to result as the ADC oversampling conversion data) */
#define ADC_RIGHTBITSHIFT_8                (LL_ADC_OVS_SHIFT_RIGHT_8) /*!< ADC oversampling shift of 8 (sum of the ADC conversions data is divided by 256 to result as the ADC oversampling conversion data) */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_OVS_DISCONT_MODE  Oversampling - Discontinuous mode
  * @{
  */
#define ADC_TRIGGEREDMODE_SINGLE_TRIGGER   (LL_ADC_OVS_REG_CONT)          /*!< ADC oversampling discontinuous mode: continuous mode (all conversions of oversampling ratio are done from 1 trigger) */
#define ADC_TRIGGEREDMODE_MULTI_TRIGGER    (LL_ADC_OVS_REG_DISCONT)       /*!< ADC oversampling discontinuous mode: discontinuous mode (each conversion of oversampling ratio needs a trigger) */
/**
  * @}
  */

/** @defgroup ADC_HAL_EC_REG_TRIGGER_FREQ  ADC group regular - Trigger frequency mode
  * @{
  */
#define ADC_TRIGGER_FREQ_HIGH              (LL_ADC_TRIGGER_FREQ_HIGH) /*!< ADC trigger frequency mode set to high frequency. Note: ADC trigger frequency mode must be set to low frequency when a duration is exceeded before ADC conversion start trigger event (between ADC enable and ADC conversion start trigger event or between two ADC conversion start trigger event). Duration value: Refer to device datasheet, parameter "tIdle". */
#define ADC_TRIGGER_FREQ_LOW               (LL_ADC_TRIGGER_FREQ_LOW)  /*!< ADC trigger frequency mode set to low frequency. Note: ADC trigger frequency mode must be set to low frequency when a duration is exceeded before ADC conversion start trigger event (between ADC enable and ADC conversion start trigger event or between two ADC conversion start trigger event). Duration value: Refer to device datasheet, parameter "tIdle". */
/**
  * @}
  */

/** @defgroup ADC_Event_type ADC Event type
  * @{
  */
#define ADC_EOSMP_EVENT          (ADC_FLAG_EOSMP) /*!< ADC End of Sampling event */
#define ADC_AWD1_EVENT           (ADC_FLAG_AWD1)  /*!< ADC Analog watchdog 1 event (main analog watchdog, present on all STM32 series) */
#define ADC_AWD2_EVENT           (ADC_FLAG_AWD2)  /*!< ADC Analog watchdog 2 event (additional analog watchdog, not present on all STM32 series) */
#define ADC_AWD3_EVENT           (ADC_FLAG_AWD3)  /*!< ADC Analog watchdog 3 event (additional analog watchdog, not present on all STM32 series) */
#define ADC_OVR_EVENT            (ADC_FLAG_OVR)   /*!< ADC overrun event */
/**
  * @}
  */
#define ADC_AWD_EVENT            ADC_AWD1_EVENT      /*!< ADC Analog watchdog 1 event: Naming for compatibility with other STM32 devices having only one analog watchdog */

/** @defgroup ADC_interrupts_definition ADC interrupts definition
  * @{
  */
#define ADC_IT_RDY           ADC_IER_ADRDYIE    /*!< ADC Ready interrupt source */
#define ADC_IT_CCRDY         ADC_IER_CCRDYIE    /*!< ADC channel configuration ready interrupt source */
#define ADC_IT_EOSMP         ADC_IER_EOSMPIE    /*!< ADC End of sampling interrupt source */
#define ADC_IT_EOC           ADC_IER_EOCIE      /*!< ADC End of regular conversion interrupt source */
#define ADC_IT_EOS           ADC_IER_EOSIE      /*!< ADC End of regular sequence of conversions interrupt source */
#define ADC_IT_OVR           ADC_IER_OVRIE      /*!< ADC overrun interrupt source */
#define ADC_IT_AWD1          ADC_IER_AWD1IE     /*!< ADC Analog watchdog 1 interrupt source (main analog watchdog) */
#define ADC_IT_AWD2          ADC_IER_AWD2IE     /*!< ADC Analog watchdog 2 interrupt source (additional analog watchdog) */
#define ADC_IT_AWD3          ADC_IER_AWD3IE     /*!< ADC Analog watchdog 3 interrupt source (additional analog watchdog) */
/**
  * @}
  */

/** @defgroup ADC_flags_definition ADC flags definition
  * @{
  */
#define ADC_FLAG_RDY           ADC_ISR_ADRDY    /*!< ADC Ready flag */
#define ADC_FLAG_CCRDY         ADC_ISR_CCRDY    /*!< ADC channel configuration ready flag */
#define ADC_FLAG_EOSMP         ADC_ISR_EOSMP    /*!< ADC End of Sampling flag */
#define ADC_FLAG_EOC           ADC_ISR_EOC      /*!< ADC End of Regular Conversion flag */
#define ADC_FLAG_EOS           ADC_ISR_EOS      /*!< ADC End of Regular sequence of Conversions flag */
#define ADC_FLAG_OVR           ADC_ISR_OVR      /*!< ADC overrun flag */
#define ADC_FLAG_AWD1          ADC_ISR_AWD1     /*!< ADC Analog watchdog 1 flag */
#define ADC_FLAG_AWD2          ADC_ISR_AWD2     /*!< ADC Analog watchdog 2 flag */
#define ADC_FLAG_AWD3          ADC_ISR_AWD3     /*!< ADC Analog watchdog 3 flag */
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
#define ADC_IS_SOFTWARE_START_REGULAR(__HANDLE__)                              \
  (((__HANDLE__)->Instance->CFGR1 & ADC_CFGR1_EXTEN) == 0UL)

/**
  * @brief Return resolution bits in CFGR1 register RES[1:0] field.
  * @param __HANDLE__ ADC handle
  * @retval Value of bitfield RES in CFGR1 register.
  */
#define ADC_GET_RESOLUTION(__HANDLE__)                                         \
  (LL_ADC_GetResolution((__HANDLE__)->Instance))

/**
  * @brief Clear ADC error code (set it to no error code "HAL_ADC_ERROR_NONE").
  * @param __HANDLE__ ADC handle
  * @retval None
  */
#define ADC_CLEAR_ERRORCODE(__HANDLE__) ((__HANDLE__)->ErrorCode = HAL_ADC_ERROR_NONE) 

/**
  * @brief Simultaneously clear and set specific bits of the handle State.
  * @note  ADC_STATE_CLR_SET() macro is merely aliased to generic macro MODIFY_REG(),
  *        the first parameter is the ADC handle State, the second parameter is the
  *        bit field to clear, the third and last parameter is the bit field to set.
  * @retval None
  */
#define ADC_STATE_CLR_SET MODIFY_REG

/**
  * @brief Enable ADC discontinuous conversion mode for regular group
  * @param _REG_DISCONTINUOUS_MODE_: Regular discontinuous mode.
  * @retval None
  */
#define ADC_CFGR1_REG_DISCCONTINUOUS(_REG_DISCONTINUOUS_MODE_)                 \
  ((_REG_DISCONTINUOUS_MODE_) << 16U)
  
/**
  * @brief Enable the ADC auto off mode.
  * @param _AUTOOFF_ Auto off bit enable or disable.
  * @retval None
  */
#define ADC_CFGR1_AUTOOFF(_AUTOOFF_)                                           \
  ((_AUTOOFF_) << 15U)

/**
  * @brief Enable the ADC auto delay mode.
  * @param _AUTOWAIT_ Auto delay bit enable or disable.
  * @retval None
  */
#define ADC_CFGR1_AUTOWAIT(_AUTOWAIT_)                                         \
  ((_AUTOWAIT_) << 14U)

/**
  * @brief Enable ADC continuous conversion mode.
  * @param _CONTINUOUS_MODE_ Continuous mode.
  * @retval None
  */
#define ADC_CFGR1_CONTINUOUS(_CONTINUOUS_MODE_)                                \
  ((_CONTINUOUS_MODE_) << 13U)
    
/**
  * @brief Enable ADC overrun mode.
  * @param _OVERRUN_MODE_ Overrun mode.
  * @retval Overun bit setting to be programmed into CFGR register
  */
/* Note: Bit ADC_CFGR1_OVRMOD not used directly in constant                   */
/* "ADC_OVR_DATA_OVERWRITTEN" to have this case defined to 0x00, to set it    */
/* as the default case to be compliant with other STM32 devices.              */
#define ADC_CFGR1_OVERRUN(_OVERRUN_MODE_)                                      \
  ( ( (_OVERRUN_MODE_) != (ADC_OVR_DATA_PRESERVED)                             \
    )? (ADC_CFGR1_OVRMOD) : (0x00000000UL)                                     \
  )

/**
  * @brief Set ADC scan mode with differentiation of sequencer setting
  *        fixed or configurable
  * @param _SCAN_MODE_ Scan conversion mode.
  * @retval None
  */
/* Note: Scan mode set using this macro (instead of parameter direct set)     */
/*       due to different modes on other STM32 devices:                       */
/*       if scan mode is disabled, sequencer is set to fully configurable     */
/*       with setting of only rank 1 enabled afterwards.                      */
#define ADC_SCAN_SEQ_MODE(_SCAN_MODE_)                                         \
  ( (((_SCAN_MODE_) & ADC_SCAN_SEQ_FIXED_INT) != 0UL                           \
    )?                                                                         \
     ((_SCAN_MODE_) & (~ADC_SCAN_SEQ_FIXED_INT))                               \
     :                                                                         \
     (ADC_CFGR1_CHSELRMOD)                                                     \
  )

/**
  * @brief Enable the ADC DMA continuous request.
  * @param _DMACONTREQ_MODE_: DMA continuous request mode.
  * @retval None
  */
#define ADC_CFGR1_DMACONTREQ(_DMACONTREQ_MODE_)                                \
  ((_DMACONTREQ_MODE_) << 1U)

/**
  * @brief Shift the AWD threshold in function of the selected ADC resolution.
  *        Thresholds have to be left-aligned on bit 11, the LSB (right bits) are set to 0.
  *        If resolution 12 bits, no shift.
  *        If resolution 10 bits, shift of 2 ranks on the left.
  *        If resolution 8 bits, shift of 4 ranks on the left.
  *        If resolution 6 bits, shift of 6 ranks on the left.
  *        therefore, shift = (12 - resolution) = 12 - (12- (((RES[1:0]) >> 3)*2))
  * @param __HANDLE__ ADC handle
  * @param _Threshold_ Value to be shifted
  * @retval None
  */
#define ADC_AWD1THRESHOLD_SHIFT_RESOLUTION(__HANDLE__, _Threshold_)            \
  ((_Threshold_) << ((((__HANDLE__)->Instance->CFGR1 & ADC_CFGR1_RES) >> 3U)*2U))

#define IS_ADC_CLOCKPRESCALER(ADC_CLOCK) (((ADC_CLOCK) == ADC_CLOCK_SYNC_PCLK_DIV1) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_SYNC_PCLK_DIV2) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_SYNC_PCLK_DIV4) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV1  )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV2  )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV4  )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV6  )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV8  )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV10 )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV12 )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV16 )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV32 )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV64 )   ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV128 )  ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV256))

#define IS_ADC_RESOLUTION(RESOLUTION) (((RESOLUTION) == ADC_RESOLUTION_12B) || \
                                       ((RESOLUTION) == ADC_RESOLUTION_10B) || \
                                       ((RESOLUTION) == ADC_RESOLUTION_8B)  || \
                                       ((RESOLUTION) == ADC_RESOLUTION_6B)    )

#define IS_ADC_DATA_ALIGN(ALIGN) (((ALIGN) == ADC_DATAALIGN_RIGHT) || \
                                  ((ALIGN) == ADC_DATAALIGN_LEFT)    )

#define IS_ADC_SCAN_MODE(SCAN_MODE) (((SCAN_MODE) == ADC_SCAN_DISABLE)            || \
                                     ((SCAN_MODE) == ADC_SCAN_ENABLE)             || \
                                     ((SCAN_MODE) == ADC_SCAN_SEQ_FIXED)          || \
                                     ((SCAN_MODE) == ADC_SCAN_SEQ_FIXED_BACKWARD)   )

#define IS_ADC_EXTTRIG_EDGE(EDGE) (((EDGE) == ADC_EXTERNALTRIGCONVEDGE_NONE)         || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_RISING)       || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_FALLING)      || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING)  )

#if defined(TIM15) && defined(TIM6) && defined(TIM2)
#define IS_ADC_EXTTRIG(REGTRIG) (((REGTRIG) == ADC_EXTERNALTRIG_T1_TRGO2) || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T1_CC4)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T2_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T3_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T6_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T15_TRGO) || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_EXT_IT11) || \
                                 ((REGTRIG) == ADC_SOFTWARE_START)          )
#elif defined(TIM15) &&  defined(TIM6)
#define IS_ADC_EXTTRIG(REGTRIG) (((REGTRIG) == ADC_EXTERNALTRIG_T1_TRGO2) || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T1_CC4)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T3_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T6_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T15_TRGO) || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_EXT_IT11) || \
                                 ((REGTRIG) == ADC_SOFTWARE_START)          )
#elif defined(TIM2)
#define IS_ADC_EXTTRIG(REGTRIG) (((REGTRIG) == ADC_EXTERNALTRIG_T1_TRGO2) || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T1_CC4)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T2_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T3_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_EXT_IT11) || \
                                 ((REGTRIG) == ADC_SOFTWARE_START)          )
#else
#define IS_ADC_EXTTRIG(REGTRIG) (((REGTRIG) == ADC_EXTERNALTRIG_T1_TRGO2) || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T1_CC4)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_T3_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIG_EXT_IT11) || \
                                 ((REGTRIG) == ADC_SOFTWARE_START)          )
#endif

#define IS_ADC_EOC_SELECTION(EOC_SELECTION) (((EOC_SELECTION) == ADC_EOC_SINGLE_CONV)    || \
                                             ((EOC_SELECTION) == ADC_EOC_SEQ_CONV))

#define IS_ADC_OVERRUN(OVR) (((OVR) == ADC_OVR_DATA_PRESERVED)  || \
                             ((OVR) == ADC_OVR_DATA_OVERWRITTEN)  )

#define IS_ADC_REGULAR_RANK_SEQ_FIXED(RANK) (((RANK) == ADC_RANK_CHANNEL_NUMBER) || \
                                             ((RANK) == ADC_RANK_NONE)             )

#define IS_ADC_REGULAR_RANK(RANK) (((RANK) == ADC_REGULAR_RANK_1 ) || \
                                   ((RANK) == ADC_REGULAR_RANK_2 ) || \
                                   ((RANK) == ADC_REGULAR_RANK_3 ) || \
                                   ((RANK) == ADC_REGULAR_RANK_4 ) || \
                                   ((RANK) == ADC_REGULAR_RANK_5 ) || \
                                   ((RANK) == ADC_REGULAR_RANK_6 ) || \
                                   ((RANK) == ADC_REGULAR_RANK_7 ) || \
                                   ((RANK) == ADC_REGULAR_RANK_8 )   )

#define IS_ADC_CHANNEL(CHANNEL) (((CHANNEL) == ADC_CHANNEL_0)           || \
                                 ((CHANNEL) == ADC_CHANNEL_1)           || \
                                 ((CHANNEL) == ADC_CHANNEL_2)           || \
                                 ((CHANNEL) == ADC_CHANNEL_3)           || \
                                 ((CHANNEL) == ADC_CHANNEL_4)           || \
                                 ((CHANNEL) == ADC_CHANNEL_5)           || \
                                 ((CHANNEL) == ADC_CHANNEL_6)           || \
                                 ((CHANNEL) == ADC_CHANNEL_7)           || \
                                 ((CHANNEL) == ADC_CHANNEL_8)           || \
                                 ((CHANNEL) == ADC_CHANNEL_9)           || \
                                 ((CHANNEL) == ADC_CHANNEL_10)          || \
                                 ((CHANNEL) == ADC_CHANNEL_11)          || \
                                 ((CHANNEL) == ADC_CHANNEL_12)          || \
                                 ((CHANNEL) == ADC_CHANNEL_13)          || \
                                 ((CHANNEL) == ADC_CHANNEL_14)          || \
                                 ((CHANNEL) == ADC_CHANNEL_15)          || \
                                 ((CHANNEL) == ADC_CHANNEL_16)          || \
                                 ((CHANNEL) == ADC_CHANNEL_17)          || \
                                 ((CHANNEL) == ADC_CHANNEL_18)          || \
                                 ((CHANNEL) == ADC_CHANNEL_TEMPSENSOR)  || \
                                 ((CHANNEL) == ADC_CHANNEL_VREFINT)     || \
                                 ((CHANNEL) == ADC_CHANNEL_VBAT)          )

#define IS_ADC_SAMPLING_TIME_COMMON(SAMPLING_TIME_COMMON) (((SAMPLING_TIME_COMMON) == ADC_SAMPLINGTIME_COMMON_1) || \
                                                           ((SAMPLING_TIME_COMMON) == ADC_SAMPLINGTIME_COMMON_2)   )

#define IS_ADC_SAMPLE_TIME(TIME) (((TIME) == ADC_SAMPLETIME_1CYCLE_5)    || \
                                  ((TIME) == ADC_SAMPLETIME_3CYCLES_5)   || \
                                  ((TIME) == ADC_SAMPLETIME_7CYCLES_5)   || \
                                  ((TIME) == ADC_SAMPLETIME_12CYCLES_5)  || \
                                  ((TIME) == ADC_SAMPLETIME_19CYCLES_5)  || \
                                  ((TIME) == ADC_SAMPLETIME_39CYCLES_5)  || \
                                  ((TIME) == ADC_SAMPLETIME_79CYCLES_5)  || \
                                  ((TIME) == ADC_SAMPLETIME_160CYCLES_5)   )

#define IS_ADC_ANALOG_WATCHDOG_NUMBER(WATCHDOG) (((WATCHDOG) == ADC_ANALOGWATCHDOG_1) || \
                                                 ((WATCHDOG) == ADC_ANALOGWATCHDOG_2) || \
                                                 ((WATCHDOG) == ADC_ANALOGWATCHDOG_3)   )

#define IS_ADC_ANALOG_WATCHDOG_MODE(WATCHDOG) (((WATCHDOG) == ADC_ANALOGWATCHDOG_NONE)             || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_SINGLE_REG)       || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_ALL_REG)            )

#define IS_ADC_TRIGGER_FREQ(TRIGGER_FREQ) (((TRIGGER_FREQ) == LL_ADC_TRIGGER_FREQ_HIGH) || \
                                           ((TRIGGER_FREQ) == LL_ADC_TRIGGER_FREQ_LOW)    )

#define IS_ADC_EVENT_TYPE(EVENT) (((EVENT) == ADC_EOSMP_EVENT) || \
                                  ((EVENT) == ADC_AWD1_EVENT)  || \
                                  ((EVENT) == ADC_AWD2_EVENT)  || \
                                  ((EVENT) == ADC_AWD3_EVENT)  || \
                                  ((EVENT) == ADC_OVR_EVENT)     )

/**
  * @brief Verify that a given value is aligned with the ADC resolution range.
  * @param __RESOLUTION__ ADC resolution (12, 10, 8 or 6 bits).
  * @param __ADC_VALUE__ value checked against the resolution.     
  * @retval SET (__ADC_VALUE__ in line with __RESOLUTION__) or RESET (__ADC_VALUE__ not in line with __RESOLUTION__)
  */
#define IS_ADC_RANGE(__RESOLUTION__, __ADC_VALUE__) \
  ((__ADC_VALUE__) <= __LL_ADC_DIGITAL_SCALE(__RESOLUTION__))

/** @defgroup ADC_regular_nb_conv_verification ADC Regular Conversion Number Verification
  * @{
  */
#define IS_ADC_REGULAR_NB_CONV(LENGTH) (((LENGTH) >= 1UL) && ((LENGTH) <= 8UL))
/**
  * @}
  */


/* Private constants ---------------------------------------------------------*/

/** @defgroup ADC_Private_Constants ADC Private Constants
  * @{
  */

/* Combination of all post-conversion flags bits: EOC/EOS, OVR, AWD */
#define ADC_FLAG_POSTCONV_ALL    (ADC_FLAG_AWD | ADC_FLAG_OVR | ADC_FLAG_EOS | ADC_FLAG_EOC)

#define ADC_SCAN_SEQ_FIXED_INT  0x80000000U  /* Internal definition to differentiate sequencer setting fixed or configurable */

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
#if (USE_HAL_ADC_REGISTER_CALLBACKS == 1)
#define __HAL_ADC_RESET_HANDLE_STATE(__HANDLE__)                               \
  do{                                                                          \
     (__HANDLE__)->State = HAL_ADC_STATE_RESET;                               \
     (__HANDLE__)->MspInitCallback = NULL;                                     \
     (__HANDLE__)->MspDeInitCallback = NULL;                                   \
    } while(0)
#else
#define __HAL_ADC_RESET_HANDLE_STATE(__HANDLE__)                               \
  ((__HANDLE__)->State = HAL_ADC_STATE_RESET)
#endif

/**
  * @brief Enable ADC interrupt.
  * @param __HANDLE__ ADC handle
  * @param __INTERRUPT__ ADC Interrupt
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_IT_RDY    ADC Ready interrupt source
  *            @arg @ref ADC_IT_CCRDY  ADC channel configuration ready interrupt source
  *            @arg @ref ADC_IT_EOSMP  ADC End of Sampling interrupt source
  *            @arg @ref ADC_IT_EOC    ADC End of Regular Conversion interrupt source
  *            @arg @ref ADC_IT_EOS    ADC End of Regular sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_OVR    ADC overrun interrupt source
  *            @arg @ref ADC_IT_AWD1   ADC Analog watchdog 1 interrupt source (main analog watchdog)
  *            @arg @ref ADC_IT_AWD2   ADC Analog watchdog 2 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_AWD3   ADC Analog watchdog 3 interrupt source (additional analog watchdog)
  * @retval None
  */
#define __HAL_ADC_ENABLE_IT(__HANDLE__, __INTERRUPT__)                         \
  (((__HANDLE__)->Instance->IER) |= (__INTERRUPT__))

/**
  * @brief Disable ADC interrupt.
  * @param __HANDLE__ ADC handle
  * @param __INTERRUPT__ ADC Interrupt
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_IT_RDY    ADC Ready interrupt source
  *            @arg @ref ADC_IT_CCRDY  ADC channel configuration ready interrupt source
  *            @arg @ref ADC_IT_EOSMP  ADC End of Sampling interrupt source
  *            @arg @ref ADC_IT_EOC    ADC End of Regular Conversion interrupt source
  *            @arg @ref ADC_IT_EOS    ADC End of Regular sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_OVR    ADC overrun interrupt source
  *            @arg @ref ADC_IT_AWD1   ADC Analog watchdog 1 interrupt source (main analog watchdog)
  *            @arg @ref ADC_IT_AWD2   ADC Analog watchdog 2 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_AWD3   ADC Analog watchdog 3 interrupt source (additional analog watchdog)
  * @retval None
  */
#define __HAL_ADC_DISABLE_IT(__HANDLE__, __INTERRUPT__)                        \
  (((__HANDLE__)->Instance->IER) &= ~(__INTERRUPT__))

/** @brief  Checks if the specified ADC interrupt source is enabled or disabled.
  * @param __HANDLE__ ADC handle
  * @param __INTERRUPT__ ADC interrupt source to check
  *          This parameter can be one of the following values:
  *            @arg @ref ADC_IT_RDY    ADC Ready interrupt source
  *            @arg @ref ADC_IT_CCRDY  ADC channel configuration ready interrupt source
  *            @arg @ref ADC_IT_EOSMP  ADC End of Sampling interrupt source
  *            @arg @ref ADC_IT_EOC    ADC End of Regular Conversion interrupt source
  *            @arg @ref ADC_IT_EOS    ADC End of Regular sequence of Conversions interrupt source
  *            @arg @ref ADC_IT_OVR    ADC overrun interrupt source
  *            @arg @ref ADC_IT_AWD1   ADC Analog watchdog 1 interrupt source (main analog watchdog)
  *            @arg @ref ADC_IT_AWD2   ADC Analog watchdog 2 interrupt source (additional analog watchdog)
  *            @arg @ref ADC_IT_AWD3   ADC Analog watchdog 3 interrupt source (additional analog watchdog)
  * @retval State of interruption (SET or RESET)
  */
#define __HAL_ADC_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)                     \
  (((__HANDLE__)->Instance->IER & (__INTERRUPT__)) == (__INTERRUPT__))
    
/**
  * @brief Check whether the specified ADC flag is set or not.
  * @param __HANDLE__ ADC handle
  * @param __FLAG__ ADC flag
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_FLAG_RDY    ADC Ready flag
  *            @arg @ref ADC_FLAG_CCRDY  ADC channel configuration ready flag
  *            @arg @ref ADC_FLAG_EOSMP   ADC End of Sampling flag                            
  *            @arg @ref ADC_FLAG_EOC     ADC End of Regular Conversion flag                  
  *            @arg @ref ADC_FLAG_EOS     ADC End of Regular sequence of Conversions flag     
  *            @arg @ref ADC_FLAG_OVR     ADC overrun flag        
  *            @arg @ref ADC_FLAG_AWD1    ADC Analog watchdog 1 flag (main analog watchdog)
  *            @arg @ref ADC_FLAG_AWD2    ADC Analog watchdog 2 flag (additional analog watchdog)
  *            @arg @ref ADC_FLAG_AWD3    ADC Analog watchdog 3 flag (additional analog watchdog)
  * @retval State of flag (TRUE or FALSE).
  */
#define __HAL_ADC_GET_FLAG(__HANDLE__, __FLAG__)                               \
  ((((__HANDLE__)->Instance->ISR) & (__FLAG__)) == (__FLAG__))

/**
  * @brief Clear the specified ADC flag.
  * @param __HANDLE__ ADC handle
  * @param __FLAG__ ADC flag
  *        This parameter can be one of the following values:
  *            @arg @ref ADC_FLAG_RDY    ADC Ready flag
  *            @arg @ref ADC_FLAG_CCRDY  ADC channel configuration ready flag
  *            @arg @ref ADC_FLAG_EOSMP   ADC End of Sampling flag                            
  *            @arg @ref ADC_FLAG_EOC     ADC End of Regular Conversion flag                  
  *            @arg @ref ADC_FLAG_EOS     ADC End of Regular sequence of Conversions flag     
  *            @arg @ref ADC_FLAG_OVR     ADC overrun flag        
  *            @arg @ref ADC_FLAG_AWD1    ADC Analog watchdog 1 flag (main analog watchdog)
  *            @arg @ref ADC_FLAG_AWD2    ADC Analog watchdog 2 flag (additional analog watchdog)
  *            @arg @ref ADC_FLAG_AWD3    ADC Analog watchdog 3 flag (additional analog watchdog)
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
  *         @arg @ref ADC_CHANNEL_15         (1)
  *         @arg @ref ADC_CHANNEL_16         (1)
  *         @arg @ref ADC_CHANNEL_17         (1)
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval Value between Min_Data=0 and Max_Data=18
  */
#define __HAL_ADC_CHANNEL_TO_DECIMAL_NB(__CHANNEL__)                           \
         __LL_ADC_CHANNEL_TO_DECIMAL_NB((__CHANNEL__))

/**
  * @brief  Helper macro to get ADC channel in literal format ADC_CHANNEL_x
  *         from number in decimal format.
  * @note   Example:
  *           __HAL_ADC_DECIMAL_NB_TO_CHANNEL(4)
  *           will return a data equivalent to "ADC_CHANNEL_4".
  * @param  __DECIMAL_NB__ Value between Min_Data=0 and Max_Data=18
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
  *         @arg @ref ADC_CHANNEL_15         (1)
  *         @arg @ref ADC_CHANNEL_16         (1)
  *         @arg @ref ADC_CHANNEL_17         (1)
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT    (2)
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR (2)
  *         @arg @ref ADC_CHANNEL_VBAT       (2)
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().\n
  *         (2) For ADC channel read back from ADC register,
  *             comparison with internal channel parameter to be done
  *             using helper macro @ref __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL().
  */
#define __HAL_ADC_DECIMAL_NB_TO_CHANNEL(__DECIMAL_NB__)                        \
         __LL_ADC_DECIMAL_NB_TO_CHANNEL((__DECIMAL_NB__))

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
  *         @arg @ref ADC_CHANNEL_15         (1)
  *         @arg @ref ADC_CHANNEL_16         (1)
  *         @arg @ref ADC_CHANNEL_17         (1)
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
  * @retval Value "0" if the channel corresponds to a parameter definition of a ADC external channel (channel connected to a GPIO pin).
  *         Value "1" if the channel corresponds to a parameter definition of a ADC internal channel.
  */
#define __HAL_ADC_IS_CHANNEL_INTERNAL(__CHANNEL__)                             \
         __LL_ADC_IS_CHANNEL_INTERNAL((__CHANNEL__))

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
  *         @arg @ref ADC_CHANNEL_15         (1)
  *         @arg @ref ADC_CHANNEL_16         (1)
  *         @arg @ref ADC_CHANNEL_17         (1)
  *         @arg @ref ADC_CHANNEL_18
  *         @arg @ref ADC_CHANNEL_VREFINT
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref ADC_CHANNEL_VBAT
  *         
  *         (1) On STM32G0, parameter can be set in ADC group sequencer
  *             only if sequencer is set in mode "not fully configurable",
  *             refer to function @ref LL_ADC_REG_SetSequencerConfigurable().
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
         __LL_ADC_CHANNEL_INTERNAL_TO_EXTERNAL((__CHANNEL__))

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
  *         @arg @ref ADC_CHANNEL_VREFINT
  *         @arg @ref ADC_CHANNEL_TEMPSENSOR
  *         @arg @ref ADC_CHANNEL_VBAT
  * @retval Value "0" if the internal channel selected is not available on the ADC instance selected.
  *         Value "1" if the internal channel selected is available on the ADC instance selected.
  */
#define __HAL_ADC_IS_CHANNEL_INTERNAL_AVAILABLE(__ADC_INSTANCE__, __CHANNEL__)  \
         __LL_ADC_IS_CHANNEL_INTERNAL_AVAILABLE((__ADC_INSTANCE__), (__CHANNEL__))

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
         __LL_ADC_COMMON_INSTANCE((__ADCx__))

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
         __LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE((__ADCXY_COMMON__))

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
  * @retval ADC conversion data full-scale digital value
  */
#define __HAL_ADC_DIGITAL_SCALE(__ADC_RESOLUTION__)                             \
         __LL_ADC_DIGITAL_SCALE((__ADC_RESOLUTION__))

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
         __LL_ADC_CONVERT_DATA_RESOLUTION((__DATA__),\
                                          (__ADC_RESOLUTION_CURRENT__),\
                                          (__ADC_RESOLUTION_TARGET__))

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
         __LL_ADC_CALC_DATA_TO_VOLTAGE((__VREFANALOG_VOLTAGE__),\
                                       (__ADC_DATA__),\
                                       (__ADC_RESOLUTION__))

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
         __LL_ADC_CALC_VREFANALOG_VOLTAGE((__VREFINT_ADC_DATA__),\
                                          (__ADC_RESOLUTION__))

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
         __LL_ADC_CALC_TEMPERATURE((__VREFANALOG_VOLTAGE__),\
                                   (__TEMPSENSOR_ADC_DATA__),\
                                   (__ADC_RESOLUTION__))

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
  *                                       On STM32G0, refer to device datasheet parameter "Avg_Slope".
  * @param  __TEMPSENSOR_TYP_CALX_V__     Device datasheet data: Temperature sensor voltage typical value (at temperature and Vref+ defined in parameters below) (unit: mV).
  *                                       On STM32G0, refer to device datasheet parameter "V30" (corresponding to TS_CAL1).
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
         __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS((__TEMPSENSOR_TYP_AVGSLOPE__),\
                                              (__TEMPSENSOR_TYP_CALX_V__),\
                                              (__TEMPSENSOR_CALX_TEMP__),\
                                              (__VREFANALOG_VOLTAGE__),\
                                              (__TEMPSENSOR_ADC_DATA__),\
                                              (__ADC_RESOLUTION__))

/**
  * @}
  */

/**
  * @}
  */

/* Include ADC HAL Extended module */
#include "stm32g0xx_hal_adc_ex.h"

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

#if (USE_HAL_ADC_REGISTER_CALLBACKS == 1)
/* Callbacks Register/UnRegister functions  ***********************************/
HAL_StatusTypeDef HAL_ADC_RegisterCallback(ADC_HandleTypeDef *hadc, HAL_ADC_CallbackIDTypeDef CallbackID, pADC_CallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_ADC_UnRegisterCallback(ADC_HandleTypeDef *hadc, HAL_ADC_CallbackIDTypeDef CallbackID);
#endif /* USE_HAL_ADC_REGISTER_CALLBACKS */
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

/* Private functions ---------------------------------------------------------*/
HAL_StatusTypeDef ADC_ConversionStop(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef ADC_Disable(ADC_HandleTypeDef* hadc);

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


#endif /* STM32G0xx_HAL_ADC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
