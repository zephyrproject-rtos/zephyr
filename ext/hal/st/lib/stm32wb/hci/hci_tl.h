/**
 ******************************************************************************
 * @file    hci_tl.h
 * @author  MCD Application Team
 * @brief   Constants and functions for HCI layer. See Bluetooth Core
 *          v 4.0, Vol. 2, Part E.
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


#ifndef __HCI_TL_H_
#define __HCI_TL_H_

#include "stm32_wpan_common.h"
#include "tl.h"

/* Exported defines -----------------------------------------------------------*/
typedef enum
{
  HCI_TL_UserEventFlow_Disable,
  HCI_TL_UserEventFlow_Enable,
} HCI_TL_UserEventFlowStatus_t;

typedef enum
{
  HCI_TL_CmdBusy,
  HCI_TL_CmdAvailable
} HCI_TL_CmdStatus_t;

/**
 * @brief Structure used to manage the BUS IO operations.
 *        All the structure fields will point to functions defined at user level.
 * @{
 */
typedef struct
{
  int32_t (* Init)    (void* pConf); /**< Pointer to HCI TL function for the IO Bus initialization */
  int32_t (* DeInit)  (void); /**< Pointer to HCI TL function for the IO Bus de-initialization */
  int32_t (* Reset)   (void); /**< Pointer to HCI TL function for the IO Bus reset */
  int32_t (* Receive) (uint8_t*, uint16_t); /**< Pointer to HCI TL function for the IO Bus data reception */
  int32_t (* Send)    (uint8_t*, uint16_t); /**< Pointer to HCI TL function for the IO Bus data transmission */
  int32_t (* DataAck) (uint8_t*, uint16_t* len); /**< Pointer to HCI TL function for the IO Bus data ack reception */
  int32_t (* GetTick) (void); /**< Pointer to BSP function for getting the HAL time base timestamp */
} tHciIO;
/**
 * @}
 */

/**
 * @brief Contain the HCI context
 * @{
 */
typedef struct
{
  tHciIO io; /**< Manage the BUS IO operations */
  void (* UserEvtRx) (void * pData); /**< ACI events callback function pointer */
} tHciContext;

typedef struct
{
  HCI_TL_UserEventFlowStatus_t status;
  TL_EvtPacket_t *pckt;
} tHCI_UserEvtRxParam;

typedef struct
{
  uint8_t *p_cmdbuffer;
  void (* StatusNotCallBack) (HCI_TL_CmdStatus_t status);
} HCI_TL_HciInitConf_t;

/**
 * @brief  Register IO bus services.
 * @param  fops The HCI IO structure managing the IO BUS
 * @retval None
 */
void hci_register_io_bus(tHciIO* fops);

/**
 * @brief  Interrupt service routine that must be called when the BlueNRG
 *         reports a packet received or an event to the host through the
 *         BlueNRG-MS interrupt line.
 *
 * @param  pdata Packet or event pointer
 * @retval None
 */
void hci_notify_asynch_evt(void* pdata);

/**
 * @brief  This function resume the User Event Flow which has been stopped on return
 *         from UserEvtRx() when the User Event has not been processed.
 *
 * @param  None
 * @retval None
 */
void hci_resume_flow(void);


/**
 * @brief  This function is called when an ACI/HCI command is sent and the response
 *         is waited from the BLE core.
 *         The application shall implement a mechanism to not return from this function
 *         until the waited event is received.
 *         This is notified to the application with hci_cmd_resp_release().
 *         It is called from the same context the HCI command has been sent.
 *
 * @param  timeout: Waiting timeout
 * @retval None
 */
void hci_cmd_resp_wait(uint32_t timeout);

/**
 * @brief  This function is called when an ACI/HCI command is sent and the response is
 *         received from the BLE core.
 *
 * @param  flag: Release flag
 * @retval None
 */
void hci_cmd_resp_release(uint32_t flag);



/**
 * END OF SECTION - FUNCTIONS TO BE IMPLEMENTED BY THE APPLICATION
 *********************************************************************************************************************
 */


/**
 *********************************************************************************************************************
 * START OF SECTION - PROCESS TO BE CALLED BY THE SCHEDULER
 */

/**
 * @brief  This process shall be called by the scheduler each time it is requested with TL_BLE_HCI_UserEvtProcReq()
 *         This process may send an ACI/HCI command when the svc_ctl.c module is used
 *
 * @param  None
 * @retval None
 */

void hci_user_evt_proc(void);

/**
 * END OF SECTION - PROCESS TO BE CALLED BY THE SCHEDULER
 *********************************************************************************************************************
 */


/**
 *********************************************************************************************************************
 * START OF SECTION - INTERFACES USED BY THE BLE DRIVER
 */

/**
 * @brief Initialize the Host Controller Interface.
 *        This function must be called before any data can be received
 *        from BLE controller.
 *
 * @param  pData: ACI events callback function pointer
 *         This callback is triggered when an user event is received from
 *         the BLE core device.
 * @param  pConf: Configuration structure pointer
 * @retval None
 */
void hci_init(void(* UserEvtRx)(void* pData), void* pConf);

/**
 * END OF SECTION - INTERFACES USED BY THE BLE DRIVER
 *********************************************************************************************************************
 */

#endif /* __TL_BLE_HCI_H_ */
