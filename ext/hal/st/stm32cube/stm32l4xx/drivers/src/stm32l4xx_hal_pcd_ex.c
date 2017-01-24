/**
  ******************************************************************************
  * @file    stm32l4xx_hal_pcd_ex.c
  * @author  MCD Application Team
  * @version V1.6.0
  * @date    28-October-2016
  * @brief   PCD Extended HAL module driver.  
  *          This file provides firmware functions to manage the following 
  *          functionalities of the USB Peripheral Controller:
  *           + Extended features functions
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
#include "stm32l4xx_hal.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */
#ifdef HAL_PCD_MODULE_ENABLED

#if defined(STM32L432xx) || defined(STM32L433xx) || defined(STM32L442xx) || defined(STM32L443xx) || \
    defined(STM32L452xx) || defined(STM32L462xx) || \
    defined(STM32L475xx) || defined(STM32L476xx) || defined(STM32L485xx) || defined(STM32L486xx)

/** @defgroup PCDEx PCDEx
  * @brief PCD Extended HAL module driver
  * @{
  */

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
#if defined (USB_OTG_FS)
/**
  * @brief  Set Tx FIFO
  * @param  hpcd: PCD handle
  * @param  fifo: The number of Tx fifo
  * @param  size: Fifo size
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_SetTxFiFo(PCD_HandleTypeDef *hpcd, uint8_t fifo, uint16_t size)
{
  uint8_t index = 0;
  uint32_t Tx_Offset = 0;

  /*  TXn min size = 16 words. (n  : Transmit FIFO index)
      When a TxFIFO is not used, the Configuration should be as follows: 
          case 1 :  n > m    and Txn is not used    (n,m  : Transmit FIFO indexes)
         --> Txm can use the space allocated for Txn.
         case2  :  n < m    and Txn is not used    (n,m  : Transmit FIFO indexes)
         --> Txn should be configured with the minimum space of 16 words
     The FIFO is used optimally when used TxFIFOs are allocated in the top 
         of the FIFO.Ex: use EP1 and EP2 as IN instead of EP1 and EP3 as IN ones.
     When DMA is used 3n * FIFO locations should be reserved for internal DMA registers */
  
  Tx_Offset = hpcd->Instance->GRXFSIZ;
  
  if(fifo == 0)
  {
    hpcd->Instance->DIEPTXF0_HNPTXFSIZ = (size << 16) | Tx_Offset;
  }
  else
  {
    Tx_Offset += (hpcd->Instance->DIEPTXF0_HNPTXFSIZ) >> 16;
    for (index = 0; index < (fifo - 1); index++)
    {
      Tx_Offset += (hpcd->Instance->DIEPTXF[index] >> 16);
    }
    
    /* Multiply Tx_Size by 2 to get higher performance */
    hpcd->Instance->DIEPTXF[fifo - 1] = (size << 16) | Tx_Offset;
  }
  
  return HAL_OK;
}

/**
  * @brief  Set Rx FIFO
  * @param  hpcd: PCD handle
  * @param  size: Size of Rx fifo
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_SetRxFiFo(PCD_HandleTypeDef *hpcd, uint16_t size)
{
  hpcd->Instance->GRXFSIZ = size;
  
  return HAL_OK;
}

/**
  * @brief  Activate LPM feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateLPM(PCD_HandleTypeDef *hpcd)
{
  USB_OTG_GlobalTypeDef *USBx = hpcd->Instance;  
  
  hpcd->lpm_active = ENABLE;
  hpcd->LPM_State = LPM_L0;
  USBx->GINTMSK |= USB_OTG_GINTMSK_LPMINTM;
  USBx->GLPMCFG |= (USB_OTG_GLPMCFG_LPMEN | USB_OTG_GLPMCFG_LPMACK | USB_OTG_GLPMCFG_ENBESL);

  return HAL_OK;  
}

/**
  * @brief  Deactivate LPM feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateLPM(PCD_HandleTypeDef *hpcd)
{
  USB_OTG_GlobalTypeDef *USBx = hpcd->Instance;  
  
  hpcd->lpm_active = DISABLE;
  USBx->GINTMSK &= ~USB_OTG_GINTMSK_LPMINTM;
  USBx->GLPMCFG &= ~(USB_OTG_GLPMCFG_LPMEN | USB_OTG_GLPMCFG_LPMACK | USB_OTG_GLPMCFG_ENBESL);
  
  return HAL_OK;  
}

/**
  * @brief  Handle BatteryCharging Process.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
void HAL_PCDEx_BCD_VBUSDetect(PCD_HandleTypeDef *hpcd)
{
  USB_OTG_GlobalTypeDef *USBx = hpcd->Instance;  
  uint32_t tickstart = HAL_GetTick();
  
  /* Start BCD When device is connected */
  if (USBx_DEVICE->DCTL & USB_OTG_DCTL_SDIS)
  { 
    /* Enable DCD : Data Contact Detect */
    USBx->GCCFG |= USB_OTG_GCCFG_DCDEN;
    
    /* Wait Detect flag or a timeout is happen*/
    while ((USBx->GCCFG & USB_OTG_GCCFG_DCDET) == 0)
    {
      /* Check for the Timeout */
      if((HAL_GetTick() - tickstart ) > 1000)
      {
        HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_ERROR);
        return;
      }
    }
    
    /* Right response got */
    HAL_Delay(100); 
    
    /* Check Detect flag*/
    if (USBx->GCCFG & USB_OTG_GCCFG_DCDET)
    {
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CONTACT_DETECTION);
    }
    
    /*Primary detection: checks if connected to Standard Downstream Port  
    (without charging capability) */
    USBx->GCCFG &=~ USB_OTG_GCCFG_DCDEN;
    USBx->GCCFG |=  USB_OTG_GCCFG_PDEN;
    HAL_Delay(100); 
    
    if (!(USBx->GCCFG & USB_OTG_GCCFG_PDET))
    {
      /* Case of Standard Downstream Port */
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_STD_DOWNSTREAM_PORT);
    }
    else  
    {
      /* start secondary detection to check connection to Charging Downstream 
      Port or Dedicated Charging Port */
      USBx->GCCFG &=~ USB_OTG_GCCFG_PDEN;
      USBx->GCCFG |=  USB_OTG_GCCFG_SDEN;
      HAL_Delay(100); 
      
      if ((USBx->GCCFG) & USB_OTG_GCCFG_SDET)
      { 
        /* case Dedicated Charging Port  */
        HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DEDICATED_CHARGING_PORT);
      }
      else
      {
        /* case Charging Downstream Port  */
        HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CHARGING_DOWNSTREAM_PORT);
      }
    }
    /* Battery Charging capability discovery finished */
    HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DISCOVERY_COMPLETED);
  }
}

/**
  * @brief  Activate BatteryCharging feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_OTG_GlobalTypeDef *USBx = hpcd->Instance;  

  hpcd->battery_charging_active = ENABLE; 
  USBx->GCCFG |= (USB_OTG_GCCFG_BCDEN);
  
  return HAL_OK;  
}

/**
  * @brief  Deactivate BatteryCharging feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_OTG_GlobalTypeDef *USBx = hpcd->Instance;  
  hpcd->battery_charging_active = DISABLE; 
  USBx->GCCFG &= ~(USB_OTG_GCCFG_BCDEN);
  return HAL_OK;  
}
#endif /* USB_OTG_FS */

#if defined (USB)
/**
  * @brief  Configure PMA for EP
  * @param  hpcd : Device instance
  * @param  ep_addr: endpoint address
  * @param  ep_kind: endpoint Kind
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
  PCD_EPTypeDef *ep = NULL;
  
  /* initialize ep structure*/
  if ((0x80 & ep_addr) == 0x80)
  {
    ep = &hpcd->IN_ep[ep_addr & 0x7F];
  }
  else
  {
    ep = &hpcd->OUT_ep[ep_addr];
  }
  
  /* Here we check if the endpoint is single or double Buffer*/
  if (ep_kind == PCD_SNG_BUF)
  {
    /*Single Buffer*/
    ep->doublebuffer = 0;
    /*Configure te PMA*/
    ep->pmaadress = (uint16_t)pmaadress;
  }
  else /*USB_DBL_BUF*/
  {
    /*Double Buffer Endpoint*/
    ep->doublebuffer = 1;
    /*Configure the PMA*/
    ep->pmaaddr0 =  pmaadress & 0xFFFF;
    ep->pmaaddr1 =  (pmaadress & 0xFFFF0000) >> 16;
  }
  
  return HAL_OK; 
}

/**
  * @brief  Activate BatteryCharging feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->battery_charging_active = ENABLE;
  
  USBx->BCDR |= (USB_BCDR_BCDEN);
  /* Enable DCD : Data Contact Detect */
  USBx->BCDR |= (USB_BCDR_DCDEN);
  
  return HAL_OK;  
}

/**
  * @brief  Deactivate BatteryCharging feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateBCD(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->battery_charging_active = DISABLE;
  
  USBx->BCDR &= ~(USB_BCDR_BCDEN);
  return HAL_OK;  
}

/**
  * @brief  Handle BatteryCharging Process.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
void HAL_PCDEx_BCD_VBUSDetect(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance;
  uint32_t tickstart = HAL_GetTick();
   
  /* Wait Detect flag or a timeout is happen*/
  while ((USBx->BCDR & USB_BCDR_DCDET) == 0)
  {
    /* Check for the Timeout */
    if((HAL_GetTick() - tickstart ) > 1000)
    {
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_ERROR);
      return;
    }
  }

  HAL_Delay(300);
  
  /* Data Pin Contact ? Check Detect flag */
  if (USBx->BCDR & USB_BCDR_DCDET)
  {
    HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CONTACT_DETECTION);
  }   
  /* Primary detection: checks if connected to Standard Downstream Port  
  (without charging capability) */
  USBx->BCDR &= ~(USB_BCDR_DCDEN);
  USBx->BCDR |= (USB_BCDR_PDEN);
  HAL_Delay(300);
  
  /* If Charger detect ? */
  if (USBx->BCDR & USB_BCDR_PDET)
  {
    /* Start secondary detection to check connection to Charging Downstream 
    Port or Dedicated Charging Port */
    USBx->BCDR &= ~(USB_BCDR_PDEN);
    USBx->BCDR |= (USB_BCDR_SDEN);
    HAL_Delay(300);
    
    /* If CDP ? */
    if (USBx->BCDR & USB_BCDR_SDET)
    {
      /* Dedicated Downstream Port DCP */
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DEDICATED_CHARGING_PORT);
    }
    else
    {
      /* Charging Downstream Port CDP */
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_CHARGING_DOWNSTREAM_PORT);
      
      /* Battery Charging capability discovery finished 
      Start Enumeration*/
      HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_DISCOVERY_COMPLETED);
    }
  }
  else /* NO */
  {
    /* Standard Downstream Port */
    HAL_PCDEx_BCD_Callback(hpcd, PCD_BCD_STD_DOWNSTREAM_PORT);
  }
}

/**
  * @brief  Activate LPM feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_ActivateLPM(PCD_HandleTypeDef *hpcd)
{

  USB_TypeDef *USBx = hpcd->Instance;
  hpcd->lpm_active = ENABLE;
  hpcd->LPM_State = LPM_L0;
  
  USBx->LPMCSR |= (USB_LPMCSR_LMPEN);
  USBx->LPMCSR |= (USB_LPMCSR_LPMACK);
    
 
  return HAL_OK;  
}

/**
  * @brief  Deactivate LPM feature.
  * @param  hpcd: PCD handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PCDEx_DeActivateLPM(PCD_HandleTypeDef *hpcd)
{
  USB_TypeDef *USBx = hpcd->Instance; 
  
  hpcd->lpm_active = DISABLE;
  
  USBx->LPMCSR &= ~ (USB_LPMCSR_LMPEN);
  USBx->LPMCSR &= ~ (USB_LPMCSR_LPMACK);
  
  return HAL_OK;  
}

#endif /* USB */

/**
  * @brief  Send LPM message to user layer callback.
  * @param  hpcd: PCD handle
  * @param  msg: LPM message
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
  * @param  hpcd: PCD handle
  * @param  msg: LPM message
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
/**
  * @}
  */

/**
  * @}
  */

#endif /* STM32L432xx || STM32L433xx || STM32L442xx || STM32L443xx || */
       /* STM32L452xx || STM32L462xx || */
       /* STM32L475xx || STM32L476xx || STM32L485xx || STM32L486xx */

#endif /* HAL_PCD_MODULE_ENABLED */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
