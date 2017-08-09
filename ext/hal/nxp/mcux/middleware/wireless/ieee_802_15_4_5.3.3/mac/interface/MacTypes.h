/*! *********************************************************************************
* \defgroup MacTypes Mac Data Types
* @{
********************************************************************************** */
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

#ifndef _MAC_TYPES_H
#define _MAC_TYPES_H


/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "EmbeddedTypes.h"
#include "MacFunctionalityDefines.h"


/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/
/*! This constant describes the maximum number of PAN Descriptors that a PAN Descriptor list element can contain during a Scan procedure */
#define gScanResultsPerBlock_c      (5)

/*! Maximum number of MAC Scan results. Must multiple of \ref gScanResultsPerBlock_c */
#define gMaxScanResults_c           (2*gScanResultsPerBlock_c)

/************************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
************************************************************************************/

/*! This type describes the enumerations used as result values by the primitives defined in the MAC layer. */
typedef enum
{
    gSuccess_c                = 0x00,        /*!< The requested operation was completed successfully. For a transmission request, this value indicates a successful transmission.*/
    gPanAtCapacity_c          = 0x01,        /*!< Association Status - PAN at capacity.*/
    gPanAccessDenied_c        = 0x02,        /*!< Association Status - PAN access denied.*/
    gSlotframeNotFound_c      = 0x03,        /*!< MLME-SET-SLOTFRAME Status – requested slotframe not found */
    gMaxSlotframesExceeded_c  = 0x04,        /*!< MLME-SET-SLOTFRAME Status – maximum number of slotframes reached */
    gUnknownLink_c            = 0x05,        /*!< MLME-SET-LINK Status – requested link not found */
    gMaxLinksExceeded_c       = 0x06,        /*!< MLME-SET-LINK Status – maximum number of links  reached */
    gNoSync_c                 = 0x07,        /*!< MLME-TSCH-MODE Status – synchronization could not be performed to the PAN Coordinator */
    gRxEnableDone_c           = 0xd0,        /*!< NOT in Spec: MLME-RX-ENABLE ended */
#ifdef gMAC2011_d
    gDpsNotSupported_d                  = 0xd1, /*!< DPS related */

    gSoundingNotSupported_d             = 0xd2, /*!< Sounding related */

    gComputationNeeded_d                = 0xd3, /*!< Calibration related */

    gRangingNotSupported_d              = 0xd4, /*!< Ranging related */
    gRangingActive_d                    = 0xd5, /*!< Ranging related */
    gRangingRequestedButNotsupported_d  = 0xd6, /*!< Ranging related */
    gNoRangingRequested_d               = 0xd7, /*!< Ranging related */

    gUnavailableDevice_c                = 0xd8,  /*!< The DeviceDescriptor lookup lrocedure has failed for the received frame. No device was found which matches the input addressing mode, address and PAN Id */
    gUnavailableSecurityLevel_c         = 0xd9,  /*!< The SecurityLevelDescriptor lookup procedure has failed for the received frame. No SecurityLevelDescriptor was found which matches the input frmae type and command frame identifier. */
#endif
    gPollReqDecryptionError_c = 0xda,        /*!< NOT in Spec: The decryption of the received MAC Data Request Command frame failed */
    gCounterError_c           = 0xdb,        /*!< The frame counter purportedly applied by the originator of the received frame is invalid.*/
    gImproperKeyType_c        = 0xdc,        /*!< The key purportedly applied by the originator of the received frame is not allowed to be used with that frame type according to the key usage policy of the recipient.*/
    gImproperSecurityLevel_c  = 0xdd,        /*!< The security level purportedly applied by the originator of the received frame does not meet the minimum security level required/expected by the recipient for that frame type.*/
    gUnsupportedLegacy_c      = 0xde,        /*!< The received frame was purportedly secured using security based on IEEE Std 802.15.4-2003, and such security is not supported by this standard.*/
    gUnsupportedSecurity_c    = 0xdf,        /*!< The security purportedly applied by the originator of the received frame is not supported.*/
    gBeaconLoss_c             = 0xe0,        /*!< The beacon was lost following a synchronization request.*/
    gChannelAccessFailure_c   = 0xe1,        /*!< A transmission could not take place due to activity on the channel, i.e., the CSMA-CA mechanism has failed.*/
    gDenied_c                 = 0xe2,        /*!< The GTS request has been denied by the PAN coordinator.*/
    gDisableTrxFailure_c      = 0xe3,        /*!< The attempt to disable the transceiver has failed.*/
    gSecurityError_c          = 0xe4,        /*!< Cryptographic processing of the received secured frame failed.*/
    gFrameTooLong_c           = 0xe5,        /*!< Either a frame resulting from processing has a length that is greater than aMaxPHYPacketSize or a requested transaction is too large to fit in the CAP or GTS.*/
    gInvalidGts_c             = 0xe6,        /*!< The requested GTS transmission failed because the specified GTS either did not have a transmit GTS direction or was not defined.*/
    gInvalidHandle_c          = 0xe7,        /*!< A request to purge an MSDU from the transaction queue was made using an MSDU handle that was not found in the transaction table.*/
    gInvalidParameter_c       = 0xe8,        /*!< A parameter in the primitive is either not supported or is out of the valid range.*/
    gNoAck_c                  = 0xe9,        /*!< No acknowledgment was received after macMaxFrameRetries.*/
    gNoBeacon_c               = 0xea,        /*!< A scan operation failed to find any network beacons.*/
    gNoData_c                 = 0xeb,        /*!< No response data were available following a request.*/
    gNoShortAddress_c         = 0xec,        /*!< The operation failed because a 16-bit short address was not allocated.*/
    gOutOfCap_c               = 0xed,        /*!< A receiver enable request was unsuccessful because it could not be completed within the CAP. It is used only to meet the backwards compatibility requirements for IEEE Std 802.15.4-2003.*/
    gPanIdConflict_c          = 0xee,        /*!< A PAN identifier conflict has been detected and communicated to the PAN coordinator.*/
    gRealignment_c            = 0xef,        /*!< A coordinator realignment command has been received.*/
    gTransactionExpired_c     = 0xf0,        /*!< The transaction has expired and its information was discarded.*/
    gTransactionOverflow_c    = 0xf1,        /*!< There is no capacity to store the transaction.*/
    gTxActive_c               = 0xf2,        /*!< The transceiver was in the transmitter enabled state when the receiver was requested to be enabled.It is used only to meet the backwards compatibility requirements for IEEE Std 802.15.4-2003.*/
    gUnavailableKey_c         = 0xf3,        /*!< The key purportedly used by the originator of the received frame is not available or, if available, the originating device is not known or is blacklisted with that particular key.*/
    gUnsupportedAttribute_c   = 0xf4,        /*!< A SET/GET request was issued with the identifier of a PIB attribute that is not supported.*/
    gInvalidAddress_c         = 0xf5,        /*!< A request to send data was unsuccessful because neither the source address parameters nor the destination address parameters were present.*/
    gOnTimeTooLong_c          = 0xf6,        /*!< A receiver enable request was unsuccessful because it specified a number of symbols that was longer than the beacon interval.*/
    gPastTime_c               = 0xf7,        /*!< A receiver enable request was unsuccessful because it could not be completed within the current superframe and was not permitted to be deferred until the next superframe.*/
    gTrackingOff_c            = 0xf8,        /*!< The device was instructed to start sending beacons based on the timing of the beacon transmissions of its coordinator, but the device is not currently tracking the beacon of its coordinator.*/
    gInvalidIndex_c           = 0xf9,        /*!< An attempt to write to a MAC PIB attribute that is in a table failed because the specified table index was out of range.*/
    gLimitReached_c           = 0xfa,        /*!< A scan operation terminated prematurely because the number of PAN descriptors stored reached an implementation-specified maximum.*/
    gReadOnly_c               = 0xfb,        /*!< A SET/GET request was issued with the identifier of an attribute that is read only.*/
    gScanInProgress_c         = 0xfc,        /*!< A request to perform a scan operation failed because the MLME was in the process of performing a previously initiated scan operation.*/
    gSuperframeOverlap_c      = 0xfd,        /*!< The device was instructed to start sending beacons based on the timing of the beacon transmissions of its coordinator, but the instructed start time overlapped the transmission time of the beacon of its coordinator.*/
} resultType_t;

/*! \brief The 802.15.4 addressing modes */
typedef enum
{
    gAddrModeNoAddress_c            = 0x00,        /*!< No address (addressing fields omitted)*/
    gAddrModeReserved_c             = 0x01,        /*!< Reserved value*/
    gAddrModeShortAddress_c         = 0x02,        /*!< 16-bit short address*/
    gAddrModeExtendedAddress_c      = 0x03,        /*!< 64-bit extended address*/
} addrModeType_t;

/*! Bit type used to indicate the transmission options for the MSDU */
typedef enum
{
    gMacTxOptionsNone_c             = 0x00,        /*!< No Tx options specified. */
    gMacTxOptionsAck_c              = 0x01,        /*!< Acknowledged transmission is required.*/
    gMacTxOptionGts_c               = 0x02,        /*!< GTS transmission is required.*/
    gMacTxOptionIndirect_c          = 0x04,        /*!< Indirect transmission is required.*/
    gMacTxOptionFramePending_c      = 0x40,        /*!< Force set the FP bit of the Frame Control field. */
    gMacTxOptionAltExtAddr_c        = 0x80,        /*!< Use alternate Extended Address for nonce creation (MAC Security).*/
}macTxOptions_t;

/*! \brief MAC Security */
typedef enum
{
    gMacSecurityNone_c              = 0x00, /*!<    Data confidentiality: NO \n
                                                    Data authenticity: NO */

    gMacSecurityMic32_c             = 0x01, /*!<    Data confidentiality: NO \n
                                                    Data authenticity: YES \n
                                                    Data authenticity size: 4 bytes */

    gMacSecurityMic64_c             = 0x02, /*!<    Data confidentiality: NO \n
                                                    Data authenticity: YES \n
                                                    Data authenticity size: 8 bytes */

    gMacSecurityMic128_c            = 0x03, /*!<    Data confidentiality: NO \n
                                                    Data authenticity: YES \n
                                                    Data authenticity size: 16 bytes */

    gMacSecurityEnc_c               = 0x04, /*!<    Data confidentiality: YES \n
                                                    Data authenticity: NO */

    gMacSecurityEncMic32_c          = 0x05, /*!<    Data confidentiality: YES \n
                                                    Data authenticity: YES \n
                                                    Data authenticity size: 4 bytes */

    gMacSecurityEncMic64_c          = 0x06, /*!<    Data confidentiality: YES \n
                                                    Data authenticity: YES \n
                                                    Data authenticity size: 8 bytes */

    gMacSecurityEncMic128_c         = 0x07, /*!<    Data confidentiality: YES \n
                                                    Data authenticity: YES \n
                                                    Data authenticity size: 16 bytes */
} macSecurityLevel_t;

/*! \brief Security Key Identifier */
typedef enum
{
    gKeyIdMode0_c                   = 0x00,        /*!< Key is determined implicitly from the originator and recipient(s) of the frame, as indicated in the frame header.\n
                                                        KeyId length (bytes): 0 */
    gKeyIdMode1_c                   = 0x01,        /*!< Key is determined from the 1-octet Key Index subfield of the Key Identifier field of the auxiliary security header in conjunction with macDefaultKeySource.\n
                                                        KeyId length (bytes): 1 */
    gKeyIdMode2_c                   = 0x02,        /*!< Key is determined explicitly from the 4-octet Key Source subfield and the 1-octet Key Index subfield of the Key Identifier field of the auxiliary security header.\n
                                                        KeyId length (bytes): 5 */
    gKeyIdMode3_c                   = 0x03,        /*!< Key is determined explicitly from the 8-octet Key Source subfield and the 1-octet Key Index subfield of the Key Identifier field of the auxiliary security header.\n
                                                        KeyId length (bytes): 9 */
} keyIdModeType_t;

typedef uint8_t logicalChannelId_t;
/*! Logical channel values */
typedef enum {
#ifdef gPHY_802_15_4g_d
  gLogicalChannel0_c = 0,
  gLogicalChannel1_c = 1,
  gLogicalChannel2_c = 2,
  gLogicalChannel3_c = 3,
  gLogicalChannel4_c = 4,
  gLogicalChannel5_c = 5,
  gLogicalChannel6_c = 6,
  gLogicalChannel7_c = 7,
  gLogicalChannel8_c = 8,
  gLogicalChannel9_c = 9,
  gLogicalChannel10_c = 10,
#endif //gPHY_802_15_4g_d  
  gLogicalChannel11_c = 11,
  gLogicalChannel12_c = 12,
  gLogicalChannel13_c = 13,
  gLogicalChannel14_c = 14,
  gLogicalChannel15_c = 15,
  gLogicalChannel16_c = 16,
  gLogicalChannel17_c = 17,
  gLogicalChannel18_c = 18,
  gLogicalChannel19_c = 19,
  gLogicalChannel20_c = 20,
  gLogicalChannel21_c = 21,
  gLogicalChannel22_c = 22,
  gLogicalChannel23_c = 23,
  gLogicalChannel24_c = 24,
  gLogicalChannel25_c = 25,
  gLogicalChannel26_c = 26,
#ifdef gPHY_802_15_4g_d  
  gLogicalChannel27_c = 27,
  gLogicalChannel28_c = 28,
  gLogicalChannel29_c = 29,
  gLogicalChannel30_c = 30,
  gLogicalChannel31_c = 31,
  gLogicalChannel32_c = 32,
  gLogicalChannel33_c = 33,
  gLogicalChannel34_c = 34,
  gLogicalChannel35_c = 35,
  gLogicalChannel36_c = 36,
  gLogicalChannel37_c = 37,
  gLogicalChannel38_c = 38,
  gLogicalChannel39_c = 39,
  gLogicalChannel40_c = 40,
  gLogicalChannel41_c = 41,
  gLogicalChannel42_c = 42,
  gLogicalChannel43_c = 43,
  gLogicalChannel44_c = 44,
  gLogicalChannel45_c = 45,
  gLogicalChannel46_c = 46,
  gLogicalChannel47_c = 47,
  gLogicalChannel48_c = 48,
  gLogicalChannel49_c = 49,
  gLogicalChannel50_c = 50,
  gLogicalChannel51_c = 51,
  gLogicalChannel52_c = 52,
  gLogicalChannel53_c = 53,
  gLogicalChannel54_c = 54,
  gLogicalChannel55_c = 55,
  gLogicalChannel56_c = 56,
  gLogicalChannel57_c = 57,
  gLogicalChannel58_c = 58,
  gLogicalChannel59_c = 59,
  gLogicalChannel60_c = 60,
  gLogicalChannel61_c = 61,
  gLogicalChannel62_c = 62,
  gLogicalChannel63_c = 63,
  gLogicalChannel64_c = 64,
  gLogicalChannel65_c = 65,
  gLogicalChannel66_c = 66,
  gLogicalChannel67_c = 67,
  gLogicalChannel68_c = 68,
  gLogicalChannel69_c = 69,
  gLogicalChannel70_c = 70,
  gLogicalChannel71_c = 71,
  gLogicalChannel72_c = 72,
  gLogicalChannel73_c = 73,
  gLogicalChannel74_c = 74,
  gLogicalChannel75_c = 75,
  gLogicalChannel76_c = 76,
  gLogicalChannel77_c = 77,
  gLogicalChannel78_c = 78,
  gLogicalChannel79_c = 79,
  gLogicalChannel80_c = 80,
  gLogicalChannel81_c = 81,
  gLogicalChannel82_c = 82,
  gLogicalChannel83_c = 83,
  gLogicalChannel84_c = 84,
  gLogicalChannel85_c = 85,
  gLogicalChannel86_c = 86,
  gLogicalChannel87_c = 87,
  gLogicalChannel88_c = 88,
  gLogicalChannel89_c = 89,
  gLogicalChannel90_c = 90,
  gLogicalChannel91_c = 91,
  gLogicalChannel92_c = 92,
  gLogicalChannel93_c = 93,
  gLogicalChannel94_c = 94,
  gLogicalChannel95_c = 95,
  gLogicalChannel96_c = 96,
  gLogicalChannel97_c = 97,
  gLogicalChannel98_c = 98,
  gLogicalChannel99_c = 99,
  gLogicalChannel100_c = 100,
  gLogicalChannel101_c = 101,
  gLogicalChannel102_c = 102,
  gLogicalChannel103_c = 103,
  gLogicalChannel104_c = 104,
  gLogicalChannel105_c = 105,
  gLogicalChannel106_c = 106,
  gLogicalChannel107_c = 107,
  gLogicalChannel108_c = 108,
  gLogicalChannel109_c = 109,
  gLogicalChannel110_c = 110,
  gLogicalChannel111_c = 111,
  gLogicalChannel112_c = 112,
  gLogicalChannel113_c = 113,
  gLogicalChannel114_c = 114,
  gLogicalChannel115_c = 115,
  gLogicalChannel116_c = 116,
  gLogicalChannel117_c = 117,
  gLogicalChannel118_c = 118,
  gLogicalChannel119_c = 119,
  gLogicalChannel120_c = 120,
  gLogicalChannel121_c = 121,
  gLogicalChannel122_c = 122,
  gLogicalChannel123_c = 123,
  gLogicalChannel124_c = 124,
  gLogicalChannel125_c = 125,
  gLogicalChannel126_c = 126,
  gLogicalChannel127_c = 127,
  gLogicalChannel128_c = 128,
#endif /*gPHY_802_15_4g_d  */
} logicalChannelId_tag;

#ifdef gPHY_802_15_4g_d
typedef uint32_t channelMask_t[5];
#else
typedef uint32_t channelMask_t;
#endif /*gPHY_802_15_4g_d*/

/*! \brief These values can be also be used for the other 3 bytes of the channel masks
For instance {0x00, 0x00, gChannelMask00_c, 0x00} represents gChannelMask32_c */
typedef enum
{
#ifdef gPHY_802_15_4g_d
    gChannelMask00_c             = 0x00000001,  /*!< Channel 0 bit-mask */
    gChannelMask01_c             = 0x00000002,  /*!< Channel 1 bit-mask */
    gChannelMask02_c             = 0x00000004,  /*!< Channel 2 bit-mask */
    gChannelMask03_c             = 0x00000008,  /*!< Channel 3 bit-mask */
    gChannelMask04_c             = 0x00000010,  /*!< Channel 4 bit-mask */
    gChannelMask05_c             = 0x00000020,  /*!< Channel 5 bit-mask */
    gChannelMask06_c             = 0x00000040,  /*!< Channel 6 bit-mask */
    gChannelMask07_c             = 0x00000080,  /*!< Channel 7 bit-mask */
    gChannelMask08_c             = 0x00000100,  /*!< Channel 8 bit-mask */
    gChannelMask09_c             = 0x00000200,  /*!< Channel 9 bit-mask */
    gChannelMask10_c             = 0x00000400,  /*!< Channel 10 bit-mask */
#endif   
    gChannelMask11_c             = 0x00000800,  /*!< Channel 11 bit-mask */
    gChannelMask12_c             = 0x00001000,  /*!< Channel 12 bit-mask */
    gChannelMask13_c             = 0x00002000,  /*!< Channel 13 bit-mask */
    gChannelMask14_c             = 0x00004000,  /*!< Channel 14 bit-mask */
    gChannelMask15_c             = 0x00008000,  /*!< Channel 15 bit-mask */
    gChannelMask16_c             = 0x00010000,  /*!< Channel 16 bit-mask */
    gChannelMask17_c             = 0x00020000,  /*!< Channel 17 bit-mask */
    gChannelMask18_c             = 0x00040000,  /*!< Channel 18 bit-mask */
    gChannelMask19_c             = 0x00080000,  /*!< Channel 19 bit-mask */
    gChannelMask20_c             = 0x00100000,  /*!< Channel 20 bit-mask */
    gChannelMask21_c             = 0x00200000,  /*!< Channel 21 bit-mask */
    gChannelMask22_c             = 0x00400000,  /*!< Channel 22 bit-mask */
    gChannelMask23_c             = 0x00800000,  /*!< Channel 23 bit-mask */
    gChannelMask24_c             = 0x01000000,  /*!< Channel 24 bit-mask */
    gChannelMask25_c             = 0x02000000,  /*!< Channel 25 bit-mask */
    gChannelMask26_c             = 0x04000000,  /*!< Channel 26 bit-mask */
#ifdef gPHY_802_15_4g_d
    gChannelMask27_c             = 0x08000000,  /*!< Channel 27 bit-mask */
    gChannelMask28_c             = 0x10000000,  /*!< Channel 28 bit-mask */
    gChannelMask29_c             = 0x20000000,  /*!< Channel 29 bit-mask */
    gChannelMask30_c             = 0x40000000,  /*!< Channel 30 bit-mask */
    gChannelMask31_c             = 0x80000000,  /*!< Channel 31 bit-mask */
#endif
}channelMask_tag;

/*! \brief The MAC Channel Page Ids */
typedef enum{
    gChannelPageId0_c               = 0x00,        /*!< 
                                                        - Channel 0 is in 868 MHz band using BPSK
                                                        - Channels 1 to 10 are in 915 MHz band using BPSK
                                                        - Channels 11 to 26 are in 2.4 GHz band using O-QPSK*/
    gChannelPageId1_c               = 0x01,        /*!< 
                                                       - Channel 0 is in 868 MHz band using ASK
                                                       - Channels 1 to 10 are in 915 MHz band using ASK
                                                       - Channels 11 to 26 are Reserved*/
    gChannelPageId2_c               = 0x02,        /*!< 
                                                       - Channel 0 is in 868 MHz band using O-QPSK
                                                       - Channels 1 to 10 are in 915 MHz band using O-QPSK
                                                       - Channels 11 to 26 are Reserved*/
#ifdef gPHY_802_15_4g_d    
    gChannelPageId9_c               = 0x09,        /*!< 802.15.4g standard-defined SUN Phy Modes */
#endif
} channelPageId_t;

#ifdef gMAC2011_d
/* Ranging related */
/*! \cond */
typedef enum
{
    gRangingNonRanging_c,
    gRangingAllRanging_c,
    gRangingPhyHeaderOnly_c
}macRanging_t;

typedef bool_t macRangingReceived_t;
typedef enum
{
    gRangingOff_d = FALSE,
    gRangingOn_d = TRUE
}macRangingRxControl_t;

/* The pulse repetition value of the transmitted PPDU */
typedef enum
{
    gUwbPulsePrfOff,
    gUwbPulseNominal4m_c,
    gUwbPulseNominal16m_c,
    gUwbPulseNominal64m_c
}macUwbPulseRepetition_t;
/*! \endcond */
#endif

/*! \brief The MAC Capability Information \ref macCapabilityInfo_tag */
typedef uint8_t macCapabilityInfo_t;
typedef enum
{
    gCapInfoAltPanCoord_c           = 0x01,        /*!< The device is capable of becoming the PAN coordinator.*/
    gCapInfoDeviceFfd_c             = 0x02,        /*!< The device is an FFD.*/
    gCapInfoPowerMains_c            = 0x04,        /*!< The device is mains-powered, and not battery-powered.*/
    gCapInfoRxWhenIdle_c            = 0x08,        /*!< Receiver is on when unit is idle.*/
    gCapInfoSecurity_c              = 0x40,        /*!< The device can send/receive secured MAC frames.*/
    gCapInfoAllocAddr_c             = 0x80,        /*!< The device asks the coordinator to allocate a 16-bit short address as a result of the association procedure.*/
}macCapabilityInfo_tag;

typedef enum
{
    gDisassociateCoord_t            = 0x01,        /*!< The coordinator wishes the device to leave the PAN.*/
    gDisassociateDevice_t           = 0x02,        /*!< The device wishes to leave the PAN.*/
}macDisassociateReason_t;

/*! This type describes the Scan procedure supported by the MAC layer. */
typedef enum
{
    gScanModeED_c                   = 0x00,        /*!< Energy Detection scan.*/
    gScanModeActive_c               = 0x01,        /*!< Active scan.*/
    gScanModePassive_c              = 0x02,        /*!< Passive scan.*/
    gScanModeOrphan_c               = 0x03,        /*!< Orphan scan.*/
    gScanModeFastED_c               = 0x04,        /*!< Fast Energy Detect (Not in spec) */
}macScanType_t;

/*! \brief The 802.15.4 MAC PIB Id \ref macPibId_t */
typedef uint8_t pibId_t;
typedef enum
{
    /* Freescale specific MAC PIBs */
    gMPibRole_c                                 = 0x20,    /*!< The MAC role: Device/Coordinator */
    gMPibLogicalChannel_c                       = 0x21,    /*!< Logical channel number */
    gMPibTreemodeStartTime_c                    = 0x22,    /*!< Unused */
    gMPibPanIdConflictDetection_c               = 0x23,    /*!< Unused */
    gMPibBeaconResponseDenied_c                 = 0x24,    /*!< Ignore MAC Beacon Request commands */
    gMPibNBSuperFrameInterval_c                 = 0x25,    /*!< Unused */
    gMPibBeaconPayloadLengthVendor              = 0x26,    /*!< Unused */
    gMPibBeaconResponseLQIThreshold_c           = 0x27,    /*!< Unused */
    gMPibUseExtendedAddressForBeacon_c          = 0x28,    /*!< Force the use of the extended address for Beacons */
    gMPibCsmaDisableMask_c                      = 0x29,    /*!< Disable first iteration of the CSMA-CA for the specified frame types */
    gMPibAlternateExtendedAddress_c             = 0x2A,    /*!< Alternate Extended Address used for nonce creation (MAC Security) */
    gMPibMac2003Compatibility_c                 = 0x2F,    /*!< Maintain OTA compatibillity with 802.15.4/2003 */
    /* In MAC2011 this is a standard PIB, in MAC2006 it is a costant. It must be modifiable for testing. */
    gMPibExtendedAddress_c                      = 0x30,    /*!< The device's extended address (64-bit) */
    
#if gTschSupport_d
    gMPibDisconnectTime_c                       = 0x31,
    gMPibJoinPriority_c                         = 0x32,
    gMPibASN_c                                  = 0x33,    /*!< Absolute Slot Number*/
    gMPibNoHLBuffers_c                          = 0x34,
    gMPibEBSN_c                                 = 0x35,    /*!< Enhanced Beacon Sequence Number*/
    gMPibTschEnabled_c                          = 0x36,
    gMPibEBIEList_c                             = 0x37,
    gMPibTimeslotTemplate_c                     = 0x38,
    gMPibHoppingSequenceList_c                  = 0x39,
    gMPibHoppingSequenceLength_c                = 0x3A,
    gMPibTschRole_c                             = 0x3B,
#endif

    /* Standard MAC PIBs */
    gMPibAckWaitDuration_c                      = 0x40,    /*!< Number of symbols to wait for an ACK (ReadOnly) */
    gMPibAssociationPermit_c                    = 0x41,    /*!< Allow devices o associate (Coordinator only) */
    gMPibAutoRequest_c                          = 0x42,    /*!< Send automatic MAC Data Requests, and notifications for every Beacon received */
    gMPibBattLifeExt_c                          = 0x43,    /*!< Unused */
    gMPibBattLifeExtPeriods_c                   = 0x44,    /*!< Unused */
    gMPibBeaconPayload_c                        = 0x45,    /*!< Array, containind the Beacon payload */
    gMPibBeaconPayloadLength_c                  = 0x46,    /*!< Number of bytes used for Beacon payload */
    gMPibBeaconOrder_c                          = 0x47,    /*!< How often the beacon is to be transmitted. A value of 15 indicated a non-Beaconned PAN */
    gMPibBeaconTxTime_c                         = 0x48,    /*!< The time of transmission of the most recent Beacon*/
    gMPibBSN_c                                  = 0x49,    /*!< Beacon Sequence Number */
    gMPibCoordExtendedAddress_c                 = 0x4a,    /*!< The extended address of the associated Coordinator */
    gMPibCoordShortAddress_c                    = 0x4b,    /*!< The short address of the associated Coordinator */
    gMPibDSN_c                                  = 0x4c,    /*!< Data Sequence Number */
    gMPibGtsPermit_c                            = 0x4d,    /*!< Coordinator accepts GTS or not */
    gMPibMaxCSMABackoffs_c                      = 0x4e,    /*!< The maximum number of backoffs the CSMA-CA algorithm will attempt before declaring a channel access failure */
    gMPibMinBE_c                                = 0x4f,    /*!< The minimum value of the backoff exponent (BE) in the CSMA-CA algorithm */
    gMPibPanId_c                                = 0x50,    /*!< The 16-bit identifier of the PAN on which the device is operating. If this value is 0xffff, the device is not associated. */
    gMPibPromiscuousMode_c                      = 0x51,    /*!< Remove all MAC filtering. All packets are received (including ACKs) */
    gMPibRxOnWhenIdle_c                         = 0x52,    /*!< Keep the RX on, when MAC is idle */
    gMPibShortAddress_c                         = 0x53,    /*!< The 16-bit address that the device uses to communicate in the PAN. */
    gMPibSuperframeOrder_c                      = 0x54,    /*!< The length of the active portion of the superframe, including the beacon frame */
    gMPibTransactionPersistenceTime_c           = 0x55,    /*!< The storage time of a transaction (in units: aBaseSuperframeDuration for Non-Beaconned) */
    gMPibAssociatedPANCoord_c                   = 0x56,    /*!< Indication of whether the device is associated to the PAN through the PAN coordinator. */
    gMPibMaxBE_c                                = 0x57,    /*!< The maximum value of the backoff exponent, BE, in the CSMA-CA algorithm */
    gMPibMaxFrameTotalWaitTime_c                = 0x58,    /*!< Number of symbols to wait for a frame intended as a response (see 802.15.4 spec for details) */
    gMPibMaxFrameRetries_c                      = 0x59,    /*!< The maximum number of retries allowed after a transmission failure  */
    gMPibResponseWaitTime_c                     = 0x5a,    /*!< The maximum time, in multiples of aBaseSuperframeDuration, a device shall wait for a response command frame to be available following a request command frame */
    gMPibSyncSymbolOffset_c                     = 0x5b,    /*!< Offset between the frame timestamp and the first symbol after SFD */
    gMPibTimestampSupported_c                   = 0x5c,    /*!< Indicates if frame timestamp is supported (ReadOnly) */
    gMPibSecurityEnabled_c                      = 0x5d,    /*!< Indication of whether the MAC sublayer has security enabled */
    gMPibMinLIFSPeriod_c                        = 0x5e,    /*!< Value not in standard - taken from bare metal MAC */
    gMPibMinSIFSPeriod_c                        = 0x5f,    /*!< Value not in standard - taken from bare metal MAC */
#ifdef gMAC2011_d
    gMPibTxControlActiveDuration_c              = 0x60,    /*!< Unused */
    gMPibTxControlPauseDuration_c               = 0x61,    /*!< Unused */
    gMPibTxTotalDuration_c                      = 0x62,    /*!< Unused */
#endif
#if gCslSupport_d
    gMPibCslPeriod_c                            = 0x63,
    gMPibCslMaxPeriod_c                         = 0x64,
    gMPibCslFramePendingWait_c                  = 0x65,
    gMPibEnhAckWaitDuration_c                   = 0x66,
#endif
#if gRitSupport_d
    gMPibRitPeriod_c                            = 0x67,
    gMPibRitDataWaitDuration_c                  = 0x68,
    gMPibRitTxWaitDuration_c                    = 0x69,
    gMPibRitIe_c                                = 0x6A,
#endif
#ifdef gPHY_802_15_4g_d
    gMPibPhyMode_c                              = 0x6B,
    gMPibPhyCCADuration_c                       = 0x6C,
    gMPibPhyFSKScramblePSDU_c                   = 0x6D,
    gMPibPhyFrequencyBand_c                     = 0x6E,
#endif
    
    /* MAC2006 Security Related PIB Attributes */
    gMPibKeyTable_c                             = 0x71,
    gMPibKeyTableEntries_c                      = 0x72,    /*!< Freescale specific MAC 2011 */
    gMPibDeviceTable_c                          = 0x73,
    gMPibDeviceTableEntries_c                   = 0x74,    /*!< Freescale specific MAC 2011 */
    gMPibSecurityLevelTable_c                   = 0x75,
    gMPibSecurityLevelTableEntries_c            = 0x76,    /*!< Freescale specific MAC 2011 */
    gMPibFrameCounter_c                         = 0x77,
    gMPibAutoRequestSecurityLevel_c             = 0x78,    /*!< The security level used for automatic MAC requests (ex: Data Request) */
    gMPibAutoRequestKeyIdMode_c                 = 0x79,    /*!< The key Id mode used for automatic MAC requests*/
    gMPibAutoRequestKeySource_c                 = 0x7a,    /*!< The key source used for automatic MAC requests*/
    gMPibAutoRequestKeyIndex_c                  = 0x7b,    /*!< The key index used for automatic MAC requests*/
#ifndef gMAC2011_d
    gMPibDefaultKeySource_c                     = 0x7c,
    gMPibPANCoordExtendedAddress_c              = 0x7d,
    gMPibPANCoordShortAddress_c                 = 0x7e,
#endif
    /* Freescale specific MAC 2006 and MAC2011 security PIBs */
    /* KeyDescriptor */
    gMPibKeyIdLookupList_c                      = 0x7F,
    gMPibKeyIdLookupListEntries_c               = 0x80,    /*!< Freescale specific MAC 2011 */
#ifndef gMAC2011_d
    gMPibKeyDeviceList_c                        = 0x81,
    gMPibKeyDeviceListEntries_c                 = 0x82,
#else
    gMPibDeviceDescriptorHandleList_c           = 0x9C,
    gMPibDeviceDescriptorHandleListEntries_c    = 0x9D,    /*!< Freescale specific MAC 2011 */
#endif
    gMPibKeyUsageList_c                         = 0x83,
    gMPibKeyUsageListEntries_c                  = 0x84,    /*!< Freescale specific MAC 2011 */
    gMPibKey_c                                  = 0x85,
    /* KeyUsageDescriptor */
    gMPibKeyUsageFrameType_c                    = 0x86,
    gMPibKeyUsageCommnadFrameIdentifier_c       = 0x87,
#ifndef gMAC2011_d
    /* KeyDeviceDescriptor */
    gMPibKeyDeviceDescriptorHandle_c            = 0x88,
    gMPibUniqueDevice_c                         = 0x89,
    gMPibBlackListed_c                          = 0x8A,
#else
    /* DeviceDescriptorHandleList */
    gMPibDeviceDescriptorHandle_c               = 0x9E,    /*!< Freescale specific MAC 2011 */
#endif
    /* SecurityLevelDescriptor */
    gMPibSecLevFrameType_c                      = 0x8B,
    gMPibSecLevCommnadFrameIdentifier_c         = 0x8C,
    gMPibSecLevSecurityMinimum_c                = 0x8D,
    gMPibSecLevDeviceOverrideSecurityMinimum_c  = 0x8E,
#ifdef gMAC2011_d
    gMPibSecLevAllowedSecurityLevels_c          = 0x9F,
#endif
    /* DeviceDescriptor */
    gMPibDeviceDescriptorPanId_c                = 0x8F,
    gMPibDeviceDescriptorShortAddress_c         = 0x90,
    gMPibDeviceDescriptorExtAddress_c           = 0x91,
    gMPibDeviceDescriptorFrameCounter_c         = 0x92,
    gMPibDeviceDescriptorExempt                 = 0x93,
    /* KeyIdLookupDescriptor */
#ifndef gMAC2011_d
    gMPibKeyIdLookupData_c                      = 0x94,
    gMPibKeyIdLookupDataSize_c                  = 0x95,
#else
    gMPibKeyIdLookupKeyIdMode_c                 = 0xA0,
    gMPibKeyIdLookupKeySource_c                 = 0xA1,
    gMPibKeyIdLookupKeyIndex_c                  = 0xA2,
    gMPibKeyIdLookupDeviceAddressMode_c         = 0xA3,
    gMPibKeyIdLookupDevicePANId_c               = 0xA4,
    gMPibKeyIdLookupDeviceAddress_c             = 0xA5,
#endif
    gMPibiKeyTableCrtEntry_c                    = 0x96,
    gMPibiDeviceTableCrtEntry_c                 = 0x97,
    gMPibiSecurityLevelTableCrtEntry_c          = 0x98,
    gMPibiKeyIdLookuplistCrtEntry_c             = 0x99,
#ifndef gMAC2011_d
    gMPibiKeyDeviceListCrtEntry_c               = 0x9B,
#else
    gMPibiDeviceDescriptorHandleListCrtEntry_c  = 0XA6,
#endif
    gMPibiKeyUsageListCrtEntry_c                = 0x9A,
}macPibId_t;

/*! \brief The superframe specification, as specified in the received beacon frame. */
typedef struct macSuperframeSpec_tag
{
    uint16_t    beaconOrder             :4;        /*!< Specifies the transmission interval of the beacon*/
    uint16_t    superframeOrder         :4;        /*!< Specify the length of time during which the superframe is active (i.e., receiver enabled), including the beacon frame transmission time.*/
    uint16_t    finalCapSlot            :4;        /*!< Specifies the final superframe slot utilized by the CAP.*/
    uint16_t    ble                     :1;        /*!< This field is set to one if frames transmitted to the beaconing device during its CAP are required to start in or before macBattLifeExtPeriods full backoff periods after the IFS period following the beacon. Otherwise, the BLE subfield shall be set to zero.*/
    uint16_t    reserved                :1;
    uint16_t    panCoordinator          :1;        /*!< Field set to one if the beacon frame is being transmitted by the PAN coordinator. Otherwise, the PAN Coordinator subfield shall be set to zero.*/
    uint16_t    associationPermit       :1;        /*!< The field is set to one if macAssociationPermit is set to TRUE (i.e., the coordinator is accepting association to the PAN). The association permit bit shall be set to zero if the coordinator is currently not accepting association requests on its network.*/
}macSuperframeSpec_t;

typedef struct gtsCharacteristics_tag
{
    uint8_t     gtsLength               :4;        /*!< The number of superframe slots that formes the GTS */
    uint8_t     gtsDirection            :1;        /*!< 1 = RX slot, 0 = TX slot */
    uint8_t     characteristicsType     :1;        /*!< 1 = Allocate GTS, 0 = Deallocate GTS */
    uint8_t     reserved                :2;
}gtsCharacteristics_t;

/* \brief This structure is packed to map directly on upper layer structures */
typedef PACKED_STRUCT panDescriptor_tag
{
    uint64_t            coordAddress;              /*!< The address of the coordinator as specified in the received beacon frame.*/
    uint16_t            coordPanId;                /*!< PAN identifier of the coordinator as specified in the received beacon frame.*/
    addrModeType_t      coordAddrMode;             /*!< The coordinator addressing mode corresponding to the received beacon frame. */
    logicalChannelId_t  logicalChannel;            /*!< The current logical channel occupied by the network. */
#if 0
    channelPageId_t     channelPage;               /*!< The current channel page occupied by the network.*/
#endif
#ifdef gMAC2011_d
    resultType_t        securityStatus;            /*!< gSuccess_c if there was no error in the security processing of the frame. One of the other status codes indicating an error in the security processing otherwise.*/
#else
    resultType_t        securityFailure;           /*!< gSuccess_c if there was no error in the security processing of the frame. One of the other status codes indicating an error in the security processing otherwise.*/
#endif
    macSuperframeSpec_t superframeSpec;            /*!< The superframe specification as specified in the received beacon frame.*/
    bool_t              gtsPermit;                 /*!< TRUE if the beacon is from the PAN coordinator that is accepting GTS requests.*/
    uint8_t             linkQuality;               /*!< The LQI at which the network beacon was received. Lower values represent lower LQI.*/
    uint8_t             timeStamp[3];              /*!< The time, in symbols, at which the data were transmitted. This is a 24-bit value.*/
    macSecurityLevel_t  securityLevel;             /*!< Indicates the security level to be used. */
    keyIdModeType_t     keyIdMode;                 /*!< This parameter is ignored if the securityLevel parameter is set to gMacSecurityNone_c. */
    uint64_t            keySource;                 /*!< Indicates the originator of the key to be used. Ignored if the keyIdMode parameter is ignored or set to gKeyIdMode0_c*/
    uint8_t             keyIndex;                  /*!< Indicates the index of the key to be used. Ignored if the keyIdMode parameter is ignored or set to gKeyIdMode0_c*/
#if 0
#ifdef gMAC2011_d
    uint8_t*            pCodeList;
#endif
#endif
} panDescriptor_t;

/* MAC2006 Security Tables type definitions */

typedef struct keyIdLookupDescriptor_tag
{
#ifndef gMAC2011_d
    uint8_t                     lookupData[9];  /*!< Data used to identify the key. */
    uint8_t                     lookupDataSize; /*!< A value of 0x00 indicates a set of 5 octets; a value of 0x01 indicates a set of 9 octets. */
#else
    uint64_t                    keySource;
    uint64_t                    deviceAddress;
    uint16_t                    devicePANId;
    keyIdModeType_t             keyIdMode;
    uint8_t                     keyIndex;
    addrModeType_t              deviceAddrMode;
#endif
}keyIdLookupDescriptor_t;

typedef struct deviceAddrDescriptor_tag
{
    uint64_t                    extAddress;   /*!< The 64-bit IEEE extended address of the device in this DeviceDescriptor. 
                                                   This element is also used in unsecuring operations on incoming frames. */
    uint16_t                    PANId;        /*!< The 16-bit PAN identifier of the device in this DeviceDescriptor. */
    uint16_t                    shortAddress; /*!< The 16-bit short address of the device in this DeviceDescriptor.
                                                   A value of 0xfffe indicates that this device is using only its extended address.
                                                   A value of 0xffff indicates that this value is unknown. */
    uint8_t                     usageCnt;     /*!< Internal use only */
}deviceAddrDescriptor_t;

typedef struct deviceDescriptor_tag
{
    uint32_t                    frameCounter; /*!< The incoming frame counter of the device in this DeviceDescriptor.  */
    bool_t                      exempt;       /*!< Indication of whether the device may override the minimum security level settings */
    uint8_t                     deviceAddrDescriptorHandle; /*!< Internal use only */
}deviceDescriptor_t;

#ifndef gMAC2011_d
typedef struct keyDeviceDescriptor_tag
{
    uint8_t                     deviceDescriptorHandle; /*!< Internal use only */
    bool_t                      uniqueDevice; /*!< Indication of whether the device indicated by DeviceDescriptorHandle is uniquely associated with the KeyDescriptor, i.e., it is a link key as opposed to a group key. */
    bool_t                      blackListed;  /*!< Indication of whether the device indicated by DeviceDescriptorHandle previously communicated with this key prior to the exhaustion of the frame counter. 
                                                   If TRUE, this indicates that the device shall not use this key further because it exhausted its use of the frame counter used with this key. */
}keyDeviceDescriptor_t;
#endif

typedef struct keyUsageDescriptor_tag
{
    uint8_t                     frameType;
    uint8_t                     commandFrameIdentifier;
}keyUsageDescriptor_t;

typedef struct keyDescriptor_tag
{
    keyIdLookupDescriptor_t*    keyIdLookupList; /*!< A list of KeyIdLookupDescriptor entries used to identify this KeyDescriptor. */
    keyUsageDescriptor_t*       keyUsageList;    /*!< A list of KeyUsageDescriptor entries indicating which frame types this key may be used with. */
#ifndef gMAC2011_d
    keyDeviceDescriptor_t*      keyDeviceList;   /*!< A list of KeyDeviceDescriptor entries indicating which devices are currently using this key, including their blacklist status. */
    uint8_t                     keyDeviceListEntries; /*!< The number of entries in KeyDeviceList. */
#else
    uint8_t*                    deviceDescriptorHandleList;
    uint8_t                     deviceDescriptorHandleListEntries;
#endif
    uint8_t                     keyIdLookupListEntries; /*!< The number of entries in KeyIdLookupList. */
    uint8_t                     keyUsageListEntries;    /*!< The number of entries in KeyUsageList. */
    uint8_t                     key[16];                /*!< The actual value of the key. */
}keyDescriptor_t;

typedef struct securityLevelDescriptor_tag
{
    uint8_t                     frameType;
    uint8_t                     commandFrameIdentifier;
    uint8_t                     securityMinimum; /*!< The minimal required/expected security level for incoming MAC frames with the indicated frame type and, if present, command frame type */
    bool_t                      deviceOverrideSecurityMinimum; /*!< Indication of whether originating devices for which the Exempt flag is set may override the minimum security level indicated by the SecurityMinimum element. 
                                                                    If TRUE, this indicates that for originating devices with Exempt status, the incoming security level zero is acceptable, in addition to the incoming security levels meeting the minimum expected security level indicated by the SecurityMinimum element. */
#ifdef gMAC2011_d
    macSecurityLevel_t          allowedSecurityLevels[8];
#endif
}securityLevelDescriptor_t;

#if gCslSupport_d
typedef struct macCslEntry_tag
{
    uint64_t           nextSample; /*!< Absolute time for the node channel sample. */
    uint16_t           checksum;   /*!< Checksum of the addressing information of the node. */
    uint16_t           period;     /*!< Interval between node channel samples. */
    bool_t             inUse;      /*!< Entry is currently active. */
}macCslEntry_t;
#endif

#if gRitSupport_d
/*! This type describes the LE RIT Information element. */
typedef PACKED_STRUCT macRitIe_tag
{
    uint8_t  T0; /*!< Time to first listen interval. */
    uint8_t  N;  /*!< Number of repeat listen. */
    uint16_t T;  /*!< Repeat listen interval. */
}macRitIe_t;

typedef struct macRitEntry_tag
{
    uint64_t           lastTimestamp; /*!< Absolute time for the last RIT Data Request of the node. */
    macRitIe_t         ritIe;         /*!< Listening schedule of the node, if any. */
    uint16_t           checksum;      /*!< Checksum of the addressing information of the node. */
    bool_t             inUse;         /*!< Entry is currently active. */
}macRitEntry_t;
#endif

/*! This type describes the Beacon frame types. */
typedef enum
{
    gMacBeacon_c         = 0x00,
    gMacEnhancedBeacon_c = 0x01,
}beaconType_t;

/*! This type describes the Payload Information Element IDs that can be set in the EB IE List PIB to be included in the Enhanced Beacon frame. */
typedef enum
{
    gMacPayloadIeIdChannelHoppingSequence_c = 0x09,
    gMacPayloadIeIdTschSynchronization_c    = 0x1a,
    gMacPayloadIeIdTschSlotframeAndLink_c   = 0x1b,
    gMacPayloadIeIdTschTimeslot_c           = 0x1c,
}macPayloadIeId_t;

/*! This type describes the TSCH Link types. */
typedef enum
{
    gMacLinkTypeNormal_c       = 0x00, /*!< Normal Link type */
    gMacLinkTypeAdvertising_c  = 0x01, /*!< Advertising Link type that can be used for sending the Enhanced Beacon frames */
}macLinkType_t;

/*! This type describes the TSCH Set Slotframe operation types. */
typedef enum
{
    gMacSetSlotframeOpAdd_c    = 0x00,
    gMacSetSlotframeOpDelete_c = 0x02,
    gMacSetSlotframeOpModify_c = 0x03,
}macSetSlotframeOp_t;

/*! This type describes the TSCH Set Link operation types. */
typedef enum
{
    gMacSetLinkOpAdd_c         = 0x00,
    gMacSetLinkOpDelete_c      = 0x01,
    gMacSetLinkOpModify_c      = 0x02,
}macSetLinkOp_t;

/*! This type describes the TSCH Mode operation types. */
typedef enum
{
    gMacTschModeOff_c          = 0x00,
    gMacTschModeOn_c           = 0x01,
}macTschMode_t;

/*! This type describes the TSCH Role */
typedef enum
{
    gMacTschRolePANCoordinator_c  = 0x00, /*!< Node is the Coordinator of the TSCH PAN */
    gMacTschRoleDevice_c          = 0x01, /*!< Node joins a TSCH PAN */
}macTschRole_t;

/*! This type describes the list of Information Element IDs that can be included in an Enhanced Beacon */
typedef macPayloadIeId_t macEBIEList_t[4];

typedef struct macLinkOptions_tag
{
    uint8_t tx:1;          /*!< Tx Link  */
    uint8_t rx:1;          /*!< Rx Link */
    uint8_t shared:1;      /*!< Shared Link (only for Tx) */
    uint8_t timekeeping:1; /*!< Timekeeping (only for Rx) */
    uint8_t reserved:4;
}macLinkOptions_t;

typedef PACKED_STRUCT macSlotframeIe_tag
{
    uint8_t  macSlotframeHandle; /*!< Unique identifier of the slotframe */
    uint16_t macSlotframeSize;   /*!< Slotframe size in timeslots */
}macSlotframeIe_t;

typedef PACKED_STRUCT macTschLinkIe_tag
{
    uint16_t         timeslot;       /*!< Timeslot of the link inside the slotframe */
    uint16_t         channelOffset;  /*!< Channel offset used for computing channel */
    macLinkOptions_t macLinkOptions; /*!< Link options */
}macTschLinkIe_t;

typedef struct macSlotframe_tag
{
    macSlotframeIe_t macSlotframeIe; /*!< Slotframe IE as included in EB */
    bool_t           inUse;          /*!< Slotframe is currently active */
}macSlotframe_t;

typedef struct macLink_tag
{
    uint16_t         macLinkHandle;   /*!< Unique identifier of Link */
    uint16_t         macNodeAddress;  /*!< Node address of the Link */
    macLinkType_t    macLinkType;     /*!< Link type normal or advertising */
    uint8_t          slotframeHandle; /*!< Slotframe identifier for the Link */
    macTschLinkIe_t  macLinkIe;       /*!< Link IE as included in EB */
    bool_t           inUse;           /*!< Link is currently active */
}macLink_t;

typedef PACKED_STRUCT macTimeslotTemplate_tag
{
    uint8_t  macTimeslotTemplateId; /*!< Identifier of Timeslot Template */
    uint16_t macTsCCAOffset;        /*!< The time between the beginning of timeslot and start of CCA operation, in µs */
    uint16_t macTsCCA;              /*!< Duration of CCA, in µs */
    uint16_t macTsTxOffset;         /*!< The time between the beginning of the timeslot and the start of frame transmission, in µs */
    uint16_t macTsRxOffset;         /*!< Beginning of the timeslot to when the receiver shall be listening, in µs */
    uint16_t macTsRxAckDelay;       /*!< End of frame to when the transmitter shall listen for Acknowledgment, in µs */
    uint16_t macTsTxAckDelay;       /*!< End of frame to start of Acknowledgment, in µs */
    uint16_t macTsRxWait;           /*!< The time to wait for start of frame, in µs */
    uint16_t macTsAckWait;          /*!< The minimum time to wait for start of an Acknowledgment, in µs */
    uint16_t macTsRxTx;             /*!< Transmit to Receive turnaround, in µs */
    uint16_t macTsMaxAck;           /*!< Transmission time to send Acknowledgment, in µs */
    uint16_t macTsMaxTx;            /*!< Transmission time to send the maximum length frame, in µs */
    uint16_t macTsTimeslotLength;   /*!< The total length of the timeslot including any unused time after frame transmission and Acknowledgment, in µs */
}macTimeslotTemplate_t;

typedef PACKED_STRUCT macHoppingSequenceIe_tag
{
    uint8_t  macHoppingSequenceId; /*!< Unique identifier of Hopping Sequence */
    uint8_t  macChannelPage;       /*!< Channel page in use */
    uint16_t macNumberOfChannels;  /*!< Maximum number of channels supported by current PHY mode */
    uint32_t macPhyConfiguration;  /*!< PHY configuration, channel list is in the channel bitmap */
#if 0
    macExtendedBitmap;
    uint16_t macHoppingSequenceLength;
    macHoppingSequenceList;
    uint16_t macCurrentHop;
#endif
}macHoppingSequenceIe_t;

typedef PACKED_STRUCT macTschSynchronizationIe_tag
{
    uint8_t macASN[5];       /*!< TSCH Absolute Sequence Number */
    uint8_t macJoinPriority; /*!< The lowest join priority from the TSCH Synchronization IE in an
                                  Enhanced beacon, when the device joined the network + 1. */
}macTschSynchronizationIe_t;

typedef PACKED_STRUCT macTschSlotframeAndLinkIe_tag
{
    uint8_t numberOfSlotframes; /*!< Number of slotframes included in the IE */
#if 0
    macTschSlotframeIe_t * numberOfSlotframes
#endif
}macTschSlotframeAndLinkIe_t;

typedef PACKED_STRUCT macTschSlotframeIe_tag
{
    macSlotframeIe_t slotframe;     /*!< Slotframe IE */
    uint8_t          numberOfLinks; /*!< Number of links included in the IE */
#if 0
    macTschLinkIe_t * numberOfLinks;
#endif
}macTschSlotframeIe_t;

typedef PACKED_STRUCT macAckTimeCorrectionIe_tag
{
    uint16_t timeSyncInfo; /*!< TSCH Time synchronization as included in the ACK / NACK IE */
}macAckTimeCorrectionIe_t;

typedef struct macTschNeighbor_tag
{
    uint16_t nodeAddr;             /*!< Node address of the clock source */
    uint16_t keepAlivePeriod;      /*!< Keep alive period set for the clock source */
    uint16_t cntTsSinceLastPacket; /*!< Timeslot counter since last exchanged packet */
}macTschNeighbor_t;

/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/


/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

#endif  /* _MAC_TYPES_H */

/*! *********************************************************************************
* @}
********************************************************************************** */
