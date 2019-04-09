/******************************************************************************
*  Filename:       rf_ieee_cmd.h
*  Revised:        2018-01-15 06:15:14 +0100 (Mon, 15 Jan 2018)
*  Revision:       18170
*
*  Description:    CC13x2/CC26x2 API for IEEE 802.15.4 commands
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef __IEEE_CMD_H
#define __IEEE_CMD_H

#ifndef __RFC_STRUCT
#define __RFC_STRUCT
#endif

#ifndef __RFC_STRUCT_ATTR
#if defined(__GNUC__)
#define __RFC_STRUCT_ATTR __attribute__ ((aligned (4)))
#elif defined(__TI_ARM__)
#define __RFC_STRUCT_ATTR __attribute__ ((__packed__,aligned (4)))
#else
#define __RFC_STRUCT_ATTR
#endif
#endif

//! \addtogroup rfc
//! @{

//! \addtogroup ieee_cmd
//! @{

#include <stdint.h>
#include "rf_mailbox.h"
#include "rf_common_cmd.h"

typedef struct __RFC_STRUCT rfc_CMD_IEEE_RX_s rfc_CMD_IEEE_RX_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_ED_SCAN_s rfc_CMD_IEEE_ED_SCAN_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_TX_s rfc_CMD_IEEE_TX_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_CSMA_s rfc_CMD_IEEE_CSMA_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_RX_ACK_s rfc_CMD_IEEE_RX_ACK_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_ABORT_BG_s rfc_CMD_IEEE_ABORT_BG_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_MOD_CCA_s rfc_CMD_IEEE_MOD_CCA_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_MOD_FILT_s rfc_CMD_IEEE_MOD_FILT_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_MOD_SRC_MATCH_s rfc_CMD_IEEE_MOD_SRC_MATCH_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_ABORT_FG_s rfc_CMD_IEEE_ABORT_FG_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_STOP_FG_s rfc_CMD_IEEE_STOP_FG_t;
typedef struct __RFC_STRUCT rfc_CMD_IEEE_CCA_REQ_s rfc_CMD_IEEE_CCA_REQ_t;
typedef struct __RFC_STRUCT rfc_ieeeRxOutput_s rfc_ieeeRxOutput_t;
typedef struct __RFC_STRUCT rfc_shortAddrEntry_s rfc_shortAddrEntry_t;
typedef struct __RFC_STRUCT rfc_ieeeRxCorrCrc_s rfc_ieeeRxCorrCrc_t;

//! \addtogroup CMD_IEEE_RX
//! @{
#define CMD_IEEE_RX                                             0x2801
//! IEEE 802.15.4 Receive Command
struct __RFC_STRUCT rfc_CMD_IEEE_RX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2801
   uint16_t status;                     //!< \brief An integer telling the status of the command. This value is
                                        //!<        updated by the radio CPU during operation and may be read by the
                                        //!<        system CPU at any time.
   rfc_radioOp_t *pNextOp;              //!<        Pointer to the next operation to run after this operation is done
   ratmr_t startTime;                   //!<        Absolute or relative start time (depending on the value of <code>startTrigger</code>)
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } startTrigger;                      //!<        Identification of the trigger that starts the operation
   struct {
      uint8_t rule:4;                   //!<        Condition for running next command: Rule for how to proceed
      uint8_t nSkip:4;                  //!<        Number of skips + 1 if the rule involves skipping. 0: same, 1: next, 2: skip next, ...
   } condition;
   uint8_t channel;                     //!< \brief Channel to tune to in the start of the operation<br>
                                        //!<        0: Use existing channel<br>
                                        //!<        11--26: Use as IEEE 802.15.4 channel, i.e. frequency is (2405 + 5 &times; (channel - 11)) MHz<br>
                                        //!<        60--207: Frequency is  (2300 + channel) MHz<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t bAutoFlushCrc:1;          //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushIgn:1;          //!<        If 1, automatically remove packets that can be ignored according to frame filtering from Rx queue
      uint8_t bIncludePhyHdr:1;         //!<        If 1, include the received PHY header field in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendCorrCrc:1;         //!<        If 1, append a correlation value and CRC result byte to the packet in the Rx queue
      uint8_t bAppendSrcInd:1;          //!<        If 1, append an index from the source matching algorithm
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   rfc_ieeeRxOutput_t *pOutput;         //!<        Pointer to output structure (NULL: Do not store results)
   struct {
      uint16_t frameFiltEn:1;           //!< \brief 0: Disable frame filtering<br>
                                        //!<        1: Enable frame filtering
      uint16_t frameFiltStop:1;         //!< \brief 0: Receive all packets to the end<br>
                                        //!<        1: Stop receiving frame once frame filtering has caused the frame to be rejected.
      uint16_t autoAckEn:1;             //!< \brief 0: Disable auto ACK<br>
                                        //!<        1: Enable auto ACK.
      uint16_t slottedAckEn:1;          //!< \brief 0: Non-slotted ACK<br>
                                        //!<        1: Slotted ACK.
      uint16_t autoPendEn:1;            //!< \brief 0: Auto-pend disabled<br>
                                        //!<        1: Auto-pend enabled
      uint16_t defaultPend:1;           //!<        The value of the pending data bit in auto ACK packets that are not subject to auto-pend
      uint16_t bPendDataReqOnly:1;      //!< \brief 0: Use auto-pend for any packet<br>
                                        //!<        1: Use auto-pend for data request packets only
      uint16_t bPanCoord:1;             //!< \brief 0: Device is not PAN coordinator<br>
                                        //!<        1: Device is PAN coordinator
      uint16_t maxFrameVersion:2;       //!<        Reject frames where the frame version field in the FCF is greater than this value
      uint16_t fcfReservedMask:3;       //!<        Value to be AND-ed with the reserved part of the FCF; frame rejected if result is non-zero
      uint16_t modifyFtFilter:2;        //!< \brief Treatment of MSB of frame type field before frame-type filtering:<br>
                                        //!<        0: No modification<br>
                                        //!<        1: Invert MSB<br>
                                        //!<        2: Set MSB to 0<br>
                                        //!<        3: Set MSB to 1
      uint16_t bStrictLenFilter:1;      //!< \brief 0: Accept acknowledgement frames of any length >= 5<br>
                                        //!<        1: Accept only acknowledgement frames of length 5
   } frameFiltOpt;                      //!<        Frame filtering options
   struct {
      uint8_t bAcceptFt0Beacon:1;       //!< \brief Treatment of frames with frame type 000 (beacon):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt1Data:1;         //!< \brief Treatment of frames with frame type 001 (data):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt2Ack:1;          //!< \brief Treatment of frames with frame type 010 (ACK):<br>
                                        //!<        0: Reject, unless running ACK receive command<br>
                                        //!<        1: Always accept
      uint8_t bAcceptFt3MacCmd:1;       //!< \brief Treatment of frames with frame type 011 (MAC command):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt4Reserved:1;     //!< \brief Treatment of frames with frame type 100 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt5Reserved:1;     //!< \brief Treatment of frames with frame type 101 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt6Reserved:1;     //!< \brief Treatment of frames with frame type 110 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt7Reserved:1;     //!< \brief Treatment of frames with frame type 111 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
   } frameTypes;                        //!<        Frame types to receive in frame filtering
   struct {
      uint8_t ccaEnEnergy:1;            //!<        Enable energy scan as CCA source
      uint8_t ccaEnCorr:1;              //!<        Enable correlator based carrier sense as CCA source
      uint8_t ccaEnSync:1;              //!<        Enable sync found based carrier sense as CCA source
      uint8_t ccaCorrOp:1;              //!< \brief Operator to use between energy based and correlator based CCA<br>
                                        //!<        0: Report busy channel if either ccaEnergy or ccaCorr are busy<br>
                                        //!<        1: Report busy channel if both ccaEnergy and ccaCorr are busy
      uint8_t ccaSyncOp:1;              //!< \brief Operator to use between sync found based CCA and the others<br>
                                        //!<        0: Always report busy channel if ccaSync is busy<br>
                                        //!<        1: Always report idle channel if ccaSync is idle
      uint8_t ccaCorrThr:2;             //!<        Threshold for number of correlation peaks in correlator based carrier sense
   } ccaOpt;                            //!<        CCA options
   int8_t ccaRssiThr;                   //!<        RSSI threshold for CCA
   uint8_t __dummy0;
   uint8_t numExtEntries;               //!<        Number of extended address entries
   uint8_t numShortEntries;             //!<        Number of short address entries
   uint32_t* pExtEntryList;             //!<        Pointer to list of extended address entries
   uint32_t* pShortEntryList;           //!<        Pointer to list of short address entries
   uint64_t localExtAddr;               //!<        The extended address of the local device
   uint16_t localShortAddr;             //!<        The short address of the local device
   uint16_t localPanID;                 //!<        The PAN ID of the local device
   uint16_t __dummy1;
   uint8_t __dummy2;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the Rx operation
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the Rx
                                        //!<        operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_ED_SCAN
//! @{
#define CMD_IEEE_ED_SCAN                                        0x2802
//! IEEE 802.15.4 Energy Detect Scan Command
struct __RFC_STRUCT rfc_CMD_IEEE_ED_SCAN_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2802
   uint16_t status;                     //!< \brief An integer telling the status of the command. This value is
                                        //!<        updated by the radio CPU during operation and may be read by the
                                        //!<        system CPU at any time.
   rfc_radioOp_t *pNextOp;              //!<        Pointer to the next operation to run after this operation is done
   ratmr_t startTime;                   //!<        Absolute or relative start time (depending on the value of <code>startTrigger</code>)
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } startTrigger;                      //!<        Identification of the trigger that starts the operation
   struct {
      uint8_t rule:4;                   //!<        Condition for running next command: Rule for how to proceed
      uint8_t nSkip:4;                  //!<        Number of skips + 1 if the rule involves skipping. 0: same, 1: next, 2: skip next, ...
   } condition;
   uint8_t channel;                     //!< \brief Channel to tune to in the start of the operation<br>
                                        //!<        0: Use existing channel<br>
                                        //!<        11--26: Use as IEEE 802.15.4 channel, i.e. frequency is (2405 + 5 &times; (channel - 11)) MHz<br>
                                        //!<        60--207: Frequency is  (2300 + channel) MHz<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t ccaEnEnergy:1;            //!<        Enable energy scan as CCA source
      uint8_t ccaEnCorr:1;              //!<        Enable correlator based carrier sense as CCA source
      uint8_t ccaEnSync:1;              //!<        Enable sync found based carrier sense as CCA source
      uint8_t ccaCorrOp:1;              //!< \brief Operator to use between energy based and correlator based CCA<br>
                                        //!<        0: Report busy channel if either ccaEnergy or ccaCorr are busy<br>
                                        //!<        1: Report busy channel if both ccaEnergy and ccaCorr are busy
      uint8_t ccaSyncOp:1;              //!< \brief Operator to use between sync found based CCA and the others<br>
                                        //!<        0: Always report busy channel if ccaSync is busy<br>
                                        //!<        1: Always report idle channel if ccaSync is idle
      uint8_t ccaCorrThr:2;             //!<        Threshold for number of correlation peaks in correlator based carrier sense
   } ccaOpt;                            //!<        CCA options
   int8_t ccaRssiThr;                   //!<        RSSI threshold for CCA
   uint8_t __dummy0;
   int8_t maxRssi;                      //!<        The maximum RSSI recorded during the ED scan
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the Rx operation
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the Rx
                                        //!<        operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_TX
//! @{
#define CMD_IEEE_TX                                             0x2C01
//! IEEE 802.15.4 Transmit Command
struct __RFC_STRUCT rfc_CMD_IEEE_TX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2C01
   uint16_t status;                     //!< \brief An integer telling the status of the command. This value is
                                        //!<        updated by the radio CPU during operation and may be read by the
                                        //!<        system CPU at any time.
   rfc_radioOp_t *pNextOp;              //!<        Pointer to the next operation to run after this operation is done
   ratmr_t startTime;                   //!<        Absolute or relative start time (depending on the value of <code>startTrigger</code>)
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } startTrigger;                      //!<        Identification of the trigger that starts the operation
   struct {
      uint8_t rule:4;                   //!<        Condition for running next command: Rule for how to proceed
      uint8_t nSkip:4;                  //!<        Number of skips + 1 if the rule involves skipping. 0: same, 1: next, 2: skip next, ...
   } condition;
   struct {
      uint8_t bIncludePhyHdr:1;         //!< \brief 0: Find PHY header automatically<br>
                                        //!<        1: Insert PHY header from the buffer
      uint8_t bIncludeCrc:1;            //!< \brief 0: Append automatically calculated CRC<br>
                                        //!<        1: Insert FCS (CRC) from the buffer
      uint8_t :1;
      uint8_t payloadLenMsb:5;          //!< \brief Most significant bits of payload length. Should only be non-zero to create long
                                        //!<        non-standard packets for test purposes
   } txOpt;
   uint8_t payloadLen;                  //!<        Number of bytes in the payload
   uint8_t* pPayload;                   //!<        Pointer to payload buffer of size <code>payloadLen</code>
   ratmr_t timeStamp;                   //!<        Time stamp of transmitted frame
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_CSMA
//! @{
#define CMD_IEEE_CSMA                                           0x2C02
//! IEEE 802.15.4 CSMA-CA Command
struct __RFC_STRUCT rfc_CMD_IEEE_CSMA_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2C02
   uint16_t status;                     //!< \brief An integer telling the status of the command. This value is
                                        //!<        updated by the radio CPU during operation and may be read by the
                                        //!<        system CPU at any time.
   rfc_radioOp_t *pNextOp;              //!<        Pointer to the next operation to run after this operation is done
   ratmr_t startTime;                   //!<        Absolute or relative start time (depending on the value of <code>startTrigger</code>)
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } startTrigger;                      //!<        Identification of the trigger that starts the operation
   struct {
      uint8_t rule:4;                   //!<        Condition for running next command: Rule for how to proceed
      uint8_t nSkip:4;                  //!<        Number of skips + 1 if the rule involves skipping. 0: same, 1: next, 2: skip next, ...
   } condition;
   uint16_t randomState;                //!<        The state of the pseudo-random generator
   uint8_t macMaxBE;                    //!<        The IEEE 802.15.4 MAC parameter <i>macMaxBE</i>
   uint8_t macMaxCSMABackoffs;          //!<        The IEEE 802.15.4 MAC parameter <i>macMaxCSMABackoffs</i>
   struct {
      uint8_t initCW:5;                 //!<        The initialization value for the CW parameter
      uint8_t bSlotted:1;               //!< \brief 0:  non-slotted CSMA<br>
                                        //!<        1: slotted CSMA
      uint8_t rxOffMode:2;              //!< \brief 0: RX stays on during CSMA backoffs<br>
                                        //!<        1: The CSMA-CA algorithm will suspend the receiver if no frame is being received<br>
                                        //!<        2: The CSMA-CA algorithm will suspend the receiver if no frame is being received,
                                        //!<        or after finishing it (including auto ACK) otherwise<br>
                                        //!<        3: The CSMA-CA algorithm will suspend the receiver immediately during back-offs
   } csmaConfig;
   uint8_t NB;                          //!<        The NB parameter from the IEEE 802.15.4 CSMA-CA algorithm
   uint8_t BE;                          //!<        The BE parameter from the IEEE 802.15.4 CSMA-CA algorithm
   uint8_t remainingPeriods;            //!<        The number of remaining periods from a paused backoff countdown
   int8_t lastRssi;                     //!<        RSSI measured at the last CCA operation
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the CSMA-CA operation
   ratmr_t lastTimeStamp;               //!<        Time of the last CCA operation
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        CSMA-CA operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_RX_ACK
//! @{
#define CMD_IEEE_RX_ACK                                         0x2C03
//! IEEE 802.15.4 Receive Acknowledgement Command
struct __RFC_STRUCT rfc_CMD_IEEE_RX_ACK_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2C03
   uint16_t status;                     //!< \brief An integer telling the status of the command. This value is
                                        //!<        updated by the radio CPU during operation and may be read by the
                                        //!<        system CPU at any time.
   rfc_radioOp_t *pNextOp;              //!<        Pointer to the next operation to run after this operation is done
   ratmr_t startTime;                   //!<        Absolute or relative start time (depending on the value of <code>startTrigger</code>)
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } startTrigger;                      //!<        Identification of the trigger that starts the operation
   struct {
      uint8_t rule:4;                   //!<        Condition for running next command: Rule for how to proceed
      uint8_t nSkip:4;                  //!<        Number of skips + 1 if the rule involves skipping. 0: same, 1: next, 2: skip next, ...
   } condition;
   uint8_t seqNo;                       //!<        Sequence number to expect
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to give up acknowledgement reception
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to give up
                                        //!<        acknowledgement reception
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_ABORT_BG
//! @{
#define CMD_IEEE_ABORT_BG                                       0x2C04
//! IEEE 802.15.4 Abort Background Level Command
struct __RFC_STRUCT rfc_CMD_IEEE_ABORT_BG_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2C04
   uint16_t status;                     //!< \brief An integer telling the status of the command. This value is
                                        //!<        updated by the radio CPU during operation and may be read by the
                                        //!<        system CPU at any time.
   rfc_radioOp_t *pNextOp;              //!<        Pointer to the next operation to run after this operation is done
   ratmr_t startTime;                   //!<        Absolute or relative start time (depending on the value of <code>startTrigger</code>)
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } startTrigger;                      //!<        Identification of the trigger that starts the operation
   struct {
      uint8_t rule:4;                   //!<        Condition for running next command: Rule for how to proceed
      uint8_t nSkip:4;                  //!<        Number of skips + 1 if the rule involves skipping. 0: same, 1: next, 2: skip next, ...
   } condition;
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_MOD_CCA
//! @{
#define CMD_IEEE_MOD_CCA                                        0x2001
//! IEEE 802.15.4 Modify CCA Parameter Command
struct __RFC_STRUCT rfc_CMD_IEEE_MOD_CCA_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2001
   struct {
      uint8_t ccaEnEnergy:1;            //!<        Enable energy scan as CCA source
      uint8_t ccaEnCorr:1;              //!<        Enable correlator based carrier sense as CCA source
      uint8_t ccaEnSync:1;              //!<        Enable sync found based carrier sense as CCA source
      uint8_t ccaCorrOp:1;              //!< \brief Operator to use between energy based and correlator based CCA<br>
                                        //!<        0: Report busy channel if either ccaEnergy or ccaCorr are busy<br>
                                        //!<        1: Report busy channel if both ccaEnergy and ccaCorr are busy
      uint8_t ccaSyncOp:1;              //!< \brief Operator to use between sync found based CCA and the others<br>
                                        //!<        0: Always report busy channel if ccaSync is busy<br>
                                        //!<        1: Always report idle channel if ccaSync is idle
      uint8_t ccaCorrThr:2;             //!<        Threshold for number of correlation peaks in correlator based carrier sense
   } newCcaOpt;                         //!<        New value of <code>ccaOpt</code> for the running background level operation
   int8_t newCcaRssiThr;                //!<        New value of <code>ccaRssiThr</code> for the running background level operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_MOD_FILT
//! @{
#define CMD_IEEE_MOD_FILT                                       0x2002
//! IEEE 802.15.4 Modify Frame Filtering Parameter Command
struct __RFC_STRUCT rfc_CMD_IEEE_MOD_FILT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2002
   struct {
      uint16_t frameFiltEn:1;           //!< \brief 0: Disable frame filtering<br>
                                        //!<        1: Enable frame filtering
      uint16_t frameFiltStop:1;         //!< \brief 0: Receive all packets to the end<br>
                                        //!<        1: Stop receiving frame once frame filtering has caused the frame to be rejected.
      uint16_t autoAckEn:1;             //!< \brief 0: Disable auto ACK<br>
                                        //!<        1: Enable auto ACK.
      uint16_t slottedAckEn:1;          //!< \brief 0: Non-slotted ACK<br>
                                        //!<        1: Slotted ACK.
      uint16_t autoPendEn:1;            //!< \brief 0: Auto-pend disabled<br>
                                        //!<        1: Auto-pend enabled
      uint16_t defaultPend:1;           //!<        The value of the pending data bit in auto ACK packets that are not subject to auto-pend
      uint16_t bPendDataReqOnly:1;      //!< \brief 0: Use auto-pend for any packet<br>
                                        //!<        1: Use auto-pend for data request packets only
      uint16_t bPanCoord:1;             //!< \brief 0: Device is not PAN coordinator<br>
                                        //!<        1: Device is PAN coordinator
      uint16_t maxFrameVersion:2;       //!<        Reject frames where the frame version field in the FCF is greater than this value
      uint16_t fcfReservedMask:3;       //!<        Value to be AND-ed with the reserved part of the FCF; frame rejected if result is non-zero
      uint16_t modifyFtFilter:2;        //!< \brief Treatment of MSB of frame type field before frame-type filtering:<br>
                                        //!<        0: No modification<br>
                                        //!<        1: Invert MSB<br>
                                        //!<        2: Set MSB to 0<br>
                                        //!<        3: Set MSB to 1
      uint16_t bStrictLenFilter:1;      //!< \brief 0: Accept acknowledgement frames of any length >= 5<br>
                                        //!<        1: Accept only acknowledgement frames of length 5
   } newFrameFiltOpt;                   //!<        New value of <code>frameFiltOpt</code> for the running background level operation
   struct {
      uint8_t bAcceptFt0Beacon:1;       //!< \brief Treatment of frames with frame type 000 (beacon):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt1Data:1;         //!< \brief Treatment of frames with frame type 001 (data):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt2Ack:1;          //!< \brief Treatment of frames with frame type 010 (ACK):<br>
                                        //!<        0: Reject, unless running ACK receive command<br>
                                        //!<        1: Always accept
      uint8_t bAcceptFt3MacCmd:1;       //!< \brief Treatment of frames with frame type 011 (MAC command):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt4Reserved:1;     //!< \brief Treatment of frames with frame type 100 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt5Reserved:1;     //!< \brief Treatment of frames with frame type 101 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt6Reserved:1;     //!< \brief Treatment of frames with frame type 110 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
      uint8_t bAcceptFt7Reserved:1;     //!< \brief Treatment of frames with frame type 111 (reserved):<br>
                                        //!<        0: Reject<br>
                                        //!<        1: Accept
   } newFrameTypes;                     //!<        New value of <code>frameTypes</code> for the running background level operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_MOD_SRC_MATCH
//! @{
#define CMD_IEEE_MOD_SRC_MATCH                                  0x2003
//! IEEE 802.15.4 Enable/Disable Source Matching Entry Command
struct __RFC_STRUCT rfc_CMD_IEEE_MOD_SRC_MATCH_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2003
   struct {
      uint8_t bEnable:1;                //!< \brief 0: Disable entry<br>
                                        //!<        1: Enable entry
      uint8_t srcPend:1;                //!<        New value of the pending bit for the entry
      uint8_t entryType:1;              //!< \brief 0: Short address<br>
                                        //!<        1: Extended address
   } options;
   uint8_t entryNo;                     //!<        Index of entry to enable or disable
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_ABORT_FG
//! @{
#define CMD_IEEE_ABORT_FG                                       0x2401
//! IEEE 802.15.4 Abort Foreground Level Command
struct __RFC_STRUCT rfc_CMD_IEEE_ABORT_FG_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2401
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_STOP_FG
//! @{
#define CMD_IEEE_STOP_FG                                        0x2402
//! IEEE 802.15.4 Gracefully Stop Foreground Level Command
struct __RFC_STRUCT rfc_CMD_IEEE_STOP_FG_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2402
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_IEEE_CCA_REQ
//! @{
#define CMD_IEEE_CCA_REQ                                        0x2403
//! IEEE 802.15.4 CCA and RSSI Information Request Command
struct __RFC_STRUCT rfc_CMD_IEEE_CCA_REQ_s {
   uint16_t commandNo;                  //!<        The command ID number 0x2403
   int8_t currentRssi;                  //!<        The RSSI currently observed on the channel
   int8_t maxRssi;                      //!<        The maximum RSSI observed on the channel since Rx was started
   struct {
      uint8_t ccaState:2;               //!< \brief Value of the current CCA state<br>
                                        //!<        0: Idle<br>
                                        //!<        1: Busy<br>
                                        //!<        2: Invalid
      uint8_t ccaEnergy:2;              //!< \brief Value of the current energy detect CCA state<br>
                                        //!<        0: Idle<br>
                                        //!<        1: Busy<br>
                                        //!<        2: Invalid
      uint8_t ccaCorr:2;                //!< \brief Value of the current correlator based carrier sense CCA state<br>
                                        //!<        0: Idle<br>
                                        //!<        1: Busy<br>
                                        //!<        2: Invalid
      uint8_t ccaSync:1;                //!< \brief Value of the current sync found based carrier sense CCA state<br>
                                        //!<        0: Idle<br>
                                        //!<        1: Busy
   } ccaInfo;
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ieeeRxOutput
//! @{
//! Output structure for CMD_IEEE_RX

struct __RFC_STRUCT rfc_ieeeRxOutput_s {
   uint8_t nTxAck;                      //!<        Total number of transmitted ACK frames
   uint8_t nRxBeacon;                   //!<        Number of received beacon frames
   uint8_t nRxData;                     //!<        Number of received data frames
   uint8_t nRxAck;                      //!<        Number of received acknowledgement frames
   uint8_t nRxMacCmd;                   //!<        Number of received MAC command frames
   uint8_t nRxReserved;                 //!<        Number of received frames with reserved frame type
   uint8_t nRxNok;                      //!<        Number of received frames with CRC error
   uint8_t nRxIgnored;                  //!<        Number of frames received that are to be ignored
   uint8_t nRxBufFull;                  //!<        Number of received frames discarded because the Rx buffer was full
   int8_t lastRssi;                     //!<        RSSI of last received frame
   int8_t maxRssi;                      //!<        Highest RSSI observed in the operation
   uint8_t __dummy0;
   ratmr_t beaconTimeStamp;             //!<        Time stamp of last received beacon frame
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup shortAddrEntry
//! @{
//! Structure for short address entries

struct __RFC_STRUCT rfc_shortAddrEntry_s {
   uint16_t shortAddr;                  //!<        Short address
   uint16_t panId;                      //!<        PAN ID
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ieeeRxCorrCrc
//! @{
//! Receive status byte that may be appended to message in receive buffer

struct __RFC_STRUCT rfc_ieeeRxCorrCrc_s {
   struct {
      uint8_t corr:6;                   //!<        The correlation value
      uint8_t bIgnore:1;                //!<        1 if the packet should be rejected by frame filtering, 0 otherwise
      uint8_t bCrcErr:1;                //!<        1 if the packet was received with CRC error, 0 otherwise
   } status;
} __RFC_STRUCT_ATTR;

//! @}

//! @}
//! @}
#endif
