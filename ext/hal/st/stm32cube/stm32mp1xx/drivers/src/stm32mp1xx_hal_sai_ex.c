/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_sai_ex.c
  * @author  MCD Application Team
  * @brief   SAI Extended HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionality of the SAI Peripheral Controller:
  *           + Modify PDM microphone delays.
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#include "stm32mp1xx_hal.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */
#ifdef HAL_SAI_MODULE_ENABLED

/** @defgroup SAIEx SAIEx
  * @brief SAI Extended HAL module driver
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
#define SAI_PDM_DELAY_MASK          0x77U
#define SAI_PDM_DELAY_OFFSET        8U
#define SAI_PDM_RIGHT_DELAY_OFFSET  4U

/* Private macros ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @defgroup SAIEx_Exported_Functions SAIEx Extended Exported Functions
  * @{
  */

/** @defgroup SAIEx_Exported_Functions_Group1 Peripheral Control functions
  * @brief    SAIEx control functions
  *
@verbatim
 ===============================================================================
                 ##### Extended features functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Modify PDM microphone delays

@endverbatim
  * @{
  */

/**
  * @brief  Configure PDM microphone delays.
  * @param  hsai SAI handle.
  * @param  pdmMicDelay Microphone delays configuration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SAIEx_ConfigPdmMicDelay(SAI_HandleTypeDef *hsai, SAIEx_PdmMicDelayParamTypeDef *pdmMicDelay)
{
  HAL_StatusTypeDef status = HAL_OK;
  SAI_TypeDef *SaiBaseAddress;

  /* Get the SAI base address according to the SAI handle */
  SaiBaseAddress = (hsai->Instance == SAI1_Block_A) ? SAI1 : \
                   ((hsai->Instance == SAI4_Block_A) ? SAI4 : \
                     NULL);

  /* Check that SAI sub-block is SAI sub-block A */
  if (SaiBaseAddress == NULL)
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check microphone delay parameters */
    assert_param(IS_SAI_PDM_MIC_PAIRS_NUMBER(pdmMicDelay->MicPair));
    assert_param(IS_SAI_PDM_MIC_DELAY(pdmMicDelay->LeftDelay));
    assert_param(IS_SAI_PDM_MIC_DELAY(pdmMicDelay->RightDelay));

    /* Check SAI state */
    if (hsai->State != HAL_SAI_STATE_RESET)
    {
      /* Reset current delays for specified microphone */
      SaiBaseAddress->PDMDLY &= ~(SAI_PDM_DELAY_MASK << (SAI_PDM_DELAY_OFFSET * (pdmMicDelay->MicPair - 1)));

      /* Apply new microphone delays */
      SaiBaseAddress->PDMDLY |= (((pdmMicDelay->RightDelay << SAI_PDM_RIGHT_DELAY_OFFSET) | pdmMicDelay->LeftDelay) << \
                                 (SAI_PDM_DELAY_OFFSET * (pdmMicDelay->MicPair - 1)));
    }
    else
    {
      status = HAL_ERROR;
    }
  }
  return status;
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

#endif /* HAL_SAI_MODULE_ENABLED */
/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
