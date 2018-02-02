/**
  ******************************************************************************
  * @file    stm32l0xx_ll_comp.c
  * @author  MCD Application Team
  * @brief   COMP LL module driver
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
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_ll_comp.h"

#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32L0xx_LL_Driver
  * @{
  */

#if defined (COMP1) || defined (COMP2)

/** @addtogroup COMP_LL COMP
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/** @addtogroup COMP_LL_Private_Macros
  * @{
  */

/* Check of parameters for configuration of COMP hierarchical scope:          */
/* COMP instance.                                                             */

#define IS_LL_COMP_POWER_MODE(__POWER_MODE__)                                  \
  (   ((__POWER_MODE__) == LL_COMP_POWERMODE_MEDIUMSPEED)                      \
   || ((__POWER_MODE__) == LL_COMP_POWERMODE_ULTRALOWPOWER)                    \
  )

#if defined (STM32L011xx) || defined (STM32L021xx)
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (((__COMP_INSTANCE__) == COMP1)                                              \
    ? (                                                                        \
       (__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1                              \
      )                                                                        \
      :                                                                        \
      (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO2)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO3)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO4)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO5)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO6)                         \
      )                                                                        \
  )
#else
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (((__COMP_INSTANCE__) == COMP1)                                              \
    ? (                                                                        \
       (__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1                              \
      )                                                                        \
      :                                                                        \
      (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO2)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO3)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO4)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO5)                         \
      )                                                                        \
  )
#endif

/* Note: On this STM32 serie, comparator input minus parameters are           */
/*       the different depending on COMP instances.                           */
#define IS_LL_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__)             \
  (((__COMP_INSTANCE__) == COMP1)                                              \
    ? (                                                                        \
          ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                   \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH1)                  \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH2)                  \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO1)                       \
      )                                                                        \
      :                                                                        \
      (                                                                        \
          ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_4VREFINT)                \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_2VREFINT)                \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_3_4VREFINT)                \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                   \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH1)                  \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH2)                  \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO1)                       \
       || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO2)                       \
      )                                                                        \
  )

#define IS_LL_COMP_OUTPUT_POLARITY(__POLARITY__)                               \
  (   ((__POLARITY__) == LL_COMP_OUTPUTPOL_NONINVERTED)                        \
   || ((__POLARITY__) == LL_COMP_OUTPUTPOL_INVERTED)                           \
  )

/**
  * @}
  */


/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup COMP_LL_Exported_Functions
  * @{
  */

/** @addtogroup COMP_LL_EF_Init
  * @{
  */

/**
  * @brief  De-initialize registers of the selected COMP instance
  *         to their default reset values.
  * @note   If comparator is locked, de-initialization by software is
  *         not possible.
  *         The only way to unlock the comparator is a device hardware reset.
  * @param  COMPx COMP instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: COMP registers are de-initialized
  *          - ERROR: COMP registers are not de-initialized
  */
ErrorStatus LL_COMP_DeInit(COMP_TypeDef *COMPx)
{
  ErrorStatus status = SUCCESS;
  
  /* Check the parameters */
  assert_param(IS_COMP_ALL_INSTANCE(COMPx));
  
  /* Note: Hardware constraint (refer to description of this function):       */
  /*       COMP instance must not be locked.                                  */
  if(LL_COMP_IsLocked(COMPx) == 0U)
  {
    if(COMPx == COMP1)
    {
      CLEAR_BIT(COMPx->CSR,
                (  COMP_CSR_COMP1EN
                 | COMP_CSR_COMP1INNSEL
                 | COMP_CSR_COMP1WM
                 | COMP_CSR_COMP1LPTIM1IN1
                 | COMP_CSR_COMP1POLARITY
                 | COMP_CSR_COMP1LOCK
                ) 
               );
    }
    else
    {
      CLEAR_BIT(COMPx->CSR,
                (  COMP_CSR_COMP2EN
                 | COMP_CSR_COMP2SPEED
                 | COMP_CSR_COMP2INNSEL
                 | COMP_CSR_COMP2INPSEL
                 | COMP_CSR_COMP2LPTIM1IN2
                 | COMP_CSR_COMP2LPTIM1IN1
                 | COMP_CSR_COMP2POLARITY
                 | COMP_CSR_COMP2LOCK
                ) 
               );
    }

  }
  else
  {
    /* Comparator instance is locked: de-initialization by software is         */
    /* not possible.                                                           */
    /* The only way to unlock the comparator is a device hardware reset.       */
    status = ERROR;
  }
  
  return status;
}

/**
  * @brief  Initialize some features of COMP instance.
  * @note   This function configures features of the selected COMP instance.
  *         Some features are also available at scope COMP common instance
  *         (common to several COMP instances).
  *         Refer to functions having argument "COMPxy_COMMON" as parameter.
  * @param  COMPx COMP instance
  * @param  COMP_InitStruct Pointer to a @ref LL_COMP_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: COMP registers are initialized
  *          - ERROR: COMP registers are not initialized
  */
ErrorStatus LL_COMP_Init(COMP_TypeDef *COMPx, LL_COMP_InitTypeDef *COMP_InitStruct)
{
  ErrorStatus status = SUCCESS;
  
  /* Check the parameters */
  assert_param(IS_COMP_ALL_INSTANCE(COMPx));
  if(COMPx == COMP2)
  {
    assert_param(IS_LL_COMP_POWER_MODE(COMP_InitStruct->PowerMode));
    assert_param(IS_LL_COMP_INPUT_PLUS(COMPx, COMP_InitStruct->InputPlus));
  }
  assert_param(IS_LL_COMP_INPUT_MINUS(COMPx, COMP_InitStruct->InputMinus));
  assert_param(IS_LL_COMP_OUTPUT_POLARITY(COMP_InitStruct->OutputPolarity));
  
  /* Note: Hardware constraint (refer to description of this function)        */
  /*       COMP instance must not be locked.                                  */
  if(LL_COMP_IsLocked(COMPx) == 0U)
  {
    /* Configuration of comparator instance :                                 */
    /*  - PowerMode                                                           */
    /*  - InputPlus                                                           */
    /*  - InputMinus                                                          */
    /*  - OutputPolarity                                                      */
    /* Note: Connection switch is applicable only to COMP instance COMP1,     */
    /*       therefore is COMP2 is selected the equivalent bit is             */
    /*       kept unmodified.                                                 */
    if(COMPx == COMP1)
    {
      MODIFY_REG(COMPx->CSR,
                 ( COMP_CSR_COMP1INNSEL
                  | COMP_CSR_COMP1POLARITY
                 ) 
                ,
                 (  COMP_InitStruct->InputMinus
                  | COMP_InitStruct->OutputPolarity
                 ) 
                );
    }
    else
    {
      MODIFY_REG(COMPx->CSR,
                 (  COMP_CSR_COMP2SPEED       
                  | COMP_CSR_COMP2INPSEL      
                  | COMP_CSR_COMP2INNSEL      
                  | COMP_CSR_COMP2POLARITY
                 ) 
                ,
                 (  COMP_InitStruct->PowerMode
                  | COMP_InitStruct->InputPlus
                  | COMP_InitStruct->InputMinus
                  | COMP_InitStruct->OutputPolarity
                 ) 
                );
    }

  }
  else
  {
    /* Initialization error: COMP instance is locked.                         */
    status = ERROR;
  }
  
  return status;
}

/**
  * @brief Set each @ref LL_COMP_InitTypeDef field to default value.
  * @param COMP_InitStruct: pointer to a @ref LL_COMP_InitTypeDef structure
  *                         whose fields will be set to default values.
  * @retval None
  */
void LL_COMP_StructInit(LL_COMP_InitTypeDef *COMP_InitStruct)
{
  /* Set COMP_InitStruct fields to default values */
  COMP_InitStruct->PowerMode            = LL_COMP_POWERMODE_MEDIUMSPEED;
  COMP_InitStruct->InputPlus            = LL_COMP_INPUT_PLUS_IO1;
  COMP_InitStruct->InputMinus           = LL_COMP_INPUT_MINUS_VREFINT;
  COMP_InitStruct->OutputPolarity       = LL_COMP_OUTPUTPOL_NONINVERTED;
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

#endif /* COMP1 || COMP2 */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
