/**
  ******************************************************************************
  * @file    stm32l0xx_hal_comp.c
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
  * @brief   COMP HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities of the COMP peripheral:
  *           + Initialization and de-initialization functions
  *           + Start/Stop operation functions in polling mode
  *           + Start/Stop operation functions in interrupt mode (through EXTI interrupt)
  *           + Peripheral control functions
  *           + Peripheral state functions
  *         
  @verbatim
================================================================================
          ##### COMP Peripheral features #####
================================================================================
           
  [..]       
      The STM32L0xx device family integrates two analog comparators instances
      COMP1 and COMP2:
      (#) The COMP input minus (inverting input) and input plus (non inverting input)
          can be set to internal references or to GPIO pins
          (refer to GPIO list in reference manual).
  
      (#) The COMP output level is available using HAL_COMP_GetOutputLevel()
          and can be redirected to other peripherals: GPIO pins (in mode
          alternate functions for comparator), timers.
          (refer to GPIO list in reference manual).
  
      (#) Pairs of comparators instances can be combined in window mode
          (2 consecutive instances odd and even COMP<x> and COMP<x+1>).
  
      (#) The comparators have interrupt capability through the EXTI controller
          with wake-up from sleep and stop modes:
          (++) COMP1 is internally connected to EXTI Line 21
          (++) COMP2 is internally connected to EXTI Line 22

          From the corresponding IRQ handler, the right interrupt source can be retrieved
          using macro __HAL_COMP_COMP1_EXTI_GET_FLAG() and __HAL_COMP_COMP2_EXTI_GET_FLAG().

            ##### How to use this driver #####
================================================================================
  [..]
      This driver provides functions to configure and program the comparator instances
      of STM32L0xx devices.

      To use the comparator, perform the following steps:
      
      (#)  Initialize the COMP low level resources by implementing the HAL_COMP_MspInit():
      (++) Configure the GPIO connected to comparator inputs plus and minus in analog mode
           using HAL_GPIO_Init().
      (++) If needed, configure the GPIO connected to comparator output in alternate function mode
           using HAL_GPIO_Init().
      (++) If required enable the COMP interrupt by configuring and enabling EXTI line in Interrupt mode and 
           selecting the desired sensitivity level using HAL_GPIO_Init() function. After that enable the comparator
           interrupt vector using HAL_NVIC_EnableIRQ() function.
      
      (#) Configure the comparator using HAL_COMP_Init() function:
      (++) Select the input minus (inverting input)
      (++) Select the input plus (non-inverting input)
      (++) Select the output polarity  
      (++) Select the power mode
      (++) Select the window mode
      
      -@@- HAL_COMP_Init() calls internally __HAL_RCC_SYSCFG_CLK_ENABLE()
          to enable internal control clock of the comparators.
          However, this is a legacy strategy. In future STM32 families,
          COMP clock enable must be implemented by user in "HAL_COMP_MspInit()".
          Therefore, for compatibility anticipation, it is recommended to 
          implement __HAL_RCC_SYSCFG_CLK_ENABLE() in "HAL_COMP_MspInit()".
      
      (#) Reconfiguration on-the-fly of comparator can be done by calling again
          function HAL_COMP_Init() with new input structure parameters values.
      
      (#) Enable the comparator using HAL_COMP_Start() function.
      
      (#) Use HAL_COMP_TriggerCallback() or HAL_COMP_GetOutputLevel() functions
          to manage comparator outputs (events and output level).
      
      (#) Disable the comparator using HAL_COMP_Stop() function.
      
      (#) De-initialize the comparator using HAL_COMP_DeInit() function.
      
      (#) For safety purpose, comparator configuration can be locked using HAL_COMP_Lock() function.
          The only way to unlock the comparator is a device hardware reset.
  
  @endverbatim
  ******************************************************************************

  Table 1. COMP inputs and output for STM32L0xx devices
  +---------------------------------------------------------+
  |                |                |   COMP1   |   COMP2   |
  |----------------|----------------|-----------|-----------|
  |                | IO1            |    PA1    |    PA3    |
  | Input plus     | IO2            |    ---    |    PA4    |
  |                | IO3            |    ---    |    PB5    |
  |                | IO4            |    ---    |    PB6    |
  |                | IO5            |    ---    |    PB7    |
  |----------------|----------------|-----------------------|
  |                | 1/4 VrefInt    |    ---    | Available |
  |                | 1/2 VrefInt    |    ---    | Available |
  |                | 3/4 VrefInt    |    ---    | Available |
  | Input minus    | VrefInt        | Available | Available |
  |                | DAC1 channel 1 | Available | Available |
  |                | DAC1 channel 2 | Available | Available |
  |                | IO1            |    PA0    |    PA2    |
  |                | IO2            |    PA5    |    PA5    |
  |                | IO3            |    ---    |    PB3    |
  +---------------------------------------------------------+
  | Output         |                |  PA0  (1) |  PA2  (1) |
  |                |                |  PA6  (1) |  PA7  (1) |
  |                |                |  PA11 (1) |  PA12 (1) |
  |                |                |  LPTIM    |  LPTIM    |
  |                |                |  TIM  (2) |  TIM  (2) |
  +-----------------------------------------------------------+
  (1) GPIO must be set to alternate function for comparator
  (2) Comparators output to timers is set in timers instances.

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

/** @defgroup COMP COMP
  * @brief COMP HAL module driver
  * @{
  */

#ifdef HAL_COMP_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup COMP_Private_Constants
  * @{
  */

/* Delay for COMP startup time.                                               */
/* Note: Delay required to reach propagation delay specification.             */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "tSTART").                                                       */
/* Unit: us                                                                   */
#define COMP_DELAY_STARTUP_US             ((uint32_t) 25U)  /*!< Delay for COMP startup time */

/* Delay for COMP voltage scaler stabilization time (voltage from VrefInt,    */
/* delay based on VrefInt startup time).                                      */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "TVREFINT").                                                     */
/* Unit: us                                                                   */
#define COMP_DELAY_VOLTAGE_SCALER_STAB_US ((uint32_t)3000U)  /*!< Delay for COMP voltage scaler stabilization time */

#define COMP_OUTPUT_LEVEL_BITOFFSET_POS  ((uint32_t)  30U)

#define C_REV_ID_A              0x1000U /* Cut1.0 */
#define C_REV_ID_Z              0x1008U /* Cut1.1 */
#define C_REV_ID_Y              0x1003U /* Cut1.2 */

#define C_DEV_ID_L073           0x447U
#define C_DEV_ID_L053           0x417U

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
    [..]  This section provides functions to initialize and de-initialize comparators 

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the COMP according to the specified
  *         parameters in the COMP_InitTypeDef and initialize the associated handle.
  * @note   If the selected comparator is locked, initialization can't be performed.
  *         To unlock the configuration, perform a system reset.
  * @note   When the LPTIM connection is enabled, the following pins LPTIM_IN1(PB5, PC0)
            and LPTIM_IN2(PB7, PC2) should not be configured in alternate function.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Init(COMP_HandleTypeDef *hcomp)
{
  uint32_t tmp_csr = 0U;
  uint32_t exti_line = 0U;
  uint32_t comp_voltage_scaler_not_initialized = 0U;
  __IO uint32_t wait_loop_index = 0U;
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || (__HAL_COMP_IS_LOCKED(hcomp)))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameters */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));
    assert_param(IS_COMP_INPUT_PLUS(hcomp->Instance, hcomp->Init.NonInvertingInput));
    assert_param(IS_COMP_INPUT_MINUS(hcomp->Instance, hcomp->Init.InvertingInput));
    assert_param(IS_COMP_OUTPUTPOL(hcomp->Init.OutputPol));
    assert_param(IS_COMP_POWERMODE(hcomp->Init.Mode));
    assert_param(IS_COMP_TRIGGERMODE(hcomp->Init.TriggerMode));
    assert_param(IS_COMP_WINDOWMODE(hcomp->Init.WindowMode));
    
    if(hcomp->State == HAL_COMP_STATE_RESET)
    {
      /* Allocate lock resource and initialize it */
      hcomp->Lock = HAL_UNLOCKED;
      
      /* Init SYSCFG and the low level hardware to access comparators */
      /* Note: HAL_COMP_Init() calls __HAL_RCC_SYSCFG_CLK_ENABLE()            */
      /*       to enable internal control clock of the comparators.           */
      /*       However, this is a legacy strategy. In future STM32 families,  */
      /*       COMP clock enable must be implemented by user                  */
      /*       in "HAL_COMP_MspInit()".                                       */
      /*       Therefore, for compatibility anticipation, it is recommended   */
      /*       to implement __HAL_RCC_SYSCFG_CLK_ENABLE()                     */
      /*       in "HAL_COMP_MspInit()".                                       */
      __HAL_RCC_SYSCFG_CLK_ENABLE();
      
      /* Init the low level hardware */
      HAL_COMP_MspInit(hcomp);
    }
    
    /* Set COMP parameters */
    tmp_csr = (hcomp->Init.InvertingInput   |
               hcomp->Init.OutputPol         );
    
    /* Configuration specific to comparator instance: COMP2 */
    if ((hcomp->Instance) == COMP2)
    {
      /* Comparator input plus configuration is available on COMP2 only */
      /* Comparator power mode configuration is available on COMP2 only */
      tmp_csr |= (hcomp->Init.NonInvertingInput |
                  hcomp->Init.Mode               );
      
      /* COMP2 specificity: when using VrefInt or subdivision of VrefInt,     */
      /* specific path must be enabled.                                       */
      if((hcomp->Init.InvertingInput == COMP_INPUT_MINUS_VREFINT)    ||
         (hcomp->Init.InvertingInput == COMP_INPUT_MINUS_1_4VREFINT) ||
         (hcomp->Init.InvertingInput == COMP_INPUT_MINUS_1_2VREFINT) ||
         (hcomp->Init.InvertingInput == COMP_INPUT_MINUS_3_4VREFINT)   )
      {
        /* Memorize voltage scaler state before initialization */
        comp_voltage_scaler_not_initialized = (READ_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP) == 0U);
        
        SET_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP  );
        
        /* Delay for COMP scaler bridge voltage stabilization */
        /* Apply the delay if voltage scaler bridge is enabled for the first time */
        if (comp_voltage_scaler_not_initialized != 0U)
        {
          /* Wait loop initialization and execution */
          /* Note: Variable divided by 2 to compensate partially              */
          /*       CPU processing cycles.                                     */
          wait_loop_index = (COMP_DELAY_VOLTAGE_SCALER_STAB_US * (SystemCoreClock / (1000000U * 2U)));
          while(wait_loop_index != 0U)
          {
            wait_loop_index--;
          }
        }
      }
    }
    
    /* Set comparator output connection to LPTIM */
    if (hcomp->Init.LPTIMConnection != COMP_LPTIMCONNECTION_DISABLED)
    {
      /* LPTIM connexion requested on COMP1 */
      if ((hcomp->Instance) == COMP1)
      {
        /* Note : COMP1 can be connected to the input 1 of LPTIM if requested */
        assert_param(IS_COMP1_LPTIMCONNECTION(hcomp->Init.LPTIMConnection));
        if (hcomp->Init.LPTIMConnection == COMP_LPTIMCONNECTION_IN1_ENABLED)
        {
          tmp_csr |= (COMP_CSR_COMP1LPTIM1IN1);
        }
      }
      else
      {
        /* Check the MCU_ID in order to allow or not the COMP2 connection to LPTIM-input2 */
        if (((HAL_GetDEVID() == C_DEV_ID_L073) && (HAL_GetREVID() == C_REV_ID_A))
                          ||
            ((HAL_GetDEVID() == C_DEV_ID_L053) && (HAL_GetREVID() == C_REV_ID_A))
                          ||
            ((HAL_GetDEVID() == C_DEV_ID_L053) && (HAL_GetREVID() == C_REV_ID_Z)))
        {
          /* Note : COMP2 can be connected only to input 1 of LPTIM if requested */
          assert_param(IS_COMP2_LPTIMCONNECTION_RESTRICTED(hcomp->Init.LPTIMConnection));
          
          tmp_csr |= (COMP_CSR_COMP2LPTIM1IN1);
        }
        /* LPTIM connexion requested on COMP2 */
        else
        {
           /* Note : COMP2 can be connected to input 1 or input2  of LPTIM if requested */
          assert_param(IS_COMP2_LPTIMCONNECTION(hcomp->Init.LPTIMConnection));
          switch (hcomp->Init.LPTIMConnection)
          {
          case  COMP_LPTIMCONNECTION_IN1_ENABLED :
              tmp_csr |= (COMP_CSR_COMP2LPTIM1IN1);
              break;
          case  COMP_LPTIMCONNECTION_IN2_ENABLED :
              tmp_csr |= (COMP_CSR_COMP2LPTIM1IN2);
              break;
          default :
              break;
          }
        }
      }
    }
      
    /* Update comparator register */
    if ((hcomp->Instance) == COMP1)
    {
      MODIFY_REG(hcomp->Instance->CSR,
                 COMP_CSR_COMP1INNSEL     | COMP_CSR_COMP1WM       |
                 COMP_CSR_COMP1LPTIM1IN1  | COMP_CSR_COMP1POLARITY  ,
                 tmp_csr
                );
    }
    else /* Instance == COMP2 */
    {
      MODIFY_REG(hcomp->Instance->CSR,
                 COMP_CSR_COMP2SPEED     | COMP_CSR_COMP2INNSEL    |
                 COMP_CSR_COMP2INPSEL    | COMP_CSR_COMP2POLARITY  |
                 COMP_CSR_COMP2LPTIM1IN2 | COMP_CSR_COMP2LPTIM1IN1  ,
                 tmp_csr
                );
    }
    
    /* Set window mode */
    /* Note: Window mode bit is located into 1 out of the 2 pairs of COMP     */
    /*       instances. Therefore, this function can update another COMP      */
    /*       instance that the one currently selected.                        */
    if(hcomp->Init.WindowMode == COMP_WINDOWMODE_COMP1_INPUT_PLUS_COMMON)
    {
      SET_BIT(COMP12_COMMON->CSR, COMP_CSR_WINMODE);
    }
    else
    {
      CLEAR_BIT(COMP12_COMMON->CSR, COMP_CSR_WINMODE);
    }
    
    /* Get the EXTI line corresponding to the selected COMP instance */
    exti_line = COMP_GET_EXTI_LINE(hcomp->Instance);
    
    /* Manage EXTI settings */
    if((hcomp->Init.TriggerMode & (COMP_EXTI_IT | COMP_EXTI_EVENT)) != RESET)
    {
      /* Configure EXTI rising edge */
      if((hcomp->Init.TriggerMode & COMP_EXTI_RISING) != RESET)
      {
        SET_BIT(EXTI->RTSR, exti_line);
      }
      else
      {
        CLEAR_BIT(EXTI->RTSR, exti_line);
      }
      
      /* Configure EXTI falling edge */
      if((hcomp->Init.TriggerMode & COMP_EXTI_FALLING) != RESET)
      {
        SET_BIT(EXTI->FTSR, exti_line);
      }
      else
      {
        CLEAR_BIT(EXTI->FTSR, exti_line);
      }
      
      /* Clear COMP EXTI pending bit (if any) */
      WRITE_REG(EXTI->PR, exti_line);
      
      /* Configure EXTI event mode */
      if((hcomp->Init.TriggerMode & COMP_EXTI_EVENT) != RESET)
      {
        SET_BIT(EXTI->EMR, exti_line);
      }
      else
      {
        CLEAR_BIT(EXTI->EMR, exti_line);
      }
      
      /* Configure EXTI interrupt mode */
      if((hcomp->Init.TriggerMode & COMP_EXTI_IT) != RESET)
      {
        SET_BIT(EXTI->IMR, exti_line);
      }
      else
      {
        CLEAR_BIT(EXTI->IMR, exti_line);
      }
    }
    else
    {
      /* Disable EXTI event generation */
      CLEAR_BIT(EXTI->EMR, exti_line);
    }
    
    /* Set HAL COMP handle state */
    /* Note: Transition from state reset to state ready,                      */
    /*       otherwise (coming from state ready or busy) no state update.     */
    if (hcomp->State == HAL_COMP_STATE_RESET)
    {
      hcomp->State = HAL_COMP_STATE_READY;
    }
  }
  
  return status;
}

/**
  * @brief  DeInitialize the COMP peripheral.
  * @note   Deinitialization cannot be performed if the COMP configuration is locked.
  *         To unlock the configuration, perform a system reset.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_DeInit(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || (__HAL_COMP_IS_LOCKED(hcomp)))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));
    
    /* Set COMP_CSR register to reset value */
    WRITE_REG(hcomp->Instance->CSR, 0x00000000U);
    
    /* DeInit the low level hardware: SYSCFG, GPIO, CLOCK and NVIC */
    HAL_COMP_MspDeInit(hcomp);
    
    /* Set HAL COMP handle state */
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

/** @defgroup COMP_Exported_Functions_Group2 Start-Stop operation functions 
 *  @brief   Start-Stop operation functions. 
 *
@verbatim   
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================  
    [..]  This section provides functions allowing to:
      (+) Start a comparator instance.
      (+) Stop a comparator instance.

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
  __IO uint32_t wait_loop_index = 0U;
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || (__HAL_COMP_IS_LOCKED(hcomp)))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));

    if(hcomp->State == HAL_COMP_STATE_READY)
    {
      /* Enable the selected comparator */
      SET_BIT(hcomp->Instance->CSR, COMP_CSR_COMPxEN);
      
      /* Set HAL COMP handle state */
      hcomp->State = HAL_COMP_STATE_BUSY;
      
      /* Delay for COMP startup time */
      /* Wait loop initialization and execution */
      /* Note: Variable divided by 2 to compensate partially                  */
      /*       CPU processing cycles.                                         */
      wait_loop_index = (COMP_DELAY_STARTUP_US * (SystemCoreClock / (1000000U * 2U)));
      while(wait_loop_index != 0U)
      {
        wait_loop_index--;
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
  * @brief  Stop the comparator.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Stop(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || (__HAL_COMP_IS_LOCKED(hcomp)))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));
    
    if((hcomp->State == HAL_COMP_STATE_BUSY)  ||
       (hcomp->State == HAL_COMP_STATE_READY)   )
    {
      /* Disable the selected comparator */
      CLEAR_BIT(hcomp->Instance->CSR, COMP_CSR_COMPxEN);

      /* Set HAL COMP handle state */
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
  * @brief  Comparator IRQ handler.
  * @param  hcomp  COMP handle
  * @retval None
  */
void HAL_COMP_IRQHandler(COMP_HandleTypeDef *hcomp)
{
  /* Get the EXTI line corresponding to the selected COMP instance */
  uint32_t exti_line = COMP_GET_EXTI_LINE(hcomp->Instance);
  
  /* Check COMP EXTI flag */
  if(READ_BIT(EXTI->PR, exti_line) != RESET)
  {
    /* Clear COMP EXTI pending bit */
    WRITE_REG(EXTI->PR, exti_line);
    
    /* COMP trigger user callback */
    HAL_COMP_TriggerCallback(hcomp);
  }
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
  * @note   Locking the comparator from reset state is possible
  *         if __HAL_RCC_SYSCFG_CLK_ENABLE() is being called before.
  * @param  hcomp  COMP handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_COMP_Lock(COMP_HandleTypeDef *hcomp)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check the COMP handle allocation and lock status */
  if((hcomp == NULL) || (__HAL_COMP_IS_LOCKED(hcomp)))
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check the parameter */
    assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));
    
    /* Set HAL COMP handle state */
    hcomp->State = ((HAL_COMP_StateTypeDef)(hcomp->State | COMP_STATE_BITFIELD_LOCK));
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
  *           - Comparator output is low when the input plus is at a lower
  *             voltage than the input minus
  *           - Comparator output is high when the input plus is at a higher
  *             voltage than the input minus
  *         If the polarity is inverted:
  *           - Comparator output is high when the input plus is at a lower
  *             voltage than the input minus
  *           - Comparator output is low when the input plus is at a higher
  *             voltage than the input minus
  * @param  hcomp  COMP handle
  * @retval Returns the selected comparator output level: 
  *         @arg @ref COMP_OUTPUT_LEVEL_LOW
  *         @arg @ref COMP_OUTPUT_LEVEL_HIGH
  *       
  */
uint32_t HAL_COMP_GetOutputLevel(COMP_HandleTypeDef *hcomp)
{
  /* Check the parameter */
  assert_param(IS_COMP_ALL_INSTANCE(hcomp->Instance));
  
  return (uint32_t)(READ_BIT(hcomp->Instance->CSR, COMP_CSR_COMPxOUTVALUE)
                    >> COMP_OUTPUT_LEVEL_BITOFFSET_POS);
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

/** @defgroup COMP_Exported_Functions_Group4 Peripheral State functions 
 *  @brief   Peripheral State functions. 
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

  /* Return HAL COMP handle state */
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
