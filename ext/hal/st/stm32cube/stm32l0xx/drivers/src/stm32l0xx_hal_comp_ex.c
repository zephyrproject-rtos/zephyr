/**
  ******************************************************************************
  * @file    stm32l0xx_hal_comp_ex.c
  * @author  MCD Application Team
  * @brief   Extended COMP HAL module driver.
  * @brief   This file provides firmware functions to manage voltage reference
  *          VrefInt that must be specifically controled for comparator
  *          instance COMP2.
  @verbatim 
  ==============================================================================
               ##### COMP peripheral Extended features  #####
  ==============================================================================

  [..] Comparing to other previous devices, the COMP interface for STM32L0XX
       devices contains the following additional features

       (+) Possibility to enable or disable the VREFINT which is used as input
           to the comparator.


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

#ifdef HAL_COMP_MODULE_ENABLED

/** @addtogroup COMPEx
  * @brief Extended COMP HAL module driver
  * @{
  */

/* Private define ------------------------------------------------------------*/
/** @addtogroup COMP_Private_Constants
  * @{
  */

/* Delay for COMP voltage scaler stabilization time (voltage from VrefInt,    */
/* delay based on VrefInt startup time).                                      */
/* Literal set to maximum value (refer to device datasheet,                   */
/* parameter "TVREFINT").                                                     */
/* Unit: us                                                                   */
#define COMP_DELAY_VOLTAGE_SCALER_STAB_US ((uint32_t)3000U)  /*!< Delay for COMP voltage scaler stabilization time */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup COMPEx_Exported_Functions
  * @{
  */

/** @addtogroup COMPEx_Exported_Functions_Group1
  * @brief  Extended functions to manage VREFINT for the comparator
  *
  * @{
  */

/**
  * @brief  Enable Vrefint and path to comparator, used by comparator
  *         instance COMP2 input based on VrefInt or subdivision of VrefInt.
  * @note   The equivalent of this function is managed automatically when
  *         using function "HAL_COMP_Init()".
  * @note   VrefInt requires a startup time
  *         (refer to device datasheet, parameter "TVREFINT").
  *         This function waits for the startup time
  *         (alternative solution: poll for bit SYSCFG_CFGR3_VREFINT_RDYF set).
  * @retval None
  */
void HAL_COMPEx_EnableVREFINT(void)
{
  __IO uint32_t wait_loop_index = 0U;
  
  /* Enable the Buffer for the COMP by setting ENBUFLP_VREFINT_COMP bit in the CFGR3 register */
  SYSCFG->CFGR3 |= (SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP);
  
  /* Wait loop initialization and execution */
  /* Note: Variable divided by 2 to compensate partially              */
  /*       CPU processing cycles.                                     */
  wait_loop_index = (COMP_DELAY_VOLTAGE_SCALER_STAB_US * (SystemCoreClock / (1000000U * 2U)));
  while(wait_loop_index != 0U)
  {
    wait_loop_index--;
  }
}

/**
  * @brief  Disable Vrefint and path to comparator, used by comparator
  *         instance COMP2 input based on VrefInt or subdivision of VrefInt.
  * @retval None
  */
void HAL_COMPEx_DisableVREFINT(void)
{
  /* Disable the Vrefint by resetting ENBUFLP_VREFINT_COMP bit in the CFGR3 register */
  SYSCFG->CFGR3 &= (uint32_t)~((uint32_t)(SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP));
}

/**
  * @}
  */

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
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
