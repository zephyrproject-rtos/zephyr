/**
  ******************************************************************************
  * @file    stm32f3xx_hal_pcd_ex.c
  * @author  MCD Application Team
  * @brief   Extended PCD HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the USB Peripheral Controller:
  *           + Configuration of the PMA for EP
  *
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
#include "stm32f3xx_hal.h"

#ifdef HAL_PCD_MODULE_ENABLED

#if defined(STM32F302xE) || defined(STM32F303xE) || \
    defined(STM32F302xC) || defined(STM32F303xC) || \
    defined(STM32F302x8)                         || \
    defined(STM32F373xC)

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @defgroup PCDEx PCDEx
  * @brief PCD Extended HAL module driver
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions ---------------------------------------------------------*/

/** @defgroup PCDEx_Exported_Functions PCDEx Exported Functions
  * @{
  */

/** @defgroup PCDEx_Exported_Functions_Group1 Peripheral Control functions
  * @brief    PCDEx control functions
  *
@verbatim
 ===============================================================================
              ##### Extended Peripheral Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Update PMA configuration

@endverbatim
  * @{
  */

/**
  * @brief Configure PMA for EP
  * @param  hpcd PCD handle
  * @param  ep_addr endpoint address
  * @param  ep_kind endpoint Kind
  *                @arg USB_SNG_BUF: Single Buffer used
  *                @arg USB_DBL_BUF: Double Buffer used
  * @param  pmaadress EP address in The PMA: In case of single buffer endpoint
  *                   this parameter is 16-bit value providing the address
  *                   in PMA allocated to endpoint.
  *                   In case of double buffer endpoint this parameter
  *                   is a 32-bit value providing the endpoint buffer 0 address
  *                   in the LSB part of 32-bit value and endpoint buffer 1 address
  *                   in the MSB part of 32-bit value.
  * @retval : status
  */

HAL_StatusTypeDef  HAL_PCDEx_PMAConfig(PCD_HandleTypeDef *hpcd,
                        uint16_t ep_addr,
                        uint16_t ep_kind,
                        uint32_t pmaadress)

{
  PCD_EPTypeDef *ep;

  /* initialize ep structure*/
  if ((0x80U & ep_addr) == 0x80U)
  {
    ep = &hpcd->IN_ep[ep_addr & 0x7FU];
  }
  else
  {
    ep = &hpcd->OUT_ep[ep_addr];
  }

  /* Here we check if the endpoint is single or double Buffer*/
  if (ep_kind == PCD_SNG_BUF)
  {
    /*Single Buffer*/
    ep->doublebuffer = 0U;
    /*Configure the PMA*/
    ep->pmaadress = (uint16_t)pmaadress;
  }
  else /*USB_DBL_BUF*/
  {
    /*Double Buffer Endpoint*/
    ep->doublebuffer = 1;
    /*Configure the PMA*/
    ep->pmaaddr0 =  pmaadress & 0xFFFFU;
    ep->pmaaddr1 =  (pmaadress & 0xFFFF0000U) >> 16U;
  }

  return HAL_OK;
}
/**
  * @}
  */

/**
  * @}
  */

/** @defgroup PCDEx_Private_Functions PCD Extended Private Functions
  * @{
  */
#if defined(STM32F303xC)                         || \
    defined(STM32F303x8) || defined(STM32F334x8) || \
    defined(STM32F301x8)                         || \
    defined(STM32F373xC) || defined(STM32F378xx) || \
    defined(STM32F302xC)

/**
  * @brief Copy a buffer from user memory area to packet memory area (PMA)
  * @param   USBx: USB peripheral instance register address.
  * @param   pbUsrBuf: pointer to user memory area.
  * @param   wPMABufAddr: address into PMA.
  * @param   wNBytes: no. of bytes to be copied.
  * @retval None
  */
void PCD_WritePMA(USB_TypeDef  *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n =  ((uint32_t)((uint32_t)wNBytes + 1U)) >> 1U;

  uint32_t i, temp1, temp2;
  uint16_t *pdwVal;
  pdwVal = (uint16_t *)((uint32_t)(wPMABufAddr * 2 + (uint32_t)USBx + 0x400U));

  for (i = n; i != 0; i--)
  {
    temp1 = (uint16_t) * pbUsrBuf;
    pbUsrBuf++;
    temp2 = temp1 | ((uint16_t)((uint16_t)  * pbUsrBuf << 8U)) ;
    *pdwVal++ = temp2;
    pdwVal++;
    pbUsrBuf++;
  }
}

/**
  * @brief Copy a buffer from user memory area to packet memory area (PMA)
  * @param   USBx: USB peripheral instance register address.
  * @param   pbUsrBuf: pointer to user memory area.
  * @param   wPMABufAddr: address into PMA.
  * @param   wNBytes: no. of bytes to be copied.
  * @retval None
  */
void PCD_ReadPMA(USB_TypeDef  *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n = (uint32_t)wNBytes >> 1U;
  uint32_t i;
  uint16_t *pdwVal;
  uint32_t temp;
  pdwVal = (uint16_t *)((uint32_t)(wPMABufAddr * 2 + (uint32_t)USBx + 0x400U));

  for (i = n; i != 0U; i--)
  {
    temp = *pdwVal++;
    *pbUsrBuf++ = ((temp >> 0) & 0xFF);
    *pbUsrBuf++ = ((temp >> 8) & 0xFF);
    pdwVal++;
  }

  if (wNBytes % 2)
  {
    temp = *pdwVal++;
    *pbUsrBuf++ = ((temp >> 0) & 0xFF);
  }
}
#endif /* STM32F303xC                || */
       /* STM32F303x8 || STM32F334x8 || */
       /* STM32F301x8                || */
       /* STM32F373xC || STM32F378xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || \
    defined(STM32F302x8)
/**
  * @brief Copy a buffer from user memory area to packet memory area (PMA)
  * @param   USBx: USB peripheral instance register address.
  * @param   pbUsrBuf: pointer to user memory area.
  * @param   wPMABufAddr: address into PMA.
  * @param   wNBytes: no. of bytes to be copied.
  * @retval None
  */
void PCD_WritePMA(USB_TypeDef  *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n =  ((uint32_t)((uint32_t)wNBytes + 1U)) >> 1U;
  uint32_t i;
  uint16_t temp1, temp2;
  uint16_t *pdwVal;
  pdwVal = (uint16_t *)((uint32_t)(wPMABufAddr + (uint32_t)USBx + 0x400U));
  for (i = n; i != 0U; i--)
  {
    temp1 = (uint16_t) * pbUsrBuf;
    pbUsrBuf++;
    temp2 = temp1 | ((uint16_t)((uint16_t)  * pbUsrBuf << 8U)) ;
    *pdwVal++ = temp2;
    pbUsrBuf++;
  }
}

/**
  * @brief Copy a buffer from user memory area to packet memory area (PMA)
  * @param   USBx: USB peripheral instance register address.
  * @param   pbUsrBuf: pointer to user memory area.
  * @param   wPMABufAddr: address into PMA.
  * @param   wNBytes: no. of bytes to be copied.
  * @retval None
  */
void PCD_ReadPMA(USB_TypeDef  *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n = (uint32_t)wNBytes >> 1U;
  uint32_t i;
  uint16_t *pdwVal;
  uint32_t temp;
  pdwVal = (uint16_t *)((uint32_t)(wPMABufAddr + (uint32_t)USBx + 0x400U));

  for (i = n; i != 0U; i--)
  {
    temp = *pdwVal++;
    *pbUsrBuf++ = ((temp >> 0) & 0xFF);
    *pbUsrBuf++ = ((temp >> 8) & 0xFF);
  }

  if (wNBytes % 2)
  {
    temp = *pdwVal++;
    *pbUsrBuf++ = ((temp >> 0) & 0xFF);
  }
}
#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC                || */
       /* STM32F302x8                   */

/**
  * @}
  */

/** @addtogroup PCDEx_Exported_Functions PCDEx Exported Functions
  * @{
  */

/** @addtogroup PCDEx_Exported_Functions_Group2 Extended Initialization and de-initialization functions
  * @{
  */
/**
  * @brief  Software Device Connection
  * @param  hpcd PCD handle
  * @param  state Device state
  * @retval None
  */
 __weak void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hpcd);
  UNUSED(state);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PCDEx_SetConnectionState could be implenetd in the user file
   */
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

/**
  * @}
  */

#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F302x8                || */
       /* STM32F373xC                   */

#endif /* HAL_PCD_MODULE_ENABLED */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
