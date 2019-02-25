/**
  ******************************************************************************
  * @file    stm32wbxx_ll_usb.h
  * @author  MCD Application Team
  * @brief   Header file of USB Low Layer HAL module.
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
#ifndef STM32WBxx_LL_USB_H
#define STM32WBxx_LL_USB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal_def.h"

#if defined (USB)
/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @addtogroup USB_LL
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  USB Mode definition
  */



typedef enum
{
  USB_DEVICE_MODE  = 0
} USB_ModeTypeDef;

/**
  * @brief  USB Initialization Structure definition
  */
typedef struct
{
  uint32_t dev_endpoints;           /*!< Device Endpoints number.
                                         This parameter depends on the used USB core.
                                         This parameter must be a number between Min_Data = 1 and Max_Data = 15 */

  uint32_t speed;                   /*!< USB Core speed.
                                         This parameter can be any value of @ref USB_Core_Speed                 */

  uint32_t ep0_mps;                 /*!< Set the Endpoint 0 Max Packet size.                                    */

  uint32_t phy_itface;              /*!< Select the used PHY interface.
                                         This parameter can be any value of @ref USB_Core_PHY                   */

  uint32_t Sof_enable;              /*!< Enable or disable the output of the SOF signal.                        */

  uint32_t low_power_enable;        /*!< Enable or disable Low Power mode                                       */

  uint32_t lpm_enable;              /*!< Enable or disable Battery charging.                                    */

  uint32_t battery_charging_enable; /*!< Enable or disable Battery charging.                                    */
} USB_CfgTypeDef;

typedef struct
{
  uint8_t   num;             /*!< Endpoint number
                                  This parameter must be a number between Min_Data = 1 and Max_Data = 15    */

  uint8_t   is_in;           /*!< Endpoint direction
                                  This parameter must be a number between Min_Data = 0 and Max_Data = 1     */

  uint8_t   is_stall;        /*!< Endpoint stall condition
                                  This parameter must be a number between Min_Data = 0 and Max_Data = 1     */

  uint8_t   type;            /*!< Endpoint type
                                  This parameter can be any value of @ref USB_EP_Type                       */

  uint8_t   data_pid_start;  /*!< Initial data PID
                                  This parameter must be a number between Min_Data = 0 and Max_Data = 1     */

  uint16_t  pmaadress;       /*!< PMA Address
                                  This parameter can be any value between Min_addr = 0 and Max_addr = 1K    */

  uint16_t  pmaaddr0;        /*!< PMA Address0
                                  This parameter can be any value between Min_addr = 0 and Max_addr = 1K    */

  uint16_t  pmaaddr1;        /*!< PMA Address1
                                  This parameter can be any value between Min_addr = 0 and Max_addr = 1K    */

  uint8_t   doublebuffer;    /*!< Double buffer enable
                                  This parameter can be 0 or 1                                              */

  uint16_t  tx_fifo_num;     /*!< This parameter is not required by USB Device FS peripheral, it is used
                                  only by USB OTG FS peripheral
                                  This parameter is added to ensure compatibility across USB peripherals    */

  uint32_t  maxpacket;       /*!< Endpoint Max packet size
                                  This parameter must be a number between Min_Data = 0 and Max_Data = 64KB  */

  uint8_t   *xfer_buff;      /*!< Pointer to transfer buffer                                                */

  uint32_t  xfer_len;        /*!< Current transfer length                                                   */

  uint32_t  xfer_count;      /*!< Partial transfer length in case of multi packet transfer                  */

} USB_EPTypeDef;


/* Exported constants --------------------------------------------------------*/

/** @defgroup PCD_Exported_Constants PCD Exported Constants
  * @{
  */


/** @defgroup USB_LL_EP0_MPS USB Low Layer EP0 MPS
  * @{
  */
#define DEP0CTL_MPS_64                         0U
#define DEP0CTL_MPS_32                         1U
#define DEP0CTL_MPS_16                         2U
#define DEP0CTL_MPS_8                          3U
/**
  * @}
  */

/** @defgroup USB_LL_EP_Type USB Low Layer EP Type
  * @{
  */
#define EP_TYPE_CTRL                           0U
#define EP_TYPE_ISOC                           1U
#define EP_TYPE_BULK                           2U
#define EP_TYPE_INTR                           3U
#define EP_TYPE_MSK                            3U
/**
  * @}
  */

#define BTABLE_ADDRESS                         0x000U
#define PMA_ACCESS                             1U

#define EP_ADDR_MSK                            0x7U
/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup USB_LL_Exported_Functions USB Low Layer Exported Functions
  * @{
  */


HAL_StatusTypeDef USB_CoreInit(USB_TypeDef *USBx, USB_CfgTypeDef cfg);
HAL_StatusTypeDef USB_DevInit(USB_TypeDef *USBx, USB_CfgTypeDef cfg);
HAL_StatusTypeDef USB_EnableGlobalInt(USB_TypeDef *USBx);
HAL_StatusTypeDef USB_DisableGlobalInt(USB_TypeDef *USBx);
HAL_StatusTypeDef USB_SetCurrentMode(USB_TypeDef *USBx, USB_ModeTypeDef mode);
HAL_StatusTypeDef USB_SetDevSpeed(USB_TypeDef *USBx, uint8_t speed);
HAL_StatusTypeDef USB_FlushRxFifo(USB_TypeDef *USBx);
HAL_StatusTypeDef USB_FlushTxFifo(USB_TypeDef *USBx, uint32_t num);
HAL_StatusTypeDef USB_ActivateEndpoint(USB_TypeDef *USBx, USB_EPTypeDef *ep);
HAL_StatusTypeDef USB_DeactivateEndpoint(USB_TypeDef *USBx, USB_EPTypeDef *ep);
HAL_StatusTypeDef USB_EPStartXfer(USB_TypeDef *USBx, USB_EPTypeDef *ep);
HAL_StatusTypeDef USB_WritePacket(USB_TypeDef *USBx, uint8_t *src, uint8_t ch_ep_num, uint16_t len);
void             *USB_ReadPacket(USB_TypeDef *USBx, uint8_t *dest, uint16_t len);
HAL_StatusTypeDef USB_EPSetStall(USB_TypeDef *USBx, USB_EPTypeDef *ep);
HAL_StatusTypeDef USB_EPClearStall(USB_TypeDef *USBx, USB_EPTypeDef *ep);
HAL_StatusTypeDef USB_SetDevAddress(USB_TypeDef *USBx, uint8_t address);
HAL_StatusTypeDef USB_DevConnect(USB_TypeDef *USBx);
HAL_StatusTypeDef USB_DevDisconnect(USB_TypeDef *USBx);
HAL_StatusTypeDef USB_StopDevice(USB_TypeDef *USBx);
HAL_StatusTypeDef USB_EP0_OutStart(USB_TypeDef *USBx, uint8_t *psetup);
uint32_t          USB_ReadInterrupts(USB_TypeDef *USBx);
uint32_t          USB_ReadDevAllOutEpInterrupt(USB_TypeDef *USBx);
uint32_t          USB_ReadDevOutEPInterrupt(USB_TypeDef *USBx, uint8_t epnum);
uint32_t          USB_ReadDevAllInEpInterrupt(USB_TypeDef *USBx);
uint32_t          USB_ReadDevInEPInterrupt(USB_TypeDef *USBx, uint8_t epnum);
void              USB_ClearInterrupts(USB_TypeDef *USBx, uint32_t interrupt);

HAL_StatusTypeDef USB_ActivateRemoteWakeup(USB_TypeDef *USBx);
HAL_StatusTypeDef USB_DeActivateRemoteWakeup(USB_TypeDef *USBx);
void USB_WritePMA(USB_TypeDef  *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes);
void USB_ReadPMA(USB_TypeDef  *USBx, uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes);

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
#endif /* defined (USB) */

#ifdef __cplusplus
}
#endif


#endif /* STM32WBxx_LL_USB_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
