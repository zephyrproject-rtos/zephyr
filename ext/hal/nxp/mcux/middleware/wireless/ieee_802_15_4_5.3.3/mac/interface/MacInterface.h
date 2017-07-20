/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
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

/*! *********************************************************************************
* \defgroup MacInterface Mac Interface
*
* The MAC sublayer enables the transmission of MAC frames through the use of the physical channel. 
* Besides the data service, it offers a management interface and manages access to the physical channel and network beaconing. 
* It also controls frame validation, guarantees time slots, and handles node associations. 
* It also provides means for application-appropriate security mechanisms.
*
* The MAC sublayer provides two services: the MAC data service, and the MAC management service interfacing to the MAC sublayer management entity (MLME) service access point (SAP) (known as MLME-SAP). 
* The MAC data service enables the transmission and reception of MAC protocol data units (MPDUs) across the PHY data service.
* The MAC Layer interfaces with the Network Layer through a unified set of SAP function calls and function callbacks, specific to the MAC services.
* If the interface primitives are implemented as function calls, the Network Layer calls the exposed functions (provided by the MAC Layer) to issue commands / requests.
* If the interface primitives are implemented as function callbacks, these are implemented by the Network Layer and registered its callbacks by sending their pointers to the MAC layer through a dedicated function.
*
* @{
********************************************************************************** */

#ifndef _MAC_INTERFACE_H
#define _MAC_INTERFACE_H

#ifdef __cplusplus
    extern "C" {
#endif

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "MacTypes.h"
#include "MacMessages.h"

/************************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
************************************************************************************/

/*!
 *  The number of MAC instances used
 */
#ifndef gMacInstancesCnt_c
#define gMacInstancesCnt_c  1
#endif

/*!
 *  The stack size of the MAC Task (RTOS only)
 */
#ifndef gMacTaskStackSize_c
#define gMacTaskStackSize_c 1280
#endif

/*!
 *  The priority of the MAC task
 */
#ifndef gMacTaskPriority_c
#define gMacTaskPriority_c  1
#endif

/* 802.15.4 MAC build version  */
/*! \cond DOXY_SKIP_TAG */
#define gMacVerMajor_c   5

#ifdef gPHY_802_15_4g_d
  #define gMacVerMinor_c   1
  #define gMacVerPatch_c   0
  #define gMacBuildNo_c    30
#else
  #define gMacVerMinor_c   0
  #define gMacVerPatch_c   5
  #define gMacBuildNo_c    2
#endif

#if (gMacUsePackedStructs_d)
#define MAC_STRUCT PACKED_STRUCT
#define MAC_UNION PACKED_UNION
#else
#define MAC_STRUCT struct
#define MAC_UNION union
#endif
/*! \endcond */

/*! The Id of the MemManager Pool from which the MAC layer should allocate memory */
#ifndef gMacPoolId_d
#define gMacPoolId_d 0
#endif

/************************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
************************************************************************************/

/*!
 *  First logical channel used by the MAC layer
 */
#ifdef gPHY_802_15_4g_d
#define gMacFirstLogicalChannel_c    (gLogicalChannel0_c)
#else
#define gMacFirstLogicalChannel_c    (gLogicalChannel11_c)
#endif

/*!
 *  Last logical channel used by the MAC layer
 */
#ifdef gPHY_802_15_4g_d
#define gMacLastLogicalChannel_c     (gLogicalChannel127_c)
#else
#define gMacLastLogicalChannel_c     (gLogicalChannel26_c)
#endif

/*!
 *  The defaul channel page used
 */
#ifdef gPHY_802_15_4g_d
#define gDefaultChannelPageId_c     (gChannelPageId9_c)
#else                                   
#define gDefaultChannelPageId_c     (gChannelPageId0_c)
#endif


/*! The states of the MAC layer  */
typedef enum {
    gMacStateIdle_c     = 0x00, /*!< MAC layer is idle */
    gMacStateBusy_c     = 0x01, /*!< MAC layer is Busy */
    gMacIdleRx_c        = 0x02, /*!< MAC layer is idle, but the Rx is on */
    gMacStateNotEmpty_c = 0x03  /*!< MAC layer is idle, but the input queue is not empty */
}macState_t;

/************************************************************************************/

/*! The MCPS-DATA.request primitive is generated by a local entity when a data SPDU (MSDU) is to be transferred to a peer SSCS entity */
typedef MAC_STRUCT mcpsDataReq_tag
{
    uint16_t                srcPanId;    /*!< PAN identifier of the entity from which the MSDU is being sent (Not in Spec) */
    uint64_t                srcAddr;     /*!< The individual device address of the entity from which the MSDU is being sent. (Not in Spec) */
    addrModeType_t          srcAddrMode; /*!< The source addressing mode for this primitive. */
    addrModeType_t          dstAddrMode; /*!< The destination addressing mode for this primitive. */
    uint16_t                dstPanId;    /*!< PAN identifier of the entity to which the MSDU is being transferred. */
    uint64_t                dstAddr;     /*!< The individual device address of the entity to which the MSDU is being transferred. */
    uint8_t                 msduLength;  /*!< The number of octets contained in the MSDU to be transmitted by the MAC sublayer entity. */
    uint8_t                 msduHandle;  /*!< The handle associated with the MSDU to be transmitted by the MAC sublayer entity. */
    macTxOptions_t          txOptions;   /*!< Indicates the transmission options for this MSDU. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode;  /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource;  /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;   /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 *pMsdu;     /*!< Pointer to a set of octets forming the MSDU to be transmitted by the MAC sublayer entity.
                                             The MAC layer does not free this buffer! */
} mcpsDataReq_t;

/*!  The MCPS-DATA.confirm primitive reports the results of a request to transfer a data SPDU (MSDU) from a local SSCS entity to a single peer SSCS entity. */
typedef MAC_STRUCT mcpsDataCnf_tag
{
    uint8_t                 msduHandle; /*!< The handle associated with the MSDU being confirmed. */
    resultType_t            status;
    uint32_t                timestamp;  /*!< The time (in symbols), at which the data were transmitted. This is a 24-bit value. */
} mcpsDataCnf_t;

/*!  The MCPS-DATA.indication primitive indicates the transfer of data SPDU (MSDU) from the MAC sublayer to the local SSCS entity. */
typedef MAC_STRUCT mcpsDataInd_tag
{
    uint64_t                dstAddr;     /*!< The individual device address of the entity to which the MSDU is being transferred. */
    uint16_t                dstPanId;    /*!< PAN identifier of the entity to which the MSDU is being transferred. */
    addrModeType_t          dstAddrMode; /*!< The destination addressing mode for this primitive. */
    uint64_t                srcAddr;     /*!< The individual device address of the entity from which the MSDU was received. */
    uint16_t                srcPanId;    /*!< PAN identifier of the entity from which the MSDU was received. */
    addrModeType_t          srcAddrMode; /*!< The source addressing mode for this primitive. */
    uint8_t                 msduLength;      /*!< The number of octets contained in the MSDU being indicated by the MAC sublayer entity. */
    uint8_t                 mpduLinkQuality; /*!< The LQI value measured during the reception of the MPDU. Lower values represent lower LQI */
    uint8_t                 dsn;             /*!< Data Sequence Number of the packet */
    uint32_t                timestamp;       /*!< The time, in symbols, at which the data were received. This is a 24-bit value */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 *pMsdu;          /*!< Pointer to a set of octets forming the MSDU to be transmitted by the MAC sublayer entity.
                                                  Upper layer must NOT free this pointer! */
} mcpsDataInd_t;

/*!  The MCPS-PURGE.request primitive allows the next higher layer to purge an MSDU from the transaction queue. */
typedef MAC_STRUCT mcpsPurgeReq_tag
{
    uint8_t                 msduHandle;       /*!< The handle of the MSDU requested to be purged from the transaction queue. */
} mcpsPurgeReq_t;

/*!  The MCPS-PURGE.confirm primitive allows the MAC sublayer to notify the next higher layer of the success of its request to purge an MSDU from the transaction queue. */
typedef MAC_STRUCT mcpsPurgeCnf_tag
{
    uint8_t                 msduHandle; /*!< The handle of the MSDU requested to be purged from the transaction queue. */
    resultType_t            status;     /*!< Status of the Purge Request: gSuccess_c, gInvalidHandle_c */
} mcpsPurgeCnf_t;

/*!  The MCPS-DATA.indication primitive indicates the transfer of a data SPDU (MSDU) from the MAC sublayer to the local SSCS entity. */
typedef MAC_STRUCT mcpsPromInd_tag
{
    uint8_t                 mpduLinkQuality; /*!< The LQI value measured during the reception of the MPDU. Lower values represent lower LQI */
    uint32_t                timeStamp;       /*!< The time, in symbols, at which the data were received. This is a 24-bit value */
    uint8_t                 msduLength;      /*!< The number of octets contained in the MSDU being indicated by the MAC sublayer entity. */
    uint8_t                 *pMsdu;          /*!< Pointer to a set of octets forming the MSDU to be transmitted by the MAC sublayer entity.
                                                  Upper layer must NOT free this pointer! */
} mcpsPromInd_t;

/*!  The MLME-ASSOCIATE.request primitive enables a device to request an association with a coordinator. */
typedef MAC_STRUCT mlmeAssociateReq_tag
{
    uint64_t                coordAddress;   /*!< The address of the coordinator with which to associate. */
    uint16_t                coordPanId;     /*!< The identifier of the PAN with which to associate. */
    addrModeType_t          coordAddrMode;  /*!< The coordinator addressing mode for this primitive and subsequent MPDU */
    logicalChannelId_t      logicalChannel; /*!< Indicates the logical channel on which to attempt association */
    macSecurityLevel_t      securityLevel;  /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    macCapabilityInfo_t     capabilityInfo; /*!< The operational capabilities of the device requesting association */
    channelPageId_t         channelPage;
} mlmeAssociateReq_t;

/*!  The MLME-ASSOCIATE.confirm primitive is used to inform the next higher layer of the initiating device whether its request to associate was successful or unsuccessful. */
typedef MAC_STRUCT mlmeAssociateCnf_tag
{
    uint16_t                assocShortAddress; /*!< The short device address allocated by the coordinator on successful association. 
                                                    This parameter will be equal to 0xffff if the association attempt was unsuccessful. */
    resultType_t            status;
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeAssociateCnf_t;

/*!  The MLME-ASSOCIATE.response primitive is used to initiate a response to an MLME-ASSOCIATE.indication primitive. */
typedef MAC_STRUCT mlmeAssociateRes_tag
{
    uint64_t                deviceAddress; /*!< The address of the device requesting association. */
    uint16_t                assocShortAddress; /*!< Contains the short device address allocated by the coordinator on successful association. 
                                                    This parameter is set to 0xfffe if the no short address was allocated. The 64-bit address must be used.
                                                    This parameter is set to 0xffff if the association was unsuccessful */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    resultType_t            status;            /*!< The status of the association attempt: gAssociationSuccessful_c, gPanAtCapacity_c, gPanAccessDenied_c */
} mlmeAssociateRes_t;

/*!  The MLME-ASSOCIATE.indication primitive is used to indicate the reception of an association request command. */
typedef MAC_STRUCT mlmeAssociateInd_tag
{
    uint64_t                deviceAddress; /*!< The 64-bit address of the device requesting association. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    macCapabilityInfo_t     capabilityInfo; /*!< The operational capabilities of the device requesting association */
} mlmeAssociateInd_t;

/*!  The MLME-DISASSOCIATE.request primitive is used by an associated device to notify the coordinator of its intent to leave the PAN. It is also used by the coordinator to instruct an associated device to leave the PAN. */
typedef MAC_STRUCT mlmeDisassociateReq_tag
{
    uint64_t                deviceAddress; /*!< The address of the device to which to send the disassociation notification command. */
    uint16_t                devicePanId;   /*!< The PAN identifier of the device to which to send the disassociation notification command.*/
    addrModeType_t          deviceAddrMode;/*!< The addressing mode of the device to which to send the disassociation notification command */
    macDisassociateReason_t disassociateReason;
    bool_t                  txIndirect;    /*!< Indicates whether the disassociation notification shall be sent indirectly. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeDisassociateReq_t;

/*!  The MLME-DISASSOCIATE.confirm primitive reports the results of an MLME-DISASSOCIATE.request primitive. */
typedef MAC_STRUCT mlmeDisassociateCnf_tag
{
    uint64_t                deviceAddress; /*!< The address of the device that has either requested disassociation or has been instructed to disassociate by its coordinator. */
    uint16_t                devicePanId;   /*!< The PAN identifier of the device that has either requested disassociation or has been instructed to disassociate by its coordinator. */
    addrModeType_t          deviceAddrMode;/*!< The addressing mode of the device that has either requested disassociation or has been instructed to disassociate by its coordinator.  */
    resultType_t            status; /*!< The status of the disassociation attempt: gSuccess_c, gTransactionOverflow_c, gTransactionExpired_c, gNoAck_c, gChannelAccessFailure_c, gCounterError_c, gFrameTooLong_c, gUnavailableKey_c */
} mlmeDisassociateCnf_t;

/*!  The MLME-DISASSOCIATE.indication primitive is used to indicate the reception of a disassociation notification command. */
typedef MAC_STRUCT mlmeDisassociateInd_tag
{
    uint64_t                deviceAddress; /*!< The address of the device requesting disassociation. */
    macDisassociateReason_t disassociateReason;
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeDisassociateInd_t;

/*!  The MLME-GET.request primitive requests information about a given PIB attribute. This request is synchronous */
typedef MAC_STRUCT mlmeGetReq_tag
{
    pibId_t                 pibAttribute;      /*!< The identifier of the PIB attribute. */
    uint8_t                 pibAttributeIndex; /*!< The index within the table of the specified PIB attribute to read.
                                                    This parameter is valid only for the MAC PIB attributes that are tables; it is ignored when accessing PHY PIB attributes. */
    void*                   pibAttributeValue; /*!< Pointer to a location where the PIB value will be stored. (Not in Spec) */
}mlmeGetReq_t;

/* MLME-SET.Confirm structure (Not used - Request is synchronous) */
/*! \cond DOXY_SKIP_TAG */
typedef MAC_STRUCT mlmeGetCnf_tag
{
    resultType_t            status;
    pibId_t                 pibAttribute;
    uint8_t                 pibAttributeIndex;
    void*                   pibAttributeValue;
}mlmeGetCnf_t;
/*! \endcond */

/*!  The MLME-RX-ENABLE.request primitive enables the next higher layer to request that the receiver is either enabled for a finite period of time or disabled. */
typedef MAC_STRUCT mlmeRxEnableReq_tag
{
    bool_t                  deferPermit; /*!< TRUE if the requested operation can be deferred until the next superframe if the requested time has already passed. 
                                              FALSE if the requested operation is only to be attempted in the current superframe. 
                                              This parameter is ignored for non beacon-enabled PANs. */
    uint32_t                rxOnTime;    /*!< Contains the number of symbols measured from the start of the superframe before the receiver is to be enabled or disabled. 
                                              This is a 24-bit value, and it is ignored for non beacon-enabled PANs. */
    uint32_t                rxOnDuration;/*!< Contains the number of symbols for which the receiver is to be enabled. 
                                              If this parameter is equal to 0x000000, the receiver is disabled. 
                                              This is a 24-bit value. */
#ifdef gMAC2011_d
    macRangingRxControl_t   rangingRxControl;
#endif
} mlmeRxEnableReq_t;

/*!  The MLME-RX-ENABLE.confirm primitive reports the results of the attempt to enable or disable the receiver. */
typedef MAC_STRUCT mlmeRxEnableCnf_tag
{
    resultType_t            status; /*!< Possible values: gSuccess_c, gPastTime_c, gOnTimeTooLong_c, gInvalidParameter_c */
} mlmeRxEnableCnf_t;

/*!  The MLME-SAP scan primitives define how a device can determine the energy usage or the presence or absence of PANs in a communications channel.
     The MLME-SCAN.request primitive is used to initiate a channel scan over a given list of channels. 
     A device can use the channel scan to measure the energy on the channel, search for the coordinator with which it associated, or search for all coordinators transmitting beacon frames within the POS of the scanning device. */
typedef MAC_STRUCT mlmeScanReq_tag
{
    macScanType_t           scanType;      /*!< Energy Detect, Active, Passive, Orphan */
    channelMask_t           scanChannels;  /*!< Channel bit-mask (1 = scan, 0 = do not scan) */
    uint8_t                 scanDuration;  /*!< Valid rage: 0-14. Duration in symbols: aBaseSuperframeDuration * ((2^scanDuration) + 1) */
    channelPageId_t         channelPage;
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeScanReq_t;

/*!  PAN Descriptor Block structure */
typedef MAC_STRUCT panDescriptorBlock_tag
{
    panDescriptor_t               panDescriptorList[gScanResultsPerBlock_c]; /*!< Array of PAN descriptors. */
    uint8_t                       panDescriptorCount; /*!< The number of PAD descriptors in this block. */
    struct panDescriptorBlock_tag *pNext; /*!< Pointer to the next PAN descriptor block, or NULL if this is the last block. */
} panDescriptorBlock_t;

/*!  The MLME-SCAN.confirm primitive reports the result of the channel scan request. The panDescriptor_t type provides a link pointer for a chained list. */
typedef MAC_STRUCT mlmeScanCnf_tag
{
    resultType_t            status;                    /*!< The status of the scan attempt: gSuccess_c, gLimitReached_c, gNoBeacon_c, gScanInProgress_c, gCounterError_c, gFrameTooLong_c, gUnavailableKey_c, gUnsupportedSecurity_c, gInvalidParameter_c */
    macScanType_t           scanType;                  /*!< Energy Detect, Active, Passive, Orphan */
    channelPageId_t         channelPage;
    uint8_t                 resultListSize;            /*!< The number of elements returned in the appropriate result lists */
    channelMask_t           unscannedChannels;         /*!< Indicates which channels given in the request were not scanned (1 = not scanned, 0 = scanned or not requested). 
                                                            This parameter is not valid for ED scans */
    MAC_UNION {
        uint8_t*              pEnergyDetectList;       /*!< The list of energy measurements, one for each channel searched during an ED scan. 
                                                            Upper layer must free this message */
        panDescriptorBlock_t* pPanDescriptorBlockList; /*!< Pointer to array of PAN descriptors, one for each beacon found during an active or passive scan. This parameter is null for orphan scans or when macAutoRequest is set to FALSE during an active or passive scan.
                                                            Upper layer must free this message */
    } resList;
} mlmeScanCnf_t;

/*!  The MLME-SAP communication status primitive defines how the MLME communicates to the next higher layer about transmission status, when the transmission was instigated by a response primitive, and about security errors on incoming packets.
     The MLME-COMM-STATUS.indication primitive allows the MLME to indicate a communications status. */
typedef MAC_STRUCT mlmeCommStatusInd_tag
{
    uint64_t                srcAddress;  /*!< Contains the individual device address of the entity, from which the frame causing the error originated. */
    uint16_t                panId;       /*!< PAN identifier of the device from which the frame was received, or to which the frame was being sent. */
    addrModeType_t          srcAddrMode; /*!< The source addressing mode for this primitive. */
    uint64_t                destAddress; /*!< Contains the individual device address of the device, for which the frame was intended. */
    addrModeType_t          destAddrMode;  /*!< The destination addressing mode for this primitive. */
    resultType_t            status;        /*!< The communication status */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeCommStatusInd_t;

/*!  The MLME-SET.request primitive is generated by the next higher layer, and issued to its MLME to write the indicated PIB attribute. (Request is synchronous) */
typedef MAC_STRUCT mlmeSetReq_tag
{
    pibId_t                 pibAttribute;      /*!< The identifier of the PIB attribute. */
    uint8_t                 pibAttributeIndex; /*!< The index within the table of the specified PIB attribute to read.
                                                    This parameter is valid only for the MAC PIB attributes that are tables; it is ignored when accessing PHY PIB attributes. */
    void*                   pibAttributeValue; /*!< Pointer to the new value of the PIB */
} mlmeSetReq_t;

/* MLME-SET.Confirm structure (Not used - Request is synchronous) */
/*! \cond DOXY_SKIP_TAG */
typedef MAC_STRUCT mlmeSetCnf_tag
{
    resultType_t            status;
    pibId_t                 pibAttribute;
    uint8_t                 pibAttributeIndex;
} mlmeSetCnf_t;
/*! \endcond */

/*!  The MLME-RESET.request primitive is generated by the next higher layer and issued to the MLME to request a reset of the MAC sublayer to its initial conditions. 
     This primitive is issued before using the MLME-START.request or the MLME-ASSOCIATE.request primitives. (Request is synchronous) */
typedef MAC_STRUCT mlmeResetReq_tag
{
    bool_t                  setDefaultPIB; /*!< If TRUE, all MAC PIBs are set to their default values */
} mlmeResetReq_t;

/*!  MLME-RESET.Confirm structure (Not used - Request is synchronous) */
typedef MAC_STRUCT mlmeResetCnf_tag
{
    resultType_t            status;
} mlmeResetCnf_t;

/*!  The MLME-START.request primitive enables the PAN coordinator to initiate a new PAN, or to begin using a new superframe configuration. 
     This primitive may also be used by a device already associated with an existing PAN to begin using a new superframe configuration */
typedef MAC_STRUCT mlmeStartReq_tag
{
    uint16_t                panId;                /*!< The PAN identifier to be used by the device. */
    logicalChannelId_t      logicalChannel;       /*!< The logical channel on which to start using the new superframe configuration. */
    channelPageId_t         channelPage;          /*!< The channel page on which to start using the new superframe configuration */
    uint32_t                startTime;            /*!< The time at which to begin transmitting beacons. If this parameter is equal to 0x000000, beacon transmissions will begin immediately. This is a 24 bit value. */
    uint8_t                 beaconOrder;          /*!< How often the beacon is to be transmitted (0-15). A value of 15 indicates that the coordinator will not transmit periodic beacons. */
    uint8_t                 superframeOrder;      /*!< The length of the active portion of the superframe, including the beacon frame (0-14). If the beaconOrder parameter (BO) has a value of 15, this parameter is ignored. */
    bool_t                  panCoordinator;       /*!< Indicates if the device will become the PAN coordinator of a new PAN or begin using a new superframe configuration on the PAN with which it is associated. */
    bool_t                  batteryLifeExtension; /*!< If this value is TRUE, the receiver of the beaconing device is disabled macBattLifeExtPeriods full backoff periods after the inter-frame spacing (IFS) period following the beacon frame. 
                                                       If this value is FALSE, the receiver of the beaconing device remains enabled for the entire CAP. */
    bool_t                  coordRealignment;     /*!< Indicates if a coordinator realignment command is to be transmitted before changing the superframe */
    macSecurityLevel_t      coordRealignSecurityLevel; /*!< The security level to be used */
    keyIdModeType_t         coordRealignKeyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                coordRealignKeySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 coordRealignKeyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    macSecurityLevel_t      beaconSecurityLevel; /*!< The security level to be used */
    keyIdModeType_t         beaconKeyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                beaconKeySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 beaconKeyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeStartReq_t;

/*!  The MLME-START.confirm primitive reports the results of the attempt to start using a new superframe configuration.
     The MLME-START.confirm primitive is generated by the MLME, and issued to NWK layer in response to the MLME-START.request primitive. 
     The MLME-START.confirm primitive returns a status of either SUCCESS, indicating that the MAC sublayer has started using the new superframe configuration, or an appropriate error code.
 */
typedef MAC_STRUCT mlmeStartCnf_tag
{
    resultType_t             status; /*!< Possible values: gSuccess_c, gNoShortAddress_c, gSuperframeOverlap_c, gTrackingOff_c, gInvalidParameter_c, gCounterError_c, gFrameTooLong_c, gUnavailableKey_c, gUnsupportedSecurity_c, gChannelAccessFailure_c */
} mlmeStartCnf_t;

/*!  The MLME-POLL.request primitive prompts the device to request data from the coordinator */
typedef MAC_STRUCT mlmePollReq_tag
{
    addrModeType_t          coordAddrMode; /*!< The coordinator addressing mode to which the poll request is intended. */
    uint16_t                coordPanId;    /*!< The PAN identifier of the coordinator for which the poll is intended. */
    uint64_t                coordAddress;  /*!< The address of the coordinator for which the poll is intended. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmePollReq_t;

/*!  MLME-POLL.Confirm structure */
typedef MAC_STRUCT mlmePollCnf_tag
{
    resultType_t            status;
} mlmePollCnf_t;

/*!  The MLME-BEACON-NOTIFY.indication primitive is used to send parameters contained within a beacon frame received by the MAC sublayer to the next higher layer. 
     The primitive also sends a measure of the LQI and the time the beacon frame was received. */
typedef MAC_STRUCT mlmeBeaconNotifyInd_tag
{
    uint8_t                 bsn;            /*!< The beacon sequence number */
    uint8_t                 pendAddrSpec;   /*!< The beacon pending address specification.
                                                 Bits 0-2 indicate the number of short addresses contained in the Address List field of the beacon frame.
                                                 Bits 4-6 indicate the number of extended addresses contained in the Address List field of the beacon frame. */
    uint8_t                 sduLength;      /*!< The number of octets contained in the beacon payload of the beacon frame received by the MAC sublayer. */
    uint8_t*                pAddrList;      /*!< The list of addresses of the devices for which the beacon source has data */
    panDescriptor_t*        pPanDescriptor; /*!< The PANDescriptor for the received beacon. */
    uint8_t*                pSdu;           /*!< The set of octets comprising the beacon payload to be transferred from the MAC sublayer entity to the next higher layer. */
#if gTschSupport_d
    uint8_t                 ebsn;
    beaconType_t            beaconType;
#endif
    void*                   pBufferRoot; /*!< Pointer to the start of the message received by air. 
                                              The upper layer must free this buffer before freeing the indication message. */
} mlmeBeaconNotifyInd_t;

/*!  The MLME-GTS.request primitive enables a device to send a request to the PAN coordinator to allocate a new GTS or to deallocate an existing GTS. 
     This primitive is also used by the PAN coordinator to initiate a GTS deallocation. */
typedef MAC_STRUCT mlmeGtsReq_tag
{
    gtsCharacteristics_t    gtsCharacteristics; /*!< The characteristics of the GTS request, including whether the request is for the allocation of a new GTS or the deallocation of an existing GTS. */
    macSecurityLevel_t      securityLevel;      /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeGtsReq_t;

/*!  The MLME-GTS.confirm primitive reports the results of a request to allocate a new GTS or deallocate an existing GTS. */
typedef MAC_STRUCT mlmeGtsCnf_tag
{
    resultType_t            status; /*!< Possible values: gSuccess_c, gDenied_c, gNoShortAddress_c, gChannelAccessFailure_c, gNoAck_c, gNoData_c, gCounterError_c, gFrameTooLong_c, gUnavailableKey_c, gUnsupportedSecurity_c, gInvalidParameter_c */
    gtsCharacteristics_t    gtsCharacteristics;
} mlmeGtsCnf_t;

/*!  The MLME-GTS.indication primitive indicates that a GTS has been allocated or that a previously allocated GTS has been deallocated. */
typedef MAC_STRUCT mlmeGtsInd_tag
{
    uint16_t                deviceAddress;      /*!< The 16-bit short address of the device that has been allocated or deallocated a GTS. */
    gtsCharacteristics_t    gtsCharacteristics; /*!< The characteristics of the GTS. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeGtsInd_t;

/*!  The MLME-ORPHAN.indication primitive allows the MLME of a coordinator to notify the next higher layer of the presence of an orphaned device. */
typedef MAC_STRUCT mlmeOrphanInd_tag
{
    uint64_t                orphanAddress; /*!< The extended address of the orphaned device. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeOrphanInd_t;

/*!  The MLME-ORPHAN.response primitive is generated by the next higher layer and issued to its MLME when it reaches a decision about whether the orphaned device indicated in the MLME-ORPHAN.indication primitive is associated. */
typedef MAC_STRUCT mlmeOrphanRes_tag
{
    uint64_t                orphanAddress;    /*!< The extended address of the orphaned device. */
    uint16_t                shortAddress;     /*!< The 16-bit short address allocated to the orphaned device if it is associated with this coordinator. */
    bool_t                  associatedMember; /*!< TRUE if the orphaned device is associated with this coordinator or FALSE otherwise. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeOrphanRes_t;

/*!  The MLME-SYNC.request primitive is generated by the next higher layer of a device on a beacon-enabled PAN, and issued to its MLME to synchronize with the coordinator. */
typedef MAC_STRUCT mlmeSyncReq_tag
{
    logicalChannelId_t      logicalChannel; /*!< The logical channel on which to attempt coordinator synchronization. */
    channelPageId_t         channelPage;    /*!< The channel page on which to attempt coordinator synchronization. */
    bool_t                  trackBeacon;    /*!< TRUE if the MLME is to synchronize with the next beacon, and to attempt to track all future beacons. 
                                                 FALSE if the MLME is to synchronize with only the next beacon. */
} mlmeSyncReq_t;

/*!  This primitive is generated by the MLME of a device, and issued to its next higher layer in the event of losing synchronization with the coordinator. 
     It is also generated by the MLME of the PAN coordinator and issued to its next higher layer in the event of a PAN ID conflict. */
typedef MAC_STRUCT mlmeSyncLossInd_tag
{
    resultType_t            lossReason; /*!< Possible values: gPanIdConflict_c, gRealignment_c, gBeaconLost_c */
    uint16_t                panId;      /*!< The PAN identifier with which the device lost the synchronization, or to which it was realigned. */
    logicalChannelId_t      logicalChannel; /*!< The logical channel on which the device lost the synchronization, or to which it was realigned. */
    channelPageId_t         channelPage;    /*!< The channel page on which the device lost the synchronization, or to which it was realigned. */
    macSecurityLevel_t      securityLevel; /*!< The security level to be used */
    keyIdModeType_t         keyIdMode; /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */
    uint64_t                keySource; /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
    uint8_t                 keyIndex;  /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */
} mlmeSyncLossInd_t;

/*!  The MLME-POLL-NOTIFY.indication primitive reports a poll indication message to the network (Not in spec) */
typedef MAC_STRUCT mlmePollNotifyInd_tag
{
    addrModeType_t          srcAddrMode; /*!< The source addressing mode for this primitive. */
    uint64_t                srcAddr;     /*!< The individual device address of the entity from which the notification was received. */
    uint16_t                srcPanId;    /*!< PAN identifier of the entity from which the notification was received. */
} mlmePollNotifyInd_t;

/*!  The MLME-SET-SLOTFRAME.request primitive is generated by the next higher layer, and issued to the MLME to request a set of a TSCH slotframe to the MAC sublayer. */
typedef MAC_STRUCT mlmeSetSlotframeReq_tag
{
    uint8_t                 slotframeHandle; /*!< Unique identifier of the slotframe */
    macSetSlotframeOp_t     operation;       /*!< Operation to be performed on the link (add, delete or modify) */
    uint16_t                size;            /*!< Number of timeslots in the new slotframe */
} mlmeSetSlotframeReq_t;

/*!  The MLME-SET-SLOTFRAME.confirm primitive is generated by the MAC layer and issued to the next higher layer to confirm the set of a TSCH slotframe to the MAC sublayer. */
typedef MAC_STRUCT mlmeSetSlotframeCnf_tag
{
    uint8_t                 slotframeHandle; /*!< Unique identifier of the slotframe to be added, deleted, or modified. */
    resultType_t            status; /*!< Possible values: gSuccess_c, gInvalidParameter_c, gSlotframeNotFound_c, gMaxSlotframesExceeded_c */
} mlmeSetSlotframeCnf_t;

/*!  The MLME-SET-LINK.request primitive is generated by the next higher layer, and issued to the MLME to request a set of a TSCH link to the MAC sublayer. */
typedef MAC_STRUCT mlmeSetLinkReq_tag
{
    macSetLinkOp_t          operation;       /*!< Operation to be performed on the link (add, delete or modify) */
    uint16_t                linkHandle;      /*!< Unique identifier (local to specified slotframe) for the link. */
    uint8_t                 slotframeHandle; /*!< The slotframeHandle of the slotframe to which the link is associated. */
    uint16_t                timeslot;        /*!< Timeslot of the link to be added. */
    uint16_t                channelOffset;   /*!< The Channel offset of the link. */
    macLinkOptions_t        linkOptions;     /*!< Bit0: TX
                                                  Bit1: RX
                                                  Bit2: Shared (for TX links)
                                                  Bit3: Timekeeping (for RX links) */
    macLinkType_t           linkType;        /*!< Type of the link; indicates if the link may be used for sending Enhanced Beacons. */
    uint16_t                nodeAddr;        /*!< Address of the neighbour device connected via the link. 0xffff indicates that the link can be used for frames sent with broadcast address. */
} mlmeSetLinkReq_t;

/*!  The MLME-SET-LINK.confirm primitive is generated by the MAC layer, and issued to the next higher layer to confirm the set of a TSCH link to the MAC sublayer */
typedef MAC_STRUCT mlmeSetLinkCnf_tag
{
    resultType_t            status;          /*<! Possible values: gSuccess_c, gInvalidParameter_c, gUnkonwnLink_c, gMaxLinksExceeded_c */
    uint16_t                linkHandle;      /*!< A unique (local to specified slotframe) identifier of the link. */
    uint8_t                 slotframeHandle; /*!< The slotframeHandle of the slotframe to which the link is associated. */
} mlmeSetLinkCnf_t;

/*!  The MLME-TSCH-MODE.request primitive is generated by the next higher layer, and issued to the MLME to request the TSCH enable or disable procedure */
typedef MAC_STRUCT mlmeTschModeReq_tag
{
    macTschMode_t           tschMode;
} mlmeTschModeReq_t;

/*!  The MLME-TSCH-MODE.confirm primitive is generated by the MAC layer, and issued to the next higher layer to confirm the enable or disable of the TSCH module. */
typedef MAC_STRUCT mlmeTschModeCnf_tag
{
    macTschMode_t           tschMode;
    resultType_t            status;   /*!< Possible values: gSuccess_c, gNoSync_c */
} mlmeTschModeCnf_t;

/*!  The MLME-KEEP-ALIVE.request primitive is generated by the next higher layer, and issued to the MLME to request monitor of frames exchanged with a TSCH neighbour used as clock source, and configure the keep-alive period in timeslots for that clock source */
typedef MAC_STRUCT mlmeKeepAliveReq_tag
{
    uint16_t                dstAddr;         /*!< Address of the neighbor device with which to maintain the timing. Keep-alives with dstAddr of 0xffff do not expect to be acknowledged, and cannot be used for timekeeping */
    uint16_t                keepAlivePeriod; /*!< Period in timeslots after which a frame is sent if no frames have been sent to dstAddr. */
} mlmeKeepAliveReq_t;

/*!  The MLME-KEEP-ALIVE.confirm primitive is generated by the MAC layer, and issued to the next higher layer to confirm the request to configure the keep-alive frames for a TSCH clock source. */
typedef MAC_STRUCT mlmeKeepAliveCnf_tag
{
    resultType_t            status; /*!< Possible values: gSuccess_c, gInvalidParameter_c */
} mlmeKeepAliveCnf_t;

/*!  The MLME-BEACON.request primitive is generated by the next higher layer, and issued to the MLME to request the MAC layer to send a beacon or enhanced beacon frame with the specified parameters. 
     It is used for advertising the TSCH parameters to allow new devices to join the network. */
typedef MAC_STRUCT mlmeBeaconReq_tag
{
    beaconType_t            beaconType;          /*!< Indicates whether to send a beacon or an enhanced beacon. */
    uint8_t                 channel;             /*!< The channel number to use. */
    channelPageId_t         channelPage;         /*!< The channel page to use. */
    uint8_t                 superframeOrder;     /*!< The length of the active portion of the superframe, including the beacon frame (0-15). */
    macSecurityLevel_t      beaconSecurityLevel; /*!< The security level to be used */ /*!< Indicates the security level to be used in beacon frame. */
    keyIdModeType_t         beaconKeyIdMode;     /*!< The mode used to identify the key to be used. This parameter is ignored if the SecurityLevel parameter is set to 0x00. */     /*!< The field contains the mode used to identify the key to be used. This parameter is ignored if the beaconSecurityLevel parameter is set to gMacSecurityNone_c.  */
    uint64_t                beaconKeySource;     /*!< The originator of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */     /*!< Indicates the originator of the key to be used. Ignored if the beaconKeyIdMode parameter is ignored or set to gKeyIdMode0_c */
    uint8_t                 beaconKeyIndex;      /*!< The index of the key to be used. This parameter is ignored if the KeyIdMode parameter is ignored or set to 0x00. */      /*!< Indicates the index of the key to be used. Ignored if the beaconKeyIdMode parameter is ignored or set to gKeyIdMode0_c. */
    addrModeType_t          dstAddrMode;         /*!< The destination addressing mode for this primitive. */         /*!< The destination addressing mode for this primitive and subsequent MPDU */
    uint64_t                dstAddr;             /*!< The individual device address of the entity to which the MSDU is being transferred */
    bool_t                  bsnSuppression;      /*!< If BeaconType = 0x01, then if BSNSuppression is TRUE, the EBSN is omitted from the frame and the Sequence Number Suppression field of the Frame Control field is set to 1. */
} mlmeBeaconReq_t;

/*!  The MLME-BEACON.confirm primitive is generated by the MAC layer, and issued to the next higher layer to indicate the result of the MLME-BEACON.request primitive. */
typedef MAC_STRUCT mlmeBeaconCnf_tag
{
    resultType_t            status; /*!< Indicates results of the MLME-BEACON.request: gSuccess_c, gChannelAccessFailure_c, gFrameTooLong_c, gInvalidParameter_c */
} mlmeBeaconCnf_t;

/************************************************************************************/

/*!  Common MAC message header structure */
typedef MAC_STRUCT macMsgHeader_tag{
    macMessageId_t      msgType; /*!< The id of the MAC massage */
} macMsgHeader_t;

/*!  This structure describes the generic MAC message that enables communication from the NWK layer to the MCPS Service Access Point */
typedef MAC_STRUCT nwkToMcpsMessage_tag
{
    macMessageId_t      msgType; /*!< The id of the MAC massage */
    MAC_UNION
    {
        mcpsDataReq_t       dataReq;
        mcpsPurgeReq_t      purgeReq;
    } msgData;
} nwkToMcpsMessage_t;

/*!  This structure describes the generic MAC message that enables communication from the MCPS Service Access Point to the NWK layer */
typedef MAC_STRUCT mcpsToNwkMessage_tag
{
    macMessageId_t      msgType; /*!< The id of the MAC massage */
    MAC_UNION
    {
        mcpsDataCnf_t       dataCnf;
        mcpsDataInd_t       dataInd;
        mcpsPurgeCnf_t      purgeCnf;
        mcpsPromInd_t       promInd;
    } msgData;
} mcpsToNwkMessage_t;

/*!  This structure describes the generic MAC message that enables communication from the NWK layer to the MLME Service Access Point. */
typedef MAC_STRUCT mlmeMessage_tag
{
    macMessageId_t      msgType; /*!< The id of the MAC massage */
    MAC_UNION
    {
        mlmeAssociateReq_t      associateReq;
        mlmeAssociateRes_t      associateRes;
        mlmeDisassociateReq_t   disassociateReq;
        mlmeGetReq_t            getReq;
        mlmeGtsReq_t            gtsReq;
        mlmeOrphanRes_t         orphanRes;
        mlmeResetReq_t          resetReq;
        mlmeRxEnableReq_t       rxEnableReq;
        mlmeScanReq_t           scanReq;
        mlmeSetReq_t            setReq;
        mlmeStartReq_t          startReq;
        mlmeSyncReq_t           syncReq;
        mlmePollReq_t           pollReq;
        mlmeSetSlotframeReq_t   setSlotframeReq;
        mlmeSetLinkReq_t        setLinkReq;
        mlmeTschModeReq_t       tschModeReq;
        mlmeKeepAliveReq_t      keepAliveReq;
        mlmeBeaconReq_t         beaconReq;
    } msgData;
} mlmeMessage_t;

/*!  This structure describes the generic MAC message that enables communication from the MLME Service Access Point to the NWK layer. */
typedef MAC_STRUCT nwkMessage_tag
{
    macMessageId_t      msgType; /*!< The id of the MAC massage */
    MAC_UNION
    {
        mlmeAssociateInd_t      associateInd;
        mlmeAssociateCnf_t      associateCnf;
        mlmeDisassociateInd_t   disassociateInd;
        mlmeDisassociateCnf_t   disassociateCnf;
        mlmeBeaconNotifyInd_t   beaconNotifyInd;
        mlmeGetCnf_t            getCnf;         /*!< Not used. Request is synchronous */
        mlmeGtsInd_t            gtsInd;
        mlmeGtsCnf_t            gtsCnf;
        mlmeOrphanInd_t         orphanInd;
        mlmeResetCnf_t          resetCnf;       /*!< Not used. Request is synchronous */
        mlmeRxEnableCnf_t       rxEnableCnf;
        mlmeScanCnf_t           scanCnf;
        mlmeCommStatusInd_t     commStatusInd;
        mlmeSetCnf_t            setCnf;         /*!< Not used. Request is synchronous */
        mlmeStartCnf_t          startCnf;
        mlmeSyncLossInd_t       syncLossInd;
        mlmePollCnf_t           pollCnf;
        mlmePollNotifyInd_t     pollNotifyInd;
        mlmeSetSlotframeCnf_t   setSlotframeCnf;/*!< Not used. Request is synchronous */
        mlmeSetLinkCnf_t        setLinkCnf;     /*!< Not used. Request is synchronous */
        mlmeTschModeCnf_t       tschModeCnf;    /*!< Not used. Request is synchronous */
        mlmeKeepAliveCnf_t      keepAliveCnf;   /*!< Not used. Request is synchronous */
        mlmeBeaconCnf_t         beaconCnf;        
    } msgData;
} nwkMessage_t;


/*! *********************************************************************************
*   Callback function type used for sending messages to upper layer.
* The callback must be installed by upper layers through the Mac_RegisterSapHandlers() function
*
* \param[in]  pMsg  Pointer to the message sent by the MCPS
* \param[in]  macInstanceId  The instance of the upper layer
*
* \return  Returns the status of the message sending operation.
*
********************************************************************************** */
typedef resultType_t (*MCPS_NWK_SapHandler_t) (mcpsToNwkMessage_t* pMsg, instanceId_t upperInstanceId);

/*! *********************************************************************************
*   Callback function type used for sending messages to upper layer.
* The callback must be installed by upper layers through the Mac_RegisterSapHandlers() function
*
* \param[in]  pMsg  Pointer to the message sent by the MLME
* \param[in]  macInstanceId  The instance of the upper layer
*
* \return  Returns the status of the message sending operation.
*
********************************************************************************** */
typedef resultType_t (*MLME_NWK_SapHandler_t) (nwkMessage_t* pMsg, instanceId_t upperInstanceId);


/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*   Initialize all the 802.15.4 MAC instances.
*
* \remarks  The 802.15.4 PHY must be allready initialized.
*
********************************************************************************** */
void MAC_Init( void );

/*! *********************************************************************************
*   Bind upper layer with MAC layer.
*
* \param[in]  nwkId  The instance of the upper layer
*
* \return  The instance of the 802.15.4 MAC bind.
*
* \retval  gInvalidInstanceId_c No MAC instance is available.
*
********************************************************************************** */
instanceId_t BindToMAC( instanceId_t nwkId );

/*! *********************************************************************************
*   This function will unbind a MAC instance from the upper layer
*
* \param[in] macInstanceId instance of the MAC
*
********************************************************************************** */
void UnBindFromMAC ( instanceId_t macInstanceId );

/*! *********************************************************************************
*   This function checks the states of all instances of the MAC, and reports the state.
*
* \return  If at least one of the instances has an Active sequence, gMacStateBusy_c is returned
*          Else if at least one of the instances has pending messages, gMacStateNotEmpty_c is returned
*          Else if all the instances are Idle and gMacStateIdle_c is returned
*          Else if the MAC is Idle, but the PHY is in RX, gMacIdleRx_c is returned
*
*
********************************************************************************** */
macState_t Mac_GetState( void );

/*! *********************************************************************************
*   This function registers the MCPS and MLME SAPs callbacks, offering support for 
*   the MCPS and MLME to NWK message interactions.
*
* \param[in]  pMCPS_NWK_SapHandler  Pointer to the MCPS callback handler function
* \param[in]  pMLME_NWK_SapHandler  Pointer to the MCPS callback handler function
* \param[in]  macInstanceId  The instance of the MAC layer
*
********************************************************************************** */
void Mac_RegisterSapHandlers( MCPS_NWK_SapHandler_t pMCPS_NWK_SapHandler,
                              MLME_NWK_SapHandler_t pMLME_NWK_SapHandler,
                              instanceId_t macInstanceId );

/*! *********************************************************************************
*   The SAP used by the upper layer to send MCPS requests
*
* \param[in]  pMsg  Pointer to the MCPS request to be sent to the MAC layer
* \param[in]  macInstanceId  The instance of the MAC layer
*
* \return  gSuccess_c if the message was sent to the MAC Layer.
*
********************************************************************************** */
resultType_t NWK_MCPS_SapHandler( nwkToMcpsMessage_t* pMsg, instanceId_t macInstanceId );

/*! *********************************************************************************
*   The SAP used by the upper layer to send MLME requests
*
* \param[in]  pMsg  Pointer to the MLME request to be sent to the MAC layer
* \param[in]  macInstanceId  The instance of the MAC layer
*
* \return  gSuccess_c if the message was sent to the MAC Layer.
*
********************************************************************************** */
resultType_t NWK_MLME_SapHandler( mlmeMessage_t*      pMsg, instanceId_t macInstanceId );

/*! *********************************************************************************
*   The SAP used by the PHY Data component to notify the MAC layer
*
* \param[in]  pMsg  Pointer to the message sent by the PHY to the MAC layer
* \param[in]  macInstanceId  The instance of the MAC layer
*
* \return  gSuccess_c
*
********************************************************************************** */
resultType_t PD_MAC_SapHandler  ( void* pMsg, instanceId_t macInstanceId );

/*! *********************************************************************************
*   The SAP used by the PHY Layer Management Entry to notify the MAC layer
*
* \param[in]  pMsg  Pointer to the message sent by the PHY to the MAC layer
* \param[in]  macInstanceId  The instance of the MAC layer
*
* \return  gSuccess_c
*
********************************************************************************** */
resultType_t PLME_MAC_SapHandler( void* pMsg, instanceId_t macInstanceId );

/*! *********************************************************************************
*  MAC Helper Function: This function returns the maximum MSDU that can be 
*  accommodated considering the current PHY configuration.
*
* \param[in]  pParams  Pointer to the MCPS-DATA.Request
*
* \return  The maximum size of the MSDU.
*
********************************************************************************** */
uint16_t Mac_GetMaxMsduLength( mcpsDataReq_t* pParams );

#ifdef __cplusplus
    }
#endif

#endif  /* _MAC_INTERFACE_H */

/*! *********************************************************************************
* @}
********************************************************************************** */
