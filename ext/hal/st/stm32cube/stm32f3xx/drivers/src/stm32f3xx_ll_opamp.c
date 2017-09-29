/**
  ******************************************************************************
  * @file    stm32f3xx_ll_opamp.c
  * @author  MCD Application Team
  * @brief   OPAMP LL module driver
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
#include "stm32f3xx_ll_opamp.h"

#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32F3xx_LL_Driver
  * @{
  */

#if defined (OPAMP1) || defined (OPAMP2) || defined (OPAMP3) || defined (OPAMP4)

/** @addtogroup OPAMP_LL OPAMP
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/** @addtogroup OPAMP_LL_Private_Macros
  * @{
  */

/* Check of parameters for configuration of OPAMP hierarchical scope:         */
/* OPAMP instance.                                                            */

#define IS_LL_OPAMP_FUNCTIONAL_MODE(__FUNCTIONAL_MODE__)                       \
  (   ((__FUNCTIONAL_MODE__) == LL_OPAMP_MODE_STANDALONE)                      \
   || ((__FUNCTIONAL_MODE__) == LL_OPAMP_MODE_FOLLOWER)                        \
   || ((__FUNCTIONAL_MODE__) == LL_OPAMP_MODE_PGA)                             \
  )

/* Note: Comparator non-inverting inputs parameters are the same on all       */
/*       OPAMP instances.                                                     */
/*       However, comparator instance kept as macro parameter for             */
/*       compatibility with other STM32 families.                             */
#define IS_LL_OPAMP_INPUT_NONINVERTING(__OPAMPX__, __INPUT_NONINVERTING__)     \
  (   ((__INPUT_NONINVERTING__) == LL_OPAMP_INPUT_NONINVERT_IO0)               \
   || ((__INPUT_NONINVERTING__) == LL_OPAMP_INPUT_NONINVERT_IO1)               \
   || ((__INPUT_NONINVERTING__) == LL_OPAMP_INPUT_NONINVERT_IO2)               \
   || ((__INPUT_NONINVERTING__) == LL_OPAMP_INPUT_NONINVERT_IO3)               \
  )

/* Note: Comparator non-inverting inputs parameters are the same on all       */
/*       OPAMP instances.                                                     */
/*       However, comparator instance kept as macro parameter for             */
/*       compatibility with other STM32 families.                             */
#define IS_LL_OPAMP_INPUT_INVERTING(__OPAMPX__, __INPUT_INVERTING__)           \
  (   ((__INPUT_INVERTING__) == LL_OPAMP_INPUT_INVERT_IO0)                     \
   || ((__INPUT_INVERTING__) == LL_OPAMP_INPUT_INVERT_IO1)                     \
   || ((__INPUT_INVERTING__) == LL_OPAMP_INPUT_INVERT_CONNECT_NO)              \
  )

/**
  * @}
  */


/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup OPAMP_LL_Exported_Functions
  * @{
  */

/** @addtogroup OPAMP_LL_EF_Init
  * @{
  */

/**
  * @brief  De-initialize registers of the selected OPAMP instance
  *         to their default reset values.
  * @note   If comparator is locked, de-initialization by software is
  *         not possible.
  *         The only way to unlock the comparator is a device hardware reset.
  * @param  OPAMPx OPAMP instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: OPAMP registers are de-initialized
  *          - ERROR: OPAMP registers are not de-initialized
  */
ErrorStatus LL_OPAMP_DeInit(OPAMP_TypeDef* OPAMPx)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_OPAMP_ALL_INSTANCE(OPAMPx));

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       OPAMP instance must not be locked.                                 */
  if(LL_OPAMP_IsLocked(OPAMPx) == 0U)
  {
  LL_OPAMP_WriteReg(OPAMPx, CSR, 0x00000000U);
  }
  else
  {
    /* OPAMP instance is locked: de-initialization by software is              */
    /* not possible.                                                           */
    /* The only way to unlock the OPAMP is a device hardware reset.            */
    status = ERROR;
  }

  return status;
}

/**
  * @brief  Initialize some features of OPAMP instance.
  * @note   This function reset bit of calibration mode to ensure
  *         to be in functional mode, in order to have OPAMP parameters
  *         (inputs selection, ...) set with the corresponding OPAMP mode
  *         to be effective.
  * @param  OPAMPx OPAMP instance
  * @param  OPAMP_InitStruct Pointer to a @ref LL_OPAMP_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: OPAMP registers are initialized
  *          - ERROR: OPAMP registers are not initialized
  */
ErrorStatus LL_OPAMP_Init(OPAMP_TypeDef *OPAMPx, LL_OPAMP_InitTypeDef *OPAMP_InitStruct)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_OPAMP_ALL_INSTANCE(OPAMPx));
  assert_param(IS_LL_OPAMP_FUNCTIONAL_MODE(OPAMP_InitStruct->FunctionalMode));
  assert_param(IS_LL_OPAMP_INPUT_NONINVERTING(OPAMPx, OPAMP_InitStruct->InputNonInverting));

  /* Note: OPAMP inverting input can be used with OPAMP in mode standalone    */
  /*       or PGA with external capacitors for filtering circuit.             */
  /*       Otherwise (OPAMP in mode follower), OPAMP inverting input is       */
  /*       not used (not connected to GPIO pin).                              */
  if(OPAMP_InitStruct->FunctionalMode != LL_OPAMP_MODE_FOLLOWER)
  {
    assert_param(IS_LL_OPAMP_INPUT_INVERTING(OPAMPx, OPAMP_InitStruct->InputInverting));
  }

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       OPAMP instance must not be locked.                                 */
  if(LL_OPAMP_IsLocked(OPAMPx) == 0U)
  {
    /* Configuration of OPAMP instance :                                      */
    /*  - Functional mode                                                     */
    /*  - Input non-inverting                                                 */
    /*  - Input inverting                                                     */
    /* Note: Bit OPAMP_CSR_CALON reset to ensure to be in functional mode.    */
    if(OPAMP_InitStruct->FunctionalMode != LL_OPAMP_MODE_FOLLOWER)
    {
      MODIFY_REG(OPAMPx->CSR,
                   OPAMP_CSR_CALON
                 | OPAMP_CSR_VMSEL
                 | OPAMP_CSR_VPSEL
                ,
                   OPAMP_InitStruct->FunctionalMode
                 | OPAMP_InitStruct->InputNonInverting
                 | OPAMP_InitStruct->InputInverting
                );
    }
    else
    {
      MODIFY_REG(OPAMPx->CSR,
                   OPAMP_CSR_CALON
                 | OPAMP_CSR_VMSEL
                 | OPAMP_CSR_VPSEL
                ,
                   LL_OPAMP_MODE_FOLLOWER
                 | OPAMP_InitStruct->InputNonInverting
                );
    }
  }
  else
  {
    /* Initialization error: OPAMP instance is locked.                        */
    status = ERROR;
  }

  return status;
}

/**
  * @brief Set each @ref LL_OPAMP_InitTypeDef field to default value.
  * @param OPAMP_InitStruct pointer to a @ref LL_OPAMP_InitTypeDef structure
  *                         whose fields will be set to default values.
  * @retval None
  */
void LL_OPAMP_StructInit(LL_OPAMP_InitTypeDef *OPAMP_InitStruct)
{
  /* Set OPAMP_InitStruct fields to default values */
  OPAMP_InitStruct->FunctionalMode    = LL_OPAMP_MODE_FOLLOWER;
  OPAMP_InitStruct->InputNonInverting = LL_OPAMP_INPUT_NONINVERT_IO0;
  /* Note: Parameter discarded if OPAMP in functional mode follower,          */
  /*       set anyway to its default value.                                   */
  OPAMP_InitStruct->InputInverting    = LL_OPAMP_INPUT_INVERT_CONNECT_NO;
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

#endif /* OPAMP1 || OPAMP2 || OPAMP3 || OPAMP4 */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
