/**
  ******************************************************************************
  * @file    stm32l4xx_hal_pcd_ex.h
  * @author  MCD Application Team
  * @version V1.7.1
  * @date    21-April-2017
  * @brief   Header file of PCD HAL module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L4xx_HAL_PCD_EX_H
#define __STM32L4xx_HAL_PCD_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L442xx) || defined(STM32L443xx) || \
    defined(STM32L452xx) || defined(STM32L462xx) || \
    defined(STM32L475xx) || defined(STM32L476xx) || defined(STM32L485xx) || defined(STM32L486xx) || \
    defined(STM32L496xx) || defined(STM32L4A6xx)

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal_def.h"
   
/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */

/** @addtogroup PCDEx
  * @{
  */
/* Exported types ------------------------------------------------------------*/
typedef enum  
{
  PCD_LPM_L0_ACTIVE = 0x00, /* on */
  PCD_LPM_L1_ACTIVE = 0x01, /* LPM L1 sleep */
}PCD_LPM_MsgTypeDef;

typedef enum  
{
  PCD_BCD_ERROR                     = 0xFF, 
  PCD_BCD_CONTACT_DETECTION         = 0xFE,
  PCD_BCD_STD_DOWNSTREAM_PORT       = 0xFD,
  PCD_BCD_CHARGING_DOWNSTREAM_PORT  = 0xFC,
  PCD_BCD_DEDICATED_CHARGING_PORT   = 0xFB,
  PCD_BCD_DISCOVERY_COMPLETED       = 0x00,
  
}PCD_BCD_MsgTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @addtogroup PCDEx_Exported_Functions PCDEx Exported Functions
  * @{
  */
/** @addtogroup PCDEx_Exported_Functions_Group1 Peripheral Control functions
  * @{
  */

#if defined(USB_OTG_FS)
HAL_StatusTypeDef HAL_PCDEx_SetTxFiFo(PCD_HandleTypeDef *hpcd, uint8_t fifo, uint16_t size);
HAL_StatusTypeDef HAL_PCDEx_SetRxFiFo(PCD_HandleTypeDef *hpcd, uint16_t size);
#endif /* USB_OTG_FS */

#if defined (USB)
HAL_StatusTypeDef  HAL_PCDEx_PMAConfig(PCD_HandleTypeDef *hpcd, 
                                       uint16_t ep_addr,
                                       uint16_t ep_kind,
                                       uint32_t pmaadress);
#endif /* USB */
HAL_StatusTypeDef HAL_PCDEx_ActivateLPM(PCD_HandleTypeDef *hpcd);
HAL_StatusTypeDef HAL_PCDEx_DeActivateLPM(PCD_HandleTypeDef *hpcd);
HAL_StatusTypeDef HAL_PCDEx_ActivateBCD(PCD_HandleTypeDef *hpcd);
HAL_StatusTypeDef HAL_PCDEx_DeActivateBCD(PCD_HandleTypeDef *hpcd);
void HAL_PCDEx_BCD_VBUSDetect(PCD_HandleTypeDef *hpcd);
void HAL_PCDEx_LPM_Callback(PCD_HandleTypeDef *hpcd, PCD_LPM_MsgTypeDef msg);
void HAL_PCDEx_BCD_Callback(PCD_HandleTypeDef *hpcd, PCD_BCD_MsgTypeDef msg);

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

#endif /* STM32L432xx || STM32L433xx || STM32L442xx || STM32L443xx || */
       /* STM32L452xx || STM32L462xx || */
       /* STM32L475xx || STM32L476xx || STM32L485xx || STM32L486xx || */
       /* STM32L496xx || STM32L4A6xx */

#ifdef __cplusplus
}
#endif


#endif /* __STM32L4xx_HAL_PCD_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
