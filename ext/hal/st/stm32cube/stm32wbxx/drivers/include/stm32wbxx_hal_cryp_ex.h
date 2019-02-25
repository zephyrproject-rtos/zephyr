/**
  ******************************************************************************
  * @file    stm32wbxx_hal_cryp_ex.h
  * @author  MCD Application Team
  * @brief   Header file of CRYPEx HAL module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32WBxx_HAL_CRYP_EX_H
#define STM32WBxx_HAL_CRYP_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */



/** @defgroup CRYPEx CRYPEx
  * @brief CRYP Extension HAL module driver.
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Private types -------------------------------------------------------------*/
/** @defgroup CRYPEx_Private_Types CRYPEx Private Types
  * @{
  */

/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/
/** @defgroup CRYPEx_Private_Variables CRYPEx Private Variables
  * @{
  */

/**
  * @}
  */

/* Private constants ---------------------------------------------------------*/
/** @defgroup CRYPEx_Private_Constants CRYPEx Private Constants
  * @{
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup CRYPEx_Private_Macros CRYPEx Private Macros
  * @{
  */

/**
 * @}
 */

/* Private functions ---------------------------------------------------------*/
/** @defgroup CRYPEx_Private_Functions CRYPEx Private Functions
  * @{
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup CRYPEx_Exported_Functions CRYPEx Exported Functions
  * @{
  */

/** @addtogroup CRYPEx_Exported_Functions_Group1
  * @{
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout);
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout);

/**
  * @}
  */

/** @addtogroup CRYPEx_Exported_Functions_Group2
  * @{
  */
void  HAL_CRYPEx_EnableAutoKeyDerivation(CRYP_HandleTypeDef *hcryp);
void  HAL_CRYPEx_DisableAutoKeyDerivation(CRYP_HandleTypeDef *hcryp);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */


/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WBxx_HAL_CRYP_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
