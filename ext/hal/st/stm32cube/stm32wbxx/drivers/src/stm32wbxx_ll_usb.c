/**
  ******************************************************************************
  * @file    stm32wbxx_ll_usb.c
  * @author  MCD Application Team
  * @brief   USB Low Layer HAL module driver.
  *
  *          This file provides firmware functions to manage the following
  *          functionalities of the USB Peripheral Controller:
  *           + Initialization/de-initialization functions
  *           + I/O operation functions
  *           + Peripheral Control functions
  *           + Peripheral State functions
  *
  @verbatim
  ==============================================================================
                    ##### How to use this driver #####
  ==============================================================================
    [..]
      (#) Fill parameters of Init structure in USB_OTG_CfgTypeDef structure.

      (#) Call USB_CoreInit() API to initialize the USB Core peripheral.

      (#) The upper HAL HCD/PCD driver will call the right routines for its internal processes.

  @endverbatim
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

/** @addtogroup STM32WBxx_LL_USB_DRIVER
  * @{
  */

#if defined (HAL_PCD_MODULE_ENABLED) || defined (HAL_HCD_MODULE_ENABLED)
#if defined (USB)
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


/**
  * @brief  Initializes the USB Core
  * @param  USBx: USB Instance
  * @param  cfg : pointer to a USB_CfgTypeDef structure that contains
  *         the configuration information for the specified USBx peripheral.
  * @retval HAL status
  */
HAL_StatusTypeDef USB_CoreInit(USB_TypeDef *USBx, USB_CfgTypeDef cfg)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(cfg);

  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */

  return HAL_OK;
}

/**
  * @brief  USB_EnableGlobalInt
  *         Enables the controller's Global Int in the AHB Config reg
  * @param  USBx : Selected device
  * @retval HAL status
  */
HAL_StatusTypeDef USB_EnableGlobalInt(USB_TypeDef *USBx)
{
  uint16_t winterruptmask;

  /* Set winterruptmask variable */
  winterruptmask = USB_CNTR_CTRM  | USB_CNTR_WKUPM |
                   USB_CNTR_SUSPM | USB_CNTR_ERRM |
                   USB_CNTR_SOFM | USB_CNTR_ESOFM |
                   USB_CNTR_RESETM | USB_CNTR_L1REQM;

  /* Set interrupt mask */
  USBx->CNTR |= winterruptmask;

  return HAL_OK;
}

/**
  * @brief  USB_DisableGlobalInt
  *         Disable the controller's Global Int in the AHB Config reg
  * @param  USBx : Selected device
  * @retval HAL status
*/
HAL_StatusTypeDef USB_DisableGlobalInt(USB_TypeDef *USBx)
{
  uint16_t winterruptmask;

  /* Set winterruptmask variable */
  winterruptmask = USB_CNTR_CTRM  | USB_CNTR_WKUPM |
                   USB_CNTR_SUSPM | USB_CNTR_ERRM |
                   USB_CNTR_SOFM | USB_CNTR_ESOFM |
                   USB_CNTR_RESETM | USB_CNTR_L1REQM;

  /* Clear interrupt mask */
  USBx->CNTR &= ~winterruptmask;

  return HAL_OK;
}

/**
  * @brief  USB_SetCurrentMode : Set functional mode
  * @param  USBx : Selected device
  * @param  mode :  current core mode
  *          This parameter can be one of the these values:
  *            @arg USB_DEVICE_MODE: Peripheral mode mode
  * @retval HAL status
  */
HAL_StatusTypeDef USB_SetCurrentMode(USB_TypeDef *USBx, USB_ModeTypeDef mode)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(mode);

  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return HAL_OK;
}

/**
  * @brief  USB_DevInit : Initializes the USB controller registers
  *         for device mode
  * @param  USBx : Selected device
  * @param  cfg  : pointer to a USB_CfgTypeDef structure that contains
  *         the configuration information for the specified USBx peripheral.
  * @retval HAL status
  */
HAL_StatusTypeDef USB_DevInit(USB_TypeDef *USBx, USB_CfgTypeDef cfg)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(cfg);

  /* Init Device */
  /*CNTR_FRES = 1*/
  USBx->CNTR = USB_CNTR_FRES;

  /*CNTR_FRES = 0*/
  USBx->CNTR = 0;

  /*Clear pending interrupts*/
  USBx->ISTR = 0;

  /*Set Btable Address*/
  USBx->BTABLE = BTABLE_ADDRESS;

  /* Enable USB Device Interrupt mask */
  (void)USB_EnableGlobalInt(USBx);

  return HAL_OK;
}

/**
  * @brief  USB_SetDevSpeed :Initializes the device speed
  *         depending on the PHY type and the enumeration speed of the device.
  * @param  USBx  Selected device
  * @param  speed  device speed
  * @retval  Hal status
  */
HAL_StatusTypeDef USB_SetDevSpeed(USB_TypeDef *USBx, uint8_t speed)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(speed);

  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */

  return HAL_OK;
}

/**
  * @brief  USB_FlushTxFifo : Flush a Tx FIFO
  * @param  USBx : Selected device
  * @param  num : FIFO number
  *         This parameter can be a value from 1 to 15
            15 means Flush all Tx FIFOs
  * @retval HAL status
  */
HAL_StatusTypeDef USB_FlushTxFifo(USB_TypeDef *USBx, uint32_t num)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(num);

  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */

  return HAL_OK;
}

/**
  * @brief  USB_FlushRxFifo : Flush Rx FIFO
  * @param  USBx : Selected device
  * @retval HAL status
  */
HAL_StatusTypeDef USB_FlushRxFifo(USB_TypeDef *USBx)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);

  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */

  return HAL_OK;
}

/**
  * @brief  Activate and configure an endpoint
  * @param  USBx : Selected device
  * @param  ep: pointer to endpoint structure
  * @retval HAL status
  */
HAL_StatusTypeDef USB_ActivateEndpoint(USB_TypeDef *USBx, USB_EPTypeDef *ep)
{
  HAL_StatusTypeDef ret = HAL_OK;
  uint16_t wEpRegVal;

  wEpRegVal = PCD_GET_ENDPOINT(USBx, ep->num) & USB_EP_T_MASK;

  /* initialize Endpoint */
  switch (ep->type)
  {
    case EP_TYPE_CTRL:
      wEpRegVal |= USB_EP_CONTROL;
      break;

    case EP_TYPE_BULK:
      wEpRegVal |= USB_EP_BULK;
      break;

    case EP_TYPE_INTR:
      wEpRegVal |= USB_EP_INTERRUPT;
      break;

    case EP_TYPE_ISOC:
      wEpRegVal |= USB_EP_ISOCHRONOUS;
      break;

    default:
      ret = HAL_ERROR;
      break;
  }

  PCD_SET_ENDPOINT(USBx, ep->num, wEpRegVal | USB_EP_CTR_RX | USB_EP_CTR_TX);

  PCD_SET_EP_ADDRESS(USBx, ep->num, ep->num);

  if (ep->doublebuffer == 0U)
  {
    if (ep->is_in != 0U)
    {
      /*Set the endpoint Transmit buffer address */
      PCD_SET_EP_TX_ADDRESS(USBx, ep->num, ep->pmaadress);
      PCD_CLEAR_TX_DTOG(USBx, ep->num);

      if (ep->type != EP_TYPE_ISOC)
      {
        /* Configure NAK status for the Endpoint */
        PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_NAK);
      }
      else
      {
        /* Configure TX Endpoint to disabled state */
        PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_DIS);
      }
    }
    else
    {
      /*Set the endpoint Receive buffer address */
      PCD_SET_EP_RX_ADDRESS(USBx, ep->num, ep->pmaadress);
      /*Set the endpoint Receive buffer counter*/
      PCD_SET_EP_RX_CNT(USBx, ep->num, ep->maxpacket);
      PCD_CLEAR_RX_DTOG(USBx, ep->num);
      /* Configure VALID status for the Endpoint*/
      PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_VALID);
    }
  }
  /*Double Buffer*/
  else
  {
    /* Set the endpoint as double buffered */
    PCD_SET_EP_DBUF(USBx, ep->num);
    /* Set buffer address for double buffered mode */
    PCD_SET_EP_DBUF_ADDR(USBx, ep->num, ep->pmaaddr0, ep->pmaaddr1);

    if (ep->is_in == 0U)
    {
      /* Clear the data toggle bits for the endpoint IN/OUT */
      PCD_CLEAR_RX_DTOG(USBx, ep->num);
      PCD_CLEAR_TX_DTOG(USBx, ep->num);

      /* Reset value of the data toggle bits for the endpoint out */
      PCD_TX_DTOG(USBx, ep->num);

      PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_VALID);
      PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_DIS);
    }
    else
    {
      /* Clear the data toggle bits for the endpoint IN/OUT */
      PCD_CLEAR_RX_DTOG(USBx, ep->num);
      PCD_CLEAR_TX_DTOG(USBx, ep->num);
      PCD_RX_DTOG(USBx, ep->num);

      if (ep->type != EP_TYPE_ISOC)
      {
        /* Configure NAK status for the Endpoint */
        PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_NAK);
      }
      else
      {
        /* Configure TX Endpoint to disabled state */
        PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_DIS);
      }

      PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_DIS);
    }
  }

  return ret;
}

/**
  * @brief  De-activate and de-initialize an endpoint
  * @param  USBx : Selected device
  * @param  ep: pointer to endpoint structure
  * @retval HAL status
  */
HAL_StatusTypeDef USB_DeactivateEndpoint(USB_TypeDef *USBx, USB_EPTypeDef *ep)
{
  if (ep->doublebuffer == 0U)
  {
    if (ep->is_in != 0U)
    {
      PCD_CLEAR_TX_DTOG(USBx, ep->num);
      /* Configure DISABLE status for the Endpoint*/
      PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_DIS);
    }
    else
    {
      PCD_CLEAR_RX_DTOG(USBx, ep->num);
      /* Configure DISABLE status for the Endpoint*/
      PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_DIS);
    }
  }
  /*Double Buffer*/
  else
  {
    if (ep->is_in == 0U)
    {
      /* Clear the data toggle bits for the endpoint IN/OUT*/
      PCD_CLEAR_RX_DTOG(USBx, ep->num);
      PCD_CLEAR_TX_DTOG(USBx, ep->num);

      /* Reset value of the data toggle bits for the endpoint out*/
      PCD_TX_DTOG(USBx, ep->num);

      PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_DIS);
      PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_DIS);
    }
    else
    {
      /* Clear the data toggle bits for the endpoint IN/OUT*/
      PCD_CLEAR_RX_DTOG(USBx, ep->num);
      PCD_CLEAR_TX_DTOG(USBx, ep->num);
      PCD_RX_DTOG(USBx, ep->num);
      /* Configure DISABLE status for the Endpoint*/
      PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_DIS);
      PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_DIS);
    }
  }

  return HAL_OK;
}

/**
  * @brief  USB_EPStartXfer : setup and starts a transfer over an EP
  * @param  USBx : Selected device
  * @param  ep: pointer to endpoint structure
  * @retval HAL status
  */
HAL_StatusTypeDef USB_EPStartXfer(USB_TypeDef *USBx, USB_EPTypeDef *ep)
{
  uint16_t pmabuffer;
  uint32_t len;

  /* IN endpoint */
  if (ep->is_in == 1U)
  {
    /*Multi packet transfer*/
    if (ep->xfer_len > ep->maxpacket)
    {
      len = ep->maxpacket;
      ep->xfer_len -= len;
    }
    else
    {
      len = ep->xfer_len;
      ep->xfer_len = 0U;
    }

    /* configure and validate Tx endpoint */
    if (ep->doublebuffer == 0U)
    {
      USB_WritePMA(USBx, ep->xfer_buff, ep->pmaadress, (uint16_t)len);
      PCD_SET_EP_TX_CNT(USBx, ep->num, len);
    }
    else
    {
      /* Write the data to the USB endpoint */
      if ((PCD_GET_ENDPOINT(USBx, ep->num) & USB_EP_DTOG_TX) != 0U)
      {
        /* Set the Double buffer counter for pmabuffer1 */
        PCD_SET_EP_DBUF1_CNT(USBx, ep->num, ep->is_in, len);
        pmabuffer = ep->pmaaddr1;
      }
      else
      {
        /* Set the Double buffer counter for pmabuffer0 */
        PCD_SET_EP_DBUF0_CNT(USBx, ep->num, ep->is_in, len);
        pmabuffer = ep->pmaaddr0;
      }
      USB_WritePMA(USBx, ep->xfer_buff, pmabuffer, (uint16_t)len);
      PCD_FreeUserBuffer(USBx, ep->num, ep->is_in);
    }

    PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_VALID);
  }
  else /* OUT endpoint */
  {
    /* Multi packet transfer*/
    if (ep->xfer_len > ep->maxpacket)
    {
      len = ep->maxpacket;
      ep->xfer_len -= len;
    }
    else
    {
      len = ep->xfer_len;
      ep->xfer_len = 0U;
    }

    /* configure and validate Rx endpoint */
    if (ep->doublebuffer == 0U)
    {
      /*Set RX buffer count*/
      PCD_SET_EP_RX_CNT(USBx, ep->num, len);
    }
    else
    {
      /*Set the Double buffer counter*/
      PCD_SET_EP_DBUF_CNT(USBx, ep->num, ep->is_in, len);
    }

    PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_VALID);
  }

  return HAL_OK;
}

/**
  * @brief  USB_WritePacket : Writes a packet into the Tx FIFO associated
  *         with the EP/channel
  * @param  USBx : Selected device
  * @param  src :  pointer to source buffer
  * @param  ch_ep_num : endpoint or host channel number
  * @param  len : Number of bytes to write
  * @retval HAL status
  */
HAL_StatusTypeDef USB_WritePacket(USB_TypeDef *USBx, uint8_t *src, uint8_t ch_ep_num, uint16_t len)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(src);
  UNUSED(ch_ep_num);
  UNUSED(len);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return HAL_OK;
}

/**
  * @brief  USB_ReadPacket : read a packet from the Tx FIFO associated
  *         with the EP/channel
  * @param  USBx : Selected device
  * @param  dest : destination pointer
  * @param  len : Number of bytes to read
  * @retval pointer to destination buffer
  */
void *USB_ReadPacket(USB_TypeDef *USBx, uint8_t *dest, uint16_t len)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(dest);
  UNUSED(len);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return ((void *)NULL);
}

/**
  * @brief  USB_EPSetStall : set a stall condition over an EP
  * @param  USBx : Selected device
  * @param  ep: pointer to endpoint structure
  * @retval HAL status
  */
HAL_StatusTypeDef USB_EPSetStall(USB_TypeDef *USBx, USB_EPTypeDef *ep)
{
  if (ep->is_in != 0U)
  {
    PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_STALL);
  }
  else
  {
    PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_STALL);
  }

  return HAL_OK;
}

/**
  * @brief  USB_EPClearStall : Clear a stall condition over an EP
  * @param  USBx : Selected device
  * @param  ep: pointer to endpoint structure
  * @retval HAL status
  */
HAL_StatusTypeDef USB_EPClearStall(USB_TypeDef *USBx, USB_EPTypeDef *ep)
{
  if (ep->doublebuffer == 0U)
  {
    if (ep->is_in != 0U)
    {
      PCD_CLEAR_TX_DTOG(USBx, ep->num);

      if (ep->type != EP_TYPE_ISOC)
      {
        /* Configure NAK status for the Endpoint */
        PCD_SET_EP_TX_STATUS(USBx, ep->num, USB_EP_TX_NAK);
      }
    }
    else
    {
      PCD_CLEAR_RX_DTOG(USBx, ep->num);

      /* Configure VALID status for the Endpoint*/
      PCD_SET_EP_RX_STATUS(USBx, ep->num, USB_EP_RX_VALID);
    }
  }

  return HAL_OK;
}

/**
  * @brief  USB_StopDevice : Stop the usb device mode
  * @param  USBx : Selected device
  * @retval HAL status
  */
HAL_StatusTypeDef USB_StopDevice(USB_TypeDef *USBx)
{
  /* disable all interrupts and force USB reset */
  USBx->CNTR = USB_CNTR_FRES;

  /* clear interrupt status register */
  USBx->ISTR = 0;

  /* switch-off device */
  USBx->CNTR = (USB_CNTR_FRES | USB_CNTR_PDWN);

  return HAL_OK;
}

/**
  * @brief  USB_SetDevAddress : Stop the usb device mode
  * @param  USBx : Selected device
  * @param  address : new device address to be assigned
  *          This parameter can be a value from 0 to 255
  * @retval HAL status
  */
HAL_StatusTypeDef  USB_SetDevAddress(USB_TypeDef *USBx, uint8_t address)
{
  if (address == 0U)
  {
    /* set device address and enable function */
    USBx->DADDR = USB_DADDR_EF;
  }

  return HAL_OK;
}

/**
  * @brief  USB_DevConnect : Connect the USB device by enabling the pull-up/pull-down
  * @param  USBx : Selected device
  * @retval HAL status
  */
HAL_StatusTypeDef  USB_DevConnect(USB_TypeDef *USBx)
{
  /* Enabling DP Pull-UP bit to Connect internal PU resistor on USB DP line */
  USBx->BCDR |= USB_BCDR_DPPU;

  return HAL_OK;
}

/**
  * @brief  USB_DevDisconnect : Disconnect the USB device by disabling the pull-up/pull-down
  * @param  USBx : Selected device
  * @retval HAL status
  */
HAL_StatusTypeDef  USB_DevDisconnect(USB_TypeDef *USBx)
{
  /* Disable DP Pull-Up bit to disconnect the Internal PU resistor on USB DP line */
  USBx->BCDR &= (uint16_t)(~(USB_BCDR_DPPU));

  return HAL_OK;
}

/**
  * @brief  USB_ReadInterrupts: return the global USB interrupt status
  * @param  USBx : Selected device
  * @retval HAL status
  */
uint32_t  USB_ReadInterrupts(USB_TypeDef *USBx)
{
  uint32_t tmpreg;

  tmpreg = USBx->ISTR;
  return tmpreg;
}

/**
  * @brief  USB_ReadDevAllOutEpInterrupt: return the USB device OUT endpoints interrupt status
  * @param  USBx : Selected device
  * @retval HAL status
  */
uint32_t USB_ReadDevAllOutEpInterrupt(USB_TypeDef *USBx)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return (0);
}

/**
  * @brief  USB_ReadDevAllInEpInterrupt: return the USB device IN endpoints interrupt status
  * @param  USBx : Selected device
  * @retval HAL status
  */
uint32_t USB_ReadDevAllInEpInterrupt(USB_TypeDef *USBx)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return (0);
}

/**
  * @brief  Returns Device OUT EP Interrupt register
  * @param  USBx : Selected device
  * @param  epnum : endpoint number
  *          This parameter can be a value from 0 to 15
  * @retval Device OUT EP Interrupt register
  */
uint32_t USB_ReadDevOutEPInterrupt(USB_TypeDef *USBx, uint8_t epnum)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(epnum);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return (0);
}

/**
  * @brief  Returns Device IN EP Interrupt register
  * @param  USBx : Selected device
  * @param  epnum : endpoint number
  *          This parameter can be a value from 0 to 15
  * @retval Device IN EP Interrupt register
  */
uint32_t USB_ReadDevInEPInterrupt(USB_TypeDef *USBx, uint8_t epnum)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(epnum);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return (0);
}

/**
  * @brief  USB_ClearInterrupts: clear a USB interrupt
  * @param  USBx  Selected device
  * @param  interrupt  interrupt flag
  * @retval None
  */
void  USB_ClearInterrupts(USB_TypeDef *USBx, uint32_t interrupt)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(interrupt);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
}

/**
  * @brief  Prepare the EP0 to start the first control setup
  * @param  USBx  Selected device
  * @param  psetup  pointer to setup packet
  * @retval HAL status
  */
HAL_StatusTypeDef USB_EP0_OutStart(USB_TypeDef *USBx, uint8_t *psetup)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(USBx);
  UNUSED(psetup);
  /* NOTE : - This function is not required by USB Device FS peripheral, it is used
              only by USB OTG FS peripheral.
            - This function is added to ensure compatibility across platforms.
   */
  return HAL_OK;
}

/**
  * @brief  USB_ActivateRemoteWakeup : active remote wakeup signalling
  * @param  USBx  Selected device
  * @retval HAL status
  */
HAL_StatusTypeDef USB_ActivateRemoteWakeup(USB_TypeDef *USBx)
{
  USBx->CNTR |= USB_CNTR_RESUME;

  return HAL_OK;
}

/**
  * @brief  USB_DeActivateRemoteWakeup : de-active remote wakeup signalling
  * @param  USBx  Selected device
  * @retval HAL status
  */
HAL_StatusTypeDef USB_DeActivateRemoteWakeup(USB_TypeDef *USBx)
{
  USBx->CNTR &= ~(USB_CNTR_RESUME);
  return HAL_OK;
}

/**
  * @brief Copy a buffer from user memory area to packet memory area (PMA)
  * @param   USBx USB peripheral instance register address.
  * @param   pbUsrBuf pointer to user memory area.
  * @param   wPMABufAddr address into PMA.
  * @param   wNBytes: no. of bytes to be copied.
  * @retval None
  */
void USB_WritePMA(USB_TypeDef *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n = ((uint32_t)wNBytes + 1U) >> 1;
  uint32_t BaseAddr = (uint32_t)USBx;
  uint32_t i, temp1, temp2;
  uint16_t *pdwVal;
  uint8_t *pBuf = pbUsrBuf;

  pdwVal = (uint16_t *)(BaseAddr + 0x400U + ((uint32_t)wPMABufAddr * PMA_ACCESS));

  for (i = n; i != 0U; i--)
  {
    temp1 = (uint16_t) * pBuf;
    pBuf++;
    temp2 = temp1 | ((uint16_t)((uint16_t) * pBuf << 8));
    *pdwVal = (uint16_t)temp2;
    pdwVal++;

#if PMA_ACCESS > 1U
    pdwVal++;
#endif

    pBuf++;
  }
}

/**
  * @brief Copy a buffer from user memory area to packet memory area (PMA)
  * @param   USBx: USB peripheral instance register address.
  * @param   pbUsrBuf pointer to user memory area.
  * @param   wPMABufAddr address into PMA.
  * @param   wNBytes: no. of bytes to be copied.
  * @retval None
  */
void USB_ReadPMA(USB_TypeDef *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t n = (uint32_t)wNBytes >> 1;
  uint32_t BaseAddr = (uint32_t)USBx;
  uint32_t i, temp;
  uint16_t *pdwVal;
  uint8_t *pBuf = pbUsrBuf;

  pdwVal = (uint16_t *)(BaseAddr + 0x400U + ((uint32_t)wPMABufAddr * PMA_ACCESS));

  for (i = n; i != 0U; i--)
  {
    temp = *pdwVal;
    pdwVal++;
    *pBuf = (uint8_t)((temp >> 0) & 0xFFU);
    pBuf++;
    *pBuf = (uint8_t)((temp >> 8) & 0xFFU);
    pBuf++;

#if PMA_ACCESS > 1U
    pdwVal++;
#endif
  }

  if ((wNBytes % 2U) != 0U)
  {
    temp = *pdwVal;
    *pBuf = (uint8_t)((temp >> 0) & 0xFFU);
  }
}


/**
  * @}
  */

/**
  * @}
  */
#endif /* defined (USB) */
#endif /* defined (HAL_PCD_MODULE_ENABLED) || defined (HAL_HCD_MODULE_ENABLED) */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
