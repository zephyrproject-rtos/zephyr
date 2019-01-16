/**
  ******************************************************************************
  * @file    stm32g0xx_hal_tim_ex.c
  * @author  MCD Application Team
  * @brief   TIM HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Timer Extended peripheral:
  *           + Time Hall Sensor Interface Initialization
  *           + Time Hall Sensor Interface Start
  *           + Time Complementary signal break and dead time configuration
  *           + Time Master and Slave synchronization configuration
  *           + Time Output Compare/PWM Channel Configuration (for channels 5 and 6)
  *           + Time OCRef clear configuration
  *           + Timer remapping capabilities configuration
  @verbatim
  ==============================================================================
                      ##### TIMER Extended features #####
  ==============================================================================
  [..]
    The Timer Extended features include:
    (#) Complementary outputs with programmable dead-time for :
        (++) Output Compare
        (++) PWM generation (Edge and Center-aligned Mode)
        (++) One-pulse mode output
    (#) Synchronization circuit to control the timer with external signals and to
        interconnect several timers together.
    (#) Break input to put the timer output signals in reset state or in a known state.
    (#) Supports incremental (quadrature) encoder and hall-sensor circuitry for
        positioning purposes

            ##### How to use this driver #####
  ==============================================================================
    [..]
     (#) Initialize the TIM low level resources by implementing the following functions
         depending on the selected feature:
           (++) Hall Sensor output : HAL_TIMEx_HallSensor_MspInit()

     (#) Initialize the TIM low level resources :
        (##) Enable the TIM interface clock using __HAL_RCC_TIMx_CLK_ENABLE();
        (##) TIM pins configuration
            (+++) Enable the clock for the TIM GPIOs using the following function:
              __HAL_RCC_GPIOx_CLK_ENABLE();
            (+++) Configure these TIM pins in Alternate function mode using HAL_GPIO_Init();

     (#) The external Clock can be configured, if needed (the default clock is the
         internal clock from the APBx), using the following function:
         HAL_TIM_ConfigClockSource, the clock configuration should be done before
         any start function.

     (#) Configure the TIM in the desired functioning mode using one of the
         initialization function of this driver:
          (++) HAL_TIMEx_HallSensor_Init() and HAL_TIMEx_ConfigCommutEvent(): to use the
              Timer Hall Sensor Interface and the commutation event with the corresponding
              Interrupt and DMA request if needed (Note that One Timer is used to interface
             with the Hall sensor Interface and another Timer should be used to use
             the commutation event).

     (#) Activate the TIM peripheral using one of the start functions:
           (++) Complementary Output Compare : HAL_TIMEx_OCN_Start(), HAL_TIMEx_OCN_Start_DMA(), HAL_TIMEx_OC_Start_IT()
           (++) Complementary PWM generation : HAL_TIMEx_PWMN_Start(), HAL_TIMEx_PWMN_Start_DMA(), HAL_TIMEx_PWMN_Start_IT()
           (++) Complementary One-pulse mode output : HAL_TIMEx_OnePulseN_Start(), HAL_TIMEx_OnePulseN_Start_IT()
           (++) Hall Sensor output : HAL_TIMEx_HallSensor_Start(), HAL_TIMEx_HallSensor_Start_DMA(), HAL_TIMEx_HallSensor_Start_IT().

  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @defgroup TIMEx TIMEx
  * @brief TIM Extended HAL module driver
  * @{
  */

#ifdef HAL_TIM_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup TIMEx_Private_Constants TIM Extended Private Constants
  * @{
  */
/* Timeout for break input rearm */
#define TIM_BREAKINPUT_REARM_TIMEOUT    5UL /* 5 milliseconds */
/**
  * @}
  */
/* End of private constants --------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void TIM_CCxNChannelCmd(TIM_TypeDef* TIMx, uint32_t Channel, uint32_t ChannelNState);

/* Exported functions --------------------------------------------------------*/
/** @defgroup TIMEx_Exported_Functions TIM Extended Exported Functions
  * @{
  */

/** @defgroup TIMEx_Exported_Functions_Group1 Extended Timer Hall Sensor functions
  * @brief    Timer Hall Sensor functions
  *
@verbatim
  ==============================================================================
                      ##### Timer Hall Sensor functions #####
  ==============================================================================
  [..]
    This section provides functions allowing to:
    (+) Initialize and configure TIM HAL Sensor.
    (+) De-initialize TIM HAL Sensor.
    (+) Start the Hall Sensor Interface.
    (+) Stop the Hall Sensor Interface.
    (+) Start the Hall Sensor Interface and enable interrupts.
    (+) Stop the Hall Sensor Interface and disable interrupts.
    (+) Start the Hall Sensor Interface and enable DMA transfers.
    (+) Stop the Hall Sensor Interface and disable DMA transfers.

@endverbatim
  * @{
  */
/**
  * @brief  Initializes the TIM Hall Sensor Interface and initialize the associated handle.
  * @param  htim TIM Hall Sensor Interface handle
  * @param  sConfig TIM Hall Sensor configuration structure
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Init(TIM_HandleTypeDef *htim, TIM_HallSensor_InitTypeDef* sConfig)
{
  TIM_OC_InitTypeDef OC_Config;

  /* Check the TIM handle allocation */
  if(htim == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(htim->Instance));
  assert_param(IS_TIM_COUNTER_MODE(htim->Init.CounterMode));
  assert_param(IS_TIM_CLOCKDIVISION_DIV(htim->Init.ClockDivision));
  assert_param(IS_TIM_AUTORELOAD_PRELOAD(htim->Init.AutoReloadPreload));
  assert_param(IS_TIM_IC_POLARITY(sConfig->IC1Polarity));
  assert_param(IS_TIM_IC_PRESCALER(sConfig->IC1Prescaler));
  assert_param(IS_TIM_IC_FILTER(sConfig->IC1Filter));

  if(htim->State == HAL_TIM_STATE_RESET)
  {
    /* Allocate lock resource and initialize it */
    htim->Lock = HAL_UNLOCKED;

#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
    /* Reset interrupt callbacks to legacy week callbacks */
    TIM_ResetCallback(htim);

    if(htim->HallSensor_MspInitCallback == NULL)
    {
      htim->HallSensor_MspInitCallback = HAL_TIMEx_HallSensor_MspInit;
    }
    /* Init the low level hardware : GPIO, CLOCK, NVIC */
    htim->HallSensor_MspInitCallback(htim);
#else
    /* Init the low level hardware : GPIO, CLOCK, NVIC and DMA */
    HAL_TIMEx_HallSensor_MspInit(htim);
#endif /* USE_HAL_TIM_REGISTER_CALLBACKS */
  }

  /* Set the TIM state */
  htim->State = HAL_TIM_STATE_BUSY;

  /* Configure the Time base in the Encoder Mode */
  TIM_Base_SetConfig(htim->Instance, &htim->Init);

  /* Configure the Channel 1 as Input Channel to interface with the three Outputs of the  Hall sensor */
  TIM_TI1_SetConfig(htim->Instance, sConfig->IC1Polarity, TIM_ICSELECTION_TRC, sConfig->IC1Filter);

  /* Reset the IC1PSC Bits */
  htim->Instance->CCMR1 &= ~TIM_CCMR1_IC1PSC;
  /* Set the IC1PSC value */
  htim->Instance->CCMR1 |= sConfig->IC1Prescaler;

  /* Enable the Hall sensor interface (XOR function of the three inputs) */
  htim->Instance->CR2 |= TIM_CR2_TI1S;

  /* Select the TIM_TS_TI1F_ED signal as Input trigger for the TIM */
  htim->Instance->SMCR &= ~TIM_SMCR_TS;
  htim->Instance->SMCR |= TIM_TS_TI1F_ED;

  /* Use the TIM_TS_TI1F_ED signal to reset the TIM counter each edge detection */
  htim->Instance->SMCR &= ~TIM_SMCR_SMS;
  htim->Instance->SMCR |= TIM_SLAVEMODE_RESET;

  /* Program channel 2 in PWM 2 mode with the desired Commutation_Delay*/
  OC_Config.OCFastMode = TIM_OCFAST_DISABLE;
  OC_Config.OCIdleState = TIM_OCIDLESTATE_RESET;
  OC_Config.OCMode = TIM_OCMODE_PWM2;
  OC_Config.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  OC_Config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  OC_Config.OCPolarity = TIM_OCPOLARITY_HIGH;
  OC_Config.Pulse = sConfig->Commutation_Delay;

  TIM_OC2_SetConfig(htim->Instance, &OC_Config);

  /* Select OC2REF as trigger output on TRGO: write the MMS bits in the TIMx_CR2
    register to 101 */
  htim->Instance->CR2 &= ~TIM_CR2_MMS;
  htim->Instance->CR2 |= TIM_TRGO_OC2REF;

  /* Initialize the TIM state*/
  htim->State= HAL_TIM_STATE_READY;

  return HAL_OK;
}

/**
  * @brief  DeInitializes the TIM Hall Sensor interface
  * @param  htim TIM Hall Sensor Interface handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_DeInit(TIM_HandleTypeDef *htim)
{
  /* Check the parameters */
  assert_param(IS_TIM_INSTANCE(htim->Instance));

  htim->State = HAL_TIM_STATE_BUSY;

  /* Disable the TIM Peripheral Clock */
  __HAL_TIM_DISABLE(htim);

#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
  if(htim->HallSensor_MspDeInitCallback == NULL)
  {
    htim->HallSensor_MspDeInitCallback = HAL_TIMEx_HallSensor_MspDeInit;
  }
  /* DeInit the low level hardware */
  htim->HallSensor_MspDeInitCallback(htim);
#else
  /* DeInit the low level hardware: GPIO, CLOCK, NVIC */
  HAL_TIMEx_HallSensor_MspDeInit(htim);
#endif /* USE_HAL_TIM_REGISTER_CALLBACKS */

  /* Change TIM state */
  htim->State = HAL_TIM_STATE_RESET;

  /* Release Lock */
  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Initializes the TIM Hall Sensor MSP.
  * @param  htim TIM Hall Sensor Interface handle
  * @retval None
  */
__weak void HAL_TIMEx_HallSensor_MspInit(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);
    
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_TIMEx_HallSensor_MspInit could be implemented in the user file
   */
}

/**
  * @brief  DeInitializes TIM Hall Sensor MSP.
  * @param  htim TIM Hall Sensor Interface handle
  * @retval None
  */
__weak void HAL_TIMEx_HallSensor_MspDeInit(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);
    
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_TIMEx_HallSensor_MspDeInit could be implemented in the user file
   */
}

/**
  * @brief  Starts the TIM Hall Sensor Interface.
  * @param  htim TIM Hall Sensor Interface handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start(TIM_HandleTypeDef *htim)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(htim->Instance));

  /* Enable the Input Capture channel 1
    (in the Hall Sensor Interface the three possible channels that can be used are TIM_CHANNEL_1, TIM_CHANNEL_2 and TIM_CHANNEL_3) */
  TIM_CCxChannelCmd(htim->Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM Hall sensor Interface.
  * @param  htim TIM Hall Sensor Interface handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop(TIM_HandleTypeDef *htim)
{
  /* Check the parameters */
  assert_param(IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(htim->Instance));

  /* Disable the Input Capture channels 1, 2 and 3
    (in the Hall Sensor Interface the three possible channels that can be used are TIM_CHANNEL_1, TIM_CHANNEL_2 and TIM_CHANNEL_3) */
  TIM_CCxChannelCmd(htim->Instance, TIM_CHANNEL_1, TIM_CCx_DISABLE);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Starts the TIM Hall Sensor Interface in interrupt mode.
  * @param  htim TIM Hall Sensor Interface handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_IT(TIM_HandleTypeDef *htim)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(htim->Instance));

  /* Enable the capture compare Interrupts 1 event */
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC1);

  /* Enable the Input Capture channel 1
    (in the Hall Sensor Interface the three possible channels that can be used are TIM_CHANNEL_1, TIM_CHANNEL_2 and TIM_CHANNEL_3) */
  TIM_CCxChannelCmd(htim->Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM Hall Sensor Interface in interrupt mode.
  * @param  htim TIM Hall Sensor Interface handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_IT(TIM_HandleTypeDef *htim)
{
  /* Check the parameters */
  assert_param(IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(htim->Instance));

  /* Disable the Input Capture channel 1
    (in the Hall Sensor Interface the three possible channels that can be used are TIM_CHANNEL_1, TIM_CHANNEL_2 and TIM_CHANNEL_3) */
  TIM_CCxChannelCmd(htim->Instance, TIM_CHANNEL_1, TIM_CCx_DISABLE);

  /* Disable the capture compare Interrupts event */
  __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC1);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Starts the TIM Hall Sensor Interface in DMA mode.
  * @param  htim TIM Hall Sensor Interface handle
  * @param  pData The destination Buffer address.
  * @param  Length The length of data to be transferred from TIM peripheral to memory.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_DMA(TIM_HandleTypeDef *htim, uint32_t *pData, uint16_t Length)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(htim->Instance));

   if((htim->State == HAL_TIM_STATE_BUSY))
   {
     return HAL_BUSY;
   }
   else if((htim->State == HAL_TIM_STATE_READY))
   {
     if(((uint32_t)pData == 0U ) && (Length > 0U))
     {
       return HAL_ERROR;
     }
     else
     {
       htim->State = HAL_TIM_STATE_BUSY;
     }
   }
   else
   {
     /* nothing to do */
   }  
  /* Enable the Input Capture channel 1
    (in the Hall Sensor Interface the three possible channels that can be used are TIM_CHANNEL_1, TIM_CHANNEL_2 and TIM_CHANNEL_3) */
  TIM_CCxChannelCmd(htim->Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);

  /* Set the DMA Input Capture 1 Callbacks */
  htim->hdma[TIM_DMA_ID_CC1]->XferCpltCallback = TIM_DMACaptureCplt;
  htim->hdma[TIM_DMA_ID_CC1]->XferHalfCpltCallback = TIM_DMACaptureHalfCplt;
  /* Set the DMA error callback */
  htim->hdma[TIM_DMA_ID_CC1]->XferErrorCallback = TIM_DMAError ;

  /* Enable the DMA channel for Capture 1*/
  if (HAL_DMA_Start_IT(htim->hdma[TIM_DMA_ID_CC1], (uint32_t)&htim->Instance->CCR1, (uint32_t)pData, Length) != HAL_OK)
  {
    return HAL_ERROR;
  }
  /* Enable the capture compare 1 Interrupt */
  __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC1);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM Hall Sensor Interface in DMA mode.
  * @param  htim TIM Hall Sensor Interface handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_DMA(TIM_HandleTypeDef *htim)
{
  /* Check the parameters */
  assert_param(IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(htim->Instance));

  /* Disable the Input Capture channel 1
    (in the Hall Sensor Interface the three possible channels that can be used are TIM_CHANNEL_1, TIM_CHANNEL_2 and TIM_CHANNEL_3) */
  TIM_CCxChannelCmd(htim->Instance, TIM_CHANNEL_1, TIM_CCx_DISABLE);


  /* Disable the capture compare Interrupts 1 event */
  __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC1);

  (void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC1]);
  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup TIMEx_Exported_Functions_Group2 Extended Timer Complementary Output Compare functions
  *  @brief   Timer Complementary Output Compare functions
  *
@verbatim
  ==============================================================================
              ##### Timer Complementary Output Compare functions #####
  ==============================================================================
  [..]
    This section provides functions allowing to:
    (+) Start the Complementary Output Compare/PWM.
    (+) Stop the Complementary Output Compare/PWM.
    (+) Start the Complementary Output Compare/PWM and enable interrupts.
    (+) Stop the Complementary Output Compare/PWM and disable interrupts.
    (+) Start the Complementary Output Compare/PWM and enable DMA transfers.
    (+) Stop the Complementary Output Compare/PWM and disable DMA transfers.

@endverbatim
  * @{
  */

/**
  * @brief  Starts the TIM Output Compare signal generation on the complementary
  *         output.
  * @param  htim TIM Output Compare handle
  * @param  Channel TIM Channel to be enabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  /* Enable the Capture compare channel N */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM Output Compare signal generation on the complementary
  *         output.
  * @param  htim TIM handle
  * @param  Channel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  /* Disable the Capture compare channel N */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_DISABLE);

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Starts the TIM Output Compare signal generation in interrupt mode
  *         on the complementary output.
  * @param  htim TIM OC handle
  * @param  Channel TIM Channel to be enabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Enable the TIM Output Compare interrupt */
      __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC1);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Enable the TIM Output Compare interrupt */
      __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC2);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Enable the TIM Output Compare interrupt */
      __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC3);
      break;
    }


    default:
      break;
  }

  /* Enable the TIM Break interrupt */
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_BREAK);
  
  /* Enable the Capture compare channel N */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM Output Compare signal generation in interrupt mode
  *         on the complementary output.
  * @param  htim TIM Output Compare handle
  * @param  Channel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  uint32_t tmpccer;
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Disable the TIM Output Compare interrupt */
      __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC1);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Disable the TIM Output Compare interrupt */
      __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC2);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Disable the TIM Output Compare interrupt */
      __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC3);
      break;
    }

    default:
      break;
  }

  /* Disable the Capture compare channel N */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_DISABLE);

  /* Disable the TIM Break interrupt (only if no more channel is active) */
  tmpccer = htim->Instance->CCER;
  if ((tmpccer & (TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE)) == (uint32_t)RESET)
  {
    __HAL_TIM_DISABLE_IT(htim, TIM_IT_BREAK);
  }

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Starts the TIM Output Compare signal generation in DMA mode
  *         on the complementary output.
  * @param  htim TIM Output Compare handle
  * @param  Channel TIM Channel to be enabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @param  pData The source Buffer address.
  * @param  Length The length of data to be transferred from memory to TIM peripheral
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t *pData, uint16_t Length)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  if((htim->State == HAL_TIM_STATE_BUSY))
  {
    return HAL_BUSY;
  }
  else if((htim->State == HAL_TIM_STATE_READY))
  {
    if(((uint32_t)pData == 0U ) && (Length > 0U))
    {
      return HAL_ERROR;
    }
    else
    {
      htim->State = HAL_TIM_STATE_BUSY;
    }
  }
  else
  {
    /* nothing to do  */
  }

  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Set the DMA compare callbacks */
      htim->hdma[TIM_DMA_ID_CC1]->XferCpltCallback = TIM_DMADelayPulseCplt;
      htim->hdma[TIM_DMA_ID_CC1]->XferHalfCpltCallback = TIM_DMADelayPulseHalfCplt;

      /* Set the DMA error callback */
      htim->hdma[TIM_DMA_ID_CC1]->XferErrorCallback = TIM_DMAError ;

      /* Enable the DMA channel */
      if (HAL_DMA_Start_IT(htim->hdma[TIM_DMA_ID_CC1], (uint32_t)pData, (uint32_t)&htim->Instance->CCR1, Length) != HAL_OK)
      {
        return HAL_ERROR;
      }
      /* Enable the TIM Output Compare DMA request */
      __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC1);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Set the DMA compare callbacks */
      htim->hdma[TIM_DMA_ID_CC2]->XferCpltCallback = TIM_DMADelayPulseCplt;
      htim->hdma[TIM_DMA_ID_CC2]->XferHalfCpltCallback = TIM_DMADelayPulseHalfCplt;

      /* Set the DMA error callback */
      htim->hdma[TIM_DMA_ID_CC2]->XferErrorCallback = TIM_DMAError ;

      /* Enable the DMA channel */
      if (HAL_DMA_Start_IT(htim->hdma[TIM_DMA_ID_CC2], (uint32_t)pData, (uint32_t)&htim->Instance->CCR2, Length) != HAL_OK)
      {
        return HAL_ERROR;
      }
      /* Enable the TIM Output Compare DMA request */
      __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC2);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Set the DMA compare callbacks */
      htim->hdma[TIM_DMA_ID_CC3]->XferCpltCallback = TIM_DMADelayPulseCplt;
      htim->hdma[TIM_DMA_ID_CC3]->XferHalfCpltCallback = TIM_DMADelayPulseHalfCplt;

      /* Set the DMA error callback */
      htim->hdma[TIM_DMA_ID_CC3]->XferErrorCallback = TIM_DMAError ;

      /* Enable the DMA channel */
      if (HAL_DMA_Start_IT(htim->hdma[TIM_DMA_ID_CC3], (uint32_t)pData, (uint32_t)&htim->Instance->CCR3,Length) != HAL_OK)
      {
        return HAL_ERROR;
      }
      /* Enable the TIM Output Compare DMA request */
      __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC3);
      break;
    }

    default:
      break;
  }

  /* Enable the Capture compare channel N */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM Output Compare signal generation in DMA mode
  *         on the complementary output.
  * @param  htim TIM Output Compare handle
  * @param  Channel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Disable the TIM Output Compare DMA request */
      __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC1);
      (void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC1]);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Disable the TIM Output Compare DMA request */
      __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC2);
      (void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC2]);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Disable the TIM Output Compare DMA request */
      __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC3);
      (void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC3]);
      break;
    }

    default:
      break;
  }

  /* Disable the Capture compare channel N */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_DISABLE);

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Change the htim state */
  htim->State = HAL_TIM_STATE_READY;

  /* Return function status */
  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup TIMEx_Exported_Functions_Group3 Extended Timer Complementary PWM functions
  * @brief    Timer Complementary PWM functions
  *
@verbatim
  ==============================================================================
                 ##### Timer Complementary PWM functions #####
  ==============================================================================
  [..]
    This section provides functions allowing to:
    (+) Start the Complementary PWM.
    (+) Stop the Complementary PWM.
    (+) Start the Complementary PWM and enable interrupts.
    (+) Stop the Complementary PWM and disable interrupts.
    (+) Start the Complementary PWM and enable DMA transfers.
    (+) Stop the Complementary PWM and disable DMA transfers.
    (+) Start the Complementary Input Capture measurement.
    (+) Stop the Complementary Input Capture.
    (+) Start the Complementary Input Capture and enable interrupts.
    (+) Stop the Complementary Input Capture and disable interrupts.
    (+) Start the Complementary Input Capture and enable DMA transfers.
    (+) Stop the Complementary Input Capture and disable DMA transfers.
    (+) Start the Complementary One Pulse generation.
    (+) Stop the Complementary One Pulse.
    (+) Start the Complementary One Pulse and enable interrupts.
    (+) Stop the Complementary One Pulse and disable interrupts.

@endverbatim
  * @{
  */

/**
  * @brief  Starts the PWM signal generation on the complementary output.
  * @param  htim TIM handle
  * @param  Channel TIM Channel to be enabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  /* Enable the complementary PWM output  */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the PWM signal generation on the complementary output.
  * @param  htim TIM handle
  * @param  Channel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  /* Disable the complementary PWM output  */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_DISABLE);

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Starts the PWM signal generation in interrupt mode on the
  *         complementary output.
  * @param  htim TIM handle
  * @param  Channel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Enable the TIM Capture/Compare 1 interrupt */
      __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC1);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Enable the TIM Capture/Compare 2 interrupt */
      __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC2);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Enable the TIM Capture/Compare 3 interrupt */
      __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC3);
      break;
    }

    default:
      break;
  }

  /* Enable the TIM Break interrupt */
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_BREAK);

  /* Enable the complementary PWM output  */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the PWM signal generation in interrupt mode on the
  *         complementary output.
  * @param  htim TIM handle
  * @param  Channel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_IT (TIM_HandleTypeDef *htim, uint32_t Channel)
{
  uint32_t tmpccer;

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Disable the TIM Capture/Compare 1 interrupt */
      __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC1);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Disable the TIM Capture/Compare 2 interrupt */
      __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC2);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Disable the TIM Capture/Compare 3 interrupt */
      __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC3);
      break;
    }

    default:
      break;
  }

  /* Disable the complementary PWM output  */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_DISABLE);

  /* Disable the TIM Break interrupt (only if no more channel is active) */
  tmpccer = htim->Instance->CCER;
  if ((tmpccer & (TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE)) == (uint32_t)RESET)
  {
    __HAL_TIM_DISABLE_IT(htim, TIM_IT_BREAK);
  }

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Starts the TIM PWM signal generation in DMA mode on the
  *         complementary output
  * @param  htim TIM handle
  * @param  Channel TIM Channel to be enabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @param  pData The source Buffer address.
  * @param  Length The length of data to be transferred from memory to TIM peripheral
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t *pData, uint16_t Length)
{
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  if((htim->State == HAL_TIM_STATE_BUSY))
  {
    return HAL_BUSY;
  }
  else if((htim->State == HAL_TIM_STATE_READY))
  {
    if(((uint32_t)pData == 0U ) && (Length > 0U))
    {
      return HAL_ERROR;
    }
    else
    {
      htim->State = HAL_TIM_STATE_BUSY;
    }
  }
  else
  {
    /* nothing to do */
  }
  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Set the DMA compare callbacks */
      htim->hdma[TIM_DMA_ID_CC1]->XferCpltCallback = TIM_DMADelayPulseCplt;
      htim->hdma[TIM_DMA_ID_CC1]->XferHalfCpltCallback = TIM_DMADelayPulseHalfCplt;

      /* Set the DMA error callback */
      htim->hdma[TIM_DMA_ID_CC1]->XferErrorCallback = TIM_DMAError ;

      /* Enable the DMA channel */
      if (HAL_DMA_Start_IT(htim->hdma[TIM_DMA_ID_CC1], (uint32_t)pData, (uint32_t)&htim->Instance->CCR1, Length) != HAL_OK)
      {
        return HAL_ERROR;
      }
      /* Enable the TIM Capture/Compare 1 DMA request */
      __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC1);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Set the DMA compare callbacks */
      htim->hdma[TIM_DMA_ID_CC2]->XferCpltCallback = TIM_DMADelayPulseCplt;
      htim->hdma[TIM_DMA_ID_CC2]->XferHalfCpltCallback = TIM_DMADelayPulseHalfCplt;

      /* Set the DMA error callback */
      htim->hdma[TIM_DMA_ID_CC2]->XferErrorCallback = TIM_DMAError ;

      /* Enable the DMA channel */
      if (HAL_DMA_Start_IT(htim->hdma[TIM_DMA_ID_CC2], (uint32_t)pData, (uint32_t)&htim->Instance->CCR2, Length) != HAL_OK)
      {
        return HAL_ERROR;
      }
      /* Enable the TIM Capture/Compare 2 DMA request */
      __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC2);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Set the DMA compare callbacks */
      htim->hdma[TIM_DMA_ID_CC3]->XferCpltCallback = TIM_DMADelayPulseCplt;
      htim->hdma[TIM_DMA_ID_CC3]->XferHalfCpltCallback = TIM_DMADelayPulseHalfCplt;

      /* Set the DMA error callback */
      htim->hdma[TIM_DMA_ID_CC3]->XferErrorCallback = TIM_DMAError ;

      /* Enable the DMA channel */
      if (HAL_DMA_Start_IT(htim->hdma[TIM_DMA_ID_CC3], (uint32_t)pData, (uint32_t)&htim->Instance->CCR3,Length) != HAL_OK)
      {
        return HAL_ERROR;
      }
      /* Enable the TIM Capture/Compare 3 DMA request */
      __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_CC3);
      break;
    }

    default:
      break;
  }

  /* Enable the complementary PWM output  */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Enable the Peripheral, except in trigger mode where enable is automatically done with trigger */
  tmpsmcr = htim->Instance->SMCR & TIM_SMCR_SMS;
  if (!IS_TIM_SLAVEMODE_TRIGGER_ENABLED(tmpsmcr))
  {
  __HAL_TIM_ENABLE(htim);
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM PWM signal generation in DMA mode on the complementary
  *         output
  * @param  htim TIM handle
  * @param  Channel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, Channel));

  switch (Channel)
  {
    case TIM_CHANNEL_1:
    {
      /* Disable the TIM Capture/Compare 1 DMA request */
      __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC1);
      (void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC1]);
      break;
    }

    case TIM_CHANNEL_2:
    {
      /* Disable the TIM Capture/Compare 2 DMA request */
      __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC2);
      (void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC2]);
      break;
    }

    case TIM_CHANNEL_3:
    {
      /* Disable the TIM Capture/Compare 3 DMA request */
      __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC3);
      (void)HAL_DMA_Abort_IT(htim->hdma[TIM_DMA_ID_CC3]);
      break;
    }

    default:
      break;
  }

  /* Disable the complementary PWM output */
  TIM_CCxNChannelCmd(htim->Instance, Channel, TIM_CCxN_DISABLE);

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Change the htim state */
  htim->State = HAL_TIM_STATE_READY;

  /* Return function status */
  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup TIMEx_Exported_Functions_Group4 Extended Timer Complementary One Pulse functions
  * @brief    Timer Complementary One Pulse functions
  *
@verbatim
  ==============================================================================
                ##### Timer Complementary One Pulse functions #####
  ==============================================================================
  [..]
    This section provides functions allowing to:
    (+) Start the Complementary One Pulse generation.
    (+) Stop the Complementary One Pulse.
    (+) Start the Complementary One Pulse and enable interrupts.
    (+) Stop the Complementary One Pulse and disable interrupts.

@endverbatim
  * @{
  */

/**
  * @brief  Starts the TIM One Pulse signal generation on the complementary
  *         output.
  * @param  htim TIM One Pulse handle
  * @param  OutputChannel TIM Channel to be enabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start(TIM_HandleTypeDef *htim, uint32_t OutputChannel)
  {
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, OutputChannel));

  /* Enable the complementary One Pulse output */
  TIM_CCxNChannelCmd(htim->Instance, OutputChannel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM One Pulse signal generation on the complementary
  *         output.
  * @param  htim TIM One Pulse handle
  * @param  OutputChannel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop(TIM_HandleTypeDef *htim, uint32_t OutputChannel)
{

  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, OutputChannel));

  /* Disable the complementary One Pulse output */
  TIM_CCxNChannelCmd(htim->Instance, OutputChannel, TIM_CCxN_DISABLE);

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Starts the TIM One Pulse signal generation in interrupt mode on the
  *         complementary channel.
  * @param  htim TIM One Pulse handle
  * @param  OutputChannel TIM Channel to be enabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel)
{
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, OutputChannel));

  /* Enable the TIM Capture/Compare 1 interrupt */
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC1);

  /* Enable the TIM Capture/Compare 2 interrupt */
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_CC2);

  /* Enable the complementary One Pulse output */
  TIM_CCxNChannelCmd(htim->Instance, OutputChannel, TIM_CCxN_ENABLE);

  /* Enable the Main Output */
  __HAL_TIM_MOE_ENABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Stops the TIM One Pulse signal generation in interrupt mode on the
  *         complementary channel.
  * @param  htim TIM One Pulse handle
  * @param  OutputChannel TIM Channel to be disabled
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel)
{
  /* Check the parameters */
  assert_param(IS_TIM_CCXN_INSTANCE(htim->Instance, OutputChannel));

  /* Disable the TIM Capture/Compare 1 interrupt */
  __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC1);

  /* Disable the TIM Capture/Compare 2 interrupt */
  __HAL_TIM_DISABLE_IT(htim, TIM_IT_CC2);

  /* Disable the complementary One Pulse output */
  TIM_CCxNChannelCmd(htim->Instance, OutputChannel, TIM_CCxN_DISABLE);

  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(htim);

  /* Disable the Peripheral */
  __HAL_TIM_DISABLE(htim);

  /* Return function status */
  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup TIMEx_Exported_Functions_Group5 Extended Peripheral Control functions
  * @brief    Peripheral Control functions
  *
@verbatim
  ==============================================================================
                    ##### Peripheral Control functions #####
  ==============================================================================
  [..]
    This section provides functions allowing to:
    (+) Configure the commutation event in case of use of the Hall sensor interface.
      (+) Configure Output channels for OC and PWM mode.

      (+) Configure Complementary channels, break features and dead time.
      (+) Configure Master synchronization.
      (+) Configure timer remapping capabilities.
      (+) Select timer input source.
      (+) Enable or disable channel grouping.

@endverbatim
  * @{
  */

/**
  * @brief  Configure the TIM commutation event sequence.
  * @note  This function is mandatory to use the commutation event in order to
  *        update the configuration at each commutation detection on the TRGI input of the Timer,
  *        the typical use of this feature is with the use of another Timer(interface Timer)
  *        configured in Hall sensor interface, this interface Timer will generate the
  *        commutation at its TRGO output (connected to Timer used in this function) each time
  *        the TI1 of the Interface Timer detect a commutation at its input TI1.
  * @param  htim TIM handle
  * @param  InputTrigger the Internal trigger corresponding to the Timer Interfacing with the Hall sensor
  *          This parameter can be one of the following values:
  *            @arg TIM_TS_ITR0: Internal trigger 0 selected
  *            @arg TIM_TS_ITR1: Internal trigger 1 selected
  *            @arg TIM_TS_ITR2: Internal trigger 2 selected
  *            @arg TIM_TS_ITR3: Internal trigger 3 selected
  *            @arg TIM_TS_NONE: No trigger is needed
  * @param  CommutationSource the Commutation Event source
  *          This parameter can be one of the following values:
  *            @arg TIM_COMMUTATION_TRGI: Commutation source is the TRGI of the Interface Timer
  *            @arg TIM_COMMUTATION_SOFTWARE:  Commutation source is set by software using the COMG bit
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutEvent(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource)
{
  /* Check the parameters */
  assert_param(IS_TIM_COMMUTATION_EVENT_INSTANCE(htim->Instance));
  assert_param(IS_TIM_INTERNAL_TRIGGEREVENT_SELECTION(InputTrigger));

  __HAL_LOCK(htim);

  if ((InputTrigger == TIM_TS_ITR0) || (InputTrigger == TIM_TS_ITR1) ||
      (InputTrigger == TIM_TS_ITR2) || (InputTrigger == TIM_TS_ITR3))
  {
    /* Select the Input trigger */
    htim->Instance->SMCR &= ~TIM_SMCR_TS;
    htim->Instance->SMCR |= InputTrigger;
  }

  /* Select the Capture Compare preload feature */
  htim->Instance->CR2 |= TIM_CR2_CCPC;
  /* Select the Commutation event source */
  htim->Instance->CR2 &= ~TIM_CR2_CCUS;
  htim->Instance->CR2 |= CommutationSource;

  /* Disable Commutation Interrupt */
  __HAL_TIM_DISABLE_IT(htim, TIM_IT_COM);

  /* Disable Commutation DMA request */
  __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_COM);

  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Configure the TIM commutation event sequence with interrupt.
  * @note  This function is mandatory to use the commutation event in order to
  *        update the configuration at each commutation detection on the TRGI input of the Timer,
  *        the typical use of this feature is with the use of another Timer(interface Timer)
  *        configured in Hall sensor interface, this interface Timer will generate the
  *        commutation at its TRGO output (connected to Timer used in this function) each time
  *        the TI1 of the Interface Timer detect a commutation at its input TI1.
  * @param  htim TIM handle
  * @param  InputTrigger the Internal trigger corresponding to the Timer Interfacing with the Hall sensor
  *          This parameter can be one of the following values:
  *            @arg TIM_TS_ITR0: Internal trigger 0 selected
  *            @arg TIM_TS_ITR1: Internal trigger 1 selected
  *            @arg TIM_TS_ITR2: Internal trigger 2 selected
  *            @arg TIM_TS_ITR3: Internal trigger 3 selected
  *            @arg TIM_TS_NONE: No trigger is needed
  * @param  CommutationSource the Commutation Event source
  *          This parameter can be one of the following values:
  *            @arg TIM_COMMUTATION_TRGI: Commutation source is the TRGI of the Interface Timer
  *            @arg TIM_COMMUTATION_SOFTWARE:  Commutation source is set by software using the COMG bit
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutEvent_IT(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource)
{
  /* Check the parameters */
  assert_param(IS_TIM_COMMUTATION_EVENT_INSTANCE(htim->Instance));
  assert_param(IS_TIM_INTERNAL_TRIGGEREVENT_SELECTION(InputTrigger));

  __HAL_LOCK(htim);

  if ((InputTrigger == TIM_TS_ITR0) || (InputTrigger == TIM_TS_ITR1) ||
      (InputTrigger == TIM_TS_ITR2) || (InputTrigger == TIM_TS_ITR3))
  {
    /* Select the Input trigger */
    htim->Instance->SMCR &= ~TIM_SMCR_TS;
    htim->Instance->SMCR |= InputTrigger;
  }

  /* Select the Capture Compare preload feature */
  htim->Instance->CR2 |= TIM_CR2_CCPC;
  /* Select the Commutation event source */
  htim->Instance->CR2 &= ~TIM_CR2_CCUS;
  htim->Instance->CR2 |= CommutationSource;

  /* Disable Commutation DMA request */
  __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_COM);

  /* Enable the Commutation Interrupt */
  __HAL_TIM_ENABLE_IT(htim, TIM_IT_COM);

  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Configure the TIM commutation event sequence with DMA.
  * @note  This function is mandatory to use the commutation event in order to
  *        update the configuration at each commutation detection on the TRGI input of the Timer,
  *        the typical use of this feature is with the use of another Timer(interface Timer)
  *        configured in Hall sensor interface, this interface Timer will generate the
  *        commutation at its TRGO output (connected to Timer used in this function) each time
  *        the TI1 of the Interface Timer detect a commutation at its input TI1.
  * @note  The user should configure the DMA in his own software, in This function only the COMDE bit is set
  * @param  htim TIM handle
  * @param  InputTrigger the Internal trigger corresponding to the Timer Interfacing with the Hall sensor
  *          This parameter can be one of the following values:
  *            @arg TIM_TS_ITR0: Internal trigger 0 selected
  *            @arg TIM_TS_ITR1: Internal trigger 1 selected
  *            @arg TIM_TS_ITR2: Internal trigger 2 selected
  *            @arg TIM_TS_ITR3: Internal trigger 3 selected
  *            @arg TIM_TS_NONE: No trigger is needed
  * @param  CommutationSource the Commutation Event source
  *          This parameter can be one of the following values:
  *            @arg TIM_COMMUTATION_TRGI: Commutation source is the TRGI of the Interface Timer
  *            @arg TIM_COMMUTATION_SOFTWARE:  Commutation source is set by software using the COMG bit
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutEvent_DMA(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource)
{
  /* Check the parameters */
  assert_param(IS_TIM_COMMUTATION_EVENT_INSTANCE(htim->Instance));
  assert_param(IS_TIM_INTERNAL_TRIGGEREVENT_SELECTION(InputTrigger));

  __HAL_LOCK(htim);

  if ((InputTrigger == TIM_TS_ITR0) || (InputTrigger == TIM_TS_ITR1) ||
      (InputTrigger == TIM_TS_ITR2) || (InputTrigger == TIM_TS_ITR3))
  {
    /* Select the Input trigger */
    htim->Instance->SMCR &= ~TIM_SMCR_TS;
    htim->Instance->SMCR |= InputTrigger;
  }

  /* Select the Capture Compare preload feature */
  htim->Instance->CR2 |= TIM_CR2_CCPC;
  /* Select the Commutation event source */
  htim->Instance->CR2 &= ~TIM_CR2_CCUS;
  htim->Instance->CR2 |= CommutationSource;

  /* Enable the Commutation DMA Request */
  /* Set the DMA Commutation Callback */
  htim->hdma[TIM_DMA_ID_COMMUTATION]->XferCpltCallback = TIMEx_DMACommutationCplt;
  htim->hdma[TIM_DMA_ID_COMMUTATION]->XferHalfCpltCallback = TIMEx_DMACommutationHalfCplt;
  /* Set the DMA error callback */
  htim->hdma[TIM_DMA_ID_COMMUTATION]->XferErrorCallback = TIM_DMAError;

  /* Disable Commutation Interrupt */
  __HAL_TIM_DISABLE_IT(htim, TIM_IT_COM);

  /* Enable the Commutation DMA Request */
  __HAL_TIM_ENABLE_DMA(htim, TIM_DMA_COM);

  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Configures the TIM in master mode.
  * @param  htim TIM handle.
  * @param  sMasterConfig pointer to a TIM_MasterConfigTypeDef structure that
  *         contains the selected trigger output (TRGO) and the Master/Slave
  *         mode.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim,
                                                      TIM_MasterConfigTypeDef * sMasterConfig)
{
  uint32_t tmpcr2;
  uint32_t tmpsmcr;

  /* Check the parameters */
  assert_param(IS_TIM_SYNCHRO_INSTANCE(htim->Instance));
  assert_param(IS_TIM_TRGO_SOURCE(sMasterConfig->MasterOutputTrigger));
  assert_param(IS_TIM_MSM_STATE(sMasterConfig->MasterSlaveMode));

  /* Check input state */
  __HAL_LOCK(htim);
  
  /* Change the handler state */
  htim->State = HAL_TIM_STATE_BUSY;

  /* Get the TIMx CR2 register value */
  tmpcr2 = htim->Instance->CR2;

  /* Get the TIMx SMCR register value */
  tmpsmcr = htim->Instance->SMCR;

  /* If the timer supports ADC synchronization through TRGO2, set the master mode selection 2 */
  if (IS_TIM_TRGO2_INSTANCE(htim->Instance))
  {
    /* Check the parameters */
    assert_param(IS_TIM_TRGO2_SOURCE(sMasterConfig->MasterOutputTrigger2));

    /* Clear the MMS2 bits */
    tmpcr2 &= ~TIM_CR2_MMS2;
    /* Select the TRGO2 source*/
    tmpcr2 |= sMasterConfig->MasterOutputTrigger2;
  }

  /* Reset the MMS Bits */
  tmpcr2 &= ~TIM_CR2_MMS;
  /* Select the TRGO source */
  tmpcr2 |=  sMasterConfig->MasterOutputTrigger;

  /* Reset the MSM Bit */
  tmpsmcr &= ~TIM_SMCR_MSM;
  /* Set master mode */
  tmpsmcr |= sMasterConfig->MasterSlaveMode;

  /* Update TIMx CR2 */
  htim->Instance->CR2 = tmpcr2;

  /* Update TIMx SMCR */
  htim->Instance->SMCR = tmpsmcr;
  
  /* Change the htim state */
  htim->State = HAL_TIM_STATE_READY;

  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief   Configures the Break feature, dead time, Lock level, OSSI/OSSR State
  *         and the AOE(automatic output enable).
  * @param  htim TIM handle
  * @param  sBreakDeadTimeConfig pointer to a TIM_ConfigBreakDeadConfigTypeDef structure that
  *         contains the BDTR Register configuration  information for the TIM peripheral.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakDeadTime(TIM_HandleTypeDef *htim,
                                                TIM_BreakDeadTimeConfigTypeDef * sBreakDeadTimeConfig)
{
  /* Keep this variable initialized to 0 as it is used to configure BDTR register */
  uint32_t tmpbdtr = 0U;

  /* Check the parameters */
  assert_param(IS_TIM_BREAK_INSTANCE(htim->Instance));
  assert_param(IS_TIM_OSSR_STATE(sBreakDeadTimeConfig->OffStateRunMode));
  assert_param(IS_TIM_OSSI_STATE(sBreakDeadTimeConfig->OffStateIDLEMode));
  assert_param(IS_TIM_LOCK_LEVEL(sBreakDeadTimeConfig->LockLevel));
  assert_param(IS_TIM_DEADTIME(sBreakDeadTimeConfig->DeadTime));
  assert_param(IS_TIM_BREAK_STATE(sBreakDeadTimeConfig->BreakState));
  assert_param(IS_TIM_BREAK_POLARITY(sBreakDeadTimeConfig->BreakPolarity));
  assert_param(IS_TIM_BREAK_FILTER(sBreakDeadTimeConfig->BreakFilter));
  assert_param(IS_TIM_AUTOMATIC_OUTPUT_STATE(sBreakDeadTimeConfig->AutomaticOutput));

  /* Check input state */
  __HAL_LOCK(htim);

  /* Set the Lock level, the Break enable Bit and the Polarity, the OSSR State,
     the OSSI State, the dead time value and the Automatic Output Enable Bit */

  /* Set the BDTR bits */
  MODIFY_REG(tmpbdtr, TIM_BDTR_DTG, sBreakDeadTimeConfig->DeadTime);
  MODIFY_REG(tmpbdtr, TIM_BDTR_LOCK, sBreakDeadTimeConfig->LockLevel);
  MODIFY_REG(tmpbdtr, TIM_BDTR_OSSI, sBreakDeadTimeConfig->OffStateIDLEMode);
  MODIFY_REG(tmpbdtr, TIM_BDTR_OSSR, sBreakDeadTimeConfig->OffStateRunMode);
  MODIFY_REG(tmpbdtr, TIM_BDTR_BKE, sBreakDeadTimeConfig->BreakState);
  MODIFY_REG(tmpbdtr, TIM_BDTR_BKP, sBreakDeadTimeConfig->BreakPolarity);
  MODIFY_REG(tmpbdtr, TIM_BDTR_AOE, sBreakDeadTimeConfig->AutomaticOutput);
  MODIFY_REG(tmpbdtr, TIM_BDTR_BKF, (sBreakDeadTimeConfig->BreakFilter << TIM_BDTR_BKF_Pos));
  
  if (IS_TIM_ADVANCED_INSTANCE(htim->Instance))
  {
    /* Check the parameters */
    assert_param(IS_TIM_BREAK_AFMODE(sBreakDeadTimeConfig->BreakAFMode));
    
    /* Set BREAK AF mode */
    MODIFY_REG(tmpbdtr, TIM_BDTR_BKBID, sBreakDeadTimeConfig->BreakAFMode);
  }
  
  if (IS_TIM_BKIN2_INSTANCE(htim->Instance))
  {
    /* Check the parameters */
    assert_param(IS_TIM_BREAK2_STATE(sBreakDeadTimeConfig->Break2State));
    assert_param(IS_TIM_BREAK2_POLARITY(sBreakDeadTimeConfig->Break2Polarity));
    assert_param(IS_TIM_BREAK_FILTER(sBreakDeadTimeConfig->Break2Filter));

    /* Set the BREAK2 input related BDTR bits */
    MODIFY_REG(tmpbdtr, TIM_BDTR_BK2F, (sBreakDeadTimeConfig->Break2Filter << TIM_BDTR_BK2F_Pos));
    MODIFY_REG(tmpbdtr, TIM_BDTR_BK2E, sBreakDeadTimeConfig->Break2State);
    MODIFY_REG(tmpbdtr, TIM_BDTR_BK2P, sBreakDeadTimeConfig->Break2Polarity);

    if (IS_TIM_ADVANCED_INSTANCE(htim->Instance))
    {
      /* Check the parameters */
      assert_param(IS_TIM_BREAK2_AFMODE(sBreakDeadTimeConfig->Break2AFMode));
      
      /* Set BREAK2 AF mode */
      MODIFY_REG(tmpbdtr, TIM_BDTR_BK2BID, sBreakDeadTimeConfig->Break2AFMode);
    }
  }

  /* Set TIMx_BDTR */
  htim->Instance->BDTR = tmpbdtr;

  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Configures the break input source.
  * @param  htim TIM handle.
  * @param  BreakInput Break input to configure
  *          This parameter can be one of the following values:
  *            @arg TIM_BREAKINPUT_BRK: Timer break input
  *            @arg TIM_BREAKINPUT_BRK2: Timer break 2 input
  * @param  sBreakInputConfig Break input source configuration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakInput(TIM_HandleTypeDef *htim,
                                             uint32_t BreakInput,
                                             TIMEx_BreakInputConfigTypeDef *sBreakInputConfig)

{
  uint32_t tmporx;
  uint32_t bkin_enable_mask;
  uint32_t bkin_polarity_mask;
  uint32_t bkin_enable_bitpos;
  uint32_t bkin_polarity_bitpos;

  /* Check the parameters */
  assert_param(IS_TIM_BREAK_INSTANCE(htim->Instance));
  assert_param(IS_TIM_BREAKINPUT(BreakInput));
  assert_param(IS_TIM_BREAKINPUTSOURCE(sBreakInputConfig->Source));
  assert_param(IS_TIM_BREAKINPUTSOURCE_STATE(sBreakInputConfig->Enable));
  assert_param(IS_TIM_BREAKINPUTSOURCE_POLARITY(sBreakInputConfig->Polarity));

  /* Check input state */
  __HAL_LOCK(htim);
  
  switch(sBreakInputConfig->Source)
  {
    case TIM_BREAKINPUTSOURCE_BKIN:
    {
      bkin_enable_mask = TIM1_AF1_BKINE;
      bkin_enable_bitpos = TIM1_AF1_BKINE_Pos;
      bkin_polarity_mask = TIM1_AF1_BKINP;
      bkin_polarity_bitpos = TIM1_AF1_BKINP_Pos;
      break;
    }
#if defined(COMP1) && defined(COMP2)
    case TIM_BREAKINPUTSOURCE_COMP1:
    {
      bkin_enable_mask = TIM1_AF1_BKCMP1E;
      bkin_enable_bitpos = TIM1_AF1_BKCMP1E_Pos;
      bkin_polarity_mask = TIM1_AF1_BKCMP1P;
      bkin_polarity_bitpos = TIM1_AF1_BKCMP1P_Pos;
      break;
    }
    case TIM_BREAKINPUTSOURCE_COMP2:
    {
      bkin_enable_mask = TIM1_AF1_BKCMP2E;
      bkin_enable_bitpos = TIM1_AF1_BKCMP2E_Pos;
      bkin_polarity_mask = TIM1_AF1_BKCMP2P;
      bkin_polarity_bitpos = TIM1_AF1_BKCMP2P_Pos;
      break;
    }
#endif /* COMP1 && COMP2 */

  default:
    {
      bkin_enable_mask = 0U;
      bkin_enable_bitpos = 0U;
      bkin_polarity_mask = 0U;
      bkin_polarity_bitpos = 0U;
      break;
    }
  }
  
  switch(BreakInput)
  {
    case TIM_BREAKINPUT_BRK:
    {
      /* Get the TIMx_AF1 register value */
      tmporx = htim->Instance->AF1;
        
      /* Enable the break input */
      tmporx &= ~bkin_enable_mask;
      tmporx |= (sBreakInputConfig->Enable << bkin_enable_bitpos) & bkin_enable_mask;
        
      /* Set the break input polarity */
      tmporx &= ~bkin_polarity_mask;
      tmporx |= (sBreakInputConfig->Polarity << bkin_polarity_bitpos) & bkin_polarity_mask;
        
      /* Set TIMx_AF1 */
        htim->Instance->AF1 = tmporx;        
      break;
    }
    case TIM_BREAKINPUT_BRK2:
    {
      /* Get the TIMx_AF2 register value */
      tmporx = htim->Instance->AF2;
        
      /* Enable the break input */
      tmporx &= ~bkin_enable_mask;
      tmporx |= (sBreakInputConfig->Enable << bkin_enable_bitpos) & bkin_enable_mask;
        
      /* Set the break input polarity */
      tmporx &= ~bkin_polarity_mask;
      tmporx |= (sBreakInputConfig->Polarity << bkin_polarity_bitpos) & bkin_polarity_mask;
        
      /* Set TIMx_AF2 */
        htim->Instance->AF2 = tmporx;        
      break;
      }
  default:
      break;
  }
  
  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Configures the TIMx Remapping input capabilities.
  * @param  htim TIM handle.
  * @param  Remap specifies the TIM remapping source.
  *         For TIM1, the parameter can take one of the following values:
  *            @arg TIM_TIM1_ETR_GPIO:                TIM1 ETR is is connected to GPIO
  @if STM32G081xx
  *            @arg TIM_TIM1_ETR_COMP1:               TIM1 ETR is connected to COMP1 output
  *            @arg TIM_TIM1_ETR_COMP2:               TIM1 ETR is connected to COMP2 output
  @endif
  *            @arg TIM_TIM1_ETR_ADC1_AWD1:           TIM1 ETR is connected to ADC1 AWD1
  *            @arg TIM_TIM1_ETR_ADC1_AWD2:           TIM1 ETR is connected to ADC1 AWD2
  *            @arg TIM_TIM1_ETR_ADC1_AWD3:           TIM1 ETR is connected to ADC1 AWD3
  *
  @if STM32G081xx
  *         For TIM2, the parameter can take one of the following values:
  *            @arg TIM_TIM2_ETR_GPIO:                TIM2_ETR is connected to GPIO
  *            @arg TIM_TIM2_ETR_COMP1:               TIM2_ETR is connected to COMP1 output
  *            @arg TIM_TIM2_ETR_COMP2:               TIM2_ETR is connected to COMP2 output
  *            @arg TIM_TIM2_ETR_LSE:                 TIM2_ETR is connected to LSE
  *
  *         For TIM3, the parameter can take one of the following values:
  *            @arg TIM_TIM3_ETR_GPIO                TIM3_ETR is connected to GPIO
  *            @arg TIM_TIM3_ETR_COMP1               TIM3_ETR is connected to COMP1 output
  *            @arg TIM_TIM3_ETR_COMP2               TIM3_ETR is connected to COMP2 output
  @endif
  *
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_RemapConfig(TIM_HandleTypeDef *htim, uint32_t Remap)
{
  /* Check parameters */
  assert_param(IS_TIM_REMAP_INSTANCE(htim->Instance));
  assert_param(IS_TIM_REMAP(Remap));
  
  __HAL_LOCK(htim);

  MODIFY_REG(htim->Instance->AF1, TIM1_AF1_ETRSEL_Msk, Remap);
  
  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Select the timer input source
  * @param  htim TIM handle.
  * @param  Channel specifies the TIM Channel
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TI1 input channel
  *            @arg TIM_CHANNEL_2: TI2 input channel
  * @param  TISelection specifies the timer input source
  @if STM32G081xx
  *         For TIM1 this parameter can be one of the following values:
  *            @arg TIM_TIM1_TI1_GPIO:                TIM1 TI1 is connected to GPIO
  *            @arg TIM_TIM1_TI1_COMP1:               TIM1 TI1 is connected to COMP1 output
  *            @arg TIM_TIM1_TI2_GPIO:                TIM1 TI2 is connected to GPIO
  *            @arg TIM_TIM1_TI2_COMP2:               TIM1 TI2 is connected to COMP2 output
  *
  *         For TIM2, the parameter is one of the following values:
  *            @arg TIM_TIM2_TI1_GPIO:                TIM2 TI1 is connected to GPIO
  *            @arg TIM_TIM2_TI1_COMP1:               TIM2 TI1 is connected to COMP1 output
  *            @arg TIM_TIM2_TI2_GPIO:                TIM2 TI2 is connected to GPIO
  *            @arg TIM_TIM2_TI2_COMP2:               TIM2 TI2 is connected to COMP2 output
  *
  *         For TIM3, the parameter is one of the following values:
  *            @arg TIM_TIM3_TI1_GPIO:                TIM3 TI1 is connected to GPIO
  *            @arg TIM_TIM3_TI1_COMP1:               TIM3 TI1 is connected to COMP1 output
  *            @arg TIM_TIM3_TI2_GPIO:                TIM3 TI2 is connected to GPIO
  *            @arg TIM_TIM3_TI2_COMP2:               TIM3 TI2 is connected to COMP2 output
  @endif
  *
  *         For TIM14, the parameter is one of the following values:
  *            @arg TIM_TIM14_TI1_GPIO:               TIM14 TI1 is connected to GPIO
  *            @arg TIM_TIM14_TI1_RTC:                TIM14 TI1 is connected to RTC clock
  *            @arg TIM_TIM14_TI1_HSE_32:             TIM14 TI1 is connected to HSE div 32
  *            @arg TIM_TIM14_TI1_MCO:                TIM14 TI1 is connected to MCO
  *
  *         For TIM15, the parameter is one of the following values:
  *            @arg TIM_TIM15_TI1_GPIO:              TIM15 TI1 is connected to GPIO
  *            @arg TIM_TIM15_TI1_TIM2_CH1:           TIM15 TI1 is connected to TIM2 CH1
  *            @arg TIM_TIM15_TI1_TIM3_CH1:           TIM15 TI1 is connected to TIM3 CH1
  *            @arg TIM_TIM15_TI2_GPIO:               TIM15 TI2 is connected to GPIO
  *            @arg TIM_TIM15_TI2_TIM2_CH2:           TIM15 TI2 is connected to TIM2 CH2
  *            @arg TIM_TIM15_TI2_TIM3_CH2:           TIM15 TI2 is connected to TIM3 CH2
  *
  *         For TIM16, the parameter can have the following values:
  *            @arg TIM_TIM16_TI1_GPIO:              TIM16 TI1 is connected to GPIO
  *            @arg TIM_TIM16_TI1_LSI:               TIM16 TI1 is connected to LSI
  *            @arg TIM_TIM16_TI1_LSE:               TIM16 TI1 is connected to LSE
  *            @arg TIM_TIM16_TI1_RTC_WAKEUP:         TIM16 TI1 is connected to TRC wakeup interrupt
  *
  *         For TIM17, the parameter can have the following values:
  *            @arg TIM_TIM17_TI1_GPIO:              TIM17 TI1 is connected to GPIO
  *            @arg TIM_TIM17_TI1_HSE_32:            TIM17 TI1 is connected to HSE div 32
  *            @arg TIM_TIM17_TI1_MCO:               TIM17 TI1 is connected to MCO
  *
  * @retval HAL status
  */
HAL_StatusTypeDef  HAL_TIMEx_TISelection(TIM_HandleTypeDef *htim, uint32_t TISelection , uint32_t Channel)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check parameters */
  assert_param(IS_TIM_TISEL_INSTANCE(htim->Instance));
  assert_param(IS_TIM_TISEL(TISelection));
  
  __HAL_LOCK(htim);

  switch (Channel)
  {
    case TIM_CHANNEL_1:
      MODIFY_REG(htim->Instance->TISEL, TIM_TISEL_TI1SEL, TISelection);
      break;
    case TIM_CHANNEL_2:
      MODIFY_REG(htim->Instance->TISEL, TIM_TISEL_TI2SEL, TISelection);
      break;
    default:
      status = HAL_ERROR;
      break;
  }
  
  __HAL_UNLOCK(htim);

  return status;
}

/**
  * @brief  Group channel 5 and channel 1, 2 or 3
  * @param  htim TIM handle.
  * @param  Channels specifies the reference signal(s) the OC5REF is combined with.
  *         This parameter can be any combination of the following values:
  *         TIM_GROUPCH5_NONE: No effect of OC5REF on OC1REFC, OC2REFC and OC3REFC
  *         TIM_GROUPCH5_OC1REFC: OC1REFC is the logical AND of OC1REFC and OC5REF
  *         TIM_GROUPCH5_OC2REFC: OC2REFC is the logical AND of OC2REFC and OC5REF
  *         TIM_GROUPCH5_OC3REFC: OC3REFC is the logical AND of OC3REFC and OC5REF
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_GroupChannel5(TIM_HandleTypeDef *htim, uint32_t Channels)
{
  /* Check parameters */
  assert_param(IS_TIM_COMBINED3PHASEPWM_INSTANCE(htim->Instance));
  assert_param(IS_TIM_GROUPCH5(Channels));

  /* Process Locked */
  __HAL_LOCK(htim);

  htim->State = HAL_TIM_STATE_BUSY;

  /* Clear GC5Cx bit fields */
  htim->Instance->CCR5 &= ~(TIM_CCR5_GC5C3|TIM_CCR5_GC5C2|TIM_CCR5_GC5C1);

  /* Set GC5Cx bit fields */
  htim->Instance->CCR5 |= Channels;

  /* Change the htim state */
  htim->State = HAL_TIM_STATE_READY;

  __HAL_UNLOCK(htim);

  return HAL_OK;
}

/**
  * @brief  Disarm the designated break input (when it operates in bidirectional mode).
  * @param  htim TIM handle.
  * @param  BreakInput Break input to disarm
  *          This parameter can be one of the following values:
  *            @arg TIM_BREAKINPUT_BRK: Timer break input
  *            @arg TIM_BREAKINPUT_BRK2: Timer break 2 input
  * @note  The break input can be disarmed only when it is configured in
  *        bidirectional mode and when when MOE is reset.
  * @note  Purpose is to be able to have the input voltage back to high-state,
  *        whatever the time constant on the output .
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_DisarmBreakInput(TIM_HandleTypeDef *htim, uint32_t BreakInput)
{
  uint32_t tmpbdtr;

  /* Check the parameters */
  assert_param(IS_TIM_ADVANCED_INSTANCE(htim->Instance));
  assert_param(IS_TIM_BREAKINPUT(BreakInput));

  switch (BreakInput)
  {
    case TIM_BREAKINPUT_BRK:
      {
        /* Check initial conditions */
        tmpbdtr = READ_REG(htim->Instance->BDTR);
        if ((READ_BIT(tmpbdtr, TIM_BDTR_BKBID) == TIM_BDTR_BKBID) &&
            (READ_BIT(tmpbdtr, TIM_BDTR_MOE) == 0U))
        {
          /* Break input BRK is disarmed */
          SET_BIT(htim->Instance->BDTR, TIM_BDTR_BKDSRM);
        }
        break;
      }
    
    case TIM_BREAKINPUT_BRK2:
      {
        /* Check initial conditions */
        tmpbdtr = READ_REG(htim->Instance->BDTR);
        if ((READ_BIT(tmpbdtr, TIM_BDTR_BK2BID) == TIM_BDTR_BK2BID) &&
            (READ_BIT(tmpbdtr, TIM_BDTR_MOE) == 0U))
        {
          /* Break input BRK is disarmed */
          SET_BIT(htim->Instance->BDTR, TIM_BDTR_BK2DSRM);
        }
      break;
      }      
    default:
      break;
  }
  
  return HAL_OK;
}

/**
  * @brief  Arm the designated break input (when it operates in bidirectional mode).
  * @param  htim TIM handle.
  * @param  BreakInput Break input to arm
  *          This parameter can be one of the following values:
  *            @arg TIM_BREAKINPUT_BRK: Timer break input
  *            @arg TIM_BREAKINPUT_BRK2: Timer break 2 input
  * @note  Arming is possible at anytime, even if fault is present.
  * @note  Break input is automatically armed as soon as MOE bit is set.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_TIMEx_ReArmBreakInput(TIM_HandleTypeDef *htim, uint32_t BreakInput)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_TIM_ADVANCED_INSTANCE(htim->Instance));
  assert_param(IS_TIM_BREAKINPUT(BreakInput));

  switch (BreakInput)
  {
    case TIM_BREAKINPUT_BRK:
      {
        /* Check initial conditions */
        if (READ_BIT(htim->Instance->BDTR, TIM_BDTR_BKBID) == TIM_BDTR_BKBID)
        {
        /* Break input BRK is re-armed automatically by hardware. Poll to check whether fault condition disappeared */
        /* Init tickstart for timeout management */
        tickstart = HAL_GetTick();
        do
        {
          if (READ_BIT(htim->Instance->BDTR, TIM_BDTR_BKDSRM) != TIM_BDTR_BKDSRM)
          {
            return HAL_OK;
          }
        } while ((HAL_GetTick() - tickstart) <= TIM_BREAKINPUT_REARM_TIMEOUT);

        return HAL_TIMEOUT;
        }
        break;
      }

    case TIM_BREAKINPUT_BRK2:
      {
        /* Check initial conditions */
        if (READ_BIT(htim->Instance->BDTR, TIM_BDTR_BK2BID) == TIM_BDTR_BK2BID)
        {
        /* Break input BRK2 is re-armed automatically by hardware. Poll to check whether fault condition disappeared */
        /* Init tickstart for timeout management */
        tickstart = HAL_GetTick();
        do
        {
          if (READ_BIT(htim->Instance->BDTR, TIM_BDTR_BK2DSRM) != TIM_BDTR_BK2DSRM)
          {
            return HAL_OK;
          }
        } while ((HAL_GetTick() - tickstart) <= TIM_BREAKINPUT_REARM_TIMEOUT);

        return HAL_TIMEOUT;
        }
        break;
      }
    default:
      break;
  }

  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup TIMEx_Exported_Functions_Group6 Extended Callbacks functions
  * @brief    Extended Callbacks functions
  *
@verbatim
  ==============================================================================
                    ##### Extended Callbacks functions #####
  ==============================================================================
  [..]
    This section provides Extended TIM callback functions:
    (+) Timer Commutation callback
    (+) Timer Break callback

@endverbatim
  * @{
  */

/**
  * @brief  Hall commutation changed callback in non-blocking mode
  * @param  htim TIM handle
  * @retval None
  */
__weak void HAL_TIMEx_CommutCallback(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_TIMEx_CommutCallback could be implemented in the user file
   */
}
/**
  * @brief  Hall commutation changed half complete callback in non-blocking mode
  * @param  htim TIM handle
  * @retval None
  */
__weak void HAL_TIMEx_CommutHalfCpltCallback(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_TIMEx_CommutHalfCpltCallback could be implemented in the user file
   */
}

/**
  * @brief  Hall Break detection callback in non-blocking mode
  * @param  htim TIM handle
  * @retval None
  */
__weak void HAL_TIMEx_BreakCallback(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_TIMEx_BreakCallback could be implemented in the user file
   */
}

/**
  * @brief  Hall Break2 detection callback in non blocking mode
  * @param  htim: TIM handle
  * @retval None
  */
__weak void HAL_TIMEx_Break2Callback(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_TIMEx_Break2Callback could be implemented in the user file
   */
}
/**
  * @}
  */

/** @defgroup TIMEx_Exported_Functions_Group7 Extended Peripheral State functions
  * @brief    Extended Peripheral State functions
  *
@verbatim
  ==============================================================================
                ##### Extended Peripheral State functions #####
  ==============================================================================
  [..]
    This subsection permits to get in run-time the status of the peripheral
    and the data flow.

@endverbatim
  * @{
  */

/**
  * @brief  Return the TIM Hall Sensor interface handle state.
  * @param  htim TIM Hall Sensor handle
  * @retval HAL state
  */
HAL_TIM_StateTypeDef HAL_TIMEx_HallSensor_GetState(TIM_HandleTypeDef *htim)
{
  return htim->State;
}

/**
  * @}
  */

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
/** @addtogroup TIMEx_Private_Functions
  * @{
  */

/**
  * @brief  TIM DMA Commutation callback.
  * @param  hdma pointer to DMA handle.
  * @retval None
  */
void TIMEx_DMACommutationCplt(DMA_HandleTypeDef *hdma)
{
  TIM_HandleTypeDef* htim = ( TIM_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  /* Change the htim state */
  htim->State= HAL_TIM_STATE_READY;

#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
  htim->CommutationCallback(htim);
#else
  HAL_TIMEx_CommutCallback(htim);
#endif /* USE_HAL_TIM_REGISTER_CALLBACKS */
}

/**
  * @brief  TIM DMA Commutation half complete callback.
  * @param  hdma pointer to DMA handle.
  * @retval None
  */
void TIMEx_DMACommutationHalfCplt(DMA_HandleTypeDef *hdma)
{
  TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

  /* Change the htim state */
  htim->State = HAL_TIM_STATE_READY;

#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
  htim->CommutationHalfCpltCallback(htim);
#else
  HAL_TIMEx_CommutHalfCpltCallback(htim);
#endif /* USE_HAL_TIM_REGISTER_CALLBACKS */
}


/**
  * @brief  Enables or disables the TIM Capture Compare Channel xN.
  * @param  TIMx to select the TIM peripheral
  * @param  Channel specifies the TIM Channel
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1
  *            @arg TIM_CHANNEL_2: TIM Channel 2
  *            @arg TIM_CHANNEL_3: TIM Channel 3
  * @param  ChannelNState specifies the TIM Channel CCxNE bit new state.
  *          This parameter can be: TIM_CCxN_ENABLE or TIM_CCxN_Disable.
  * @retval None
  */
static void TIM_CCxNChannelCmd(TIM_TypeDef* TIMx, uint32_t Channel, uint32_t ChannelNState)
{
  uint32_t tmp;

  tmp = TIM_CCER_CC1NE << (Channel & 0x1FU); /* 0x1FU = 31 bits max shift */

  /* Reset the CCxNE Bit */
  TIMx->CCER &=  ~tmp;

  /* Set or reset the CCxNE Bit */
  TIMx->CCER |= (uint32_t)(ChannelNState << (Channel & 0x1FU)); /* 0x1FU = 31 bits max shift */
}
/**
  * @}
  */

#endif /* HAL_TIM_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
