/**
  ******************************************************************************
  * @file    stm32f3xx_hal_comp.c
  * @author  MCD Application Team
  * @brief   COMP HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the COMP peripheral:
  *           + Initialization and de-initialization functions
  *           + Start/Stop operation functions in polling mode.
  *           + Start/Stop operation functions in interrupt mode.
  *           + Peripheral Control functions
  *           + Peripheral State functions
  *
  @verbatim
================================================================================
          ##### COMP Peripheral features #####
================================================================================

  [..]
      The STM32F3xx device family integrates up to 7 analog comparators COMP1, COMP2...COMP7:
      (#) The non inverting input and inverting input can be set to GPIO pins.
          For STM32F3xx devices please refer to the COMP peripheral section in corresponding
          Reference Manual.

      (#) The COMP output is available using HAL_COMP_GetOutputLevel()
          and can be set on GPIO pins.
          For STM32F3xx devices please refer to the COMP peripheral section in corresponding
          Reference Manual.

      (#) The COMP output can be redirected to embedded timers (TIM1, TIM2, TIM3...).
          For STM32F3xx devices please refer to the COMP peripheral section in corresponding
          Reference Manual.

      (#) Each couple of comparators COMP1 and COMP2, COMP3 and COMP4, COMP5 and COMP6 can be combined in window
          mode and respectively COMP1, COMP3 and COMP5 non inverting input is used as common non-inverting input.

      (#) The seven comparators have interrupt capability with wake-up
          from Sleep and Stop modes (through the EXTI controller):
          (++) COMP1 is internally connected to EXTI Line 21
          (++) COMP2 is internally connected to EXTI Line 22
          (++) COMP3 is internally connected to EXTI Line 29
          (++) COMP4 is internally connected to EXTI Line 30
          (++) COMP5 is internally connected to EXTI Line 31
          (++) COMP6 is internally connected to EXTI Line 32
          (++) COMP7 is internally connected to EXTI Line 33.

          From the corresponding IRQ handler, the right interrupt source can be retrieved with the
          adequate macro __HAL_COMP_COMPx_EXTI_GET_FLAG().


            ##### How to use this driver #####
================================================================================
  [..]
      This driver provides functions to configure and program the Comparators of all STM32F3xx devices.

      To use the comparator, perform the following steps:

      (#) Fill in the HAL_COMP_MspInit() to
      (++) Configure the comparator input in analog mode using HAL_GPIO_Init()
      (++) Configure the comparator output in alternate function mode using HAL_GPIO_Init() to map the comparator
           output to the GPIO pin
      (++) If required enable the COMP interrupt (EXTI line Interrupt): by configuring and enabling EXTI line in Interrupt mode and
           selecting the desired sensitivity level using HAL_GPIO_Init() function. After that enable the comparator
           interrupt vector using HAL_NVIC_SetPriority() and HAL_NVIC_EnableIRQ() functions.

      (#) Configure the comparator using HAL_COMP_Init() function:
      (++) Select the inverting input (input minus)
      (++) Select the non-inverting input (input plus)
      (++) Select the output polarity
      (++) Select the output redirection
      (++) Select the hysteresis level
      (++) Select the power mode
      (++) Select the event/interrupt mode

      -@@- HAL_COMP_Init() calls internally __HAL_RCC_SYSCFG_CLK_ENABLE() in order
          to enable the comparator(s).

      (#) On-the-fly reconfiguration of comparator(s) may be done by calling again HAL_COMP_Init(
          function with new input parameter values; HAL_COMP_MspInit() function shall be adapted
          to support multi configurations.

      (#) Enable the comparator using HAL_COMP_Start() or HAL_COMP_Start_IT() functions.

      (#) Use HAL_COMP_TriggerCallback() and/or HAL_COMP_GetOutputLevel() functions
          to manage comparator outputs (events and output level).

      (#) Disable the comparator using HAL_COMP_Stop() or HAL_COMP_Stop_IT()
          function.

      (#) De-initialize the comparator using HAL_COMP_DeInit() function.

      (#) For safety purposes comparator(s) can be locked using HAL_COMP_Lock() function.
          Only a MCU reset can reset that protection.

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

/*
  Additional Tables:

    Table 1. COMP Inputs for the STM32F303xB/STM32F303xC/STM32F303xE devices
    +------------------------------------------------------------------------------------------+
    |                 |                | COMP1 | COMP2 | COMP3 | COMP4 | COMP5 | COMP6 | COMP7 |
    |-----------------|----------------|---------------|---------------------------------------|
    |                 | 1U/4 VREFINT    |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |
    |                 | 1U/2 VREFINT    |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |
    |                 | 3U/4 VREFINT    |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |
    | Inverting Input | VREFINT        |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |
    |                 | DAC1 OUT (PA4) |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |
    |                 | DAC2 OUT (PA5) |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |  OK   |
    |                 | IO1            |  PA0  |  PA2  |  PD15U |  PE8  |  PD13U |  PD10U |  PC0  |
    |                 | IO2            |  ---  |  ---  |  PB12U |  PB2  |  PB10U |  PB15U |  ---  |
    |-----------------|----------------|-------|-------|-------|-------|-------|-------|-------|
    |  Non Inverting  | IO1            |  PA1  |  PA7  |  PB14U |  PB0  |  PD12U |  PD11U |  PA0  |
    |    Input        | IO2            |  ---  |  PA3  |  PD14U |  PE7  |  PB13U |  PB11U |  PC1  |
    +------------------------------------------------------------------------------------------+

    Table 2. COMP Outputs for the STM32F303xB/STM32F303xC/STM32F303xE devices
    +-------------------------------------------------------+
    | COMP1 | COMP2 | COMP3 | COMP4 | COMP5 | COMP6 | COMP7 |
    |-------|-------|-------|-------|-------|-------|-------|
    |  PA0  |  PA2  |  PB1  |  PC8  |  PC7  |  PA10U |  PC2  |
    |  PF4  |  PA7  |  ---  |  PA8  |  PA9  |  PC6  |  ---  |
    |  PA6  |  PA12U |  ---  |  ---  |  ---  |  ---  |  ---  |
    |  PA11U |  PB9  |  ---  |  ---  |  ---  |  ---  |  ---  |
    |  PB8  |  ---  |  ---  |  ---  |  ---  |  ---  |  ---  |
    +-------------------------------------------------------+

    Table 3. COMP Outputs redirection to embedded timers for the STM32F303xB/STM32F303xC devices
    +----------------------------------------------------------------------------------------------------------------------+
    |     COMP1      |     COMP2      |     COMP3      |     COMP4      |     COMP5      |     COMP6      |     COMP7      |
    |----------------|----------------|----------------|----------------|----------------|----------------|----------------|
    |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN     |
    |                |                |                |                |                |                |                |
    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |
    |                |                |                |                |                |                |                |
    |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN     |
    |                |                |                |                |                |                |                |
    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |
    |                |                |                |                |                |                |                |
    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |
    |     +          |     +          |     +          |     +          |     +          |     +          |     +          |
    |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |
    |                |                |                |                |                |                |                |
    |  TIM1 OCREFCLR |  TIM1 OCREFCLR |  TIM1 OCREFCLR |  TIM8 OCREFCLR |  TIM8 OCREFCLR |  TIM8 OCREFCLR |  TIM1 OCREFCLR |
    |                |                |                |                |                |                |                |
    |  TIM1 IC1      |  TIM1 IC1      |  TIM2 OCREFCLR |  TIM3 IC3      |  TIM2 IC1      |  TIM2 IC2      |  TIM8 OCREFCLR |
    |                |                |                |                |                |                |                |
    |  TIM2 IC4      |  TIM2 IC4      |  TIM3 IC2      |  TIM3 OCREFCLR |  TIM3 OCREFCLR |  TIM2 OCREFCLR |  TIM2 IC3      |
    |                |                |                |                |                |                |                |
    |  TIM2 OCREFCLR |  TIM2 OCREFCLR |  TIM4 IC1      |  TIM4 IC2      |  TIM4 IC3      |  TIM16 OCREFCLR|  TIM1 IC2      |
    |                |                |                |                |                |                |                |
    |  TIM3 IC1      |  TIM3 IC1      |  TIM15 IC1     |  TIM15 OCREFCLR|  TIM16 BKIN    |  TIM16 IC1     |  TIM17 OCREFCLR|
    |                |                |                |                |                |                |                |
    |  TIM3 OCREFCLR |  TIM3 OCREFCLR |  TIM15 BKIN    |  TIM15 IC2     |  TIM17 IC1     |  TIM4 IC4      |  TIM17 BKIN    |
    +----------------------------------------------------------------------------------------------------------------------+

    Table 4. COMP Outputs redirection to embedded timers for the STM32F303xE devices
    +----------------------------------------------------------------------------------------------------------------------+
    |     COMP1      |     COMP2      |     COMP3      |     COMP4      |     COMP5      |     COMP6      |     COMP7      |
    |----------------|----------------|----------------|----------------|----------------|----------------|----------------|
    |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN (1U) |  TIM1 BKIN     |  TIM1 BKIN     |  TIM1 BKIN (1U) |
    |                |                |                |                |                |                |                |
    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |
    |                |                |                |                |                |                |                |
    |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN (1U) |  TIM8 BKIN     |  TIM8 BKIN     |  TIM8 BKIN (1U) |
    |                |                |                |                |                |                |                |
    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |
    |                |                |                |                |                |                |                |
    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |
    |     +          |     +          |     +          |     +          |     +          |     +          |     +          |
    |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |  TIM8BKIN2     |
    |                |                |                |                |                |                |                |
    |  TIM1 OCREFCLR |  TIM1 OCREFCLR |  TIM1 OCREFCLR |  TIM8 OCREFCLR |  TIM8 OCREFCLR |  TIM8 OCREFCLR |  TIM1 OCREFCLR |
    |                |                |                |                |                |                |                |
    |  TIM1 IC1      |  TIM1 IC1      |  TIM2 OCREFCLR |  TIM3 IC3      |  TIM2 IC1      |  TIM2 IC2      |  TIM8 OCREFCLR |
    |                |                |                |                |                |                |                |
    |  TIM2 IC4      |  TIM2 IC4      |  TIM3 IC2      |  TIM3 OCREFCLR |  TIM3 OCREFCLR |  TIM2 OCREFCLR |  TIM2 IC3      |
    |                |                |                |                |                |                |                |
    |  TIM2 OCREFCLR |  TIM2 OCREFCLR |  TIM4 IC1      |  TIM4 IC2      |  TIM4 IC3      |  TIM16 OCREFCLR|  TIM1 IC2      |
    |                |                |                |                |                |                |                |
    |  TIM3 IC1      |  TIM3 IC1      |  TIM15 IC1     |  TIM15 OCREFCLR|  TIM16 BKIN    |  TIM16 IC1     |  TIM17 OCREFCLR|
    |                |                |                |                |                |                |                |
    |  TIM3 OCREFCLR |  TIM3 OCREFCLR |  TIM15 BKIN    |  TIM15 IC2     |  TIM17 IC1     |  TIM4 IC4      |  TIM17 BKIN    |
    |                |                |                |                |                |                |                |
    |  TIM20 BKIN    |  TIM20 BKIN    |  TIM20 BKIN    |  TIM20 BKIN (1U)|  TIM20 BKIN    |  TIM20 BKIN    |  TIM20 BKIN (1U)|
    |                |                |                |                |                |                |                |
    |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |
    |                |                |                |                |                |                |                |
    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |  TIM1 BKIN2    |
    |     +          |     +          |     +          |     +          |     +          |     +          |     +          |
    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |  TIM8 BKIN2    |
    |     +          |     +          |     +          |     +          |     +          |     +          |     +          |
    |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |  TIM20 BKIN2   |
    |                |                |                |                |                |                |                |
    +----------------------------------------------------------------------------------------------------------------------+
    (1U):  This connection consists of connecting both GPIO and COMP output to TIM1/8U/20 BRK input through an OR gate, instead
          of connecting the GPIO to the TIM1/8U/20 BRK input and the COMP output to the TIM1/8U/20 BRK_ACTH input. The aim is to
          add a digital filter (3  bits) on the COMP output.

    Table 5. COMP Outputs blanking sources for the STM32F303xB/STM32F303xC/STM32F303xE devices
    +----------------------------------------------------------------------------------------------------------------------+
    |     COMP1      |     COMP2      |     COMP3      |     COMP4      |     COMP5      |     COMP6      |     COMP7      |
    |----------------|----------------|----------------|----------------|----------------|----------------|----------------|
    |  TIM1 OC5      |  TIM1 OC5      |  TIM1 OC5      |  TIM3 OC4      |  --------      |  TIM8 OC5      |  TIM1 OC5      |
    |                |                |                |                |                |                |                |
    |  TIM2 OC3      |  TIM2 OC3      |  --------      |  TIM8 OC5      |  TIM3 OC3      |  TIM2 OC4      |  TIM8 OC5      |
    |                |                |                |                |                |                |                |
    |  TIM3 OC3      |  TIM3 OC3      |  TIM2 OC4      |  TIM15 OC1     |  TIM8 OC5      |  TIM15 OC2     |  TIM15 OC2     |
    |                |                |                |                |                |                |                |
    +----------------------------------------------------------------------------------------------------------------------+

*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @defgroup COMP COMP
  * @brief COMP HAL module driver
  * @{
  */

#ifdef HAL_COMP_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup COMP_Private_Constants COMP Private Constants
  * @{
  */
#define COMP_LOCK_DISABLE                      (0x00000000U)
#define COMP_LOCK_ENABLE                       COMP_CSR_COMPxLOCK
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup COMP_Exported_Functions COMP Exported Functions
  * @{
  */

/** @defgroup COMP_Exported_Functions_Group1 Initialization/de-initialization functions
 *  @brief    Initialization and de-initialization functions.
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]  This section provides functions to initialize and de-initialize comparators.

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the COMP peripheral according to the specified
  *         parameters in the COMP_InitTypeDef and initialize the associated handle.
  * @note   If the selected comparator is locked, initialization cannot be performed.
  *         To unlock the configuration, perform a system reset.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Init(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || ((hcomp->State & COMP_STATE_BIT_LOCK) != RESET))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameters */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));
    assert_param(IS_COMP_INVERTINGINPUT(hcomp->Init.InvertingInput));
    assert_param(IS_COMP_NONINVERTINGINPUT(hcomp->Init.NonInvertingInput));
    assert_param(IS_COMP_NONINVERTINGINPUT_INSTANCE(hcomp->Instance, hcomp->Init.NonInvertingInput));
    assert_param(IS_COMP_OUTPUT(hcomp->Init.Output));
    assert_param(IS_COMP_OUTPUT_INSTANCE(hcomp->Instance, hcomp->Init.Output));
    assert_param(IS_COMP_OUTPUTPOL(hcomp->Init.OutputPol));
    assert_param(IS_COMP_HYSTERESIS(hcomp->Init.Hysteresis));
    assert_param(IS_COMP_MODE(hcomp->Init.Mode));
    assert_param(IS_COMP_BLANKINGSRCE(hcomp->Init.BlankingSrce));
    assert_param(IS_COMP_BLANKINGSRCE_INSTANCE(hcomp->Instance, hcomp->Init.BlankingSrce));
    assert_param(IS_COMP_TRIGGERMODE(hcomp->Init.TriggerMode));

    if(hcomp->Init.WindowMode != COMP_WINDOWMODE_DISABLE)
    {
      assert_param(IS_COMP_WINDOWMODE_INSTANCE(hcomp->Instance));
      assert_param(IS_COMP_WINDOWMODE(hcomp->Init.WindowMode));
    }

    /* Init SYSCFG and the low level hardware to access comparators */
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    /* Init the low level hardware : SYSCFG to access comparators */
    HAL_COMP_MspInit(hcomp);

    if(hcomp->State == HAL_COMP_STATE_RESET)
    {
      /* Allocate lock resource and initialize it */
      hcomp->Lock = HAL_UNLOCKED;
    }

    /* Manage inverting input comparator inverting input connected to a GPIO  */
    /* for STM32F302x, STM32F32xx, STM32F33x.                                 */
    hcomp->Init.InvertingInput = COMP_INVERTINGINPUT_SELECTION(hcomp->Instance, hcomp->Init.InvertingInput);

    /* Set COMP parameters */
    /*     Set COMPxINSEL bits according to hcomp->Init.InvertingInput value        */
    /*     Set COMPxNONINSEL bits according to hcomp->Init.NonInvertingInput value  */
    /*     Set COMPxBLANKING bits according to hcomp->Init.BlankingSrce value       */
    /*     Set COMPxOUTSEL bits according to hcomp->Init.Output value               */
    /*     Set COMPxPOL bit according to hcomp->Init.OutputPol value                */
    /*     Set COMPxHYST bits according to hcomp->Init.Hysteresis value             */
    /*     Set COMPxMODE bits according to hcomp->Init.Mode value                   */
    COMP_INIT(hcomp);

    /* Initialize the COMP state*/
    hcomp->State = HAL_COMP_STATE_READY;
  }

  return status;
}

/**
  * @brief  DeInitialize the COMP peripheral.
  * @note   If the selected comparator is locked, deinitialization cannot be performed.
  *         To unlock the configuration, perform a system reset.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_DeInit(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || ((hcomp->State & COMP_STATE_BIT_LOCK) != RESET))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

    /* Set COMP_CSR register to reset value */
    COMP_DEINIT(hcomp);

    /* DeInit the low level hardware: SYSCFG, GPIO, CLOCK and NVIC */
    HAL_COMP_MspDeInit(hcomp);

    hcomp->State = HAL_COMP_STATE_RESET;

    /* Release Lock */
    __HAL_UNLOCK(hcomp);
  }

  return status;
}

/**
  * @brief  Initialize the COMP MSP.
  * @param  hcomp  COMP handle
  * @retval None
  */
__weak void HAL_COMP_MspInit(COMP_HandleTypeDef *hcomp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcomp);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_COMP_MspInit could be implemented in the user file
   */
}

/**
  * @brief  DeInitialize the COMP MSP.
  * @param  hcomp  COMP handle
  * @retval None
  */
__weak void HAL_COMP_MspDeInit(COMP_HandleTypeDef *hcomp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcomp);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_COMP_MspDeInit could be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup COMP_Exported_Functions_Group2 Start Stop operation functions
 *  @brief   Start-Stop operation functions.
 *
@verbatim
 ===============================================================================
                      ##### Start Stop operation functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Start a comparator without interrupt generation.
      (+) Stop a comparator without interrupt generation.
      (+) Start a comparator with interrupt generation.
      (+) Stop a comparator with interrupt generation.
      (+) Handle interrupts from a comparator with associated callback function.

@endverbatim
  * @{
  */

/**
  * @brief  Start the comparator.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Start(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t extiline = 0U;

  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || ((hcomp->State & COMP_STATE_BIT_LOCK) != RESET))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

    if(hcomp->State == HAL_COMP_STATE_READY)
    {
      /* Get the EXTI Line output configuration */
      extiline = COMP_GET_EXTI_LINE(hcomp->Instance);

      /* Configure the event generation */
      if((hcomp->Init.TriggerMode & (COMP_TRIGGERMODE_EVENT_RISING|COMP_TRIGGERMODE_EVENT_FALLING)) != RESET)
      {
        /* Configure the event trigger rising edge */
        if((hcomp->Init.TriggerMode & COMP_TRIGGERMODE_EVENT_RISING) != RESET)
        {
          COMP_EXTI_RISING_ENABLE(extiline);
        }
        else
        {
          COMP_EXTI_RISING_DISABLE(extiline);
        }

        /* Configure the trigger falling edge */
        if((hcomp->Init.TriggerMode & COMP_TRIGGERMODE_EVENT_FALLING) != RESET)
        {
          COMP_EXTI_FALLING_ENABLE(extiline);
        }
        else
        {
          COMP_EXTI_FALLING_DISABLE(extiline);
        }

        /* Enable EXTI event generation */
        COMP_EXTI_ENABLE_EVENT(extiline);

        /* Clear COMP EXTI pending bit */
        COMP_EXTI_CLEAR_FLAG(extiline);
      }

      /* Enable the selected comparator */
      __HAL_COMP_ENABLE(hcomp);

      hcomp->State = HAL_COMP_STATE_BUSY;
    }
    else
    {
      status = HAL_ERROR;
    }
  }

  return status;
}

/**
  * @brief  Stop the comparator.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Stop(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || ((hcomp->State & COMP_STATE_BIT_LOCK) != RESET))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

    if(hcomp->State == HAL_COMP_STATE_BUSY)
    {
      /* Disable the EXTI Line event mode if any */
      COMP_EXTI_DISABLE_EVENT(COMP_GET_EXTI_LINE(hcomp->Instance));

      /* Disable the selected comparator */
      __HAL_COMP_DISABLE(hcomp);

      hcomp->State = HAL_COMP_STATE_READY;
    }
    else
    {
      status = HAL_ERROR;
    }
  }

  return status;
}

/**
  * @brief  Start the comparator in Interrupt mode.
  * @param  hcomp  COMP handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_COMP_Start_IT(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t extiline = 0U;

  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || ((hcomp->State & COMP_STATE_BIT_LOCK) != RESET))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

    if(hcomp->State == HAL_COMP_STATE_READY)
    {
      /* Configure the EXTI event generation */
      if((hcomp->Init.TriggerMode & (COMP_TRIGGERMODE_IT_RISING|COMP_TRIGGERMODE_IT_FALLING)) != RESET)
      {
        /* Get the EXTI Line output configuration */
        extiline = COMP_GET_EXTI_LINE(hcomp->Instance);

        /* Configure the trigger rising edge */
        if((hcomp->Init.TriggerMode & COMP_TRIGGERMODE_IT_RISING) != RESET)
        {
          COMP_EXTI_RISING_ENABLE(extiline);
        }
        else
        {
          COMP_EXTI_RISING_DISABLE(extiline);
        }
        /* Configure the trigger falling edge */
        if((hcomp->Init.TriggerMode & COMP_TRIGGERMODE_IT_FALLING) != RESET)
        {
          COMP_EXTI_FALLING_ENABLE(extiline);
        }
        else
        {
          COMP_EXTI_FALLING_DISABLE(extiline);
        }

        /* Clear COMP EXTI pending bit if any */
        COMP_EXTI_CLEAR_FLAG(extiline);

        /* Enable EXTI interrupt mode */
        COMP_EXTI_ENABLE_IT(extiline);

        /* Enable the selected comparator */
        __HAL_COMP_ENABLE(hcomp);

        hcomp->State = HAL_COMP_STATE_BUSY;
      }
      else
      {
        status = HAL_ERROR;
      }
    }
    else
    {
      status = HAL_ERROR;
    }
  }

  return status;
}

/**
  * @brief  Stop the comparator in Interrupt mode.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Stop_IT(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Disable the EXTI Line interrupt mode */
  COMP_EXTI_DISABLE_IT(COMP_GET_EXTI_LINE(hcomp->Instance));

  status = HAL_COMP_Stop(hcomp);

  return status;
}

/**
  * @brief  Comparator IRQ Handler.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
void HAL_COMP_IRQHandler(COMP_HandleTypeDef *hcomp)
{
  uint32_t extiline = COMP_GET_EXTI_LINE(hcomp->Instance);

  /* Check COMP EXTI flag */
  if(COMP_EXTI_GET_FLAG(extiline) != RESET)
  {
    /* Clear COMP EXTI pending bit */
    COMP_EXTI_CLEAR_FLAG(extiline);

    /* COMP trigger user callback */
    HAL_COMP_TriggerCallback(hcomp);
  }
}

/**
  * @brief  Comparator callback.
  * @param  hcomp  COMP handle
  * @retval None
  */
__weak void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcomp);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_COMP_TriggerCallback should be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup COMP_Exported_Functions_Group3 Peripheral Control functions
 *  @brief   Management functions.
 *
@verbatim
 ===============================================================================
                      ##### Peripheral Control functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to control the comparators.

@endverbatim
  * @{
  */

/**
  * @brief  Lock the selected comparator configuration.
  * @note   A system reset is required to unlock the comparator configuration.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Lock(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || ((hcomp->State & COMP_STATE_BIT_LOCK) != RESET))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

    /* Set lock flag on state */
    switch(hcomp->State)
    {
    case HAL_COMP_STATE_BUSY:
      hcomp->State = HAL_COMP_STATE_BUSY_LOCKED;
      break;
    case HAL_COMP_STATE_READY:
      hcomp->State = HAL_COMP_STATE_READY_LOCKED;
      break;
    default:
      /* unexpected state */
      status = HAL_ERROR;
      break;
    }
  }

  if(status == HAL_OK)
  {
    /* Set the lock bit corresponding to selected comparator */
    __HAL_COMP_LOCK(hcomp);
  }

  return status;
}

/**
  * @brief  Return the output level (high or low) of the selected comparator.
  *         The output level depends on the selected polarity.
  *         If the polarity is not inverted:
  *           - Comparator output is low when the non-inverting input is at a lower
  *             voltage than the inverting input
  *           - Comparator output is high when the non-inverting input is at a higher
  *             voltage than the inverting input
  *         If the polarity is inverted:
  *           - Comparator output is high when the non-inverting input is at a lower
  *             voltage than the inverting input
  *           - Comparator output is low when the non-inverting input is at a higher
  *             voltage than the inverting input
  * @param  hcomp  COMP handle
  * @retval Returns the selected comparator output level:
  *         @arg @ref COMP_OUTPUTLEVEL_LOW
  *         @arg @ref COMP_OUTPUTLEVEL_HIGH
  *
  */
uint32_t HAL_COMP_GetOutputLevel(COMP_HandleTypeDef *hcomp)
{
  uint32_t level=0U;

  /* Check the parameter */
  assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

  level = READ_BIT(hcomp->Instance->CSR, COMP_CSR_COMPxOUT);

  if(level != 0U)
  {
    return(COMP_OUTPUTLEVEL_HIGH);
  }
  return(COMP_OUTPUTLEVEL_LOW);
}

/**
  * @}
  */

/** @defgroup COMP_Exported_Functions_Group4 Peripheral State functions
 *  @brief   Peripheral State functions.
 *
@verbatim
 ===============================================================================
                      ##### Peripheral State functions #####
 ===============================================================================
    [..]
    This subsection permits to get in run-time the status of the peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Return the COMP handle state.
  * @param  hcomp  COMP handle
  * @retval HAL state
  */
HAL_COMP_StateTypeDef HAL_COMP_GetState(COMP_HandleTypeDef *hcomp)
{
  /* Check the COMP handle allocation */
  if(hcomp == NULL)
  {
    return HAL_COMP_STATE_RESET;
  }

  /* Check the parameter */
  assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

  /* Return COMP handle state */
  return hcomp->State;
}
/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_COMP_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
