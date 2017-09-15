/**
  ******************************************************************************
  * @file    stm32l4xx_ll_rng.c
  * @author  MCD Application Team
  * @brief   RNG LL module driver.
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
#include "stm32l4xx_ll_rng.h"
#include "stm32l4xx_ll_bus.h"

#ifdef  USE_FULL_ASSERT
#include "stm32_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32L4xx_LL_Driver
  * @{
  */

#if defined (RNG)

/** @addtogroup RNG_LL
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @addtogroup RNG_LL_Private_Macros
  * @{
  */
#if defined(RNG_CR_CED)
#define IS_LL_RNG_CED(__MODE__) (((__MODE__) == LL_RNG_CED_ENABLE) || \
                                 ((__MODE__) == LL_RNG_CED_DISABLE))
#endif /* defined(RNG_CR_CED) */
      
#if defined(RNG_CR_BYP)
#define IS_LL_RNG_BYPASS(__MODE__) (((__MODE__) == LL_RNG_BYP_DISABLE) || \
                                    ((__MODE__) == LL_RNG_BYP_ENABLE))
#endif /* defined(RNG_CR_BYP) */
/**
  * @}
  */
                                           
/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RNG_LL_Exported_Functions
  * @{
  */

/** @addtogroup RNG_LL_EF_Init
  * @{
  */

/**
  * @brief  De-initialize RNG registers (Registers restored to their default values).
  * @param  RNGx RNG Instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RNG registers are de-initialized
  *          - ERROR: not applicable
  */
ErrorStatus LL_RNG_DeInit(RNG_TypeDef *RNGx)
{
  /* Check the parameters */
  assert_param(IS_RNG_ALL_INSTANCE(RNGx));

  /* Enable RNG reset state */
  LL_AHB2_GRP1_ForceReset(LL_AHB2_GRP1_PERIPH_RNG);

  /* Release RNG from reset state */
  LL_AHB2_GRP1_ReleaseReset(LL_AHB2_GRP1_PERIPH_RNG);

  return (SUCCESS);
}

#if defined(RNG_CR_CED) || defined(RNG_CR_BYP)
/**
  * @brief  Initialize RNG registers according to the specified parameters in RNG_InitStruct.
  * @param  RNGx RNG Instance
  * @param  RNG_InitStruct: pointer to a LL_RNG_InitTypeDef structure
  *         that contains the configuration information for the specified RNG peripheral.
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RNG registers are initialized according to RNG_InitStruct content
  *          - ERROR: not applicable
  */
ErrorStatus LL_RNG_Init(RNG_TypeDef *RNGx, LL_RNG_InitTypeDef *RNG_InitStruct)
{
  /* Check the parameters */
  assert_param(IS_RNG_ALL_INSTANCE(RNGx));
#if defined(RNG_CR_CED)
  assert_param(IS_LL_RNG_CED(RNG_InitStruct->ClockErrorDetection));
#endif /* defined(RNG_CR_CED) */
#if defined(RNG_CR_BYP)
  assert_param(IS_LL_RNG_BYPASS(RNG_InitStruct->BypassMode));
#endif /* defined(RNG_CR_BYP) */

#if defined(RNG_CR_CED)
  /* Clock Error Detection configuration */
  MODIFY_REG(RNGx->CR, RNG_CR_CED, RNG_InitStruct->ClockErrorDetection);
#endif /* defined(RNG_CR_CED) */

#if defined(RNG_CR_BYP) 
  /* Bypass mode configuration */
  MODIFY_REG(RNGx->CR, RNG_CR_BYP, RNG_InitStruct->BypassMode);
#endif /* defined(RNG_CR_BYP) */

  return (SUCCESS);
}
#endif /* defined(RNG_CR_CED) || defined(RNG_CR_BYP) */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* defined (RNG) */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

