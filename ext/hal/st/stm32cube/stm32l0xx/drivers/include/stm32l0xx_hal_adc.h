/**
  ******************************************************************************
  * @file    stm32l0xx_hal_adc.h
  * @author  MCD Application Team
  * @brief   Header file of ADC HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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
#ifndef __STM32L0xx_HAL_ADC_H
#define __STM32L0xx_HAL_ADC_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal_def.h"

/** @addtogroup STM32L0xx_HAL_Driver
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
  uint32_t ClockPrescaler;        /*!< Select ADC clock source (synchronous clock derived from APB clock or asynchronous clock derived from ADC dedicated HSI RC oscillator) and clock prescaler.
                                       This parameter can be a value of @ref ADC_ClockPrescaler.
                                       Note: In case of synchronous clock mode based on HCLK/1, the configuration must be enabled only
                                             if the system clock has a 50% duty clock cycle (APB prescaler configured inside RCC 
                                             must be bypassed and PCLK clock must have 50% duty cycle). Refer to reference manual for details.
                                       Note: In case of usage of the ADC dedicated HSI RC oscillator, it must be preliminarily enabled at RCC top level.
                                       Note: This parameter can be modified only if the ADC is disabled. */

  uint32_t Resolution;            /*!< Configure the ADC resolution. 
                                       This parameter can be a value of @ref ADC_Resolution */

  uint32_t DataAlign;             /*!< Specify ADC data alignment in conversion data register (right or left).
                                       Refer to reference manual for alignments formats versus resolutions.
                                       This parameter can be a value of @ref ADC_Data_align */

  uint32_t ScanConvMode;          /*!< Configure the sequencer of regular group.
                                       This parameter can be associated to parameter 'DiscontinuousConvMode' to have main sequence subdivided in successive parts.
                                       Sequencer is automatically enabled if several channels are set (sequencer cannot be disabled, as it can be the case on other STM32 devices):
                                       If only 1 channel is set: Conversion is performed in single mode.
                                       If several channels are set:  Conversions are performed in sequence mode (ranks defined by each channel number: channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
                                                                     Scan direction can be set to forward (from channel 0 to channel 18) or backward (from channel 18 to channel 0).
                                       This parameter can be a value of @ref ADC_Scan_mode */

  uint32_t EOCSelection;          /*!< Specify which EOC (End Of Conversion) flag is used for conversion by polling and interruption: end of unitary conversion or end of sequence conversions.
                                       This parameter can be a value of @ref ADC_EOCSelection. */

  uint32_t LowPowerAutoWait;      /*!< Select the dynamic low power Auto Delay: new conversion start only when the previous
                                       conversion (for ADC group regular) has been retrieved by user software,
                                       using function HAL_ADC_GetValue().
                                       This feature automatically adapts the frequency of ADC conversions triggers to the speed of the system that reads the data. Moreover, this avoids risk of overrun
                                       for low frequency applications. 
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: Do not use with interruption or DMA (HAL_ADC_Start_IT(), HAL_ADC_Start_DMA()) since they clear immediately the EOC flag
                                             to free the IRQ vector sequencer.
                                             Do use with polling: 1. Start conversion with HAL_ADC_Start(), 2. Later on, when ADC conversion data is needed:
                                             use HAL_ADC_PollForConversion() to ensure that conversion is completed and HAL_ADC_GetValue() to retrieve conversion result and trig another conversion start. */

  uint32_t LowPowerAutoPowerOff;  /*!< Select the auto-off mode: the ADC automatically powers-off after a conversion and automatically wakes-up when a new conversion is triggered (with startup time between trigger and start of sampling).
                                       This feature can be combined with automatic wait mode (parameter 'LowPowerAutoWait').
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: If enabled, this feature also turns off the ADC dedicated 14 MHz RC oscillator (HSI14) */

  uint32_t ContinuousConvMode;    /*!< Specify whether the conversion is performed in single mode (one conversion) or continuous mode for ADC group regular,
                                       after the first ADC conversion start trigger occurred (software start or external trigger).
                                       This parameter can be set to ENABLE or DISABLE. */

  uint32_t DiscontinuousConvMode; /*!< Specify whether the conversions sequence of ADC group regular is performed in Complete-sequence/Discontinuous-sequence
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

  uint32_t DMAContinuousRequests; /*!< Specify whether the DMA requests are performed in one shot mode (DMA transfer stops when number of conversions is reached)
                                       or in continuous mode (DMA transfer unlimited, whatever number of conversions).
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: In continuous mode, DMA must be configured in circular mode. Otherwise an overrun will be triggered when DMA buffer maximum pointer is reached. */

  uint32_t Overrun;               /*!< Select the behavior in case of overrun: data overwritten or preserved (default).
                                       This parameter can be a value of @ref ADC_Overrun.
                                       Note: In case of overrun set to data preserved and usage with programming model with interruption (HAL_Start_IT()): ADC IRQ handler has to clear 
                                       end of conversion flags, this induces the release of the preserved data. If needed, this data can be saved in function 
                                       HAL_ADC_ConvCpltCallback(), placed in user program code (called before end of conversion flags clear).
                                       Note: Error reporting with respect to the conversion mode:
                                             - Usage with ADC conversion by polling for event or interruption: Error is reported only if overrun is set to data preserved. If overrun is set to data 
                                               overwritten, user can willingly not read all the converted data, this is not considered as an erroneous case.
                                             - Usage with ADC conversion by DMA: Error is reported whatever overrun setting (DMA is expected to process all data from data register). */

  uint32_t LowPowerFrequencyMode; /*!< When selecting an analog ADC clock frequency lower than 2.8MHz,
                                       it is mandatory to first enable the Low Frequency Mode.
                                       This parameter can be set to ENABLE or DISABLE.
                                       Note: This parameter can be modified only if there is no conversion is ongoing. */


  uint32_t SamplingTime;                 /*!< The sample time common to all channels.
                                              Unit: ADC clock cycles
                                              This parameter can be a value of @ref ADC_sampling_times
                                              Note: This parameter can be modified only if there is no conversion ongoing. */

  uint32_t OversamplingMode;              /*!< Specify whether the oversampling feature is enabled or disabled.
                                               This parameter can be set to ENABLE or DISABLE.
                                               Note: This parameter can be modified only if there is no conversion is ongoing on ADC group regular. */


  ADC_OversamplingTypeDef  Oversample;   /*!< Specify the Oversampling parameters
                                              Caution: this setting overwrites the previous oversampling configuration if oversampling is already enabled. */
}ADC_InitTypeDef;

/**
  * @brief  Structure definition of ADC channel for regular group
  * @note   The setting of these parameters by function HAL_ADC_ConfigChannel() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled or enabled without conversion on going on regular group.
  *         If ADC is not in the appropriate state to modify some parameters, these parameters setting is bypassed
  *         without error reporting (as it can be the expected behavior in case of intended action to update another parameter (which fulfills the ADC state condition) on the fly).
  */
typedef struct
{
  uint32_t Channel;                /*!< Specify the channel to configure into ADC regular group.
                                        This parameter can be a value of @ref ADC_channels
                                        Note: Depending on devices, some channels may not be available on device package pins. Refer to device datasheet for channels availability. */

  uint32_t Rank;                   /*!< Add or remove the channel from ADC regular group sequencer. 
                                        On STM32L0 devices,  number of ranks in the sequence is defined by number of channels enabled, rank of each channel is defined by channel number 
                                        (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...).
                                        Despite the channel rank is fixed, this parameter allow an additional possibility: to remove the selected rank (selected channel) from sequencer.
                                        This parameter can be a value of @ref ADC_rank */
}ADC_ChannelConfTypeDef;

/**
  * @brief  Structure definition of ADC analog watchdog
  * @note   The setting of these parameters by function HAL_ADC_AnalogWDGConfig() is conditioned to ADC state.
  *         ADC state can be either:
  *          - For all parameters: ADC disabled or ADC enabled without conversion on going on ADC group regular
  *          - For parameters 'HighThreshold' and 'LowThreshold': ADC enabled with conversion on going on regular group (AWD thresholds can be modify on the fly while ADC conversion is on going)
  */
typedef struct
{
  uint32_t WatchdogMode;      /*!< Configure the ADC analog watchdog mode: single/all channels.
                                   This parameter can be a value of @ref ADC_analog_watchdog_mode */

  uint32_t Channel;           /*!< Select which ADC channel to monitor by analog watchdog.
                                   This parameter has an effect only if watchdog mode is configured on single channel (parameter WatchdogMode)
                                   This parameter can be a value of @ref ADC_channels */

  uint32_t ITMode;            /*!< Specify whether the analog watchdog is configured in interrupt or polling mode.
                                   This parameter can be set to ENABLE or DISABLE */
  uint32_t HighThreshold;     /*!< Configures the ADC analog watchdog High threshold value.
                                   Depending of ADC resolution selected (12, 10, 8 or 6 bits),
                                   this parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF, 0x3FF, 0xFF or 0x3F respectively. */

  uint32_t LowThreshold;      /*!< Configures the ADC analog watchdog High threshold value.
                                   Depending of ADC resolution selected (12, 10, 8 or 6 bits),
                                   this parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF, 0x3FF, 0xFF or 0x3F respectively. */
}ADC_AnalogWDGConfTypeDef;

/**
  * @brief  HAL ADC state machine: ADC states definition (bitfields)
  * @note   ADC state machine is managed by bitfields, state must be compared
  *         with bit by bit.
  *         For example:                                                         
  *           " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_REG_BUSY)) "
  *           " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_AWD1)    ) "
  */
/* States of ADC global scope */
#define HAL_ADC_STATE_RESET             ((uint32_t)0x00000000)    /*!< ADC not yet initialized or disabled */
#define HAL_ADC_STATE_READY             ((uint32_t)0x00000001)    /*!< ADC peripheral ready for use */
#define HAL_ADC_STATE_BUSY_INTERNAL     ((uint32_t)0x00000002)    /*!< ADC is busy due to an internal process (initialization, calibration) */
#define HAL_ADC_STATE_TIMEOUT           ((uint32_t)0x00000004)    /*!< TimeOut occurrence */

/* States of ADC errors */
#define HAL_ADC_STATE_ERROR_INTERNAL    ((uint32_t)0x00000010)    /*!< Internal error occurrence */
#define HAL_ADC_STATE_ERROR_CONFIG      ((uint32_t)0x00000020)    /*!< Configuration error occurrence */
#define HAL_ADC_STATE_ERROR_DMA         ((uint32_t)0x00000040)    /*!< DMA error occurrence */

/* States of ADC group regular */
#define HAL_ADC_STATE_REG_BUSY          ((uint32_t)0x00000100)    /*!< A conversion on ADC group regular is ongoing or can occur (either by continuous mode,
                                                                       external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available)) */
#define HAL_ADC_STATE_REG_EOC           ((uint32_t)0x00000200)    /*!< Conversion data available on group regular */
#define HAL_ADC_STATE_REG_OVR           ((uint32_t)0x00000400)    /*!< Overrun occurrence */
#define HAL_ADC_STATE_REG_EOSMP         ((uint32_t)0x00000800)    /*!< Not available on this STM32 serie: End Of Sampling flag raised  */

/* States of ADC group injected */
#define HAL_ADC_STATE_INJ_BUSY          ((uint32_t)0x00001000)    /*!< Not available on this STM32 serie: A conversion on group injected is ongoing or can occur (either by auto-injection mode,
                                                                       external trigger, low power auto power-on (if feature available), multimode ADC master control (if feature available)) */
#define HAL_ADC_STATE_INJ_EOC           ((uint32_t)0x00002000)    /*!< Not available on this STM32 serie: Conversion data available on group injected */
#define HAL_ADC_STATE_INJ_JQOVF         ((uint32_t)0x00004000)    /*!< Not available on this STM32 serie: Injected queue overflow occurrence */

/* States of ADC analog watchdogs */
#define HAL_ADC_STATE_AWD1              ((uint32_t)0x00010000)    /*!< Out-of-window occurrence of ADC analog watchdog 1 */
#define HAL_ADC_STATE_AWD2              ((uint32_t)0x00020000)    /*!< Not available on this STM32 serie: Out-of-window occurrence of ADC analog watchdog 2 */
#define HAL_ADC_STATE_AWD3              ((uint32_t)0x00040000)    /*!< Not available on this STM32 serie: Out-of-window occurrence of ADC analog watchdog 3 */

/* States of ADC multi-mode */
#define HAL_ADC_STATE_MULTIMODE_SLAVE   ((uint32_t)0x00100000)    /*!< Not available on this STM32 serie: ADC in multimode slave state, controlled by another ADC master (when feature available) */



/** 
  * @brief  ADC handle Structure definition
  */
typedef struct
{
  ADC_TypeDef                   *Instance;              /*!< Register base address */

  ADC_InitTypeDef               Init;                   /*!< ADC required parameters */

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
#define HAL_ADC_ERROR_NONE        ((uint32_t)0x00U)   /*!< No error                                    */
#define HAL_ADC_ERROR_INTERNAL    ((uint32_t)0x01U)   /*!< ADC IP internal error (problem of clocking,
                                                          enable/disable, erroneous state, ...)        */
#define HAL_ADC_ERROR_OVR         ((uint32_t)0x02U)   /*!< Overrun error                               */
#define HAL_ADC_ERROR_DMA         ((uint32_t)0x04U)   /*!< DMA transfer error                          */
/**
  * @}
  */

/** @defgroup ADC_TimeOut_Values ADC TimeOut Values
  * @{
  */

  /* Fixed timeout values for ADC calibration, enable settling time, disable  */
  /* settling time.                                                           */
  /* Values defined to be higher than worst cases: low clocks freq,           */
  /* maximum prescalers.                                                      */
  /* Unit: ms                                                                 */      
#define ADC_ENABLE_TIMEOUT            10U
#define ADC_DISABLE_TIMEOUT           10U
#define ADC_STOP_CONVERSION_TIMEOUT   10U

  /* Delay of 10us fixed to worst case: maximum CPU frequency 180MHz to have  */
  /* the minimum number of CPU cycles to fulfill this delay                   */
  #define ADC_DELAY_10US_MIN_CPU_CYCLES         1800U 
/**
  * @}
  */

/** @defgroup ADC_ClockPrescaler ADC Clock Prescaler
  * @{
  */     
#define ADC_CLOCK_ASYNC_DIV1              ((uint32_t)0x00000000U)                               /*!< ADC Asynchronous clock mode divided by 1 */
#define ADC_CLOCK_ASYNC_DIV2              (ADC_CCR_PRESC_0)                                     /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV4              (ADC_CCR_PRESC_1)                                     /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV6              (ADC_CCR_PRESC_1 | ADC_CCR_PRESC_0)                   /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV8              (ADC_CCR_PRESC_2)                                     /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV10             (ADC_CCR_PRESC_2 | ADC_CCR_PRESC_0)                   /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV12             (ADC_CCR_PRESC_2 | ADC_CCR_PRESC_1)                   /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV16             (ADC_CCR_PRESC_2 | ADC_CCR_PRESC_1 | ADC_CCR_PRESC_0) /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV32             (ADC_CCR_PRESC_3)                                     /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV64             (ADC_CCR_PRESC_3 | ADC_CCR_PRESC_0)                   /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV128            (ADC_CCR_PRESC_3 | ADC_CCR_PRESC_1)                   /*!< ADC Asynchronous clock mode divided by 2 */
#define ADC_CLOCK_ASYNC_DIV256            (ADC_CCR_PRESC_3 | ADC_CCR_PRESC_1 | ADC_CCR_PRESC_0) /*!< ADC Asynchronous clock mode divided by 2 */

#define ADC_CLOCK_SYNC_PCLK_DIV1         ((uint32_t)ADC_CFGR2_CKMODE)    /*!< Synchronous clock mode divided by 1 
                                                                               This configuration must be enabled only if PCLK has a 50%
                                                                               duty clock cycle (APB prescaler configured inside the RCC must be bypassed and the system clock
                                                                               must by 50% duty cycle)*/
#define ADC_CLOCK_SYNC_PCLK_DIV2         ((uint32_t)ADC_CFGR2_CKMODE_0)  /*!< Synchronous clock mode divided by 2 */
#define ADC_CLOCK_SYNC_PCLK_DIV4         ((uint32_t)ADC_CFGR2_CKMODE_1)  /*!< Synchronous clock mode divided by 4 */

/**                                                       
  * @}
  */ 

/** @defgroup ADC_Resolution ADC Resolution
  * @{
  */
#define ADC_RESOLUTION_12B      ((uint32_t)0x00000000U)          /*!< ADC 12-bit resolution */
#define ADC_RESOLUTION_10B      ((uint32_t)ADC_CFGR1_RES_0)      /*!< ADC 10-bit resolution */
#define ADC_RESOLUTION_8B       ((uint32_t)ADC_CFGR1_RES_1)      /*!< ADC 8-bit resolution */
#define ADC_RESOLUTION_6B       ((uint32_t)ADC_CFGR1_RES)        /*!< ADC 6-bit resolution */
/**
  * @}
  */

/** @defgroup ADC_Data_align ADC conversion data alignment
  * @{
  */
#define ADC_DATAALIGN_RIGHT      ((uint32_t)0x00000000U)
#define ADC_DATAALIGN_LEFT       ((uint32_t)ADC_CFGR1_ALIGN)
/**
  * @}
  */

/** @defgroup ADC_regular_external_trigger_edge ADC External Trigger Source Edge for Regular Group
  * @{
  */
#define ADC_EXTERNALTRIGCONVEDGE_NONE           ((uint32_t)0x00000000U)
#define ADC_EXTERNALTRIGCONVEDGE_RISING         ((uint32_t)ADC_CFGR1_EXTEN_0)         
#define ADC_EXTERNALTRIGCONVEDGE_FALLING        ((uint32_t)ADC_CFGR1_EXTEN_1)
#define ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING  ((uint32_t)ADC_CFGR1_EXTEN)
/**
  * @}
  */ 

/** @defgroup ADC_EOCSelection ADC EOC Selection
  * @{
  */ 
#define ADC_EOC_SINGLE_CONV         ((uint32_t) ADC_ISR_EOC)
#define ADC_EOC_SEQ_CONV            ((uint32_t) ADC_ISR_EOS)
#define ADC_EOC_SINGLE_SEQ_CONV     ((uint32_t)(ADC_ISR_EOC | ADC_ISR_EOS))  /*!< reserved for future use */
/**
  * @}
  */ 

/** @defgroup ADC_Overrun ADC Overrun
  * @{
  */ 
#define ADC_OVR_DATA_PRESERVED              ((uint32_t)0x00000000U)
#define ADC_OVR_DATA_OVERWRITTEN            ((uint32_t)ADC_CFGR1_OVRMOD)
/**
  * @}
  */


/** @defgroup ADC_rank ADC rank
  * @{
  */
#define ADC_RANK_CHANNEL_NUMBER                 ((uint32_t)0x00001000U)  /*!< Enable the rank of the selected channels. Number of ranks in the sequence is defined by number of channels enabled, rank of each channel is defined by channel number (channel 0 fixed on rank 0, channel 1 fixed on rank1, ...) */
#define ADC_RANK_NONE                           ((uint32_t)0x00001001U)  /*!< Disable the selected rank (selected channel) from sequencer */
/**
  * @}
  */


/** @defgroup ADC_channels ADC_Channels
  * @{
  */
#define ADC_CHANNEL_0           ((uint32_t)(ADC_CHSELR_CHSEL0))
#define ADC_CHANNEL_1           ((uint32_t)(ADC_CHSELR_CHSEL1) | ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_2           ((uint32_t)(ADC_CHSELR_CHSEL2) | ADC_CFGR1_AWDCH_1)
#define ADC_CHANNEL_3           ((uint32_t)(ADC_CHSELR_CHSEL3)| ADC_CFGR1_AWDCH_1 | ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_4           ((uint32_t)(ADC_CHSELR_CHSEL4)| ADC_CFGR1_AWDCH_2)
#define ADC_CHANNEL_5           ((uint32_t)(ADC_CHSELR_CHSEL5)| ADC_CFGR1_AWDCH_2| ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_6           ((uint32_t)(ADC_CHSELR_CHSEL6)| ADC_CFGR1_AWDCH_2| ADC_CFGR1_AWDCH_1)
#define ADC_CHANNEL_7           ((uint32_t)(ADC_CHSELR_CHSEL7)| ADC_CFGR1_AWDCH_2| ADC_CFGR1_AWDCH_1 | ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_8           ((uint32_t)(ADC_CHSELR_CHSEL8)| ADC_CFGR1_AWDCH_3)
#define ADC_CHANNEL_9           ((uint32_t)(ADC_CHSELR_CHSEL9)| ADC_CFGR1_AWDCH_3| ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_10          ((uint32_t)(ADC_CHSELR_CHSEL10)| ADC_CFGR1_AWDCH_3| ADC_CFGR1_AWDCH_1)
#define ADC_CHANNEL_11          ((uint32_t)(ADC_CHSELR_CHSEL11)| ADC_CFGR1_AWDCH_3| ADC_CFGR1_AWDCH_1| ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_12          ((uint32_t)(ADC_CHSELR_CHSEL12)| ADC_CFGR1_AWDCH_3| ADC_CFGR1_AWDCH_2)
#define ADC_CHANNEL_13          ((uint32_t)(ADC_CHSELR_CHSEL13)| ADC_CFGR1_AWDCH_3| ADC_CFGR1_AWDCH_2| ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_14          ((uint32_t)(ADC_CHSELR_CHSEL14)| ADC_CFGR1_AWDCH_3| ADC_CFGR1_AWDCH_2| ADC_CFGR1_AWDCH_1)
#define ADC_CHANNEL_15          ((uint32_t)(ADC_CHSELR_CHSEL15)| ADC_CFGR1_AWDCH_3| ADC_CFGR1_AWDCH_2| ADC_CFGR1_AWDCH_1| ADC_CFGR1_AWDCH_0)
#if defined (STM32L053xx) || defined (STM32L063xx) || defined (STM32L073xx) || defined (STM32L083xx)
#define ADC_CHANNEL_16          ((uint32_t)(ADC_CHSELR_CHSEL16)| ADC_CFGR1_AWDCH_4)
#endif
#define ADC_CHANNEL_17          ((uint32_t)(ADC_CHSELR_CHSEL17)| ADC_CFGR1_AWDCH_4| ADC_CFGR1_AWDCH_0)
#define ADC_CHANNEL_18          ((uint32_t)(ADC_CHSELR_CHSEL18)| ADC_CFGR1_AWDCH_4| ADC_CFGR1_AWDCH_1)

/* Internal channels */
#if defined (STM32L053xx) || defined (STM32L063xx) || defined (STM32L073xx) || defined (STM32L083xx)
#define ADC_CHANNEL_VLCD         ADC_CHANNEL_16    
#endif
#define ADC_CHANNEL_VREFINT      ADC_CHANNEL_17
#define ADC_CHANNEL_TEMPSENSOR   ADC_CHANNEL_18    
/**
  * @}
  */

/** @defgroup ADC_Channel_AWD_Masks ADC Channel Masks
  * @{
  */
#define ADC_CHANNEL_MASK        ((uint32_t)0x0007FFFFU)
#define ADC_CHANNEL_AWD_MASK    ((uint32_t)0x7C000000U)
/**
  * @}
  */

/** @defgroup ADC_sampling_times ADC Sampling Cycles
  * @{
  */
#define ADC_SAMPLETIME_1CYCLE_5       ((uint32_t)0x00000000U)                         /*!<  ADC sampling time 1.5 cycle */
#define ADC_SAMPLETIME_3CYCLES_5      ((uint32_t)ADC_SMPR_SMPR_0)                     /*!<  ADC sampling time 3.5 CYCLES */
#define ADC_SAMPLETIME_7CYCLES_5      ((uint32_t)ADC_SMPR_SMPR_1)                     /*!<  ADC sampling time 7.5 CYCLES */
#define ADC_SAMPLETIME_12CYCLES_5     ((uint32_t)(ADC_SMPR_SMPR_1 | ADC_SMPR_SMPR_0)) /*!<  ADC sampling time 12.5 CYCLES */
#define ADC_SAMPLETIME_19CYCLES_5     ((uint32_t)ADC_SMPR_SMPR_2)                     /*!<  ADC sampling time 19.5 CYCLES */
#define ADC_SAMPLETIME_39CYCLES_5     ((uint32_t)(ADC_SMPR_SMPR_2 | ADC_SMPR_SMPR_0)) /*!<  ADC sampling time 39.5 CYCLES */
#define ADC_SAMPLETIME_79CYCLES_5     ((uint32_t)(ADC_SMPR_SMPR_2 | ADC_SMPR_SMPR_1)) /*!<  ADC sampling time 79.5 CYCLES */
#define ADC_SAMPLETIME_160CYCLES_5    ((uint32_t)ADC_SMPR_SMPR)                       /*!<  ADC sampling time 160.5 CYCLES */
/**
  * @}
  */

/** @defgroup ADC_Scan_mode ADC Scan mode
  * @{
  */
/* Note: Scan mode values must be compatible with other STM32 devices having  */
/*       a configurable sequencer.                                            */
/*       Scan direction setting values are defined by taking in account       */
/*       already defined values for other STM32 devices:                      */
/*         ADC_SCAN_DISABLE         ((uint32_t)0x00000000)                    */
/*         ADC_SCAN_ENABLE          ((uint32_t)0x00000001)                    */
/*       Scan direction forward is considered as default setting equivalent   */
/*       to scan enable.                                                      */
/*       Scan direction backward is considered as additional setting.         */
/*       In case of migration from another STM32 device, the user will be     */
/*       warned of change of setting choices with assert check.               */
#define ADC_SCAN_DIRECTION_FORWARD        ((uint32_t)0x00000001U)        /*!< Scan direction forward: from channel 0 to channel 18 */
#define ADC_SCAN_DIRECTION_BACKWARD       ((uint32_t)0x00000002U)        /*!< Scan direction backward: from channel 18 to channel 0 */

#define ADC_SCAN_ENABLE         ADC_SCAN_DIRECTION_FORWARD             /* For compatibility with other STM32 devices */
/**
  * @}
  */

/** @defgroup ADC_Oversampling_Ratio ADC Oversampling Ratio
  * @{
  */

#define ADC_OVERSAMPLING_RATIO_2                    ((uint32_t)0x00000000U)  /*!<  ADC Oversampling ratio 2x */
#define ADC_OVERSAMPLING_RATIO_4                    ((uint32_t)0x00000004U)  /*!<  ADC Oversampling ratio 4x */
#define ADC_OVERSAMPLING_RATIO_8                    ((uint32_t)0x00000008U)  /*!<  ADC Oversampling ratio 8x */
#define ADC_OVERSAMPLING_RATIO_16                   ((uint32_t)0x0000000CU)  /*!<  ADC Oversampling ratio 16x */
#define ADC_OVERSAMPLING_RATIO_32                   ((uint32_t)0x00000010U)  /*!<  ADC Oversampling ratio 32x */
#define ADC_OVERSAMPLING_RATIO_64                   ((uint32_t)0x00000014U)  /*!<  ADC Oversampling ratio 64x */
#define ADC_OVERSAMPLING_RATIO_128                  ((uint32_t)0x00000018U)  /*!<  ADC Oversampling ratio 128x */
#define ADC_OVERSAMPLING_RATIO_256                  ((uint32_t)0x0000001CU)  /*!<  ADC Oversampling ratio 256x */
/**
  * @}
  */

/** @defgroup ADC_Right_Bit_Shift ADC Right Bit Shift
  * @{
  */
#define ADC_RIGHTBITSHIFT_NONE                       ((uint32_t)0x00000000U)  /*!<  ADC No bit shift for oversampling */
#define ADC_RIGHTBITSHIFT_1                          ((uint32_t)0x00000020U)  /*!<  ADC 1 bit shift for oversampling */
#define ADC_RIGHTBITSHIFT_2                          ((uint32_t)0x00000040U)  /*!<  ADC 2 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_3                          ((uint32_t)0x00000060U)  /*!<  ADC 3 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_4                          ((uint32_t)0x00000080U)  /*!<  ADC 4 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_5                          ((uint32_t)0x000000A0U)  /*!<  ADC 5 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_6                          ((uint32_t)0x000000C0U)  /*!<  ADC 6 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_7                          ((uint32_t)0x000000E0U)  /*!<  ADC 7 bits shift for oversampling */
#define ADC_RIGHTBITSHIFT_8                          ((uint32_t)0x00000100U)  /*!<  ADC 8 bits shift for oversampling */
/**
  * @}
  */

/** @defgroup ADC_Triggered_Oversampling_Mode ADC Triggered Oversampling Mode
  * @{
  */
#define ADC_TRIGGEREDMODE_SINGLE_TRIGGER            ((uint32_t)0x00000000U)  /*!<  ADC No bit shift for oversampling */
#define ADC_TRIGGEREDMODE_MULTI_TRIGGER             ((uint32_t)0x00000200U)  /*!<  ADC No bit shift for oversampling */
/**
  * @}
  */

/** @defgroup ADC_analog_watchdog_mode ADC Analog Watchdog Mode
  * @{
  */ 
#define ADC_ANALOGWATCHDOG_NONE                     ((uint32_t) 0x00000000U)
#define ADC_ANALOGWATCHDOG_SINGLE_REG               ((uint32_t)(ADC_CFGR1_AWDSGL | ADC_CFGR1_AWDEN))
#define ADC_ANALOGWATCHDOG_ALL_REG                  ((uint32_t) ADC_CFGR1_AWDEN)
/**
  * @}
  */

/** @defgroup ADC_conversion_type ADC Conversion Group
  * @{
  */ 
#define ADC_REGULAR_GROUP                         ((uint32_t)(ADC_FLAG_EOC | ADC_FLAG_EOS))                                              
/**
  * @}
  */

/** @defgroup ADC_Event_type ADC Event
  * @{
  */
#define ADC_AWD_EVENT              ((uint32_t)ADC_FLAG_AWD)
#define ADC_OVR_EVENT              ((uint32_t)ADC_FLAG_OVR)
/**
  * @}
  */
  
/** @defgroup ADC_interrupts_definition ADC Interrupts Definition
  * @{
  */
#define ADC_IT_RDY           ADC_IER_ADRDYIE     /*!< ADC Ready (ADRDY) interrupt source */
#define ADC_IT_EOSMP         ADC_IER_EOSMPIE     /*!< ADC End of Sampling interrupt source */
#define ADC_IT_EOC           ADC_IER_EOCIE       /*!< ADC End of Regular Conversion interrupt source */
#define ADC_IT_EOS           ADC_IER_EOSEQIE     /*!< ADC End of Regular sequence of Conversions interrupt source */
#define ADC_IT_OVR           ADC_IER_OVRIE       /*!< ADC overrun interrupt source */
#define ADC_IT_AWD           ADC_IER_AWDIE       /*!< ADC Analog watchdog 1 interrupt source */
#define ADC_IT_EOCAL         ADC_IER_EOCALIE     /*!< ADC End of Calibration interrupt source */
/**
  * @}
  */

/** @defgroup ADC_flags_definition ADC flags definition
  * @{
  */
#define ADC_FLAG_RDY           ADC_ISR_ADRDY    /*!< ADC Ready flag */
#define ADC_FLAG_EOSMP         ADC_ISR_EOSMP    /*!< ADC End of Sampling flag */
#define ADC_FLAG_EOC           ADC_ISR_EOC      /*!< ADC End of Regular Conversion flag */
#define ADC_FLAG_EOS           ADC_ISR_EOSEQ    /*!< ADC End of Regular sequence of Conversions flag */
#define ADC_FLAG_OVR           ADC_ISR_OVR      /*!< ADC overrun flag */
#define ADC_FLAG_AWD           ADC_ISR_AWD      /*!< ADC Analog watchdog flag */
#define ADC_FLAG_EOCAL         ADC_ISR_EOCAL    /*!< ADC Enf Of Calibration flag */


#define ADC_FLAG_ALL    (ADC_FLAG_RDY | ADC_FLAG_EOSMP | ADC_FLAG_EOC | ADC_FLAG_EOS |  \
                         ADC_FLAG_OVR | ADC_FLAG_AWD   | ADC_FLAG_EOCAL)
/**
  * @}
  */

/**
  * @}
  */


/* Exported macro ------------------------------------------------------------*/

/** @defgroup ADC_Exported_Macros ADC Exported Macros
  * @{
  */
/** @brief Reset ADC handle state
  * @param  __HANDLE__: ADC handle
  * @retval None
  */
#define __HAL_ADC_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_ADC_STATE_RESET)

/**
  * @brief Enable the ADC peripheral
  * @param __HANDLE__: ADC handle
  * @retval None
  */
#define __HAL_ADC_ENABLE(__HANDLE__) ((__HANDLE__)->Instance->CR |= ADC_CR_ADEN)

/**
  * @brief Verification of hardware constraints before ADC can be enabled
  * @param __HANDLE__: ADC handle
  * @retval SET (ADC can be enabled) or RESET (ADC cannot be enabled)
  */
#define ADC_ENABLING_CONDITIONS(__HANDLE__)           \
       (( ( ((__HANDLE__)->Instance->CR) &                  \
            (ADC_CR_ADCAL | ADC_CR_ADSTP | ADC_CR_ADSTART | \
             ADC_CR_ADDIS | ADC_CR_ADEN )                   \
           ) == RESET                                       \
        ) ? SET : RESET)

/**
  * @brief Disable the ADC peripheral
  * @param __HANDLE__: ADC handle
  * @retval None
  */
#define __HAL_ADC_DISABLE(__HANDLE__)                                          \
  do{                                                                          \
         (__HANDLE__)->Instance->CR |= ADC_CR_ADDIS;                           \
          __HAL_ADC_CLEAR_FLAG((__HANDLE__), (ADC_FLAG_EOSMP | ADC_FLAG_RDY)); \
  } while(0)
    
/**
  * @brief Verification of hardware constraints before ADC can be disabled
  * @param __HANDLE__: ADC handle
  * @retval SET (ADC can be disabled) or RESET (ADC cannot be disabled)
  */
#define ADC_DISABLING_CONDITIONS(__HANDLE__)                             \
       (( ( ((__HANDLE__)->Instance->CR) &                                     \
            (ADC_CR_ADSTART | ADC_CR_ADEN)) == ADC_CR_ADEN   \
        ) ? SET : RESET)
         
/**
  * @brief Verification of ADC state: enabled or disabled
  * @param __HANDLE__: ADC handle
  * @retval SET (ADC enabled) or RESET (ADC disabled)
  */
#define ADC_IS_ENABLE(__HANDLE__)                                                    \
       (( ((((__HANDLE__)->Instance->CR) & (ADC_CR_ADEN | ADC_CR_ADDIS)) == ADC_CR_ADEN) && \
          ((((__HANDLE__)->Instance->ISR) & ADC_FLAG_RDY) == ADC_FLAG_RDY)                  \
        ) ? SET : RESET)
         
/**
  * @brief Returns resolution bits in CFGR register: RES[1:0]. Return value among parameter to @ref ADC_Resolution.
  * @param __HANDLE__: ADC handle
  * @retval None
  */
#define ADC_GET_RESOLUTION(__HANDLE__) (((__HANDLE__)->Instance->CFGR1) & ADC_CFGR1_RES)
/**
  * @brief Test if conversion trigger of regular group is software start
  *        or external trigger.
  * @param __HANDLE__: ADC handle
  * @retval SET (software start) or RESET (external trigger)
  */
#define ADC_IS_SOFTWARE_START_REGULAR(__HANDLE__)                              \
  (((__HANDLE__)->Instance->CFGR1 & ADC_CFGR1_EXTEN) == RESET)



/**
  * @brief Check if no conversion on going on regular group
  * @param __HANDLE__: ADC handle
  * @retval SET (conversion is on going) or RESET (no conversion is on going)
  */
#define ADC_IS_CONVERSION_ONGOING_REGULAR(__HANDLE__)                          \
  (( (((__HANDLE__)->Instance->CR) & ADC_CR_ADSTART) == RESET                  \
  ) ? RESET : SET)

/**
  * @brief Enable ADC continuous conversion mode.
  * @param _CONTINUOUS_MODE_: Continuous mode.
  * @retval None
  */
#define ADC_CONTINUOUS(_CONTINUOUS_MODE_) ((_CONTINUOUS_MODE_) << 13U)

/**
  * @brief Enable ADC scan mode to convert multiple ranks with sequencer.
  * @param _SCAN_MODE_: Scan conversion mode.
  * @retval None
  */
#define ADC_SCANDIR(_SCAN_MODE_)                                   \
  ( ( (_SCAN_MODE_) == (ADC_SCAN_DIRECTION_BACKWARD)                           \
    )? (ADC_CFGR1_SCANDIR) : (0x00000000U)                                      \
  )

/**
  * @brief Configures the number of discontinuous conversions for the regular group channels.
  * @param _NBR_DISCONTINUOUS_CONV_: Number of discontinuous conversions.
  * @retval None
  */
#define __HAL_ADC_CFGR1_DISCONTINUOUS_NUM(_NBR_DISCONTINUOUS_CONV_) (((_NBR_DISCONTINUOUS_CONV_) - 1U) << 17U)

/**
  * @brief Enable the ADC DMA continuous request.
  * @param _DMAContReq_MODE_: DMA continuous request mode.
  * @retval None
  */
#define ADC_DMACONTREQ(_DMAContReq_MODE_) ((_DMAContReq_MODE_) << 1U)

/**
  * @brief Enable the ADC Auto Delay.
  * @param _AutoDelay_: Auto delay bit enable or disable.
  * @retval None
  */
#define __HAL_ADC_CFGR1_AutoDelay(_AutoDelay_) ((_AutoDelay_) << 14U)

/**
  * @brief Enable the ADC LowPowerAutoPowerOff.
  * @param _AUTOFF_: AutoOff bit enable or disable.
  * @retval None
  */
#define __HAL_ADC_CFGR1_AUTOFF(_AUTOFF_) ((_AUTOFF_) << 15U)

/**
  * @brief Configure the analog watchdog high threshold into registers TR1, TR2 or TR3.
  * @param _Threshold_: Threshold value
  * @retval None
  */
#define ADC_TRX_HIGHTHRESHOLD(_Threshold_) ((_Threshold_) << 16U)

/**
  * @brief Enable the ADC Low Frequency mode.
  * @param _LOW_FREQUENCY_MODE_: Low Frequency mode.
  * @retval None
  */
#define __HAL_ADC_CCR_LOWFREQUENCY(_LOW_FREQUENCY_MODE_) ((_LOW_FREQUENCY_MODE_) << 25U)

/**
  * @brief Shift the offset in function of the selected ADC resolution. 
  *        Offset has to be left-aligned on bit 11, the LSB (right bits) are set to 0
  *        If resolution 12 bits, no shift.
  *        If resolution 10 bits, shift of 2 ranks on the right.
  *        If resolution 8 bits, shift of 4 ranks on the right.
  *        If resolution 6 bits, shift of 6 ranks on the right.
  *        therefore, shift = (12 - resolution) = 12 - (12- (((RES[1:0]) >> 3)*2))
  * @param __HANDLE__: ADC handle.
  * @param _Offset_: Value to be shifted
  * @retval None
  */
#define ADC_OFFSET_SHIFT_RESOLUTION(__HANDLE__, _Offset_) \
        ((_Offset_) << ((((__HANDLE__)->Instance->CFGR & ADC_CFGR1_RES) >> 3U)*2U))

/**
  * @brief Shift the AWD1 threshold in function of the selected ADC resolution.
  *        Thresholds have to be left-aligned on bit 11, the LSB (right bits) are set to 0
  *        If resolution 12 bits, no shift.
  *        If resolution 10 bits, shift of 2 ranks on the right.
  *        If resolution 8 bits, shift of 4 ranks on the right.
  *        If resolution 6 bits, shift of 6 ranks on the right.
  *        therefore, shift = (12 - resolution) = 12 - (12- (((RES[1:0]) >> 3)*2))
  * @param __HANDLE__: ADC handle.
  * @param _Threshold_: Value to be shifted
  * @retval None
  */
#define ADC_AWD1THRESHOLD_SHIFT_RESOLUTION(__HANDLE__, _Threshold_) \
        ((_Threshold_) << ((((__HANDLE__)->Instance->CFGR1 & ADC_CFGR1_RES) >> 3U)*2U))

/**
  * @brief Shift the value on the left, less significant are set to 0. 
  * @param _Value_: Value to be shifted
  * @param _Shift_: Number of shift to be done
  * @retval None
  */
#define __HAL_ADC_Value_Shift_left(_Value_, _Shift_) ((_Value_) << (_Shift_))


/**
  * @brief Enable the ADC end of conversion interrupt.
  * @param __HANDLE__: ADC handle.
  * @param __INTERRUPT__: ADC Interrupt.
  * @retval None
  */
#define __HAL_ADC_ENABLE_IT(__HANDLE__, __INTERRUPT__)  \
  (((__HANDLE__)->Instance->IER) |= (__INTERRUPT__))

/**
  * @brief Disable the ADC end of conversion interrupt.
  * @param __HANDLE__: ADC handle.
  * @param __INTERRUPT__: ADC interrupt.
  * @retval None
  */
#define __HAL_ADC_DISABLE_IT(__HANDLE__, __INTERRUPT__) \
  (((__HANDLE__)->Instance->IER) &= ~(__INTERRUPT__))

/** @brief  Checks if the specified ADC interrupt source is enabled or disabled.
  * @param __HANDLE__: ADC handle
  * @param __INTERRUPT__: ADC interrupt source to check
  *            @arg ...
  *            @arg ...
  * @retval State of interruption (TRUE or FALSE)
  */
#define __HAL_ADC_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)                     \
  (((__HANDLE__)->Instance->IER & (__INTERRUPT__)) == (__INTERRUPT__))

/**
  * @brief Clear the ADC's pending flags
  * @param __HANDLE__: ADC handle.
  * @param __FLAG__: ADC flag.
  * @retval None
  */
/* Note: bit cleared bit by writing 1 */
#define __HAL_ADC_CLEAR_FLAG(__HANDLE__, __FLAG__) \
  (((__HANDLE__)->Instance->ISR) = (__FLAG__))

/**
  * @brief Get the selected ADC's flag status.
  * @param __HANDLE__: ADC handle.
  * @param __FLAG__: ADC flag.
  * @retval None
  */
#define __HAL_ADC_GET_FLAG(__HANDLE__, __FLAG__) \
  ((((__HANDLE__)->Instance->ISR) & (__FLAG__)) == (__FLAG__))


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
  * @brief Configuration of ADC clock & prescaler: clock source PCLK or Asynchronous with selectable prescaler
  * @param __HANDLE__: ADC handle
  * @retval None
  */

#define __HAL_ADC_CLOCK_PRESCALER(__HANDLE__)                                       \
  do{                                                                               \
      if ((((__HANDLE__)->Init.ClockPrescaler) == ADC_CLOCK_SYNC_PCLK_DIV1) ||  \
          (((__HANDLE__)->Init.ClockPrescaler) == ADC_CLOCK_SYNC_PCLK_DIV2) ||  \
          (((__HANDLE__)->Init.ClockPrescaler) == ADC_CLOCK_SYNC_PCLK_DIV4))    \
      {                                                                             \
        (__HANDLE__)->Instance->CFGR2 &= ~(ADC_CFGR2_CKMODE);                       \
        (__HANDLE__)->Instance->CFGR2 |=  (__HANDLE__)->Init.ClockPrescaler;        \
      }                                                                             \
      else                                                                          \
      {                                                                             \
        /* CKMOD bits must be reset */                                              \
        (__HANDLE__)->Instance->CFGR2 &= ~(ADC_CFGR2_CKMODE);                       \
        ADC->CCR &= ~(ADC_CCR_PRESC);                                               \
        ADC->CCR |=  (__HANDLE__)->Init.ClockPrescaler;                             \
      }                                                                             \
  } while(0)


#define IS_ADC_CLOCKPRESCALER(ADC_CLOCK) (((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV1) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_SYNC_PCLK_DIV1) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_SYNC_PCLK_DIV2) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_SYNC_PCLK_DIV4) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV1  ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV2  ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV4  ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV6  ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV8  ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV10 ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV12 ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV16 ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV32 ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV64 ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV128 ) ||\
                                          ((ADC_CLOCK) == ADC_CLOCK_ASYNC_DIV256))

#define IS_ADC_RESOLUTION(RESOLUTION) (((RESOLUTION) == ADC_RESOLUTION_12B) || \
                                       ((RESOLUTION) == ADC_RESOLUTION_10B) || \
                                       ((RESOLUTION) == ADC_RESOLUTION_8B) || \
                                       ((RESOLUTION) == ADC_RESOLUTION_6B))

#define IS_ADC_RESOLUTION_8_6_BITS(RESOLUTION) (((RESOLUTION) == ADC_RESOLUTION_8B) || \
                                                ((RESOLUTION) == ADC_RESOLUTION_6B))

#define IS_ADC_DATA_ALIGN(ALIGN) (((ALIGN) == ADC_DATAALIGN_RIGHT) || \
                                  ((ALIGN) == ADC_DATAALIGN_LEFT))

#define IS_ADC_EXTTRIG_EDGE(EDGE) (((EDGE) == ADC_EXTERNALTRIGCONVEDGE_NONE) || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_RISING) || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_FALLING) || \
                                   ((EDGE) == ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING))

#define IS_ADC_EOC_SELECTION(EOC_SELECTION) (((EOC_SELECTION) == ADC_EOC_SINGLE_CONV)   || \
                                             ((EOC_SELECTION) == ADC_EOC_SEQ_CONV)      || \
                                             ((EOC_SELECTION) == ADC_EOC_SINGLE_SEQ_CONV))

#define IS_ADC_OVERRUN(OVR) (((OVR) == ADC_OVR_DATA_PRESERVED) || \
                             ((OVR) == ADC_OVR_DATA_OVERWRITTEN))

#define IS_ADC_RANK(WATCHDOG) (((WATCHDOG) == ADC_RANK_CHANNEL_NUMBER) || \
                               ((WATCHDOG) == ADC_RANK_NONE))

#if defined (STM32L053xx) || defined (STM32L063xx) || defined (STM32L073xx) || defined (STM32L083xx)
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
                                 ((CHANNEL) == ADC_CHANNEL_TEMPSENSOR)  || \
                                 ((CHANNEL) == ADC_CHANNEL_VREFINT)     || \
                                 ((CHANNEL) == ADC_CHANNEL_VLCD))
#else
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
                                 ((CHANNEL) == ADC_CHANNEL_TEMPSENSOR)  || \
                                 ((CHANNEL) == ADC_CHANNEL_VREFINT))
#endif

#define IS_ADC_SAMPLE_TIME(TIME) (((TIME) == ADC_SAMPLETIME_1CYCLE_5   ) || \
                                  ((TIME) == ADC_SAMPLETIME_3CYCLES_5  ) || \
                                  ((TIME) == ADC_SAMPLETIME_7CYCLES_5  ) || \
                                  ((TIME) == ADC_SAMPLETIME_12CYCLES_5 ) || \
                                  ((TIME) == ADC_SAMPLETIME_19CYCLES_5 ) || \
                                  ((TIME) == ADC_SAMPLETIME_39CYCLES_5 ) || \
                                  ((TIME) == ADC_SAMPLETIME_79CYCLES_5 ) || \
                                  ((TIME) == ADC_SAMPLETIME_160CYCLES_5))

#define IS_ADC_SCAN_MODE(SCAN_MODE) (((SCAN_MODE) == ADC_SCAN_DIRECTION_FORWARD) || \
                                     ((SCAN_MODE) == ADC_SCAN_DIRECTION_BACKWARD))

#define IS_ADC_OVERSAMPLING_RATIO(RATIO)          (((RATIO) == ADC_OVERSAMPLING_RATIO_2   ) || \
                                                   ((RATIO) == ADC_OVERSAMPLING_RATIO_4   ) || \
                                                   ((RATIO) == ADC_OVERSAMPLING_RATIO_8   ) || \
                                                   ((RATIO) == ADC_OVERSAMPLING_RATIO_16  ) || \
                                                   ((RATIO) == ADC_OVERSAMPLING_RATIO_32  ) || \
                                                   ((RATIO) == ADC_OVERSAMPLING_RATIO_64  ) || \
                                                   ((RATIO) == ADC_OVERSAMPLING_RATIO_128 ) || \
                                                   ((RATIO) == ADC_OVERSAMPLING_RATIO_256 ))

#define IS_ADC_RIGHT_BIT_SHIFT(SHIFT)               (((SHIFT) == ADC_RIGHTBITSHIFT_NONE) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_1   ) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_2   ) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_3   ) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_4   ) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_5   ) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_6   ) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_7   ) || \
                                                     ((SHIFT) == ADC_RIGHTBITSHIFT_8   ))

#define IS_ADC_TRIGGERED_OVERSAMPLING_MODE(MODE)     (((MODE) == ADC_TRIGGEREDMODE_SINGLE_TRIGGER) || \
                                                      ((MODE) == ADC_TRIGGEREDMODE_MULTI_TRIGGER) )

#define IS_ADC_ANALOG_WATCHDOG_MODE(WATCHDOG)     (((WATCHDOG) == ADC_ANALOGWATCHDOG_NONE      )   || \
                                                   ((WATCHDOG) == ADC_ANALOGWATCHDOG_SINGLE_REG)   || \
                                                   ((WATCHDOG) == ADC_ANALOGWATCHDOG_ALL_REG   ))

#define IS_ADC_CONVERSION_GROUP(CONVERSION)   ((CONVERSION) == ADC_REGULAR_GROUP)

#define IS_ADC_EVENT_TYPE(EVENT) (((EVENT) == ADC_AWD_EVENT) || \
                                  ((EVENT) == ADC_OVR_EVENT))


/** @defgroup ADC_range_verification ADC Range Verification
  * in function of ADC resolution selected (12, 10, 8 or 6 bits)
  * @{
  */
#define IS_ADC_RANGE(RESOLUTION, ADC_VALUE)                                     \
   ((((RESOLUTION) == ADC_RESOLUTION_12B) && ((ADC_VALUE) <= ((uint32_t)0x0FFFU))) || \
    (((RESOLUTION) == ADC_RESOLUTION_10B) && ((ADC_VALUE) <= ((uint32_t)0x03FFU))) || \
    (((RESOLUTION) == ADC_RESOLUTION_8B)  && ((ADC_VALUE) <= ((uint32_t)0x00FFU))) || \
    (((RESOLUTION) == ADC_RESOLUTION_6B)  && ((ADC_VALUE) <= ((uint32_t)0x003FU))))
/**
  * @}
  */

/** @defgroup ADC_regular_nb_conv_verification ADC Regular Nb Conversion Verification
  * @{
  */
#define IS_ADC_REGULAR_NB_CONV(LENGTH) (((LENGTH) >= ((uint32_t)1U)) && ((LENGTH) <= ((uint32_t)16U)))
/**
  * @}
  */

/**
  * @}
  */

/* Include ADC HAL Extended module */
#include "stm32l0xx_hal_adc_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup ADC_Exported_Functions
  * @{
  */

/** @addtogroup ADC_Exported_Functions_Group1
  * @brief    Initialization and Configuration functions
  * @{
  */
/* Initialization and de-initialization functions  ****************************/
HAL_StatusTypeDef    HAL_ADC_Init(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef    HAL_ADC_DeInit(ADC_HandleTypeDef *hadc);
void                 HAL_ADC_MspInit(ADC_HandleTypeDef* hadc);
void                 HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc);
/**
  * @}
  */

/** @addtogroup ADC_Exported_Functions_Group2
  * @brief    IO operation functions
  * @{
  */
/* IO operation functions  *****************************************************/

/* Blocking mode: Polling */
HAL_StatusTypeDef    HAL_ADC_Start(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef    HAL_ADC_Stop(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef    HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);
HAL_StatusTypeDef    HAL_ADC_PollForEvent(ADC_HandleTypeDef* hadc, uint32_t EventType, uint32_t Timeout);

/* Non-blocking mode: Interruption */
HAL_StatusTypeDef    HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef    HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc);

/* Non-blocking mode: DMA */
HAL_StatusTypeDef    HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length);
HAL_StatusTypeDef    HAL_ADC_Stop_DMA(ADC_HandleTypeDef* hadc);

/* ADC retrieve conversion value intended to be used with polling or interruption */
uint32_t             HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);

/* ADC IRQHandler and Callbacks used in non-blocking modes (Interruption and DMA) */
void                 HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc);
void                 HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void                 HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);
void                 HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc);
void                 HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc);
/**
  * @}
  */

/** @addtogroup ADC_Exported_Functions_Group3 Peripheral Control functions
 *  @brief    Peripheral Control functions 
 * @{
 */
/* Peripheral Control functions ***********************************************/
HAL_StatusTypeDef    HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig);
HAL_StatusTypeDef    HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig);
/**
  * @}
  */

/* Peripheral State functions *************************************************/
/** @addtogroup ADC_Exported_Functions_Group4
  * @{
  */
uint32_t             HAL_ADC_GetState(ADC_HandleTypeDef* hadc);
uint32_t             HAL_ADC_GetError(ADC_HandleTypeDef *hadc);
/**
  * @}
  */


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


#endif /*__STM32L0xx_HAL_ADC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
