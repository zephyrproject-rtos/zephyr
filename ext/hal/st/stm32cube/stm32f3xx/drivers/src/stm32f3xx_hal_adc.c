/**
  ******************************************************************************
  * @file    stm32f3xx_hal_adc.c
  * @author  MCD Application Team
  * @brief   This file provides firmware functions to manage the following
  *          functionalities of the Analog to Digital Convertor (ADC)
  *          peripheral:
  *           + Initialization and de-initialization functions
  *             ++ Initialization and Configuration of ADC
  *           + Operation functions
  *             ++ Start, stop, get result of conversions of regular
  *                group, using 3 possible modes: polling, interruption or DMA.
  *           + Control functions
  *             ++ Channels configuration on regular group
  *             ++ Channels configuration on injected group
  *             ++ Analog Watchdog configuration
  *           + State functions
  *             ++ ADC state machine management
  *             ++ Interrupts and flags management
  *          Other functions (extended functions) are available in file
  *          "stm32f3xx_hal_adc_ex.c".
  *
  @verbatim
  ==============================================================================
                     ##### ADC peripheral features #####
  ==============================================================================
  [..]
  (+) 12-bit, 10-bit, 8-bit or 6-bit configurable resolution (available only on
      STM32F30xxC devices).

  (+) Interrupt generation at the end of regular conversion, end of injected
      conversion, and in case of analog watchdog or overrun events.

  (+) Single and continuous conversion modes.

  (+) Scan mode for conversion of several channels sequentially.

  (+) Data alignment with in-built data coherency.

  (+) Programmable sampling time (channel wise)

  (+) ADC conversion of regular group and injected group.

  (+) External trigger (timer or EXTI) with configurable polarity
      for both regular and injected groups.

  (+) DMA request generation for transfer of conversions data of regular group.

  (+) Multimode dual mode (available on devices with 2 ADCs or more).

  (+) Configurable DMA data storage in Multimode Dual mode (available on devices
      with 2 DCs or more).

  (+) Configurable delay between conversions in Dual interleaved mode (available
      on devices with 2 DCs or more).

  (+) ADC calibration

  (+) ADC channels selectable single/differential input (available only on
      STM32F30xxC devices)

  (+) ADC Injected sequencer&channels configuration context queue (available
      only on STM32F30xxC devices)

  (+) ADC offset on injected and regular groups (offset on regular group
      available only on STM32F30xxC devices)

  (+) ADC supply requirements: 2.4 V to 3.6 V at full speed and down to 1.8 V at
      slower speed.

  (+) ADC input range: from Vref- (connected to Vssa) to Vref+ (connected to
      Vdda or to an external voltage reference).


                     ##### How to use this driver #####
  ==============================================================================
    [..]

     *** Configuration of top level parameters related to ADC ***
     ============================================================
     [..]

    (#) Enable the ADC interface
      (++) As prerequisite, ADC clock must be configured at RCC top level.

        (++) For STM32F30x/STM32F33x devices:
             Two possible clock sources: synchronous clock derived from AHB clock
             or asynchronous clock derived from ADC dedicated PLL 72MHz.
              - Synchronous clock is mandatory since used as ADC core clock.
                Synchronous clock can be used optionally as ADC conversion clock, depending on ADC init structure clock setting.
                Synchronous clock is configured using macro __ADCx_CLK_ENABLE().
              - Asynchronous can be used optionally as ADC conversion clock, depending on ADC init structure clock setting.
                Asynchronous clock is configured using function HAL_RCCEx_PeriphCLKConfig().
             (+++) For example, in case of device with a single ADC:
                   Into HAL_ADC_MspInit() (recommended code location) or with
                   other device clock parameters configuration:
               (+++) __HAL_RCC_ADC1_CLK_ENABLE()                            (mandatory)
               (+++) PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC (optional, if ADC conversion from asynchronous clock)
               (+++) PeriphClkInit.Adc1ClockSelection = RCC_ADC1PLLCLK_DIV1 (optional, if ADC conversion from asynchronous clock)
               (+++) HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInitStructure) (optional, if ADC conversion from asynchronous clock)

             (+++) For example, in case of device with 4 ADCs:

               (+++) if((hadc->Instance == ADC1) || (hadc->Instance == ADC2))
               (+++) {
               (+++)   __HAL_RCC_ADC12_CLK_ENABLE()                             (mandatory)
               (+++)   PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC   (optional, if ADC conversion from asynchronous clock)
               (+++)   PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1 (optional, if ADC conversion from asynchronous clock)
               (+++)   HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInitStructure)   (optional, if ADC conversion from asynchronous clock)
               (+++) }
               (+++) else
               (+++) {
               (+++)   __HAL_RCC_ADC34_CLK_ENABLE()                              (mandatory)
               (+++)   PeriphClkInit.Adc34ClockSelection = RCC_ADC34PLLCLK_DIV1; (optional, if ADC conversion from asynchronous clock)
               (+++)   HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInitStructure);   (optional, if ADC conversion from asynchronous clock)
               (+++) }

        (++) For STM32F37x devices:
             One clock setting is mandatory:
             ADC clock (core and conversion clock) from APB2 clock.
             (+++) Example:
                   Into HAL_ADC_MspInit() (recommended code location) or with
                   other device clock parameters configuration:
               (+++) PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC
               (+++) PeriphClkInit.AdcClockSelection = RCC_ADCPLLCLK_DIV2
               (+++) HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit)

    (#) ADC pins configuration
         (++) Enable the clock for the ADC GPIOs
              using macro __HAL_RCC_GPIOx_CLK_ENABLE()
         (++) Configure these ADC pins in analog mode
              using function HAL_GPIO_Init()

    (#) Optionally, in case of usage of ADC with interruptions:
         (++) Configure the NVIC for ADC
              using function HAL_NVIC_EnableIRQ(ADCx_IRQn)
         (++) Insert the ADC interruption handler function HAL_ADC_IRQHandler()
              into the function of corresponding ADC interruption vector
              ADCx_IRQHandler().

    (#) Optionally, in case of usage of DMA:
         (++) Configure the DMA (DMA channel, mode normal or circular, ...)
              using function HAL_DMA_Init().
         (++) Configure the NVIC for DMA
              using function HAL_NVIC_EnableIRQ(DMAx_Channelx_IRQn)
         (++) Insert the ADC interruption handler function HAL_ADC_IRQHandler()
              into the function of corresponding DMA interruption vector
              DMAx_Channelx_IRQHandler().

     *** Configuration of ADC, groups regular/injected, channels parameters ***
     ==========================================================================
     [..]

    (#) Configure the ADC parameters (resolution, data alignment, ...)
        and regular group parameters (conversion trigger, sequencer, ...)
        using function HAL_ADC_Init().

    (#) Configure the channels for regular group parameters (channel number,
        channel rank into sequencer, ..., into regular group)
        using function HAL_ADC_ConfigChannel().

    (#) Optionally, configure the injected group parameters (conversion trigger,
        sequencer, ..., of injected group)
        and the channels for injected group parameters (channel number,
        channel rank into sequencer, ..., into injected group)
        using function HAL_ADCEx_InjectedConfigChannel().

    (#) Optionally, configure the analog watchdog parameters (channels
        monitored, thresholds, ...)
        using function HAL_ADC_AnalogWDGConfig().

    (#) Optionally, for devices with several ADC instances: configure the
        multimode parameters
        using function HAL_ADCEx_MultiModeConfigChannel().

     *** Execution of ADC conversions ***
     ====================================
     [..]

    (#) Optionally, perform an automatic ADC calibration to improve the
        conversion accuracy
        using function HAL_ADCEx_Calibration_Start().

    (#) ADC driver can be used among three modes: polling, interruption,
        transfer by DMA.

        (++) ADC conversion by polling:
          (+++) Activate the ADC peripheral and start conversions
                using function HAL_ADC_Start()
          (+++) Wait for ADC conversion completion
                using function HAL_ADC_PollForConversion()
                (or for injected group: HAL_ADCEx_InjectedPollForConversion() )
          (+++) Retrieve conversion results
                using function HAL_ADC_GetValue()
                (or for injected group: HAL_ADCEx_InjectedGetValue() )
          (+++) Stop conversion and disable the ADC peripheral
                using function HAL_ADC_Stop()

        (++) ADC conversion by interruption:
          (+++) Activate the ADC peripheral and start conversions
                using function HAL_ADC_Start_IT()
          (+++) Wait for ADC conversion completion by call of function
                HAL_ADC_ConvCpltCallback()
                (this function must be implemented in user program)
                (or for injected group: HAL_ADCEx_InjectedConvCpltCallback() )
          (+++) Retrieve conversion results
                using function HAL_ADC_GetValue()
                (or for injected group: HAL_ADCEx_InjectedGetValue() )
          (+++) Stop conversion and disable the ADC peripheral
                using function HAL_ADC_Stop_IT()

        (++) ADC conversion with transfer by DMA:
          (+++) Activate the ADC peripheral and start conversions
                using function HAL_ADC_Start_DMA()
          (+++) Wait for ADC conversion completion by call of function
                HAL_ADC_ConvCpltCallback() or HAL_ADC_ConvHalfCpltCallback()
                (these functions must be implemented in user program)
          (+++) Conversion results are automatically transferred by DMA into
                destination variable address.
          (+++) Stop conversion and disable the ADC peripheral
                using function HAL_ADC_Stop_DMA()

        (++) For devices with several ADCs: ADC multimode conversion
             with transfer by DMA:
          (+++) Activate the ADC peripheral (slave)
                using function HAL_ADC_Start()
                (conversion start pending ADC master)
          (+++) Activate the ADC peripheral (master) and start conversions
                using function HAL_ADCEx_MultiModeStart_DMA()
          (+++) Wait for ADC conversion completion by call of function
                HAL_ADC_ConvCpltCallback() or HAL_ADC_ConvHalfCpltCallback()
                (these functions must be implemented in user program)
          (+++) Conversion results are automatically transferred by DMA into
                destination variable address.
          (+++) Stop conversion and disable the ADC peripheral (master)
                using function HAL_ADCEx_MultiModeStop_DMA()
          (+++) Stop conversion and disable the ADC peripheral (slave)
                using function HAL_ADC_Stop_IT()

     [..]

    (@) Callback functions must be implemented in user program:
      (+@) HAL_ADC_ErrorCallback()
      (+@) HAL_ADC_LevelOutOfWindowCallback() (callback of analog watchdog)
      (+@) HAL_ADC_ConvCpltCallback()
      (+@) HAL_ADC_ConvHalfCpltCallback
      (+@) HAL_ADCEx_InjectedConvCpltCallback()
      (+@) HAL_ADCEx_InjectedQueueOverflowCallback() (for STM32F30x/STM32F33x devices)

     *** Deinitialization of ADC ***
     ============================================================
     [..]

    (#) Disable the ADC interface
      (++) ADC clock can be hard reset and disabled at RCC top level.
        (++) Hard reset of ADC peripherals
             using macro __ADCx_FORCE_RESET(), __ADCx_RELEASE_RESET().
        (++) ADC clock disable
             using the equivalent macro/functions as configuration step.

        (++) For STM32F30x/STM32F33x devices:
           Caution: For devices with several ADCs:
           These settings impact both ADC of common group: ADC1&ADC2, ADC3&ADC4
           if available (ADC2, ADC3, ADC4 availability depends on STM32 product)

             (+++) For example, in case of device with a single ADC:
                   Into HAL_ADC_MspDeInit() (recommended code location) or with
                   other device clock parameters configuration:
               (+++) __HAL_RCC_ADC1_FORCE_RESET()                           (optional)
               (+++) __HAL_RCC_ADC1_RELEASE_RESET()                         (optional)
               (+++) __HAL_RCC_ADC1_CLK_DISABLE()                           (mandatory)
               (+++) PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC (optional, if configured before)
               (+++) PeriphClkInit.Adc1ClockSelection = RCC_ADC1PLLCLK_OFF  (optional, if configured before)
               (+++) HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInitStructure) (optional, if configured before)

             (+++) For example, in case of device with 4 ADCs:
               (+++) if((hadc->Instance == ADC1) || (hadc->Instance == ADC2))
               (+++) {
               (+++)   __HAL_RCC_ADC12_FORCE_RESET()                            (optional)
               (+++)   __HAL_RCC_ADC12_RELEASE_RESET()                          (optional)
               (+++)   __HAL_RCC_ADC12_CLK_DISABLE()                            (mandatory)
               (+++)   PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC   (optional, if configured before)
               (+++)   PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_OFF  (optional, if configured before)
               (+++)   HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInitStructure)   (optional, if configured before)
               (+++) }
               (+++) else
               (+++) {
               (+++)   __HAL_RCC_ADC32_FORCE_RESET()                            (optional)
               (+++)   __HAL_RCC_ADC32_RELEASE_RESET()                          (optional)
               (+++)   __HAL_RCC_ADC34_CLK_DISABLE()                            (mandatory)
               (+++)   PeriphClkInit.Adc34ClockSelection = RCC_ADC34PLLCLK_OFF  (optional, if configured before)
               (+++)   HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInitStructure)   (optional, if configured before)
               (+++) }

        (++) For STM32F37x devices:
             (+++) Example:
                   Into HAL_ADC_MspDeInit() (recommended code location) or with
                   other device clock parameters configuration:
               (+++) PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC
               (+++) PeriphClkInit.AdcClockSelection = RCC_ADCPLLCLK_OFF
               (+++) HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit)

    (#) ADC pins configuration
         (++) Disable the clock for the ADC GPIOs
              using macro __HAL_RCC_GPIOx_CLK_DISABLE()

    (#) Optionally, in case of usage of ADC with interruptions:
         (++) Disable the NVIC for ADC
              using function HAL_NVIC_EnableIRQ(ADCx_IRQn)

    (#) Optionally, in case of usage of DMA:
         (++) Deinitialize the DMA
              using function HAL_DMA_Init().
         (++) Disable the NVIC for DMA
              using function HAL_NVIC_EnableIRQ(DMAx_Channelx_IRQn)

    [..]

    @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @defgroup ADC ADC
  * @brief ADC HAL module driver
  * @{
  */

#ifdef HAL_ADC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup ADC_Exported_Functions ADC Exported Functions
  * @{
  */

/** @defgroup ADC_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Initialize and configure the ADC.
      (+) De-initialize the ADC.

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the ADC peripheral and regular group according to
  *         parameters specified in structure "ADC_InitTypeDef".
  * @note   As prerequisite, ADC clock must be configured at RCC top level
  *         depending on both possible clock sources: PLL clock or AHB clock.
  *         See commented example code below that can be copied and uncommented
  *         into HAL_ADC_MspInit().
  * @note   Possibility to update parameters on the fly:
  *         This function initializes the ADC MSP (HAL_ADC_MspInit()) only when
  *         coming from ADC state reset. Following calls to this function can
  *         be used to reconfigure some parameters of ADC_InitTypeDef
  *         structure on the fly, without modifying MSP configuration. If ADC
  *         MSP has to be modified again, HAL_ADC_DeInit() must be called
  *         before HAL_ADC_Init().
  *         The setting of these parameters is conditioned to ADC state.
  *         For parameters constraints, see comments of structure
  *         "ADC_InitTypeDef".
  * @note   This function configures the ADC within 2 scopes: scope of entire
  *         ADC and scope of regular group. For parameters details, see comments
  *         of structure "ADC_InitTypeDef".
  * @note   For devices with several ADCs: parameters related to common ADC
  *         registers (ADC clock mode) are set only if all ADCs sharing the
  *         same common group are disabled.
  *         If this is not the case, these common parameters setting are
  *         bypassed without error reporting: it can be the intended behaviour in
  *         case of update of a parameter of ADC_InitTypeDef on the fly,
  *         without  disabling the other ADCs sharing the same common group.
  * @param  hadc ADC handle
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Deinitialize the ADC peripheral registers to their default reset
  *         values, with deinitialization of the ADC MSP.
  * @note   For devices with several ADCs: reset of ADC common registers is done
  *         only if all ADCs sharing the same common group are disabled.
  *         If this is not the case, reset of these common parameters reset is
  *         bypassed without error reporting: it can be the intended behaviour in
  *         case of reset of a single ADC while the other ADCs sharing the same
  *         common group is still running.
  * @note   For devices with several ADCs: Global reset of all ADCs sharing a
  *         common group is possible.
  *         As this function is intended to reset a single ADC, to not impact
  *         other ADCs, instructions for global reset of multiple ADCs have been
  *         let commented below.
  *         If needed, the example code can be copied and uncommented into
  *         function HAL_ADC_MspDeInit().
  * @param  hadc ADC handle
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Initializes the ADC MSP.
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_MspInit must be implemented in the user file.
   */
}

/**
  * @brief  DeInitializes the ADC MSP.
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_MspDeInit must be implemented in the user file.
   */
}

/**
  * @}
  */

/** @defgroup ADC_Exported_Functions_Group2 Input and Output operation functions
 *  @brief    IO operation functions
 *
@verbatim
 ===============================================================================
             ##### IO operation functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Start conversion of regular group.
      (+) Stop conversion of regular group.
      (+) Poll for conversion complete on regular group.
      (+) Poll for conversion event.
      (+) Get result of regular channel conversion.
      (+) Start conversion of regular group and enable interruptions.
      (+) Stop conversion of regular group and disable interruptions.
      (+) Handle ADC interrupt request
      (+) Start conversion of regular group and enable DMA transfer.
      (+) Stop conversion of regular group and disable ADC DMA transfer.

@endverbatim
  * @{
  */
/**
  * @brief  Enables ADC, starts conversion of regular group.
  *         Interruptions enabled in this function: None.
  * @note:  Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC slave first, then ADC master.
  *         For ADC slave, ADC is enabled only (conversion is not started).
  *         For ADC master, ADC is enabled and multimode conversion is started.
  * @param  hadc ADC handle
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Stop ADC conversion of regular group (and injected group in
  *         case of auto_injection mode), disable ADC peripheral.
  * @note:  ADC peripheral disable is forcing stop of potential
  *         conversion on injected group. If injected group is under use, it
  *         should be preliminarily stopped using HAL_ADCEx_InjectedStop function.
  * @note:  Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC master first, then ADC slave.
  *         For ADC master, converson is stopped and ADC is disabled.
  *         For ADC slave, ADC is disabled only (conversion stop of ADC master
  *         has already stopped conversion of ADC slave).
  * @param  hadc ADC handle
  * @retval HAL status.
  */
__weak HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Wait for regular group conversion to be completed.
  * @param  hadc ADC handle
  * @param  Timeout Timeout value in millisecond.
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  UNUSED(Timeout);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Poll for conversion event.
  * @param  hadc ADC handle
  * @param  EventType the ADC event type.
  *          This parameter can be one of the following values:
  *            @arg ADC_AWD_EVENT: ADC Analog watchdog 1 event (main analog watchdog, present on all STM32 devices)
  *            @arg ADC_AWD2_EVENT: ADC Analog watchdog 2 event (additional analog watchdog, present only on STM32F3 devices)
  *            @arg ADC_AWD3_EVENT: ADC Analog watchdog 3 event (additional analog watchdog, present only on STM32F3 devices)
  *            @arg ADC_OVR_EVENT: ADC Overrun event
  *            @arg ADC_JQOVF_EVENT: ADC Injected context queue overflow event
  * @param  Timeout Timeout value in millisecond.
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_PollForEvent(ADC_HandleTypeDef* hadc, uint32_t EventType, uint32_t Timeout)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  UNUSED(EventType);
  UNUSED(Timeout);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Enables ADC, starts conversion of regular group with interruption.
  *         Interruptions enabled in this function:
  *          - EOC (end of conversion of regular group) or EOS (end of
  *            sequence of regular group) depending on ADC initialization
  *            parameter "EOCSelection" (if available)
  *          - overrun (if available)
  *         Each of these interruptions has its dedicated callback function.
  * @note:  Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC slave first, then ADC master.
  *         For ADC slave, ADC is enabled only (conversion is not started).
  *         For ADC master, ADC is enabled and multimode conversion is started.
  * @param  hadc ADC handle
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Stop ADC conversion of regular group (and injected group in
  *         case of auto_injection mode), disable interruption of
  *         end-of-conversion, disable ADC peripheral.
  * @note:  ADC peripheral disable is forcing stop of potential
  *         conversion on injected group. If injected group is under use, it
  *         should be preliminarily stopped using HAL_ADCEx_InjectedStop function.
  * @note:  Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC master first, then ADC slave.
  *         For ADC master, conversion is stopped and ADC is disabled.
  *         For ADC slave, ADC is disabled only (conversion stop of ADC master
  *         has already stopped conversion of ADC slave).
  * @param  hadc ADC handle
  * @retval HAL status.
  */
__weak HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Enables ADC, starts conversion of regular group and transfers result
  *         through DMA.
  *         Interruptions enabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun (if available)
  *         Each of these interruptions has its dedicated callback function.
  * @note:  Case of multimode enabled (for devices with several ADCs): This
  *         function is for single-ADC mode only. For multimode, use the
  *         dedicated MultimodeStart function.
  * @param  hadc ADC handle
  * @param  pData The destination Buffer address.
  * @param  Length The length of data to be transferred from ADC peripheral to memory.
  * @retval None
  */
__weak HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  UNUSED(pData);
  UNUSED(Length);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Stop ADC conversion of regular group (and injected group in
  *         case of auto_injection mode), disable ADC DMA transfer, disable
  *         ADC peripheral.
  * @note:  ADC peripheral disable is forcing stop of potential
  *         conversion on injected group. If injected group is under use, it
  *         should be preliminarily stopped using HAL_ADCEx_InjectedStop function.
  * @note:  Case of multimode enabled (for devices with several ADCs): This
  *         function is for single-ADC mode only. For multimode, use the
  *         dedicated MultimodeStop function.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
__weak HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Get ADC regular group conversion result.
  * @note   Reading DR register automatically clears EOC (end of conversion of
  *         regular group) flag.
  *         Additionally, this functions clears EOS (end of sequence of
  *         regular group) flag, in case of the end of the sequence is reached.
  * @param  hadc ADC handle
  * @retval Converted value
  */
__weak uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc)
{
  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return ADC converted value */
  return hadc->Instance->DR;
}

/**
  * @brief  Handles ADC interrupt request.
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */
}

/**
  * @brief  Conversion complete callback in non blocking mode
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_ConvCpltCallback must be implemented in the user file.
   */
}

/**
  * @brief  Conversion DMA half-transfer callback in non blocking mode
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_ConvHalfCpltCallback must be implemented in the user file.
  */
}

/**
  * @brief  Analog watchdog callback in non blocking mode.
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_LevelOoutOfWindowCallback must be implemented in the user file.
  */
}

/**
  * @brief  ADC error callback in non blocking mode
  *        (ADC conversion with interruption or transfer by DMA)
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_ErrorCallback must be implemented in the user file.
  */
}

/**
  * @}
  */

/** @defgroup ADC_Exported_Functions_Group3 Peripheral Control functions
 *  @brief    Peripheral Control functions
 *
@verbatim
 ===============================================================================
             ##### Peripheral Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure channels on regular group
      (+) Configure the analog watchdog

@endverbatim
  * @{
  */

/**
  * @brief  Configures the the selected channel to be linked to the regular
  *         group.
  * @note   In case of usage of internal measurement channels:
  *         Vbat/VrefInt/TempSensor.
  *         The recommended sampling time is at least:
  *          - For devices STM32F37x: 17.1us for temperature sensor
  *          - For the other STM32F3 devices: 2.2us for each of channels
  *            Vbat/VrefInt/TempSensor.
  *         These internal paths can be be disabled using function
  *         HAL_ADC_DeInit().
  * @note   Possibility to update parameters on the fly:
  *         This function initializes channel into regular group, following
  *         calls to this function can be used to reconfigure some parameters
  *         of structure "ADC_ChannelConfTypeDef" on the fly, without reseting
  *         the ADC.
  *         The setting of these parameters is conditioned to ADC state.
  *         For parameters constraints, see comments of structure
  *         "ADC_ChannelConfTypeDef".
  * @param  hadc ADC handle
  * @param  sConfig Structure of ADC channel for regular group.
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  UNUSED(sConfig);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @brief  Configures the analog watchdog.
  * @note   Possibility to update parameters on the fly:
  *         This function initializes the selected analog watchdog, following
  *         calls to this function can be used to reconfigure some parameters
  *         of structure "ADC_AnalogWDGConfTypeDef" on the fly, without reseting
  *         the ADC.
  *         The setting of these parameters is conditioned to ADC state.
  *         For parameters constraints, see comments of structure
  *         "ADC_AnalogWDGConfTypeDef".
  * @param  hadc ADC handle
  * @param  AnalogWDGConfig Structure of ADC analog watchdog configuration
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  UNUSED(AnalogWDGConfig);

  /* Note : This function is defined into this file for library reference. */
  /*        Function content is located into file stm32f3xx_hal_adc_ex.c   */

  /* Return function status */
  return HAL_ERROR;
}

/**
  * @}
  */

/** @defgroup ADC_Exported_Functions_Group4 Peripheral State functions
 *  @brief   ADC Peripheral State functions
 *
@verbatim
 ===============================================================================
            ##### Peripheral state and errors functions #####
 ===============================================================================
    [..]
    This subsection provides functions to get in run-time the status of the
    peripheral.
      (+) Check the ADC state
      (+) Check the ADC error code

@endverbatim
  * @{
  */

/**
  * @brief  return the ADC state
  * @note   ADC state machine is managed by bitfield, state must be compared
  *         with bit by bit.
  *         For example:
  *           " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_REG_BUSY)) "
  *           " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_AWD1)    ) "
  * @param  hadc ADC handle
  * @retval HAL state
  */
uint32_t HAL_ADC_GetState(ADC_HandleTypeDef* hadc)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Return ADC state */
  return hadc->State;
}

/**
  * @brief  Return the ADC error code
  * @param  hadc ADC handle
  * @retval ADC Error Code
  */
uint32_t HAL_ADC_GetError(ADC_HandleTypeDef *hadc)
{
  return hadc->ErrorCode;
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_ADC_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
