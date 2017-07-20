/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* This is the private header file for the FSCI communication module.
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


#ifndef _FSCI_COMMUNICATION_H_
#define _FSCI_COMMUNICATION_H_


/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"
#include "SerialManager.h"
#include "TimersManager.h"
#include "FsciInterface.h"

#include "fsl_os_abstraction.h"

#ifndef gFsciRxAckTimeoutUseTmr_c
#define gFsciRxAckTimeoutUseTmr_c (USE_RTOS)
#endif

#ifndef mFsciRxTimeoutUsePolling_c
#define mFsciRxTimeoutUsePolling_c 0
#endif

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
typedef enum {
  PACKET_IS_VALID,
  PACKET_IS_TO_SHORT,
  FRAMING_ERROR,
  INTERNAL_ERROR
} fsci_packetStatus_t;

typedef struct fsciComm_tag{
    clientPacket_t    *pPacketFromClient;
    clientPacketHdr_t  pktHeader;
    uint16_t           bytesReceived;
#if gFsciHostSupport_c
    osaMutexId_t       syncHostMutexId;
#endif
#if gFsciRxAck_c
    osaMutexId_t       syncTxRxAckMutexId;
#if gFsciRxAckTimeoutUseTmr_c
    tmrTimerID_t       ackWaitTmr;
#endif
    uint8_t            txRetryCnt;
    volatile bool_t    ackReceived;
    volatile bool_t    ackWaitOngoing;
#endif
#if gFsciRxTimeout_c
#if mFsciRxTimeoutUsePolling_c
    uint64_t           lastRxByteTs;
#else
    tmrTimerID_t       rxRestartTmr;
#endif
    bool_t             rxOngoing;
    volatile bool_t    rxTmrExpired;
#endif    
}fsciComm_t;

typedef enum{
    gFSCIHost_RspReady_c     = (1<<0),
}fsciHostEventType_t;

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */
extern uint8_t gFsciSerialInterfaces[];

/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */
#define gFSCI_StartMarker_c     0x02
#define gFSCI_EndMarker_c       0x03
#define gFSCI_EscapeChar_c      0x7F

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
void FSCI_commInit( void* initStruct );
void FSCI_receivePacket( void* param );
uint32_t FSCI_GetVirtualInterface(uint32_t fsciInterface);
uint32_t FSCI_GetFsciInterface(uint32_t hwInterface, uint32_t virtualInterface);
uint32_t FSCI_encodeEscapeSeq( uint8_t* pDataIn, uint32_t len, uint8_t* pDataOut );
void FSCI_decodeEscapeSeq( uint8_t* pData, uint32_t len );
uint8_t FSCI_computeChecksum( void *pBuffer, uint16_t size );

#if gFsciHostSupport_c
void FSCI_HostSyncLock(uint32_t fsciInstance, opGroup_t OG, opCode_t OC);
void FSCI_HostSyncUnlock(uint32_t fsciInstance);
#endif

#endif /* _FSCI_COMMUNICATION_H_ */