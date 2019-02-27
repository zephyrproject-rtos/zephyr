/**
 ******************************************************************************
 * @file    shci.c
 * @author  MCD Application Team
 * @brief   HCI command for the system channel
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
#include "stm32_wpan_common.h"

#include "shci_tl.h"
#include "shci.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Local Functions Definition ------------------------------------------------------*/
/* Public Functions Definition ------------------------------------------------------*/

/**
 *  C2 COMMAND
 */
SHCI_CmdStatus_t SHCI_C2_FUS_Get_State( void )
{
  /**
   * Buffer is large enough to hold either a command with no parameter
   * or a command complete without payload
   */
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_CmdPacket_t * p_cmd;
  TL_EvtPacket_t * p_rsp;

  p_cmd = (TL_CmdPacket_t *)local_buffer;
  p_rsp = (TL_EvtPacket_t *)local_buffer;

  p_cmd->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_FUS_GET_STATE;

  p_cmd->cmdserial.cmd.plen = 0 ;

  shci_send( p_cmd, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_BLE_Init( SHCI_C2_Ble_Init_Cmd_Packet_t *pCmdPacket )
{
  /**
   * Buffer is large enough to hold a command complete without payload
   */
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_EvtPacket_t * p_rsp;

  p_rsp = (TL_EvtPacket_t *)local_buffer;

 ((TL_CmdPacket_t *)pCmdPacket)->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_BLE_INIT;

 ((TL_CmdPacket_t *)pCmdPacket)->cmdserial.cmd.plen = sizeof( SHCI_C2_Ble_Init_Cmd_Param_t ) ;

  shci_send( (TL_CmdPacket_t *)pCmdPacket, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_THREAD_Init( void )
{
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_CmdPacket_t * p_cmd;
  TL_EvtPacket_t * p_rsp;

  p_cmd = (TL_CmdPacket_t *)local_buffer;
  p_rsp = (TL_EvtPacket_t *)local_buffer;

  p_cmd->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_THREAD_INIT;

  p_cmd->cmdserial.cmd.plen = 0 ;

  shci_send( p_cmd, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_DEBUG_Init( SHCI_C2_DEBUG_Init_Cmd_Packet_t *pCmdPacket  )
{
  /**
   * Buffer is large enough to hold a command complete without payload
   */
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_EvtPacket_t * p_rsp;

  p_rsp = (TL_EvtPacket_t *)local_buffer;

  ((TL_CmdPacket_t *)pCmdPacket)->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_DEBUG_INIT;
  ((TL_CmdPacket_t *)pCmdPacket)->cmdserial.cmd.plen = sizeof( SHCI_C2_DEBUG_Init_Cmd_Packet_t ) ;

  shci_send( (TL_CmdPacket_t *)pCmdPacket, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_FLASH_Erase_Activity( SHCI_C2_FLASH_Erase_Activity_Cmd_Packet_t *pCmdPacket )
{
  /**
   * Buffer is large enough to hold a command complete without payload
   */
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_EvtPacket_t * p_rsp;

  p_rsp = (TL_EvtPacket_t *)local_buffer;

 ((TL_CmdPacket_t *)pCmdPacket)->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_FLASH_ERASE_ACTIVITY;

 ((TL_CmdPacket_t *)pCmdPacket)->cmdserial.cmd.plen = sizeof( SHCI_C2_FLASH_Erase_Activity_Cmd_Packet_t ) ;

  shci_send( (TL_CmdPacket_t *)pCmdPacket, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_CONCURRENT_SetMode( SHCI_C2_CONCURRENT_Mode_Param_t Mode )
{
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_CmdPacket_t * p_cmd;
  TL_EvtPacket_t * p_rsp;

  p_cmd = (TL_CmdPacket_t *)local_buffer;
  p_rsp = (TL_EvtPacket_t *)local_buffer;

  p_cmd->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_CONCURRENT_SET_MODE;
  p_cmd->cmdserial.cmd.plen = 1;
  p_cmd->cmdserial.cmd.payload[0] = Mode;

  shci_send( p_cmd, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_FLASH_StoreData( SHCI_C2_FLASH_Ip_t Ip )
{
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_CmdPacket_t * p_cmd;
  TL_EvtPacket_t * p_rsp;

  p_cmd = (TL_CmdPacket_t *)local_buffer;
  p_rsp = (TL_EvtPacket_t *)local_buffer;

  p_cmd->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_FLASH_STORE_DATA;
  p_cmd->cmdserial.cmd.plen = 1;
  p_cmd->cmdserial.cmd.payload[0] = Ip;

  shci_send( p_cmd, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_FLASH_EraseData( SHCI_C2_FLASH_Ip_t Ip )
{
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_CmdPacket_t * p_cmd;
  TL_EvtPacket_t * p_rsp;

  p_cmd = (TL_CmdPacket_t *)local_buffer;
  p_rsp = (TL_EvtPacket_t *)local_buffer;

  p_cmd->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_FLASH_ERASE_DATA;
  p_cmd->cmdserial.cmd.plen = 1;
  p_cmd->cmdserial.cmd.payload[0] = Ip;

  shci_send( p_cmd, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_RADIO_AllowLowPower( SHCI_C2_FLASH_Ip_t Ip,uint8_t  FlagRadioLowPowerOn)
{
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_CmdPacket_t * p_cmd;
  TL_EvtPacket_t * p_rsp;

  p_cmd = (TL_CmdPacket_t *)local_buffer;
  p_rsp = (TL_EvtPacket_t *)local_buffer;

  p_cmd->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_RADIO_ALLOW_LOW_POWER;
  p_cmd->cmdserial.cmd.plen = 2;
  p_cmd->cmdserial.cmd.payload[0] = Ip;
  p_cmd->cmdserial.cmd.payload[1] = FlagRadioLowPowerOn;
  shci_send( p_cmd, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}

SHCI_CmdStatus_t SHCI_C2_MAC_802_15_4_Init( void )
{
  uint8_t local_buffer[TL_BLEEVT_CS_BUFFER_SIZE];
  TL_CmdPacket_t * p_cmd;
  TL_EvtPacket_t * p_rsp;

  p_cmd = (TL_CmdPacket_t *)local_buffer;
  p_rsp = (TL_EvtPacket_t *)local_buffer;

  p_cmd->cmdserial.cmd.cmdcode = SHCI_OPCODE_C2_MAC_802_15_4_INIT;

  p_cmd->cmdserial.cmd.plen = 0 ;

  shci_send( p_cmd, p_rsp );

  return (SHCI_CmdStatus_t)(((TL_CcEvt_t*)(p_rsp->evtserial.evt.payload))->payload[0]);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
