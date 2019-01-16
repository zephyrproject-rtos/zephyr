/**
  ******************************************************************************
  * @file    stm32g0xx_hal_flash_ex.h
  * @author  MCD Application Team
  * @brief   Header file of FLASH HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
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
#ifndef STM32G0xx_HAL_FLASH_EX_H
#define STM32G0xx_HAL_FLASH_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_def.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup FLASHEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup FLASHEx_Exported_Constants FLASH Exported Constants
  * @{
  */
/** @defgroup FLASHEx_Empty_Check FLASHEx Empty Check
  * @{
  */
#define FLASH_PROG_NOT_EMPTY                0x00000000u         /*!< 1st location in Flash is programmed */
#define FLASH_PROG_EMPTY                    FLASH_ACR_PROGEMPTY /*!< 1st location in Flash is empty */
/**
  * @}
  */
/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @addtogroup FLASHEx_Exported_Functions
  * @{
  */

/* Extended Program operation functions  *************************************/
/** @addtogroup FLASHEx_Exported_Functions_Group1
  * @{
  */
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *PageError);
HAL_StatusTypeDef HAL_FLASHEx_Erase_IT(FLASH_EraseInitTypeDef *pEraseInit);
void              HAL_FLASHEx_EnableDebugger(void);
void              HAL_FLASHEx_DisableDebugger(void);
uint32_t          HAL_FLASHEx_FlashEmptyCheck(void);
void              HAL_FLASHEx_ForceFlashEmpty(uint32_t FlashEmpty);
#if defined(FLASH_SECURABLE_MEMORY_SUPPORT)
void              HAL_FLASHEx_EnableSecMemProtection(uint32_t Bank);
#endif
HAL_StatusTypeDef HAL_FLASHEx_OBProgram(FLASH_OBProgramInitTypeDef *pOBInit);
void              HAL_FLASHEx_OBGetConfig(FLASH_OBProgramInitTypeDef *pOBInit);
/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup FLASHEx_Private_Constants FLASHEx Private Constants
  * @{
  */
#define FLASH_PCROP_GRANULARITY_OFFSET             9u
#define FLASH_PCROP_GRANULARITY                    (1u << FLASH_PCROP_GRANULARITY_OFFSET)
/**
  * @}
  */


/** @defgroup FLASHEx_Private_Macros FLASHEx Private Macros
  *  @{
  */
#define IS_FLASH_EMPTY_CHECK(__VALUE__)     (((__VALUE__) == FLASH_PROG_EMPTY) || ((__VALUE__) == FLASH_PROG_NOT_EMPTY))

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

#endif /* STM32G0xx_HAL_FLASH_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
