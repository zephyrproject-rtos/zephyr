/**
  ******************************************************************************
  * @file    stm32mp1xx_ll_delayblock.h
  * @author  MCD Application Team
  * @brief   Header file of Delay Block module.
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
#ifndef __STM32MP1xx_LL_DLYB_H
#define __STM32MP1xx_LL_DLYB_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup DELAYBLOCK_LL
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/ 
/** @defgroup DELAYBLOCK_LL_Exported_Types DELAYBLOCK_LL Exported Types
  * @{
  */
  

/**
  * @}
  */
  
/* Exported constants --------------------------------------------------------*/
/** @defgroup DLYB_Exported_Constants Delay Block Exported Constants
  * @{
  */


#define DLYB_MAX_UNIT   ((uint32_t)0x00000080U) /*!< Max UNIT value (128)  */

/** @defgroup DLYB_Instance DLYB Instance
  * @{
  */
#define IS_DLYB_ALL_INSTANCE(INSTANCE)  (((INSTANCE) == DLYB_SDMMC1) || \
                                         ((INSTANCE) == DLYB_SDMMC2) || \
                                         ((INSTANCE) == DLYB_QUADSPI))
/**
  * @}
  */

/**
  * @}
  */ 
 
/* Peripheral Control functions  ************************************************/
/** @addtogroup HAL_DELAYBLOCK_LL_Group3 Delay Block functions
  * @{
  */
HAL_StatusTypeDef DelayBlock_Enable(DLYB_TypeDef *dlyb);
HAL_StatusTypeDef DelayBlock_Disable(DLYB_TypeDef *dlyb);

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

#endif /* __STM32MP1xx_LL_DLYB_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
