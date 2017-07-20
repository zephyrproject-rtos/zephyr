/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* This file is the interface file for the FSCI module
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _FSCI_INTERFACE_H_
#define _FSCI_INTERFACE_H_


/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"
#include "SerialManager.h"


/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */

/*
 * FSCI Configuration
 */
#if !defined(gFsciIncluded_c)
#define gFsciIncluded_c           0 /* Enable/Disable FSCI module */
#endif

#ifndef gFsciMaxOpGroups_c
#define gFsciMaxOpGroups_c       8
#endif

#ifndef gFsciMaxInterfaces_c
#define gFsciMaxInterfaces_c      1
#endif

#ifndef gFsciMaxVirtualInterfaces_c
#define gFsciMaxVirtualInterfaces_c      0
#endif

#ifndef gFsciMaxPayloadLen_c
#define gFsciMaxPayloadLen_c    245 /* bytes */
#endif

#ifndef gFsciLenHas2Bytes_c
#define gFsciLenHas2Bytes_c       0 /* boolean */
#endif

#ifndef gFsciUseEscapeSeq_c
#define gFsciUseEscapeSeq_c       0 /* boolean */
#endif

#ifndef gFsciUseFmtLog_c
#define gFsciUseFmtLog_c          0 /* boolean */
#endif

#ifndef gFsciUseFileDataLog_c
#define gFsciUseFileDataLog_c     0 /* boolean */
#endif

#ifndef gFsciTimestampSize_c
#define gFsciTimestampSize_c      0 /* bytes */
#endif

#ifndef gFsciLoggingInterface_c
#define gFsciLoggingInterface_c   1 /* [0..gFsciMaxInterfaces_c) */
#endif

#ifndef gFsciHostSupport_c
#define gFsciHostSupport_c        0 /* boolean */
#endif

#ifndef gFsciHostSyncUseEvent_c
#define gFsciHostSyncUseEvent_c   0 /* boolean */
#endif

#ifndef gFsciTxAck_c
#define gFsciTxAck_c              0 /* boolean */
#endif

#ifndef gFsciRxAck_c
#define gFsciRxAck_c              0 /* boolean */
#endif

#ifndef gFsciRxTimeout_c
#define gFsciRxTimeout_c          1 /* boolean */
#endif

#define mFsciInvalidInterface_c   (0xFF)

/* Used for maintaining backward compatibillity */
#define gFSCI_McpsSapId_c  1
#define gFSCI_MlmeSapId_c  2
#define gFSCI_AspSapId_c   3
#define gFSCI_NldeSapId_c  4
#define gFSCI_NlmeSapId_c  5
#define gFSCI_AspdeSapId_c 6
#define gFSCI_AfdeSapId_c  7
#define gFSCI_ApsmeSapId_c 8
#define gFSCI_ZdpSapId_c   9

/*
 * The Test Tool uses an Opcode Group to specify which SAP Handler a packet
 * in intended for or received from.
 */
#define gFSCI_LoggingOpcodeGroup_c       0xB0    /* FSCI data logging utillity    */
#define gFSCI_ReqOpcodeGroup_c           0xA3    /* FSCI utility Requests         */
#define gFSCI_CnfOpcodeGroup_c           0xA4    /* FSCI utility Confirmations/Indications    */
#define gFSCI_ReservedOpGroup_c          0x52

/* Aditional bytes added by FSCI to a packet */
#if gFsciMaxVirtualInterfaces_c
#define gFsci_TailBytes_c (2)
#else
#define gFsci_TailBytes_c (1)
#endif

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
typedef uint8_t clientPacketStatus_t;
typedef uint8_t opGroup_t;
typedef uint8_t opCode_t;
#if gFsciLenHas2Bytes_c
typedef uint16_t fsciLen_t;
#else
typedef uint8_t fsciLen_t;
#endif

/* FSCI status codes */
typedef enum{
    gFsciSuccess_c                 = 0x00,
    gFsciSAPHook_c                 = 0xEF,
    gFsciSAPDisabled_c             = 0xF0,
    gFsciSAPInfoNotFound_c         = 0xF1,
    gFsciUnknownPIB_c              = 0xF2,
    gFsciAppMsgTooBig_c            = 0xF3,
    gFsciOutOfMessages_c           = 0xF4,
    gFsciEndPointTableIsFull_c     = 0xF5,
    gFsciEndPointNotFound_c        = 0xF6,
    gFsciUnknownOpcodeGroup_c      = 0xF7,
    gFsciOpcodeGroupIsDisabled_c   = 0xF8,
    gFsciDebugPrintFailed_c        = 0xF9,
    gFsciReadOnly_c                = 0xFA,
    gFsciUnknownIBIdentifier_c     = 0xFB,
    gFsciRequestIsDisabled_c       = 0xFC,
    gFsciUnknownOpcode_c           = 0xFD,
    gFsciTooBig_c                  = 0xFE,
    gFsciError_c                   = 0xFF    /* General catchall error. */
} gFsciStatus_t;

/* Message Handler Function type definition */
typedef void (*pfMsgHandler_t)(void* pData, void* param, uint32_t fsciInterface);
typedef gFsciStatus_t (*pfMonitor_t) (opGroup_t opGroup, void *pData, void* param, uint32_t fsciInterface);

/* defines the Operation Group table entry */
typedef struct gFsciOpGroup_tag
{
    pfMsgHandler_t pfOpGroupHandler;
    void*          param;
    opGroup_t      opGroup;
    uint8_t        mode;
    uint8_t        fsciInterfaceId;
} gFsciOpGroup_t;

/* Format of packets exchanged between the external client and Fsci. */
typedef PACKED_STRUCT clientPacketHdr_tag
{
    uint8_t    startMarker;
    opGroup_t  opGroup;
    opCode_t   opCode;
    fsciLen_t  len;      /* Actual length of payload[] */
} clientPacketHdr_t;


/* The terminal checksum is actually stored at payload[len]. The aditional 
   gFsci_TailBytes_c bytes insures that there is always space for it, even 
   if the payload is full. */
typedef PACKED_STRUCT clientPacketStructured_tag
{
    clientPacketHdr_t header;
    uint8_t payload[gFsciMaxPayloadLen_c + gFsci_TailBytes_c];
} clientPacketStructured_t;

/* defines the working packet type */
typedef PACKED_UNION clientPacket_tag
{
    /* The entire packet as unformatted data. */
    uint8_t raw[sizeof(clientPacketStructured_t)];
    /* The packet as header + payload. */
    clientPacketStructured_t structured;
    /* A minimal packet with only a status value as a payload. */
    PACKED_STRUCT
    {                              /* The packet as header + payload. */
        clientPacketHdr_t header;
        clientPacketStatus_t status;
    } headerAndStatus;
} clientPacket_t;

/* defines the FSCI OpGroup operating mode */
typedef enum{
    gFsciDisableMode_c = 0,
    gFsciHookMode_c    = 1,
    gFsciMonitorMode_c = 2,
    gFsciInvalidMode = 0xFF
} gFsciMode_t;

/* FSCI Serial Interface initialization structure */
typedef struct{
    uint32_t              baudrate;
    serialInterfaceType_t interfaceType;
    uint8_t               interfaceChannel;
    uint8_t               virtualInterface;
} gFsciSerialConfig_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
extern uint8_t gFsciTxBlocking;
extern bool_t (*pfFSCI_OtaSupportCalback)(clientPacket_t* pPacket);

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
void FSCI_Init(void* argument);
gFsciStatus_t FSCI_RegisterOpGroup (opGroup_t opGroup, gFsciMode_t mode,
                                    pfMsgHandler_t pfHandler,
                                    void* param,
                                    uint32_t fsciInterface);

/* Monitoring SAPs */
gFsciStatus_t FSCI_Monitor (opGroup_t opGroup, void *pData, void* param, uint32_t fsciInterface);

gFsciStatus_t FSCI_ProcessRxPkt (clientPacket_t* pPacket, uint32_t fsciInterface);
gFsciStatus_t FSCI_CallRegisteredFunc (opGroup_t opGroup, void *pData, uint32_t fsciInterface);

void FSCI_transmitFormatedPacket( void *pPacket, uint32_t fsciInterface );
void FSCI_transmitPayload(uint8_t OG, uint8_t OC, void * pMsg, uint16_t msgLen, uint32_t fsciInterface);
void FSCI_Error(uint8_t errorCode, uint32_t fsciInterface);

uint8_t* FSCI_GetFormattedPacket(uint8_t OG, uint8_t OC, void *pMsg, uint16_t msgLen, uint16_t *pOutLen);

#if gFsciTxAck_c
void FSCI_Ack(uint8_t checksum, uint32_t fsciInterface);
#endif

#if gFsciHostSupport_c
gFsciStatus_t FSCI_ResetReq(uint32_t fsciInterface);
#endif

/* Data logging */
void FSCI_Print(uint8_t readyToSend, void *pSrc, fsciLen_t len);

#if gFsciUseFmtLog_c
void FSCI_LogFormatedText (const char *fmt, ...);
#endif

#if gFsciUseFileDataLog_c
void FSCI_LogToFile (char *fileName, uint8_t *pData, uint16_t dataSize, uint8_t mode);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FSCI_INTERFACE_H_ */