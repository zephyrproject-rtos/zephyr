/**
  ******************************************************************************
  * @file    stm32wbxx_hal_pcd_ex.c
  * @author  MCD Application Team
  * @brief   PCD Extended HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the USB Peripheral Controller:
  *           + Extended features functions
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
#include "stm32wbxx_hal.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup PCDEx PCDEx
  * @brief PCD Extended HAL module driver
  * @{
  */

#ifdef HAL_PCD_MODULE_ENABLED

#if defined (USB)
/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

/** @defgroup PCDEx_Exported_Functions PCDEx Exported Functions
  * @{
  */

/** @defgroup PCDEx_Exported_Functions_Group1 Peripheral Control functions
  * @brief    PCDEx control functions
 *
@verbatim
 ===============================================================================
                 ##### Extended features functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Update FIFO configuration

@endverbatim
  * @{
  */

/**
  * @brief  Configure PMA for EP
  * @param  hpcd  Device instance
  * @param  ep_addr endpoint address
  * @param  ep_kind endpoint Kind
  *                  USB_SNG_BUF: Single Buffer used
  *                  USB_DBL_BUF: Double Buffer used
  * @param  pmaadress: EP address in The PMA: In case of single buffer endpoint
  *                   this parameter is 16-bit value providing the address
  *                   in PMA allocated to endpoint.
  *                   In case of double buffer endpoint this parameter
  *                   is a 32-bit value providing the endpoint buffer 0 address
  *                   in the LSB part of 32-bit value and endpoint buffer 1 address
  *                   in the MSB part of 32-bit value.
  * @retval HAL status
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
    ep = &hpcd->IN_ep[ep_addr & EP_ADDR_MSK];
  }
  else
  {
    ep = &hpcd->OUT_ep[ep_addr];
  }

  /* Here we check if the endpoint is single or double Buffer*/
  if (ep_kind == PCD_SNG_BUF)
  {
    /* Single Buffer */
    ep->doublebuffer = 0U;
    /* Configure the PMA */
    ep->pmaadress = (uint16_t)pmaadress;
  }
  else /* USB_DBL_BUF */
  {
    /* Double Buffer Endpoint */
    ep->doublebuffer = 1U;
    /* Configure the PMA */
    ep->pmaaddr0 = (uint16_t)(pmaadress & 0xFFFFU);
    ep->pmaaddr1 = (uint16_t)((pmaadress & 0xFFFF0000U) >> 16);
  }

  return HAL_OK;
}

/**
  * @brief  Activate BatteryCharging feature.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->battery_charging_active = 1U;

  /* Enable DCD : Data Contact Detect */
  USBx->BCDR &= ~(USB_BCDR_PDEN);
  USBx->BCDR &= ~(USB_BCDR_SDEN);
  USBx->BCDR |= USB_BCDR_DCDEN;

  return HAL_OK;
}

/**
  * @brief  Deactivate BatteryCharging feature.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->battery_charging_active = 0U;

  USBx->BCDR &= ~(USB_BCDR_BCDEN);

  return HAL_OK;
}

/**
  * @brief  Handle BatteryCharging Process.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
void HAL_PCDEx_BCD_VBUSDetect(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  uint32_t tickstart = HAL_GetTick();

  /* Wait Detect flag or a timeout is happen*/
  while ((USBx->BCDR & USB_BCDR_DCDET) == 0U)
  {
    /* Check for the Timeout */
    if ((HAL_GetTick() - tickstart) > 1000U)
    {
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
      hpcd->BCDCallback(hpcd, PCD_BCD_ERROR);
#else
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_ERROR);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */

      return;
    }
  }

  HAL_Delay(200U);

  /* Data Pin Contact ? Check Detect flag */
  if ((USBx->BCDR & USB_BCDR_DCDET) == USB_BCDR_DCDET)
  {
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
    hpcd->BCDCallback(hpcd, PCD_BCD_CONTACT_DETECTION);
#else
    HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CONTACT_DETECTION);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
  }
  /* Primary detection: checks if connected to Standard Downstream Port
  (without charging capability) */
  USBx->BCDR &= ~(USB_BCDR_DCDEN);
  HAL_Delay(50U);
  USBx->BCDR |= (USB_BCDR_PDEN);
  HAL_Delay(50U);

  /* If Charger detect ? */
  if ((USBx->BCDR & USB_BCDR_PDET) == USB_BCDR_PDET)
  {
    /* Start secondary detection to check connection to Charging Downstream
    Port or Dedicated Charging Port */
    USBx->BCDR &= ~(USB_BCDR_PDEN);
    HAL_Delay(50U);
    USBx->BCDR |= (USB_BCDR_SDEN);
    HAL_Delay(50U);

    /* If CDP ? */
    if ((USBx->BCDR & USB_BCDR_SDET) == USB_BCDR_SDET)
    {
      /* Dedicated Downstream Port DCP */
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
      hpcd->BCDCallback(hpcd, PCD_BCD_DEDICATED_CHARGING_PORT);
#else
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DEDICATED_CHARGING_PORT);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    }
    else
    {
      /* Charging Downstream Port CDP */
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
      hpcd->BCDCallback(hpcd, PCD_BCD_CHARGING_DOWNSTREAM_PORT);
#else
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CHARGING_DOWNSTREAM_PORT);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    }
  }
  else /* NO */
  {
    /* Standard Downstream Port */
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
    hpcd->BCDCallback(hpcd, PCD_BCD_STD_DOWNSTREAM_PORT);
#else
    HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_STD_DOWNSTREAM_PORT);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
  }

  /* Battery Charging capability discovery finished Start Enumeration */
  (void)HAL_PCDEx_DeActivateBCD(hpcd);
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
  hpcd->BCDCallback(hpcd, PCD_BCD_DISCOVERY_COMPLETED);
#else
  HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DISCOVERY_COMPLETED);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
}


/**
  * @brief  Activate LPM feature.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateLPM(PCD_HandleTypeDef *hpcd)
{

  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->lpm_active = 1U;
  hpcd->LPM_State = LPM_L0;

  USBx->LPMCSR |= USB_LPMCSR_LMPEN;
  USBx->LPMCSR |= USB_LPMCSR_LPMACK;

  return HAL_OK;
}

/**
  * @brief  Deactivate LPM feature.
  * @param  hpcd PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateLPM(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;

  hpcd->lpm_active = 0U;

  USBx->LPMCSR &= ~(USB_LPMCSR_LMPEN);
  USBx->LPMCSR &= ~(USB_LPMCSR_LPMACK);

  return HAL_OK;
}



/**
  * @brief  Send LPM message to user layer callback.
  * @param  hpcd PCD handle
  * @param  msg LPM message
  * @retval HAL status
  */
__weak void HAL_PCDEx_LPM_Callback(PCD_HandleTypeDef *hpcd, PCD_LPM_MsgTypeDef msg)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hpcd);
  UNUSED(msg);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_PCDEx_LPM_Callback could be implemented in the user file
   */
}

/**
  * @brief  Send BatteryCharging message to user layer callback.
  * @param  hpcd PCD handle
  * @param  msg LPM message
  * @retval HAL status
  */
__weak void HAL_PCDEx_BCD_Callback(PCD_HandleTypeDef *hpcd, PCD_BCD_MsgTypeDef msg)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hpcd);
  UNUSED(msg);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_PCDEx_BCD_Callback could be implemented in the user file
   */
}

/**
  * @}
  */

/**
  * @}
  */
#endif /* defined (USB) */
#endif /* HAL_PCD_MODULE_ENABLED */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
