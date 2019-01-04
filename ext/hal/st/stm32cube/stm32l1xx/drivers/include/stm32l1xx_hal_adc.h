/**
  ******************************************************************************
  * @file    stm32l1xx_hal_adc.h
  * @author  MCD Application Team
  * @brief   Header file containing functions prototypes of ADC HAL library.
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
#ifndef __STM32L1xx_HAL_ADC_H
#define __STM32L1xx_HAL_ADC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal_def.h"

/** @addtogroup STM32L1xx_HAL_Driver
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
  * @brief  Structure definition of ADC and regular group initialization 
  * @note   Parameters of this structure are shared within 2 scopes:
  *          - Scope entire ADC (affects regular and injected groups): ClockPrescaler, Resolution, ScanConvMode, DataAlign, ScanConvMode, EOCSelection, LowPowerAutoWait, LowPowerAutoPowerOff, ChannelsBank.
  *          - Scope regular group: ContinuousConvMode, NbrOfConversion, DiscontinuousConvMode, NbrOfDiscConversion, ExternalTrigConvEdge, ExternalTrigConv.
  * @note   The setting of these parameters with function HAL_ADC_Init() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled
  *          - For all parameters except 'Resolution', 'ScanConvMode', 'LowPowerAutoWait', 'LowPowerAutoPowerOff', 'DiscontinuousConvMode', 'NbrOfDiscConversion' : ADC enabled without conversion on going on regular group.
  *          - For parameters 'ExternalTrigConv' and 'ExternalTrigConvEdge': ADC enabled, even with conversion on going.
  *         If ADC is not in the appropriate state to modify some parameters, these parameters setting is bypassed
  *         without error reporting (as it can be the expected behaviour in case of intended action to update another parameter (which fullfills the ADC state condition) on the fly).
  */
typedef struct
{
  uint32_t ClockPrescaler;        /*!< Select ADC clock source (asynchronous clock derived from HSI RC oscillator) and clock prescaler.
                                       This parameter can be a value of @ref ADC_ClockPrescaler
                                       Note: In case of usage of channels on injected group, ADC frequency should be lower than AHB clock frequency /4 for resolution 12 or 10 bits, 
                                             AHB clock frequency /3 for resolution 8 bits, AHB clock frequency /2 for resolution 6 bits.
                                       Note: HSI RC oscillator must be preliminarily enabled at RCC top level. */
  uint32_t Resolution;            /*!< Configures the ADC resolution. 
                                       This parameter can be a value of @ref ADC_Resolution */
  uint32_t DataAlign;             /*!< Specifies ADC data alignment to right (MSB on register bit 11 and LSB on register bit 0) (default setting)
                                       or to left (if regular group: MSB on register bit 15 and LSB on register bit 4, if injected group (MSB kept as signed value due to potential negative value after offset application): MSB on register bit 14 and LSB on register bit 3).
                                       This parameter can be a value of @ref ADC_Data_align */
  uint32_t ScanConvMode;          /*!< Configures the sequencer of regular and injected groups.
                                       This parameter can be associated to parameter 'DiscontinuousConvMode' to have main sequence subdivided in successive parts.
                                       If disabled: Conversion is performed in single mode (one channel converted, the one defined in rank 1).
                                                    Parameters 'NbrOfConversion' and 'InjectedNbrOfConversion' are discarded (equivalent to set to 1).
                                       If enabled:  Conversions are performed in sequence mode (multiple ranks defined by 'NbrOfConversion'/'InjectedNbrOfConversion' and each channel rank).
                                                    Scan direction is upward: from rank1 to rank 'n'.
                                       This parameter can be a value of @ref ADC_Scan_mode */
  uint32_t EOCSelection;          /*!< Specifies what EOC (End Of Conversion) flag is used for conversion by polling and interruption: end of conversion of each rank or complete sequence.
                                       This parameter can be a value of @ref ADC_EOCSelection.
                                       Note: For injected group, end of conversion (flag&IT) is raised only at the end of the sequence.
                                             Therefore, if end of conversion is set to end of each conversion, injected group should not be used with interruption (HAL_ADCEx_InjectedStart_IT)
                                             or polling (HAL_ADCEx_InjectedStart and HAL_ADCEx_InjectedPollForConversion). By the way, polling is still possible since driver will use an estimated timing for end of injected conversion.
                                       Note: If overrun feature is intended to be used, use ADC in mode 'interruption' (function HAL_ADC_Start_IT() ) with parameter EOCSelection set to end of each conversion or in mode 'transfer by DMA' (function HAL_ADC_Start_DMA()).
                                             If overrun feature is intended to be bypassed, use ADC in mode 'polling' or 'interruption' with parameter EOCSelection must be set to end of sequence */
  uint32_t LowPowerAutoWait;      /*!< Selects the dynamic low power Auto Delay: new conversion start only when the previous
                                       conversion (for regular group) or previous sequence (for injected group) has been treated by user software, using function HAL_ADC_GetValue() or HAL_ADCEx_InjectedGetValue().
                                       This feature automatically adapts the speed of ADC to the speed of the system that reads the data. Moreover, this avoids risk of overrun for low frequency applications. 
                                       This parameter can be a value of @ref ADC_LowPowerAutoWait.
                                       Note: Do not use with interruption or DMA (HAL_ADC_Start_IT(), HAL_ADC_Start_DMA()) since they have to clear immediately the EOC flag to free the IRQ vector sequencer.
                                             Do use with polling: 1. Start conversion with HAL_ADC_Start(), 2. Later on, when conversion data is needed: use HAL_ADC_PollForConversion() to ensure that conversion is completed
                                             and use HAL_ADC_GetValue() to retrieve conversion result and trig another conversion (in case of usage of injected group, use the equivalent functions HAL_ADCExInjected_Start(), HAL_ADCEx_InjectedGetValue(), ...).
                                       Note: ADC clock latency and some timing constraints depending on clock prescaler have to be taken into account: refer to reference manual (register ADC_CR2 bit DELS description). */
  uint32_t LowPowerAutoPowerOff;  /*!< Selects the auto-off mode: the ADC automatically powers-off after a conversion and automatically wakes-up when a new conversion is triggered (with startup time between trigger and start of sampling).
                                       This feature can be combined with automatic wait mode (parameter 'LowPowerAutoWait').
                                       This parameter can be a value of @ref ADC_LowPowerAutoPowerOff. */
  uint32_t ChannelsBank;          /*!< Selects the ADC channels bank.
                                       This parameter can be a value of @ref ADC_ChannelsBank.
                                       Note: Banks availability depends on devices categories.
                                       Note: To change bank selection on the fly, without going through execution of 'HAL_ADC_Init()', macro '__HAL_ADC_CHANNELS_BANK()' can be used directly. */
  uint32_t ContinuousConvMode;    /*!< Specifies whether the conversion is performed in single mode (one conversion) or continuous mode for regular group,
                                       after the selected trigger occurred (software start or external trigger).
                                       This parameter can be set to ENABLE or DISABLE. */
#if defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC) || defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
  uint32_t NbrOfConversion;       /*!< Specifies the number of ranks that will be converted within the regular group sequencer.
                                       To use regular group sequencer and convert several ranks, parameter 'ScanConvMode' must be enabled.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 28. */
#else
  uint32_t NbrOfConversion;       /*!< Specifies the number of ranks that will be converted within the regular group sequencer.
                                       To use regular group sequencer and convert several ranks, parameter 'ScanConvMode' must be enabled.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 27. */
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
  uint32_t DiscontinuousConvMode; /*!< Specifies whether the conversions sequence of regular group is performed in Complete-sequence/Discontinuous-sequence (main sequence subdivided in successive parts).
                                       Discontinuous mode is used only if sequencer is enabled (parameter 'ScanConvMode'). If sequencer is disabled, this parameter is discarded.
                                       Discontinuous mode can be enabled only if continuous mode is disabled. If continuous mode is enabled, this parameter setting is discarded.
                                       This parameter can be set to ENABLE or DISABLE. */
  uint32_t NbrOfDiscConversion;   /*!< Specifies the number of discontinuous conversions in which the  main sequence of regular group (parameter NbrOfConversion) will be subdivided.
                                       If parameter 'DiscontinuousConvMode' is disabled, this parameter is discarded.
                                       This parameter must be a number between Min_Data = 1 and Max_Data = 8. */
  uint32_t ExternalTrigConv;      /*!< Selects the external event used to trigger the conversion start of regular group.
                                       If set to ADC_SOFTWARE_START, external triggers are disabled.
                                       If set to external trigger source, triggering is on event rising edge by default.
                                       This parameter can be a value of @ref ADC_External_trigger_source_Regular */
  uint32_t ExternalTrigConvEdge;  /*!< Selects the external trigger edge of regular group.
                                       If trigger is set to ADC_SOFTWARE_START, this parameter is discarded.
                                       This parameter can be a value of @ref ADC_External_trigger_edge_Regular */
  uint32_t DMAContinuousRequests; /*!< Specifies whether the DMA requests are performed in one shot mode (DMA transfer stop when number of conversions is reached)
                                       or in Continuous mode (DMA transfer unlimited, whatever number of conversions).
                                       Note: In continuous mode, DMA must be configured in circular mode. Otherwise an overrun will be triggered when DMA buffer maximum pointer is reached.
                                       Note: This parameter must be modified when no conversion is on going on both regular and injected groups (ADC disabled, or ADC enabled without continuous mode or external trigger that could launch a conversion).
                                       This parameter can be set to ENABLE or DISABLE. */
}ADC_InitTypeDef;

/** 
  * @brief  Structure definition of ADC channel for regular group   
  * @note   The setting of these parameters with function HAL_ADC_ConfigChannel() is conditioned to ADC state.
  *         ADC can be either disabled or enabled without conversion on going on regular group.
  */ 
typedef struct 
{
  uint32_t Channel;                /*!< Specifies the channel to configure into ADC regular group.
                                        This parameter can be a value of @ref ADC_channels
                                        Note: Depending on devices, some channels may not be available on package pins. Refer to device datasheet for channels availability.
                                              Maximum number of channels by device category (without taking in account each device package constraints): 
                                              STM32L1 category 1, 2: 24 channels on external pins + 3 channels on internal measurement paths (VrefInt, Temp sensor, Vcomp): Channel 0 to channel 26.
                                              STM32L1 category 3:    25 channels on external pins + 3 channels on internal measurement paths (VrefInt, Temp sensor, Vcomp): Channel 0 to channel 26, 1 additional channel in bank B. Note: OPAMP1 and OPAMP2 are connected internally but not increasing internal channels number: they are sharing ADC input with external channels ADC_IN3 and ADC_IN8.
                                              STM32L1 category 4, 5: 40 channels on external pins + 3 channels on internal measurement paths (VrefInt, Temp sensor, Vcomp): Channel 0 to channel 31, 11 additional channels in bank B. Note: OPAMP1 and OPAMP2 are connected internally but not increasing internal channels number: they are sharing ADC input with external channels ADC_IN3 and ADC_IN8.
                                        Note: In case of peripherals OPAMPx not used: 3 channels (3, 8, 13) can be configured as direct channels (fast channels). Refer to macro ' __HAL_ADC_CHANNEL_SPEED_FAST() '.
                                        Note: In case of peripheral OPAMP3 and ADC channel OPAMP3 used (OPAMP3 available on STM32L1 devices Cat.4 only): the analog switch COMP1_SW1 must be closed. Refer to macro: ' __HAL_OPAMP_OPAMP3OUT_CONNECT_ADC_COMP1() '. */
  uint32_t Rank;                   /*!< Specifies the rank in the regular group sequencer.
                                        This parameter can be a value of @ref ADC_regular_rank
                                        Note: In case of need to disable a channel or change order of conversion sequencer, rank containing a previous channel setting can be overwritten by the new channel setting (or parameter number of conversions can be adjusted) */
  uint32_t SamplingTime;           /*!< Sampling time value to be set for the selected channel.
                                        Unit: ADC clock cycles
                                        Conversion time is the addition of sampling time and processing time (12 ADC clock cycles at ADC resolution 12 bits, 11 cycles at 10 bits, 9 cycles at 8 bits, 7 cycles at 6 bits).
                                        This parameter can be a value of @ref ADC_sampling_times
                                        Caution: This parameter updates the parameter property of the channel, that can be used into regular and/or injected groups.
                                                 If this same channel has been previously configured in the other group (regular/injected), it will be updated to last setting.
                                        Note: In case of usage of internal measurement channels (VrefInt/Vbat/TempSensor),
                                              sampling time constraints must be respected (sampling time can be adjusted in function of ADC clock frequency and sampling time setting)
                                              Refer to device datasheet for timings values, parameters TS_vrefint, TS_temp (values rough order: 4us min). */
}ADC_ChannelConfTypeDef;

/**
  * @brief  ADC Configuration analog watchdog definition
  * @note   The setting of these parameters with function is conditioned to ADC state.
  *         ADC state can be either disabled or enabled without conversion on going on regular and injected groups.
  */
typedef struct
{
  uint32_t WatchdogMode;      /*!< Configures the ADC analog watchdog mode: single/all channels, regular/injected group.
                                   This parameter can be a value of @ref ADC_analog_watchdog_mode. */
  uint32_t Channel;           /*!< Selects which ADC channel to monitor by analog watchdog.
                                   This parameter has an effect only if watchdog mode is configured on single channel (parameter WatchdogMode)
                                   This parameter can be a value of @ref ADC_channels. */
  uint32_t ITMode;            /*!< Specifies whether the analog watchdog is configured in interrupt or polling mode.
                                   This parameter can be set to ENABLE or DISABLE */
  uint32_t HighThreshold;     /*!< Configures the ADC analog watchdog High threshold value.
                                   This parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF. */
  uint32_t LowThreshold;      /*!< Configures the ADC analog watchdog High threshold value.
                                   This parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF. */
  uint32_t WatchdogNumber;    /*!< Reserved for future use, can be set to 0 */
}ADC_AnalogWDGConfTypeDef;

/** 
  * @brief  HAL ADC state machine: ADC states definition (bitfields)
  */ 
/* States of ADC global scope */
#define HAL_ADC_STATE_RESET             (0x00000000U)    /*!< ADC not yet initialized or disabled */
#define HAL_ADC_STATE_READY             (0x00000001U)    /*!< ADC peripheral ready for use */
#define HAL_ADC_STATE_BUSY_INTERNAL     (0x00000002U)    /*!< ADC is busy to internal process (initialization, calibration) */
#define HAL_ADC_STATE_TIMEOUT           (0x00000004U)    /*!< TimeOut occurrence */

/* States of ADC errors */
#define HAL_ADC_STATE_ERROR_INTERNAL    (0x00000010U)    /*!< Internal error occurrence */
#define HAL_ADC_STATE_ERROR_CONFIG      (0x00000020U)    /*!< Configuration error occurrence */
#define HAL_ADC_STATE_ERROR_DMA         (0x00000040U)    /*!< DMA error occurrence */

/* States of ADC group regular */
#define HAL_ADC_STATE_REG_BUSY          (0x00000100U)    /*!< A conversion on group regular is ongoing or can occur (either by continuous mode,
                                                                       external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available)) */
#define HAL_ADC_STATE_REG_EOC           (0x00000200U)    /*!< Conversion data available on group regular */
#define HAL_ADC_STATE_REG_OVR           (0x00000400U)    /*!< Overrun occurrence */
#define HAL_ADC_STATE_REG_EOSMP         (0x00000800U)    /*!< Not available on STM32L1 device: End Of Sampling flag raised  */

/* States of ADC group injected */
#define HAL_ADC_STATE_INJ_BUSY          (0x00001000U)    /*!< A conversion on group injected is ongoing or can occur (either by auto-injection mode,
                                                                       external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available)) */
#define HAL_ADC_STATE_INJ_EOC           (0x00002000U)    /*!< Conversion data available on group injected */
#define HAL_ADC_STATE_INJ_JQOVF         (0x00004000U)    /*!< Not available on STM32L1 device: Injected queue overflow occurrence */

/* States of ADC analog watchdogs */
#define HAL_ADC_STATE_AWD1              (0x00010000U)    /*!< Out-of-window occurrence of analog watchdog 1 */
#define HAL_ADC_STATE_AWD2              (0x00020000U)    /*!< Not available on STM32L1 device: Out-of-window occurrence of analog watchdog 2 */
#define HAL_ADC_STATE_AWD3              (0x00040000U)    /*!< Not available on STM32L1 device: Out-of-window occurrence of analog watchdog 3 */

/* States of ADC multi-mode */
#define HAL_ADC_STATE_MULTIMODE_SLAVE   (0x00100000U)    /*!< Not available on STM32L1 device: ADC in multimode slave state, controlled by another ADC master ( */


/** 
  * @brief  ADC handle Structure definition
  */ 
typedef struct
{
  ADC_TypeDef                   *Instance;              /*!< Register base address */

  ADC_InitTypeDef               Init;                   /*!< ADC required parameters */

  __IO uint32_t                 NbrOfConversionRank ;   /*!< ADC conversion rank counter */

  DMA_HandleTypeDef             *DMA_Handle;            /*!< Pointer DMA Handler */

  HAL_LockTypeDef               Lock;                   /*!< ADC locking object */

  __IO uint32_t                 State;                  /*!< ADC communication state (bitmap of ADC states) */

  __IO uint32_t                 ErrorCode;              /*!< ADC Error code */
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
#define HAL_ADC_ERROR_NONE        (0x00U)   /*!< No error                                              */
#define HAL_ADC_ERROR_INTERNAL    (0x01U)   /*!< ADC IP internal error: if problem of clocking, 
                                                          enable/disable, erroneous state                       */
#define HAL_ADC_ERROR_OVR         (0x02U)   /*!< Overrun error                                         */
#define HAL_ADC_ERROR_DMA         (0x04U)   /*!< DMA transfer error                                    */
/**
  * @}
  */

/** @defgroup ADC_ClockPrescaler ADC ClockPrescaler
  * @{
  */
#define ADC_CLOCK_ASYNC_DIV1          (0x00000000U)                   /*!< ADC asynchronous clock derived from ADC dedicated HSI without prescaler */
#define ADC_CLOCK_ASYNC_DIV2          ((uint32_t)ADC_CCR_ADCPRE_0)    /*!< ADC asynchronous clock derived from ADC dedicated HSI divided by a prescaler of 2 */
#define ADC_CLOCK_ASYNC_DIV4          ((uint32_t)ADC_CCR_ADCPRE_1)    /*!< ADC asynchronous clock derived from ADC dedicated HSI divided by a prescaler of 4 */
/**
  * @}
  */

/** @defgroup ADC_Resolution ADC Resolution
  * @{
  */
#define ADC_RESOLUTION_12B      (0x00000000U)                   /*!<  ADC 12-bit resolution */
#define ADC_RESOLUTION_10B      ((uint32_t)ADC_CR1_RES_0)       /*!<  ADC 10-bit resolution */
#define ADC_RESOLUTION_8B       ((uint32_t)ADC_CR1_RES_1)       /*!<  ADC 8-bit resolution */
#define ADC_RESOLUTION_6B       ((uint32_t)ADC_CR1_RES)         /*!<  ADC 6-bit resolution */
/**
  * @}
  */

/** @defgroup ADC_Data_align ADC Data_align
  * @{
  */
#define ADC_DATAALIGN_RIGHT      (0x00000000U)
#define ADC_DATAALIGN_LEFT       ((uint32_t)ADC_CR2_ALIGN)
/**
  * @}
  */

/** @defgroup ADC_Scan_mode ADC Scan mode
  * @{
  */
#define ADC_SCAN_DISABLE         (0x00000000U)
#define ADC_SCAN_ENABLE          ((uint32_t)ADC_CR1_SCAN)
/**
  * @}
  */

/** @defgroup ADC_External_trigger_edge_Regular ADC external trigger enable for regular group
  * @{
  */
#define ADC_EXTERNALTRIGCONVEDGE_NONE           (0x00000000U)
#define ADC_EXTERNALTRIGCONVEDGE_RISING         ((uint32_t)ADC_CR2_EXTEN_0)
#define ADC_EXTERNALTRIGCONVEDGE_FALLING        ((uint32_t)ADC_CR2_EXTEN_1)
#define ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING  ((uint32_t)ADC_CR2_EXTEN)
/**
  * @}
  */

/** @defgroup ADC_External_trigger_source_Regular ADC External trigger source Regular
  * @{
  */
/* List of external triggers with generic trigger name, sorted by trigger     */
/* name:                                                                      */

/* External triggers of regular group for ADC1 */
#define ADC_EXTERNALTRIGCONV_T2_CC3      ADC_EXTERNALTRIG_T2_CC3
#define ADC_EXTERNALTRIGCONV_T2_CC2      ADC_EXTERNALTRIG_T2_CC2
#define ADC_EXTERNALTRIGCONV_T2_TRGO     ADC_EXTERNALTRIG_T2_TRGO
#define ADC_EXTERNALTRIGCONV_T3_CC1      ADC_EXTERNALTRIG_T3_CC1
#define ADC_EXTERNALTRIGCONV_T3_CC3      ADC_EXTERNALTRIG_T3_CC3
#define ADC_EXTERNALTRIGCONV_T3_TRGO     ADC_EXTERNALTRIG_T3_TRGO
#define ADC_EXTERNALTRIGCONV_T4_CC4      ADC_EXTERNALTRIG_T4_CC4
#define ADC_EXTERNALTRIGCONV_T4_TRGO     ADC_EXTERNALTRIG_T4_TRGO
#define ADC_EXTERNALTRIGCONV_T6_TRGO     ADC_EXTERNALTRIG_T6_TRGO
#define ADC_EXTERNALTRIGCONV_T9_CC2      ADC_EXTERNALTRIG_T9_CC2
#define ADC_EXTERNALTRIGCONV_T9_TRGO     ADC_EXTERNALTRIG_T9_TRGO
#define ADC_EXTERNALTRIGCONV_EXT_IT11    ADC_EXTERNALTRIG_EXT_IT11
#define ADC_SOFTWARE_START               (0x00000010U)
/**
  * @}
  */

/** @defgroup ADC_EOCSelection ADC EOCSelection
  * @{
  */
#define ADC_EOC_SEQ_CONV            (0x00000000U)
#define ADC_EOC_SINGLE_CONV         ((uint32_t)ADC_CR2_EOCS)
/**
  * @}
  */

/** @defgroup ADC_LowPowerAutoWait ADC LowPowerAutoWait
  * @{
  */
/*!< Note : For compatibility with other STM32 devices with ADC autowait      */
/* feature limited to enable or disable settings:                             */
/* Setting "ADC_AUTOWAIT_UNTIL_DATA_READ" is equivalent to "ENABLE".          */

#define ADC_AUTOWAIT_DISABLE                (0x00000000U)
#define ADC_AUTOWAIT_UNTIL_DATA_READ        ((uint32_t)(                                  ADC_CR2_DELS_0)) /*!< Insert a delay between ADC conversions: infinite delay, until the result of previous conversion is read */
#define ADC_AUTOWAIT_7_APBCLOCKCYCLES       ((uint32_t)(                 ADC_CR2_DELS_1                 )) /*!< Insert a delay between ADC conversions: 7 APB clock cycles */
#define ADC_AUTOWAIT_15_APBCLOCKCYCLES      ((uint32_t)(                 ADC_CR2_DELS_1 | ADC_CR2_DELS_0)) /*!< Insert a delay between ADC conversions: 15 APB clock cycles */
#define ADC_AUTOWAIT_31_APBCLOCKCYCLES      ((uint32_t)(ADC_CR2_DELS_2                                  )) /*!< Insert a delay between ADC conversions: 31 APB clock cycles */
#define ADC_AUTOWAIT_63_APBCLOCKCYCLES      ((uint32_t)(ADC_CR2_DELS_2                  | ADC_CR2_DELS_0)) /*!< Insert a delay between ADC conversions: 63 APB clock cycles */
#define ADC_AUTOWAIT_127_APBCLOCKCYCLES     ((uint32_t)(ADC_CR2_DELS_2 | ADC_CR2_DELS_1                 )) /*!< Insert a delay between ADC conversions: 127 APB clock cycles */
#define ADC_AUTOWAIT_255_APBCLOCKCYCLES     ((uint32_t)(ADC_CR2_DELS_2 | ADC_CR2_DELS_1 | ADC_CR2_DELS_0)) /*!< Insert a delay between ADC conversions: 255 APB clock cycles */

/**
  * @}
  */

/** @defgroup ADC_LowPowerAutoPowerOff ADC LowPowerAutoPowerOff
  * @{
  */
#define ADC_AUTOPOWEROFF_DISABLE            (0x00000000U)
#define ADC_AUTOPOWEROFF_IDLE_PHASE         ((uint32_t)ADC_CR1_PDI)                     /*!< ADC power off when ADC is not converting (idle phase) */
#define ADC_AUTOPOWEROFF_DELAY_PHASE        ((uint32_t)ADC_CR1_PDD)                     /*!< ADC power off when a delay is inserted between conversions (see parameter ADC_LowPowerAutoWait) */
#define ADC_AUTOPOWEROFF_IDLE_DELAY_PHASES  ((uint32_t)(ADC_CR1_PDI | ADC_CR1_PDD))     /*!< ADC power off when ADC is not converting (idle phase) and when a delay is inserted between conversions */
/**
  * @}
  */


/** @defgroup ADC_ChannelsBank ADC ChannelsBank
  * @{
  */
#if defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC) || defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define ADC_CHANNELS_BANK_A                 (0x00000000U)
#define ADC_CHANNELS_BANK_B                 ((uint32_t)ADC_CR2_CFG)

#define IS_ADC_CHANNELSBANK(BANK) (((BANK) == ADC_CHANNELS_BANK_A) || \
                                   ((BANK) == ADC_CHANNELS_BANK_B)   )
#else
#define ADC_CHANNELS_BANK_A                 (0x00000000U)

#define IS_ADC_CHANNELSBANK(BANK) (((BANK) == ADC_CHANNELS_BANK_A))
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
/**
  * @}
  */

/** @defgroup ADC_channels ADC channels
  * @{
  */
/* Note: Depending on devices, some channels may not be available on package  */
/*       pins. Refer to device datasheet for channels availability.           */
#define ADC_CHANNEL_0           (0x00000000U)                                                                                     /* Channel different in bank A and bank B */
#define ADC_CHANNEL_1           ((uint32_t)(                                                                    ADC_SQR5_SQ1_0))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_2           ((uint32_t)(                                                   ADC_SQR5_SQ1_1                 ))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_3           ((uint32_t)(                                                   ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_4           ((uint32_t)(                                  ADC_SQR5_SQ1_2                                  ))  /* Direct (fast) channel */
#define ADC_CHANNEL_5           ((uint32_t)(                                  ADC_SQR5_SQ1_2                  | ADC_SQR5_SQ1_0))  /* Direct (fast) channel */
#define ADC_CHANNEL_6           ((uint32_t)(                                  ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1                 ))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_7           ((uint32_t)(                                  ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_8           ((uint32_t)(                 ADC_SQR5_SQ1_3                                                   ))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_9           ((uint32_t)(                 ADC_SQR5_SQ1_3                                   | ADC_SQR5_SQ1_0))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_10          ((uint32_t)(                 ADC_SQR5_SQ1_3                  | ADC_SQR5_SQ1_1                 ))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_11          ((uint32_t)(                 ADC_SQR5_SQ1_3                  | ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_12          ((uint32_t)(                 ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2                                  ))  /* Channel different in bank A and bank B */
#define ADC_CHANNEL_13          ((uint32_t)(                 ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2                  | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_14          ((uint32_t)(                 ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1                 ))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_15          ((uint32_t)(                 ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_16          ((uint32_t)(ADC_SQR5_SQ1_4                                                                    ))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_17          ((uint32_t)(ADC_SQR5_SQ1_4                                                    | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_18          ((uint32_t)(ADC_SQR5_SQ1_4                                   | ADC_SQR5_SQ1_1                 ))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_19          ((uint32_t)(ADC_SQR5_SQ1_4                                   | ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_20          ((uint32_t)(ADC_SQR5_SQ1_4                  | ADC_SQR5_SQ1_2                                  ))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_21          ((uint32_t)(ADC_SQR5_SQ1_4                  | ADC_SQR5_SQ1_2                  | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_22          ((uint32_t)(ADC_SQR5_SQ1_4                  | ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1                 ))  /* Direct (fast) channel */
#define ADC_CHANNEL_23          ((uint32_t)(ADC_SQR5_SQ1_4                  | ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Direct (fast) channel */
#define ADC_CHANNEL_24          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3                                                   ))  /* Direct (fast) channel */
#define ADC_CHANNEL_25          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3                                   | ADC_SQR5_SQ1_0))  /* Direct (fast) channel */
#define ADC_CHANNEL_26          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3                  | ADC_SQR5_SQ1_1                 ))  /* Channel common to both bank A and bank B */
#if defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define ADC_CHANNEL_27          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3                  | ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_28          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2                                  ))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_29          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2                  | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_30          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1                 ))  /* Channel common to both bank A and bank B */
#define ADC_CHANNEL_31          ((uint32_t)(ADC_SQR5_SQ1_4 | ADC_SQR5_SQ1_3 | ADC_SQR5_SQ1_2 | ADC_SQR5_SQ1_1 | ADC_SQR5_SQ1_0))  /* Channel common to both bank A and bank B */
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#define ADC_CHANNEL_TEMPSENSOR  ADC_CHANNEL_16  /* ADC internal channel (no connection on device pin). Channel common to both bank A and bank B. */
#define ADC_CHANNEL_VREFINT     ADC_CHANNEL_17  /* ADC internal channel (no connection on device pin). Channel common to both bank A and bank B. */
#define ADC_CHANNEL_VCOMP       ADC_CHANNEL_26  /* ADC internal channel (no connection on device pin). Channel common to both bank A and bank B. */

#if defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC) || defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define ADC_CHANNEL_VOPAMP1     ADC_CHANNEL_3   /* Internal connection from OPAMP1 output to ADC switch matrix */
#define ADC_CHANNEL_VOPAMP2     ADC_CHANNEL_8   /* Internal connection from OPAMP2 output to ADC switch matrix */
#if defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD)
#define ADC_CHANNEL_VOPAMP3     ADC_CHANNEL_13  /* Internal connection from OPAMP3 output to ADC switch matrix */
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD */
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
/**
  * @}
  */

/** @defgroup ADC_sampling_times ADC sampling times
  * @{
  */
#define ADC_SAMPLETIME_4CYCLES      (0x00000000U)                                     /*!< Sampling time 4 ADC clock cycles */
#define ADC_SAMPLETIME_9CYCLES      ((uint32_t) ADC_SMPR3_SMP0_0)                     /*!< Sampling time 9 ADC clock cycles */
#define ADC_SAMPLETIME_16CYCLES     ((uint32_t) ADC_SMPR3_SMP0_1)                     /*!< Sampling time 16 ADC clock cycles */
#define ADC_SAMPLETIME_24CYCLES     ((uint32_t)(ADC_SMPR3_SMP0_1 | ADC_SMPR3_SMP0_0)) /*!< Sampling time 24 ADC clock cycles */
#define ADC_SAMPLETIME_48CYCLES     ((uint32_t) ADC_SMPR3_SMP0_2)                     /*!< Sampling time 48 ADC clock cycles */
#define ADC_SAMPLETIME_96CYCLES     ((uint32_t)(ADC_SMPR3_SMP0_2 | ADC_SMPR3_SMP0_0)) /*!< Sampling time 96 ADC clock cycles */
#define ADC_SAMPLETIME_192CYCLES    ((uint32_t)(ADC_SMPR3_SMP0_2 | ADC_SMPR3_SMP0_1)) /*!< Sampling time 192 ADC clock cycles */
#define ADC_SAMPLETIME_384CYCLES    ((uint32_t) ADC_SMPR3_SMP0)                       /*!< Sampling time 384 ADC clock cycles */
/**
  * @}
  */

/** @defgroup ADC_sampling_times_all_channels ADC sampling times all channels
  * @{
  */
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR3BIT2                                          \
     (ADC_SMPR3_SMP9_2 | ADC_SMPR3_SMP8_2 | ADC_SMPR3_SMP7_2 | ADC_SMPR3_SMP6_2 |     \
      ADC_SMPR3_SMP5_2 | ADC_SMPR3_SMP4_2 | ADC_SMPR3_SMP3_2 | ADC_SMPR3_SMP2_2 |     \
      ADC_SMPR3_SMP1_2 | ADC_SMPR3_SMP0_2)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR2BIT2                                          \
     (ADC_SMPR2_SMP19_2 | ADC_SMPR2_SMP18_2 | ADC_SMPR2_SMP17_2 | ADC_SMPR2_SMP16_2 | \
      ADC_SMPR2_SMP15_2 | ADC_SMPR2_SMP14_2 | ADC_SMPR2_SMP13_2 | ADC_SMPR2_SMP12_2 | \
      ADC_SMPR2_SMP11_2 | ADC_SMPR2_SMP10_2)
#if defined(STM32L100xB) || defined (STM32L151xB) || defined (STM32L152xB) || defined(STM32L100xBA) || defined (STM32L151xBA) || defined (STM32L152xBA) || defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR1BIT2                                          \
     (ADC_SMPR1_SMP26_2 | ADC_SMPR1_SMP25_2 | ADC_SMPR1_SMP24_2 | ADC_SMPR1_SMP23_2 | \
      ADC_SMPR1_SMP22_2 | ADC_SMPR1_SMP21_2 | ADC_SMPR1_SMP20_2)
#endif /* STM32L100xB || STM32L151xB || STM32L152xB || STM32L100xBA || STM32L151xBA || STM32L152xBA || STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC */
#if defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR1BIT2                                          \
     (ADC_SMPR1_SMP29_2 | ADC_SMPR1_SMP28_2 | ADC_SMPR1_SMP27_2 | ADC_SMPR1_SMP26_2 | \
      ADC_SMPR1_SMP25_2 | ADC_SMPR1_SMP24_2 | ADC_SMPR1_SMP23_2 | ADC_SMPR1_SMP22_2 | \
      ADC_SMPR1_SMP21_2 | ADC_SMPR1_SMP20_2)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR0BIT2                                          \
     (ADC_SMPR0_SMP31_2 | ADC_SMPR0_SMP30_2 )
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
       
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR3BIT1                                          \
     (ADC_SMPR3_SMP9_1 | ADC_SMPR3_SMP8_1 | ADC_SMPR3_SMP7_1 | ADC_SMPR3_SMP6_1 |     \
      ADC_SMPR3_SMP5_1 | ADC_SMPR3_SMP4_1 | ADC_SMPR3_SMP3_1 | ADC_SMPR3_SMP2_1 |     \
      ADC_SMPR3_SMP1_1 | ADC_SMPR3_SMP0_1)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR2BIT1                                          \
     (ADC_SMPR2_SMP19_1 | ADC_SMPR2_SMP18_1 | ADC_SMPR2_SMP17_1 | ADC_SMPR2_SMP16_1 | \
      ADC_SMPR2_SMP15_1 | ADC_SMPR2_SMP14_1 | ADC_SMPR2_SMP13_1 | ADC_SMPR2_SMP12_1 | \
      ADC_SMPR2_SMP11_1 | ADC_SMPR2_SMP10_1)
#if defined(STM32L100xB) || defined (STM32L151xB) || defined (STM32L152xB) || defined(STM32L100xBA) || defined (STM32L151xBA) || defined (STM32L152xBA) || defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR1BIT1                                          \
     (ADC_SMPR1_SMP26_1 | ADC_SMPR1_SMP25_1 | ADC_SMPR1_SMP24_1 | ADC_SMPR1_SMP23_1 | \
      ADC_SMPR1_SMP22_1 | ADC_SMPR1_SMP21_1 | ADC_SMPR1_SMP20_1)
#endif /* STM32L100xB || STM32L151xB || STM32L152xB || STM32L100xBA || STM32L151xBA || STM32L152xBA || STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC */
#if defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR1BIT1                                          \
     (ADC_SMPR1_SMP29_1 | ADC_SMPR1_SMP28_1 | ADC_SMPR1_SMP27_1 | ADC_SMPR1_SMP26_1 | \
      ADC_SMPR1_SMP25_1 | ADC_SMPR1_SMP24_1 | ADC_SMPR1_SMP23_1 | ADC_SMPR1_SMP22_1 | \
      ADC_SMPR1_SMP21_1 | ADC_SMPR1_SMP20_1)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR0BIT1                                          \
     (ADC_SMPR0_SMP31_1 | ADC_SMPR0_SMP30_1 )
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
       
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR3BIT0                                          \
     (ADC_SMPR3_SMP9_0 | ADC_SMPR3_SMP8_0 | ADC_SMPR3_SMP7_0 | ADC_SMPR3_SMP6_0 |     \
      ADC_SMPR3_SMP5_0 | ADC_SMPR3_SMP4_0 | ADC_SMPR3_SMP3_0 | ADC_SMPR3_SMP2_0 |     \
      ADC_SMPR3_SMP1_0 | ADC_SMPR3_SMP0_0)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR2BIT0                                          \
     (ADC_SMPR2_SMP19_0 | ADC_SMPR2_SMP18_0 | ADC_SMPR2_SMP17_0 | ADC_SMPR2_SMP16_0 | \
      ADC_SMPR2_SMP15_0 | ADC_SMPR2_SMP14_0 | ADC_SMPR2_SMP13_0 | ADC_SMPR2_SMP12_0 | \
      ADC_SMPR2_SMP11_0 | ADC_SMPR2_SMP10_0)
#if defined(STM32L100xB) || defined (STM32L151xB) || defined (STM32L152xB) || defined(STM32L100xBA) || defined (STM32L151xBA) || defined (STM32L152xBA) || defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR1BIT0                                          \
     (ADC_SMPR1_SMP26_0 | ADC_SMPR1_SMP25_0 | ADC_SMPR1_SMP24_0 | ADC_SMPR1_SMP23_0 | \
      ADC_SMPR1_SMP22_0 | ADC_SMPR1_SMP21_0 | ADC_SMPR1_SMP20_0)
#endif /* STM32L100xB || STM32L151xB || STM32L152xB || STM32L100xBA || STM32L151xBA || STM32L152xBA || STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC */
#if defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR1BIT0                                          \
     (ADC_SMPR1_SMP29_0 | ADC_SMPR1_SMP28_0 | ADC_SMPR1_SMP27_0 | ADC_SMPR1_SMP26_0 | \
      ADC_SMPR1_SMP25_0 | ADC_SMPR1_SMP24_0 | ADC_SMPR1_SMP23_0 | ADC_SMPR1_SMP22_0 | \
      ADC_SMPR1_SMP21_0 | ADC_SMPR1_SMP20_0)
#define ADC_SAMPLETIME_ALLCHANNELS_SMPR0BIT0                                          \
     (ADC_SMPR0_SMP31_0 | ADC_SMPR0_SMP30_0 )
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
/**
  * @}
  */

/** @defgroup ADC_regular_rank ADC rank into regular group
  * @{
  */
#define ADC_REGULAR_RANK_1    (0x00000001U)
#define ADC_REGULAR_RANK_2    (0x00000002U)
#define ADC_REGULAR_RANK_3    (0x00000003U)
#define ADC_REGULAR_RANK_4    (0x00000004U)
#define ADC_REGULAR_RANK_5    (0x00000005U)
#define ADC_REGULAR_RANK_6    (0x00000006U)
#define ADC_REGULAR_RANK_7    (0x00000007U)
#define ADC_REGULAR_RANK_8    (0x00000008U)
#define ADC_REGULAR_RANK_9    (0x00000009U)
#define ADC_REGULAR_RANK_10   (0x0000000AU)
#define ADC_REGULAR_RANK_11   (0x0000000BU)
#define ADC_REGULAR_RANK_12   (0x0000000CU)
#define ADC_REGULAR_RANK_13   (0x0000000DU)
#define ADC_REGULAR_RANK_14   (0x0000000EU)
#define ADC_REGULAR_RANK_15   (0x0000000FU)
#define ADC_REGULAR_RANK_16   (0x00000010U)
#define ADC_REGULAR_RANK_17   (0x00000011U)
#define ADC_REGULAR_RANK_18   (0x00000012U)
#define ADC_REGULAR_RANK_19   (0x00000013U)
#define ADC_REGULAR_RANK_20   (0x00000014U)
#define ADC_REGULAR_RANK_21   (0x00000015U)
#define ADC_REGULAR_RANK_22   (0x00000016U)
#define ADC_REGULAR_RANK_23   (0x00000017U)
#define ADC_REGULAR_RANK_24   (0x00000018U)
#define ADC_REGULAR_RANK_25   (0x00000019U)
#define ADC_REGULAR_RANK_26   (0x0000001AU)
#define ADC_REGULAR_RANK_27   (0x0000001BU)
#if defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC) || defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define ADC_REGULAR_RANK_28   (0x0000001CU)
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */
/**
  * @}
  */

/** @defgroup ADC_analog_watchdog_mode ADC analog watchdog mode
  * @{
  */
#define ADC_ANALOGWATCHDOG_NONE                 (0x00000000U)
#define ADC_ANALOGWATCHDOG_SINGLE_REG           ((uint32_t)(ADC_CR1_AWDSGL | ADC_CR1_AWDEN))
#define ADC_ANALOGWATCHDOG_SINGLE_INJEC         ((uint32_t)(ADC_CR1_AWDSGL | ADC_CR1_JAWDEN))
#define ADC_ANALOGWATCHDOG_SINGLE_REGINJEC      ((uint32_t)(ADC_CR1_AWDSGL | ADC_CR1_AWDEN | ADC_CR1_JAWDEN))
#define ADC_ANALOGWATCHDOG_ALL_REG              ((uint32_t) ADC_CR1_AWDEN)
#define ADC_ANALOGWATCHDOG_ALL_INJEC            ((uint32_t) ADC_CR1_JAWDEN)
#define ADC_ANALOGWATCHDOG_ALL_REGINJEC         ((uint32_t)(ADC_CR1_AWDEN | ADC_CR1_JAWDEN))
/**
  * @}
  */

/** @defgroup ADC_conversion_group ADC conversion group
  * @{
  */
#define ADC_REGULAR_GROUP             ((uint32_t)(ADC_FLAG_EOC))
#define ADC_INJECTED_GROUP            ((uint32_t)(ADC_FLAG_JEOC))
#define ADC_REGULAR_INJECTED_GROUP    ((uint32_t)(ADC_FLAG_EOC | ADC_FLAG_JEOC))
/**
  * @}
  */

/** @defgroup ADC_Event_type ADC Event type
  * @{
  */
#define ADC_AWD_EVENT               ((uint32_t)ADC_FLAG_AWD)   /*!< ADC Analog watchdog event */
#define ADC_OVR_EVENT               ((uint32_t)ADC_FLAG_OVR)   /*!< ADC overrun event */
/**
  * @}
  */

/** @defgroup ADC_interrupts_definition ADC interrupts definition
  * @{
  */
#define ADC_IT_EOC           ADC_CR1_EOCIE        /*!< ADC End of Regular Conversion interrupt source */
#define ADC_IT_JEOC          ADC_CR1_JEOCIE       /*!< ADC End of Injected Conversion interrupt source */
#define ADC_IT_AWD           ADC_CR1_AWDIE        /*!< ADC Analog watchdog interrupt source */
#define ADC_IT_OVR           ADC_CR1_OVRIE        /*!< ADC overrun interrupt source */
/**
  * @}
  */

/** @defgroup ADC_flags_definition ADC flags definition
  * @{
  */
#define ADC_FLAG_AWD           ADC_SR_AWD      /*!< ADC Analog watchdog flag */
#define ADC_FLAG_EOC           ADC_SR_EOC      /*!< ADC End of Regular conversion flag */
#define ADC_FLAG_JEOC          ADC_SR_JEOC     /*!< ADC End of Injected conversion flag */
#define ADC_FLAG_JSTRT         ADC_SR_JSTRT    /*!< ADC Injected group start flag */
#define ADC_FLAG_STRT          ADC_SR_STRT     /*!< ADC Regular group start flag */
#define ADC_FLAG_OVR           ADC_SR_OVR      /*!< ADC overrun flag */
#define ADC_FLAG_ADONS         ADC_SR_ADONS    /*!< ADC ready status flag */
#define ADC_FLAG_RCNR          ADC_SR_RCNR     /*!< ADC Regular group ready status flag */
#define ADC_FLAG_JCNR          ADC_SR_JCNR     /*!< ADC Injected group ready status flag */
/**
  * @}
  */

/**
  * @}
  */ 


/* Private constants ---------------------------------------------------------*/

/** @addtogroup ADC_Private_Constants ADC Private Constants
  * @{
  */

/* List of external triggers of regular group for ADC1:                       */
/* (used internally by HAL driver. To not use into HAL structure parameters)  */

/* External triggers of regular group for ADC1 */
#define ADC_EXTERNALTRIG_T9_CC2         (0x00000000U)
#define ADC_EXTERNALTRIG_T9_TRGO        ((uint32_t)(                                                         ADC_CR2_EXTSEL_0))
#define ADC_EXTERNALTRIG_T2_CC3         ((uint32_t)(                                      ADC_CR2_EXTSEL_1                   ))
#define ADC_EXTERNALTRIG_T2_CC2         ((uint32_t)(                                      ADC_CR2_EXTSEL_1 | ADC_CR2_EXTSEL_0))
#define ADC_EXTERNALTRIG_T3_TRGO        ((uint32_t)(                   ADC_CR2_EXTSEL_2                                      ))
#define ADC_EXTERNALTRIG_T4_CC4         ((uint32_t)(                   ADC_CR2_EXTSEL_2 |                    ADC_CR2_EXTSEL_0))
#define ADC_EXTERNALTRIG_T2_TRGO        ((uint32_t)(                   ADC_CR2_EXTSEL_2 | ADC_CR2_EXTSEL_1                   ))
#define ADC_EXTERNALTRIG_T3_CC1         ((uint32_t)(                   ADC_CR2_EXTSEL_2 | ADC_CR2_EXTSEL_1 | ADC_CR2_EXTSEL_0))
#define ADC_EXTERNALTRIG_T3_CC3         ((uint32_t)(ADC_CR2_EXTSEL_3                                                         ))
#define ADC_EXTERNALTRIG_T4_TRGO        ((uint32_t)(ADC_CR2_EXTSEL_3                                       | ADC_CR2_EXTSEL_0))
#define ADC_EXTERNALTRIG_T6_TRGO        ((uint32_t)(ADC_CR2_EXTSEL_3                    | ADC_CR2_EXTSEL_1                   ))
#define ADC_EXTERNALTRIG_EXT_IT11       ((uint32_t)(ADC_CR2_EXTSEL_3 | ADC_CR2_EXTSEL_2 | ADC_CR2_EXTSEL_1 | ADC_CR2_EXTSEL_0))

/* Combination of all post-conversion flags bits: EOC/EOS, JEOC/JEOS, OVR, AWDx */
#define ADC_FLAG_POSTCONV_ALL   (ADC_FLAG_EOC | ADC_FLAG_JEOC | ADC_FLAG_AWD | \
                                 ADC_FLAG_OVR)

/**
  * @}
  */


/* Exported macro ------------------------------------------------------------*/

/** @defgroup ADC_Exported_Macros ADC Exported Macros
  * @{
  */
/* Macro for internal HAL driver usage, and possibly can be used into code of */
/* final user.                                                                */

/**
  * @brief Enable the ADC peripheral
  * @param __HANDLE__: ADC handle
  * @retval None
  */
#define __HAL_ADC_ENABLE(__HANDLE__)                                           \
  (__HANDLE__)->Instance->CR2 |= ADC_CR2_ADON

/**
  * @brief Disable the ADC peripheral
  * @param __HANDLE__: ADC handle
  * @retval None
  */
#define __HAL_ADC_DISABLE(__HANDLE__)                                          \
  (__HANDLE__)->Instance->CR2 &= ~ADC_CR2_ADON

/**
  * @brief Enable the ADC end of conversion interrupt.
  * @param __HANDLE__: ADC handle
  * @param __INTERRUPT__: ADC Interrupt
  *          This parameter can be any combination of the following values:
  *            @arg ADC_IT_EOC: ADC End of Regular Conversion interrupt source
  *            @arg ADC_IT_JEOC: ADC End of Injected Conversion interrupt source
  *            @arg ADC_IT_AWD: ADC Analog watchdog interrupt source
  *            @arg ADC_IT_OVR: ADC overrun interrupt source
  * @retval None
  */
#define __HAL_ADC_ENABLE_IT(__HANDLE__, __INTERRUPT__)                         \
  (SET_BIT((__HANDLE__)->Instance->CR1, (__INTERRUPT__)))
    
/**
  * @brief Disable the ADC end of conversion interrupt.
  * @param __HANDLE__: ADC handle
  * @param __INTERRUPT__: ADC Interrupt
  *          This parameter can be any combination of the following values:
  *            @arg ADC_IT_EOC: ADC End of Regular Conversion interrupt source
  *            @arg ADC_IT_JEOC: ADC End of Injected Conversion interrupt source
  *            @arg ADC_IT_AWD: ADC Analog watchdog interrupt source
  *            @arg ADC_IT_OVR: ADC overrun interrupt source
  * @retval None
  */
#define __HAL_ADC_DISABLE_IT(__HANDLE__, __INTERRUPT__)                        \
  (CLEAR_BIT((__HANDLE__)->Instance->CR1, (__INTERRUPT__)))

/** @brief  Checks if the specified ADC interrupt source is enabled or disabled.
  * @param __HANDLE__: ADC handle
  * @param __INTERRUPT__: ADC interrupt source to check
  *          This parameter can be any combination of the following values:
  *            @arg ADC_IT_EOC: ADC End of Regular Conversion interrupt source
  *            @arg ADC_IT_JEOC: ADC End of Injected Conversion interrupt source
  *            @arg ADC_IT_AWD: ADC Analog watchdog interrupt source
  *            @arg ADC_IT_OVR: ADC overrun interrupt source
  * @retval State of interruption (SET or RESET)
  */
#define __HAL_ADC_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)                     \
  (((__HANDLE__)->Instance->CR1 & (__INTERRUPT__)) == (__INTERRUPT__))

/**
  * @brief Get the selected ADC's flag status.
  * @param __HANDLE__: ADC handle
  * @param __FLAG__: ADC flag
  *          This parameter can be any combination of the following values:
  *            @arg ADC_FLAG_STRT: ADC Regular group start flag
  *            @arg ADC_FLAG_JSTRT: ADC Injected group start flag
  *            @arg ADC_FLAG_EOC: ADC End of Regular conversion flag
  *            @arg ADC_FLAG_JEOC: ADC End of Injected conversion flag
  *            @arg ADC_FLAG_AWD: ADC Analog watchdog flag
  *            @arg ADC_FLAG_OVR: ADC overrun flag
  *            @arg ADC_FLAG_ADONS: ADC ready status flag
  *            @arg ADC_FLAG_RCNR: ADC Regular group ready status flag
  *            @arg ADC_FLAG_JCNR: ADC Injected group ready status flag
  * @retval None
  */
#define __HAL_ADC_GET_FLAG(__HANDLE__, __FLAG__)                               \
  ((((__HANDLE__)->Instance->SR) & (__FLAG__)) == (__FLAG__))

/**
  * @brief Clear the ADC's pending flags
  * @param __HANDLE__: ADC handle
  * @param __FLAG__: ADC flag
  *            @arg ADC_FLAG_STRT: ADC Regular group start flag
  *            @arg ADC_FLAG_JSTRT: ADC Injected group start flag
  *            @arg ADC_FLAG_EOC: ADC End of Regular conversion flag
  *            @arg ADC_FLAG_JEOC: ADC End of Injected conversion flag
  *            @arg ADC_FLAG_AWD: ADC Analog watchdog flag
  *            @arg ADC_FLAG_OVR: ADC overrun flag
  *            @arg ADC_FLAG_ADONS: ADC ready status flag
  *            @arg ADC_FLAG_RCNR: ADC Regular group ready status flag
  *            @arg ADC_FLAG_JCNR: ADC Injected group ready status flag
  * @retval None
  */
#define __HAL_ADC_CLEAR_FLAG(__HANDLE__, __FLAG__)                             \
  (((__HANDLE__)->Instance->SR) = ~(__FLAG__))

/** @brief  Reset ADC handle state
  * @param  __HANDLE__: ADC handle
  * @retval None
  */
#define __HAL_ADC_RESET_HANDLE_STATE(__HANDLE__)                               \
  ((__HANDLE__)->State = HAL_ADC_STATE_RESET)
   
/**
  * @}
  */

/* Private macro ------------------------------------------------------------*/

/** @defgroup ADC_Private_Macros ADC Private Macros
  * @{
  */
/* Macro reserved for internal HAL driver usage, not intended to be used in   */
/* code of final user.                                                        */

/**
  * @brief Verification of ADC state: enabled or disabled
  * @param __HANDLE__: ADC handle
  * @retval SET (ADC enabled) or RESET (ADC disabled)
  */
#define ADC_IS_ENABLE(__HANDLE__)                                              \
  ((( ((__HANDLE__)->Instance->SR & ADC_SR_ADONS) == ADC_SR_ADONS )            \
  ) ? SET : RESET)

/**
  * @brief Test if conversion trigger of regular group is software start
  *        or external trigger.
  * @param __HANDLE__: ADC handle
  * @retval SET (software start) or RESET (external trigger)
  */
#define ADC_IS_SOFTWARE_START_REGULAR(__HANDLE__)                              \
  (((__HANDLE__)->Instance->CR2 & ADC_CR2_EXTEN) == RESET)

/**
  * @brief Test if conversion trigger of injected group is software start
  *        or external trigger.
  * @param __HANDLE__: ADC handle
  * @retval SET (software start) or RESET (external trigger)
  */
#define ADC_IS_SOFTWARE_START_INJECTED(__HANDLE__)                             \
  (((__HANDLE__)->Instance->CR2 & ADC_CR2_JEXTEN) == RESET)

/**
  * @brief Simultaneously clears and sets specific bits of the handle State
  * @note: ADC_STATE_CLR_SET() macro is merely aliased to generic macro MODIFY_REG(),
  *        the first parameter is the ADC handle State, the second parameter is the
  *        bit field to clear, the third and last parameter is the bit field to set.
  * @retval None
  */
#define ADC_STATE_CLR_SET MODIFY_REG

/**
  * @brief Clear ADC error code (set it to error code: "no error")
  * @param __HANDLE__: ADC handle
  * @retval None
  */
#define ADC_CLEAR_ERRORCODE(__HANDLE__)                                        \
  ((__HANDLE__)->ErrorCode = HAL_ADC_ERROR_NONE)

/**
  * @brief Set ADC number of ranks into regular channel sequence length.
  * @param _NbrOfConversion_: Regular channel sequence length 
  * @retval None
  */
#define ADC_SQR1_L_SHIFT(_NbrOfConversion_)                                    \
  (((_NbrOfConversion_) - (uint8_t)1) << POSITION_VAL(ADC_SQR1_L))

/**
  * @brief Set the ADC's sample time for channel numbers between 10 and 18.
  * @param _SAMPLETIME_: Sample time parameter.
  * @param _CHANNELNB_: Channel number.  
  * @retval None
  */
#define ADC_SMPR2(_SAMPLETIME_, _CHANNELNB_)                                   \
  ((_SAMPLETIME_) << (3 * ((_CHANNELNB_) - 10)))

/**
  * @brief Set the ADC's sample time for channel numbers between 0 and 9.
  * @param _SAMPLETIME_: Sample time parameter.
  * @param _CHANNELNB_: Channel number.  
  * @retval None
  */
#define ADC_SMPR3(_SAMPLETIME_, _CHANNELNB_)                                   \
  ((_SAMPLETIME_) << (3 * (_CHANNELNB_)))

/**
  * @brief Set the selected regular channel rank for rank between 1 and 6.
  * @param _CHANNELNB_: Channel number.
  * @param _RANKNB_: Rank number.    
  * @retval None
  */
#define ADC_SQR5_RK(_CHANNELNB_, _RANKNB_)                                     \
  ((_CHANNELNB_) << (5 * ((_RANKNB_) - 1)))

/**
  * @brief Set the selected regular channel rank for rank between 7 and 12.
  * @param _CHANNELNB_: Channel number.
  * @param _RANKNB_: Rank number.    
  * @retval None
  */
#define ADC_SQR4_RK(_CHANNELNB_, _RANKNB_)                                     \
  ((_CHANNELNB_) << (5 * ((_RANKNB_) - 7)))

/**
  * @brief Set the selected regular channel rank for rank between 13 and 18.
  * @param _CHANNELNB_: Channel number.
  * @param _RANKNB_: Rank number.    
  * @retval None
  */
#define ADC_SQR3_RK(_CHANNELNB_, _RANKNB_)                                     \
  ((_CHANNELNB_) << (5 * ((_RANKNB_) - 13)))

/**
  * @brief Set the selected regular channel rank for rank between 19 and 24.
  * @param _CHANNELNB_: Channel number.
  * @param _RANKNB_: Rank number.    
  * @retval None
  */
#define ADC_SQR2_RK(_CHANNELNB_, _RANKNB_)                                     \
  ((_CHANNELNB_) << (5 * ((_RANKNB_) - 19)))
      
/**
  * @brief Set the selected regular channel rank for rank between 25 and 28.
  * @param _CHANNELNB_: Channel number.
  * @param _RANKNB_: Rank number.    
  * @retval None
  */
#define ADC_SQR1_RK(_CHANNELNB_, _RANKNB_)                                     \
  ((_CHANNELNB_) << (5 * ((_RANKNB_) - 25)))
      
/**
  * @brief Set the injected sequence length.
  * @param _JSQR_JL_: Sequence length.
  * @retval None
  */
#define ADC_JSQR_JL_SHIFT(_JSQR_JL_)   (((_JSQR_JL_) -1) << 20)

/**
  * @brief Set the selected injected channel rank
  *        Note: on STM32L1 devices, channel rank position in JSQR register
  *              is depending on total number of ranks selected into
  *              injected sequencer (ranks sequence starting from 4-JL)
  * @param _CHANNELNB_: Channel number.
  * @param _RANKNB_: Rank number.
  * @param _JSQR_JL_: Sequence length.
  * @retval None
  */
#define ADC_JSQR_RK_JL(_CHANNELNB_, _RANKNB_, _JSQR_JL_)                       \
  ((_CHANNELNB_) << (5 * ((4 - ((_JSQR_JL_) - (_RANKNB_))) - 1)))

/**
  * @brief Enable the ADC DMA continuous request.
  * @param _DMACONTREQ_MODE_: DMA continuous request mode.
  * @retval None
  */
#define ADC_CR2_DMACONTREQ(_DMACONTREQ_MODE_)                                  \
  ((_DMACONTREQ_MODE_) << POSITION_VAL(ADC_CR2_DDS))

/**
  * @brief Enable ADC continuous conversion mode.
  * @param _CONTINUOUS_MODE_: Continuous mode.
  * @retval None
  */
#define ADC_CR2_CONTINUOUS(_CONTINUOUS_MODE_)                                  \
  ((_CONTINUOUS_MODE_) << POSITION_VAL(ADC_CR2_CONT))

/**
  * @brief Configures the number of discontinuous conversions for the regular group channels.
  * @param _NBR_DISCONTINUOUS_CONV_: Number of discontinuous conversions.
  * @retval None
  */
#define ADC_CR1_DISCONTINUOUS_NUM(_NBR_DISCONTINUOUS_CONV_)                    \
  (((_NBR_DISCONTINUOUS_CONV_) - 1) << POSITION_VAL(ADC_CR1_DISCNUM))

/**
  * @brief Enable ADC scan mode to convert multiple ranks with sequencer.
  * @param _SCAN_MODE_: Scan conversion mode.
  * @retval None
  */
/* Note: Scan mode is compared to ENABLE for legacy purpose, this parameter   */
/*       is equivalent to ADC_SCAN_ENABLE.                                    */
#define ADC_CR1_SCAN_SET(_SCAN_MODE_)                                          \
  (( ((_SCAN_MODE_) == ADC_SCAN_ENABLE) || ((_SCAN_MODE_) == ENABLE)           \
   )? (ADC_SCAN_ENABLE) : (ADC_SCAN_DISABLE)                                   \
  )


#define IS_ADC_CLOCKPRESCALER(ADC_CLOCK) (((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV1) || \
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV2) || \
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV4)   )

#define IS_ADC_RESOLUTION(RESOLUTION) (((RESOLUTION) == ADC_RESOLUTION_12B) || \
                                       ((RESOLUTION) == ADC_RESOLUTION_10B) || \
                                       ((RESOLUTION) == ADC_RESOLUTION_8B)  || \
                                       ((RESOLUTION) == ADC_RESOLUTION_6B)    )

#define IS_ADC_RESOLUTION_8_6_BITS(RESOLUTION) (((RESOLUTION) == ADC_RESOLUTION_8B) || \
                                                ((RESOLUTION) == ADC_RESOLUTION_6B)   )

#define IS_ADC_DATA_ALIGN(ALIGN) (((ALIGN) == ADC_DATAALIGN_RIGHT) || \
                                  ((ALIGN) == ADC_DATAALIGN_LEFT)    )

#define IS_ADC_SCAN_MODE(SCAN_MODE) (((SCAN_MODE) == ADC_SCAN_DISABLE) || \
                                     ((SCAN_MODE) == ADC_SCAN_ENABLE)    )

#define IS_ADC_EXTTRIG_EDGE(EDGE) (((EDGE) == ADC_EXTERNALTRIGCONVEDGE_NONE)         || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_RISING)       || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_FALLING)      || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING)  )

#define IS_ADC_EXTTRIG(REGTRIG) (((REGTRIG) == ADC_EXTERNALTRIGCONV_T2_CC3)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T2_CC2)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T2_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T3_CC1)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T3_CC3)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T3_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T4_CC4)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T4_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T6_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T9_CC2)   || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_T9_TRGO)  || \
                                 ((REGTRIG) == ADC_EXTERNALTRIGCONV_EXT_IT11) || \
                                 ((REGTRIG) == ADC_SOFTWARE_START)              )

#define IS_ADC_EOC_SELECTION(EOC_SELECTION) (((EOC_SELECTION) == ADC_EOC_SINGLE_CONV)    || \
                                             ((EOC_SELECTION) == ADC_EOC_SEQ_CONV)         )

#define IS_ADC_AUTOWAIT(AUTOWAIT) (((AUTOWAIT) == ADC_AUTOWAIT_DISABLE)            || \
                                   ((AUTOWAIT) == ADC_AUTOWAIT_UNTIL_DATA_READ)    || \
                                   ((AUTOWAIT) == ADC_AUTOWAIT_7_APBCLOCKCYCLES)   || \
                                   ((AUTOWAIT) == ADC_AUTOWAIT_15_APBCLOCKCYCLES)  || \
                                   ((AUTOWAIT) == ADC_AUTOWAIT_31_APBCLOCKCYCLES)  || \
                                   ((AUTOWAIT) == ADC_AUTOWAIT_63_APBCLOCKCYCLES)  || \
                                   ((AUTOWAIT) == ADC_AUTOWAIT_127_APBCLOCKCYCLES) || \
                                   ((AUTOWAIT) == ADC_AUTOWAIT_255_APBCLOCKCYCLES)   )

#define IS_ADC_AUTOPOWEROFF(AUTOPOWEROFF) (((AUTOPOWEROFF) == ADC_AUTOPOWEROFF_DISABLE)          || \
                                           ((AUTOPOWEROFF) == ADC_AUTOPOWEROFF_IDLE_PHASE)       || \
                                           ((AUTOPOWEROFF) == ADC_AUTOPOWEROFF_DELAY_PHASE)      || \
                                           ((AUTOPOWEROFF) == ADC_AUTOPOWEROFF_IDLE_DELAY_PHASES)  )

#if defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC) || defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)

#define IS_ADC_CHANNELSBANK(BANK) (((BANK) == ADC_CHANNELS_BANK_A) || \
                                   ((BANK) == ADC_CHANNELS_BANK_B)   )
#else

#define IS_ADC_CHANNELSBANK(BANK) (((BANK) == ADC_CHANNELS_BANK_A))
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#if defined(STM32L100xB) || defined (STM32L151xB) || defined (STM32L152xB) || defined(STM32L100xBA) || defined (STM32L151xBA) || defined (STM32L152xBA) || defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC)
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
                                 ((CHANNEL) == ADC_CHANNEL_19)          || \
                                 ((CHANNEL) == ADC_CHANNEL_20)          || \
                                 ((CHANNEL) == ADC_CHANNEL_21)          || \
                                 ((CHANNEL) == ADC_CHANNEL_22)          || \
                                 ((CHANNEL) == ADC_CHANNEL_23)          || \
                                 ((CHANNEL) == ADC_CHANNEL_24)          || \
                                 ((CHANNEL) == ADC_CHANNEL_25)          || \
                                 ((CHANNEL) == ADC_CHANNEL_26)            )
#endif /* STM32L100xB || STM32L151xB || STM32L152xB || STM32L100xBA || STM32L151xBA || STM32L152xBA || STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC */
#if defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
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
                                 ((CHANNEL) == ADC_CHANNEL_19)          || \
                                 ((CHANNEL) == ADC_CHANNEL_20)          || \
                                 ((CHANNEL) == ADC_CHANNEL_21)          || \
                                 ((CHANNEL) == ADC_CHANNEL_22)          || \
                                 ((CHANNEL) == ADC_CHANNEL_23)          || \
                                 ((CHANNEL) == ADC_CHANNEL_24)          || \
                                 ((CHANNEL) == ADC_CHANNEL_25)          || \
                                 ((CHANNEL) == ADC_CHANNEL_26)          || \
                                 ((CHANNEL) == ADC_CHANNEL_27)          || \
                                 ((CHANNEL) == ADC_CHANNEL_28)          || \
                                 ((CHANNEL) == ADC_CHANNEL_29)          || \
                                 ((CHANNEL) == ADC_CHANNEL_30)          || \
                                 ((CHANNEL) == ADC_CHANNEL_31)            )
#endif /* STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#define IS_ADC_SAMPLE_TIME(TIME) (((TIME) == ADC_SAMPLETIME_4CYCLES)   || \
                                  ((TIME) == ADC_SAMPLETIME_9CYCLES)   || \
                                  ((TIME) == ADC_SAMPLETIME_16CYCLES)  || \
                                  ((TIME) == ADC_SAMPLETIME_24CYCLES)  || \
                                  ((TIME) == ADC_SAMPLETIME_48CYCLES)  || \
                                  ((TIME) == ADC_SAMPLETIME_96CYCLES)  || \
                                  ((TIME) == ADC_SAMPLETIME_192CYCLES) || \
                                  ((TIME) == ADC_SAMPLETIME_384CYCLES)   )

#if defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC) || defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
       
#define IS_ADC_REGULAR_RANK(CHANNEL) (((CHANNEL) == ADC_REGULAR_RANK_1 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_2 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_3 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_4 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_5 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_6 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_7 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_8 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_9 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_10) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_11) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_12) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_13) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_14) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_15) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_16) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_17) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_18) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_19) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_20) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_21) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_22) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_23) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_24) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_25) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_26) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_27) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_28)   )
#else

#define IS_ADC_REGULAR_RANK(CHANNEL) (((CHANNEL) == ADC_REGULAR_RANK_1 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_2 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_3 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_4 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_5 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_6 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_7 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_8 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_9 ) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_10) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_11) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_12) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_13) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_14) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_15) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_16) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_17) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_18) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_19) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_20) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_21) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_22) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_23) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_24) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_25) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_26) || \
                                      ((CHANNEL) == ADC_REGULAR_RANK_27)   )
#endif /* STM32L100xC || STM32L151xC || STM32L152xC || STM32L162xC || STM32L151xCA || STM32L151xD || STM32L152xCA || STM32L152xD || STM32L162xCA || STM32L162xD || STM32L151xE || STM32L151xDX || STM32L152xE || STM32L152xDX || STM32L162xE || STM32L162xDX */

#define IS_ADC_ANALOG_WATCHDOG_MODE(WATCHDOG) (((WATCHDOG) == ADC_ANALOGWATCHDOG_NONE)             || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_SINGLE_REG)       || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_SINGLE_INJEC)     || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_SINGLE_REGINJEC)  || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_ALL_REG)          || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_ALL_INJEC)        || \
                                               ((WATCHDOG) == ADC_ANALOGWATCHDOG_ALL_REGINJEC)       )

#define IS_ADC_CONVERSION_GROUP(CONVERSION) (((CONVERSION) == ADC_REGULAR_GROUP)         || \
                                             ((CONVERSION) == ADC_INJECTED_GROUP)        || \
                                             ((CONVERSION) == ADC_REGULAR_INJECTED_GROUP)  )

#define IS_ADC_EVENT_TYPE(EVENT) (((EVENT) == ADC_AWD_EVENT)  || \
                                  ((EVENT) == ADC_FLAG_OVR) )

/**
  * @brief Verify that a ADC data is within range corresponding to
  *        ADC resolution.
  * @param __RESOLUTION__: ADC resolution (12, 10, 8 or 6 bits).
  * @param __ADC_DATA__: value checked against the resolution.     
  * @retval SET: ADC data is within range corresponding to ADC resolution
  *         RESET: ADC data is not within range corresponding to ADC resolution
  *
  */  
#define IS_ADC_RANGE(__RESOLUTION__, __ADC_DATA__)                                          \
   ((((__RESOLUTION__) == ADC_RESOLUTION_12B) && ((__ADC_DATA__) <= (0x0FFFU))) || \
    (((__RESOLUTION__) == ADC_RESOLUTION_10B) && ((__ADC_DATA__) <= (0x03FFU))) || \
    (((__RESOLUTION__) == ADC_RESOLUTION_8B)  && ((__ADC_DATA__) <= (0x00FFU))) || \
    (((__RESOLUTION__) == ADC_RESOLUTION_6B)  && ((__ADC_DATA__) <= (0x003FU)))   )


#if defined(STM32L100xC) || defined (STM32L151xC) || defined (STM32L152xC) || defined (STM32L162xC) || defined(STM32L151xCA) || defined (STM32L151xD) || defined (STM32L152xCA) || defined (STM32L152xD) || defined (STM32L162xCA) || defined (STM32L162xD) || defined(STM32L151xE) || defined(STM32L151xDX) || defined (STM32L152xE) || defined (STM32L152xDX) || defined (STM32L162xE) || defined (STM32L162xDX)
#define IS_ADC_REGULAR_NB_CONV(LENGTH) (((LENGTH) >= (1U)) && ((LENGTH) <= (28U)))
#else
#define IS_ADC_REGULAR_NB_CONV(LENGTH) (((LENGTH) >= (1U)) && ((LENGTH) <= (27U)))
#endif

#define IS_ADC_REGULAR_DISCONT_NUMBER(NUMBER) (((NUMBER) >= (1U)) && ((NUMBER) <= (8U)))

/**
  * @}
  */
    
    
/* Include ADC HAL Extension module */
#include "stm32l1xx_hal_adc_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup ADC_Exported_Functions
  * @{
  */

/** @addtogroup ADC_Exported_Functions_Group1
  * @{
  */


/* Initialization and de-initialization functions  **********************************/
HAL_StatusTypeDef       HAL_ADC_Init(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef       HAL_ADC_DeInit(ADC_HandleTypeDef *hadc);
void                    HAL_ADC_MspInit(ADC_HandleTypeDef* hadc);
void                    HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc);
/**
  * @}
  */

/* IO operation functions  *****************************************************/

/** @addtogroup ADC_Exported_Functions_Group2
  * @{
  */


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


/* Peripheral Control functions ***********************************************/
/** @addtogroup ADC_Exported_Functions_Group3
  * @{
  */
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


/* Internal HAL driver functions **********************************************/
/** @addtogroup ADC_Private_Functions
  * @{
  */

HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef ADC_ConversionStop_Disable(ADC_HandleTypeDef* hadc);
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


#endif /* __STM32L1xx_HAL_ADC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
