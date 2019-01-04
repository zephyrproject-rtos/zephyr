/**
  ******************************************************************************
  * @file    stm32l1xx_hal_comp.c
  * @author  MCD Application Team
  * @brief   COMP HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities of the COMP peripheral:
  *           + Initialization and de-initialization functions
  *           + I/O operation functions
  *           + Peripheral Control functions 
  *           + Peripheral State functions
  *         
  @verbatim
================================================================================
          ##### COMP Peripheral features #####
================================================================================
  [..]       
      The STM32L1xx device family integrates 2 analog comparators COMP1 and 
      COMP2:
      (#) The non inverting input and inverting input can be set to GPIO pins.
          HAL COMP driver configures the Routing Interface (RI) to connect the
          selected I/O pins to comparator input.
          Caution: Comparator COMP1 and ADC cannot be used at the same time as 
          ADC since they share the ADC switch matrix: COMP1 non-inverting 
          input is routed through ADC switch matrix. Except if ADC is intended 
          to measure voltage on COMP1 non-inverting input: it can be performed 
          on ADC channel VCOMP.

      (#) The COMP output is available using HAL_COMP_GetOutputLevel().

      (#) The COMP output can be redirected to embedded timers (TIM2, TIM3, 
          TIM4, TIM10).
          COMP output cannot be redirected to any I/O pin.

      (#) The comparators COMP1 and COMP2 can be combined in window mode. 
          In this mode, COMP2 non inverting input is used as common 
          non-inverting input.
  
      (#) The 2 comparators have interrupt capability with wake-up
          from Sleep and Stop modes (through the EXTI controller):
          (++) COMP1 is internally connected to EXTI Line 21
          (++) COMP2 is internally connected to EXTI Line 22

          From the corresponding IRQ handler, the right interrupt source can be retrieved with the 
          macros __HAL_COMP_COMP1_EXTI_GET_FLAG() and __HAL_COMP_COMP2_EXTI_GET_FLAG().
  
      (#) The comparators also offer the possibility to output the voltage 
          reference (VrefInt), used on inverting inputs, on I/O pin through 
          a buffer. To use it, refer to macro "__HAL_SYSCFG_VREFINT_OUT_ENABLE()".

            ##### How to use this driver #####
================================================================================
  [..]
      This driver provides functions to configure and program the Comparators of all STM32L1xx devices.

      To use the comparator, perform the following steps:
  
      (#)  Initialize the COMP low level resources by implementing the HAL_COMP_MspInit().
      (++) Configure the comparator input I/O pin using HAL_GPIO_Init():
           - For all inputs: I/O pin in analog mode (Schmitt trigger disabled)
           - Possible alternate configuration, for non-inverting inputs of comparator 2: I/O pin in floating mode (Schmitt trigger enabled).
           It is recommended to use analog configuration to avoid any overconsumption around VDD/2.
      (++) Enable COMP Peripheral clock using macro __HAL_RCC_COMP_CLK_ENABLE()
      (++) If required enable the COMP interrupt (EXTI line Interrupt): enable
           the comparator interrupt vector using HAL_NVIC_EnableIRQ(COMP_IRQn)
           and HAL_NVIC_SetPriority(COMP_IRQn, xxx, xxx) functions.
  
      (#) Configure the comparator using HAL_COMP_Init() function:
      (++) Select the inverting input (COMP2 only)
      (++) Select the non-inverting input
      (++) Select the output redirection to timers (COMP2 only)
      (++) Select the speed mode (COMP2 only)
      (++) Select the window mode (related to COMP1 and COMP2, but selected 
           by COMP2 only)
      (++) Select the pull-up/down resistors on non-inverting input (COMP1 only)

      (#) Enable the comparator using HAL_COMP_Start() or HAL_COMP_Start_IT() 
          function
    
      (#) If needed, use HAL_COMP_GetOutputLevel() or HAL_COMP_TriggerCallback()
          functions to manage comparator actions (output level or events)

      (#) Disable the comparator using HAL_COMP_Stop() or HAL_COMP_Stop_IT() 
          function

      (#) De-initialize the comparator using HAL_COMP_DeInit() function


  @endverbatim
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

/*
  Additionnal remark: 
    Table 1. COMP Inputs for the STM32L1xx devices
    +----------------------------------------------------------------------+
    |                 |                                |  COMP1  |  COMP2  |
    |-----------------|--------------------------------|---------|---------|
    |                 | 1/4 VREFINT                    |   --    |   OK    |
    |                 | 1/2 VREFINT                    |   --    |   OK    |
    |                 | 3/4 VREFINT                    |   --    |   OK    |
    | Inverting       | VREFINT                        |   OK    |   OK    |
    | input           | DAC Ch1 OUT (PA4)              |   --    |   OK    |
    |                 | DAC Ch2 OUT (PA5)              |   --    |   OK    |
    |                 | IO: PB3                        |   --    |   OK    |
    |-----------------|--------------------------------|---------|---------|
    |                 | IO:                            |         |         |
    |                 |   PB4, 5, 6*, 7*               |   ---   |   OK    |
    | Non-inverting   |   PA0*, 1*, 2*, 3*, 4, 5, 6, 7 |   OK    |   ---   |
    | input           |   PB0, 1, 12, 13, 14, 15       |   OK    |   ---   |
    |                 |   PC0, 1, 2, 3, 4, 5           |   OK    |   ---   |
    |                 |   PE7, 8, 9, 10                |   OK    |   ---   |
    |                 |   PF6, 7, 8, 9, 10             |   OK    |   ---   |
    |                 | OPAMP1 output                  |   OK    |   ---   |
    |                 | OPAMP2 output                  |   OK    |   ---   |
    |                 | OPAMP3 output**                |   OK    |   ---   |
    +----------------------------------------------------------------------+
    *: Available on devices category Cat.3, Cat.4, Cat.5 only. 
    **: Available on devices category Cat.4 only. 
    
    [..] Table 2. COMP Outputs redirection to embedded timers
    +-----------------------------------+     
    |      COMP1      |      COMP2      |
    |-----------------|-----------------|
    |                 |  TIM2 IC4       |
    |                 |  TIM2 OCREF CLR |
    | (no redirection |  TIM3 IC4       |
    |   to timers)    |  TIM3 OCREF CLR |
    |                 |  TIM4 IC4       |
    |                 |  TIM4 OCREF CLR |
    |                 |  TIM10 IC1      |
    +-----------------------------------+
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/** @addtogroup STM32L1xx_HAL_Driver
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
  /* Delay for COMP start-up time.                                            */
  /* Maximum delay is 10us for comparator 1 and 25us for comparator 2 in slow */
  /* mode (refer to device datasheet, parameter tSTART).                      */
  /* Delay in CPU cycles, fixed to worst case: maximum CPU frequency 32MHz to */
  /* have the minimum number of CPU cycles to fulfill this delay.             */
  /*  - Comparator 1: delay minimum of 320 CPU cycles. Wait loop takes 3 CPU  */
  /*                 cycles per iteration, therefore total wait iterations    */
  /*                 number must be initialized at 106 iterations.            */
  /*  - Comparator 2: delay minimum of 800 CPU cycles. Wait loop takes 3 CPU  */
  /*                 cycles per iteration, therefore total wait iterations    */
  /*                 number must be initialized at 266 iterations.            */
#define COMP1_START_DELAY_CPU_CYCLES       (106U)
#define COMP2_START_DELAY_CPU_CYCLES       (266U)

  /* Comparator status "locked": to update COMP handle state (software lock   */
  /* only on COMP of STM32L1xx devices) by bitfield:                          */
  /* states HAL_COMP_STATE_READY_LOCKED, HAL_COMP_STATE_BUSY_LOCKED.          */
#define COMP_STATE_BIT_LOCK     (0x00000010U)  

/**
  * @}
  */


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup COMP_Exported_Functions COMP Exported Functions
  * @{
  */

/** @defgroup COMP_Exported_Functions_Group1 Initialization and de-initialization functions 
 *  @brief    Initialization and Configuration functions 
 *
@verbatim    
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]  This section provides functions to initialize and de-initialize comparators 

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the COMP according to the specified
  *         parameters in the COMP_InitTypeDef and create the associated handle.
  * @note   If the selected comparator is locked, initialization can't be performed.
  *         To unlock the configuration, perform a system reset.
  * @param  hcomp: COMP handle
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
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

    if (hcomp->Instance == COMP1)
    {
      assert_param(IS_COMP_NONINVERTINGINPUTPULL(hcomp->Init.NonInvertingInputPull));
    }
    else /* if (hcomp->Instance == COMP2) */
    {
      assert_param(IS_COMP_INVERTINGINPUT(hcomp->Init.InvertingInput));
      assert_param(IS_COMP_OUTPUT(hcomp->Init.Output));
      assert_param(IS_COMP_MODE(hcomp->Init.Mode));
      assert_param(IS_COMP_WINDOWMODE(hcomp->Init.WindowMode));
    }
  
    /* In window mode, non-inverting inputs of the 2 comparators are          */
    /* connected together and are using inputs of COMP2 only. If COMP1 is     */
    /* selected, this parameter is discarded.                                 */
    if ((hcomp->Init.WindowMode == COMP_WINDOWMODE_DISABLE) ||
        (hcomp->Instance == COMP2)                            )
    {
      assert_param(IS_COMP_NONINVERTINGINPUT(hcomp->Init.NonInvertingInput));
    }
    
      
    /* Enable SYSCFG clock and the low level hardware to access comparators */
    if(hcomp->State == HAL_COMP_STATE_RESET)
    {
      /* Allocate lock resource and initialize it */
      hcomp->Lock = HAL_UNLOCKED;

      /* Enable SYSCFG clock to control the routing Interface (RI) */
      __HAL_RCC_SYSCFG_CLK_ENABLE();
      
      /* Init the low level hardware */
      HAL_COMP_MspInit(hcomp);
    }
  
    /* Configuration of comparator:                                           */
    /*  - Output selection                                                    */
    /*  - Inverting input selection                                           */
    /*  - Window mode                                                         */
    /*  - Mode fast/slow speed                                                */
    /*  - Inverting input pull-up/down resistors                              */
    
    /* Configuration depending on comparator instance */
    if (hcomp->Instance == COMP1)
    {
      MODIFY_REG(COMP->CSR, COMP_CSR_400KPD | COMP_CSR_10KPD | COMP_CSR_400KPU | COMP_CSR_10KPU,
                            hcomp->Init.NonInvertingInputPull                                   );
    }
    else /* if (hcomp->Instance == COMP2) */
    {
      /* Note: If comparator 2 is not enabled, inverting input (parameter     */
      /*       "hcomp->Init.InvertingInput") is configured into function      */
      /*       "HAL_COMP_Start()" since inverting  input selection also       */
      /*       enables the comparator 2.                                      */
      /*       If comparator 2 is already enabled, inverting input is         */
      /*       reconfigured on the fly.                                       */
      if (__COMP_IS_ENABLED(hcomp) == RESET)
      {
        MODIFY_REG(COMP->CSR, COMP_CSR_OUTSEL  |
                              COMP_CSR_WNDWE   |
                              COMP_CSR_SPEED          ,
                              hcomp->Init.Output     |
                              hcomp->Init.WindowMode |
                              hcomp->Init.Mode        );
      }
      else
      {
        MODIFY_REG(COMP->CSR, COMP_CSR_OUTSEL  |
                              COMP_CSR_INSEL   |
                              COMP_CSR_WNDWE   |
                              COMP_CSR_SPEED              ,
                              hcomp->Init.Output         |
                              hcomp->Init.InvertingInput |
                              hcomp->Init.WindowMode     |
                              hcomp->Init.Mode            );
      }
    }
    
    /* Configure Routing Interface (RI) switches for comparator non-inverting */
    /* input.                                                                 */
    /* Except in 2 cases:                                                     */
    /* - if non-inverting input has no selection: it can be the case for      */
    /*   COMP1 in window mode.                                                */
    /* - particular case for PC3: if switch COMP1_SW1 is closed               */
    /*   (by macro "__HAL_OPAMP_OPAMP3OUT_CONNECT_ADC_COMP1()" or             */
    /*   "__HAL_RI_SWITCH_COMP1_SW1_CLOSE()"), connection between pin PC3     */
    /*    (or OPAMP3, if available) and COMP1 is done directly, without going */
    /*    through ADC switch matrix.                                          */
    if (__COMP_ROUTING_INTERFACE_TOBECONFIGURED(hcomp))
    {
      if (hcomp->Instance == COMP1)
      {
        /* Enable the switch control mode */
        __HAL_RI_SWITCHCONTROLMODE_ENABLE();

        /* Close the analog switch of ADC switch matrix to COMP1 (ADC         */
        /* channel 26: Vcomp)                                                 */
        __HAL_RI_IOSWITCH_CLOSE(RI_IOSWITCH_VCOMP);
      }

      /* Close the I/O analog switch corresponding to comparator              */
      /* non-inverting input selected.                                        */
      __HAL_RI_IOSWITCH_CLOSE(hcomp->Init.NonInvertingInput);
    }

    
    /* Initialize the COMP state*/
    if(hcomp->State == HAL_COMP_STATE_RESET)
    {
      hcomp->State = HAL_COMP_STATE_READY;
    }
  }
  
  return status;
}


/**
  * @brief  DeInitializes the COMP peripheral 
  * @note   Deinitialization can't be performed if the COMP configuration is locked.
  *         To unlock the configuration, perform a system reset.
  * @param  hcomp: COMP handle
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
    
    /* Reset configuration depending on comparator instance */
    if (hcomp->Instance == COMP1)
    {
      CLEAR_BIT(COMP->CSR , COMP_CSR_400KPD | COMP_CSR_10KPD | COMP_CSR_400KPU | COMP_CSR_10KPU);
    }
    else /* if (hcomp->Instance == COMP2) */
    {
      CLEAR_BIT(COMP->CSR , COMP_CSR_OUTSEL |
                            COMP_CSR_WNDWE  |
                            COMP_CSR_INSEL  |
                            COMP_CSR_SPEED   );
    }
    
    
    /* Restore default state of Routing Interface (RI) switches for           */
    /* comparator non-inverting input.                                        */
    if (hcomp->Init.NonInvertingInput != COMP_NONINVERTINGINPUT_NONE)
    {
      /* Open the I/O analog switch corresponding to comparator               */
      /* non-inverting input selected.                                        */
      __HAL_RI_IOSWITCH_OPEN(hcomp->Init.NonInvertingInput);
    }
    if (hcomp->Instance == COMP1)
    {
      /* Open the analog switch of ADC switch matrix to COMP1 (ADC            */
      /* channel 26: Vcomp)                                                   */
      __HAL_RI_IOSWITCH_OPEN(RI_IOSWITCH_VCOMP);
      
      /* Disable the switch control mode */
      __HAL_RI_SWITCHCONTROLMODE_DISABLE();
    }
  
    
    /* DeInit the low level hardware: SYSCFG, GPIO, CLOCK and NVIC */
    HAL_COMP_MspDeInit(hcomp);

    hcomp->State = HAL_COMP_STATE_RESET;
    
    /* Process unlocked */
    __HAL_UNLOCK(hcomp);
  }
  
  return status;
}

/**
  * @brief  Initializes the COMP MSP.
  * @param  hcomp: COMP handle
  * @retval None
  */
__weak void HAL_COMP_MspInit(COMP_HandleTypeDef *hcomp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcomp);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_COMP_MspInit could be implenetd in the user file
   */
}

/**
  * @brief  DeInitializes COMP MSP.
  * @param  hcomp: COMP handle
  * @retval None
  */
__weak void HAL_COMP_MspDeInit(COMP_HandleTypeDef *hcomp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hcomp);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_COMP_MspDeInit could be implenetd in the user file
   */
}

/**
  * @}
  */

/** @defgroup COMP_Exported_Functions_Group2 I/O operation functions 
 *  @brief   I/O operation functions
 *
@verbatim   
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions allowing to manage the COMP 
    start and stop actions with or without interruption on ExtI line.

@endverbatim
  * @{
  */

/**
  * @brief  Start the comparator 
  * @param  hcomp: COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Start(COMP_HandleTypeDef *hcomp)
{ 
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t wait_loop_cycles = 0;
  __IO uint32_t wait_loop_index = 0;
  
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
      
      /* Note: For comparator 2, inverting input (parameter                   */
      /*       "hcomp->Init.InvertingInput") is configured into this          */
      /*       function instead of function "HAL_COMP_Init()" since           */
      /*       inverting input selection also enables the comparator 2.       */
      __HAL_COMP_ENABLE(hcomp);

      /* Set delay for COMP start-up time */
      if (hcomp->Instance == COMP1)
      {
        wait_loop_cycles = COMP1_START_DELAY_CPU_CYCLES;
      }
      else /* if (hcomp->Instance == COMP2) */
      {
        wait_loop_cycles = COMP2_START_DELAY_CPU_CYCLES;
      }

      /* Delay for COMP start-up time.                                         */
      /* Delay fixed to worst case: maximum CPU frequency                     */
      while(wait_loop_index < wait_loop_cycles)
      {
        wait_loop_index++;
      }

      /* Update COMP state */
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
  * @brief  Stop the comparator 
  * @param  hcomp: COMP handle
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
      /* Disable the selected comparator */
      __HAL_COMP_DISABLE(hcomp);
      
      /* Update COMP state */
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
  * @brief  Enables the interrupt and starts the comparator
  * @param  hcomp: COMP handle
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_COMP_Start_IT(COMP_HandleTypeDef *hcomp)
{ 
  HAL_StatusTypeDef status = HAL_OK; 
  uint32_t extiline = 0;
  
  status = HAL_COMP_Start(hcomp);
  if(status == HAL_OK)
  {
    /* Check the parameter */
    assert_param(IS_COMP_TRIGGERMODE(hcomp->Init.TriggerMode));
    
    /* Get the Exti Line output configuration */
    extiline = COMP_GET_EXTI_LINE(hcomp->Instance);
    
    /* Configure the trigger rising edge */
    if((hcomp->Init.TriggerMode & COMP_TRIGGERMODE_IT_RISING) != RESET)
    {
      SET_BIT(EXTI->RTSR, extiline);
    }
    else
    {
      CLEAR_BIT(EXTI->RTSR, extiline);
    }
  
    /* Configure the trigger falling edge */
    if((hcomp->Init.TriggerMode & COMP_TRIGGERMODE_IT_FALLING) != RESET)
    {
      SET_BIT(EXTI->FTSR, extiline);
    }
    else
    {
      CLEAR_BIT(EXTI->FTSR, extiline);
    }
    
    /* Clear COMP EXTI pending bit */
    WRITE_REG(EXTI->PR, extiline);
    
    /* Enable EXTI interrupt mode */
    SET_BIT(EXTI->IMR, extiline);
    
  }

  return status;
}

/**
  * @brief  Disable the interrupt and Stop the comparator 
  * @param  hcomp: COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Stop_IT(COMP_HandleTypeDef *hcomp)
{ 
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Disable the EXTI Line interrupt mode */
  CLEAR_BIT(EXTI->IMR, COMP_GET_EXTI_LINE(hcomp->Instance));
  
  status = HAL_COMP_Stop(hcomp);
  
  return status;
}

/**
  * @brief  Comparator IRQ Handler 
  * @param  hcomp: COMP handle
  * @retval HAL status
  */
void HAL_COMP_IRQHandler(COMP_HandleTypeDef *hcomp)
{
  uint32_t extiline = COMP_GET_EXTI_LINE(hcomp->Instance);
  
  /* Check COMP Exti flag */
  if(READ_BIT(EXTI->PR, extiline) != RESET)
  {
    /* Clear COMP EXTI pending bit */
    WRITE_REG(EXTI->PR, extiline);

    /* COMP trigger user callback */
    HAL_COMP_TriggerCallback(hcomp);    
  }
}

/**
  * @}
  */

/** @defgroup COMP_Exported_Functions_Group3 Peripheral Control functions
 *  @brief   Peripheral Control functions
 *
@verbatim   
 ===============================================================================
                      ##### Peripheral Control functions #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions allowing to control the COMP 
    management functions: Lock status, comparator output level check, IRQ
    callback (in case of usage of comparator with interruption on ExtI line).

@endverbatim
  * @{
  */

/**
  * @brief  Lock the selected comparator configuration.
  *         Caution: On STM32L1, HAL COMP lock is software lock only (not  
  *         hardware lock as on some other STM32 devices)
  * @param  hcomp: COMP handle
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
  
  return status; 
}

/**
  * @brief  Return the output level (high or low) of the selected comparator. 
  *         The output level depends on the selected polarity.
  *           - Comparator output is low when the non-inverting input is at a lower
  *             voltage than the inverting input
  *           - Comparator output is high when the non-inverting input is at a higher
  *             voltage than the inverting input
  * @param  hcomp: COMP handle
  * @retval Returns the selected comparator output level: COMP_OUTPUTLEVEL_LOW or COMP_OUTPUTLEVEL_HIGH.
  *       
  */
uint32_t HAL_COMP_GetOutputLevel(COMP_HandleTypeDef *hcomp)
{
  uint32_t level = 0;
  
  /* Check the parameter */
  assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

  /* Read output level of the selected comparator */
  if(READ_BIT(COMP->CSR, __COMP_CSR_CMPXOUT(hcomp)) == RESET)
  {
    level = COMP_OUTPUTLEVEL_LOW;
  }
  else
  {
    level = COMP_OUTPUTLEVEL_HIGH;
  }
  
  return(level);
}

/**
  * @brief  Comparator callback.
  * @param  hcomp: COMP handle
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

/** @defgroup COMP_Exported_Functions_Group4 Peripheral State functions 
 *  @brief   Peripheral State functions
 *
@verbatim   
 ===============================================================================
                      ##### Peripheral State functions #####
 ===============================================================================  
    [..]
    This subsection permit to get in run-time the status of the peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Return the COMP state
  * @param  hcomp : COMP handle
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
