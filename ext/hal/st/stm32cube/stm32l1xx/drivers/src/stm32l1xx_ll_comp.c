/**
  ******************************************************************************
  * @file    stm32l1xx_ll_comp.c
  * @author  MCD Application Team
  * @brief   COMP LL module driver
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
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_ll_comp.h"

#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32L1xx_LL_Driver
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

/* Note: On this STM32 serie, comparator input plus parameters are            */
/*       the different depending on COMP instances.                           */
#if defined(RI_ASCR1_CH_31)
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (((__COMP_INSTANCE__) == COMP1)                                              \
    ? (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO5)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO6)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO7)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO8)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO9)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO10)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO11)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO12)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO13)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO14)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO15)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO16)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO17)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO18)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO19)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO20)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO21)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO22)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO23)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO24)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO25)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO26)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO27)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO28)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO29)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO30)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO31)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO32)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO33)                        \
      )                                                                        \
      :                                                                        \
      (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO2)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO3)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO4)                         \
      )                                                                        \
  )
#else
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (((__COMP_INSTANCE__) == COMP1)                                              \
    ? (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO5)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO6)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO7)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO8)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO9)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO10)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO11)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO12)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO13)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO14)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO15)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO16)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO17)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO18)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO19)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO20)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO21)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO22)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO23)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO24)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO25)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO26)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO27)                        \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO28)                        \
      )                                                                        \
      :                                                                        \
      (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO2)                         \
      )                                                                        \
  )
#endif

/* Note: On this STM32 serie, comparator input minus parameters are           */
/*       the different depending on COMP instances.                           */
#define IS_LL_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__)             \
  (((__COMP_INSTANCE__) == COMP1)                                              \
    ? (                                                                        \
          ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                   \
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
      )                                                                        \
  )

#define IS_LL_COMP_OUTPUT_SELECTION(__OUTPUT_SELECTION__)                      \
  (   ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_NONE)                          \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC4)                      \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCREFCLR)                 \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC4)                      \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCREFCLR)                 \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC4)                      \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_OCREFCLR)                 \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM10_IC1)                     \
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
  
    /* Note: On this STM32 serie, only COMP instance COMP2 has                */
    /*       features settables: power mode, input minus selection            */
    /*       and output selection.                                            */
    /* Note: On this STM32 serie, setting COMP instance COMP2 input minus     */
    /*       is enabling the comparator.                                      */
    /*       Reset COMP2 input minus also disable the comparator.             */
    /* Note: In case of de-initialization of COMP instance COMP1:             */
    /*       Switch COMP_CSR_SW1 is not modified because can be used          */
    /*       to connect OPAMP3 to ADC.                                        */
    /*       Switches RI_ASCR1_VCOMP, RI_ASCR1_SCM are reset: let routing     */
    /*       interface under control of ADC.                                  */
    if(COMPx == COMP1)
    {
      CLEAR_BIT(COMP->CSR,
                (  COMP_CSR_CMP1EN
                 | COMP_CSR_10KPU
                 | COMP_CSR_400KPU
                 | COMP_CSR_10KPD
                 | COMP_CSR_400KPD
                ) 
               );
    }
    else
    {
      CLEAR_BIT(COMP->CSR,
                (  COMP_CSR_SPEED
                 | COMP_CSR_INSEL
                 | COMP_CSR_OUTSEL
                ) 
               );
    }
    
    /* Set comparator input plus */
    LL_COMP_SetInputPlus(COMPx, LL_COMP_INPUT_PLUS_NONE);
    
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
    assert_param(IS_LL_COMP_INPUT_MINUS(COMPx, COMP_InitStruct->InputMinus));
    assert_param(IS_LL_COMP_OUTPUT_SELECTION(COMP_InitStruct->OutputSelection));
  }
  assert_param(IS_LL_COMP_INPUT_PLUS(COMPx, COMP_InitStruct->InputPlus));
  
    /* Configuration of comparator instance :                                 */
    /*  - PowerMode                                                           */
    /*  - InputPlus                                                           */
    /*  - InputMinus                                                          */
    /*  - OutputSelection                                                     */
    /* Note: On this STM32 serie, only COMP instance COMP2 has                */
    /*       features settables: power mode, input minus selection            */
    /*       and output selection.                                            */
    /* Note: On this STM32 serie, setting COMP instance COMP2 input minus     */
    /*       is enabling the comparator.                                      */
    if(COMPx == COMP2)
    {
      MODIFY_REG(COMP->CSR,
                   COMP_CSR_SPEED
                 | COMP_CSR_INSEL
                 | COMP_CSR_OUTSEL
                ,
                   COMP_InitStruct->PowerMode
                 | COMP_InitStruct->InputMinus
                 | COMP_InitStruct->OutputSelection
                );
    }
    
    /* Set comparator input plus */
    LL_COMP_SetInputPlus(COMPx, COMP_InitStruct->InputPlus);
    
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
  COMP_InitStruct->PowerMode            = LL_COMP_POWERMODE_ULTRALOWPOWER;
  COMP_InitStruct->InputPlus            = LL_COMP_INPUT_PLUS_IO1;
  COMP_InitStruct->InputMinus           = LL_COMP_INPUT_MINUS_VREFINT;
  COMP_InitStruct->OutputSelection      = LL_COMP_OUTPUT_NONE;
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
