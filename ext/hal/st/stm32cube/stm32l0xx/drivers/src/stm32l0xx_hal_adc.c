/**
  ******************************************************************************
  * @file    stm32l0xx_hal_adc.c
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Analog to Digital Convertor (ADC)
  *          peripheral:
  *           + Initialization and de-initialization functions
  *             ++ Initialization and Configuration of ADC
  *           + Operation functions
  *             ++ Start, stop, get result of conversions of regular 
  *             group, using 3 possible modes : polling, interruption or DMA.
  *           + Control functions
  *             ++ Channels configuration on regular group
  *             ++ Analog Watchdog configuration
  *           + State functions
  *             ++ ADC state machine management
  *             ++ Interrupts and flags management
  *          Other functions (extended functions) are available in file 
  *          "stm32l0xx_hal_adc_ex.c".
  *         
  @verbatim
  ==============================================================================
                     ##### ADC peripheral features #####
  ==============================================================================
  [..] 
  (+) 12-bit, 10-bit, 8-bit or 6-bit configurable resolution

  (+) A built-in hardware oversampler can handle multiple conversions and average
      them into a single data with increased data width, up to 16-bit.

  (+) Interrupt generation at the end of regular conversion and in case of 
      analog watchdog or overrun events.

  (+) Single and continuous conversion modes.
  
  (+) Scan mode for conversion of several channels sequentially.
  
  (+) Data alignment with in-built data coherency.

  (+) Programmable sampling time (common for all channels)
  
  (+) ADC conversion of regular group.
  
  (+) External trigger (timer or EXTI) with configurable polarity

  (+) DMA request generation for transfer of conversions data of regular group.

  (+) ADC calibration

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
           Caution: On STM32L0, ADC clock frequency max is 16MHz (refer
                    to device datasheet).
                    Therefore, ADC clock prescaler must be configured in 
                    function of ADC clock source frequency to remain below
                    this maximum frequency.

        (++) Two clock settings are mandatory: 
             (+++) ADC clock (core clock, also possibly conversion clock).

             (+++) ADC clock (conversions clock).
                   Two possible clock sources: synchronous clock derived from APB clock
                   or asynchronous clock derived from ADC dedicated HSI RC oscillator
                   16MHz.
                   If asynchronous clock is selected, parameter "HSIState" must be set either:
                   - to "...HSIState = RCC_HSI_ON" to maintain the HSI16 oscillator
                     always enabled: can be used to supply the main system clock.

             (+++) Example:
                   Into HAL_ADC_MspInit() (recommended code location) or with
                   other device clock parameters configuration:
               (+++) __HAL_RCC_ADC1_CLK_ENABLE(); (mandatory)

               HSI16 enable : (optional: if asynchronous clock selected)
               (+++) RCC_OscInitTypeDef   RCC_OscInitStructure;
               (+++) RCC_OscInitStructure.OscillatorType = RCC_OSCILLATORTYPE_HSI;
               (+++) RCC_OscInitStructure.HSI16CalibrationValue = RCC_HSICALIBRATION_DEFAULT;
               (+++) RCC_OscInitStructure.HSIState = RCC_HSI_ON;
               (+++) RCC_OscInitStructure.PLL...   (optional if used for system clock)
               (+++) HAL_RCC_OscConfig(&RCC_OscInitStructure);

        (++) ADC clock source and clock prescaler are configured at ADC level with
             parameter "ClockPrescaler" using function HAL_ADC_Init().

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

     *** Configuration of ADC, group regular, channels parameters ***
     ================================================================
     [..]

    (#) Configure the ADC parameters (resolution, data alignment, oversampler, continuous mode, ...)
        and regular group parameters (conversion trigger, sequencer, ...)
        using function HAL_ADC_Init().

    (#) Configure the channels for regular group parameters (channel number, 
        channel rank into sequencer, ..., into regular group)
        using function HAL_ADC_ConfigChannel().

    (#) Optionally, configure the analog watchdog parameters (channels
        monitored, thresholds, ...)
        using function HAL_ADC_AnalogWDGConfig().


    (#) When device is in mode low-power (low-power run, low-power sleep or stop mode), 
        function "HAL_ADCEx_EnableVREFINT()" must be called before function HAL_ADC_Init().
        In case of internal temperature sensor to be measured:
        function "HAL_ADCEx_EnableVREFINTTempSensor()" must be called similarilly

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
          (+++) Retrieve conversion results 
                using function HAL_ADC_GetValue()
          (+++) Stop conversion and disable the ADC peripheral 
                using function HAL_ADC_Stop()

        (++) ADC conversion by interruption: 
          (+++) Activate the ADC peripheral and start conversions
                using function HAL_ADC_Start_IT()
          (+++) Wait for ADC conversion completion by call of function
                HAL_ADC_ConvCpltCallback()
                (this function must be implemented in user program)
          (+++) Retrieve conversion results 
                using function HAL_ADC_GetValue()
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
  
     [..]    
            
    (@) Callback functions must be implemented in user program:
      (+@) HAL_ADC_ErrorCallback()
      (+@) HAL_ADC_LevelOutOfWindowCallback() (callback of analog watchdog)
      (+@) HAL_ADC_ConvCpltCallback()
      (+@) HAL_ADC_ConvHalfCpltCallback

     *** Deinitialization of ADC ***
     ============================================================
     [..]

    (#) Disable the ADC interface
      (++) ADC clock can be hard reset and disabled at RCC top level.
        (++) Hard reset of ADC peripherals
             using macro __ADCx_FORCE_RESET(), __ADCx_RELEASE_RESET().
        (++) ADC clock disable
             using the equivalent macro/functions as configuration step.
             (+++) Example:
                   Into HAL_ADC_MspDeInit() (recommended code location) or with
                   other device clock parameters configuration:
               (+++) RCC_OscInitStructure.OscillatorType = RCC_OSCILLATORTYPE_HSI;
               (+++) RCC_OscInitStructure.HSIState = RCC_HSI_OFF; (if not used for system clock)
               (+++) HAL_RCC_OscConfig(&RCC_OscInitStructure);

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
#include "stm32l0xx_hal.h"

/** @addtogroup STM32L0xx_HAL_Driver
  * @{
  */

#ifdef HAL_ADC_MODULE_ENABLED

/** @addtogroup ADC 
  * @brief ADC driver modules
  * @{
  */ 

/** @addtogroup ADC_Private
  * @{
  */
    
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Delay for ADC stabilization time.                                          */
/* Maximum delay is 1us (refer to device datasheet, parameter tSTART). */
/* Unit: us */
#define ADC_STAB_DELAY_US       ((uint32_t) 1U)

/* Delay for temperature sensor stabilization time. */
/* Maximum delay is 10us (refer to device datasheet, parameter tSTART). */
/* Unit: us */
#define ADC_TEMPSENSOR_DELAY_US ((uint32_t) 10U) 
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @addtogroup ADC_Private
  * @{
  */ 
static HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc);
static HAL_StatusTypeDef ADC_Disable(ADC_HandleTypeDef* hadc);
static HAL_StatusTypeDef ADC_ConversionStop(ADC_HandleTypeDef* hadc);
static void ADC_DMAConvCplt(DMA_HandleTypeDef *hdma);
static void ADC_DMAHalfConvCplt(DMA_HandleTypeDef *hdma);
static void ADC_DMAError(DMA_HandleTypeDef *hdma);
static void ADC_DelayMicroSecond(uint32_t microSecond);
/**
  * @}
  */

/** @addtogroup ADC_Exported_Functions
  * @{
  */ 

/** @addtogroup ADC_Exported_Functions_Group1
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
  *         depending on both possible clock sources: APB clock of HSI clock.
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
  * @note   When device is in mode low-power (low-power run, low-power sleep or stop mode), 
  *         function "HAL_ADCEx_EnableVREFINT()" must be called before function HAL_ADC_Init() 
  *         (in case of previous ADC operations: function HAL_ADC_DeInit() must be called first).
  *         In case of internal temperature sensor to be measured:
  *         function "HAL_ADCEx_EnableVREFINTTempSensor()" must be called similarilly.  
  * @param  hadc: ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc)
{
 
  /* Check ADC handle */
  if(hadc == NULL)
  {
    return HAL_ERROR;
  }
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_CLOCKPRESCALER(hadc->Init.ClockPrescaler));
  assert_param(IS_ADC_RESOLUTION(hadc->Init.Resolution));
  assert_param(IS_ADC_SAMPLE_TIME(hadc->Init.SamplingTime));
  assert_param(IS_ADC_SCAN_MODE(hadc->Init.ScanConvMode));  
  assert_param(IS_ADC_DATA_ALIGN(hadc->Init.DataAlign)); 
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DiscontinuousConvMode));
  assert_param(IS_ADC_EXTTRIG_EDGE(hadc->Init.ExternalTrigConvEdge));
  assert_param(IS_ADC_EXTTRIG(hadc->Init.ExternalTrigConv));   
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DMAContinuousRequests));
  assert_param(IS_ADC_OVERRUN(hadc->Init.Overrun));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.LowPowerAutoWait));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.LowPowerFrequencyMode));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.LowPowerAutoPowerOff));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.OversamplingMode));

  /* As prerequisite, into HAL_ADC_MspInit(), ADC clock must be configured    */
  /* at RCC top level depending on both possible clock sources:               */
  /* APB clock or HSI clock.                                                  */
  /* Refer to header of this file for more details on clock enabling procedure*/
  
  /* Actions performed only if ADC is coming from state reset:                */
  /* - Initialization of ADC MSP                                              */
  /* - ADC voltage regulator enable                                           */
  if(hadc->State == HAL_ADC_STATE_RESET)
  {
    /* Initialize ADC error code */
    ADC_CLEAR_ERRORCODE(hadc);
    
    /* Allocate lock resource and initialize it */
    hadc->Lock = HAL_UNLOCKED;

    /* Init the low level hardware */
    HAL_ADC_MspInit(hadc);
  }
  
  /* Configuration of ADC parameters if previous preliminary actions are      */ 
  /* correctly completed.                                                     */
  /* and if there is no conversion on going on regular group (ADC can be      */ 
  /* enabled anyway, in case of call of this function to update a parameter   */
  /* on the fly).                                                             */
  if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL) ||
     (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) != RESET)  )
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
        
    /* Process unlocked */
    __HAL_UNLOCK(hadc);
    return HAL_ERROR;
  }

  /* Set ADC state */
  ADC_STATE_CLR_SET(hadc->State,
                    HAL_ADC_STATE_REG_BUSY,
                    HAL_ADC_STATE_BUSY_INTERNAL);

  /* Parameters update conditioned to ADC state:                            */
  /* Parameters that can be updated only when ADC is disabled:              */
  /*  - ADC clock mode                                                      */
  /*  - ADC clock prescaler                                                 */
  /*  - ADC Resolution                                                      */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    /* Some parameters of this register are not reset, since they are set   */
    /* by other functions and must be kept in case of usage of this         */
    /* function on the fly (update of a parameter of ADC_InitTypeDef        */
    /* without needing to reconfigure all other ADC groups/channels         */
    /* parameters):                                                         */
    /*   - internal measurement paths: Vbat, temperature sensor, Vref       */
    /*     (set into HAL_ADC_ConfigChannel() )                              */
   
    /* Configuration of ADC clock: clock source PCLK or asynchronous with 
    selectable prescaler */
    __HAL_ADC_CLOCK_PRESCALER(hadc);
    
    /* Configuration of ADC:                                                */
    /*  - Resolution                                                        */
    hadc->Instance->CFGR1 &= ~( ADC_CFGR1_RES);
    hadc->Instance->CFGR1 |= hadc->Init.Resolution;    
  }
  
  /* Set the Low Frequency mode */
  ADC->CCR &= (uint32_t)~ADC_CCR_LFMEN;
  ADC->CCR |=__HAL_ADC_CCR_LOWFREQUENCY(hadc->Init.LowPowerFrequencyMode);  
   
  /* Enable voltage regulator (if disabled at this step) */
  if (HAL_IS_BIT_CLR(hadc->Instance->CR, ADC_CR_ADVREGEN))
  {
    /* Set ADVREGEN bit */
    hadc->Instance->CR |= ADC_CR_ADVREGEN;
  }
  
  /* Configuration of ADC:                                                    */
  /*  - Resolution                                                            */
  /*  - Data alignment                                                        */
  /*  - Scan direction                                                        */
  /*  - External trigger to start conversion                                  */
  /*  - External trigger polarity                                             */
  /*  - Continuous conversion mode                                            */
  /*  - DMA continuous request                                                */
  /*  - Overrun                                                               */
  /*  - AutoDelay feature                                                     */
  /*  - Discontinuous mode                                                    */
  hadc->Instance->CFGR1 &= ~(ADC_CFGR1_ALIGN  |
                             ADC_CFGR1_SCANDIR  |
                             ADC_CFGR1_EXTSEL |
                             ADC_CFGR1_EXTEN  |
                             ADC_CFGR1_CONT   |
                             ADC_CFGR1_DMACFG |
                             ADC_CFGR1_OVRMOD |
                             ADC_CFGR1_AUTDLY |
                             ADC_CFGR1_AUTOFF |
                             ADC_CFGR1_DISCEN);
  
  hadc->Instance->CFGR1 |= (hadc->Init.DataAlign                             |
                            ADC_SCANDIR(hadc->Init.ScanConvMode)             |
                            ADC_CONTINUOUS(hadc->Init.ContinuousConvMode)    | 
                            ADC_DMACONTREQ(hadc->Init.DMAContinuousRequests) |
                            hadc->Init.Overrun                               |
                            __HAL_ADC_CFGR1_AutoDelay(hadc->Init.LowPowerAutoWait) |
                            __HAL_ADC_CFGR1_AUTOFF(hadc->Init.LowPowerAutoPowerOff));
  
  /* Enable external trigger if trigger selection is different of software  */
  /* start.                                                                 */
  /* Note: This configuration keeps the hardware feature of parameter       */
  /*       ExternalTrigConvEdge "trigger edge none" equivalent to           */
  /*       software start.                                                  */
  if (hadc->Init.ExternalTrigConv != ADC_SOFTWARE_START)
  {
    hadc->Instance->CFGR1 |= hadc->Init.ExternalTrigConv |
                             hadc->Init.ExternalTrigConvEdge;
  }
  
  /* Enable discontinuous mode only if continuous mode is disabled */
  if (hadc->Init.DiscontinuousConvMode == ENABLE)
  {
    if (hadc->Init.ContinuousConvMode == DISABLE)
    {
      /* Enable the selected ADC group regular discontinuous mode */
      hadc->Instance->CFGR1 |= (ADC_CFGR1_DISCEN);
    }
    else
    {
      /* ADC regular group discontinuous was intended to be enabled,        */
      /* but ADC regular group modes continuous and sequencer discontinuous */
      /* cannot be enabled simultaneously.                                  */
      
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
      
      /* Set ADC error code to ADC IP internal error */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
    }
  }
  
  if (hadc->Init.OversamplingMode == ENABLE)
  {
    assert_param(IS_ADC_OVERSAMPLING_RATIO(hadc->Init.Oversample.Ratio));
    assert_param(IS_ADC_RIGHT_BIT_SHIFT(hadc->Init.Oversample.RightBitShift));
    assert_param(IS_ADC_TRIGGERED_OVERSAMPLING_MODE(hadc->Init.Oversample.TriggeredMode));

    /* Configuration of Oversampler:                                          */
    /*  - Oversampling Ratio                                                  */
    /*  - Right bit shift                                                     */
    /*  - Triggered mode                                                      */
    
    hadc->Instance->CFGR2 &= ~( ADC_CFGR2_OVSR |
                                ADC_CFGR2_OVSS |
                                ADC_CFGR2_TOVS );
    
    hadc->Instance->CFGR2 |= ( hadc->Init.Oversample.Ratio         |
                               hadc->Init.Oversample.RightBitShift             |
                               hadc->Init.Oversample.TriggeredMode );
    
    /* Enable OverSampling mode */
     hadc->Instance->CFGR2 |= ADC_CFGR2_OVSE;
  }
  else
  {
    if(HAL_IS_BIT_SET(hadc->Instance->CFGR2, ADC_CFGR2_OVSE))
    {
      /* Disable OverSampling mode if needed */
      hadc->Instance->CFGR2 &= ~ADC_CFGR2_OVSE;
    }
  }    
  
  /* Clear the old sampling time */
  hadc->Instance->SMPR &= (uint32_t)(~ADC_SMPR_SMPR);
  
  /* Set the new sample time */
  hadc->Instance->SMPR |= hadc->Init.SamplingTime;
  
  /* Clear ADC error code */
  ADC_CLEAR_ERRORCODE(hadc);

  /* Set the ADC state */
  ADC_STATE_CLR_SET(hadc->State,
                    HAL_ADC_STATE_BUSY_INTERNAL,
                    HAL_ADC_STATE_READY);


  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Deinitialize the ADC peripheral registers to their default reset
  *         values, with deinitialization of the ADC MSP.
  * @note   For devices with several ADCs: reset of ADC common registers is done 
  *         only if all ADCs sharing the same common group are disabled.
  *         If this is not the case, reset of these common parameters reset is  
  *         bypassed without error reporting: it can be the intended behavior in
  *         case of reset of a single ADC while the other ADCs sharing the same 
  *         common group is still running.
  * @param  hadc: ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check ADC handle */
  if(hadc == NULL)
  {
     return HAL_ERROR;
  }
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Set ADC state */
  SET_BIT(hadc->State, HAL_ADC_STATE_BUSY_INTERNAL);
  
  /* Stop potential conversion on going, on regular group */
  tmp_hal_status = ADC_ConversionStop(hadc);
  
  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {   
    /* Disable the ADC peripheral */
    tmp_hal_status = ADC_Disable(hadc);
    
    /* Check if ADC is effectively disabled */
    if (tmp_hal_status != HAL_ERROR)
    {
      /* Change ADC state */
      hadc->State = HAL_ADC_STATE_READY;
    }
  }  
  
  
  /* Configuration of ADC parameters if previous preliminary actions are      */ 
  /* correctly completed.                                                     */
  if (tmp_hal_status != HAL_ERROR)
  {
    
    /* ========== Reset ADC registers ========== */
    /* Reset register IER */
    __HAL_ADC_DISABLE_IT(hadc, (ADC_IT_AWD | ADC_IT_OVR | ADC_IT_EOCAL | ADC_IT_EOS |  \
                                ADC_IT_EOC | ADC_IT_RDY | ADC_IT_EOSMP ));
  
        
    /* Reset register ISR */
    __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_AWD | ADC_FLAG_EOCAL | ADC_FLAG_OVR | ADC_FLAG_EOS |  \
                                ADC_FLAG_EOC | ADC_FLAG_EOSMP | ADC_FLAG_RDY));
  
    
    /* Reset register CR */
    /* Disable voltage regulator */
    /* Note: Regulator disable useful for power saving */
    /* Reset ADVREGEN bit */
    hadc->Instance->CR &= ~ADC_CR_ADVREGEN;
    
    /* Bits ADC_CR_ADSTP, ADC_CR_ADSTART are in access mode "read-set": no direct reset applicable */
    /* No action */
    
    /* Reset register CFGR1 */
    hadc->Instance->CFGR1 &= ~(ADC_CFGR1_AWDCH  | ADC_CFGR1_AWDEN  | ADC_CFGR1_AWDSGL | \
                               ADC_CFGR1_DISCEN | ADC_CFGR1_AUTOFF | ADC_CFGR1_AUTDLY | \
                               ADC_CFGR1_CONT   | ADC_CFGR1_OVRMOD | ADC_CFGR1_EXTEN  | \
                               ADC_CFGR1_EXTSEL | ADC_CFGR1_ALIGN  | ADC_CFGR1_RES    | \
                               ADC_CFGR1_SCANDIR| ADC_CFGR1_DMACFG | ADC_CFGR1_DMAEN);
  
    /* Reset register CFGR2 */
    hadc->Instance->CFGR2 &= ~(ADC_CFGR2_TOVS  | ADC_CFGR2_OVSS  | ADC_CFGR2_OVSR | \
                               ADC_CFGR2_OVSE  | ADC_CFGR2_CKMODE );
  
    
    /* Reset register SMPR */
    hadc->Instance->SMPR &= ~(ADC_SMPR_SMPR);
    
    /* Reset register TR */
    hadc->Instance->TR &= ~(ADC_TR_LT | ADC_TR_HT);
    
    /* Reset register CALFACT */
    hadc->Instance->CALFACT &= ~(ADC_CALFACT_CALFACT);
  
  
  
  
    
    /* Reset register DR */
    /* bits in access mode read only, no direct reset applicable*/
  
    /* Reset register CALFACT */
    hadc->Instance->CALFACT &= ~(ADC_CALFACT_CALFACT);
  
    /* ========== Hard reset ADC peripheral ========== */
    /* Performs a global reset of the entire ADC peripheral: ADC state is     */
    /* forced to a similar state after device power-on.                       */
    /* If needed, copy-paste and uncomment the following reset code into      */
    /* function "void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)":              */
    /*                                                                        */
    /*  __HAL_RCC_ADC1_FORCE_RESET()                                                  */
    /*  __HAL_RCC_ADC1_RELEASE_RESET()                                                */
  
    /* DeInit the low level hardware */
    HAL_ADC_MspDeInit(hadc);
    
    /* Set ADC error code to none */
    ADC_CLEAR_ERRORCODE(hadc);
    
      /* Set ADC state */
    hadc->State = HAL_ADC_STATE_RESET; 
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}

    
/**
  * @brief  Initializes the ADC MSP.
  * @param  hadc: ADC handle
  * @retval None
  */
__weak void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_ADC_MspInit could be implemented in the user file
   */ 
}

/**
  * @brief  DeInitializes the ADC MSP.
  * @param  hadc: ADC handle
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

/** @addtogroup ADC_Exported_Functions_Group2
 *  @brief    I/O operation functions 
 *
@verbatim   
 ===============================================================================
             ##### IO operation functions #####
 ===============================================================================  
    [..]  This section provides functions allowing to:
      (+) Start conversion of regular group.
      (+) Stop conversion of regular group.
      (+) Poll for conversion complete on regular group.
      (+) poll for conversion event.
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
  * @param  hadc: ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Perform ADC enable and conversion start if no conversion is on going */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
  {
    /* Process locked */
    __HAL_LOCK(hadc);
    
    /* Enable the ADC peripheral */
    /* If low power mode AutoPowerOff is enabled, power-on/off phases are       */
    /* performed automatically by hardware.                                     */
    if (hadc->Init.LowPowerAutoPowerOff != ENABLE)
    {
      tmp_hal_status = ADC_Enable(hadc);
    }
    
    /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to regular group conversion results   */
      /* - Set state bitfield related to regular operation                    */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP,
                        HAL_ADC_STATE_REG_BUSY);
      
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
        
    /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
    __HAL_UNLOCK(hadc);
      
      /* Clear regular group conversion flag and overrun flag */
      /* (To ensure of no unknown state from potential previous ADC           */
      /* operations)                                                          */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));
      
      /* Enable conversion of regular group.                                  */
      /* If software start has been selected, conversion starts immediately.  */
      /* If external trigger has been selected, conversion will start at next */
      /* trigger event.                                                       */
      hadc->Instance->CR |= ADC_CR_ADSTART;
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Stop ADC conversion of regular group, disable ADC peripheral.
  * @param  hadc: ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential conversion on going, on regular group */
  tmp_hal_status = ADC_ConversionStop(hadc);
  
  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {
    /* 2. Disable the ADC peripheral */
    tmp_hal_status = ADC_Disable(hadc);
    
    /* Check if ADC is effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_REG_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }  
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Wait for regular group conversion to be completed.
  * @note   ADC conversion flags EOS (end of sequence) and EOC (end of
  *         conversion) are cleared by this function, with an exception:
  *         if low power feature "LowPowerAutoWait" is enabled, flags are 
  *         not cleared to not interfere with this feature until data register
  *         is read using function HAL_ADC_GetValue().
  * @note   This function cannot be used in a particular setup: ADC configured 
  *         in DMA mode and polling for end of each conversion (ADC init
  *         parameter "EOCSelection" set to ADC_EOC_SINGLE_CONV).
  *         In this case, DMA resets the flag EOC and polling cannot be
  *         performed on each conversion. Nevertheless, polling can still 
  *         be performed on the complete sequence (ADC init
  *         parameter "EOCSelection" set to ADC_EOC_SEQ_CONV).
  * @param  hadc: ADC handle
  * @param  Timeout: Timeout value in millisecond.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout)
{
  uint32_t tickstart;
  uint32_t tmp_Flag_EOC;
 
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_EOC_SELECTION(hadc->Init.EOCSelection));

  /* If end of conversion selected to end of sequence */
  if (hadc->Init.EOCSelection == ADC_EOC_SEQ_CONV)
  {
    tmp_Flag_EOC = ADC_FLAG_EOS;
  }
  /* If end of conversion selected to end of each conversion */
  else /* ADC_EOC_SINGLE_CONV */
  {
    /* Verification that ADC configuration is compliant with polling for      */
    /* each conversion:                                                       */
    /* Particular case is ADC configured in DMA mode and ADC sequencer with   */
    /* several ranks and polling for end of each conversion.                  */
    /* For code simplicity sake, this particular case is generalized to       */
    /* ADC configured in DMA mode and and polling for end of each conversion. */
    if (HAL_IS_BIT_SET(hadc->Instance->CFGR1, ADC_CFGR1_DMAEN))
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
      
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
      
      return HAL_ERROR;
    }
    else
    {
      tmp_Flag_EOC = (ADC_FLAG_EOC | ADC_FLAG_EOS);
    }
  }
  
  /* Get tick count */
  tickstart = HAL_GetTick();
  
  /* Wait until End of Conversion flag is raised */
  while(HAL_IS_BIT_CLR(hadc->Instance->ISR, tmp_Flag_EOC))
  {
    /* Check if timeout is disabled (set to infinite wait) */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U) || ((HAL_GetTick()-tickstart) > Timeout))
      {
        /* Update ADC state machine to timeout */
        SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);
        
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
        
        return HAL_TIMEOUT;
      }
    }
  }
  
  /* Update ADC state machine */
  SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC);
  
  /* Determine whether any further conversion upcoming on group regular       */
  /* by external trigger, continuous mode or scan sequence on going.          */
  if(ADC_IS_SOFTWARE_START_REGULAR(hadc)        && 
     (hadc->Init.ContinuousConvMode == DISABLE)   )
  {
    /* If End of Sequence is reached, disable interrupts */
    if( __HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOS) )
    {
      /* Allowed to modify bits ADC_IT_EOC/ADC_IT_EOS only if bit             */
      /* ADSTART==0 (no conversion on going)                                  */
      if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
      {
        /* Disable ADC end of single conversion interrupt on group regular */
        /* Note: Overrun interrupt was enabled with EOC interrupt in          */
        /* HAL_Start_IT(), but is not disabled here because can be used       */
        /* by overrun IRQ process below.                                      */
        __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC | ADC_IT_EOS);
        
        /* Set ADC state */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_REG_BUSY,
                          HAL_ADC_STATE_READY);
      }
      else
      {
        /* Change ADC state to error state */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
        
        /* Set ADC error code to ADC IP internal error */
        SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
      }
    }
  }
  
  /* Clear end of conversion flag of regular group if low power feature       */
  /* "LowPowerAutoWait " is disabled, to not interfere with this feature      */
  /* until data register is read using function HAL_ADC_GetValue().           */
  if (hadc->Init.LowPowerAutoWait == DISABLE)
  {
    /* Clear regular group conversion flag */
    __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS));
  }
  
  /* Return ADC state */
  return HAL_OK;
}

/**
  * @brief  Poll for conversion event.
  * @param  hadc: ADC handle
  * @param  EventType: the ADC event type.
  *          This parameter can be one of the following values:
  *            @arg ADC_AWD_EVENT: ADC Analog watchdog event
  *            @arg ADC_OVR_EVENT: ADC Overrun event
  * @param  Timeout: Timeout value in millisecond.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_PollForEvent(ADC_HandleTypeDef* hadc, uint32_t EventType, uint32_t Timeout)
{
  uint32_t tickstart = 0U; 
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_EVENT_TYPE(EventType));
  
  /* Get tick count */
  tickstart = HAL_GetTick();
  
  /* Check selected event flag */
  while(__HAL_ADC_GET_FLAG(hadc, EventType) == RESET)
  {
    /* Check if timeout is disabled (set to infinite wait) */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
      {
        /* Update ADC state machine to timeout */
        SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);
        
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
        
        return HAL_TIMEOUT;
      }
    }
  }
  
  switch(EventType)
  {
  /* Analog watchdog (level out of window) event */
  case ADC_AWD_EVENT:
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD1);
    
    /* Clear ADC analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD);
    break;
    
  /* Overrun event */
  default: /* Case ADC_OVR_EVENT */
    /* If overrun is set to overwrite previous data, overrun event is not     */
    /* considered as an error.                                                */
    /* (cf ref manual "Managing conversions without using the DMA and without */
    /* overrun ")                                                             */
    if (hadc->Init.Overrun == ADC_OVR_DATA_PRESERVED)
    {
      /* Set ADC state */
      SET_BIT(hadc->State, HAL_ADC_STATE_REG_OVR);
        
      /* Set ADC error code to overrun */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_OVR);
    }
    
    /* Clear ADC Overrun flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_OVR);
    break;
  }
  
  /* Return ADC state */
  return HAL_OK;
}

/**
  * @brief  Enables ADC, starts conversion of regular group with interruption.
  *         Interruptions enabled in this function:
  *          - EOC (end of conversion of regular group) or EOS (end of 
  *            sequence of regular group) depending on ADC initialization 
  *            parameter "EOCSelection"
  *          - overrun (if available)
  *         Each of these interruptions has its dedicated callback function.
  * @param  hadc: ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
    
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Perform ADC enable and conversion start if no conversion is on going */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
  {
    /* Process locked */
    __HAL_LOCK(hadc);
    
    /* Enable the ADC peripheral */
    /* If low power mode AutoPowerOff is enabled, power-on/off phases are       */
    /* performed automatically by hardware.                                     */
    if (hadc->Init.LowPowerAutoPowerOff != ENABLE)
    {
      tmp_hal_status = ADC_Enable(hadc);
    }
    
    /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to regular group conversion results   */
      /* - Set state bitfield related to regular operation                    */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP,
                        HAL_ADC_STATE_REG_BUSY);
      
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
      
      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);
      
      /* Clear regular group conversion flag and overrun flag */
      /* (To ensure of no unknown state from potential previous ADC           */
      /* operations)                                                          */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));
      
      /* Enable ADC end of conversion interrupt */
      /* Enable ADC overrun interrupt */
      assert_param(IS_ADC_EOC_SELECTION(hadc->Init.EOCSelection));
      switch(hadc->Init.EOCSelection)
      {
        case ADC_EOC_SEQ_CONV: 
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC);
          __HAL_ADC_ENABLE_IT(hadc, (ADC_IT_EOS | ADC_IT_OVR));
          break;
        /* case ADC_EOC_SINGLE_CONV */
        default:
          __HAL_ADC_ENABLE_IT(hadc, (ADC_IT_EOC | ADC_IT_EOS | ADC_IT_OVR));
          break;
      }
      
      /* Enable conversion of regular group.                                  */
      /* If software start has been selected, conversion starts immediately.  */
      /* If external trigger has been selected, conversion will start at next */
      /* trigger event.                                                       */
      hadc->Instance->CR |= ADC_CR_ADSTART;
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }

  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Stop ADC conversion of regular group, disable interruption of 
  *         end-of-conversion, disable ADC peripheral.
  * @param  hadc: ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
    
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential conversion on going, on regular group */
  tmp_hal_status = ADC_ConversionStop(hadc);
  
  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {
    /* Disable ADC end of conversion interrupt for regular group */
    /* Disable ADC overrun interrupt */
    __HAL_ADC_DISABLE_IT(hadc, (ADC_IT_EOC | ADC_IT_EOS | ADC_IT_OVR));
    
    /* 2. Disable the ADC peripheral */
    tmp_hal_status = ADC_Disable(hadc);
    
    /* Check if ADC is effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_REG_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Enables ADC, starts conversion of regular group and transfers result
  *         through DMA.
  *         Interruptions enabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun
  *         Each of these interruptions has its dedicated callback function.
  * @param  hadc: ADC handle
  * @param  pData: The destination Buffer address.
  * @param  Length: The length of data to be transferred from ADC peripheral to memory.
  * @retval None
  */
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Perform ADC enable and conversion start if no conversion is on going */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
  {
    /* Process locked */
    __HAL_LOCK(hadc);
    
      /* Enable the ADC peripheral */
    /* If low power mode AutoPowerOff is enabled, power-on/off phases are       */
    /* performed automatically by hardware.                                     */
    if (hadc->Init.LowPowerAutoPowerOff != ENABLE)
    {
      tmp_hal_status = ADC_Enable(hadc);
    }
    
    /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to regular group conversion results   */
      /* - Set state bitfield related to regular operation                    */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP,
                        HAL_ADC_STATE_REG_BUSY);
      
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
      
      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);
      
      /* Set the DMA transfer complete callback */
      hadc->DMA_Handle->XferCpltCallback = ADC_DMAConvCplt;
  
      /* Set the DMA half transfer complete callback */
      hadc->DMA_Handle->XferHalfCpltCallback = ADC_DMAHalfConvCplt;
      
      /* Set the DMA error callback */
      hadc->DMA_Handle->XferErrorCallback = ADC_DMAError;
      
      
      /* Manage ADC and DMA start: ADC overrun interruption, DMA start, ADC   */
      /* start (in case of SW start):                                         */
      
      /* Clear regular group conversion flag and overrun flag */
      /* (To ensure of no unknown state from potential previous ADC           */
      /* operations)                                                          */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));
      
      /* Enable ADC overrun interrupt */
      __HAL_ADC_ENABLE_IT(hadc, ADC_IT_OVR);
      
      /* Enable ADC DMA mode */
      hadc->Instance->CFGR1 |= ADC_CFGR1_DMAEN;
      
      /* Start the DMA channel */
      HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&hadc->Instance->DR, (uint32_t)pData, Length);
       
      /* Enable conversion of regular group.                                  */
      /* If software start has been selected, conversion starts immediately.  */
      /* If external trigger has been selected, conversion will start at next */
      /* trigger event.                                                       */
      hadc->Instance->CR |= ADC_CR_ADSTART;
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Stop ADC conversion of regular group, disable ADC DMA transfer, disable 
  *         ADC peripheral.
  *         Each of these interruptions has its dedicated callback function.
  * @param  hadc: ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* 1. Stop potential conversion on going, on regular group */
  tmp_hal_status = ADC_ConversionStop(hadc);
  
  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {
    /* Disable ADC DMA (ADC DMA configuration ADC_CFGR_DMACFG is kept) */
    hadc->Instance->CFGR1 &= ~ADC_CFGR1_DMAEN;
    
    /* Disable the DMA channel (in case of DMA in circular mode or stop while */
    /* while DMA transfer is on going)                                        */
    tmp_hal_status = HAL_DMA_Abort(hadc->DMA_Handle);   
    
    /* Check if DMA channel effectively disabled */
    if (tmp_hal_status != HAL_OK)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);
    }
    
    /* Disable ADC overrun interrupt */
    __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);
    
    /* 2. Disable the ADC peripheral */
    /* Update "tmp_hal_status" only if DMA channel disabling passed, to keep  */
    /* in memory a potential failing status.                                  */
    if (tmp_hal_status == HAL_OK)
    {
      tmp_hal_status = ADC_Disable(hadc);
    }
    else
    {
      ADC_Disable(hadc);
    }

    /* Check if ADC is effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_REG_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }  
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Get ADC regular group conversion result.
  * @note   Reading DR register automatically clears EOC (end of conversion of
  *         regular group) flag.
  * @note   This function does not clear ADC flag EOS
  *         (ADC group regular end of sequence conversion).
  *         Occurrence of flag EOS rising:
  *          - If sequencer is composed of 1 rank, flag EOS is equivalent
  *            to flag EOC.
  *          - If sequencer is composed of several ranks, during the scan
  *            sequence flag EOC only is raised, at the end of the scan sequence
  *            both flags EOC and EOS are raised.
  *         To clear this flag, either use function:
  *         in programming model IT: @ref HAL_ADC_IRQHandler(), in programming
  *         model polling: @ref HAL_ADC_PollForConversion()
  *         or @ref __HAL_ADC_CLEAR_FLAG(&hadc, ADC_FLAG_EOS).
  * @param  hadc: ADC handle
  * @retval Converted value
  */
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc)
{       
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Note: EOC flag is not cleared here by software because automatically     */
  /*       cleared by hardware when reading register DR.                      */
  
  /* Return ADC converted value */ 
  return hadc->Instance->DR;
}

/**
  * @brief  Handles ADC interrupt request.  
  * @param  hadc: ADC handle
  * @retval None
  */
void HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  
  /* ========== Check End of Conversion flag for regular group ========== */
  if( (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOC) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_EOC)) || 
      (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOS) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_EOS)) )
  {
    /* Update state machine on conversion status if not in error state */
    if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL))
    {
      /* Set ADC state */
      SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC); 
    }

    /* Determine whether any further conversion upcoming on group regular     */
    /* by external trigger, continuous mode or scan sequence on going.        */
    if(ADC_IS_SOFTWARE_START_REGULAR(hadc)        && 
       (hadc->Init.ContinuousConvMode == DISABLE)   )
    {
      /* If End of Sequence is reached, disable interrupts */
      if( __HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOS) )
      {
        /* Allowed to modify bits ADC_IT_EOC/ADC_IT_EOS only if bit           */
          /* ADSTART==0 (no conversion on going)                                */
        if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
          {
          /* Disable ADC end of single conversion interrupt on group regular */
          /* Note: Overrun interrupt was enabled with EOC interrupt in        */
          /* HAL_Start_IT(), but is not disabled here because can be used     */
          /* by overrun IRQ process below.                                    */
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC | ADC_IT_EOS);
          
          /* Set ADC state */
          ADC_STATE_CLR_SET(hadc->State,
                            HAL_ADC_STATE_REG_BUSY,
                            HAL_ADC_STATE_READY);
          }
          else
          {
            /* Change ADC state to error state */
          SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
          
            /* Set ADC error code to ADC IP internal error */
          SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
        }
      }
    }    

    /* Conversion complete callback */
    /* Note: into callback, to determine if conversion has been triggered     */
    /*       from EOC or EOS, possibility to use:                             */
    /*        " if( __HAL_ADC_GET_FLAG(&hadc, ADC_FLAG_EOS)) "                */
    HAL_ADC_ConvCpltCallback(hadc);
    
    
    /* Clear regular group conversion flag */
    /* Note: in case of overrun set to ADC_OVR_DATA_PRESERVED, end of         */
    /*       conversion flags clear induces the release of the preserved data.*/
    /*       Therefore, if the preserved data value is needed, it must be     */
    /*       read preliminarily into HAL_ADC_ConvCpltCallback().              */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS) );
    }
  
  /* ========== Check Analog watchdog flags ========== */
  if(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_AWD) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_AWD))
  {
      /* Set ADC state */
      SET_BIT(hadc->State, HAL_ADC_STATE_AWD1);
    
    /* Level out of window callback */
    HAL_ADC_LevelOutOfWindowCallback(hadc);
    
    /* Clear ADC Analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD);    
   
  }  
  
  
  /* ========== Check Overrun flag ========== */
  if(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_OVR) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_OVR))
  {
    /* If overrun is set to overwrite previous data (default setting),        */
    /* overrun event is not considered as an error.                           */
    /* (cf ref manual "Managing conversions without using the DMA and without */
    /* overrun ")                                                             */
    /* Exception for usage with DMA overrun event always considered as an     */
    /* error.                                                                 */
    if ((hadc->Init.Overrun == ADC_OVR_DATA_PRESERVED)            ||
        HAL_IS_BIT_SET(hadc->Instance->CFGR1, ADC_CFGR1_DMAEN)  )
    {
      /* Set ADC error code to overrun */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_OVR);
      
      /* Clear ADC overrun flag */
      __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_OVR);
      
      /* Error callback */ 
      HAL_ADC_ErrorCallback(hadc);
    }
    
    /* Clear the Overrun flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_OVR);
  }
}

/**
  * @brief  Conversion complete callback in non blocking mode 
  * @param  hadc: ADC handle
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
  * @param  hadc: ADC handle
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
  * @param  hadc: ADC handle
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
  * @param  hadc: ADC handle
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

/** @addtogroup ADC_Exported_Functions_Group3
 *  @brief      Peripheral Control functions 
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
  *         VrefInt/Vlcd(STM32L0x3xx only)/TempSensor.
  *         Sampling time constraints must be respected (sampling time can be 
  *         adjusted in function of ADC clock frequency and sampling time 
  *         setting).
  *         Refer to device datasheet for timings values, parameters TS_vrefint,
  *         TS_vlcd (STM32L0x3xx only), TS_temp (values rough order: 5us to 17us).
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
  * @param  hadc: ADC handle
  * @param  sConfig: Structure of ADC channel for regular group.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_CHANNEL(sConfig->Channel));
  assert_param(IS_ADC_RANK(sConfig->Rank));
  
  /* Process locked */
  __HAL_LOCK(hadc);    
  
  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular group:                                    */
  /*  - Channel number                                                        */
  /*  - Management of internal measurement channels: Vbat/VrefInt/TempSensor  */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) != RESET)
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
    /* Process unlocked */
    __HAL_UNLOCK(hadc);
    return HAL_ERROR;
  }
  
  if (sConfig->Rank != ADC_RANK_NONE)
  {
    /* Enable selected channels */
    hadc->Instance->CHSELR |= (uint32_t)(sConfig->Channel & ADC_CHANNEL_MASK);
    
    /* Management of internal measurement channels: Vlcd (STM32L0x3xx only)/VrefInt/TempSensor */
    /* internal measurement paths enable: If internal channel selected, enable  */
    /* dedicated internal buffers and path.                                     */
    
    /* If Temperature sensor channel is selected, then enable the internal      */
    /* buffers and path  */
    if (((sConfig->Channel & ADC_CHANNEL_MASK) & ADC_CHANNEL_TEMPSENSOR ) == (ADC_CHANNEL_TEMPSENSOR & ADC_CHANNEL_MASK))
    {
      ADC->CCR |= ADC_CCR_TSEN;   
      
      /* Delay for temperature sensor stabilization time */
      ADC_DelayMicroSecond(ADC_TEMPSENSOR_DELAY_US);
    }
    
    /* If VRefInt channel is selected, then enable the internal buffers and path   */
    if (((sConfig->Channel & ADC_CHANNEL_MASK) & ADC_CHANNEL_VREFINT) == (ADC_CHANNEL_VREFINT & ADC_CHANNEL_MASK))
    {
      ADC->CCR |= ADC_CCR_VREFEN;   
    }
    
#if defined (STM32L053xx) || defined (STM32L063xx) || defined (STM32L073xx) || defined (STM32L083xx)
    /* If Vlcd channel is selected, then enable the internal buffers and path   */
    if (((sConfig->Channel & ADC_CHANNEL_MASK) & ADC_CHANNEL_VLCD) == (ADC_CHANNEL_VLCD & ADC_CHANNEL_MASK))
    {
      ADC->CCR |= ADC_CCR_VLCDEN;   
    }
#endif
  }
  else
  {
    /* Regular sequence configuration */
    /* Reset the channel selection register from the selected channel */
    hadc->Instance->CHSELR &= ~((uint32_t)(sConfig->Channel & ADC_CHANNEL_MASK));
    
    /* Management of internal measurement channels: VrefInt/TempSensor/Vbat */
    /* internal measurement paths disable: If internal channel selected,    */
    /* disable dedicated internal buffers and path.                         */
    if (((sConfig->Channel & ADC_CHANNEL_MASK) & ADC_CHANNEL_TEMPSENSOR ) == (ADC_CHANNEL_TEMPSENSOR & ADC_CHANNEL_MASK))
    {
      ADC->CCR &= ~ADC_CCR_TSEN;   
    }
    
    /* If VRefInt channel is selected, then enable the internal buffers and path   */
    if (((sConfig->Channel & ADC_CHANNEL_MASK) & ADC_CHANNEL_VREFINT) == (ADC_CHANNEL_VREFINT & ADC_CHANNEL_MASK))
    {
      ADC->CCR &= ~ADC_CCR_VREFEN;   
    }
    
#if defined (STM32L053xx) || defined (STM32L063xx) || defined (STM32L073xx) || defined (STM32L083xx)
    /* If Vlcd channel is selected, then enable the internal buffers and path   */
    if (((sConfig->Channel & ADC_CHANNEL_MASK) & ADC_CHANNEL_VLCD) == (ADC_CHANNEL_VLCD & ADC_CHANNEL_MASK))
    {
      ADC->CCR &= ~ADC_CCR_VLCDEN;   
    }
#endif
  }
 
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return HAL_OK;
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
  * @param  hadc: ADC handle
  * @param  AnalogWDGConfig: Structure of ADC analog watchdog configuration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  
  uint32_t tmpAWDHighThresholdShifted;
  uint32_t tmpAWDLowThresholdShifted;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_ANALOG_WATCHDOG_MODE(AnalogWDGConfig->WatchdogMode));
  assert_param(IS_ADC_CHANNEL(AnalogWDGConfig->Channel));
  assert_param(IS_FUNCTIONAL_STATE(AnalogWDGConfig->ITMode));
  
  /* Verify if threshold is within the selected ADC resolution */
  assert_param(IS_ADC_RANGE(ADC_GET_RESOLUTION(hadc), AnalogWDGConfig->HighThreshold));
  assert_param(IS_ADC_RANGE(ADC_GET_RESOLUTION(hadc), AnalogWDGConfig->LowThreshold));
  
  if(AnalogWDGConfig->WatchdogMode == ADC_ANALOGWATCHDOG_SINGLE_REG)
  {
    assert_param(IS_ADC_CHANNEL(AnalogWDGConfig->Channel));
  }
  
  /* Process locked */
  __HAL_LOCK(hadc);
  
  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular group:                                    */
  /*  - Analog watchdog channels                                              */
  /*  - Analog watchdog thresholds                                            */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
  {
    /* Configure ADC Analog watchdog interrupt */
    if(AnalogWDGConfig->ITMode == ENABLE)
    {
      /* Enable the ADC Analog watchdog interrupt */
      __HAL_ADC_ENABLE_IT(hadc, ADC_IT_AWD);
    }
    else
    {
      /* Disable the ADC Analog watchdog interrupt */
      __HAL_ADC_DISABLE_IT(hadc, ADC_IT_AWD);
    }
      
    /* Configuration of analog watchdog:                                        */
    /*  - Set the analog watchdog mode                                          */
    /*  - Set the Analog watchdog channel (is not used if watchdog              */
    /*    mode "all channels": ADC_CFGR1_AWD1SGL=0)                             */
    hadc->Instance->CFGR1 &= ~( ADC_CFGR1_AWDSGL |
                               ADC_CFGR1_AWDEN  |
                               ADC_CFGR1_AWDCH);
    
    hadc->Instance->CFGR1 |= ( AnalogWDGConfig->WatchdogMode |
                              (AnalogWDGConfig->Channel & ADC_CHANNEL_AWD_MASK));
    
    
    /* Shift the offset in function of the selected ADC resolution: Thresholds  */
    /* have to be left-aligned on bit 11, the LSB (right bits) are set to 0     */
    tmpAWDHighThresholdShifted = ADC_AWD1THRESHOLD_SHIFT_RESOLUTION(hadc, AnalogWDGConfig->HighThreshold);
    tmpAWDLowThresholdShifted  = ADC_AWD1THRESHOLD_SHIFT_RESOLUTION(hadc, AnalogWDGConfig->LowThreshold);
    
    /* Clear High & Low high thresholds */
    hadc->Instance->TR &= (uint32_t) ~ (ADC_TR_HT | ADC_TR_LT);
    
    /* Set the high threshold */
    hadc->Instance->TR = ADC_TRX_HIGHTHRESHOLD (tmpAWDHighThresholdShifted);
    /* Set the low threshold */
    hadc->Instance->TR |= tmpAWDLowThresholdShifted;  
  }
  else
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
  
    tmp_hal_status = HAL_ERROR;
  }
  
  
  /* Process unlocked */
  __HAL_UNLOCK(hadc);
  
  /* Return function status */
  return tmp_hal_status;
}

/**
  * @}
  */

/** @addtogroup ADC_Exported_Functions_Group4
 *  @brief   ADC Peripheral State functions 
 *
@verbatim   
 ===============================================================================
            ##### ADC Peripheral State functions #####
 ===============================================================================  
    [..]
    This subsection provides functions allowing to
      (+) Check the ADC state.
      (+) handle ADC interrupt request.
         
@endverbatim
  * @{
  */

/**
  * @brief  return the ADC state
  * @param  hadc: ADC handle
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
  * @param  hadc: ADC handle
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


/** @addtogroup ADC_Private
  * @{
  */

/**
  * @brief  Enable the selected ADC.
  * @note   Prerequisite condition to use this function: ADC must be disabled
  *         and voltage regulator must be enabled (done into HAL_ADC_Init()).
  * @note If low power mode AutoPowerOff is enabled, power-on/off phases are
  * performed automatically by hardware.
  * In this mode, this function is useless and must not be called because 
  * flag ADC_FLAG_RDY is not usable.
  * Therefore, this function must be called under condition of
  * "if (hadc->Init.LowPowerAutoPowerOff != ENABLE)". 
  * @param  hadc: ADC handle
  * @retval HAL status.
  */
static HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc)
{
  uint32_t tickstart = 0U;

  /* ADC enable and wait for ADC ready (in case of ADC is disabled or         */
  /* enabling phase not yet completed: flag ADC ready not yet set).           */
  /* Timeout implemented to not be stuck if ADC cannot be enabled (possible   */
  /* causes: ADC clock not running, ...).                                     */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    /* Check if conditions to enable the ADC are fulfilled */
    if (ADC_ENABLING_CONDITIONS(hadc) == RESET)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
      
      /* Set ADC error code to ADC IP internal error */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
      
      return HAL_ERROR;
    }
    
    /* Enable the ADC peripheral */
    __HAL_ADC_ENABLE(hadc);
    
    /* Delay for ADC stabilization time. */
    ADC_DelayMicroSecond(ADC_STAB_DELAY_US);

    /* Get tick count */
    tickstart = HAL_GetTick();  
    
    /* Wait for ADC effectively enabled */
      while(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_RDY) == RESET)
      {
          if((HAL_GetTick() - tickstart ) > ADC_ENABLE_TIMEOUT)
          {
            /* Update ADC state machine to error */
            SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
            
            /* Set ADC error code to ADC IP internal error */
            SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
            
            return HAL_ERROR;
          }
        }
    
  }
   
  /* Return HAL status */
  return HAL_OK;
}

/**
  * @brief  Disable the selected ADC.
  * @note   Prerequisite condition to use this function: ADC conversions must be
  *         stopped.
  * @param  hadc: ADC handle
  * @retval HAL status.
  */
static HAL_StatusTypeDef ADC_Disable(ADC_HandleTypeDef* hadc)
{
  uint32_t tickstart = 0U;
  
  /* Verification if ADC is not already disabled:                             */
  /* Note: forbidden to disable ADC (set bit ADC_CR_ADDIS) if ADC is already  */
  /* disabled.                                                                */
  if (ADC_IS_ENABLE(hadc) != RESET )
  {
    /* Check if conditions to disable the ADC are fulfilled */
    if (ADC_DISABLING_CONDITIONS(hadc) != RESET)
    {
      /* Disable the ADC peripheral */
      __HAL_ADC_DISABLE(hadc);
    }
    else
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
      
      /* Set ADC error code to ADC IP internal error */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
      
      return HAL_ERROR;
    }
     
    /* Wait for ADC effectively disabled */
    /* Get tick count */
    tickstart = HAL_GetTick();
    
    while(HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADEN))
    {
        if((HAL_GetTick() - tickstart ) > ADC_DISABLE_TIMEOUT)
        {
          /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
          
        /* Set ADC error code to ADC IP internal error */
        SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
          
          return HAL_ERROR;
        }
      }
    }
  
  /* Return HAL status */
  return HAL_OK;
}

/**
  * @brief  Stop ADC conversion.
  * @note   Prerequisite condition to use this function: ADC conversions must be
  *         stopped to disable the ADC.
  * @param  hadc: ADC handle
  * @retval HAL status.
  */
static HAL_StatusTypeDef ADC_ConversionStop(ADC_HandleTypeDef* hadc)
{
  uint32_t tickstart = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
    
  /* Verification if ADC is not already stopped on regular group to bypass    */
  /* this function if not needed.                                             */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc))
  {
    
    /* Stop potential conversion on going on regular group */
    /* Software is allowed to set ADSTP only when ADSTART=1 and ADDIS=0 */
    if (HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADSTART) && 
        HAL_IS_BIT_CLR(hadc->Instance->CR, ADC_CR_ADDIS)                  )
    {
      /* Stop conversions on regular group */
      hadc->Instance->CR |= ADC_CR_ADSTP;
    }
    
    /* Wait for conversion effectively stopped */
    /* Get tick count */
    tickstart = HAL_GetTick();
      
    while((hadc->Instance->CR & ADC_CR_ADSTART) != RESET)
    {
      if((HAL_GetTick() - tickstart) > ADC_STOP_CONVERSION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
        
        /* Set ADC error code to ADC IP internal error */
        SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
        
        return HAL_ERROR;
      }
    }
    
  }
   
  /* Return HAL status */
  return HAL_OK;
}

/**
  * @brief  DMA transfer complete callback. 
  * @param  hdma: pointer to DMA handle.
  * @retval None
  */
static void ADC_DMAConvCplt(DMA_HandleTypeDef *hdma)   
{
  /* Retrieve ADC handle corresponding to current DMA handle */
    ADC_HandleTypeDef* hadc = ( ADC_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  /* Update state machine on conversion status if not in error state */
  if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL | HAL_ADC_STATE_ERROR_DMA))
  {
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC); 
    
    /* Determine whether any further conversion upcoming on group regular     */
    /* by external trigger, continuous mode or scan sequence on going.        */
    if(ADC_IS_SOFTWARE_START_REGULAR(hadc)        && 
       (hadc->Init.ContinuousConvMode == DISABLE)   )
    {
      /* If End of Sequence is reached, disable interrupts */
      if( __HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOS) )
      {
        /* Allowed to modify bits ADC_IT_EOC/ADC_IT_EOS only if bit           */
        /* ADSTART==0 (no conversion on going)                                */
        if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
        {
          /* Disable ADC end of single conversion interrupt on group regular */
          /* Note: Overrun interrupt was enabled with EOC interrupt in        */
          /* HAL_Start_IT(), but is not disabled here because can be used     */
          /* by overrun IRQ process below.                                    */
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC | ADC_IT_EOS);
          
          /* Set ADC state */
          ADC_STATE_CLR_SET(hadc->State,
                            HAL_ADC_STATE_REG_BUSY,
                            HAL_ADC_STATE_READY);
        }
        else
        {
          /* Change ADC state to error state */
          SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);
          
          /* Set ADC error code to ADC IP internal error */
          SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
        }
      }
    }
    
    /* Conversion complete callback */
    HAL_ADC_ConvCpltCallback(hadc); 
}
  else
  {
    /* Call DMA error callback */
    hadc->DMA_Handle->XferErrorCallback(hdma);
  }

}

/**
  * @brief  DMA half transfer complete callback. 
  * @param  hdma: pointer to DMA handle.
  * @retval None
  */
static void ADC_DMAHalfConvCplt(DMA_HandleTypeDef *hdma)   
{
  /* Retrieve ADC handle corresponding to current DMA handle */
    ADC_HandleTypeDef* hadc = ( ADC_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;
    
  /* Half conversion callback */
    HAL_ADC_ConvHalfCpltCallback(hadc); 
}

/**
  * @brief  DMA error callback 
  * @param  hdma: pointer to DMA handle.
  * @retval None
  */
static void ADC_DMAError(DMA_HandleTypeDef *hdma)   
{
  /* Retrieve ADC handle corresponding to current DMA handle */
  ADC_HandleTypeDef* hadc = ( ADC_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  /* Set ADC state */
  SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);
  
    /* Set ADC error code to DMA error */
  SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_DMA);
  
  /* Error callback */
  HAL_ADC_ErrorCallback(hadc); 
}

/**
  * @brief  Delay micro seconds 
  * @param  microSecond : delay
  * @retval None
  */
static void ADC_DelayMicroSecond(uint32_t microSecond)
{
  /* Compute number of CPU cycles to wait for */
  __IO uint32_t waitLoopIndex = (microSecond * (SystemCoreClock / 1000000U));

  while(waitLoopIndex != 0U)
  {
    waitLoopIndex--;
  } 
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
