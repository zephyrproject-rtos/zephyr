/**
  ******************************************************************************
  * @file    stm32f3xx_hal_adc_ex.c
  * @author  MCD Application Team
  * @brief   This file provides firmware functions to manage the following
  *          functionalities of the Analog to Digital Convertor (ADC)
  *          peripheral:
  *           + Operation functions
  *             ++ Start, stop, get result of conversions of injected
  *                group, using 2 possible modes: polling, interruption.
  *             ++ Multimode feature (available on devices with 2 ADCs or more)
  *             ++ Calibration (ADC automatic self-calibration)
  *           + Control functions
  *             ++ Channels configuration on injected group
  *          Other functions (generic functions) are available in file
  *          "stm32f3xx_hal_adc.c".
  *
  @verbatim
  [..]
  (@) Sections "ADC peripheral features" and "How to use this driver" are
      available in file of generic functions "stm32f3xx_hal_adc.c".
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

/** @defgroup ADCEx ADCEx
  * @brief ADC Extended HAL module driver
  * @{
  */

#ifdef HAL_ADC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup ADCEx_Private_Constants ADCEx Private Constants
  * @{
  */
#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
  /* Fixed timeout values for ADC calibration, enable settling time, disable  */
  /* settling time.                                                           */
  /* Values defined to be higher than worst cases: low clock frequency,       */
  /* maximum prescalers.                                                      */
  /* Ex of profile low frequency : Clock source at 0.5 MHz, ADC clock         */
  /* prescaler 256 (devices STM32F30xx), sampling time 7.5 ADC clock cycles,  */
  /* resolution 12 bits.                                                      */
  /* Unit: ms                                                                 */
  #define ADC_CALIBRATION_TIMEOUT         ( 10U)
  #define ADC_ENABLE_TIMEOUT              (  2U)
  #define ADC_DISABLE_TIMEOUT             (  2U)
  #define ADC_STOP_CONVERSION_TIMEOUT     ( 11U)

  /* Timeout to wait for current conversion on going to be completed.         */
  /* Timeout fixed to worst case, for 1 channel.                              */
  /*   - maximum sampling time (601.5 adc_clk)                                */
  /*   - ADC resolution (Tsar 12 bits= 12.5 adc_clk)                          */
  /*   - ADC clock (from PLL with prescaler 256 (devices STM32F30xx))         */
  /* Unit: cycles of CPU clock.                                               */
  #define ADC_CONVERSION_TIME_MAX_CPU_CYCLES ( 156928U)

  /* Delay for ADC stabilization time (ADC voltage regulator start-up time)   */
  /* Maximum delay is 10us (refer to device datasheet, param. TADCVREG_STUP). */
  /* Unit: us                                                                 */
  #define ADC_STAB_DELAY_US               ( 10U)

  /* Delay for temperature sensor stabilization time.                         */
  /* Maximum delay is 10us (refer device datasheet, parameter tSTART).        */
  /* Unit: us                                                                 */
  #define ADC_TEMPSENSOR_DELAY_US         ( 10U)

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
  /* Timeout values for ADC enable and disable settling time.                 */
  /* Values defined to be higher than worst cases: low clocks freq,           */
  /* maximum prescaler.                                                       */
  /* Ex of profile low frequency : Clock source at 0.1 MHz, ADC clock         */
  /* prescaler 4U, sampling time 12.5 ADC clock cycles, resolution 12 bits.    */
  /* Unit: ms                                                                 */
  #define ADC_ENABLE_TIMEOUT              ( 2U)
  #define ADC_DISABLE_TIMEOUT             ( 2U)

  /* Delay for ADC calibration:                                               */
  /* Hardware prerequisite before starting a calibration: the ADC must have   */
  /* been in power-on state for at least two ADC clock cycles.                */
  /* Unit: ADC clock cycles                                                   */
  #define ADC_PRECALIBRATION_DELAY_ADCCLOCKCYCLES       ( 2U)

  /* Timeout value for ADC calibration                                        */
  /* Value defined to be higher than worst cases: low clocks freq,            */
  /* maximum prescaler.                                                       */
  /* Ex of profile low frequency : Clock source at 0.1 MHz, ADC clock         */
  /* prescaler 4U, sampling time 12.5 ADC clock cycles, resolution 12 bits.    */
  /* Unit: ms                                                                 */
  #define ADC_CALIBRATION_TIMEOUT         ( 10U)

  /* Delay for ADC stabilization time.                                        */
  /* Maximum delay is 1us (refer to device datasheet, parameter tSTAB).       */
  /* Unit: us                                                                 */
  #define ADC_STAB_DELAY_US               ( 1U)

  /* Delay for temperature sensor stabilization time.                         */
  /* Maximum delay is 10us (refer to device datasheet, parameter tSTART).     */
  /* Unit: us                                                                 */
  #define ADC_TEMPSENSOR_DELAY_US         ( 10U)

  /* Maximum number of CPU cycles corresponding to 1 ADC cycle                */
  /* Value fixed to worst case: clock prescalers slowing down ADC clock to    */
  /* minimum frequency                                                        */
  /*   - AHB prescaler: 16                                                    */
  /*   - ADC prescaler: 8                                                     */
  /* Unit: cycles of CPU clock.                                               */
  #define ADC_CYCLE_WORST_CASE_CPU_CYCLES ( 128U)

  /* ADC conversion cycles (unit: ADC clock cycles)                           */
  /* (selected sampling time + conversion time of 12.5 ADC clock cycles, with */
  /* resolution 12 bits)                                                      */
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_1CYCLE5    ( 14U)
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_7CYCLES5   ( 20U)
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_13CYCLES5  ( 26U)
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_28CYCLES5  ( 41U)
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_41CYCLES5  ( 54U)
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_55CYCLES5  ( 68U)
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_71CYCLES5  ( 84U)
  #define ADC_CONVERSIONCLOCKCYCLES_SAMPLETIME_239CYCLES5 (252U)
#endif /* STM32F373xC || STM32F378xx */
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
static HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc);
static HAL_StatusTypeDef ADC_Disable(ADC_HandleTypeDef* hadc);
static HAL_StatusTypeDef ADC_ConversionStop(ADC_HandleTypeDef* hadc, uint32_t ConversionGroup);
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
static HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc);
static HAL_StatusTypeDef ADC_ConversionStop_Disable(ADC_HandleTypeDef* hadc);
#endif /* STM32F373xC || STM32F378xx */

static void ADC_DMAConvCplt(DMA_HandleTypeDef *hdma);
static void ADC_DMAHalfConvCplt(DMA_HandleTypeDef *hdma);
static void ADC_DMAError(DMA_HandleTypeDef *hdma);

/* Exported functions --------------------------------------------------------*/

/** @defgroup ADCEx_Exported_Functions ADCEx Exported Functions
  * @{
  */

/** @defgroup ADCEx_Exported_Functions_Group1 ADCEx Initialization and de-initialization functions
  * @brief    ADC Extended Initialization and Configuration functions
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

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Initializes the ADC peripheral and regular group according to
  *         parameters specified in structure "ADC_InitTypeDef".
  * @note   As prerequisite, ADC clock must be configured at RCC top level
  *         depending on possible clock sources: AHB clock or PLL clock.
  *         See commented example code below that can be copied and uncommented
  *         into HAL_ADC_MspInit().
  * @note   Possibility to update parameters on the fly:
  *         This function initializes the ADC MSP (HAL_ADC_MspInit()) only when
  *         coming from ADC state reset. Following calls to this function can
  *         be used to reconfigure some parameters of ADC_InitTypeDef
  *         structure on the fly, without modifying MSP configuration. If ADC
  *         MSP has to be modified again, HAL_ADC_DeInit() must be called
  *         before HAL_ADC_Init().
  *         The setting of these parameters is conditioned by ADC state.
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
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  ADC_Common_TypeDef *tmpADC_Common;
  ADC_HandleTypeDef tmphadcSharingSameCommonRegister;
  uint32_t tmpCFGR = 0U;
  __IO uint32_t wait_loop_index = 0U;

  /* Check ADC handle */
  if(hadc == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_CLOCKPRESCALER(hadc->Init.ClockPrescaler));
  assert_param(IS_ADC_RESOLUTION(hadc->Init.Resolution));
  assert_param(IS_ADC_DATA_ALIGN(hadc->Init.DataAlign));
  assert_param(IS_ADC_SCAN_MODE(hadc->Init.ScanConvMode));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  assert_param(IS_ADC_EXTTRIG_EDGE(hadc->Init.ExternalTrigConvEdge));
  assert_param(IS_ADC_EXTTRIG(hadc->Init.ExternalTrigConv));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DMAContinuousRequests));
  assert_param(IS_ADC_EOC_SELECTION(hadc->Init.EOCSelection));
  assert_param(IS_ADC_OVERRUN(hadc->Init.Overrun));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.LowPowerAutoWait));

  if(hadc->Init.ScanConvMode != ADC_SCAN_DISABLE)
  {
    assert_param(IS_ADC_REGULAR_NB_CONV(hadc->Init.NbrOfConversion));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DiscontinuousConvMode));
    if(hadc->Init.DiscontinuousConvMode != DISABLE)
    {
      assert_param(IS_ADC_REGULAR_DISCONT_NUMBER(hadc->Init.NbrOfDiscConversion));
    }
  }

  /* Configuration of ADC core parameters and ADC MSP related parameters */
  if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL))
  {
    /* As prerequisite, into HAL_ADC_MspInit(), ADC clock must be configured  */
    /* at RCC top level.                                                      */
    /* Refer to header of this file for more details on clock enabling        */
    /* procedure.                                                             */

    /* Actions performed only if ADC is coming from state reset:              */
    /* - Initialization of ADC MSP                                            */
    /* - ADC voltage regulator enable                                         */
    if (hadc->State == HAL_ADC_STATE_RESET)
    {
      /* Initialize ADC error code */
      ADC_CLEAR_ERRORCODE(hadc);

      /* Initialize HAL ADC API internal variables */
      hadc->InjectionConfig.ChannelCount = 0U;
      hadc->InjectionConfig.ContextQueue = 0U;

      /* Allocate lock resource and initialize it */
      hadc->Lock = HAL_UNLOCKED;

      /* Init the low level hardware */
      HAL_ADC_MspInit(hadc);

      /* Enable voltage regulator (if disabled at this step) */
      if (HAL_IS_BIT_CLR(hadc->Instance->CR, ADC_CR_ADVREGEN_0))
      {
        /* Note: The software must wait for the startup time of the ADC       */
        /*       voltage regulator before launching a calibration or          */
        /*       enabling the ADC. This temporization must be implemented by  */
        /*       software and is equal to 10 us in the worst case             */
        /*       process/temperature/power supply.                            */

        /* Disable the ADC (if not already disabled) */
        tmp_hal_status = ADC_Disable(hadc);

        /* Check if ADC is effectively disabled */
        /* Configuration of ADC parameters if previous preliminary actions    */
        /* are correctly completed.                                           */
        if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL) &&
            (tmp_hal_status == HAL_OK)                                  )
        {
          /* Set ADC state */
          ADC_STATE_CLR_SET(hadc->State,
                            HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                            HAL_ADC_STATE_BUSY_INTERNAL);

          /* Set the intermediate state before moving the ADC voltage         */
          /* regulator to state enable.                                       */
          CLEAR_BIT(hadc->Instance->CR, (ADC_CR_ADVREGEN_1 | ADC_CR_ADVREGEN_0));
          /* Set ADVREGEN bits to 0x01U */
          SET_BIT(hadc->Instance->CR, ADC_CR_ADVREGEN_0);

          /* Delay for ADC stabilization time.                                */
          /* Compute number of CPU cycles to wait for */
          wait_loop_index = (ADC_STAB_DELAY_US * (SystemCoreClock / 1000000U));
          while(wait_loop_index != 0U)
          {
            wait_loop_index--;
          }
        }
      }
    }

    /* Verification that ADC voltage regulator is correctly enabled, whether  */
    /* or not ADC is coming from state reset (if any potential problem of     */
    /* clocking, voltage regulator would not be enabled).                     */
    if (HAL_IS_BIT_CLR(hadc->Instance->CR, ADC_CR_ADVREGEN_0) ||
        HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADVREGEN_1)   )
    {
      /* Update ADC state machine to error */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_BUSY_INTERNAL,
                        HAL_ADC_STATE_ERROR_INTERNAL);

      /* Set ADC error code to ADC IP internal error */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);

      tmp_hal_status = HAL_ERROR;
    }
  }


  /* Configuration of ADC parameters if previous preliminary actions are      */
  /* correctly completed and if there is no conversion on going on regular    */
  /* group (ADC may already be enabled at this point if HAL_ADC_Init() is     */
  /* called to update a parameter on the fly).                                */
  if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL) &&
      (tmp_hal_status == HAL_OK)                                &&
      (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)          )
  {
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_REG_BUSY,
                      HAL_ADC_STATE_BUSY_INTERNAL);

    /* Configuration of common ADC parameters                                 */

    /* Pointer to the common control register to which is belonging hadc      */
    /* (Depending on STM32F3 product, there may be up to 4 ADC and 2 common   */
    /* control registers)                                                     */
    tmpADC_Common = ADC_COMMON_REGISTER(hadc);

    /* Set handle of the other ADC sharing the same common register           */
    ADC_COMMON_ADC_OTHER(hadc, &tmphadcSharingSameCommonRegister);


    /* Parameters update conditioned to ADC state:                            */
    /* Parameters that can be updated only when ADC is disabled:              */
    /*  - Multimode clock configuration                                       */
    if ((ADC_IS_ENABLE(hadc) == RESET)                                   &&
        ((tmphadcSharingSameCommonRegister.Instance == NULL)         ||
         (ADC_IS_ENABLE(&tmphadcSharingSameCommonRegister) == RESET)   )   )
    {
      /* Reset configuration of ADC common register CCR:                      */
      /*   - ADC clock mode: CKMODE                                           */
      /* Some parameters of this register are not reset, since they are set   */
      /* by other functions and must be kept in case of usage of this         */
      /* function on the fly (update of a parameter of ADC_InitTypeDef        */
      /* without needing to reconfigure all other ADC groups/channels         */
      /* parameters):                                                         */
      /*   - multimode related parameters: MDMA, DMACFG, DELAY, MULTI (set    */
      /*     into HAL_ADCEx_MultiModeConfigChannel() )                        */
      /*   - internal measurement paths: Vbat, temperature sensor, Vref       */
      /*     (set into HAL_ADC_ConfigChannel() or                             */
      /*     HAL_ADCEx_InjectedConfigChannel() )                              */

      MODIFY_REG(tmpADC_Common->CCR       ,
                 ADC_CCR_CKMODE           ,
                 hadc->Init.ClockPrescaler );
    }


    /* Configuration of ADC:                                                  */
    /*  - resolution                                                          */
    /*  - data alignment                                                      */
    /*  - external trigger to start conversion                                */
    /*  - external trigger polarity                                           */
    /*  - continuous conversion mode                                          */
    /*  - overrun                                                             */
    /*  - discontinuous mode                                                  */
    SET_BIT(tmpCFGR, ADC_CFGR_CONTINUOUS(hadc->Init.ContinuousConvMode) |
                     ADC_CFGR_OVERRUN(hadc->Init.Overrun)               |
                     hadc->Init.DataAlign                               |
                     hadc->Init.Resolution                               );

    /* Enable discontinuous mode only if continuous mode is disabled */
    if (hadc->Init.DiscontinuousConvMode == ENABLE)
    {
      if (hadc->Init.ContinuousConvMode == DISABLE)
      {
        /* Enable the selected ADC regular discontinuous mode */
        /* Set the number of channels to be converted in discontinuous mode */
        SET_BIT(tmpCFGR, ADC_CFGR_DISCEN                                            |
                         ADC_CFGR_DISCONTINUOUS_NUM(hadc->Init.NbrOfDiscConversion)  );
      }
      else
      {
        /* ADC regular group discontinuous was intended to be enabled,        */
        /* but ADC regular group modes continuous and sequencer discontinuous */
        /* cannot be enabled simultaneously.                                  */

        /* Update ADC state machine to error */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_BUSY_INTERNAL,
                          HAL_ADC_STATE_ERROR_CONFIG);

        /* Set ADC error code to ADC IP internal error */
        SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
      }
    }

    /* Enable external trigger if trigger selection is different of software  */
    /* start.                                                                 */
    /* Note: This configuration keeps the hardware feature of parameter       */
    /*       ExternalTrigConvEdge "trigger edge none" equivalent to           */
    /*       software start.                                                  */
    if (hadc->Init.ExternalTrigConv != ADC_SOFTWARE_START)
    {
      SET_BIT(tmpCFGR, ADC_CFGR_EXTSEL_SET(hadc, hadc->Init.ExternalTrigConv) |
                       hadc->Init.ExternalTrigConvEdge                         );
    }

    /* Parameters update conditioned to ADC state:                            */
    /* Parameters that can be updated when ADC is disabled or enabled without */
    /* conversion on going on regular and injected groups:                    */
    /*  - DMA continuous request                                              */
    /*  - LowPowerAutoWait feature                                            */
    if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)
    {
      CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_AUTDLY |
                                      ADC_CFGR_DMACFG  );

      SET_BIT(tmpCFGR, ADC_CFGR_AUTOWAIT(hadc->Init.LowPowerAutoWait)       |
                       ADC_CFGR_DMACONTREQ(hadc->Init.DMAContinuousRequests) );
    }

    /* Update ADC configuration register with previous settings */
    MODIFY_REG(hadc->Instance->CFGR,
               ADC_CFGR_DISCNUM |
               ADC_CFGR_DISCEN  |
               ADC_CFGR_CONT    |
               ADC_CFGR_OVRMOD  |
               ADC_CFGR_EXTSEL  |
               ADC_CFGR_EXTEN   |
               ADC_CFGR_ALIGN   |
               ADC_CFGR_RES        ,
               tmpCFGR              );


    /* Configuration of regular group sequencer:                              */
    /* - if scan mode is disabled, regular channels sequence length is set to */
    /*   0x00: 1 channel converted (channel on regular rank 1U)                */
    /*   Parameter "NbrOfConversion" is discarded.                            */
    /*   Note: Scan mode is not present by hardware on this device, but       */
    /*   emulated by software for alignment over all STM32 devices.           */
    /* - if scan mode is enabled, regular channels sequence length is set to  */
    /*   parameter "NbrOfConversion"                                          */
    if (hadc->Init.ScanConvMode == ADC_SCAN_ENABLE)
    {
      /* Set number of ranks in regular group sequencer */
      MODIFY_REG(hadc->Instance->SQR1                     ,
                 ADC_SQR1_L                               ,
                 (hadc->Init.NbrOfConversion - (uint8_t)1U) );
    }
    else
    {
      CLEAR_BIT(hadc->Instance->SQR1, ADC_SQR1_L);
    }

    /* Set ADC error code to none */
    ADC_CLEAR_ERRORCODE(hadc);

    /* Set the ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_BUSY_INTERNAL,
                      HAL_ADC_STATE_READY);
  }
  else
  {
    /* Update ADC state machine to error */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_BUSY_INTERNAL,
                      HAL_ADC_STATE_ERROR_INTERNAL);

    tmp_hal_status = HAL_ERROR;
  }


  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Initializes the ADC peripheral and regular group according to
  *         parameters specified in structure "ADC_InitTypeDef".
  * @note   As prerequisite, ADC clock must be configured at RCC top level
  *         (clock source APB2).
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
  * @param  hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tmp_cr1 = 0U;
  uint32_t tmp_cr2 = 0U;
  uint32_t tmp_sqr1 = 0U;

  /* Check ADC handle */
  if(hadc == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_DATA_ALIGN(hadc->Init.DataAlign));
  assert_param(IS_ADC_SCAN_MODE(hadc->Init.ScanConvMode));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  assert_param(IS_ADC_EXTTRIG(hadc->Init.ExternalTrigConv));

  if(hadc->Init.ScanConvMode != ADC_SCAN_DISABLE)
  {
    assert_param(IS_ADC_REGULAR_NB_CONV(hadc->Init.NbrOfConversion));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DiscontinuousConvMode));
    if(hadc->Init.DiscontinuousConvMode != DISABLE)
    {
      assert_param(IS_ADC_REGULAR_DISCONT_NUMBER(hadc->Init.NbrOfDiscConversion));
    }
  }

  /* As prerequisite, into HAL_ADC_MspInit(), ADC clock must be configured    */
  /* at RCC top level.                                                        */
  /* Refer to header of this file for more details on clock enabling          */
  /* procedure.                                                               */

  /* Actions performed only if ADC is coming from state reset:                */
  /* - Initialization of ADC MSP                                              */
  if (hadc->State == HAL_ADC_STATE_RESET)
  {
    /* Initialize ADC error code */
    ADC_CLEAR_ERRORCODE(hadc);

    /* Allocate lock resource and initialize it */
    hadc->Lock = HAL_UNLOCKED;

    /* Init the low level hardware */
    HAL_ADC_MspInit(hadc);
  }

  /* Stop potential conversion on going, on regular and injected groups */
  /* Disable ADC peripheral */
  /* Note: In case of ADC already enabled, precaution to not launch an        */
  /*       unwanted conversion while modifying register CR2 by writing 1 to   */
  /*       bit ADON.                                                          */
  tmp_hal_status = ADC_ConversionStop_Disable(hadc);


  /* Configuration of ADC parameters if previous preliminary actions are      */
  /* correctly completed.                                                     */
  if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL) &&
      (tmp_hal_status == HAL_OK)                                  )
  {
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                      HAL_ADC_STATE_BUSY_INTERNAL);

    /* Set ADC parameters */

    /* Configuration of ADC:                                                  */
    /*  - data alignment                                                      */
    /*  - external trigger to start conversion                                */
    /*  - external trigger polarity (always set to 1U, because needed for all  */
    /*    triggers: external trigger of SW start)                             */
    /*  - continuous conversion mode                                          */
    /* Note: External trigger polarity (ADC_CR2_EXTTRIG) is set into          */
    /*       HAL_ADC_Start_xxx functions because if set in this function,     */
    /*       a conversion on injected group would start a conversion also on  */
    /*       regular group after ADC enabling.                                */
    tmp_cr2 |= (hadc->Init.DataAlign                             |
                hadc->Init.ExternalTrigConv                      |
                ADC_CR2_CONTINUOUS(hadc->Init.ContinuousConvMode) );

    /* Configuration of ADC:                                                  */
    /*  - scan mode                                                           */
    /*  - discontinuous mode disable/enable                                   */
    /*  - discontinuous mode number of conversions                            */
    tmp_cr1 |= (ADC_CR1_SCAN_SET(hadc->Init.ScanConvMode));

    /* Enable discontinuous mode only if continuous mode is disabled */
    /* Note: If parameter "Init.ScanConvMode" is set to disable, parameter    */
    /*       discontinuous is set anyway, but will have no effect on ADC HW.  */
    if (hadc->Init.DiscontinuousConvMode == ENABLE)
    {
      if (hadc->Init.ContinuousConvMode == DISABLE)
      {
        /* Enable the selected ADC regular discontinuous mode */
        /* Set the number of channels to be converted in discontinuous mode */
      tmp_cr1 |= (ADC_CR1_DISCEN                                           |
                  ADC_CR1_DISCONTINUOUS_NUM(hadc->Init.NbrOfDiscConversion) );
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

    /* Update ADC configuration register CR1 with previous settings */
      MODIFY_REG(hadc->Instance->CR1,
                 ADC_CR1_SCAN    |
                 ADC_CR1_DISCEN  |
                 ADC_CR1_DISCNUM    ,
                 tmp_cr1             );

    /* Update ADC configuration register CR2 with previous settings */
      MODIFY_REG(hadc->Instance->CR2,
                 ADC_CR2_ALIGN   |
                 ADC_CR2_EXTSEL  |
                 ADC_CR2_EXTTRIG |
                 ADC_CR2_CONT       ,
                 tmp_cr2             );

    /* Configuration of regular group sequencer:                              */
    /* - if scan mode is disabled, regular channels sequence length is set to */
    /*   0x00: 1 channel converted (channel on regular rank 1U)                */
    /*   Parameter "NbrOfConversion" is discarded.                            */
    /*   Note: Scan mode is present by hardware on this device and, if        */
    /*   disabled, discards automatically nb of conversions. Anyway, nb of    */
    /*   conversions is forced to 0x00 for alignment over all STM32 devices.  */
    /* - if scan mode is enabled, regular channels sequence length is set to  */
    /*   parameter "NbrOfConversion"                                          */
    if (ADC_CR1_SCAN_SET(hadc->Init.ScanConvMode) == ADC_SCAN_ENABLE)
    {
      tmp_sqr1 = ADC_SQR1_L_SHIFT(hadc->Init.NbrOfConversion);
    }

    MODIFY_REG(hadc->Instance->SQR1,
               ADC_SQR1_L          ,
               tmp_sqr1             );

    /* Check back that ADC registers have effectively been configured to      */
    /* ensure of no potential problem of ADC core IP clocking.                */
    /* Check through register CR2 (excluding bits set in other functions:     */
    /* execution control bits (ADON, JSWSTART, SWSTART), regular group bits   */
    /* (DMA), injected group bits (JEXTTRIG and JEXTSEL), channel internal    */
    /* measurement path bit (TSVREFE).                                        */
    if (READ_BIT(hadc->Instance->CR2, ~(ADC_CR2_ADON | ADC_CR2_DMA |
                                        ADC_CR2_SWSTART | ADC_CR2_JSWSTART |
                                        ADC_CR2_JEXTTRIG | ADC_CR2_JEXTSEL |
                                        ADC_CR2_TSVREFE                     ))
         == tmp_cr2)
    {
      /* Set ADC error code to none */
      ADC_CLEAR_ERRORCODE(hadc);

      /* Set the ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_BUSY_INTERNAL,
                        HAL_ADC_STATE_READY);
    }
    else
    {
      /* Update ADC state machine to error */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_BUSY_INTERNAL,
                        HAL_ADC_STATE_ERROR_INTERNAL);

      /* Set ADC error code to ADC IP internal error */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);

      tmp_hal_status = HAL_ERROR;
    }

  }
  else
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);

    tmp_hal_status = HAL_ERROR;
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
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
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  ADC_Common_TypeDef *tmpADC_Common;
  ADC_HandleTypeDef tmphadcSharingSameCommonRegister;

  /* Check ADC handle */
  if(hadc == NULL)
  {
     return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Set ADC state */
  SET_BIT(hadc->State, HAL_ADC_STATE_BUSY_INTERNAL);

  /* Stop potential conversion on going, on regular and injected groups */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_INJECTED_GROUP);

  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {
    /* Flush register JSQR: queue sequencer reset when injected queue         */
    /* sequencer is enabled and ADC disabled.                                 */
    /* Enable injected queue sequencer after injected conversion stop         */
    SET_BIT(hadc->Instance->CFGR, ADC_CFGR_JQM);

    /* Disable the ADC peripheral */
    tmp_hal_status = ADC_Disable(hadc);

    /* Check if ADC is effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Change ADC state */
      hadc->State = HAL_ADC_STATE_READY;
    }
    else
    {
      tmp_hal_status = HAL_ERROR;
    }
  }


  /* Configuration of ADC parameters if previous preliminary actions are      */
  /* correctly completed.                                                     */
  if (tmp_hal_status == HAL_OK)
  {
    /* ========== Reset ADC registers ========== */
    /* Reset register IER */
    __HAL_ADC_DISABLE_IT(hadc, (ADC_IT_AWD3  | ADC_IT_AWD2 | ADC_IT_AWD1 |
                                ADC_IT_JQOVF | ADC_IT_OVR  |
                                ADC_IT_JEOS  | ADC_IT_JEOC |
                                ADC_IT_EOS   | ADC_IT_EOC  |
                                ADC_IT_EOSMP | ADC_IT_RDY                 ) );

    /* Reset register ISR */
    __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_AWD3  | ADC_FLAG_AWD2 | ADC_FLAG_AWD1 |
                                ADC_FLAG_JQOVF | ADC_FLAG_OVR  |
                                ADC_FLAG_JEOS  | ADC_FLAG_JEOC |
                                ADC_FLAG_EOS   | ADC_FLAG_EOC  |
                                ADC_FLAG_EOSMP | ADC_FLAG_RDY                   ) );

    /* Reset register CR */
    /* Bits ADC_CR_JADSTP, ADC_CR_ADSTP, ADC_CR_JADSTART, ADC_CR_ADSTART are  */
    /* in access mode "read-set": no direct reset applicable.                 */
    /* Reset Calibration mode to default setting (single ended):              */
    /* Disable voltage regulator:                                             */
    /* Note: Voltage regulator disable is conditioned to ADC state disabled:  */
    /*       already done above.                                              */
    /* Note: Voltage regulator disable is intended for power saving.          */
    /* Sequence to disable voltage regulator:                                 */
    /* 1. Set the intermediate state before moving the ADC voltage regulator  */
    /*    to disable state.                                                   */
    CLEAR_BIT(hadc->Instance->CR, ADC_CR_ADVREGEN_1 | ADC_CR_ADVREGEN_0 | ADC_CR_ADCALDIF);
    /* 2. Set ADVREGEN bits to 0x10U */
    SET_BIT(hadc->Instance->CR, ADC_CR_ADVREGEN_1);

    /* Reset register CFGR */
    CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_AWD1CH  | ADC_CFGR_JAUTO   | ADC_CFGR_JAWD1EN |
                                    ADC_CFGR_AWD1EN  | ADC_CFGR_AWD1SGL | ADC_CFGR_JQM     |
                                    ADC_CFGR_JDISCEN | ADC_CFGR_DISCNUM | ADC_CFGR_DISCEN  |
                                    ADC_CFGR_AUTDLY  | ADC_CFGR_CONT    | ADC_CFGR_OVRMOD  |
                                    ADC_CFGR_EXTEN   | ADC_CFGR_EXTSEL  | ADC_CFGR_ALIGN   |
                                    ADC_CFGR_RES     | ADC_CFGR_DMACFG  | ADC_CFGR_DMAEN    );

    /* Reset register SMPR1 */
    CLEAR_BIT(hadc->Instance->SMPR1, ADC_SMPR1_SMP9 | ADC_SMPR1_SMP8 | ADC_SMPR1_SMP7 |
                                     ADC_SMPR1_SMP6 | ADC_SMPR1_SMP5 | ADC_SMPR1_SMP4 |
                                     ADC_SMPR1_SMP3 | ADC_SMPR1_SMP2 | ADC_SMPR1_SMP1  );

    /* Reset register SMPR2 */
    CLEAR_BIT(hadc->Instance->SMPR2, ADC_SMPR2_SMP18 | ADC_SMPR2_SMP17 | ADC_SMPR2_SMP16 |
                                     ADC_SMPR2_SMP15 | ADC_SMPR2_SMP14 | ADC_SMPR2_SMP13 |
                                     ADC_SMPR2_SMP12 | ADC_SMPR2_SMP11 | ADC_SMPR2_SMP10  );

    /* Reset register TR1 */
    CLEAR_BIT(hadc->Instance->TR1, ADC_TR1_HT1 | ADC_TR1_LT1);

    /* Reset register TR2 */
    CLEAR_BIT(hadc->Instance->TR2, ADC_TR2_HT2 | ADC_TR2_LT2);

    /* Reset register TR3 */
    CLEAR_BIT(hadc->Instance->TR3, ADC_TR3_HT3 | ADC_TR3_LT3);

    /* Reset register SQR1 */
    CLEAR_BIT(hadc->Instance->SQR1, ADC_SQR1_SQ4 | ADC_SQR1_SQ3 | ADC_SQR1_SQ2 |
                                    ADC_SQR1_SQ1 | ADC_SQR1_L);

    /* Reset register SQR2 */
    CLEAR_BIT(hadc->Instance->SQR2, ADC_SQR2_SQ9 | ADC_SQR2_SQ8 | ADC_SQR2_SQ7 |
                                    ADC_SQR2_SQ6 | ADC_SQR2_SQ5);

    /* Reset register SQR3 */
    CLEAR_BIT(hadc->Instance->SQR3, ADC_SQR3_SQ14 | ADC_SQR3_SQ13 | ADC_SQR3_SQ12 |
                                    ADC_SQR3_SQ11 | ADC_SQR3_SQ10);

    /* Reset register SQR4 */
    CLEAR_BIT(hadc->Instance->SQR4, ADC_SQR4_SQ16 | ADC_SQR4_SQ15);

    /* Reset register DR */
    /* bits in access mode read only, no direct reset applicable*/

    /* Reset register OFR1 */
    CLEAR_BIT(hadc->Instance->OFR1, ADC_OFR1_OFFSET1_EN | ADC_OFR1_OFFSET1_CH | ADC_OFR1_OFFSET1);
    /* Reset register OFR2 */
    CLEAR_BIT(hadc->Instance->OFR2, ADC_OFR2_OFFSET2_EN | ADC_OFR2_OFFSET2_CH | ADC_OFR2_OFFSET2);
    /* Reset register OFR3 */
    CLEAR_BIT(hadc->Instance->OFR3, ADC_OFR3_OFFSET3_EN | ADC_OFR3_OFFSET3_CH | ADC_OFR3_OFFSET3);
    /* Reset register OFR4 */
    CLEAR_BIT(hadc->Instance->OFR4, ADC_OFR4_OFFSET4_EN | ADC_OFR4_OFFSET4_CH | ADC_OFR4_OFFSET4);

    /* Reset registers JDR1, JDR2, JDR3, JDR4 */
    /* bits in access mode read only, no direct reset applicable*/

    /* Reset register AWD2CR */
    CLEAR_BIT(hadc->Instance->AWD2CR, ADC_AWD2CR_AWD2CH);

    /* Reset register AWD3CR */
    CLEAR_BIT(hadc->Instance->AWD3CR, ADC_AWD3CR_AWD3CH);

    /* Reset register DIFSEL */
    CLEAR_BIT(hadc->Instance->DIFSEL, ADC_DIFSEL_DIFSEL);

    /* Reset register CALFACT */
    CLEAR_BIT(hadc->Instance->CALFACT, ADC_CALFACT_CALFACT_D | ADC_CALFACT_CALFACT_S);






    /* ========== Reset common ADC registers ========== */

    /* Pointer to the common control register to which is belonging hadc      */
    /* (Depending on STM32F3 product, there may be up to 4 ADC and 2 common   */
    /* control registers)                                                     */
    tmpADC_Common = ADC_COMMON_REGISTER(hadc);

    /* Set handle of the other ADC sharing the same common register           */
    ADC_COMMON_ADC_OTHER(hadc, &tmphadcSharingSameCommonRegister);

    /* Software is allowed to change common parameters only when all ADCs of  */
    /* the common group are disabled.                                         */
    if ((ADC_IS_ENABLE(hadc) == RESET)                                  &&
        ( (tmphadcSharingSameCommonRegister.Instance == NULL) ||
          (ADC_IS_ENABLE(&tmphadcSharingSameCommonRegister) == RESET) )   )
    {
      /* Reset configuration of ADC common register CCR:
        - clock mode: CKMODE
        - multimode related parameters: MDMA, DMACFG, DELAY, MULTI (set into
          HAL_ADCEx_MultiModeConfigChannel() )
        - internal measurement paths: Vbat, temperature sensor, Vref (set into
          HAL_ADC_ConfigChannel() or HAL_ADCEx_InjectedConfigChannel() )
      */
      CLEAR_BIT(tmpADC_Common->CCR, ADC_CCR_CKMODE |
                                    ADC_CCR_VBATEN |
                                    ADC_CCR_TSEN   |
                                    ADC_CCR_VREFEN |
                                    ADC_CCR_MDMA   |
                                    ADC_CCR_DMACFG |
                                    ADC_CCR_DELAY  |
                                    ADC_CCR_MULTI   );

      /* Other ADC common registers (CSR, CDR) are in access mode read only,
         no direct reset applicable */
    }


    /* ========== Hard reset and clock disable of ADC peripheral ========== */
    /* Into HAL_ADC_MspDeInit(), ADC clock can be hard reset and disabled     */
    /* at RCC top level.                                                      */
    /* Refer to header of this file for more details on clock disabling       */
    /* procedure.                                                             */


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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Deinitialize the ADC peripheral registers to its default reset values.
  * @param  hadc ADC handle
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

  /* Stop potential conversion on going, on regular and injected groups */
  /* Disable ADC peripheral */
  tmp_hal_status = ADC_ConversionStop_Disable(hadc);


  /* Configuration of ADC parameters if previous preliminary actions are      */
  /* correctly completed.                                                     */
  if (tmp_hal_status == HAL_OK)
  {
    /* ========== Reset ADC registers ========== */
    /* Reset register SR */
    __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_AWD | ADC_FLAG_JEOC | ADC_FLAG_EOC |
                                ADC_FLAG_JSTRT | ADC_FLAG_STRT));

    /* Reset register CR1 */
    CLEAR_BIT(hadc->Instance->CR1, (ADC_CR1_AWDEN   | ADC_CR1_JAWDEN | ADC_CR1_DISCNUM |
                                    ADC_CR1_JDISCEN | ADC_CR1_DISCEN | ADC_CR1_JAUTO   |
                                    ADC_CR1_AWDSGL  | ADC_CR1_SCAN   | ADC_CR1_JEOCIE  |
                                    ADC_CR1_AWDIE   | ADC_CR1_EOCIE  | ADC_CR1_AWDCH    ));

    /* Reset register CR2 */
    CLEAR_BIT(hadc->Instance->CR2, (ADC_CR2_TSVREFE | ADC_CR2_SWSTART | ADC_CR2_JSWSTART |
                                    ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL  | ADC_CR2_JEXTTRIG |
                                    ADC_CR2_JEXTSEL | ADC_CR2_ALIGN   | ADC_CR2_DMA      |
                                    ADC_CR2_RSTCAL  | ADC_CR2_CAL     | ADC_CR2_CONT     |
                                    ADC_CR2_ADON                                          ));

    /* Reset register SMPR1 */
    CLEAR_BIT(hadc->Instance->SMPR1, (ADC_SMPR1_SMP18 | ADC_SMPR1_SMP17 | ADC_SMPR1_SMP15 |
                                      ADC_SMPR1_SMP15 | ADC_SMPR1_SMP14 | ADC_SMPR1_SMP13 |
                                      ADC_SMPR1_SMP12 | ADC_SMPR1_SMP11 | ADC_SMPR1_SMP10  ));

    /* Reset register SMPR2 */
    CLEAR_BIT(hadc->Instance->SMPR2, (ADC_SMPR2_SMP9 | ADC_SMPR2_SMP8 | ADC_SMPR2_SMP7 |
                                      ADC_SMPR2_SMP6 | ADC_SMPR2_SMP5 | ADC_SMPR2_SMP4 |
                                      ADC_SMPR2_SMP3 | ADC_SMPR2_SMP2 | ADC_SMPR2_SMP1 |
                                      ADC_SMPR2_SMP0                                    ));

    /* Reset register JOFR1 */
    CLEAR_BIT(hadc->Instance->JOFR1, ADC_JOFR1_JOFFSET1);
    /* Reset register JOFR2 */
    CLEAR_BIT(hadc->Instance->JOFR2, ADC_JOFR2_JOFFSET2);
    /* Reset register JOFR3 */
    CLEAR_BIT(hadc->Instance->JOFR3, ADC_JOFR3_JOFFSET3);
    /* Reset register JOFR4 */
    CLEAR_BIT(hadc->Instance->JOFR4, ADC_JOFR4_JOFFSET4);

    /* Reset register HTR */
    CLEAR_BIT(hadc->Instance->HTR, ADC_HTR_HT);
    /* Reset register LTR */
    CLEAR_BIT(hadc->Instance->LTR, ADC_LTR_LT);

    /* Reset register SQR1 */
    CLEAR_BIT(hadc->Instance->SQR1, ADC_SQR1_L    |
                                    ADC_SQR1_SQ16 | ADC_SQR1_SQ15 |
                                    ADC_SQR1_SQ14 | ADC_SQR1_SQ13  );

    /* Reset register SQR1 */
    CLEAR_BIT(hadc->Instance->SQR1, ADC_SQR1_L    |
                                    ADC_SQR1_SQ16 | ADC_SQR1_SQ15 |
                                    ADC_SQR1_SQ14 | ADC_SQR1_SQ13  );

    /* Reset register SQR2 */
    CLEAR_BIT(hadc->Instance->SQR2, ADC_SQR2_SQ12 | ADC_SQR2_SQ11 | ADC_SQR2_SQ10 |
                                    ADC_SQR2_SQ9  | ADC_SQR2_SQ8  | ADC_SQR2_SQ7   );

    /* Reset register SQR3 */
    CLEAR_BIT(hadc->Instance->SQR3, ADC_SQR3_SQ6 | ADC_SQR3_SQ5 | ADC_SQR3_SQ4 |
                                    ADC_SQR3_SQ3 | ADC_SQR3_SQ2 | ADC_SQR3_SQ1  );

    /* Reset register JSQR */
    CLEAR_BIT(hadc->Instance->JSQR, ADC_JSQR_JL |
                                    ADC_JSQR_JSQ4 | ADC_JSQR_JSQ3 |
                                    ADC_JSQR_JSQ2 | ADC_JSQR_JSQ1  );

    /* Reset register JSQR */
    CLEAR_BIT(hadc->Instance->JSQR, ADC_JSQR_JL |
                                    ADC_JSQR_JSQ4 | ADC_JSQR_JSQ3 |
                                    ADC_JSQR_JSQ2 | ADC_JSQR_JSQ1  );

    /* Reset register DR */
    /* bits in access mode read only, no direct reset applicable*/

    /* Reset registers JDR1, JDR2, JDR3, JDR4 */
    /* bits in access mode read only, no direct reset applicable*/

    /* Reset VBAT measurement path, in case of enabled before by selecting    */
    /* channel ADC_CHANNEL_VBAT. */
    SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_VBAT);


    /* ========== Hard reset ADC peripheral ========== */
    /* Performs a global reset of the entire ADC peripheral: ADC state is     */
    /* forced to a similar state after device power-on.                       */
    /* If needed, copy-paste and uncomment the following reset code into      */
    /* function "void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)":              */
    /*                                                                        */
    /*  __HAL_RCC_ADC1_FORCE_RESET()                                          */
    /*  __HAL_RCC_ADC1_RELEASE_RESET()                                        */

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
#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

/** @defgroup ADCEx_Exported_Functions_Group2 ADCEx Input and Output operation functions
  * @brief    ADC Extended IO operation functions
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

      (+) Start conversion of injected group.
      (+) Stop conversion of injected group.
      (+) Poll for conversion complete on injected group.
      (+) Get result of injected channel conversion.
      (+) Start conversion of injected group and enable interruptions.
      (+) Stop conversion of injected group and disable interruptions.

      (+) Start multimode and enable DMA transfer.
      (+) Stop multimode and disable ADC DMA transfer.
      (+) Get result of multimode conversion.

      (+) Perform the ADC self-calibration for single or differential ending.
      (+) Get calibration factors for single or differential ending.
      (+) Set calibration factors for single or differential ending.

@endverbatim
  * @{
  */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Enables ADC, starts conversion of regular group.
  *         Interruptions enabled in this function: None.
  * @note   Case of multimode enabled (for devices with several ADCs):
  *         if ADC is slave, ADC is enabled only (conversion is not started).
  *         if ADC is master, ADC is enabled and multimode conversion is started.
  * @param  hadc ADC handle
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
    tmp_hal_status = ADC_Enable(hadc);

    /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to regular group conversion results   */
      /* - Set state bitfield related to regular operation                    */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP,
                        HAL_ADC_STATE_REG_BUSY);

      /* Set group injected state (from auto-injection) and multimode state   */
      /* for all cases of multimode: independent mode, multimode ADC master   */
      /* or multimode ADC slave (for devices with several ADCs):              */
      if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
      {
        /* Set ADC state (ADC independent or master) */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);

        /* If conversions on group regular are also triggering group injected,*/
        /* update ADC state.                                                  */
        if (READ_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO) != RESET)
        {
          ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
        }
      }
      else
      {
        /* Set ADC state (ADC slave) */
        SET_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);

        /* If conversions on group regular are also triggering group injected,*/
        /* update ADC state.                                                  */
        if (ADC_MULTIMODE_AUTO_INJECTED(hadc))
        {
          ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
        }
      }

      /* State machine update: Check if an injected conversion is ongoing */
      if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY))
      {
        /* Reset ADC error code fields related to conversions on group regular*/
        CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
      }
      else
      {
        /* Reset ADC all error code fields */
        ADC_CLEAR_ERRORCODE(hadc);
      }

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
      /* Case of multimode enabled (for devices with several ADCs):           */
      /*  - if ADC is slave, ADC is enabled only (conversion is not started). */
      /*  - if ADC is master, ADC is enabled and conversion is started.       */
      if (ADC_NONMULTIMODE_REG_OR_MULTIMODEMASTER(hadc))
      {
        SET_BIT(hadc->Instance->CR, ADC_CR_ADSTART);
      }
    }
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Enables ADC, starts conversion of regular group.
  *         Interruptions enabled in this function: None.
  * @param  hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Enable the ADC peripheral */
  tmp_hal_status = ADC_Enable(hadc);

  /* Start conversion if ADC is effectively enabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state                                                          */
    /* - Clear state bitfield related to regular group conversion results     */
    /* - Set state bitfield related to regular operation                      */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC,
                      HAL_ADC_STATE_REG_BUSY);

    /* Set group injected state (from auto-injection) */
    /* If conversions on group regular are also triggering group injected,    */
    /* update ADC state.                                                      */
    if (READ_BIT(hadc->Instance->CR1, ADC_CR1_JAUTO) != RESET)
    {
      ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
    }

    /* State machine update: Check if an injected conversion is ongoing */
    if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY))
    {
      /* Reset ADC error code fields related to conversions on group regular */
      CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
    }
    else
    {
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
    }

    /* Process unlocked */
    /* Unlock before starting ADC conversions: in case of potential           */
    /* interruption, to let the process to ADC IRQ Handler.                   */
    __HAL_UNLOCK(hadc);

    /* Clear regular group conversion flag and overrun flag */
    /* (To ensure of no unknown state from potential previous ADC operations) */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_EOC);

    /* Enable conversion of regular group.                                    */
    /* If software start has been selected, conversion starts immediately.    */
    /* If external trigger has been selected, conversion will start at next   */
    /* trigger event.                                                         */
    /* Note: Alternate trigger for single conversion could be to force an     */
    /*       additional set of bit ADON "hadc->Instance->CR2 |= ADC_CR2_ADON;"*/
    if (ADC_IS_SOFTWARE_START_REGULAR(hadc))
    {
      /* Start ADC conversion on regular group with SW start */
      SET_BIT(hadc->Instance->CR2, (ADC_CR2_SWSTART | ADC_CR2_EXTTRIG));
    }
    else
    {
      /* Start ADC conversion on regular group with external trigger */
      SET_BIT(hadc->Instance->CR2, ADC_CR2_EXTTRIG);
    }
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Stop ADC conversion of both groups regular and injected,
  *         disable ADC peripheral.
  * @note   ADC peripheral disable is forcing interruption of potential
  *         conversion on injected group. If injected group is under use,
  *         it should be preliminarily stopped using function
  *         @ref HAL_ADCEx_InjectedStop().
  *         To stop ADC conversion only on ADC group regular
  *         while letting ADC group injected conversions running,
  *         use function @ref HAL_ADCEx_RegularStop().
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* 1. Stop potential conversion on going, on regular and injected groups */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_INJECTED_GROUP);

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
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Stop ADC conversion of regular group (and injected channels in
  *         case of auto_injection mode), disable ADC peripheral.
  * @note   ADC peripheral disable is forcing interruption of potential
  *         conversion on injected group. If injected group is under use, it
  *         should be preliminarily stopped using HAL_ADCEx_InjectedStop function.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential conversion on going, on regular and injected groups */
  /* Disable ADC peripheral */
  tmp_hal_status = ADC_ConversionStop_Disable(hadc);

  /* Check if ADC is effectively disabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                      HAL_ADC_STATE_READY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
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
  * @param  hadc ADC handle
  * @param  Timeout Timeout value in millisecond.
  * @note   Depending on init parameter "EOCSelection", flags EOS or EOC is
  *         checked and cleared depending on autodelay status (bit AUTDLY).
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout)
{
  uint32_t tickstart;
  uint32_t tmp_Flag_EOC;
  ADC_Common_TypeDef *tmpADC_Common;
  uint32_t tmp_cfgr     = 0x0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

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

    /* Pointer to the common control register to which is belonging hadc      */
    /* (Depending on STM32F3 product, there may have up to 4 ADC and 2 common */
    /* control registers)                                                     */
    tmpADC_Common = ADC_COMMON_REGISTER(hadc);

    /* Check DMA configuration, depending on MultiMode set or not */
    if (READ_BIT(tmpADC_Common->CCR, ADC_CCR_MULTI) == ADC_MODE_INDEPENDENT)
    {
      if (HAL_IS_BIT_SET(hadc->Instance->CFGR, ADC_CFGR_DMAEN))
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }
    else
    {
      /* MultiMode is enabled, Common Control Register MDMA bits must be checked */
      if (READ_BIT(tmpADC_Common->CCR, ADC_CCR_MDMA) != RESET)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }

    tmp_Flag_EOC = (ADC_FLAG_EOC | ADC_FLAG_EOS);

  }

  /* Get relevant register CFGR in ADC instance of ADC master or slave      */
  /* in function of multimode state (for devices with multimode             */
  /* available).                                                            */
  if(ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
  {
    tmp_cfgr = READ_REG(hadc->Instance->CFGR);
  }
  else
  {
    tmp_cfgr = READ_REG(ADC_MASTER_INSTANCE(hadc)->CFGR);
  }

  /* Get tick count */
  tickstart = HAL_GetTick();

  /* Wait until End of Conversion or End of Sequence flag is raised */
  while(HAL_IS_BIT_CLR(hadc->Instance->ISR, tmp_Flag_EOC))
  {
    /* Check if timeout is disabled (set to infinite wait) */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
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
  if(ADC_IS_SOFTWARE_START_REGULAR(hadc)           &&
     (READ_BIT (tmp_cfgr, ADC_CFGR_CONT) == RESET)   )
  {
    /* If End of Sequence is reached, disable interrupts */
    if( __HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOS) )
    {
      /* Allowed to modify bits ADC_IT_EOC/ADC_IT_EOS only if bit             */
      /* ADSTART==0 (no conversion on going)                                  */
      if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
      {
        /* Set ADC state */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);

        if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_INJ_BUSY))
        {
          SET_BIT(hadc->State, HAL_ADC_STATE_READY);
        }
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
  if (READ_BIT (tmp_cfgr, ADC_CFGR_AUTDLY) == RESET)
  {
    /* Clear regular group conversion flag */
    /* (EOC or EOS depending on HAL ADC initialization parameter) */
    __HAL_ADC_CLEAR_FLAG(hadc, tmp_Flag_EOC);
  }

  /* Return ADC state */
  return HAL_OK;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Wait for regular group conversion to be completed.
  * @note   This function cannot be used in a particular setup: ADC configured
  *         in DMA mode.
  *         In this case, DMA resets the flag EOC and polling cannot be
  *         performed on each conversion.
  * @note   On STM32F37x devices, limitation in case of sequencer enabled
  *         (several ranks selected): polling cannot be done on each
  *         conversion inside the sequence. In this case, polling is replaced by
  *         wait for maximum conversion time.
  * @param  hadc ADC handle
  * @param  Timeout Timeout value in millisecond.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout)
{
  uint32_t tickstart;

  /* Variables for polling in case of scan mode enabled */
  uint32_t Conversion_Timeout_CPU_cycles_max = 0U;
  uint32_t Conversion_Timeout_CPU_cycles = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Verification that ADC configuration is compliant with polling for        */
  /* each conversion:                                                         */
  /* Particular case is ADC configured in DMA mode                            */
  if (HAL_IS_BIT_SET(hadc->Instance->CR2, ADC_CR2_DMA))
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

    /* Process unlocked */
    __HAL_UNLOCK(hadc);

    return HAL_ERROR;
  }

  /* Get tick count */
  tickstart = HAL_GetTick();

  /* Polling for end of conversion: differentiation if single/sequence        */
  /* conversion.                                                              */
  /*  - If single conversion for regular group (Scan mode disabled or enabled */
  /*    with NbrOfConversion =1U), flag EOC is used to determine the           */
  /*    conversion completion.                                                */
  /*  - If sequence conversion for regular group (scan mode enabled and       */
  /*    NbrOfConversion >=2U), flag EOC is set only at the end of the          */
  /*    sequence.                                                             */
  /*    To poll for each conversion, the maximum conversion time is computed  */
  /*    from ADC conversion time (selected sampling time + conversion time of */
  /*    12.5 ADC clock cycles) and APB2/ADC clock prescalers (depending on    */
  /*    settings, conversion time range can be from 28 to 32256 CPU cycles).  */
  /*    As flag EOC is not set after each conversion, no timeout status can   */
  /*    be set.                                                               */
  if (HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_SCAN) &&
      HAL_IS_BIT_CLR(hadc->Instance->SQR1, ADC_SQR1_L)    )
  {
    /* Wait until End of Conversion flag is raised */
    while(HAL_IS_BIT_CLR(hadc->Instance->SR, ADC_FLAG_EOC))
    {
      /* Check if timeout is disabled (set to infinite wait) */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
        {
          /* Update ADC state machine to timeout */
          SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);

          /* Process unlocked */
          __HAL_UNLOCK(hadc);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  else
  {
    /* Replace polling by wait for maximum conversion time */
    /* Calculation of CPU cycles corresponding to ADC conversion cycles.      */
    /* Retrieve ADC clock prescaler and ADC maximum conversion cycles on all  */
    /* channels.                                                              */
    Conversion_Timeout_CPU_cycles_max = ADC_CLOCK_PRESCALER_RANGE() ;
    Conversion_Timeout_CPU_cycles_max *= ADC_CONVCYCLES_MAX_RANGE(hadc);

    /* Poll with maximum conversion time */
    while(Conversion_Timeout_CPU_cycles < Conversion_Timeout_CPU_cycles_max)
    {
      /* Check if timeout is disabled (set to infinite wait) */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U) || ((HAL_GetTick() - tickstart ) > Timeout))
        {
          /* Update ADC state machine to timeout */
          SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);

          /* Process unlocked */
          __HAL_UNLOCK(hadc);

          return HAL_TIMEOUT;
        }
      }
      Conversion_Timeout_CPU_cycles ++;
    }
  }

  /* Clear regular group conversion flag */
  __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_STRT | ADC_FLAG_EOC);

  /* Update ADC state machine */
  SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC);

  /* Determine whether any further conversion upcoming on group regular       */
  /* by external trigger, continuous mode or scan sequence on going.          */
  /* Note: On STM32F37x devices, in case of sequencer enabled                 */
  /*       (several ranks selected), end of conversion flag is raised         */
  /*       at the end of the sequence.                                        */
  if(ADC_IS_SOFTWARE_START_REGULAR(hadc)        &&
     (hadc->Init.ContinuousConvMode == DISABLE)   )
  {
    /* Set ADC state */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);

    if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_INJ_BUSY))
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_READY);
    }
  }

  /* Return ADC state */
  return HAL_OK;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Poll for conversion event.
  * @param  hadc ADC handle
  * @param  EventType the ADC event type.
  *          This parameter can be one of the following values:
  *            @arg ADC_AWD1_EVENT: ADC Analog watchdog 1 event (main analog watchdog, present on all STM32 devices)
  *            @arg ADC_AWD2_EVENT: ADC Analog watchdog 2 event (additional analog watchdog, not present on all STM32 families)
  *            @arg ADC_AWD3_EVENT: ADC Analog watchdog 3 event (additional analog watchdog, not present on all STM32 families)
  *            @arg ADC_OVR_EVENT: ADC Overrun event
  *            @arg ADC_JQOVF_EVENT: ADC Injected context queue overflow event
  * @param  Timeout Timeout value in millisecond.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_PollForEvent(ADC_HandleTypeDef* hadc, uint32_t EventType, uint32_t Timeout)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_EVENT_TYPE(EventType));

  /* Get start tick count */
  tickstart = HAL_GetTick();

  /* Check selected event flag */
  while(__HAL_ADC_GET_FLAG(hadc, EventType) == RESET)
  {
    /* Check if timeout is disabled (set to infinite wait) */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U) || ((HAL_GetTick() - tickstart ) > Timeout))
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
  /* Note: In case of several analog watchdog enabled, if needed to know      */
  /* which one triggered and on which ADCx, test ADC state of analog watchdog */
  /* flags HAL_ADC_STATE_AWD1/2U/3 using function "HAL_ADC_GetState()".        */
  /* For example:                                                             */
  /*  " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_AWD1)) "    */
  /*  " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_AWD2)) "    */
  /*  " if (HAL_IS_BIT_SET(HAL_ADC_GetState(hadc1), HAL_ADC_STATE_AWD3)) "    */
  /* Check analog watchdog 1 flag */
  case ADC_AWD_EVENT:
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD1);

    /* Clear ADC analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD1);
    break;

  /* Check analog watchdog 2 flag */
  case ADC_AWD2_EVENT:
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD2);

    /* Clear ADC analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD2);
    break;

  /* Check analog watchdog 3 flag */
  case ADC_AWD3_EVENT:
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD3);

    /* Clear ADC analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD3);
    break;

  /* Injected context queue overflow event */
  case ADC_JQOVF_EVENT:
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_INJ_JQOVF);

    /* Set ADC error code to Injected context queue overflow */
    SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_JQOVF);

    /* Clear ADC Injected context queue overflow flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JQOVF);
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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Poll for conversion event.
  * @param  hadc ADC handle
  * @param  EventType the ADC event type.
  *          This parameter can be one of the following values:
  *            @arg ADC_AWD_EVENT: ADC Analog watchdog event.
  * @param  Timeout Timeout value in millisecond.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_PollForEvent(ADC_HandleTypeDef* hadc, uint32_t EventType, uint32_t Timeout)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_EVENT_TYPE(EventType));

  tickstart = HAL_GetTick();

  /* Check selected event flag */
  while(__HAL_ADC_GET_FLAG(hadc, EventType) == RESET)
  {
    /* Check if timeout is disabled (set to infinite wait) */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
      {
        /* Update ADC state machine to timeout */
        SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }
  }

  /* Analog watchdog (level out of window) event */
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD1);

  /* Clear ADC analog watchdog flag */
  __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD);

  /* Return ADC state */
  return HAL_OK;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Enables ADC, starts conversion of regular group with interruption.
  *         Interruptions enabled in this function:
  *          - EOC (end of conversion of regular group) or EOS (end of
  *            sequence of regular group) depending on ADC initialization
  *            parameter "EOCSelection"
  *          - overrun, depending on ADC initialization parameter "Overrun"
  *         Each of these interruptions has its dedicated callback function.
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC slave first, then ADC master.
  *         For ADC slave, ADC is enabled only (conversion is not started).
  *         For ADC master, ADC is enabled and multimode conversion is started.
  * @param  hadc ADC handle
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
    tmp_hal_status = ADC_Enable(hadc);

    /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to regular group conversion results   */
      /* - Set state bitfield related to regular operation                    */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP,
                        HAL_ADC_STATE_REG_BUSY);

      /* Set group injected state (from auto-injection) and multimode state   */
      /* for all cases of multimode: independent mode, multimode ADC master   */
      /* or multimode ADC slave (for devices with several ADCs):              */
      if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
      {
        /* Set ADC state (ADC independent or master) */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);

        /* If conversions on group regular are also triggering group injected,*/
        /* update ADC state.                                                  */
        if (READ_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO) != RESET)
        {
          ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
        }
      }
      else
      {
        /* Set ADC state (ADC slave) */
        SET_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);

        /* If conversions on group regular are also triggering group injected,*/
        /* update ADC state.                                                  */
        if (ADC_MULTIMODE_AUTO_INJECTED(hadc))
        {
          ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
        }
      }

      /* State machine update: Check if an injected conversion is ongoing */
      if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY))
      {
        /* Reset ADC error code fields related to conversions on group regular*/
        CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
      }
      else
      {
        /* Reset ADC all error code fields */
        ADC_CLEAR_ERRORCODE(hadc);
      }

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
      switch(hadc->Init.EOCSelection)
      {
        case ADC_EOC_SEQ_CONV:
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC);
          __HAL_ADC_ENABLE_IT(hadc, (ADC_IT_EOS));
          break;
        /* case ADC_EOC_SINGLE_CONV */
        default:
          __HAL_ADC_ENABLE_IT(hadc, (ADC_IT_EOC | ADC_IT_EOS));
          break;
      }

      /* If overrun is set to overwrite previous data (default setting),      */
      /* overrun interrupt is not activated (overrun event is not considered  */
      /* as an error).                                                        */
      /* (cf ref manual "Managing conversions without using the DMA and       */
      /* without overrun ")                                                   */
      if (hadc->Init.Overrun == ADC_OVR_DATA_PRESERVED)
      {
        __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);
      }

      /* Enable conversion of regular group.                                  */
      /* If software start has been selected, conversion starts immediately.  */
      /* If external trigger has been selected, conversion will start at next */
      /* trigger event.                                                       */
      /* Case of multimode enabled (for devices with several ADCs):           */
      /*  - if ADC is slave, ADC is enabled only (conversion is not started). */
      /*  - if ADC is master, ADC is enabled and conversion is started.       */
      if (ADC_NONMULTIMODE_REG_OR_MULTIMODEMASTER(hadc))
      {
        SET_BIT(hadc->Instance->CR, ADC_CR_ADSTART);
      }
    }
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Enables ADC, starts conversion of regular group with interruption.
  *         Interruptions enabled in this function:
  *          - EOC (end of conversion of regular group)
  *         Each of these interruptions has its dedicated callback function.
  * @param  hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Enable the ADC peripheral */
  tmp_hal_status = ADC_Enable(hadc);

  /* Start conversion if ADC is effectively enabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state                                                          */
    /* - Clear state bitfield related to regular group conversion results     */
    /* - Set state bitfield related to regular operation                      */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC,
                      HAL_ADC_STATE_REG_BUSY);

    /* Set group injected state (from auto-injection) */
    /* If conversions on group regular are also triggering group injected,    */
    /* update ADC state.                                                      */
    if (READ_BIT(hadc->Instance->CR1, ADC_CR1_JAUTO) != RESET)
    {
      ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
    }

    /* State machine update: Check if an injected conversion is ongoing */
    if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY))
    {
      /* Reset ADC error code fields related to conversions on group regular */
      CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
    }
    else
    {
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
    }

    /* Process unlocked */
    /* Unlock before starting ADC conversions: in case of potential           */
    /* interruption, to let the process to ADC IRQ Handler.                   */
    __HAL_UNLOCK(hadc);

    /* Clear regular group conversion flag and overrun flag */
    /* (To ensure of no unknown state from potential previous ADC operations) */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_EOC);

    /* Enable end of conversion interrupt for regular group */
    __HAL_ADC_ENABLE_IT(hadc, ADC_IT_EOC);

    /* Enable conversion of regular group.                                    */
    /* If software start has been selected, conversion starts immediately.    */
    /* If external trigger has been selected, conversion will start at next   */
    /* trigger event.                                                         */
    if (ADC_IS_SOFTWARE_START_REGULAR(hadc))
    {
      /* Start ADC conversion on regular group with SW start */
      SET_BIT(hadc->Instance->CR2, (ADC_CR2_SWSTART | ADC_CR2_EXTTRIG));
    }
    else
    {
      /* Start ADC conversion on regular group with external trigger */
      SET_BIT(hadc->Instance->CR2, ADC_CR2_EXTTRIG);
    }
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Stop ADC conversion of both groups regular and injected,
  *         disable ADC peripheral.
  *         Interruptions disabled in this function:
  *          - EOC (end of conversion of regular group) and EOS (end of
  *            sequence of regular group)
  *          - overrun
  * @note   ADC peripheral disable is forcing interruption of potential
  *         conversion on injected group. If injected group is under use,
  *         it should be preliminarily stopped using function
  *         @ref HAL_ADCEx_InjectedStop().
  *         To stop ADC conversion only on ADC group regular
  *         while letting ADC group injected conversions running,
  *         use function @ref HAL_ADCEx_RegularStop_IT().
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* 1. Stop potential conversion on going, on regular and injected groups */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_INJECTED_GROUP);

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
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Stop ADC conversion of regular group (and injected group in
  *         case of auto_injection mode), disable interrution of
  *         end-of-conversion, disable ADC peripheral.
  * @param  hadc ADC handle
  * @retval None
  */
HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential conversion on going, on regular and injected groups */
  /* Disable ADC peripheral */
  tmp_hal_status = ADC_ConversionStop_Disable(hadc);

  /* Check if ADC is effectively disabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Disable ADC end of conversion interrupt for regular group */
    __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC);

    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                      HAL_ADC_STATE_READY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Enables ADC, starts conversion of regular group and transfers result
  *         through DMA.
  *         Interruptions enabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun
  *         Each of these interruptions has its dedicated callback function.
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function is for single-ADC mode only. For multimode, use the
  *         dedicated MultimodeStart function.
  * @param  hadc ADC handle
  * @param  pData The destination Buffer address.
  * @param  Length The length of data to be transferred from ADC peripheral to memory.
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

    /* Verification if multimode is disabled (for devices with several ADC)   */
    /* If multimode is enabled, dedicated function multimode conversion       */
    /* start DMA must be used.                                                */
    if(ADC_COMMON_CCR_MULTI(hadc) == RESET)
    {
      /* Enable the ADC peripheral */
      tmp_hal_status = ADC_Enable(hadc);

      /* Start conversion if ADC is effectively enabled */
      if (tmp_hal_status == HAL_OK)
      {
        /* Set ADC state                                                      */
        /* - Clear state bitfield related to regular group conversion results */
        /* - Set state bitfield related to regular operation                  */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP,
                          HAL_ADC_STATE_REG_BUSY);

        /* Set group injected state (from auto-injection) and multimode state */
        /* for all cases of multimode: independent mode, multimode ADC master */
        /* or multimode ADC slave (for devices with several ADCs):            */
        if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
        {
          /* Set ADC state (ADC independent or master) */
          CLEAR_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);

          /* If conversions on group regular are also triggering group injected,*/
          /* update ADC state.                                                  */
          if (READ_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO) != RESET)
          {
            ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
          }
        }
        else
        {
          /* Set ADC state (ADC slave) */
          SET_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);

          /* If conversions on group regular are also triggering group injected,*/
          /* update ADC state.                                                  */
          if (ADC_MULTIMODE_AUTO_INJECTED(hadc))
          {
            ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
          }
        }

        /* State machine update: Check if an injected conversion is ongoing */
        if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY))
        {
          /* Reset ADC error code fields related to conversions on group regular*/
          CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
        }
        else
        {
          /* Reset ADC all error code fields */
          ADC_CLEAR_ERRORCODE(hadc);
        }

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


        /* Manage ADC and DMA start: ADC overrun interruption, DMA start, ADC */
        /* start (in case of SW start):                                       */

        /* Clear regular group conversion flag and overrun flag */
        /* (To ensure of no unknown state from potential previous ADC         */
        /* operations)                                                        */
        __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));

        /* Enable ADC overrun interrupt */
        __HAL_ADC_ENABLE_IT(hadc, ADC_IT_OVR);

        /* Enable ADC DMA mode */
        SET_BIT(hadc->Instance->CFGR, ADC_CFGR_DMAEN);

        /* Start the DMA channel */
        HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&hadc->Instance->DR, (uint32_t)pData, Length);

        /* Enable conversion of regular group.                                */
        /* If software start has been selected, conversion starts immediately.*/
        /* If external trigger has been selected, conversion will start at    */
        /* next trigger event.                                                */
        SET_BIT(hadc->Instance->CR, ADC_CR_ADSTART);

      }
      else
      {
        /* Process unlocked */
        __HAL_UNLOCK(hadc);
      }
    }
    else
    {
      tmp_hal_status = HAL_ERROR;

      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Enables ADC, starts conversion of regular group and transfers result
  *         through DMA.
  *         Interruptions enabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *         Each of these interruptions has its dedicated callback function.
  * @note   For devices with several ADCs: This function is for single-ADC mode
  *         only. For multimode, use the dedicated MultimodeStart function.
  * @param  hadc ADC handle
  * @param  pData The destination Buffer address.
  * @param  Length The length of data to be transferred from ADC peripheral to memory.
  * @retval None
  */
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Enable the ADC peripheral */
  tmp_hal_status = ADC_Enable(hadc);

  /* Start conversion if ADC is effectively enabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state                                                          */
    /* - Clear state bitfield related to regular group conversion results     */
    /* - Set state bitfield related to regular operation                      */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC,
                      HAL_ADC_STATE_REG_BUSY);

    /* Set group injected state (from auto-injection) */
    /* If conversions on group regular are also triggering group injected,    */
    /* update ADC state.                                                      */
    if (READ_BIT(hadc->Instance->CR1, ADC_CR1_JAUTO) != RESET)
    {
      ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
    }

    /* State machine update: Check if an injected conversion is ongoing */
    if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY))
    {
      /* Reset ADC error code fields related to conversions on group regular */
      CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
    }
    else
    {
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
    }

    /* Process unlocked */
    /* Unlock before starting ADC conversions: in case of potential           */
    /* interruption, to let the process to ADC IRQ Handler.                   */
    __HAL_UNLOCK(hadc);

    /* Set the DMA transfer complete callback */
    hadc->DMA_Handle->XferCpltCallback = ADC_DMAConvCplt;

    /* Set the DMA half transfer complete callback */
    hadc->DMA_Handle->XferHalfCpltCallback = ADC_DMAHalfConvCplt;

    /* Set the DMA error callback */
    hadc->DMA_Handle->XferErrorCallback = ADC_DMAError;


    /* Manage ADC and DMA start: ADC overrun interruption, DMA start, ADC     */
    /* start (in case of SW start):                                           */

    /* Clear regular group conversion flag and overrun flag */
    /* (To ensure of no unknown state from potential previous ADC operations) */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_EOC);

    /* Enable ADC DMA mode */
    hadc->Instance->CR2 |= ADC_CR2_DMA;

    /* Start the DMA channel */
    HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&hadc->Instance->DR, (uint32_t)pData, Length);

    /* Enable conversion of regular group.                                    */
    /* If software start has been selected, conversion starts immediately.    */
    /* If external trigger has been selected, conversion will start at next   */
    /* trigger event.                                                         */
    /* Note: Alternate trigger for single conversion could be to force an     */
    /*       additional set of bit ADON "hadc->Instance->CR2 |= ADC_CR2_ADON;"*/
    if (ADC_IS_SOFTWARE_START_REGULAR(hadc))
    {
      /* Start ADC conversion on regular group with SW start */
      SET_BIT(hadc->Instance->CR2, (ADC_CR2_SWSTART | ADC_CR2_EXTTRIG));
    }
    else
    {
      /* Start ADC conversion on regular group with external trigger */
      SET_BIT(hadc->Instance->CR2, ADC_CR2_EXTTRIG);
    }
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Stop ADC conversion of both groups regular and injected,
  *         disable ADC DMA transfer, disable ADC peripheral.
  *         Interruptions disabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun
  * @note   ADC peripheral disable is forcing interruption of potential
  *         conversion on injected group. If injected group is under use,
  *         it should be preliminarily stopped using function
  *         @ref HAL_ADCEx_InjectedStop().
  *         To stop ADC conversion only on ADC group regular
  *         while letting ADC group injected conversions running,
  *         use function @ref HAL_ADCEx_RegularStop_DMA().
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function is for single-ADC mode only. For multimode, use the
  *         dedicated MultimodeStop function.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* 1. Stop potential conversion on going, on regular and injected groups */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_INJECTED_GROUP);

  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {
    /* Disable ADC DMA (ADC DMA configuration ADC_CFGR_DMACFG is kept) */
    CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_DMAEN);

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
    /* Update "tmp_hal_status" only if DMA channel disabling passed,          */
    /* to retain a potential failing status.                                  */
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
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }

  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Stop ADC conversion of regular group (and injected group in
  *         case of auto_injection mode), disable ADC DMA transfer, disable
  *         ADC peripheral.
  * @note   ADC peripheral disable is forcing interruption of potential
  *         conversion on injected group. If injected group is under use, it
  *         should be preliminarily stopped using HAL_ADCEx_InjectedStop function.
  * @note   For devices with several ADCs: This function is for single-ADC mode
  *         only. For multimode, use the dedicated MultimodeStop function.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential conversion on going, on regular and injected groups */
  /* Disable ADC peripheral */
  tmp_hal_status = ADC_ConversionStop_Disable(hadc);

  /* Check if ADC is effectively disabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Disable ADC DMA mode */
    hadc->Instance->CR2 &= ~ADC_CR2_DMA;

    /* Disable the DMA channel (in case of DMA in circular mode or stop while */
    /* while DMA transfer is on going)                                        */
    tmp_hal_status = HAL_DMA_Abort(hadc->DMA_Handle);

    /* Check if DMA channel effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
    else
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);
    }
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Get ADC regular group conversion result.
  * @note   Reading register DR automatically clears ADC flag EOC
  *         (ADC group regular end of unitary conversion).
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
  * @param  hadc ADC handle
  * @retval ADC group regular conversion data
  */
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Note: ADC flag EOC is not cleared here by software because               */
  /*       automatically cleared by hardware when reading register DR.        */

  /* Return ADC converted value */
  return hadc->Instance->DR;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Get ADC regular group conversion result.
  * @note   Reading register DR automatically clears ADC flag EOC
  *         (ADC group regular end of unitary conversion).
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
  * @param  hadc ADC handle
  * @retval ADC group regular conversion data
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
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Handles ADC interrupt request.
  * @param  hadc ADC handle
  * @retval None
  */
void HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc)
{
  uint32_t overrun_error = 0U; /* flag set if overrun occurrence has to be considered as an error */
  ADC_Common_TypeDef *tmpADC_Common;
  uint32_t tmp_cfgr     = 0x0U;
  uint32_t tmp_cfgr_jqm = 0x0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  assert_param(IS_ADC_EOC_SELECTION(hadc->Init.EOCSelection));

  /* ========== Check End of Conversion flag for regular group ========== */
  if( (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOC) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_EOC)) ||
      (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOS) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_EOS))   )
  {
    /* Update state machine on conversion status if not in error state */
    if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL))
    {
      /* Set ADC state */
      SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC);
    }

    /* Get relevant register CFGR in ADC instance of ADC master or slave    */
    /* in function of multimode state (for devices with multimode           */
    /* available).                                                          */
    if (ADC_NONMULTIMODE_REG_OR_MULTIMODEMASTER(hadc))
    {
      tmp_cfgr = READ_REG(hadc->Instance->CFGR);
    }
    else
    {
      tmp_cfgr = READ_REG(ADC_MASTER_INSTANCE(hadc)->CFGR);
    }

    /* Disable interruption if no further conversion upcoming by regular      */
    /* external trigger or by continuous mode,                                */
    /* and if scan sequence if completed.                                     */
    if(ADC_IS_SOFTWARE_START_REGULAR(hadc)         &&
       (READ_BIT(tmp_cfgr, ADC_CFGR_CONT) == RESET)  )
    {
      /* If End of Sequence is reached, disable interrupts */
      if( __HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOS) )
      {
        /* Allowed to modify bits ADC_IT_EOC/ADC_IT_EOS only if bit           */
        /* ADSTART==0 (no conversion on going)                                */
        if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
        {
          /* Disable ADC end of sequence conversion interrupt */
          /* Note: Overrun interrupt was enabled with EOC interrupt in        */
          /* HAL_Start_IT(), but is not disabled here because can be used     */
          /* by overrun IRQ process below.                                    */
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC | ADC_IT_EOS);

          /* Set ADC state */
          CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);

          if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_INJ_BUSY))
          {
            SET_BIT(hadc->State, HAL_ADC_STATE_READY);
          }
        }
        else
        {
          /* Update ADC state machine to error */
          SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);

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
    /*       conversion flags clear induces the release of the preserved      */
    /*       data.                                                            */
    /*       Therefore, if the preserved data value is needed, it must be     */
    /*       read preliminarily into HAL_ADC_ConvCpltCallback().              */
    __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS) );
  }


  /* ========== Check End of Conversion flag for injected group ========== */
  if( (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_JEOC) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_JEOC)) ||
      (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_JEOS) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_JEOS))   )
  {
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_INJ_EOC);

    /* Get relevant register CFGR in ADC instance of ADC master or slave      */
    /* in function of multimode state (for devices with multimode             */
    /* available).                                                            */
    if (ADC_NONMULTIMODE_REG_OR_MULTIMODEMASTER(hadc))
    {
      tmp_cfgr = READ_REG(hadc->Instance->CFGR);
    }
    else
    {
      tmp_cfgr = READ_REG(ADC_MASTER_INSTANCE(hadc)->CFGR);
    }

    /* Disable interruption if no further conversion upcoming by injected     */
    /* external trigger or by automatic injected conversion with regular      */
    /* group having no further conversion upcoming (same conditions as        */
    /* regular group interruption disabling above),                           */
    /* and if injected scan sequence is completed.                            */
    if(ADC_IS_SOFTWARE_START_INJECTED(hadc)                   ||
       ((READ_BIT (tmp_cfgr, ADC_CFGR_JAUTO) == RESET)    &&
        (ADC_IS_SOFTWARE_START_REGULAR(hadc)          &&
        (READ_BIT (tmp_cfgr, ADC_CFGR_CONT) == RESET)   )   )   )
    {
      /* If End of Sequence is reached, disable interrupts */
      if( __HAL_ADC_GET_FLAG(hadc, ADC_FLAG_JEOS))
      {

        /* Get relevant register CFGR in ADC instance of ADC master or slave  */
        /* in function of multimode state (for devices with multimode         */
        /* available).                                                        */
        if (ADC_NONMULTIMODE_INJ_OR_MULTIMODEMASTER(hadc))
        {
          tmp_cfgr_jqm = READ_REG(hadc->Instance->CFGR);
        }
        else
        {
          tmp_cfgr_jqm = READ_REG(ADC_MASTER_INSTANCE(hadc)->CFGR);
        }

        /* Particular case if injected contexts queue is enabled:             */
        /* when the last context has been fully processed, JSQR is reset      */
        /* by the hardware. Even if no injected conversion is planned to come */
        /* (queue empty, triggers are ignored), it can start again            */
        /* immediately after setting a new context (JADSTART is still set).   */
        /* Therefore, state of HAL ADC injected group is kept to busy.        */
        if(READ_BIT(tmp_cfgr_jqm, ADC_CFGR_JQM) == RESET)
        {
          /* Allowed to modify bits ADC_IT_JEOC/ADC_IT_JEOS only if bit       */
          /* JADSTART==0 (no conversion on going)                             */
          if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
          {
            /* Disable ADC end of sequence conversion interrupt  */
            __HAL_ADC_DISABLE_IT(hadc, ADC_IT_JEOC | ADC_IT_JEOS);

            /* Set ADC state */
            CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);

            if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
            {
              SET_BIT(hadc->State, HAL_ADC_STATE_READY);
            }
          }
          else
          {
            /* Update ADC state machine to error */
            SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);

            /* Set ADC error code to ADC IP internal error */
            SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
          }
        }
      }
    }

    /* Conversion complete callback */
    /* Note: into callback, to determine if conversion has been triggered     */
    /*       from JEOC or JEOS, possibility to use:                           */
    /*        " if( __HAL_ADC_GET_FLAG(&hadc, ADC_FLAG_JEOS)) "               */
    HAL_ADCEx_InjectedConvCpltCallback(hadc);

    /* Clear injected group conversion flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JEOC | ADC_FLAG_JEOS);
  }

  /* ========== Check analog watchdog 1 flag ========== */
  if (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_AWD1) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_AWD1))
  {
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD1);

    /* Level out of window 1 callback */
    HAL_ADC_LevelOutOfWindowCallback(hadc);
    /* Clear ADC analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD1);
  }

  /* ========== Check analog watchdog 2 flag ========== */
  if (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_AWD2) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_AWD2))
  {
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD2);

    /* Level out of window 2 callback */
    HAL_ADCEx_LevelOutOfWindow2Callback(hadc);
    /* Clear ADC analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD2);
  }

  /* ========== Check analog watchdog 3 flag ========== */
  if (__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_AWD3) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_AWD3))
  {
    /* Set ADC state */
    SET_BIT(hadc->State, HAL_ADC_STATE_AWD3);

    /* Level out of window 3 callback */
    HAL_ADCEx_LevelOutOfWindow3Callback(hadc);
    /* Clear ADC analog watchdog flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD3);
  }

  /* ========== Check Overrun flag ========== */
  if(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_OVR) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_OVR))
  {
    /* If overrun is set to overwrite previous data (default setting),        */
    /* overrun event is not considered as an error.                           */
    /* (cf ref manual "Managing conversions without using the DMA and         */
    /* without overrun ")                                                     */
    /* Exception for usage with DMA overrun event always considered as an     */
    /* error.                                                                 */
    if (hadc->Init.Overrun == ADC_OVR_DATA_PRESERVED)
    {
      overrun_error = 1U;
    }
    else
    {
      /* Pointer to the common control register to which is belonging hadc    */
      /* (Depending on STM32F3 product, there may be up to 4 ADC and 2 common */
      /* control registers)                                                   */
      tmpADC_Common = ADC_COMMON_REGISTER(hadc);

      /* Check DMA configuration, depending on MultiMode set or not */
      if (READ_BIT(tmpADC_Common->CCR, ADC_CCR_MULTI) == ADC_MODE_INDEPENDENT)
      {
        if (HAL_IS_BIT_SET(hadc->Instance->CFGR, ADC_CFGR_DMAEN))
        {
          overrun_error = 1U;
        }
      }
      else
      {
        /* MultiMode is enabled, Common Control Register MDMA bits must be checked */
        if (READ_BIT(tmpADC_Common->CCR, ADC_CCR_MDMA) != RESET)
        {
          overrun_error = 1U;
        }
      }
    }

    if (overrun_error == 1U)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_REG_OVR);

      /* Set ADC error code to ADC IP internal error */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_OVR);

      /* Error callback */
      HAL_ADC_ErrorCallback(hadc);
    }

    /* Clear the Overrun flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_OVR);

  }


  /* ========== Check Injected context queue overflow flag ========== */
  if(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_JQOVF) && __HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_JQOVF))
  {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_INJ_JQOVF);

      /* Set ADC error code to ADC IP internal error */
      SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_JQOVF);

    /* Clear the Injected context queue overflow flag */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JQOVF);

    /* Error callback */
    HAL_ADCEx_InjectedQueueOverflowCallback(hadc);
  }

}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Handles ADC interrupt request
  * @param  hadc ADC handle
  * @retval None
  */
void HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  assert_param(IS_ADC_REGULAR_NB_CONV(hadc->Init.NbrOfConversion));


  /* ========== Check End of Conversion flag for regular group ========== */
  if(__HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_EOC))
  {
    if(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_EOC) )
    {
      /* Update state machine on conversion status if not in error state */
      if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL))
      {
        /* Set ADC state */
        SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC);
      }

      /* Determine whether any further conversion upcoming on group regular   */
      /* by external trigger, continuous mode or scan sequence on going.      */
      /* Note: On STM32F37x devices, in case of sequencer enabled             */
      /*       (several ranks selected), end of conversion flag is raised     */
      /*       at the end of the sequence.                                    */
      if(ADC_IS_SOFTWARE_START_REGULAR(hadc)       &&
         (hadc->Init.ContinuousConvMode == DISABLE)  )
      {
        /* Disable ADC end of single conversion interrupt  */
        __HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC);

        /* Set ADC state */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);

        if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_INJ_BUSY))
        {
          SET_BIT(hadc->State, HAL_ADC_STATE_READY);
        }
      }

      /* Conversion complete callback */
      HAL_ADC_ConvCpltCallback(hadc);

      /* Clear regular group conversion flag */
      __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_STRT | ADC_FLAG_EOC);
    }
  }

  /* ========== Check End of Conversion flag for injected group ========== */
  if(__HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_JEOC))
  {
    if(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_JEOC))
    {
      /* Update state machine on conversion status if not in error state */
      if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL))
      {
        /* Set ADC state */
        SET_BIT(hadc->State, HAL_ADC_STATE_INJ_EOC);
      }

      /* Determine whether any further conversion upcoming on group injected  */
      /* by external trigger, scan sequence on going or by automatic injected */
      /* conversion from group regular (same conditions as group regular      */
      /* interruption disabling above).                                       */
      /* Note: On STM32F37x devices, in case of sequencer enabled             */
      /*       (several ranks selected), end of conversion flag is raised     */
      /*       at the end of the sequence.                                    */
      if(ADC_IS_SOFTWARE_START_INJECTED(hadc)                     ||
         (HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_JAUTO) &&
         (ADC_IS_SOFTWARE_START_REGULAR(hadc)       &&
          (hadc->Init.ContinuousConvMode == DISABLE)  )         )   )
      {
        /* Disable ADC end of single conversion interrupt  */
        __HAL_ADC_DISABLE_IT(hadc, ADC_IT_JEOC);

        /* Set ADC state */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);

        if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
        {
          SET_BIT(hadc->State, HAL_ADC_STATE_READY);
        }
      }

      /* Conversion complete callback */
      HAL_ADCEx_InjectedConvCpltCallback(hadc);

      /* Clear injected group conversion flag */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_JSTRT | ADC_FLAG_JEOC));
    }
  }

  /* ========== Check Analog watchdog flags ========== */
  if(__HAL_ADC_GET_IT_SOURCE(hadc, ADC_IT_AWD))
  {
    if(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_AWD))
    {
      /* Set ADC state */
      SET_BIT(hadc->State, HAL_ADC_STATE_AWD1);

      /* Level out of window callback */
      HAL_ADC_LevelOutOfWindowCallback(hadc);

      /* Clear the ADC analog watchdog flag */
      __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_AWD);
    }
  }

}
#endif /* STM32F373xC || STM32F378xx */


#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Perform an ADC automatic self-calibration
  *         Calibration prerequisite: ADC must be disabled (execute this
  *         function before HAL_ADC_Start() or after HAL_ADC_Stop() ).
  * @param  hadc ADC handle
  * @param  SingleDiff Selection of single-ended or differential input
  *          This parameter can be one of the following values:
  *            @arg ADC_SINGLE_ENDED: Channel in mode input single ended
  *            @arg ADC_DIFFERENTIAL_ENDED: Channel in mode input differential ended
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef* hadc, uint32_t SingleDiff)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Calibration prerequisite: ADC must be disabled. */

  /* Disable the ADC (if not already disabled) */
  tmp_hal_status = ADC_Disable(hadc);

  /* Check if ADC is effectively disabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Change ADC state */
    hadc->State = HAL_ADC_STATE_READY;

    /* Select calibration mode single ended or differential ended */
    hadc->Instance->CR &= (~ADC_CR_ADCALDIF);
    if (SingleDiff == ADC_DIFFERENTIAL_ENDED)
    {
      hadc->Instance->CR |= ADC_CR_ADCALDIF;
    }

    /* Start ADC calibration */
    hadc->Instance->CR |= ADC_CR_ADCAL;

    tickstart = HAL_GetTick();

    /* Wait for calibration completion */
    while(HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADCAL))
    {
      if((HAL_GetTick() - tickstart) > ADC_CALIBRATION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_BUSY_INTERNAL,
                          HAL_ADC_STATE_ERROR_INTERNAL);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }

    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_BUSY_INTERNAL,
                      HAL_ADC_STATE_READY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Perform an ADC automatic self-calibration
  *         Calibration prerequisite: ADC must be disabled (execute this
  *         function before HAL_ADC_Start() or after HAL_ADC_Stop() ).
  *         During calibration process, ADC is enabled. ADC is let enabled at
  *         the completion of this function.
  * @param  hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tickstart;
  __IO uint32_t wait_loop_index = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* 1. Calibration prerequisite:                                             */
  /*    - ADC must be disabled for at least two ADC clock cycles in disable   */
  /*      mode before ADC enable                                              */
  /* Stop potential conversion on going, on regular and injected groups       */
  /* Disable ADC peripheral */
  tmp_hal_status = ADC_ConversionStop_Disable(hadc);

  /* Check if ADC is effectively disabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                      HAL_ADC_STATE_BUSY_INTERNAL);

    /* Wait two ADC clock cycles */
    while(wait_loop_index < ADC_CYCLE_WORST_CASE_CPU_CYCLES *2U)
    {
      wait_loop_index++;
    }

    /* 2. Enable the ADC peripheral */
    ADC_Enable(hadc);


    /* 3. Resets ADC calibration registers */
    SET_BIT(hadc->Instance->CR2, ADC_CR2_RSTCAL);

    tickstart = HAL_GetTick();

    /* Wait for calibration reset completion */
    while(HAL_IS_BIT_SET(hadc->Instance->CR2, ADC_CR2_RSTCAL))
    {
      if((HAL_GetTick() - tickstart) > ADC_CALIBRATION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_BUSY_INTERNAL,
                          HAL_ADC_STATE_ERROR_INTERNAL);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }


    /* 4. Start ADC calibration */
    SET_BIT(hadc->Instance->CR2, ADC_CR2_CAL);

    tickstart = HAL_GetTick();

    /* Wait for calibration completion */
    while(HAL_IS_BIT_SET(hadc->Instance->CR2, ADC_CR2_CAL))
    {
      if((HAL_GetTick() - tickstart) > ADC_CALIBRATION_TIMEOUT)
      {
        /* Update ADC state machine to error */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_BUSY_INTERNAL,
                          HAL_ADC_STATE_ERROR_INTERNAL);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }

    /* Set ADC state */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_BUSY_INTERNAL,
                      HAL_ADC_STATE_READY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}

#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Get the calibration factor from automatic conversion result
  * @param  hadc ADC handle
  * @param  SingleDiff Selection of single-ended or differential input
  *          This parameter can be one of the following values:
  *            @arg ADC_SINGLE_ENDED: Channel in mode input single ended
  *            @arg ADC_DIFFERENTIAL_ENDED: Channel in mode input differential ended
  * @retval Converted value
  */
uint32_t HAL_ADCEx_Calibration_GetValue(ADC_HandleTypeDef* hadc, uint32_t SingleDiff)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff));

  /* Return the selected ADC calibration value */
  if (SingleDiff == ADC_DIFFERENTIAL_ENDED)
  {
    return ADC_CALFACT_DIFF_GET(hadc->Instance->CALFACT);
  }
  else
  {
    return ((hadc->Instance->CALFACT) & ADC_CALFACT_CALFACT_S);
  }
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Set the calibration factor to overwrite automatic conversion result. ADC must be enabled and no conversion on going.
  * @param  hadc ADC handle
  * @param  SingleDiff Selection of single-ended or differential input
  *          This parameter can be one of the following values:
  *            @arg ADC_SINGLE_ENDED: Channel in mode input single ended
  *            @arg ADC_DIFFERENTIAL_ENDED: Channel in mode input differential ended
  * @param  CalibrationFactor Calibration factor (coded on 7 bits maximum)
  * @retval HAL state
  */
HAL_StatusTypeDef HAL_ADCEx_Calibration_SetValue(ADC_HandleTypeDef* hadc, uint32_t SingleDiff, uint32_t CalibrationFactor)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(SingleDiff));
  assert_param(IS_ADC_CALFACT(CalibrationFactor));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Verification of hardware constraints before modifying the calibration    */
  /* factors register: ADC must be enabled, no conversion on going.           */
  if ( (ADC_IS_ENABLE(hadc) != RESET)                              &&
       (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)   )
  {
    /* Set the selected ADC calibration value */
    if (SingleDiff == ADC_DIFFERENTIAL_ENDED)
    {
      MODIFY_REG(hadc->Instance->CALFACT                ,
                 ADC_CALFACT_CALFACT_D                  ,
                 ADC_CALFACT_DIFF_SET(CalibrationFactor) );
    }
    else
    {
      MODIFY_REG(hadc->Instance->CALFACT,
                 ADC_CALFACT_CALFACT_S  ,
                 CalibrationFactor       );
    }
  }
  else
  {
    /* Update ADC state machine to error */
    SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

    /* Set ADC error code to ADC IP internal error */
    SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Enables ADC, starts conversion of injected group.
  *         Interruptions enabled in this function: None.
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC slave first, then ADC master.
  *         For ADC slave, ADC is enabled only (conversion is not started).
  *         For ADC master, ADC is enabled and multimode conversion is started.
  * @param  hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStart(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Perform ADC enable and conversion start if no conversion is on going */
  if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
  {
    /* Process locked */
    __HAL_LOCK(hadc);

    /* Enable the ADC peripheral */
    tmp_hal_status = ADC_Enable(hadc);

      /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to injected group conversion results  */
      /* - Set state bitfield related to injected operation                   */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_INJ_EOC,
                        HAL_ADC_STATE_INJ_BUSY);

      /* Case of independent mode or multimode(for devices with several ADCs):*/
      /* Set multimode state.                                                 */
      if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
      {
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }
      else
      {
        SET_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }

      /* Check if a regular conversion is ongoing */
      /* Note: On this device, there is no ADC error code fields related to   */
      /*       conversions on group injected only. In case of conversion on   */
      /*       going on group regular, no error code is reset.                */
      if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
      {
        /* Reset ADC all error code fields */
        ADC_CLEAR_ERRORCODE(hadc);
      }

      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);

      /* Clear injected group conversion flag */
      /* (To ensure of no unknown state from potential previous ADC           */
      /* operations)                                                          */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_JEOC | ADC_FLAG_JEOS));

      /* Enable conversion of injected group, if automatic injected           */
      /* conversion is disabled.                                              */
      /* If software start has been selected, conversion starts immediately.  */
      /* If external trigger has been selected, conversion will start at next */
      /* trigger event.                                                       */
      /* Case of multimode enabled (for devices with several ADCs):           */
      /*  - if ADC is slave, ADC is enabled only (conversion is not started). */
      /*  - if ADC is master, ADC is enabled and conversion is started.       */
      if (HAL_IS_BIT_CLR(hadc->Instance->CFGR, ADC_CFGR_JAUTO) &&
          ADC_NONMULTIMODE_INJ_OR_MULTIMODEMASTER(hadc)          )
      {
        SET_BIT(hadc->Instance->CR, ADC_CR_JADSTART);
      }
    }
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Enables ADC, starts conversion of injected group.
  *         Interruptions enabled in this function: None.
  * @param  hadc ADC handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStart(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Enable the ADC peripheral */
  tmp_hal_status = ADC_Enable(hadc);

  /* Start conversion if ADC is effectively enabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state                                                          */
    /* - Clear state bitfield related to injected group conversion results    */
    /* - Set state bitfield related to injected operation                     */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_READY | HAL_ADC_STATE_INJ_EOC,
                      HAL_ADC_STATE_INJ_BUSY);

    /* Check if a regular conversion is ongoing */
    /* Note: On this device, there is no ADC error code fields related to     */
    /*       conversions on group injected only. In case of conversion on     */
    /*       going on group regular, no error code is reset.                  */
    if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
    {
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
    }

    /* Process unlocked */
    /* Unlock before starting ADC conversions: in case of potential           */
    /* interruption, to let the process to ADC IRQ Handler.                   */
    __HAL_UNLOCK(hadc);

    /* Clear injected group conversion flag */
    /* (To ensure of no unknown state from potential previous ADC operations) */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JEOC);

    /* Enable conversion of injected group.                                   */
    /* If software start has been selected, conversion starts immediately.    */
    /* If external trigger has been selected, conversion will start at next   */
    /* trigger event.                                                         */
    /* If external trigger has been selected, conversion will start at next   */
    /* trigger event.                                                         */
    /* If automatic injected conversion is enabled, conversion will start     */
    /* after next regular group conversion.                                   */
    if (ADC_IS_SOFTWARE_START_INJECTED(hadc)               &&
        HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_JAUTO)   )
    {
      /* Start ADC conversion on injected group with SW start */
      SET_BIT(hadc->Instance->CR2, (ADC_CR2_JSWSTART | ADC_CR2_JEXTTRIG));
    }
    else
    {
      /* Start ADC conversion on injected group with external trigger */
      SET_BIT(hadc->Instance->CR2, ADC_CR2_JEXTTRIG);
    }
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Stop ADC group injected conversion (potential conversion on going
  *         on ADC group regular is not impacted), disable ADC peripheral
  *         if no conversion is on going on group regular.
  * @note   To stop ADC conversion of both groups regular and injected and to
  *         to disable ADC peripheral, instead of using 2 functions
  *         @ref HAL_ADCEx_RegularStop() and @ref HAL_ADCEx_InjectedStop(),
  *         use function @ref HAL_ADC_Stop().
  * @note   If injected group mode auto-injection is enabled,
  *         function HAL_ADC_Stop must be used.
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC master first, then ADC slave.
  *         For ADC master, conversion is stopped and ADC is disabled.
  *         For ADC slave, ADC is disabled only (conversion stop of ADC master
  *         has already stopped conversion of ADC slave).
  * @note   In case of auto-injection mode, HAL_ADC_Stop must be used.
  * @param  hadc ADC handle
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential ADC conversion on going and disable ADC peripheral        */
  /* conditioned to:                                                          */
  /* - In case of auto-injection mode, HAL_ADC_Stop must be used.             */
  /* - For ADC injected group conversion stop:                                */
  /*   On this STM32 family, conversion on the other group                    */
  /*   (group regular) can continue (groups regular and injected              */
  /*   conversion stop commands are independent)                              */
  /* - For ADC disable:                                                       */
  /*   No conversion on the other group (group regular) must be intended to   */
  /*   continue (groups regular and injected are both impacted by             */
  /*   ADC disable)                                                           */
  if(HAL_IS_BIT_CLR(hadc->Instance->CFGR, ADC_CFGR_JAUTO))
  {
    /* 1. Stop potential conversion on going on injected group only. */
    tmp_hal_status = ADC_ConversionStop(hadc, ADC_INJECTED_GROUP);

    /* Disable ADC peripheral if conversion on ADC group injected is          */
    /* effectively stopped and if no conversion on the other group            */
    /* (ADC group regular) is intended to continue.                           */
    if (tmp_hal_status == HAL_OK)
    {
      if((ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET) &&
         ((hadc->State & HAL_ADC_STATE_REG_BUSY) == RESET)    )
      {
        /* 2. Disable the ADC peripheral */
        tmp_hal_status = ADC_Disable(hadc);

        /* Check if ADC is effectively disabled */
        if (tmp_hal_status == HAL_OK)
        {
          /* Set ADC state */
          ADC_STATE_CLR_SET(hadc->State,
                            HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                            HAL_ADC_STATE_READY);
        }
      }
      /* Conversion on ADC group injected group is stopped, but ADC is not    */
      /* disabled since conversion on ADC group regular is still on going.    */
      else
      {
        /* Set ADC state */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);
      }
    }
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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Stop conversion of injected channels. Disable ADC peripheral if
  *         no regular conversion is on going.
  * @note   If ADC must be disabled and if conversion is on going on
  *         regular group, function HAL_ADC_Stop must be used to stop both
  *         injected and regular groups, and disable the ADC.
  * @note   In case of auto-injection mode, HAL_ADC_Stop must be used.
  * @param  hadc ADC handle
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential conversion and disable ADC peripheral                     */
  /* Conditioned to:                                                          */
  /* - No conversion on the other group (regular group) is intended to        */
  /*   continue (injected and regular groups stop conversion and ADC disable  */
  /*   are common)                                                            */
  /* - In case of auto-injection mode, HAL_ADC_Stop must be used.             */
  if(((hadc->State & HAL_ADC_STATE_REG_BUSY) == RESET)  &&
     HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_JAUTO)   )
  {
    /* Stop potential conversion on going, on regular and injected groups */
    /* Disable ADC peripheral */
    tmp_hal_status = ADC_ConversionStop_Disable(hadc);

    /* Check if ADC is effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
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
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Wait for injected group conversion to be completed.
  * @param  hadc ADC handle
  * @param  Timeout Timeout value in millisecond.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedPollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout)
{
  uint32_t tickstart;
  uint32_t tmp_Flag_EOC;
  uint32_t tmp_cfgr = 0x00000000U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* If end of conversion selected to end of sequence */
  if (hadc->Init.EOCSelection == ADC_EOC_SEQ_CONV)
  {
    tmp_Flag_EOC = ADC_FLAG_JEOS;
  }
  /* If end of conversion selected to end of each conversion */
  else /* ADC_EOC_SINGLE_CONV */
  {
    tmp_Flag_EOC = (ADC_FLAG_JEOC | ADC_FLAG_JEOS);
  }

  /* Get relevant register CFGR in ADC instance of ADC master or slave      */
  /* in function of multimode state (for devices with multimode             */
  /* available).                                                            */
  if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
  {
    tmp_cfgr = READ_REG(hadc->Instance->CFGR);
  }
  else
  {
    tmp_cfgr = READ_REG(ADC_MASTER_INSTANCE(hadc)->CFGR);
  }

  /* Get tick count */
  tickstart = HAL_GetTick();

  /* Wait until End of Conversion flag is raised */
  while(HAL_IS_BIT_CLR(hadc->Instance->ISR, tmp_Flag_EOC))
  {
    /* Check if timeout is disabled (set to infinite wait) */
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
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
  SET_BIT(hadc->State, HAL_ADC_STATE_INJ_EOC);

  /* Determine whether any further conversion upcoming on group injected      */
  /* by external trigger or by automatic injected conversion                  */
  /* from group regular.                                                      */
  if(ADC_IS_SOFTWARE_START_INJECTED(hadc)                   ||
     ((READ_BIT (tmp_cfgr, ADC_CFGR_JAUTO) == RESET)    &&
      (ADC_IS_SOFTWARE_START_REGULAR(hadc)          &&
      (READ_BIT (tmp_cfgr, ADC_CFGR_CONT) == RESET)   )   )   )
  {
    /* Set ADC state */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);

    if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_READY);
    }
  }

  /* Clear end of conversion flag of injected group if low power feature      */
  /* "Auto Wait" is disabled, to not interfere with this feature until data   */
  /* register is read using function HAL_ADC_GetValue().                      */
  if (READ_BIT (tmp_cfgr, ADC_CFGR_AUTDLY) == RESET)
  {
    /* Clear injected group conversion flag */
    /* (JEOC or JEOS depending on HAL ADC initialization parameter) */
    __HAL_ADC_CLEAR_FLAG(hadc, tmp_Flag_EOC);
  }

  /* Return ADC state */
  return HAL_OK;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Wait for injected group conversion to be completed.
  * @param  hadc ADC handle
  * @param  Timeout Timeout value in millisecond.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedPollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout)
{
  uint32_t tickstart = 0U;

  /* Variables for polling in case of scan mode enabled */
  uint32_t Conversion_Timeout_CPU_cycles_max =0U;
  uint32_t Conversion_Timeout_CPU_cycles =0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Get tick count */
  tickstart = HAL_GetTick();

  /* Polling for end of conversion: differentiation if single/sequence        */
  /* conversion.                                                              */
  /* For injected group, flag JEOC is set only at the end of the sequence,    */
  /* not for each conversion within the sequence.                             */
  /*  - If single conversion for injected group (scan mode disabled or        */
  /*    InjectedNbrOfConversion ==1U), flag JEOC is used to determine the      */
  /*    conversion completion.                                                */
  /*  - If sequence conversion for injected group (scan mode enabled and      */
  /*    InjectedNbrOfConversion >=2U), flag JEOC is set only at the end of the */
  /*    sequence.                                                             */
  /*    To poll for each conversion, the maximum conversion time is computed  */
  /*    from ADC conversion time (selected sampling time + conversion time of */
  /*    12.5 ADC clock cycles) and APB2/ADC clock prescalers (depending on    */
  /*    settings, conversion time range can be from 28 to 32256 CPU cycles).  */
  /*    As flag JEOC is not set after each conversion, no timeout status can  */
  /*    be set.                                                               */
  if ((hadc->Instance->JSQR & ADC_JSQR_JL) == RESET)
  {
    /* Wait until End of Conversion flag is raised */
    while(HAL_IS_BIT_CLR(hadc->Instance->SR, ADC_FLAG_JEOC))
    {
      /* Check if timeout is disabled (set to infinite wait) */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
        {
          /* Update ADC state machine to timeout */
          SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);

          /* Process unlocked */
          __HAL_UNLOCK(hadc);

          return HAL_TIMEOUT;
        }
      }
    }
  }
  else
  {
    /* Replace polling by wait for maximum conversion time */
    /* Calculation of CPU cycles corresponding to ADC conversion cycles.      */
    /* Retrieve ADC clock prescaler and ADC maximum conversion cycles on all  */
    /* channels.                                                              */
    Conversion_Timeout_CPU_cycles_max = ADC_CLOCK_PRESCALER_RANGE();
    Conversion_Timeout_CPU_cycles_max *= ADC_CONVCYCLES_MAX_RANGE(hadc);

    /* Poll with maximum conversion time */
    while(Conversion_Timeout_CPU_cycles < Conversion_Timeout_CPU_cycles_max)
    {
      /* Check if timeout is disabled (set to infinite wait) */
      if(Timeout != HAL_MAX_DELAY)
      {
        if((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
        {
          /* Update ADC state machine to timeout */
          SET_BIT(hadc->State, HAL_ADC_STATE_TIMEOUT);

          /* Process unlocked */
          __HAL_UNLOCK(hadc);

          return HAL_TIMEOUT;
        }
      }
      Conversion_Timeout_CPU_cycles ++;
    }
  }


  /* Clear injected group conversion flag (and regular conversion flag raised simultaneously) */
  __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JSTRT | ADC_FLAG_JEOC | ADC_FLAG_EOC);

  /* Update ADC state machine */
  SET_BIT(hadc->State, HAL_ADC_STATE_INJ_EOC);

  /* Determine whether any further conversion upcoming on group injected      */
  /* by external trigger or by automatic injected conversion                  */
  /* from group regular.                                                      */
  if(ADC_IS_SOFTWARE_START_INJECTED(hadc)                     ||
     (HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_JAUTO) &&
     (ADC_IS_SOFTWARE_START_REGULAR(hadc)        &&
      (hadc->Init.ContinuousConvMode == DISABLE)   )        )   )
  {
    /* Set ADC state */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);

    if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
    {
      SET_BIT(hadc->State, HAL_ADC_STATE_READY);
    }
  }

  /* Return ADC state */
  return HAL_OK;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Enables ADC, starts conversion of injected group with interruption.
  *         Interruptions enabled in this function:
  *          - JEOC (end of conversion of injected group) or JEOS (end of
  *            sequence of injected group) depending on ADC initialization
  *            parameter "EOCSelection"
  *         Each of these interruptions has its dedicated callback function.
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC slave first, then ADC master.
  *         For ADC slave, ADC is enabled only (conversion is not started).
  *         For ADC master, ADC is enabled and multimode conversion is started.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStart_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Perform ADC enable and conversion start if no conversion is on going */
  if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
  {
    /* Process locked */
    __HAL_LOCK(hadc);

    /* Enable the ADC peripheral */
    tmp_hal_status = ADC_Enable(hadc);

    /* Start conversion if ADC is effectively enabled */
      /* Start conversion if ADC is effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state                                                        */
      /* - Clear state bitfield related to injected group conversion results  */
      /* - Set state bitfield related to injected operation                   */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_INJ_EOC,
                        HAL_ADC_STATE_INJ_BUSY);

      /* Case of independent mode or multimode(for devices with several ADCs):*/
      /* Set multimode state.                                                 */
      if (ADC_NONMULTIMODE_OR_MULTIMODEMASTER(hadc))
      {
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }
      else
      {
        SET_BIT(hadc->State, HAL_ADC_STATE_MULTIMODE_SLAVE);
      }

      /* Check if a regular conversion is ongoing */
      /* Note: On this device, there is no ADC error code fields related to   */
      /*       conversions on group injected only. In case of conversion on   */
      /*       going on group regular, no error code is reset.                */
      if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
      {
        /* Reset ADC all error code fields */
        ADC_CLEAR_ERRORCODE(hadc);
      }

      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);

      /* Clear injected group conversion flag */
      /* (To ensure of no unknown state from potential previous ADC           */
      /* operations)                                                          */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_JEOC | ADC_FLAG_JEOS));

      /* Enable ADC Injected context queue overflow interrupt if this feature */
      /* is enabled.                                                          */
      if ((hadc->Instance->CFGR & ADC_CFGR_JQM) != RESET)
      {
        __HAL_ADC_ENABLE_IT(hadc, ADC_FLAG_JQOVF);
      }

      /* Enable ADC end of conversion interrupt */
      switch(hadc->Init.EOCSelection)
      {
        case ADC_EOC_SEQ_CONV:
          __HAL_ADC_DISABLE_IT(hadc, ADC_IT_JEOC);
          __HAL_ADC_ENABLE_IT(hadc, ADC_IT_JEOS);
          break;
        /* case ADC_EOC_SINGLE_CONV */
        default:
          __HAL_ADC_ENABLE_IT(hadc, ADC_IT_JEOC | ADC_IT_JEOS);
          break;
      }

      /* Enable conversion of injected group, if automatic injected           */
      /* conversion is disabled.                                              */
      /* If software start has been selected, conversion starts immediately.  */
      /* If external trigger has been selected, conversion will start at next */
      /* trigger event.                                                       */
      /* Case of multimode enabled (for devices with several ADCs):           */
      /*  - if ADC is slave, ADC is enabled only (conversion is not started). */
      /*  - if ADC is master, ADC is enabled and conversion is started.       */
      if (HAL_IS_BIT_CLR(hadc->Instance->CFGR, ADC_CFGR_JAUTO) &&
          ADC_NONMULTIMODE_INJ_OR_MULTIMODEMASTER(hadc)          )
      {
        SET_BIT(hadc->Instance->CR, ADC_CR_JADSTART);
      }
    }
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
    }
  }
  else
  {
    tmp_hal_status = HAL_BUSY;
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Enables ADC, starts conversion of injected group with interruption.
  *         Interruptions enabled in this function:
  *          - JEOC (end of conversion of injected group)
  *         Each of these interruptions has its dedicated callback function.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStart_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Enable the ADC peripheral */
  tmp_hal_status = ADC_Enable(hadc);

  /* Start conversion if ADC is effectively enabled */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set ADC state                                                          */
    /* - Clear state bitfield related to injected group conversion results    */
    /* - Set state bitfield related to injected operation                     */
    ADC_STATE_CLR_SET(hadc->State,
                      HAL_ADC_STATE_READY | HAL_ADC_STATE_INJ_EOC,
                      HAL_ADC_STATE_INJ_BUSY);

    /* Check if a regular conversion is ongoing */
    /* Note: On this device, there is no ADC error code fields related to     */
    /*       conversions on group injected only. In case of conversion on     */
    /*       going on group regular, no error code is reset.                  */
    if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_REG_BUSY))
    {
      /* Reset ADC all error code fields */
      ADC_CLEAR_ERRORCODE(hadc);
    }

    /* Process unlocked */
    /* Unlock before starting ADC conversions: in case of potential           */
    /* interruption, to let the process to ADC IRQ Handler.                   */
    __HAL_UNLOCK(hadc);

    /* Set ADC error code to none */
    ADC_CLEAR_ERRORCODE(hadc);

    /* Clear injected group conversion flag */
    /* (To ensure of no unknown state from potential previous ADC operations) */
    __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JEOC);

    /* Enable end of conversion interrupt for injected channels */
    __HAL_ADC_ENABLE_IT(hadc, ADC_IT_JEOC);

    /* Enable conversion of injected group.                                   */
    /* If software start has been selected, conversion starts immediately.    */
    /* If external trigger has been selected, conversion will start at next   */
    /* trigger event.                                                         */
    /* If external trigger has been selected, conversion will start at next   */
    /* trigger event.                                                         */
    /* If automatic injected conversion is enabled, conversion will start     */
    /* after next regular group conversion.                                   */
    if (ADC_IS_SOFTWARE_START_INJECTED(hadc)              &&
        HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_JAUTO)  )
    {
      /* Start ADC conversion on injected group with SW start */
      SET_BIT(hadc->Instance->CR2, (ADC_CR2_JSWSTART | ADC_CR2_JEXTTRIG));
    }
    else
    {
      /* Start ADC conversion on injected group with external trigger */
      SET_BIT(hadc->Instance->CR2, ADC_CR2_JEXTTRIG);
    }
  }

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Stop ADC group injected conversion (potential conversion on going
  *         on ADC group regular is not impacted), disable ADC peripheral
  *         if no conversion is on going on group regular.
  *         Interruptions disabled in this function:
  *          - JEOC (end of conversion of injected group) and JEOS (end of
  *            sequence of injected group)
  * @note   To stop ADC conversion of both groups regular and injected and to
  *         to disable ADC peripheral, instead of using 2 functions
  *         @ref HAL_ADCEx_RegularStop() and @ref HAL_ADCEx_InjectedStop(),
  *         use function @ref HAL_ADC_Stop().
  * @note   If injected group mode auto-injection is enabled,
  *         function HAL_ADC_Stop must be used.
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function must be called for ADC master first, then ADC slave.
  *         For ADC master, conversion is stopped and ADC is disabled.
  *         For ADC slave, ADC is disabled only (conversion stop of ADC master
  *         has already stopped conversion of ADC slave).
  * @note   In case of auto-injection mode, HAL_ADC_Stop must be used.
  * @param  hadc ADC handle
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential ADC conversion on going and disable ADC peripheral        */
  /* conditioned to:                                                          */
  /* - In case of auto-injection mode, HAL_ADC_Stop must be used.             */
  /* - For ADC injected group conversion stop:                                */
  /*   On this STM32 family, conversion on the other group                    */
  /*   (group regular) can continue (groups regular and injected              */
  /*   conversion stop commands are independent)                              */
  /* - For ADC disable:                                                       */
  /*   No conversion on the other group (group regular) must be intended to   */
  /*   continue (groups regular and injected are both impacted by             */
  /*   ADC disable)                                                           */
  if(HAL_IS_BIT_CLR(hadc->Instance->CFGR, ADC_CFGR_JAUTO))
  {
    /* 1. Stop potential conversion on going on injected group only. */
    tmp_hal_status = ADC_ConversionStop(hadc, ADC_INJECTED_GROUP);

    /* Disable ADC peripheral if conversion on ADC group injected is          */
    /* effectively stopped and if no conversion on the other group            */
    /* (ADC group regular) is intended to continue.                           */
    if (tmp_hal_status == HAL_OK)
    {
      /* Disable ADC end of conversion interrupt for injected channels */
      __HAL_ADC_DISABLE_IT(hadc, (ADC_IT_JEOC | ADC_IT_JEOS | ADC_IT_JQOVF));

      if((ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET) &&
         ((hadc->State & HAL_ADC_STATE_REG_BUSY) == RESET)    )
      {
        /* 2. Disable the ADC peripheral */
        tmp_hal_status = ADC_Disable(hadc);

        /* Check if ADC is effectively disabled */
        if (tmp_hal_status == HAL_OK)
        {
          /* Set ADC state */
          ADC_STATE_CLR_SET(hadc->State,
                            HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                            HAL_ADC_STATE_READY);
        }
      }
      /* Conversion on ADC group injected group is stopped, but ADC is not    */
      /* disabled since conversion on ADC group regular is still on going.    */
      else
      {
        /* Set ADC state */
        CLEAR_BIT(hadc->State, HAL_ADC_STATE_INJ_BUSY);
      }
    }
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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Stop conversion of injected channels, disable interruption of
  *         end-of-conversion. Disable ADC peripheral if no regular conversion
  *         is on going.
  * @note   If ADC must be disabled and if conversion is on going on
  *         regular group, function HAL_ADC_Stop must be used to stop both
  *         injected and regular groups, and disable the ADC.
  * @param  hadc ADC handle
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedStop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential conversion and disable ADC peripheral                     */
  /* Conditioned to:                                                          */
  /* - No conversion on the other group (regular group) is intended to        */
  /*   continue (injected and regular groups stop conversion and ADC disable  */
  /*   are common)                                                            */
  /* - In case of auto-injection mode, HAL_ADC_Stop must be used.             */
  if(((hadc->State & HAL_ADC_STATE_REG_BUSY) == RESET)  &&
     HAL_IS_BIT_CLR(hadc->Instance->CR1, ADC_CR1_JAUTO)   )
  {
    /* Stop potential conversion on going, on regular and injected groups */
    /* Disable ADC peripheral */
    tmp_hal_status = ADC_ConversionStop_Disable(hadc);

    /* Check if ADC is effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Disable ADC end of conversion interrupt for injected channels */
      __HAL_ADC_DISABLE_IT(hadc, ADC_IT_JEOC);

      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
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
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/**
  * @brief  With ADC configured in multimode, for ADC master:
  *         Enables ADC, starts conversion of regular group and transfers result
  *         through DMA.
  *         Multimode must have been previously configured using
  *         HAL_ADCEx_MultiModeConfigChannel() function.
  *         Interruptions enabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun
  *         Each of these interruptions has its dedicated callback function.
  * @note   ADC slave must be preliminarily enabled using single-mode
  *         HAL_ADC_Start() function.
  * @param  hadc ADC handle of ADC master (handle of ADC slave must not be used)
  * @param  pData The destination Buffer address.
  * @param  Length The length of data to be transferred from ADC peripheral to memory.
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_MultiModeStart_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  ADC_HandleTypeDef tmphadcSlave;
  ADC_Common_TypeDef *tmpADC_Common;

  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
  assert_param(IS_ADC_EXTTRIG_EDGE(hadc->Init.ExternalTrigConvEdge));
  assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DMAContinuousRequests));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Perform ADC enable and conversion start if no conversion is on going */
  /* (check on ADC master only) */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
  {
    /* Set a temporary handle of the ADC slave associated to the ADC master   */
    /* (Depending on STM32F3 product, there may be up to 2 ADC slaves)        */
    ADC_MULTI_SLAVE(hadc, &tmphadcSlave);

    if (tmphadcSlave.Instance == NULL)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

      /* Process unlocked */
      __HAL_UNLOCK(hadc);

      return HAL_ERROR;
    }


    /* Enable the ADC peripherals: master and slave (in case if not already   */
    /* enabled previously)                                                    */
    tmp_hal_status = ADC_Enable(hadc);
    if (tmp_hal_status == HAL_OK)
    {
      tmp_hal_status = ADC_Enable(&tmphadcSlave);
    }

    /* Start conversion all ADCs of multimode are effectively enabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state (ADC master)                                           */
      /* - Clear state bitfield related to regular group conversion results   */
      /* - Set state bitfield related to regular operation                    */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR | HAL_ADC_STATE_REG_EOSMP | HAL_ADC_STATE_MULTIMODE_SLAVE,
                        HAL_ADC_STATE_REG_BUSY);

      /* If conversions on group regular are also triggering group injected,  */
      /* update ADC state.                                                    */
      if (READ_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO) != RESET)
      {
        ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
      }

      /* Process unlocked */
      /* Unlock before starting ADC conversions: in case of potential         */
      /* interruption, to let the process to ADC IRQ Handler.                 */
      __HAL_UNLOCK(hadc);

      /* Set ADC error code to none */
      ADC_CLEAR_ERRORCODE(hadc);


      /* Set the DMA transfer complete callback */
      hadc->DMA_Handle->XferCpltCallback = ADC_DMAConvCplt;

      /* Set the DMA half transfer complete callback */
      hadc->DMA_Handle->XferHalfCpltCallback = ADC_DMAHalfConvCplt;

      /* Set the DMA error callback */
      hadc->DMA_Handle->XferErrorCallback = ADC_DMAError ;

      /* Pointer to the common control register to which is belonging hadc    */
      /* (Depending on STM32F3 product, there may be up to 4 ADC and 2 common */
      /* control registers)                                                   */
      tmpADC_Common = ADC_COMMON_REGISTER(hadc);


      /* Manage ADC and DMA start: ADC overrun interruption, DMA start, ADC   */
      /* start (in case of SW start):                                         */

      /* Clear regular group conversion flag and overrun flag */
      /* (To ensure of no unknown state from potential previous ADC operations) */
      __HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));

      /* Enable ADC overrun interrupt */
      __HAL_ADC_ENABLE_IT(hadc, ADC_IT_OVR);

      /* Start the DMA channel */
      HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&tmpADC_Common->CDR, (uint32_t)pData, Length);

      /* Enable conversion of regular group.                                  */
      /* If software start has been selected, conversion starts immediately.  */
      /* If external trigger has been selected, conversion will start at next */
      /* trigger event.                                                       */
      SET_BIT(hadc->Instance->CR, ADC_CR_ADSTART);

    }
    else
    {
      /* Process unlocked */
      __HAL_UNLOCK(hadc);
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
  * @brief  With ADC configured in multimode, for ADC master:
  *         Stop ADC group regular conversion (potential conversion on going
  *         on ADC group injected is not impacted),
  *         disable ADC DMA transfer, disable ADC peripheral
  *         if no conversion is on going on group injected.
  *         Interruptions disabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun
  * @note   In case of auto-injection mode, this function also stop conversion
  *         on ADC group injected.
  * @note   Multimode is kept enabled after this function. To disable multimode
  *         (set with HAL_ADCEx_MultiModeConfigChannel() ), ADC must be
  *         reinitialized using HAL_ADC_Init() or HAL_ADC_ReInit().
  * @note   In case of DMA configured in circular mode, function
  *         HAL_ADC_Stop_DMA must be called after this function with handle of
  *         ADC slave, to properly disable the DMA channel of ADC slave.
  * @param  hadc ADC handle of ADC master (handle of ADC slave must not be used)
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_MultiModeStop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tickstart;
  ADC_HandleTypeDef tmphadcSlave;

  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* 1. Stop potential multimode conversion on going, on regular and          */
  /*    injected groups.                                                      */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_INJECTED_GROUP);

  /* Disable ADC peripheral if conversions are effectively stopped */
  if (tmp_hal_status == HAL_OK)
  {
    /* Set a temporary handle of the ADC slave associated to the ADC master   */
    /* (Depending on STM32F3 product, there may be up to 2 ADC slaves)        */
    ADC_MULTI_SLAVE(hadc, &tmphadcSlave);

    if (tmphadcSlave.Instance == NULL)
    {
      /* Update ADC state machine (ADC master) to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);

      /* Process unlocked */
      __HAL_UNLOCK(hadc);

      return HAL_ERROR;
    }

    /* Procedure to disable the ADC peripheral: wait for conversions          */
    /* effectively stopped (ADC master and ADC slave), then disable ADC       */

    /* 1. Wait until ADSTP=0 for ADC master and ADC slave */
    tickstart = HAL_GetTick();

    while(ADC_IS_CONVERSION_ONGOING_REGULAR(hadc)          ||
          ADC_IS_CONVERSION_ONGOING_REGULAR(&tmphadcSlave)   )
    {
      if((HAL_GetTick() - tickstart) > ADC_STOP_CONVERSION_TIMEOUT)
      {
        /* Update ADC state machine (ADC master) to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }

    /* Disable the DMA channel (in case of DMA in circular mode or stop while */
    /* while DMA transfer is on going)                                        */
    /* Note: In case of ADC slave using its own DMA channel (multimode        */
    /*       parameter "DMAAccessMode" set to disabled):                      */
    /*       DMA channel of ADC slave should stopped after this function with */
    /*       function HAL_ADC_Stop_DMA.                                       */
    tmp_hal_status = HAL_DMA_Abort(hadc->DMA_Handle);

    /* Check if DMA channel effectively disabled */
    if (tmp_hal_status != HAL_OK)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);
    }

    /* Disable ADC overrun interrupt */
    __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);



    /* 2. Disable the ADC peripherals: master and slave */
    /* Update "tmp_hal_status" only if DMA channel disabling passed,          */
    /* to retain a potential failing status.                                  */
    if (tmp_hal_status == HAL_OK)
    {
      /* Check if ADC are effectively disabled */
      if ((ADC_Disable(hadc) != HAL_ERROR)          &&
          (ADC_Disable(&tmphadcSlave) != HAL_ERROR)   )
      {
        tmp_hal_status = HAL_OK;

        /* Change ADC state (ADC master) */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                          HAL_ADC_STATE_READY);
      }
    }
    else
    {
      /* In case of error, attempt to disable ADC instances anyway */
      ADC_Disable(hadc);
      ADC_Disable(&tmphadcSlave);

      /* Update ADC state machine (ADC master) to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
    }

  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Returns the last ADC Master&Slave regular conversions results data
  *         in the selected multi mode.
  * @note   Reading register CDR does not clear flag ADC flag EOC
  *         (ADC group regular end of unitary conversion),
  *         as it is the case for independent mode data register.
  * @param  hadc ADC handle of ADC master (handle of ADC slave must not be used)
  * @retval The converted data value.
  */
uint32_t HAL_ADCEx_MultiModeGetValue(ADC_HandleTypeDef* hadc)
{
  ADC_Common_TypeDef *tmpADC_Common;

  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));

  /* Pointer to the common control register to which is belonging hadc        */
  /* (Depending on STM32F3 product, there may be up to 4 ADC and 2 common     */
  /* control registers)                                                       */
  tmpADC_Common = ADC_COMMON_REGISTER(hadc);

  /* Return the multi mode conversion value */
  return tmpADC_Common->CDR;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Get ADC injected group conversion result.
  * @note   Reading register JDRx automatically clears ADC flag JEOC
  *         (ADC group injected end of unitary conversion).
  * @note   This function does not clear ADC flag JEOS
  *         (ADC group injected end of sequence conversion)
  *         Occurrence of flag JEOS rising:
  *          - If sequencer is composed of 1 rank, flag JEOS is equivalent
  *            to flag JEOC.
  *          - If sequencer is composed of several ranks, during the scan
  *            sequence flag JEOC only is raised, at the end of the scan sequence
  *            both flags JEOC and EOS are raised.
  *         Flag JEOS must not be cleared by this function because
  *         it would not be compliant with low power features
  *         (feature low power auto-wait, not available on all STM32 families).
  *         To clear this flag, either use function:
  *         in programming model IT: @ref HAL_ADC_IRQHandler(), in programming
  *         model polling: @ref HAL_ADCEx_InjectedPollForConversion()
  *         or @ref __HAL_ADC_CLEAR_FLAG(&hadc, ADC_FLAG_JEOS).
  * @param  hadc ADC handle
  * @param  InjectedRank the converted ADC injected rank.
  *          This parameter can be one of the following values:
  *            @arg ADC_INJECTED_RANK_1: Injected Channel1 selected
  *            @arg ADC_INJECTED_RANK_2: Injected Channel2 selected
  *            @arg ADC_INJECTED_RANK_3: Injected Channel3 selected
  *            @arg ADC_INJECTED_RANK_4: Injected Channel4 selected
  * @retval ADC group injected conversion data
  */
uint32_t HAL_ADCEx_InjectedGetValue(ADC_HandleTypeDef* hadc, uint32_t InjectedRank)
{
  uint32_t tmp_jdr = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_INJECTED_RANK(InjectedRank));

  /* Note: ADC flag JEOC is not cleared here by software because              */
  /*       automatically cleared by hardware when reading register JDRx.      */

  /* Get ADC converted value */
  switch(InjectedRank)
  {
    case ADC_INJECTED_RANK_4:
      tmp_jdr = hadc->Instance->JDR4;
      break;
    case ADC_INJECTED_RANK_3:
      tmp_jdr = hadc->Instance->JDR3;
      break;
    case ADC_INJECTED_RANK_2:
      tmp_jdr = hadc->Instance->JDR2;
      break;
    case ADC_INJECTED_RANK_1:
    default:
      tmp_jdr = hadc->Instance->JDR1;
      break;
  }

  /* Return ADC converted value */
  return tmp_jdr;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Get ADC injected group conversion result.
  * @note   Reading register JDRx automatically clears ADC flag JEOC
  *         (ADC group injected end of unitary conversion).
  * @note   This function does not clear ADC flag JEOS
  *         (ADC group injected end of sequence conversion)
  *         Occurrence of flag JEOS rising:
  *          - If sequencer is composed of 1 rank, flag JEOS is equivalent
  *            to flag JEOC.
  *          - If sequencer is composed of several ranks, during the scan
  *            sequence flag JEOC only is raised, at the end of the scan sequence
  *            both flags JEOC and EOS are raised.
  *         Flag JEOS must not be cleared by this function because
  *         it would not be compliant with low power features
  *         (feature low power auto-wait, not available on all STM32 families).
  *         To clear this flag, either use function:
  *         in programming model IT: @ref HAL_ADC_IRQHandler(), in programming
  *         model polling: @ref HAL_ADCEx_InjectedPollForConversion()
  *         or @ref __HAL_ADC_CLEAR_FLAG(&hadc, ADC_FLAG_JEOS).
  * @param  hadc ADC handle
  * @param  InjectedRank the converted ADC injected rank.
  *          This parameter can be one of the following values:
  *            @arg ADC_INJECTED_RANK_1: Injected Channel1 selected
  *            @arg ADC_INJECTED_RANK_2: Injected Channel2 selected
  *            @arg ADC_INJECTED_RANK_3: Injected Channel3 selected
  *            @arg ADC_INJECTED_RANK_4: Injected Channel4 selected
  * @retval ADC group injected conversion data
  */
uint32_t HAL_ADCEx_InjectedGetValue(ADC_HandleTypeDef* hadc, uint32_t InjectedRank)
{
  uint32_t tmp_jdr = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_INJECTED_RANK(InjectedRank));

  /* Get ADC converted value */
  switch(InjectedRank)
  {
    case ADC_INJECTED_RANK_4:
      tmp_jdr = hadc->Instance->JDR4;
      break;
    case ADC_INJECTED_RANK_3:
      tmp_jdr = hadc->Instance->JDR3;
      break;
    case ADC_INJECTED_RANK_2:
      tmp_jdr = hadc->Instance->JDR2;
      break;
    case ADC_INJECTED_RANK_1:
    default:
      tmp_jdr = hadc->Instance->JDR1;
      break;
  }

  /* Return ADC converted value */
  return tmp_jdr;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Stop ADC group regular conversion (potential conversion on going
  *         on ADC group injected is not impacted), disable ADC peripheral
  *         if no conversion is on going on group injected.
  * @note   To stop ADC conversion of both groups regular and injected and to
  *         to disable ADC peripheral, instead of using 2 functions
  *         @ref HAL_ADCEx_RegularStop() and @ref HAL_ADCEx_InjectedStop(),
  *         use function @ref HAL_ADC_Stop().
  * @note   In case of auto-injection mode, this function also stop conversion
  *         on ADC group injected.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_RegularStop(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential ADC conversion on going and disable ADC peripheral        */
  /* conditioned to:                                                          */
  /* - For ADC regular group conversion stop:                                 */
  /*   On this STM32 family, conversion on the other group                    */
  /*   (group injected) can continue (groups regular and injected             */
  /*   conversion stop commands are independent)                              */
  /* - For ADC disable:                                                       */
  /*   No conversion on the other group (group injected) must be intended to  */
  /*   continue (groups regular and injected are both impacted by             */
  /*   ADC disable)                                                           */

  /* 1. Stop potential conversion on going, on regular group only */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);

  /* Disable ADC peripheral if conversion on ADC group regular is             */
  /* effectively stopped and if no conversion on the other group              */
  /* (ADC group injected) is intended to continue.                            */
  if((ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET) &&
     ((hadc->State & HAL_ADC_STATE_INJ_BUSY) == RESET)     )
  {
    /* 2. Disable the ADC peripheral */
    tmp_hal_status = ADC_Disable(hadc);

    /* Check if ADC is effectively disabled */
    if (tmp_hal_status == HAL_OK)
    {
      /* Set ADC state */
      ADC_STATE_CLR_SET(hadc->State,
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }
  /* Conversion on ADC group regular group is stopped, but ADC is not         */
  /* disabled since conversion on ADC group injected is still on going.       */
  else
  {
    /* Set ADC state */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Stop ADC group regular conversion (potential conversion on going
  *         on ADC group injected is not impacted), disable ADC peripheral
  *         if no conversion is on going on group injected.
  *         Interruptions disabled in this function:
  *          - EOC (end of conversion of regular group) and EOS (end of
  *            sequence of regular group)
  *          - overrun
  * @note   To stop ADC conversion of both groups regular and injected and to
  *         to disable ADC peripheral, instead of using 2 functions
  *         @ref HAL_ADCEx_RegularStop() and @ref HAL_ADCEx_InjectedStop(),
  *         use function @ref HAL_ADC_Stop().
  * @note   In case of auto-injection mode, this function also stop conversion
  *         on ADC group injected.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_RegularStop_IT(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential ADC conversion on going and disable ADC peripheral        */
  /* conditioned to:                                                          */
  /* - For ADC regular group conversion stop:                                 */
  /*   On this STM32 family, conversion on the other group                    */
  /*   (group injected) can continue (groups regular and injected             */
  /*   conversion stop commands are independent)                              */
  /* - For ADC disable:                                                       */
  /*   No conversion on the other group (group injected) must be intended to  */
  /*   continue (groups regular and injected are both impacted by             */
  /*   ADC disable)                                                           */

  /* 1. Stop potential conversion on going, on regular group only */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);

  /* Disable ADC peripheral if conversion on ADC group regular is             */
  /* effectively stopped and if no conversion on the other group              */
  /* (ADC group injected) is intended to continue.                            */
  if((ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET) &&
     ((hadc->State & HAL_ADC_STATE_INJ_BUSY) == RESET)     )
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
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }
  /* Conversion on ADC group regular group is stopped, but ADC is not         */
  /* disabled since conversion on ADC group injected is still on going.       */
  else
  {
    /* Set ADC state */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}

/**
  * @brief  Stop ADC group regular conversion (potential conversion on going
  *         on ADC group injected is not impacted),
  *         disable ADC DMA transfer, disable ADC peripheral
  *         if no conversion is on going on group injected.
  *         Interruptions disabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun
  * @note   To stop ADC conversion of both groups regular and injected and to
  *         to disable ADC peripheral, instead of using 2 functions
  *         @ref HAL_ADCEx_RegularStop() and @ref HAL_ADCEx_InjectedStop(),
  *         use function @ref HAL_ADC_Stop().
  * @note   Case of multimode enabled (for devices with several ADCs): This
  *         function is for single-ADC mode only. For multimode, use the
  *         dedicated MultimodeStop function.
  * @param  hadc ADC handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_ADCEx_RegularStop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential ADC conversion on going and disable ADC peripheral        */
  /* conditioned to:                                                          */
  /* - For ADC regular group conversion stop:                                 */
  /*   On this STM32 family, conversion on the other group                    */
  /*   (group injected) can continue (groups regular and injected             */
  /*   conversion stop commands are independent)                              */
  /* - For ADC disable:                                                       */
  /*   No conversion on the other group (group injected) must be intended to  */
  /*   continue (groups regular and injected are both impacted by             */
  /*   ADC disable)                                                           */

  /* 1. Stop potential conversion on going, on regular group only */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);

  /* Disable ADC peripheral if conversion on ADC group regular is             */
  /* effectively stopped and if no conversion on the other group              */
  /* (ADC group injected) is intended to continue.                            */
  if((ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET) &&
     ((hadc->State & HAL_ADC_STATE_INJ_BUSY) == RESET)     )
  {
    /* Disable ADC DMA (ADC DMA configuration ADC_CFGR_DMACFG is kept) */
    CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_DMAEN);

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
    /* Update "tmp_hal_status" only if DMA channel disabling passed,          */
    /* to retain a potential failing status.                                  */
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
                        HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                        HAL_ADC_STATE_READY);
    }
  }
  /* Conversion on ADC group regular group is stopped, but ADC is not         */
  /* disabled since conversion on ADC group injected is still on going.       */
  else
  {
    /* Set ADC state */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/**
  * @brief  With ADC configured in multimode, for ADC master:
  *         Stop ADC group regular conversion (potential conversion on going
  *         on ADC group injected is not impacted),
  *         disable ADC DMA transfer, disable ADC peripheral
  *         if no conversion is on going on group injected.
  *         Interruptions disabled in this function:
  *          - DMA transfer complete
  *          - DMA half transfer
  *          - overrun
  * @note   To stop ADC conversion of both groups regular and injected and to
  *         to disable ADC peripheral, instead of using 2 functions
  *         @ref HAL_ADCEx_RegularMultiModeStop_DMA() and
  *         @ref HAL_ADCEx_InjectedStop(), use function
  *         @ref HAL_ADCEx_MultiModeStop_DMA.
  * @note   In case of auto-injection mode, this function also stop conversion
  *         on ADC group injected.
  * @note   Multimode is kept enabled after this function. To disable multimode
  *         (set with HAL_ADCEx_MultiModeConfigChannel() ), ADC must be
  *         reinitialized using HAL_ADC_Init() or HAL_ADC_ReInit().
  * @note   In case of DMA configured in circular mode, function
  *         HAL_ADC_Stop_DMA must be called after this function with handle of
  *         ADC slave, to properly disable the DMA channel of ADC slave.
  * @param  hadc ADC handle of ADC master (handle of ADC slave must not be used)
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_RegularMultiModeStop_DMA(ADC_HandleTypeDef* hadc)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  uint32_t tickstart;
  ADC_HandleTypeDef tmphadcSlave;

  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Stop potential ADC conversion on going and disable ADC peripheral        */
  /* conditioned to:                                                          */
  /* - For ADC regular group conversion stop:                                 */
  /*   On this STM32 family, conversion on the other group                    */
  /*   (group injected) can continue (groups regular and injected             */
  /*   conversion stop commands are independent)                              */
  /* - For ADC disable:                                                       */
  /*   No conversion on the other group (group injected) must be intended to  */
  /*   continue (groups regular and injected are both impacted by             */
  /*   ADC disable)                                                           */

  /* 1. Stop potential conversion on going, on regular group only */
  tmp_hal_status = ADC_ConversionStop(hadc, ADC_REGULAR_GROUP);

  /* Disable ADC peripheral if conversion on ADC group regular is             */
  /* effectively stopped and if no conversion on the other group              */
  /* (ADC group injected) is intended to continue.                            */
  if((ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET) &&
     ((hadc->State & HAL_ADC_STATE_INJ_BUSY) == RESET)     )
  {
    /* Set a temporary handle of the ADC slave associated to the ADC master   */
    /* (Depending on STM32F3 product, there may be up to 2 ADC slaves)        */
    ADC_MULTI_SLAVE(hadc, &tmphadcSlave);

    if (tmphadcSlave.Instance == NULL)
    {
      /* Update ADC state machine (ADC master) to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);

      /* Process unlocked */
      __HAL_UNLOCK(hadc);

      return HAL_ERROR;
    }

    /* Procedure to disable the ADC peripheral: wait for conversions          */
    /* effectively stopped (ADC master and ADC slave), then disable ADC       */

    /* 1. Wait until ADSTP=0 for ADC master and ADC slave*/
    tickstart = HAL_GetTick();

    while(ADC_IS_CONVERSION_ONGOING_REGULAR(hadc)          ||
          ADC_IS_CONVERSION_ONGOING_REGULAR(&tmphadcSlave)   )
    {
      if((HAL_GetTick() - tickstart) > ADC_STOP_CONVERSION_TIMEOUT)
      {
        /* Update ADC state machine (ADC master) to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }

    /* Disable the DMA channel (in case of DMA in circular mode or stop while */
    /* while DMA transfer is on going)                                        */
    /* Note: In case of ADC slave using its own DMA channel (multimode        */
    /*       parameter "DMAAccessMode" set to disabled):                      */
    /*       DMA channel of ADC slave should stopped after this function with */
    /*       function HAL_ADC_Stop_DMA.                                       */
    tmp_hal_status = HAL_DMA_Abort(hadc->DMA_Handle);

    /* Check if DMA channel effectively disabled */
    if (tmp_hal_status != HAL_OK)
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_DMA);
    }

    /* Disable ADC overrun interrupt */
    __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);



    /* 2. Disable the ADC peripherals: master and slave */
    /* Update "tmp_hal_status" only if DMA channel disabling passed,          */
    /* to retain a potential failing status.                                  */
    if (tmp_hal_status == HAL_OK)
    {
      /* Check if ADC are effectively disabled */
      if ((ADC_Disable(hadc) != HAL_ERROR)          &&
          (ADC_Disable(&tmphadcSlave) != HAL_ERROR)   )
      {
        tmp_hal_status = HAL_OK;

        /* Change ADC state (ADC master) */
        ADC_STATE_CLR_SET(hadc->State,
                          HAL_ADC_STATE_REG_BUSY | HAL_ADC_STATE_INJ_BUSY,
                          HAL_ADC_STATE_READY);
      }
    }
    else
    {
      /* In case of error, attempt to disable ADC instances anyway */
      ADC_Disable(hadc);
      ADC_Disable(&tmphadcSlave);

      /* Update ADC state machine (ADC master) to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);
    }

  }
  /* Conversion on ADC group regular group is stopped, but ADC is not         */
  /* disabled since conversion on ADC group injected is still on going.       */
  else
  {
    /* Set ADC state */
    CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx    */


#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

/**
  * @brief  Injected conversion complete callback in non blocking mode
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_ADCEx_InjectedConvCpltCallback could be implemented in the user file
  */
}

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Injected context queue overflow flag callback.
  * @note   This callback is called if injected context queue is enabled
            (parameter "QueueInjectedContext" in injected channel configuration)
            and if a new injected context is set when queue is full (maximum 2
            contexts).
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_InjectedQueueOverflowCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADCEx_InjectedQueueOverflowCallback must be implemented
            in the user file.
  */
}

/**
  * @brief  Analog watchdog 2 callback in non blocking mode.
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_LevelOutOfWindow2Callback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_LevelOoutOfWindow2Callback must be implemented in the user file.
  */
}

/**
  * @brief  Analog watchdog 3 callback in non blocking mode.
  * @param  hadc ADC handle
  * @retval None
  */
__weak void HAL_ADCEx_LevelOutOfWindow3Callback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);

  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_ADC_LevelOoutOfWindow3Callback must be implemented in the user file.
  */
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

/**
  * @}
  */

/** @defgroup ADCEx_Exported_Functions_Group3 ADCEx Peripheral Control functions
  * @brief    ADC Extended Peripheral Control functions
  *
@verbatim
 ===============================================================================
             ##### Peripheral Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure channels on regular group
      (+) Configure channels on injected group
      (+) Configure multimode
      (+) Configure the analog watchdog

@endverbatim
  * @{
  */


#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
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
  * @param  sConfig Structure ADC channel for regular group.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  ADC_Common_TypeDef *tmpADC_Common;
  ADC_HandleTypeDef tmphadcSharingSameCommonRegister;
  uint32_t tmpOffsetShifted;
  __IO uint32_t wait_loop_index = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_REGULAR_RANK(sConfig->Rank));
  assert_param(IS_ADC_SAMPLE_TIME(sConfig->SamplingTime));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(sConfig->SingleDiff));
  assert_param(IS_ADC_OFFSET_NUMBER(sConfig->OffsetNumber));
  assert_param(IS_ADC_RANGE(ADC_GET_RESOLUTION(hadc), sConfig->Offset));


  /* Verification of channel number: Channels 1 to 14 are available in        */
  /* differential mode. Channels 15U, 16U, 17U, 18 can be used only in           */
  /* single-ended mode.                                                       */
  if (sConfig->SingleDiff != ADC_DIFFERENTIAL_ENDED)
  {
    assert_param(IS_ADC_CHANNEL(sConfig->Channel));
  }
  else
  {
    assert_param(IS_ADC_DIFF_CHANNEL(sConfig->Channel));
  }

  /* Process locked */
  __HAL_LOCK(hadc);


  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular group:                                    */
  /*  - Channel number                                                        */
  /*  - Channel rank                                                          */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
  {
    /* Regular sequence configuration */
    /* For Rank 1 to 4U */
    if (sConfig->Rank < 5U)
    {
      MODIFY_REG(hadc->Instance->SQR1,
                 ADC_SQR1_RK(ADC_SQR2_SQ5, sConfig->Rank)    ,
                 ADC_SQR1_RK(sConfig->Channel, sConfig->Rank) );
    }
    /* For Rank 5 to 9U */
    else if (sConfig->Rank < 10U)
    {
      MODIFY_REG(hadc->Instance->SQR2,
                 ADC_SQR2_RK(ADC_SQR2_SQ5, sConfig->Rank)    ,
                 ADC_SQR2_RK(sConfig->Channel, sConfig->Rank) );
    }
    /* For Rank 10 to 14U */
    else if (sConfig->Rank < 15U)
    {
      MODIFY_REG(hadc->Instance->SQR3                        ,
                 ADC_SQR3_RK(ADC_SQR3_SQ10, sConfig->Rank)   ,
                 ADC_SQR3_RK(sConfig->Channel, sConfig->Rank) );
    }
    /* For Rank 15 to 16U */
    else
    {
      MODIFY_REG(hadc->Instance->SQR4                        ,
                 ADC_SQR4_RK(ADC_SQR4_SQ15, sConfig->Rank)   ,
                 ADC_SQR4_RK(sConfig->Channel, sConfig->Rank) );
    }


  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular group:                                    */
  /*  - Channel sampling time                                                 */
  /*  - Channel offset                                                        */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)
  {
    /* Channel sampling time configuration */
    /* For channels 10 to 18U */
    if (sConfig->Channel >= ADC_CHANNEL_10)
    {
      MODIFY_REG(hadc->Instance->SMPR2                             ,
                 ADC_SMPR2(ADC_SMPR2_SMP10, sConfig->Channel)      ,
                 ADC_SMPR2(sConfig->SamplingTime, sConfig->Channel) );
    }
    else /* For channels 1 to 9U */
    {
      MODIFY_REG(hadc->Instance->SMPR1                             ,
                 ADC_SMPR1(ADC_SMPR1_SMP0, sConfig->Channel)       ,
                 ADC_SMPR1(sConfig->SamplingTime, sConfig->Channel) );
    }


    /* Configure the offset: offset enable/disable, channel, offset value */

    /* Shift the offset in function of the selected ADC resolution. */
    /* Offset has to be left-aligned on bit 11U, the LSB (right bits) are set  */
    /* to 0.                                                                  */
    tmpOffsetShifted = ADC_OFFSET_SHIFT_RESOLUTION(hadc, sConfig->Offset);

    /* Configure the selected offset register:                                */
    /* - Enable offset                                                        */
    /* - Set channel number                                                   */
    /* - Set offset value                                                     */
    switch (sConfig->OffsetNumber)
    {
    case ADC_OFFSET_1:
      /* Configure offset register 1U */
      MODIFY_REG(hadc->Instance->OFR1               ,
                 ADC_OFR1_OFFSET1_CH |
                 ADC_OFR1_OFFSET1                   ,
                 ADC_OFR1_OFFSET1_EN               |
                 ADC_OFR_CHANNEL(sConfig->Channel) |
                 tmpOffsetShifted                    );
      break;

    case ADC_OFFSET_2:
      /* Configure offset register 2U */
      MODIFY_REG(hadc->Instance->OFR2               ,
                 ADC_OFR2_OFFSET2_CH |
                 ADC_OFR2_OFFSET2                   ,
                 ADC_OFR2_OFFSET2_EN               |
                 ADC_OFR_CHANNEL(sConfig->Channel) |
                 tmpOffsetShifted                    );
      break;

    case ADC_OFFSET_3:
      /* Configure offset register 3U */
      MODIFY_REG(hadc->Instance->OFR3               ,
                 ADC_OFR3_OFFSET3_CH |
                 ADC_OFR3_OFFSET3                   ,
                 ADC_OFR3_OFFSET3_EN               |
                 ADC_OFR_CHANNEL(sConfig->Channel) |
                 tmpOffsetShifted                    );
      break;

    case ADC_OFFSET_4:
      /* Configure offset register 4U */
      MODIFY_REG(hadc->Instance->OFR4               ,
                 ADC_OFR4_OFFSET4_CH |
                 ADC_OFR4_OFFSET4                   ,
                 ADC_OFR4_OFFSET4_EN               |
                 ADC_OFR_CHANNEL(sConfig->Channel) |
                 tmpOffsetShifted                    );
      break;

    /* Case ADC_OFFSET_NONE */
    default :
    /* Scan OFR1, OFR2, OFR3, OFR4 to check if the selected channel is        */
    /* enabled. If this is the case, offset OFRx is disabled.                 */
      if (((hadc->Instance->OFR1) & ADC_OFR1_OFFSET1_CH) == ADC_OFR_CHANNEL(sConfig->Channel))
      {
        /* Disable offset OFR1*/
        CLEAR_BIT(hadc->Instance->OFR1, ADC_OFR1_OFFSET1_EN);
      }
      if (((hadc->Instance->OFR2) & ADC_OFR2_OFFSET2_CH) == ADC_OFR_CHANNEL(sConfig->Channel))
      {
        /* Disable offset OFR2*/
        CLEAR_BIT(hadc->Instance->OFR2, ADC_OFR2_OFFSET2_EN);
      }
      if (((hadc->Instance->OFR3) & ADC_OFR3_OFFSET3_CH) == ADC_OFR_CHANNEL(sConfig->Channel))
      {
        /* Disable offset OFR3*/
        CLEAR_BIT(hadc->Instance->OFR3, ADC_OFR3_OFFSET3_EN);
      }
      if (((hadc->Instance->OFR4) & ADC_OFR4_OFFSET4_CH) == ADC_OFR_CHANNEL(sConfig->Channel))
      {
        /* Disable offset OFR4*/
        CLEAR_BIT(hadc->Instance->OFR4, ADC_OFR4_OFFSET4_EN);
      }
      break;
    }

  }


  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated only when ADC is disabled:                */
  /*  - Single or differential mode                                           */
  /*  - Internal measurement channels: Vbat/VrefInt/TempSensor                */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    /* Configuration of differential mode */
    if (sConfig->SingleDiff != ADC_DIFFERENTIAL_ENDED)
    {
      /* Disable differential mode (default mode: single-ended) */
      CLEAR_BIT(hadc->Instance->DIFSEL, ADC_DIFSEL_CHANNEL(sConfig->Channel));
    }
    else
    {
      /* Enable differential mode */
      SET_BIT(hadc->Instance->DIFSEL, ADC_DIFSEL_CHANNEL(sConfig->Channel));

      /* Channel sampling time configuration (channel ADC_INx +1              */
      /* corresponding to differential negative input).                       */
      /* For channels 10 to 18U */
      if (sConfig->Channel >= ADC_CHANNEL_10)
      {
        MODIFY_REG(hadc->Instance->SMPR2,
                   ADC_SMPR2(ADC_SMPR2_SMP10, sConfig->Channel +1U)      ,
                   ADC_SMPR2(sConfig->SamplingTime, sConfig->Channel +1U) );
      }
      else /* For channels 1 to 9U */
      {
        MODIFY_REG(hadc->Instance->SMPR1,
                   ADC_SMPR1(ADC_SMPR1_SMP0, sConfig->Channel +1U)       ,
                   ADC_SMPR1(sConfig->SamplingTime, sConfig->Channel +1U) );
      }
    }


    /* Management of internal measurement channels: VrefInt/TempSensor/Vbat   */
    /* internal measurement paths enable: If internal channel selected,       */
    /* enable dedicated internal buffers and path.                            */
    /* Note: these internal measurement paths can be disabled using           */
    /* HAL_ADC_DeInit().                                                      */

    /* Configuration of common ADC parameters                                 */
    /* Pointer to the common control register to which is belonging hadc      */
    /* (Depending on STM32F3 product, there may be up to 4 ADC and 2 common   */
    /* control registers)                                                     */
    tmpADC_Common = ADC_COMMON_REGISTER(hadc);

    /* If the requested internal measurement path has already been enabled,   */
    /* bypass the configuration processing.                                   */
    if (( (sConfig->Channel == ADC_CHANNEL_TEMPSENSOR) &&
          (HAL_IS_BIT_CLR(tmpADC_Common->CCR, ADC_CCR_TSEN))            ) ||
        ( (sConfig->Channel == ADC_CHANNEL_VBAT)       &&
          (HAL_IS_BIT_CLR(tmpADC_Common->CCR, ADC_CCR_VBATEN))          ) ||
        ( (sConfig->Channel == ADC_CHANNEL_VREFINT)    &&
          (HAL_IS_BIT_CLR(tmpADC_Common->CCR, ADC_CCR_VREFEN)))
       )
    {
      /* Configuration of common ADC parameters (continuation)                */
      /* Set handle of the other ADC sharing the same common register         */
      ADC_COMMON_ADC_OTHER(hadc, &tmphadcSharingSameCommonRegister);

      /* Software is allowed to change common parameters only when all ADCs   */
      /* of the common group are disabled.                                    */
      if ((ADC_IS_ENABLE(hadc) == RESET)                                    &&
          ( (tmphadcSharingSameCommonRegister.Instance == NULL)         ||
            (ADC_IS_ENABLE(&tmphadcSharingSameCommonRegister) == RESET)   )   )
      {
        /* If Channel_16 is selected, enable Temp. sensor measurement path    */
        /* Note: Temp. sensor internal channels available on ADC1 only        */
        if ((sConfig->Channel == ADC_CHANNEL_TEMPSENSOR) && (hadc->Instance == ADC1))
        {
          SET_BIT(tmpADC_Common->CCR, ADC_CCR_TSEN);

          /* Delay for temperature sensor stabilization time */
          /* Compute number of CPU cycles to wait for */
          wait_loop_index = (ADC_TEMPSENSOR_DELAY_US * (SystemCoreClock / 1000000U));
          while(wait_loop_index != 0U)
          {
            wait_loop_index--;
          }
        }
        /* If Channel_17 is selected, enable VBAT measurement path            */
        /* Note: VBAT internal channels available on ADC1 only                */
        else if ((sConfig->Channel == ADC_CHANNEL_VBAT) && (hadc->Instance == ADC1))
        {
          SET_BIT(tmpADC_Common->CCR, ADC_CCR_VBATEN);
        }
        /* If Channel_18 is selected, enable VREFINT measurement path         */
        /* Note: VrefInt internal channels available on all ADCs, but only    */
        /*       one ADC is allowed to be connected to VrefInt at the same    */
        /*       time.                                                        */
        else if (sConfig->Channel == ADC_CHANNEL_VREFINT)
        {
          SET_BIT(tmpADC_Common->CCR, ADC_CCR_VREFEN);
        }
      }
      /* If the requested internal measurement path has already been          */
      /* enabled and other ADC of the common group are enabled, internal      */
      /* measurement paths cannot be enabled.                                 */
      else
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        tmp_hal_status = HAL_ERROR;
      }
    }

  }

  }
  /* If a conversion is on going on regular group, no update on regular       */
  /* channel could be done on neither of the channel configuration structure  */
  /* parameters.                                                              */
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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
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
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  __IO uint32_t wait_loop_index = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_CHANNEL(sConfig->Channel));
  assert_param(IS_ADC_REGULAR_RANK(sConfig->Rank));
  assert_param(IS_ADC_SAMPLE_TIME(sConfig->SamplingTime));

  /* Process locked */
  __HAL_LOCK(hadc);


  /* Regular sequence configuration */
  /* For Rank 1 to 6U */
  if (sConfig->Rank < 7U)
  {
    MODIFY_REG(hadc->Instance->SQR3                        ,
               ADC_SQR3_RK(ADC_SQR3_SQ1, sConfig->Rank)    ,
               ADC_SQR3_RK(sConfig->Channel, sConfig->Rank) );
  }
  /* For Rank 7 to 12U */
  else if (sConfig->Rank < 13U)
  {
    MODIFY_REG(hadc->Instance->SQR2                        ,
               ADC_SQR2_RK(ADC_SQR2_SQ7, sConfig->Rank)    ,
               ADC_SQR2_RK(sConfig->Channel, sConfig->Rank) );
  }
  /* For Rank 13 to 16U */
  else
  {
    MODIFY_REG(hadc->Instance->SQR1                        ,
               ADC_SQR1_RK(ADC_SQR1_SQ13, sConfig->Rank)   ,
               ADC_SQR1_RK(sConfig->Channel, sConfig->Rank) );
  }


  /* Channel sampling time configuration */
  /* For channels 10 to 18U */
  if (sConfig->Channel > ADC_CHANNEL_10)
  {
    MODIFY_REG(hadc->Instance->SMPR1                             ,
               ADC_SMPR1(ADC_SMPR1_SMP10, sConfig->Channel)      ,
               ADC_SMPR1(sConfig->SamplingTime, sConfig->Channel) );
  }
  else   /* For channels 0 to 9U */
  {
    MODIFY_REG(hadc->Instance->SMPR2                             ,
               ADC_SMPR2(ADC_SMPR2_SMP0, sConfig->Channel)       ,
               ADC_SMPR2(sConfig->SamplingTime, sConfig->Channel) );
  }

  /* If ADC1 Channel_16 or Channel_17 is selected, enable Temperature sensor  */
  /* and VREFINT measurement path.                                            */
  if ((sConfig->Channel == ADC_CHANNEL_TEMPSENSOR) ||
      (sConfig->Channel == ADC_CHANNEL_VREFINT)      )
  {
    SET_BIT(hadc->Instance->CR2, ADC_CR2_TSVREFE);

    if ((sConfig->Channel == ADC_CHANNEL_TEMPSENSOR))
    {
      /* Delay for temperature sensor stabilization time */
      /* Compute number of CPU cycles to wait for */
      wait_loop_index = (ADC_TEMPSENSOR_DELAY_US * (SystemCoreClock / 1000000U));
      while(wait_loop_index != 0U)
      {
        wait_loop_index--;
      }
    }
  }
  /* if ADC1 Channel_18 is selected, enable VBAT measurement path */
  else if (sConfig->Channel == ADC_CHANNEL_VBAT)
  {
    SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_VBAT);
  }


  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Configures the ADC injected group and the selected channel to be
  *         linked to the injected group.
  * @note   Possibility to update parameters on the fly:
  *         This function initializes injected group, following calls to this
  *         function can be used to reconfigure some parameters of structure
  *         "ADC_InjectionConfTypeDef" on the fly, without reseting the ADC.
  *         The setting of these parameters is conditioned to ADC state.
  *         For parameters constraints, see comments of structure
  *         "ADC_InjectionConfTypeDef".
  * @note   In case of usage of internal measurement channels:
  *         Vbat/VrefInt/TempSensor.
  *         The recommended sampling time is at least:
  *          - For devices STM32F37x: 17.1us for temperature sensor
  *          - For the other STM32F3 devices: 2.2us for each of channels
  *            Vbat/VrefInt/TempSensor.
  *         These internal paths can be be disabled using function
  *         HAL_ADC_DeInit().
  * @note   To reset injected sequencer, function HAL_ADCEx_InjectedStop() can
  *         be used.
  * @note   Caution: For Injected Context Queue use: a context must be fully
  * defined before start of injected conversion: all channels configured
  * consecutively for the same ADC instance. Therefore, Number of calls of
  * HAL_ADCEx_InjectedConfigChannel() must correspond to value of parameter
  * InjectedNbrOfConversion for each context.
  *  - Example 1: If 1 context intended to be used (or not use of this feature:
  *    QueueInjectedContext=DISABLE) and usage of the 3 first injected ranks
  *    (InjectedNbrOfConversion=3), HAL_ADCEx_InjectedConfigChannel() must be
  *    called once for each channel (3 times) before launching a conversion.
  *    This function must not be called to configure the 4th injected channel:
  *    it would start a new context into context queue.
  *  - Example 2: If 2 contexts intended to be used and usage of the 3 first
  *    injected ranks (InjectedNbrOfConversion=3),
  *    HAL_ADCEx_InjectedConfigChannel() must be called once for each channel and
  *    for each context (3 channels x 2 contexts = 6 calls). Conversion can
  *    start once the 1st context is set. The 2nd context can be set on the fly.
  * @param  hadc ADC handle
  * @param  sConfigInjected Structure of ADC injected group and ADC channel for
  *         injected group.
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedConfigChannel(ADC_HandleTypeDef* hadc, ADC_InjectionConfTypeDef* sConfigInjected)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  ADC_Common_TypeDef *tmpADC_Common;
  ADC_HandleTypeDef tmphadcSharingSameCommonRegister;
  uint32_t tmpOffsetShifted;
  __IO uint32_t wait_loop_index = 0U;

  /* Injected context queue feature: temporary JSQR variables defined in      */
  /* static to be passed over calls of this function                          */
  uint32_t tmp_JSQR_ContextQueueBeingBuilt = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_SAMPLE_TIME(sConfigInjected->InjectedSamplingTime));
  assert_param(IS_ADC_SINGLE_DIFFERENTIAL(sConfigInjected->InjectedSingleDiff));
  assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->AutoInjectedConv));
  assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->QueueInjectedContext));
  assert_param(IS_ADC_EXTTRIGINJEC_EDGE(sConfigInjected->ExternalTrigInjecConvEdge));
  assert_param(IS_ADC_EXTTRIGINJEC(sConfigInjected->ExternalTrigInjecConv));
  assert_param(IS_ADC_OFFSET_NUMBER(sConfigInjected->InjectedOffsetNumber));
  assert_param(IS_ADC_RANGE(ADC_GET_RESOLUTION(hadc), sConfigInjected->InjectedOffset));

  if(hadc->Init.ScanConvMode != ADC_SCAN_DISABLE)
  {
    assert_param(IS_ADC_INJECTED_RANK(sConfigInjected->InjectedRank));
    assert_param(IS_ADC_INJECTED_NB_CONV(sConfigInjected->InjectedNbrOfConversion));
    assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->InjectedDiscontinuousConvMode));
  }

  /* Verification of channel number: Channels 1 to 14 are available in        */
  /* differential mode. Channels 15U, 16U, 17U, 18 can be used only in           */
  /* single-ended mode.                                                       */
  if (sConfigInjected->InjectedSingleDiff != ADC_DIFFERENTIAL_ENDED)
  {
    assert_param(IS_ADC_CHANNEL(sConfigInjected->InjectedChannel));
  }
  else
  {
    assert_param(IS_ADC_DIFF_CHANNEL(sConfigInjected->InjectedChannel));
  }

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Configuration of Injected group sequencer.                               */
  /* Hardware constraint: Must fully define injected context register JSQR    */
  /* before make it entering into injected sequencer queue.                   */
  /*                                                                          */
  /* - if scan mode is disabled:                                              */
  /*    * Injected channels sequence length is set to 0x00: 1 channel         */
  /*      converted (channel on injected rank 1U)                              */
  /*      Parameter "InjectedNbrOfConversion" is discarded.                   */
  /*    * Injected context register JSQR setting is simple: register is fully */
  /*      defined on one call of this function (for injected rank 1U) and can  */
  /*      be entered into queue directly.                                     */
  /* - if scan mode is enabled:                                               */
  /*    * Injected channels sequence length is set to parameter               */
  /*      "InjectedNbrOfConversion".                                          */
  /*    * Injected context register JSQR setting more complex: register is    */
  /*      fully defined over successive calls of this function, for each      */
  /*      injected channel rank. It is entered into queue only when all       */
  /*      injected ranks have been set.                                       */
  /*   Note: Scan mode is not present by hardware on this device, but used    */
  /*   by software for alignment over all STM32 devices.                      */

  if ((hadc->Init.ScanConvMode == ADC_SCAN_DISABLE)  ||
      (sConfigInjected->InjectedNbrOfConversion == 1U)  )
  {
    /* Configuration of context register JSQR:                                */
    /*  - number of ranks in injected group sequencer: fixed to 1st rank      */
    /*    (scan mode disabled, only rank 1 used)                              */
    /*  - external trigger to start conversion                                */
    /*  - external trigger polarity                                           */
    /*  - channel set to rank 1 (scan mode disabled, only rank 1 used)        */

    if (sConfigInjected->InjectedRank == ADC_INJECTED_RANK_1)
    {
      /* Enable external trigger if trigger selection is different of         */
      /* software start.                                                      */
      /* Note: This configuration keeps the hardware feature of parameter     */
      /*       ExternalTrigInjecConvEdge "trigger edge none" equivalent to    */
      /*       software start.                                                */
      if (sConfigInjected->ExternalTrigInjecConv != ADC_INJECTED_SOFTWARE_START)
      {
        SET_BIT(tmp_JSQR_ContextQueueBeingBuilt, ADC_JSQR_RK(sConfigInjected->InjectedChannel, ADC_INJECTED_RANK_1) |
                                                 ADC_JSQR_JEXTSEL_SET(hadc, sConfigInjected->ExternalTrigInjecConv) |
                                                 sConfigInjected->ExternalTrigInjecConvEdge                          );
      }
      else
      {
        SET_BIT(tmp_JSQR_ContextQueueBeingBuilt, ADC_JSQR_RK(sConfigInjected->InjectedChannel, ADC_INJECTED_RANK_1) );
      }

      /* Update ADC register JSQR */
      MODIFY_REG(hadc->Instance->JSQR           ,
                 ADC_JSQR_JSQ4    |
                 ADC_JSQR_JSQ3    |
                 ADC_JSQR_JSQ2    |
                 ADC_JSQR_JSQ1    |
                 ADC_JSQR_JEXTEN  |
                 ADC_JSQR_JEXTSEL |
                 ADC_JSQR_JL                    ,
                 tmp_JSQR_ContextQueueBeingBuilt );

      /* For debug and informative reasons, hadc handle saves JSQR setting */
      hadc->InjectionConfig.ContextQueue = tmp_JSQR_ContextQueueBeingBuilt;
    }
    /* If another injected rank than rank1 was intended to be set, and could  */
    /* not due to ScanConvMode disabled, error is reported.                   */
    else
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

      tmp_hal_status = HAL_ERROR;
    }

  }
  else
  {
    /* Case of scan mode enabled, several channels to set into injected group */
    /* sequencer.                                                             */
    /* Procedure to define injected context register JSQR over successive     */
    /* calls of this function, for each injected channel rank:                */

    /* 1. Start new context and set parameters related to all injected        */
    /*    channels: injected sequence length and trigger                      */
    if (hadc->InjectionConfig.ChannelCount == 0U)
    {
      /* Initialize number of channels that will be configured on the context */
      /*  being built                                                         */
      hadc->InjectionConfig.ChannelCount = sConfigInjected->InjectedNbrOfConversion;
      /* Initialize value that will be set into register JSQR */
      hadc->InjectionConfig.ContextQueue = 0x00000000U;

      /* Configuration of context register JSQR:                              */
      /*  - number of ranks in injected group sequencer                       */
      /*  - external trigger to start conversion                              */
      /*  - external trigger polarity                                         */

      /* Enable external trigger if trigger selection is different of         */
      /* software start.                                                      */
      /* Note: This configuration keeps the hardware feature of parameter     */
      /*       ExternalTrigInjecConvEdge "trigger edge none" equivalent to    */
      /*       software start.                                                */
      if (sConfigInjected->ExternalTrigInjecConv != ADC_INJECTED_SOFTWARE_START)
      {
        SET_BIT(hadc->InjectionConfig.ContextQueue, (sConfigInjected->InjectedNbrOfConversion - 1U)           |
                                                    ADC_JSQR_JEXTSEL_SET(hadc, sConfigInjected->ExternalTrigInjecConv) |
                                                    sConfigInjected->ExternalTrigInjecConvEdge                          );
      }
      else
      {
        SET_BIT(hadc->InjectionConfig.ContextQueue, (sConfigInjected->InjectedNbrOfConversion - 1U) );
      }

    }

      /* 2. Continue setting of context under definition with parameter       */
      /*    related to each channel: channel rank sequence                    */

      /* Set the JSQx bits for the selected rank */
      MODIFY_REG(hadc->InjectionConfig.ContextQueue                                          ,
                 ADC_JSQR_RK(ADC_SQR3_SQ10, sConfigInjected->InjectedRank)                   ,
                 ADC_JSQR_RK(sConfigInjected->InjectedChannel, sConfigInjected->InjectedRank) );

      /* Decrease channel count after setting into temporary JSQR variable */
      hadc->InjectionConfig.ChannelCount --;

      /* 3. End of context setting: If last channel set, then write context   */
      /*    into register JSQR and make it enter into queue                   */
      if (hadc->InjectionConfig.ChannelCount == 0U)
      {
        /* Update ADC register JSQR */
        MODIFY_REG(hadc->Instance->JSQR              ,
                   ADC_JSQR_JSQ4    |
                   ADC_JSQR_JSQ3    |
                   ADC_JSQR_JSQ2    |
                   ADC_JSQR_JSQ1    |
                   ADC_JSQR_JEXTEN  |
                   ADC_JSQR_JEXTSEL |
                   ADC_JSQR_JL                       ,
                   hadc->InjectionConfig.ContextQueue );
      }

  }


  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on injected group:                                   */
  /*  - Injected context queue: Queue disable (active context is kept) or     */
  /*    enable (context decremented, up to 2 contexts queued)                 */
  /*  - Injected discontinuous mode: can be enabled only if auto-injected     */
  /*    mode is disabled.                                                     */
  if (ADC_IS_CONVERSION_ONGOING_INJECTED(hadc) == RESET)
  {
    /* If auto-injected mode is disabled: no constraint                       */
    if (sConfigInjected->AutoInjectedConv == DISABLE)
    {
      MODIFY_REG(hadc->Instance->CFGR                                                            ,
                 ADC_CFGR_JQM    |
                 ADC_CFGR_JDISCEN                                                                ,
                 ADC_CFGR_INJECT_CONTEXT_QUEUE(sConfigInjected->QueueInjectedContext)           |
                 ADC_CFGR_INJECT_DISCCONTINUOUS(sConfigInjected->InjectedDiscontinuousConvMode)   );
    }
    /* If auto-injected mode is enabled: Injected discontinuous setting is    */
    /* discarded.                                                             */
    else
    {
      MODIFY_REG(hadc->Instance->CFGR                                                ,
                 ADC_CFGR_JQM    |
                 ADC_CFGR_JDISCEN                                                    ,
                 ADC_CFGR_INJECT_CONTEXT_QUEUE(sConfigInjected->QueueInjectedContext) );

      /* If injected discontinuous mode was intended to be set and could not  */
      /* due to auto-injected enabled, error is reported.                     */
      if (sConfigInjected->InjectedDiscontinuousConvMode == ENABLE)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        tmp_hal_status = HAL_ERROR;
      }
    }

  }


  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular and injected groups:                      */
  /*  - Automatic injected conversion: can be enabled if injected group       */
  /*    external triggers are disabled.                                       */
  /*  - Channel sampling time                                                 */
  /*  - Channel offset                                                        */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)
  {
    /* If injected group external triggers are disabled (set to injected      */
    /* software start): no constraint                                         */
    if (sConfigInjected->ExternalTrigInjecConv == ADC_INJECTED_SOFTWARE_START)
    {
      MODIFY_REG(hadc->Instance->CFGR                                              ,
                 ADC_CFGR_JAUTO                                                    ,
                 ADC_CFGR_INJECT_AUTO_CONVERSION(sConfigInjected->AutoInjectedConv) );
    }
    /* If Automatic injected conversion was intended to be set and could not  */
    /* due to injected group external triggers enabled, error is reported.    */
    else
    {
      /* Disable Automatic injected conversion */
      CLEAR_BIT(hadc->Instance->CFGR, ADC_CFGR_JAUTO);

      if (sConfigInjected->AutoInjectedConv == ENABLE)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        tmp_hal_status = HAL_ERROR;
      }
    }


    /* Channel sampling time configuration */
    /* For channels 10 to 18U */
    if (sConfigInjected->InjectedChannel >= ADC_CHANNEL_10)
    {
      MODIFY_REG(hadc->Instance->SMPR2                                                             ,
                 ADC_SMPR2(ADC_SMPR2_SMP10, sConfigInjected->InjectedChannel)                      ,
                 ADC_SMPR2(sConfigInjected->InjectedSamplingTime, sConfigInjected->InjectedChannel) );
    }
    else /* For channels 1 to 9U */
    {
      MODIFY_REG(hadc->Instance->SMPR1                                                             ,
                 ADC_SMPR1(ADC_SMPR1_SMP0, sConfigInjected->InjectedChannel)                       ,
                 ADC_SMPR1(sConfigInjected->InjectedSamplingTime, sConfigInjected->InjectedChannel) );
    }

    /* Configure the offset: offset enable/disable, channel, offset value */

    /* Shift the offset in function of the selected ADC resolution. */
    /* Offset has to be left-aligned on bit 11U, the LSB (right bits) are set  */
    /* to 0.                                                                  */
    tmpOffsetShifted = ADC_OFFSET_SHIFT_RESOLUTION(hadc, sConfigInjected->InjectedOffset);

    /* Configure the selected offset register:                                */
    /* - Enable offset                                                        */
    /* - Set channel number                                                   */
    /* - Set offset value                                                     */
    switch (sConfigInjected->InjectedOffsetNumber)
    {
    case ADC_OFFSET_1:
      /* Configure offset register 1U */
      MODIFY_REG(hadc->Instance->OFR1                               ,
                 ADC_OFR1_OFFSET1_CH |
                 ADC_OFR1_OFFSET1                                   ,
                 ADC_OFR1_OFFSET1_EN                               |
                 ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel) |
                 tmpOffsetShifted                                    );
      break;

    case ADC_OFFSET_2:
      /* Configure offset register 2U */
      MODIFY_REG(hadc->Instance->OFR2                               ,
                 ADC_OFR2_OFFSET2_CH |
                 ADC_OFR2_OFFSET2                                   ,
                 ADC_OFR2_OFFSET2_EN                               |
                 ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel) |
                 tmpOffsetShifted                                    );
      break;

    case ADC_OFFSET_3:
      /* Configure offset register 3U */
      MODIFY_REG(hadc->Instance->OFR3                               ,
                 ADC_OFR3_OFFSET3_CH |
                 ADC_OFR3_OFFSET3                                   ,
                 ADC_OFR3_OFFSET3_EN                               |
                 ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel) |
                 tmpOffsetShifted                                    );
      break;

    case ADC_OFFSET_4:
      /* Configure offset register 4U */
      MODIFY_REG(hadc->Instance->OFR4                               ,
                 ADC_OFR4_OFFSET4_CH |
                 ADC_OFR4_OFFSET4                                   ,
                 ADC_OFR4_OFFSET4_EN                               |
                 ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel) |
                 tmpOffsetShifted                                    );
      break;

    /* Case ADC_OFFSET_NONE */
    default :
    /* Scan OFR1, OFR2, OFR3, OFR4 to check if the selected channel is        */
    /* enabled. If this is the case, offset OFRx is disabled.                 */
      if (((hadc->Instance->OFR1) & ADC_OFR1_OFFSET1_CH) == ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel))
      {
        /* Disable offset OFR1*/
        CLEAR_BIT(hadc->Instance->OFR1, ADC_OFR1_OFFSET1_EN);
      }
      if (((hadc->Instance->OFR2) & ADC_OFR2_OFFSET2_CH) == ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel))
      {
        /* Disable offset OFR2*/
        CLEAR_BIT(hadc->Instance->OFR2, ADC_OFR2_OFFSET2_EN);
      }
      if (((hadc->Instance->OFR3) & ADC_OFR3_OFFSET3_CH) == ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel))
      {
        /* Disable offset OFR3*/
        CLEAR_BIT(hadc->Instance->OFR3, ADC_OFR3_OFFSET3_EN);
      }
      if (((hadc->Instance->OFR4) & ADC_OFR4_OFFSET4_CH) == ADC_OFR_CHANNEL(sConfigInjected->InjectedChannel))
      {
        /* Disable offset OFR4*/
        CLEAR_BIT(hadc->Instance->OFR4, ADC_OFR4_OFFSET4_EN);
      }
      break;
    }

  }


  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated only when ADC is disabled:                */
  /*  - Single or differential mode                                           */
  /*  - Internal measurement channels: Vbat/VrefInt/TempSensor                */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    /* Configuration of differential mode */
    if (sConfigInjected->InjectedSingleDiff != ADC_DIFFERENTIAL_ENDED)
    {
      /* Disable differential mode (default mode: single-ended) */
      CLEAR_BIT(hadc->Instance->DIFSEL, ADC_DIFSEL_CHANNEL(sConfigInjected->InjectedChannel));
    }
    else
    {
      /* Enable differential mode */
      SET_BIT(hadc->Instance->DIFSEL, ADC_DIFSEL_CHANNEL(sConfigInjected->InjectedChannel));

      /* Channel sampling time configuration (channel ADC_INx +1              */
      /* corresponding to differential negative input).                       */
      /* For channels 10 to 18U */
      if (sConfigInjected->InjectedChannel >= ADC_CHANNEL_10)
      {
        MODIFY_REG(hadc->Instance->SMPR2,
                   ADC_SMPR2(ADC_SMPR2_SMP10, sConfigInjected->InjectedChannel +1U),
                   ADC_SMPR2(sConfigInjected->InjectedSamplingTime, sConfigInjected->InjectedChannel +1U) );
      }
      else /* For channels 1 to 9U */
      {
        MODIFY_REG(hadc->Instance->SMPR1,
                   ADC_SMPR1(ADC_SMPR1_SMP0, sConfigInjected->InjectedChannel +1U),
                   ADC_SMPR1(sConfigInjected->InjectedSamplingTime, sConfigInjected->InjectedChannel +1U) );
      }
    }


    /* Management of internal measurement channels: VrefInt/TempSensor/Vbat   */
    /* internal measurement paths enable: If internal channel selected,       */
    /* enable dedicated internal buffers and path.                            */
    /* Note: these internal measurement paths can be disabled using           */
    /* HAL_ADC_deInit().                                                      */

    /* Configuration of common ADC parameters                                 */
    /* Pointer to the common control register to which is belonging hadc      */
    /* (Depending on STM32F3 product, there may be up to 4 ADC and 2 common   */
    /* control registers)                                                     */
    tmpADC_Common = ADC_COMMON_REGISTER(hadc);

    /* If the requested internal measurement path has already been enabled,   */
    /* bypass the configuration processing.                                   */
    if (( (sConfigInjected->InjectedChannel == ADC_CHANNEL_TEMPSENSOR) &&
          (HAL_IS_BIT_CLR(tmpADC_Common->CCR, ADC_CCR_TSEN))            ) ||
        ( (sConfigInjected->InjectedChannel == ADC_CHANNEL_VBAT)       &&
          (HAL_IS_BIT_CLR(tmpADC_Common->CCR, ADC_CCR_VBATEN))          ) ||
        ( (sConfigInjected->InjectedChannel == ADC_CHANNEL_VREFINT)    &&
          (HAL_IS_BIT_CLR(tmpADC_Common->CCR, ADC_CCR_VREFEN)))
       )
    {
      /* Configuration of common ADC parameters (continuation)                */
      /* Set handle of the other ADC sharing the same common register         */
      ADC_COMMON_ADC_OTHER(hadc, &tmphadcSharingSameCommonRegister);

      /* Software is allowed to change common parameters only when all ADCs   */
      /* of the common group are disabled.                                    */
      if ((ADC_IS_ENABLE(hadc) == RESET)                                    &&
          ( (tmphadcSharingSameCommonRegister.Instance == NULL)         ||
            (ADC_IS_ENABLE(&tmphadcSharingSameCommonRegister) == RESET)   )   )
      {
        /* If Channel_16 is selected, enable Temp. sensor measurement path    */
        /* Note: Temp. sensor internal channels available on ADC1 only        */
        if ((sConfigInjected->InjectedChannel == ADC_CHANNEL_TEMPSENSOR) && (hadc->Instance == ADC1))
        {
          SET_BIT(tmpADC_Common->CCR, ADC_CCR_TSEN);

          /* Delay for temperature sensor stabilization time */
          /* Compute number of CPU cycles to wait for */
          wait_loop_index = (ADC_TEMPSENSOR_DELAY_US * (SystemCoreClock / 1000000U));
          while(wait_loop_index != 0U)
          {
            wait_loop_index--;
          }
        }
        /* If Channel_17 is selected, enable VBAT measurement path            */
        /* Note: VBAT internal channels available on ADC1 only                */
        else if ((sConfigInjected->InjectedChannel == ADC_CHANNEL_VBAT) && (hadc->Instance == ADC1))
        {
          SET_BIT(tmpADC_Common->CCR, ADC_CCR_VBATEN);
        }
        /* If Channel_18 is selected, enable VREFINT measurement path         */
        /* Note: VrefInt internal channels available on all ADCs, but only    */
        /*       one ADC is allowed to be connected to VrefInt at the same    */
        /*       time.                                                        */
        else if (sConfigInjected->InjectedChannel == ADC_CHANNEL_VREFINT)
        {
          SET_BIT(tmpADC_Common->CCR, ADC_CCR_VREFEN);
        }
      }
      /* If the requested internal measurement path has already been enabled  */
      /* and other ADC of the common group are enabled, internal              */
      /* measurement paths cannot be enabled.                                 */
      else
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        tmp_hal_status = HAL_ERROR;
      }
    }

  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Configures the ADC injected group and the selected channel to be
  *         linked to the injected group.
  * @note   Possibility to update parameters on the fly:
  *         This function initializes injected group, following calls to this
  *         function can be used to reconfigure some parameters of structure
  *         "ADC_InjectionConfTypeDef" on the fly, without reseting the ADC.
  *         The setting of these parameters is conditioned to ADC state:
  *         this function must be called when ADC is not under conversion.
  * @note   In case of usage of internal measurement channels:
  *         Vbat/VrefInt/TempSensor.
  *         The recommended sampling time is at least:
  *          - For devices STM32F37x: 17.1us for temperature sensor
  *          - For the other STM32F3 devices: 2.2us for each of channels
  *            Vbat/VrefInt/TempSensor.
  *         These internal paths can be be disabled using function
  *         HAL_ADC_DeInit().
  * @param  hadc ADC handle
  * @param  sConfigInjected Structure of ADC injected group and ADC channel for
  *         injected group.
  * @retval None
  */
HAL_StatusTypeDef HAL_ADCEx_InjectedConfigChannel(ADC_HandleTypeDef* hadc, ADC_InjectionConfTypeDef* sConfigInjected)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  __IO uint32_t wait_loop_index = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_CHANNEL(sConfigInjected->InjectedChannel));
  assert_param(IS_ADC_SAMPLE_TIME(sConfigInjected->InjectedSamplingTime));
  assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->AutoInjectedConv));
  assert_param(IS_ADC_EXTTRIGINJEC(sConfigInjected->ExternalTrigInjecConv));
  assert_param(IS_ADC_RANGE(sConfigInjected->InjectedOffset));

  if(hadc->Init.ScanConvMode != ADC_SCAN_DISABLE)
  {
    assert_param(IS_ADC_INJECTED_RANK(sConfigInjected->InjectedRank));
    assert_param(IS_ADC_INJECTED_NB_CONV(sConfigInjected->InjectedNbrOfConversion));
    assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->InjectedDiscontinuousConvMode));
  }

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Configuration of injected group sequencer:                               */
  /* - if scan mode is disabled, injected channels sequence length is set to  */
  /*   0x00: 1 channel converted (channel on regular rank 1U)                  */
  /*   Parameter "InjectedNbrOfConversion" is discarded.                      */
  /*   Note: Scan mode is present by hardware on this device and, if          */
  /*   disabled, discards automatically nb of conversions. Anyway, nb of      */
  /*   conversions is forced to 0x00 for alignment over all STM32 devices.    */
  /* - if scan mode is enabled, injected channels sequence length is set to   */
  /*   parameter "InjectedNbrOfConversion".                                   */
  if (hadc->Init.ScanConvMode == ADC_SCAN_DISABLE)
  {
    if (sConfigInjected->InjectedRank == ADC_INJECTED_RANK_1)
    {
      /* Clear the old SQx bits for all injected ranks */
      MODIFY_REG(hadc->Instance->JSQR                           ,
                 ADC_JSQR_JL   |
                 ADC_JSQR_JSQ4 |
                 ADC_JSQR_JSQ3 |
                 ADC_JSQR_JSQ2 |
                 ADC_JSQR_JSQ1                                  ,
                 ADC_JSQR_RK_JL(sConfigInjected->InjectedChannel,
                                      ADC_INJECTED_RANK_1,
                                      0x01U)                      );
    }
    /* If another injected rank than rank1 was intended to be set, and could  */
    /* not due to ScanConvMode disabled, error is reported.                   */
    else
    {
      /* Update ADC state machine to error */
      SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

      tmp_hal_status = HAL_ERROR;
    }
  }
  else
  {
    /* Since injected channels rank conv. order depends on total number of   */
    /* injected conversions, selected rank must be below or equal to total   */
    /* number of injected conversions to be updated.                         */
    if (sConfigInjected->InjectedRank <= sConfigInjected->InjectedNbrOfConversion)
    {
      /* Clear the old SQx bits for the selected rank */
      /* Set the SQx bits for the selected rank */
      MODIFY_REG(hadc->Instance->JSQR                                         ,

                 ADC_JSQR_JL                                                 |
                 ADC_JSQR_RK_JL(ADC_JSQR_JSQ1,
                                sConfigInjected->InjectedRank,
                                sConfigInjected->InjectedNbrOfConversion)     ,

                 ADC_JSQR_JL_SHIFT(sConfigInjected->InjectedNbrOfConversion) |
                 ADC_JSQR_RK_JL(sConfigInjected->InjectedChannel,
                                sConfigInjected->InjectedRank,
                                sConfigInjected->InjectedNbrOfConversion)      );
    }
    else
    {
      /* Clear the old SQx bits for the selected rank */
      MODIFY_REG(hadc->Instance->JSQR                                     ,

                 ADC_JSQR_JL                                             |
                 ADC_JSQR_RK_JL(ADC_JSQR_JSQ1,
                                sConfigInjected->InjectedRank,
                                sConfigInjected->InjectedNbrOfConversion) ,

                 0x00000000                                                );
    }
  }

  /* Configuration of injected group                                          */
  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated only when ADC is disabled:                */
  /*  - external trigger to start conversion                                  */
  /* Parameters update not conditioned to ADC state:                          */
  /*  - Automatic injected conversion                                         */
  /*  - Injected discontinuous mode                                           */
  /* Note: In case of ADC already enabled, caution to not launch an unwanted  */
  /*       conversion while modifying register CR2 by writing 1 to bit ADON.  */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    MODIFY_REG(hadc->Instance->CR2                   ,
               ADC_CR2_JEXTSEL |
               ADC_CR2_ADON                          ,
               sConfigInjected->ExternalTrigInjecConv );
  }

  /* Configuration of injected group                                          */
  /*  - Automatic injected conversion                                         */
  /*  - Injected discontinuous mode                                           */

    /* Automatic injected conversion can be enabled if injected group         */
    /* external triggers are disabled.                                        */
    if (sConfigInjected->AutoInjectedConv == ENABLE)
    {
      if (sConfigInjected->ExternalTrigInjecConv == ADC_INJECTED_SOFTWARE_START)
      {
        SET_BIT(hadc->Instance->CR1, ADC_CR1_JAUTO);
      }
      else
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        tmp_hal_status = HAL_ERROR;
      }
    }

    /* Injected discontinuous can be enabled only if auto-injected mode is    */
    /* disabled.                                                              */
    if (sConfigInjected->InjectedDiscontinuousConvMode == ENABLE)
    {
      if (sConfigInjected->AutoInjectedConv == DISABLE)
      {
        SET_BIT(hadc->Instance->CR1, ADC_CR1_JDISCEN);
      }
      else
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

        tmp_hal_status = HAL_ERROR;
      }
    }


  /* InjectedChannel sampling time configuration */
  /* For channels 10 to 18 */
  if (sConfigInjected->InjectedChannel > ADC_CHANNEL_10)
  {
    MODIFY_REG(hadc->Instance->SMPR1,
               ADC_SMPR1(ADC_SMPR1_SMP10, sConfigInjected->InjectedChannel),
               ADC_SMPR1(sConfigInjected->InjectedSamplingTime, sConfigInjected->InjectedChannel) );
  }
  else /* For channels 1 to 9 */
  {
    MODIFY_REG(hadc->Instance->SMPR2,
               ADC_SMPR2(ADC_SMPR2_SMP0, sConfigInjected->InjectedChannel),
               ADC_SMPR2(sConfigInjected->InjectedSamplingTime, sConfigInjected->InjectedChannel) );
  }


  /* Configure the offset: offset enable/disable, InjectedChannel, offset value */
  switch(sConfigInjected->InjectedRank)
  {
    case 1:
      /* Set injected channel 1 offset */
      MODIFY_REG(hadc->Instance->JOFR1,
                 ADC_JOFR1_JOFFSET1,
                 sConfigInjected->InjectedOffset);
      break;
    case 2:
      /* Set injected channel 2 offset */
      MODIFY_REG(hadc->Instance->JOFR2,
                 ADC_JOFR2_JOFFSET2,
                 sConfigInjected->InjectedOffset);
      break;
    case 3:
      /* Set injected channel 3 offset */
      MODIFY_REG(hadc->Instance->JOFR3,
                 ADC_JOFR3_JOFFSET3,
                 sConfigInjected->InjectedOffset);
      break;
    case 4:
    default:
      MODIFY_REG(hadc->Instance->JOFR4,
                 ADC_JOFR4_JOFFSET4,
                 sConfigInjected->InjectedOffset);
      break;
  }

  /* If ADC1 Channel_16 or Channel_17 is selected, enable Temperature sensor  */
  /* and VREFINT measurement path.                                            */
  if ((sConfigInjected->InjectedChannel == ADC_CHANNEL_TEMPSENSOR) ||
      (sConfigInjected->InjectedChannel == ADC_CHANNEL_VREFINT)      )
  {
    if (READ_BIT(hadc->Instance->CR2, ADC_CR2_TSVREFE) == RESET)
    {
      SET_BIT(hadc->Instance->CR2, ADC_CR2_TSVREFE);

      if ((sConfigInjected->InjectedChannel == ADC_CHANNEL_TEMPSENSOR))
      {
        /* Delay for temperature sensor stabilization time */
        /* Compute number of CPU cycles to wait for */
        wait_loop_index = (ADC_TEMPSENSOR_DELAY_US * (SystemCoreClock / 1000000U));
        while(wait_loop_index != 0U)
        {
          wait_loop_index--;
        }
      }
    }
  }
  /* if ADC1 Channel_18 is selected, enable VBAT measurement path */
  else if (sConfigInjected->InjectedChannel == ADC_CHANNEL_VBAT)
  {
    SET_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_VBAT);
  }

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return tmp_hal_status;
}
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
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
HAL_StatusTypeDef HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;

  uint32_t tmpAWDHighThresholdShifted;
  uint32_t tmpAWDLowThresholdShifted;

  uint32_t tmpADCFlagAWD2orAWD3;
  uint32_t tmpADCITAWD2orAWD3;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_ANALOG_WATCHDOG_NUMBER(AnalogWDGConfig->WatchdogNumber));
  assert_param(IS_ADC_ANALOG_WATCHDOG_MODE(AnalogWDGConfig->WatchdogMode));
  assert_param(IS_FUNCTIONAL_STATE(AnalogWDGConfig->ITMode));

  /* Verify if threshold is within the selected ADC resolution */
  assert_param(IS_ADC_RANGE(ADC_GET_RESOLUTION(hadc), AnalogWDGConfig->HighThreshold));
  assert_param(IS_ADC_RANGE(ADC_GET_RESOLUTION(hadc), AnalogWDGConfig->LowThreshold));

  if((AnalogWDGConfig->WatchdogMode == ADC_ANALOGWATCHDOG_SINGLE_REG)     ||
     (AnalogWDGConfig->WatchdogMode == ADC_ANALOGWATCHDOG_SINGLE_INJEC)   ||
     (AnalogWDGConfig->WatchdogMode == ADC_ANALOGWATCHDOG_SINGLE_REGINJEC)  )
  {
    assert_param(IS_ADC_CHANNEL(AnalogWDGConfig->Channel));
  }

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular and injected groups:                      */
  /*  - Analog watchdog channels                                              */
  /*  - Analog watchdog thresholds                                            */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc) == RESET)
  {

    /* Analog watchdogs configuration */
    if(AnalogWDGConfig->WatchdogNumber == ADC_ANALOGWATCHDOG_1)
    {
      /* Configuration of analog watchdog:                                    */
      /*  - Set the analog watchdog enable mode: regular and/or injected      */
      /*    groups, one or overall group of channels.                         */
      /*  - Set the Analog watchdog channel (is not used if watchdog          */
      /*    mode "all channels": ADC_CFGR_AWD1SGL=0U).                         */
      MODIFY_REG(hadc->Instance->CFGR                             ,
                 ADC_CFGR_AWD1SGL |
                 ADC_CFGR_JAWD1EN |
                 ADC_CFGR_AWD1EN  |
                 ADC_CFGR_AWD1CH                                  ,
                 AnalogWDGConfig->WatchdogMode                   |
                 ADC_CFGR_AWD1CH_SHIFT(AnalogWDGConfig->Channel)   );

      /* Shift the offset in function of the selected ADC resolution:         */
      /* Thresholds have to be left-aligned on bit 11U, the LSB (right bits)   */
      /* are set to 0                                                         */
      tmpAWDHighThresholdShifted = ADC_AWD1THRESHOLD_SHIFT_RESOLUTION(hadc, AnalogWDGConfig->HighThreshold);
      tmpAWDLowThresholdShifted  = ADC_AWD1THRESHOLD_SHIFT_RESOLUTION(hadc, AnalogWDGConfig->LowThreshold);

      /* Set the high and low thresholds */
      MODIFY_REG(hadc->Instance->TR1                                ,
                 ADC_TR1_HT1 |
                 ADC_TR1_LT1                                        ,
                 ADC_TRX_HIGHTHRESHOLD(tmpAWDHighThresholdShifted) |
                 tmpAWDLowThresholdShifted                           );

      /* Clear the ADC Analog watchdog flag (in case of left enabled by       */
      /* previous ADC operations) to be ready to use for HAL_ADC_IRQHandler() */
      /* or HAL_ADC_PollForEvent().                                           */
      __HAL_ADC_CLEAR_FLAG(hadc, ADC_IT_AWD1);

      /* Configure ADC Analog watchdog interrupt */
      if(AnalogWDGConfig->ITMode == ENABLE)
      {
        /* Enable the ADC Analog watchdog interrupt */
        __HAL_ADC_ENABLE_IT(hadc, ADC_IT_AWD1);
      }
      else
      {
        /* Disable the ADC Analog watchdog interrupt */
        __HAL_ADC_DISABLE_IT(hadc, ADC_IT_AWD1);
      }

    }
    /* Case of ADC_ANALOGWATCHDOG_2 and ADC_ANALOGWATCHDOG_3 */
    else
    {
    /* Shift the threshold in function of the selected ADC resolution */
    /* have to be left-aligned on bit 7U, the LSB (right bits) are set to 0    */
      tmpAWDHighThresholdShifted = ADC_AWD23THRESHOLD_SHIFT_RESOLUTION(hadc, AnalogWDGConfig->HighThreshold);
      tmpAWDLowThresholdShifted  = ADC_AWD23THRESHOLD_SHIFT_RESOLUTION(hadc, AnalogWDGConfig->LowThreshold);

      if (AnalogWDGConfig->WatchdogNumber == ADC_ANALOGWATCHDOG_2)
      {
        /* Set the Analog watchdog channel or group of channels. This also    */
        /* enables the watchdog.                                              */
        /* Note: Conditional register reset, because several channels can be  */
        /*       set by successive calls of this function.                    */
        if (AnalogWDGConfig->WatchdogMode != ADC_ANALOGWATCHDOG_NONE)
        {
          /* Set the high and low thresholds */
          MODIFY_REG(hadc->Instance->TR2                                ,
                     ADC_TR2_HT2 |
                     ADC_TR2_LT2                                        ,
                     ADC_TRX_HIGHTHRESHOLD(tmpAWDHighThresholdShifted) |
                     tmpAWDLowThresholdShifted                           );

          SET_BIT(hadc->Instance->AWD2CR, ADC_CFGR_AWD23CR(AnalogWDGConfig->Channel));
        }
        else
        {
          CLEAR_BIT(hadc->Instance->TR2, ADC_TR2_HT2 | ADC_TR2_LT2);
          CLEAR_BIT(hadc->Instance->AWD2CR, ADC_AWD2CR_AWD2CH);
        }

        /* Set temporary variable to flag and IT of AWD2 or AWD3 for further  */
        /* settings.                                                          */
        tmpADCFlagAWD2orAWD3 = ADC_FLAG_AWD2;
        tmpADCITAWD2orAWD3 = ADC_IT_AWD2;
      }
      /* (AnalogWDGConfig->WatchdogNumber == ADC_ANALOGWATCHDOG_3) */
      else
      {
        /* Set the Analog watchdog channel or group of channels. This also    */
        /* enables the watchdog.                                              */
        /* Note: Conditionnal register reset, because several channels can be */
        /*       set by successive calls of this function.                    */
        if (AnalogWDGConfig->WatchdogMode != ADC_ANALOGWATCHDOG_NONE)
        {
          /* Set the high and low thresholds */
          MODIFY_REG(hadc->Instance->TR3                                ,
                     ADC_TR3_HT3 |
                     ADC_TR3_LT3                                        ,
                     ADC_TRX_HIGHTHRESHOLD(tmpAWDHighThresholdShifted) |
                     tmpAWDLowThresholdShifted                           );

          SET_BIT(hadc->Instance->AWD3CR, ADC_CFGR_AWD23CR(AnalogWDGConfig->Channel));
        }
        else
        {
          CLEAR_BIT(hadc->Instance->TR3, ADC_TR3_HT3 | ADC_TR3_LT3);
          CLEAR_BIT(hadc->Instance->AWD3CR, ADC_AWD3CR_AWD3CH);
        }

        /* Set temporary variable to flag and IT of AWD2 or AWD3 for further  */
        /* settings.                                                          */
        tmpADCFlagAWD2orAWD3 = ADC_FLAG_AWD3;
        tmpADCITAWD2orAWD3 = ADC_IT_AWD3;
      }

      /* Clear the ADC Analog watchdog flag (in case of left enabled by       */
      /* previous ADC operations) to be ready to use for HAL_ADC_IRQHandler() */
      /* or HAL_ADC_PollForEvent().                                           */
      __HAL_ADC_CLEAR_FLAG(hadc, tmpADCFlagAWD2orAWD3);

      /* Configure ADC Analog watchdog interrupt */
      if(AnalogWDGConfig->ITMode == ENABLE)
      {
        __HAL_ADC_ENABLE_IT(hadc, tmpADCITAWD2orAWD3);
      }
      else
      {
        __HAL_ADC_DISABLE_IT(hadc, tmpADCITAWD2orAWD3);
      }
    }

  }
  /* If a conversion is on going on regular or injected groups, no update     */
  /* could be done on neither of the AWD configuration structure parameters.  */
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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Configures the analog watchdog.
  * @note   Analog watchdog thresholds can be modified while ADC conversion
  *         is on going.
  *         In this case, some constraints must be taken into account:
  *         the programmed threshold values are effective from the next
  *         ADC EOC (end of unitary conversion).
  *         Considering that registers write delay may happen due to
  *         bus activity, this might cause an uncertainty on the
  *         effective timing of the new programmed threshold values.
  * @param  hadc ADC handle
  * @param  AnalogWDGConfig Structure of ADC analog watchdog configuration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig)
{
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_ANALOG_WATCHDOG_MODE(AnalogWDGConfig->WatchdogMode));
  assert_param(IS_FUNCTIONAL_STATE(AnalogWDGConfig->ITMode));
  assert_param(IS_ADC_RANGE(AnalogWDGConfig->HighThreshold));
  assert_param(IS_ADC_RANGE(AnalogWDGConfig->LowThreshold));

  if((AnalogWDGConfig->WatchdogMode == ADC_ANALOGWATCHDOG_SINGLE_REG)     ||
     (AnalogWDGConfig->WatchdogMode == ADC_ANALOGWATCHDOG_SINGLE_INJEC)   ||
     (AnalogWDGConfig->WatchdogMode == ADC_ANALOGWATCHDOG_SINGLE_REGINJEC)  )
  {
    assert_param(IS_ADC_CHANNEL(AnalogWDGConfig->Channel));
  }

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Analog watchdog configuration */

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
  /*  - Set the analog watchdog enable mode: regular and/or injected groups,  */
  /*    one or all channels.                                                  */
  /*  - Set the Analog watchdog channel (is not used if watchdog              */
  /*    mode "all channels": ADC_CFGR_AWD1SGL=0U).                             */
  MODIFY_REG(hadc->Instance->CR1            ,
             ADC_CR1_AWDSGL |
             ADC_CR1_JAWDEN |
             ADC_CR1_AWDEN  |
             ADC_CR1_AWDCH                  ,
             AnalogWDGConfig->WatchdogMode |
             AnalogWDGConfig->Channel       );

  /* Set the high threshold */
  WRITE_REG(hadc->Instance->HTR, AnalogWDGConfig->HighThreshold);

  /* Set the low threshold */
  WRITE_REG(hadc->Instance->LTR, AnalogWDGConfig->LowThreshold);

  /* Process unlocked */
  __HAL_UNLOCK(hadc);

  /* Return function status */
  return HAL_OK;
}
#endif /* STM32F373xC || STM32F378xx */


#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/**
  * @brief  Enable ADC multimode and configure multimode parameters
  * @note   Possibility to update parameters on the fly:
  *         This function initializes multimode parameters, following
  *         calls to this function can be used to reconfigure some parameters
  *         of structure "ADC_MultiModeTypeDef" on the fly, without reseting
  *         the ADCs (both ADCs of the common group).
  *         The setting of these parameters is conditioned to ADC state.
  *         For parameters constraints, see comments of structure
  *         "ADC_MultiModeTypeDef".
  * @note   To change back configuration from multimode to single mode, ADC must
  *         be reset (using function HAL_ADC_Init() ).
  * @param  hadc ADC handle
  * @param  multimode Structure of ADC multimode configuration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ADCEx_MultiModeConfigChannel(ADC_HandleTypeDef* hadc, ADC_MultiModeTypeDef* multimode)
{
  HAL_StatusTypeDef tmp_hal_status = HAL_OK;
  ADC_Common_TypeDef *tmpADC_Common;
  ADC_HandleTypeDef tmphadcSharingSameCommonRegister;

  /* Check the parameters */
  assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_MODE(multimode->Mode));
  if(multimode->Mode != ADC_MODE_INDEPENDENT)
  {
    assert_param(IS_ADC_DMA_ACCESS_MODE(multimode->DMAAccessMode));
    assert_param(IS_ADC_SAMPLING_DELAY(multimode->TwoSamplingDelay));
  }

  /* Set handle of the other ADC sharing the same common register             */
  ADC_COMMON_ADC_OTHER(hadc, &tmphadcSharingSameCommonRegister);
  if (tmphadcSharingSameCommonRegister.Instance == NULL)
  {
    /* Return function status */
    return HAL_ERROR;
  }

  /* Process locked */
  __HAL_LOCK(hadc);

  /* Parameters update conditioned to ADC state:                              */
  /* Parameters that can be updated when ADC is disabled or enabled without   */
  /* conversion on going on regular group:                                    */
  /*  - Multimode DMA configuration                                           */
  /*  - Multimode DMA mode                                                    */
  if ( (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc) == RESET)
    && (ADC_IS_CONVERSION_ONGOING_REGULAR(&tmphadcSharingSameCommonRegister) == RESET) )
  {
    /* Pointer to the common control register to which is belonging hadc      */
    /* (Depending on STM32F3 product, there may have up to 4 ADC and 2 common */
    /* control registers)                                                     */
    tmpADC_Common = ADC_COMMON_REGISTER(hadc);

    /* If multimode is selected, configure all multimode paramaters.          */
    /* Otherwise, reset multimode parameters (can be used in case of          */
    /* transition from multimode to independent mode).                        */
    if(multimode->Mode != ADC_MODE_INDEPENDENT)
    {
      /* Configuration of ADC common group ADC1&ADC2, ADC3&ADC4 if available    */
      /* (ADC2, ADC3, ADC4 availability depends on STM32 product)               */
      /*  - DMA access mode                                                     */
      MODIFY_REG(tmpADC_Common->CCR                                          ,
                 ADC_CCR_MDMA  |
                 ADC_CCR_DMACFG                                              ,
                 multimode->DMAAccessMode                                   |
                 ADC_CCR_MULTI_DMACONTREQ(hadc->Init.DMAContinuousRequests)   );

      /* Parameters that can be updated only when ADC is disabled:              */
      /*  - Multimode mode selection                                            */
      /*  - Set delay between two sampling phases                               */
      /*    Note: Delay range depends on selected resolution:                   */
      /*      from 1 to 12 clock cycles for 12 bits                             */
      /*      from 1 to 10 clock cycles for 10 bits,                            */
      /*      from 1 to 8 clock cycles for 8 bits                               */
      /*      from 1 to 6 clock cycles for 6 bits                               */
      /*    If a higher delay is selected, it will be clamped to maximum delay  */
      /*    range                                                               */
      /* Note: If ADC is not in the appropriate state to modify these           */
      /*       parameters, their setting is bypassed without error reporting    */
      /*       (as it can be the expected behaviour in case of intended action  */
      /*       to update parameter above (which fulfills the ADC state          */
      /*       condition: no conversion on going on group regular)              */
      /*       on the fly).                                                     */
      if ((ADC_IS_ENABLE(hadc) == RESET)                              &&
          (ADC_IS_ENABLE(&tmphadcSharingSameCommonRegister) == RESET)   )
      {
        MODIFY_REG(tmpADC_Common->CCR                                          ,
                   ADC_CCR_MULTI |
                   ADC_CCR_DELAY                                               ,
                   multimode->Mode                                            |
                   multimode->TwoSamplingDelay                                  );
      }
    }
    else /* ADC_MODE_INDEPENDENT */
    {
      CLEAR_BIT(tmpADC_Common->CCR, ADC_CCR_MDMA | ADC_CCR_DMACFG);

      /* Parameters that can be updated only when ADC is disabled:                */
      /*  - Multimode mode selection                                              */
      /*  - Multimode delay                                                       */
      if ((ADC_IS_ENABLE(hadc) == RESET)                              &&
          (ADC_IS_ENABLE(&tmphadcSharingSameCommonRegister) == RESET)   )
      {
        CLEAR_BIT(tmpADC_Common->CCR, ADC_CCR_MULTI | ADC_CCR_DELAY);
      }
    }
  }
  /* If one of the ADC sharing the same common group is enabled, no update    */
  /* could be done on neither of the multimode structure parameters.          */
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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F328xx || STM32F334x8    */

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup ADCEx_Private_Functions ADCEx Private Functions
  * @{
  */
/**
  * @brief  DMA transfer complete callback.
  * @param  hdma pointer to DMA handle.
  * @retval None
  */
static void ADC_DMAConvCplt(DMA_HandleTypeDef *hdma)
{
  /* Retrieve ADC handle corresponding to current DMA handle */
  ADC_HandleTypeDef* hadc = ( ADC_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  /* Update state machine on conversion status if not in error state */
  if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL | HAL_ADC_STATE_ERROR_DMA))
  {
    /* Update ADC state machine */
    SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC);

    /* Determine whether any further conversion upcoming on group regular     */
    /* by external trigger, continuous mode or scan sequence on going.        */
    /* Note: On STM32F3 devices, in case of sequencer enabled                 */
    /*       (several ranks selected), end of conversion flag is raised       */
    /*       at the end of the sequence.                                      */
    if(ADC_IS_SOFTWARE_START_REGULAR(hadc)        &&
       (hadc->Init.ContinuousConvMode == DISABLE)   )
    {
      /* Set ADC state */
      CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);

      if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_INJ_BUSY))
      {
        SET_BIT(hadc->State, HAL_ADC_STATE_READY);
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
  * @param  hdma pointer to DMA handle.
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
  * @param  hdma pointer to DMA handle.
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

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/**
  * @brief  Enable the selected ADC.
  * @note   Prerequisite condition to use this function: ADC must be disabled
  *         and voltage regulator must be enabled (done into HAL_ADC_Init()).
  * @param  hadc ADC handle
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

    /* Wait for ADC effectively enabled */
    tickstart = HAL_GetTick();

    while(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_RDY) == RESET)
    {
      if((HAL_GetTick() - tickstart) > ADC_ENABLE_TIMEOUT)
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
  * @param  hadc ADC handle
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
    tickstart = HAL_GetTick();

    while(HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADEN))
    {
      if((HAL_GetTick() - tickstart) > ADC_DISABLE_TIMEOUT)
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
  * @param  hadc ADC handle
  * @param  ConversionGroup ADC group regular and/or injected.
  *          This parameter can be one of the following values:
  *            @arg ADC_REGULAR_GROUP: ADC regular conversion type.
  *            @arg ADC_INJECTED_GROUP: ADC injected conversion type.
  *            @arg ADC_REGULAR_INJECTED_GROUP: ADC regular and injected conversion type.
  * @retval HAL status.
  */
static HAL_StatusTypeDef ADC_ConversionStop(ADC_HandleTypeDef* hadc, uint32_t ConversionGroup)
{
  uint32_t tmp_ADC_CR_ADSTART_JADSTART = 0U;
  uint32_t tickstart = 0U;
  uint32_t Conversion_Timeout_CPU_cycles = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(hadc->Instance));
  assert_param(IS_ADC_CONVERSION_GROUP(ConversionGroup));

  /* Verification if ADC is not already stopped (on regular and injected      */
  /* groups) to bypass this function if not needed.                           */
  if (ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED(hadc))
  {
    /* Particular case of continuous auto-injection mode combined with        */
    /* auto-delay mode.                                                       */
    /* In auto-injection mode, regular group stop ADC_CR_ADSTP is used (not   */
    /* injected group stop ADC_CR_JADSTP).                                    */
    /* Procedure to be followed: Wait until JEOS=1U, clear JEOS, set ADSTP=1   */
    /* (see reference manual).                                                */
    if ((HAL_IS_BIT_SET(hadc->Instance->CFGR, ADC_CFGR_JAUTO)) &&
         (hadc->Init.ContinuousConvMode==ENABLE)               &&
         (hadc->Init.LowPowerAutoWait==ENABLE)                   )
    {
      /* Use stop of regular group */
      ConversionGroup = ADC_REGULAR_GROUP;

      /* Wait until JEOS=1 (maximum Timeout: 4 injected conversions) */
      while(__HAL_ADC_GET_FLAG(hadc, ADC_FLAG_JEOS) == RESET)
      {
        if (Conversion_Timeout_CPU_cycles >= (ADC_CONVERSION_TIME_MAX_CPU_CYCLES *4U))
        {
          /* Update ADC state machine to error */
          SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);

          /* Set ADC error code to ADC IP internal error */
          SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);

          return HAL_ERROR;
        }
        Conversion_Timeout_CPU_cycles ++;
      }

      /* Clear JEOS */
      __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_JEOS);
    }

    /* Stop potential conversion on going on regular group */
    if (ConversionGroup != ADC_INJECTED_GROUP)
    {
      /* Software is allowed to set ADSTP only when ADSTART=1 and ADDIS=0U */
      if (HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_ADSTART) &&
          HAL_IS_BIT_CLR(hadc->Instance->CR, ADC_CR_ADDIS)     )
      {
        /* Stop conversions on regular group */
        hadc->Instance->CR |= ADC_CR_ADSTP;
      }
    }

    /* Stop potential conversion on going on injected group */
    if (ConversionGroup != ADC_REGULAR_GROUP)
    {
      /* Software is allowed to set JADSTP only when JADSTART=1 and ADDIS=0U */
      if (HAL_IS_BIT_SET(hadc->Instance->CR, ADC_CR_JADSTART) &&
          HAL_IS_BIT_CLR(hadc->Instance->CR, ADC_CR_ADDIS)      )
      {
        /* Stop conversions on injected group */
        hadc->Instance->CR |= ADC_CR_JADSTP;
      }
    }

    /* Selection of start and stop bits in function of regular or injected group */
    switch(ConversionGroup)
    {
    case ADC_REGULAR_INJECTED_GROUP:
        tmp_ADC_CR_ADSTART_JADSTART = (ADC_CR_ADSTART | ADC_CR_JADSTART);
        break;
    case ADC_INJECTED_GROUP:
        tmp_ADC_CR_ADSTART_JADSTART = ADC_CR_JADSTART;
        break;
    /* Case ADC_REGULAR_GROUP */
    default:
        tmp_ADC_CR_ADSTART_JADSTART = ADC_CR_ADSTART;
        break;
    }

    /* Wait for conversion effectively stopped */
    tickstart = HAL_GetTick();

    while((hadc->Instance->CR & tmp_ADC_CR_ADSTART_JADSTART) != RESET)
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
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Enable the selected ADC.
  * @note   Prerequisite condition to use this function: ADC must be disabled
  *         and voltage regulator must be enabled (done into HAL_ADC_Init()).
  * @param  hadc ADC handle
  * @retval HAL status.
  */
static HAL_StatusTypeDef ADC_Enable(ADC_HandleTypeDef* hadc)
{
  uint32_t tickstart = 0U;
  __IO uint32_t wait_loop_index = 0U;

  /* ADC enable and wait for ADC ready (in case of ADC is disabled or         */
  /* enabling phase not yet completed: flag ADC ready not yet set).           */
  /* Timeout implemented to not be stuck if ADC cannot be enabled (possible   */
  /* causes: ADC clock not running, ...).                                     */
  if (ADC_IS_ENABLE(hadc) == RESET)
  {
    /* Enable the Peripheral */
    __HAL_ADC_ENABLE(hadc);

    /* Delay for ADC stabilization time */
    /* Compute number of CPU cycles to wait for */
    wait_loop_index = (ADC_STAB_DELAY_US * (SystemCoreClock / 1000000U));
    while(wait_loop_index != 0U)
    {
      wait_loop_index--;
    }

    /* Get tick count */
    tickstart = HAL_GetTick();

    /* Wait for ADC effectively enabled */
    while(ADC_IS_ENABLE(hadc) == RESET)
    {
      if((HAL_GetTick() - tickstart) > ADC_ENABLE_TIMEOUT)
      {
        /* Update ADC state machine to error */
        SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_INTERNAL);

        /* Set ADC error code to ADC IP internal error */
        SET_BIT(hadc->ErrorCode, HAL_ADC_ERROR_INTERNAL);

        /* Process unlocked */
        __HAL_UNLOCK(hadc);

        return HAL_ERROR;
      }
    }
  }

  /* Return HAL status */
  return HAL_OK;
}

/**
  * @brief  Stop ADC conversion and disable the selected ADC
  * @param  hadc ADC handle
  * @retval HAL status.
  */
static HAL_StatusTypeDef ADC_ConversionStop_Disable(ADC_HandleTypeDef* hadc)
{
  uint32_t tickstart = 0U;

  /* Verification if ADC is not already disabled:                             */
  if (ADC_IS_ENABLE(hadc) != RESET)
  {
    /* Disable the ADC peripheral */
    __HAL_ADC_DISABLE(hadc);

    /* Get tick count */
    tickstart = HAL_GetTick();

    /* Wait for ADC effectively disabled */
    while(ADC_IS_ENABLE(hadc) != RESET)
    {
      if((HAL_GetTick() - tickstart) > ADC_DISABLE_TIMEOUT)
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
#endif /* STM32F373xC || STM32F378xx */
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
