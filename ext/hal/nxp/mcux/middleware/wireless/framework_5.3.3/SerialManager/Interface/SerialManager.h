/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* This is the header file for the Serial Manager interface.
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


#ifndef __SERIAL_MANAGER_H__
#define __SERIAL_MANAGER_H__

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"


/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */

/* The number of serial interfaces to be used.
0 - means that Serial Manager is disabled */
#ifndef gSerialManagerMaxInterfaces_c
#define gSerialManagerMaxInterfaces_c       (1) 
#endif

/* Serial Manager Task specification */
#ifndef gSerialTaskStackSize_c
#define gSerialTaskStackSize_c              (1024) /* bytes */
#endif
#ifndef gSerialTaskPriority_c
#define gSerialTaskPriority_c               (3)
#endif

#ifndef gSerialMgrRxBufSize_c
#define gSerialMgrRxBufSize_c               (32)
#endif

#ifndef gSerialMgrTxQueueSize_c
#define gSerialMgrTxQueueSize_c             (5)
#endif

/* Enables/Disables parameter checking */
#ifndef gSerialMgr_ParamValidation_d
#define gSerialMgr_ParamValidation_d        (1)
#endif

/* Defines if the Sender Task should block if the Tx queue is Full */
#ifndef gSerialMgr_BlockSenderOnQueueFull_c
#define gSerialMgr_BlockSenderOnQueueFull_c (1)
#endif

/* Prevent the MCU to enter Sleep during serial data transmission */
#ifndef gSerialMgr_DisallowMcuSleep_d
#define gSerialMgr_DisallowMcuSleep_d       (0)
#endif

/* Defines which serial interface can be used */
#ifndef gSerialMgrUseUart_c
#define gSerialMgrUseUart_c                 (1)
#endif
#ifndef gSerialMgrUseUSB_c
#define gSerialMgrUseUSB_c                  (0)
#endif
#ifndef gSerialMgrUseUSB_VNIC_c
#define gSerialMgrUseUSB_VNIC_c             (0)
#endif
#ifndef gSerialMgrUseIIC_c
#define gSerialMgrUseIIC_c                  (0)
#endif
#ifndef gSerialMgrUseSPI_c
#define gSerialMgrUseSPI_c                  (0)
#endif
#ifndef gSerialMgrUseCustomInterface_c
#define gSerialMgrUseCustomInterface_c      (0)
#endif

#if gSerialMgrUseSPI_c
#ifndef gSerialMgrUseFSCIHdr_c
#define gSerialMgrUseFSCIHdr_c              (0)
#endif
#endif

/* Defines the address of the slave when in master mode*/
#ifndef gSerialMgrIICAddress_c
#define gSerialMgrIICAddress_c              (0x76)
#endif

/* Defines the data available pin transition type when slave has data to Tx */
#ifndef gSerialMgrSlaveDapTxLogicOne_c
#define gSerialMgrSlaveDapTxLogicOne_c      (1)
#endif

/* Retrieves one byte from the Rx buffer. Returns the number of bytes retieved (1/0). */
#define Serial_GetByteFromRxBuffer(InterfaceId, pDst, readBytes) Serial_Read((InterfaceId), (pDst), 1, (readBytes))

/* Converts a 0x00-0x0F number to ascii '0'-'F' */
#define HexToAscii(hex) (uint8_t)( ((hex) & 0x0F) + ((((hex) & 0x0F) <= 9) ? '0' : ('A'-10)) )


#if (gSerialManagerMaxInterfaces_c) && (!gSerialMgrUseUart_c) && (!gSerialMgrUseUSB_c) && (!gSerialMgrUseUSB_VNIC_c) && (!gSerialMgrUseIIC_c) && (!gSerialMgrUseSPI_c) && (!gSerialMgrUseCustomInterface_c)
#warning The Serial Manager module is enabled, but all supported interfaces are disabled!
#endif


/*! *********************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
********************************************************************************** */

#define gSerialMgrInvalidIdx_c   (0xFF)

#define gSerialMgrBufferInUse_c (1<<0)
#define gSerialMgrSyncTx_c      (1<<1)

/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */

/* Define the types of serial interfaces */
typedef enum{
    gSerialMgrNone_c      = 0,
    gSerialMgrUart_c      = 1,
    gSerialMgrUSB_c       = 2,
    gSerialMgrUSB_VNIC_c  = 3,
    gSerialMgrIICMaster_c = 4,
    gSerialMgrIICSlave_c  = 5,
    gSerialMgrSPIMaster_c = 6,
    gSerialMgrSPISlave_c  = 7,
    gSerialMgrLpuart_c    = 8,
    gSerialMgrLpsci_c     = 9,
    gSerialMgrCustom_c    = 10
}serialInterfaceType_t;

/* Define if the Tx is blocking or not */
typedef enum {
    gNoBlock_d      = 0,
    gAllowToBlock_d = 1,
}serialBlock_t;

/* If you have to print a hex number you can choose between
BigEndian=1/LittleEndian=0, newline, commas or spaces (between bytes) */
#define gPrtHexNoFormat_c  (0x00)
#define gPrtHexBigEndian_c (1<<0)
#define gPrtHexNewLine_c   (1<<1)
#define gPrtHexCommas_c    (1<<2)
#define gPrtHexSpaces_c    (1<<3)

/* Serial Manager callback type */
typedef void (*pSerialCallBack_t)(void* param);

/* Supported baudrates for UART */
typedef enum{
    gUARTBaudRate1200_c   =   1200UL,
    gUARTBaudRate2400_c   =   2400UL,
    gUARTBaudRate4800_c   =   4800UL,
    gUARTBaudRate9600_c   =   9600UL,
    gUARTBaudRate19200_c  =  19200UL,
    gUARTBaudRate38400_c  =  38400UL,
    gUARTBaudRate57600_c  =  57600UL,
    gUARTBaudRate115200_c = 115200UL,
    gUARTBaudRate230400_c = 230400UL
}serialUartBaudRate_t;

/* Supported baudrates for SPI */
typedef enum{
    gSPI_BaudRate_100000_c  = 100000,
    gSPI_BaudRate_200000_c  = 200000,
    gSPI_BaudRate_400000_c  = 400000,
    gSPI_BaudRate_800000_c  = 800000,
    gSPI_BaudRate_1000000_c = 1000000,
    gSPI_BaudRate_2000000_c = 2000000,
    gSPI_BaudRate_4000000_c = 4000000,
    gSPI_BaudRate_8000000_c = 8000000
}serialSpiBaudRate_t;

/* Supported baudrates for IIC */
typedef enum{
    gIIC_BaudRate_50000_c  = 50000,
    gIIC_BaudRate_100000_c = 100000,
    gIIC_BaudRate_200000_c = 200000,
    gIIC_BaudRate_400000_c = 400000,
}serialIicBaudRate_t;

/* Serial Manager status codes */
typedef enum{
   gSerial_Success_c              = 0,
   gSerial_InvalidParameter_c     = 1,
   gSerial_InvalidInterface_c     = 2,
   gSerial_MaxInterfacesReached_c = 3,
   gSerial_InterfaceNotReady_c    = 4,
   gSerial_InterfaceInUse_c       = 5,
   gSerial_InternalError_c        = 6,
   gSerial_SemCreateError_c       = 7,
   gSerial_OutOfMemory_c          = 8,
   gSerial_OsError_c              = 9,
}serialStatus_t;


/*! *********************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
********************************************************************************** */


/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */
void SerialManager_Init( void );

serialStatus_t Serial_InitInterface (uint8_t *pInterfaceId,
                                     serialInterfaceType_t interfaceType,
                                     uint8_t instance);
serialStatus_t Serial_SetBaudRate (uint8_t InterfaceId, uint32_t baudRate);

serialStatus_t Serial_RxBufferByteCount (uint8_t InterfaceId, uint16_t *bytesCount);
serialStatus_t Serial_SetRxCallBack (uint8_t InterfaceId, pSerialCallBack_t cb, void *pRxParam);
serialStatus_t Serial_Read (uint8_t InterfaceId, uint8_t *pData, uint16_t dataSize, uint16_t *bytesRead);

serialStatus_t Serial_SyncWrite (uint8_t InterfaceId, uint8_t *pBuf, uint16_t bufLen);
serialStatus_t Serial_AsyncWrite (uint8_t InterfaceId, uint8_t *pBuf, uint16_t bufLen,
                                  pSerialCallBack_t cb, void *pTxParam);

serialStatus_t Serial_Print (uint8_t InterfaceId, char * pString, serialBlock_t allowToBlock);
serialStatus_t Serial_PrintHex (uint8_t InterfaceId, uint8_t *hex, uint8_t len, uint8_t flags);
serialStatus_t Serial_PrintDec (uint8_t InterfaceId, uint32_t nr);
uint32_t       Serial_GetInterfaceId(serialInterfaceType_t type, uint32_t channel);
serialStatus_t Serial_EnableLowPowerWakeup( serialInterfaceType_t interfaceType);
serialStatus_t Serial_DisableLowPowerWakeup( serialInterfaceType_t interfaceType);
bool_t Serial_IsWakeUpSource( serialInterfaceType_t interfaceType);

/* SerialManager API for a custom interface */
#if gSerialMgrUseCustomInterface_c
uint32_t Serial_CustomReceiveData(uint8_t InterfaceId, uint8_t *pRxData, uint32_t size);
extern uint32_t Serial_CustomSendData(uint8_t *pData, uint32_t size);
void Serial_CustomSendCompleted(uint32_t InterfaceId);
#endif

#endif /* __SERIAL_MANAGER_H__ */
