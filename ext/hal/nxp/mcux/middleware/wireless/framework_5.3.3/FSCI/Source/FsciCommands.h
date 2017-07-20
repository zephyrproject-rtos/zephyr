/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* This is the private header file for the FSCI commands.
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

#ifndef _FSCI_COMMANDS_H_
#define _FSCI_COMMANDS_H_


/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"


/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */
/* Define the message types that Fsci recognizes and/or generates. */
enum {
    mFsciMsgModeSelectReq_c                 = 0x00, /* Fsci-ModeSelect.Request.              */
    mFsciMsgGetModeReq_c                    = 0x02, /* Fsci-GetMode.Request.                 */

    mFsciMsgAFResetReq_c                    = 0x05, /* Fsci-AFReset.Request.                 */
    mFsciMsgAPSResetReq_c                   = 0x06, /* Fsci-APSReset.Request.                */
    mFsciMsgAPSReadyReq_c                   = 0x07, /* Fsci-SetAPSReady.Request.             */
    mFsciMsgResetCPUReq_c                   = 0x08, /* Fsci-CPU_Reset.Request.               */
    mFsciMsgDeregisterEndPoint_c            = 0x0A, /* Fsci-DeregisterEndPoint.Request       */
    mFsciMsgRegisterEndPoint_c              = 0x0B, /* Fsci-RegisterEndPoint.Request         */
    mFsciMsgGetNumberOfEndPoints_c          = 0x0C, /* Fsci-GetNumberOfEndPoints.Request     */
    mFsciMsgGetEndPointDescription_c        = 0x0D, /* Fsci-GetEndPointDescription.Request   */
    mFsciMsgGetEndPointIdList_c             = 0x0E, /* Fsci-GetEndPointIdList.Request        */
    mFsciMsgSetEpMaxWindowSize_c            = 0x0F, /* Fsci-SetEpMaxWindowSize.Request       */
    mFsciMsgGetICanHearYouList_c            = 0x10, /* Fsci-GetICanHearYouList.Request       */
    mFsciMsgSetICanHearYouList_c            = 0x11, /* Fsci-SetICanHearYouList.Request       */
    mFsciMsgGetChannelReq_c                 = 0x12, /* Fsci-GetChannel.Request               */
    mFsciMsgSetChannelReq_c                 = 0x13, /* Fsci-SetChannel.Request               */
    mFsciMsgGetPanIDReq_c                   = 0x14, /* Fsci-GetPanID.Request                 */
    mFsciMsgSetPanIDReq_c                   = 0x15, /* Fsci-SetPanID.Request                 */
    mFsciMsgGetPermissionsTable_c           = 0x16, /* Fsci-GetPermissionsTable.Request      */
    mFsciMsgSetPermissionsTable_c           = 0x17, /* Fsci-SetPermissionsTable.Request      */
    mFsciMsgRemoveFromPermissionsTable_c    = 0x18, /* Fsci-RemoveFromPermissionsTable.Request    */
    mFsciMsgAddDeviceToPermissionsTable_c   = 0x19, /* Fsci-AddDeviceToPermissionsTable.Request   */
    mFsciMsgApsmeGetIBReq_c                 = 0x20, /* Fsci-GetIB.Request, aka APSME-GET.Request  */
    mFsciMsgApsmeSetIBReq_c                 = 0x21, /* Fsci-SetIB.Request, aka APSME-SET.Request  */
    mFsciMsgNlmeGetIBReq_c                  = 0x22, /* Fsci-GetIB.Request, aka NLME-GET.Request   */
    mFsciMsgNlmeSetIBReq_c                  = 0x23, /* Fsci-SetIB.Request, aka NLME-SET.Request   */
    mFsciMsgGetNumOfMsgsReq_c               = 0x24, /* Fsci-Get number of msgs available request  */
    mFsciMsgFreeDiscoveryTablesReq_c        = 0x25, /* Fsci-FreeDiscoveryTables.Request           */
    mFsciMsgSetJoinFilterFlagReq_c          = 0x26, /* Fsci-SetJoinFilterFlag.Request             */
    mFsciMsgGetMaxApplicationPayloadReq_c   = 0x27, /* Fsci-GetMaxApplicationPayload.Request      */

    mFsciOtaSupportSetModeReq_c             = 0x28,
    mFsciOtaSupportStartImageReq_c          = 0x29,
    mFsciOtaSupportPushImageChunkReq_c      = 0x2A,
    mFsciOtaSupportCommitImageReq_c         = 0x2B,
    mFsciOtaSupportCancelImageReq_c         = 0x2C,
    mFsciOtaSupportSetFileVerPoliciesReq_c  = 0x2D,
    mFsciOtaSupportAbortOTAUpgradeReq_c     = 0x2E,
    mFsciOtaSupportImageChunkReq_c          = 0x2F,
    mFsciOtaSupportQueryImageReq_c          = 0xC2,
    mFsciOtaSupportQueryImageRsp_c          = 0xC3,
    mFsciOtaSupportImageNotifyReq_c         = 0xC4,
    mFsciOtaSupportGetClientInfo_c          = 0xC5,            

    mFsciEnableBootloaderReq_c              = 0xCF,
    
    mFsciLowLevelMemoryWriteBlock_c         = 0x30, /* Fsci-WriteRAMMemoryBlock.Request     */
    mFsciLowLevelMemoryReadBlock_c          = 0x31, /* Fsci-ReadMemoryBlock.Request         */
    mFsciLowLevelPing_c                     = 0x38, /* Fsci-Ping.Request                    */

    mFsciMsgGetApsDeviceKeyPairSet_c        = 0x3B,  /* Fsci-GetApsDeviceKeyPairSet         */
    mFsciMsgGetApsDeviceKey_c               = 0x3C,
    mFsciMsgSetApsDeviceKey_c               = 0x3D,
    mFsciMsgSetApsDeviceKeyPairSet_c        = 0x3E,
    mFsciMsgClearApsDeviceKeyPairSet_c      = 0x3F,

    mFsciMsgAllowDeviceToSleepReq_c         = 0x70, /* Fsci-SelectWakeUpPIN.Request         */
    mFsciMsgWakeUpIndication_c              = 0x71, /* Fsci-WakeUp.Indication               */
    mFsciMsgGetWakeUpReasonReq_c            = 0x72, /*                */
#if gBeeStackIncluded_d
    mFsciMsgSetApsDeviceKeyPairSetKeyInfo   = 0x40,
    mFsciMsgSetApsOverrideApsEncryption     = 0x41,
    mFsciMsgSetPollRate                     = 0x43,
    mFsciMsgSetRejoinConfigParams           = 0x44,

    mFsciMsgSetFaMaxIncomingErrorReports_c  = 0x4A,

    mFsciMsgSetHighLowRamConcentrator       = 0x50,
    
    mFsciMsgEZModeComissioningStart                     = 0x51,  /* EZ mode, Touchlink  procedure */
    mFsciMsgZllTouchlinkCommissioningStart_c            = 0x52,
    mFsciMsgZllTouchlinkCommissioningConfigure_c        = 0x53,
    mFsciMsgZllTouchlinkGetListOfCommissionedDevices_c  = 0x54,
    mFsciMsgZllTouchlinkRemoveEntry_c                   = 0x55,
    mFsciMsgClearNeighborTableEntry_c                   = 0x56,
#endif
    mFsciGetUniqueId_c                      = 0xB0,
    mFsciGetMcuId_c                         = 0xB1,
    mFsciGetSwVersions_c                    = 0xB2,

    mFsciMsgAddToAddressMapPermanent_c      = 0xC0,
    mFsciMsgRemoveFromAddressMap_c          = 0xC1,

    mFsciMsgWriteNwkMngAddressReq_c         = 0xAD, /* Fsci-WriteNwkMngAddr.Request         */

    mFsciMsgReadExtendedAdrReq_c            = 0xD2, /* Fsci-ReadExtAddr.Request             */
    mFsciMsgReadNwkMngAddressReq_c          = 0xDA, /* Fsci-ReadNwkMngAddr.Request          */
    mFsciMsgWriteExtendedAdrReq_c           = 0xDB, /* Fsci-WriteExtAddr.Request            */
    mFsciMsgStopNwkReq_c                    = 0xDC, /* Fsci-StopNwk.Request                 */
    mFsciMsgStartNwkReq_c                   = 0xDF, /* Fsci-StartNwk.Request                */
    mFsciMsgStartNwkExReq_c                 = 0xE7, /* Fsci-StartNwkEx.Request              */
    mFsciMsgStopNwkExReq_c                  = 0xE8, /* Fsci-StopNwkEx.Request               */
    mFsciMsgRestartNwkReq_c                 = 0xE0, /* Fsci-RestartNwk.Request              */

    mFsciMsgAck_c                           = 0xFD, /* Fsci acknowledgment.                 */
    mFsciMsgError_c                         = 0xFE, /* Fsci internal error.                 */
    mFsciMsgDebugPrint_c                    = 0xFF, /* printf()-style debug message.        */
};

/* defines the Operation Code table entry */
typedef struct gFsciOpCode_tag
{
    opCode_t opCode;
    bool_t (*pfOpCodeHandle)(void* pData, uint32_t fsciInterface);
}gFsciOpCode_t;

typedef PACKED_STRUCT gFsciErrorMsg_tag{
    clientPacketHdr_t header;
    clientPacketStatus_t status;
    uint8_t checksum;
    uint8_t checksum2;
}gFsciErrorMsg_t;

typedef PACKED_STRUCT gFsciAckMsg_tag{
    clientPacketHdr_t header;
    uint8_t checksumPacketReceived;
    uint8_t checksum;
    uint8_t checksum2;
}gFsciAckMsg_t;

/*! *********************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
********************************************************************************** */
#ifndef gFSCI_IncludeMacCommands_c
#define gFSCI_IncludeMacCommands_c (0)
#endif

#ifndef gFSCI_IncludeLpmCommands_c
#define gFSCI_IncludeLpmCommands_c (0)
#endif

#ifndef gFSCI_ResetCpu_c
#define gFSCI_ResetCpu_c           !(gSerialMgrUseUSB_c || gSerialMgrUseUSB_VNIC_c)
#endif

/*! *********************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
void fsciMsgHandler(void *pData, uint32_t fsciInterface);
void FSCI_SendWakeUpIndication( void );

bool_t FSCI_MsgGetNumOfMsgsReqFunc            (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgModeSelectReqFunc              (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgGetModeReqFunc                 (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgGetNVDataSetDescReqFunc        (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgGetNVPageHdrReqFunc            (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgNVSaveReqFunc                  (void* pData, uint32_t fsciInterface);
bool_t FSCI_WriteMemoryBlock                  (void* pData, uint32_t fsciInterface);
bool_t FSCI_ReadMemoryBlock                   (void* pData, uint32_t fsciInterface);
bool_t FSCI_Ping                              (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgResetCPUReqFunc                (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgWriteExtendedAdrReqFunc        (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgReadExtendedAdrReqFunc         (void* pData, uint32_t fsciInterface);
bool_t FSCI_GetLastLqiValue                   (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgAllowDeviceToSleepReqFunc      (void* pData, uint32_t fsciInterface);
bool_t FSCI_MsgGetWakeUpReasonReqFunc         (void* pData, uint32_t fsciInterface);
bool_t FSCI_ReadUniqueId                      (void* pData, uint32_t fsciInterface);
bool_t FSCI_ReadMCUId                         (void* pData, uint32_t fsciInterface);
bool_t FSCI_ReadModVer                        (void* pData, uint32_t fsciInterface);
bool_t FSCI_OtaSupportHandlerFunc             (void* pData, uint32_t fsciInterface);
bool_t FSCI_EnableBootloaderFunc              (void* pData, uint32_t fsciInterface);

#endif /* _FSCI_COMMANDS_H_ */