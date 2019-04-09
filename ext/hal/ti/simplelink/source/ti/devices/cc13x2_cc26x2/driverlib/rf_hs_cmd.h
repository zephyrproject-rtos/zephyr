/******************************************************************************
*  Filename:       rf_hs_cmd.h
*  Revised:        2018-01-15 06:15:14 +0100 (Mon, 15 Jan 2018)
*  Revision:       18170
*
*  Description:    CC13x2/CC26x2 API for high-speed mode commands
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

#ifndef __HS_CMD_H
#define __HS_CMD_H

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

//! \addtogroup hs_cmd
//! @{

#include <stdint.h>
#include "rf_mailbox.h"
#include "rf_common_cmd.h"

typedef struct __RFC_STRUCT rfc_CMD_HS_TX_s rfc_CMD_HS_TX_t;
typedef struct __RFC_STRUCT rfc_CMD_HS_RX_s rfc_CMD_HS_RX_t;
typedef struct __RFC_STRUCT rfc_hsRxOutput_s rfc_hsRxOutput_t;
typedef struct __RFC_STRUCT rfc_hsRxStatus_s rfc_hsRxStatus_t;

//! \addtogroup CMD_HS_TX
//! @{
#define CMD_HS_TX                                               0x3841
//! High-Speed Transmit Command
struct __RFC_STRUCT rfc_CMD_HS_TX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x3841
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
      uint8_t bFsOff:1;                 //!< \brief 0: Keep frequency synth on after command<br>
                                        //!<        1: Turn frequency synth off after command
      uint8_t bUseCrc:1;                //!< \brief 0: Do not append CRC<br>
                                        //!<        1: Append CRC
      uint8_t bVarLen:1;                //!< \brief 0: Fixed length<br>
                                        //!<        1: Transmit length as first half-word
      uint8_t bCheckQAtEnd:1;           //!< \brief 0: Always end with HS_DONE_OK when packet has been transmitted<br>
                                        //!<        1: Check if Tx queue is empty when packet has been transmitted
   } pktConf;
   uint8_t __dummy0;
   dataQueue_t* pQueue;                 //!<        Pointer to Tx queue
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_HS_RX
//! @{
#define CMD_HS_RX                                               0x3842
//! High-Speed Receive Command
struct __RFC_STRUCT rfc_CMD_HS_RX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x3842
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
      uint8_t bFsOff:1;                 //!< \brief 0: Keep frequency synth on after command<br>
                                        //!<        1: Turn frequency synth off after command
      uint8_t bUseCrc:1;                //!< \brief 0: Do not receive or check CRC<br>
                                        //!<        1: Receive and check CRC
      uint8_t bVarLen:1;                //!< \brief 0: Fixed length<br>
                                        //!<        1: Receive length as first byte
      uint8_t bRepeatOk:1;              //!< \brief 0: End operation after receiving a packet correctly<br>
                                        //!<        1: Go back to sync search after receiving a packet correctly
      uint8_t bRepeatNok:1;             //!< \brief 0: End operation after receiving a packet with CRC error<br>
                                        //!<        1: Go back to sync search after receiving a packet with CRC error
      uint8_t addressMode:2;            //!< \brief 0: No address check<br>
                                        //!<        1: Accept <code>address0</code> and <code>address1</code><br>
                                        //!<        2: Accept <code>address0</code>, <code>address1</code>, and 0x0000<br>
                                        //!<        3: Accept <code>address0</code>, <code>address1</code>, 0x0000, and 0xFFFF
   } pktConf;
   struct {
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bIncludeLen:1;            //!<        If 1, include the received length field in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise 3scard it
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConf;
   uint16_t maxPktLen;                  //!<        Packet length for fixed length; maximum packet length for variable length
   uint16_t address0;                   //!<        Address
   uint16_t address1;                   //!<        Address (set equal to <code>address0</code> to accept only one address)
   uint8_t __dummy0;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger classifier for ending the operation
   ratmr_t endTime;                     //!<        Time used together with <code>endTrigger</code> for ending the operation
   dataQueue_t* pQueue;                 //!<        Pointer to receive queue
   rfc_hsRxOutput_t *pOutput;           //!<        Pointer to output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup hsRxOutput
//! @{
//! Output structure for CMD_HS_RX

struct __RFC_STRUCT rfc_hsRxOutput_s {
   uint16_t nRxOk;                      //!<        Number of packets that have been received with CRC OK
   uint16_t nRxNok;                     //!<        Number of packets that have been received with CRC error
   uint16_t nRxAborted;                 //!<        Number of packets not received due to illegal length or address mismatch
   uint8_t nRxBufFull;                  //!<        Number of packets that have been received and discarded due to lack of buffer space
   int8_t lastRssi;                     //!<        RSSI of last received packet
   ratmr_t timeStamp;                   //!<        Time stamp of last received packet
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup hsRxStatus
//! @{
//! Receive status word that may be appended to message in receive buffer

struct __RFC_STRUCT rfc_hsRxStatus_s {
   struct {
      uint16_t rssi:8;                  //!<        RSSI of the received packet in dBm (signed)
      uint16_t bCrcErr:1;               //!< \brief 0: Packet received OK<br>
                                        //!<        1: Packet received with CRC error
      uint16_t addressInd:2;            //!< \brief 0: Received <code>address0</code> (or no address check)<br>
                                        //!<        1: Received <code>address1</code><br>
                                        //!<        2: Received address 0x0000<br>
                                        //!<        3: Received address 0xFFFF
   } status;
} __RFC_STRUCT_ATTR;

//! @}

//! @}
//! @}
#endif
