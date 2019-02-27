/**
 ******************************************************************************
 * @file    tl.h
 * @author  MCD Application Team
 * @brief   Header for tl module
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
#ifndef __TL_H
#define __TL_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#include "stm32_wpan_common.h"

/* Exported defines -----------------------------------------------------------*/
#define TL_BLECMD_PKT_TYPE                                              ( 0x01 )
#define TL_ACL_DATA_PKT_TYPE                                            ( 0x02 )
#define TL_BLEEVT_PKT_TYPE                                              ( 0x04 )
#define TL_OTCMD_PKT_TYPE                                               ( 0x08 )
#define TL_OTRSP_PKT_TYPE                                               ( 0x09 )
#define TL_CLICMD_PKT_TYPE                                              ( 0x0A )
#define TL_OTNOT_PKT_TYPE                                               ( 0x0C )
#define TL_OTACK_PKT_TYPE                                               ( 0x0D )
#define TL_CLINOT_PKT_TYPE                                              ( 0x0E )
#define TL_CLIACK_PKT_TYPE                                              ( 0x0F )
#define TL_SYSCMD_PKT_TYPE  	                                        ( 0x10 )
#define TL_SYSRSP_PKT_TYPE  	                                        ( 0x11 )
#define TL_SYSEVT_PKT_TYPE  	                                        ( 0x12 )
#define TL_LOCCMD_PKT_TYPE  	                                        ( 0x20 )
#define TL_LOCRSP_PKT_TYPE  	                                        ( 0x21 )
#define TL_TRACES_APP_PKT_TYPE                                          ( 0x40 )
#define TL_TRACES_WL_PKT_TYPE                                           ( 0x41 )

#define TL_CMD_HDR_SIZE                                                      (4)
#define TL_EVT_HDR_SIZE                                                      (3)
#define TL_EVT_CS_PAYLOAD_SIZE                                               (4)

#define TL_BLEEVT_CC_OPCODE                                               (0x0E)
#define TL_BLEEVT_CS_OPCODE                                               (0x0F)

#define TL_BLEEVT_CS_PACKET_SIZE        (TL_EVT_HDR_SIZE + sizeof(TL_CsEvt_t))
#define TL_BLEEVT_CS_BUFFER_SIZE        (sizeof(TL_PacketHeader_t) + TL_BLEEVT_CS_PACKET_SIZE)

/* Exported types ------------------------------------------------------------*/
/**< Packet header */
typedef PACKED_STRUCT
{
  uint32_t *next;
  uint32_t *prev;
} TL_PacketHeader_t;

/*******************************************************************************
 * Event type
 */

/**
 * This the payload of TL_Evt_t for a command status event
 */
typedef PACKED_STRUCT
{
  uint8_t   status;
  uint8_t   numcmd;
  uint16_t  cmdcode;
} TL_CsEvt_t;

/**
 * This the payload of TL_Evt_t for a command complete event
 */
typedef PACKED_STRUCT
{
  uint8_t   numcmd;
  uint16_t  cmdcode;
  uint8_t   payload[1];
} TL_CcEvt_t;

/**
 * This the payload of TL_Evt_t for an asynchronous event
 */
typedef PACKED_STRUCT
{
  uint16_t  subevtcode;
  uint8_t   payload[1];
} TL_AsynchEvt_t;

typedef PACKED_STRUCT
{
  uint8_t   evtcode;
  uint8_t   plen;
  uint8_t   payload[1];
} TL_Evt_t;

typedef PACKED_STRUCT
{
  uint8_t   type;
  TL_Evt_t  evt;
} TL_EvtSerial_t;

/**
 * This format shall be used for all events (asynchronous and command response) reported
 * by the CPU2 except for the command response of a system command where the header is not there
 * and the format to be used shall be TL_EvtSerial_t.
 * Note: Be careful that the asynchronous events reported by the CPU2 on the system channel do
 * include the header and shall use TL_EvtPacket_t format. Only the command response format on the
 * system channel is different.
 */
typedef PACKED_STRUCT
{
  TL_PacketHeader_t  header;
  TL_EvtSerial_t     evtserial;
} TL_EvtPacket_t;

/*****************************************************************************************
 * Command type
 */

typedef PACKED_STRUCT
{
  uint16_t   cmdcode;
  uint8_t   plen;
  uint8_t   payload[255];
} TL_Cmd_t;

typedef PACKED_STRUCT
{
  uint8_t   type;
  TL_Cmd_t  cmd;
} TL_CmdSerial_t;

typedef PACKED_STRUCT
{
  TL_PacketHeader_t  header;
  TL_CmdSerial_t     cmdserial;
} TL_CmdPacket_t;

/*****************************************************************************************
 * HCI ACL DATA type
 */
typedef PACKED_STRUCT
{
  uint8_t   type;
  uint16_t  handle;
  uint16_t  length;
  uint8_t   acl_data[1];
} TL_AclDataSerial_t;

typedef PACKED_STRUCT
{
  TL_PacketHeader_t  header;
  TL_AclDataSerial_t   AclDataSerial;
} TL_AclDataPacket_t;

typedef struct
{
  uint8_t  *p_BleSpareEvtBuffer;
  uint8_t  *p_SystemSpareEvtBuffer;
  uint8_t  *p_AsynchEvtPool;
  uint32_t AsynchEvtPoolSize;
  uint8_t  *p_TracesEvtPool;
  uint32_t TracesEvtPoolSize;
} TL_MM_Config_t;

typedef struct
{
  uint8_t *p_ThreadOtCmdRspBuffer;
  uint8_t *p_ThreadCliRspBuffer;
  uint8_t *p_ThreadNotAckBuffer;
} TL_TH_Config_t;

typedef struct
{
  uint8_t *p_Mac_802_15_4_CmdRspBuffer;
  uint8_t *p_Mac_802_15_4_NotAckBuffer;
} TL_MAC_802_15_4_Config_t;

/**
 * @brief Contain the BLE HCI Init Configuration
 * @{
 */
typedef struct
{
  void (* IoBusEvtCallBack) ( TL_EvtPacket_t *phcievt );
  void (* IoBusAclDataTxAck) ( void );
  uint8_t *p_cmdbuffer;
  uint8_t *p_AclDataBuffer;
} TL_BLE_InitConf_t;

/**
 * @brief Contain the SYSTEM HCI Init Configuration
 * @{
 */
typedef struct
{
  void (* IoBusCallBackCmdEvt) (TL_EvtPacket_t *phcievt);
  void (* IoBusCallBackUserEvt) (TL_EvtPacket_t *phcievt);
  uint8_t *p_cmdbuffer;
} TL_SYS_InitConf_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/******************************************************************************
 * GENERAL
 ******************************************************************************/
void TL_Enable( void );
void TL_Init( void );

/******************************************************************************
 * BLE
 ******************************************************************************/
int32_t TL_BLE_Init( void* pConf );
int32_t TL_BLE_SendCmd( uint8_t* buffer, uint16_t size );
int32_t TL_BLE_SendAclData( uint8_t* buffer, uint16_t size );

/******************************************************************************
 * SYSTEM
 ******************************************************************************/
int32_t TL_SYS_Init( void* pConf  );
int32_t TL_SYS_SendCmd( uint8_t* buffer, uint16_t size );

/******************************************************************************
 * THREAD
 ******************************************************************************/
void TL_THREAD_Init( TL_TH_Config_t *p_Config );
void TL_OT_SendCmd( void );
void TL_CLI_SendCmd( void );
void TL_OT_CmdEvtReceived( TL_EvtPacket_t * Otbuffer );
void TL_THREAD_NotReceived( TL_EvtPacket_t * Notbuffer );
void TL_THREAD_SendAck ( void );
void TL_THREAD_CliSendAck ( void );
void TL_THREAD_CliNotReceived( TL_EvtPacket_t * Notbuffer );

/******************************************************************************
 * MEMORY MANAGER
 ******************************************************************************/
void TL_MM_Init( TL_MM_Config_t *p_Config );
void TL_MM_EvtDone( TL_EvtPacket_t * hcievt );

/******************************************************************************
 * TRACES
 ******************************************************************************/
void TL_TRACES_Init( void );
void TL_TRACES_EvtReceived( TL_EvtPacket_t * hcievt );

/******************************************************************************
 * MAC 802.15.4
 ******************************************************************************/
void TL_MAC_802_15_4_Init( TL_MAC_802_15_4_Config_t *p_Config );
void TL_MAC_802_15_4_SendCmd( void );
void TL_MAC_802_15_4_CmdEvtReceived( TL_EvtPacket_t * Otbuffer );
void TL_MAC_802_15_4_NotReceived( TL_EvtPacket_t * Notbuffer );
void TL_MAC_802_15_4_SendAck ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*__TL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
