/******************************************************************************
*  Filename:       rf_common_cmd.h
*  Revised:        2018-11-02 11:52:02 +0100 (Fri, 02 Nov 2018)
*  Revision:       18756
*
*  Description:    CC13x2/CC26x2 API for common/generic commands
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

#ifndef __COMMON_CMD_H
#define __COMMON_CMD_H

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

//! \addtogroup common_cmd
//! @{

#include <stdint.h>
#include "rf_mailbox.h"

typedef struct __RFC_STRUCT rfc_command_s rfc_command_t;
typedef struct __RFC_STRUCT rfc_radioOp_s rfc_radioOp_t;
typedef struct __RFC_STRUCT rfc_CMD_NOP_s rfc_CMD_NOP_t;
typedef struct __RFC_STRUCT rfc_CMD_RADIO_SETUP_s rfc_CMD_RADIO_SETUP_t;
typedef struct __RFC_STRUCT rfc_CMD_FS_s rfc_CMD_FS_t;
typedef struct __RFC_STRUCT rfc_CMD_FS_OFF_s rfc_CMD_FS_OFF_t;
typedef struct __RFC_STRUCT rfc_CMD_RX_TEST_s rfc_CMD_RX_TEST_t;
typedef struct __RFC_STRUCT rfc_CMD_TX_TEST_s rfc_CMD_TX_TEST_t;
typedef struct __RFC_STRUCT rfc_CMD_SYNC_STOP_RAT_s rfc_CMD_SYNC_STOP_RAT_t;
typedef struct __RFC_STRUCT rfc_CMD_SYNC_START_RAT_s rfc_CMD_SYNC_START_RAT_t;
typedef struct __RFC_STRUCT rfc_CMD_RESYNC_RAT_s rfc_CMD_RESYNC_RAT_t;
typedef struct __RFC_STRUCT rfc_CMD_COUNT_s rfc_CMD_COUNT_t;
typedef struct __RFC_STRUCT rfc_CMD_FS_POWERUP_s rfc_CMD_FS_POWERUP_t;
typedef struct __RFC_STRUCT rfc_CMD_FS_POWERDOWN_s rfc_CMD_FS_POWERDOWN_t;
typedef struct __RFC_STRUCT rfc_CMD_SCH_IMM_s rfc_CMD_SCH_IMM_t;
typedef struct __RFC_STRUCT rfc_CMD_COUNT_BRANCH_s rfc_CMD_COUNT_BRANCH_t;
typedef struct __RFC_STRUCT rfc_CMD_PATTERN_CHECK_s rfc_CMD_PATTERN_CHECK_t;
typedef struct __RFC_STRUCT rfc_CMD_RADIO_SETUP_PA_s rfc_CMD_RADIO_SETUP_PA_t;
typedef struct __RFC_STRUCT rfc_CMD_ABORT_s rfc_CMD_ABORT_t;
typedef struct __RFC_STRUCT rfc_CMD_STOP_s rfc_CMD_STOP_t;
typedef struct __RFC_STRUCT rfc_CMD_GET_RSSI_s rfc_CMD_GET_RSSI_t;
typedef struct __RFC_STRUCT rfc_CMD_UPDATE_RADIO_SETUP_s rfc_CMD_UPDATE_RADIO_SETUP_t;
typedef struct __RFC_STRUCT rfc_CMD_TRIGGER_s rfc_CMD_TRIGGER_t;
typedef struct __RFC_STRUCT rfc_CMD_GET_FW_INFO_s rfc_CMD_GET_FW_INFO_t;
typedef struct __RFC_STRUCT rfc_CMD_START_RAT_s rfc_CMD_START_RAT_t;
typedef struct __RFC_STRUCT rfc_CMD_PING_s rfc_CMD_PING_t;
typedef struct __RFC_STRUCT rfc_CMD_READ_RFREG_s rfc_CMD_READ_RFREG_t;
typedef struct __RFC_STRUCT rfc_CMD_ADD_DATA_ENTRY_s rfc_CMD_ADD_DATA_ENTRY_t;
typedef struct __RFC_STRUCT rfc_CMD_REMOVE_DATA_ENTRY_s rfc_CMD_REMOVE_DATA_ENTRY_t;
typedef struct __RFC_STRUCT rfc_CMD_FLUSH_QUEUE_s rfc_CMD_FLUSH_QUEUE_t;
typedef struct __RFC_STRUCT rfc_CMD_CLEAR_RX_s rfc_CMD_CLEAR_RX_t;
typedef struct __RFC_STRUCT rfc_CMD_REMOVE_PENDING_ENTRIES_s rfc_CMD_REMOVE_PENDING_ENTRIES_t;
typedef struct __RFC_STRUCT rfc_CMD_SET_RAT_CMP_s rfc_CMD_SET_RAT_CMP_t;
typedef struct __RFC_STRUCT rfc_CMD_SET_RAT_CPT_s rfc_CMD_SET_RAT_CPT_t;
typedef struct __RFC_STRUCT rfc_CMD_DISABLE_RAT_CH_s rfc_CMD_DISABLE_RAT_CH_t;
typedef struct __RFC_STRUCT rfc_CMD_SET_RAT_OUTPUT_s rfc_CMD_SET_RAT_OUTPUT_t;
typedef struct __RFC_STRUCT rfc_CMD_ARM_RAT_CH_s rfc_CMD_ARM_RAT_CH_t;
typedef struct __RFC_STRUCT rfc_CMD_DISARM_RAT_CH_s rfc_CMD_DISARM_RAT_CH_t;
typedef struct __RFC_STRUCT rfc_CMD_SET_TX_POWER_s rfc_CMD_SET_TX_POWER_t;
typedef struct __RFC_STRUCT rfc_CMD_SET_TX20_POWER_s rfc_CMD_SET_TX20_POWER_t;
typedef struct __RFC_STRUCT rfc_CMD_CHANGE_PA_s rfc_CMD_CHANGE_PA_t;
typedef struct __RFC_STRUCT rfc_CMD_UPDATE_HPOSC_FREQ_s rfc_CMD_UPDATE_HPOSC_FREQ_t;
typedef struct __RFC_STRUCT rfc_CMD_UPDATE_FS_s rfc_CMD_UPDATE_FS_t;
typedef struct __RFC_STRUCT rfc_CMD_MODIFY_FS_s rfc_CMD_MODIFY_FS_t;
typedef struct __RFC_STRUCT rfc_CMD_BUS_REQUEST_s rfc_CMD_BUS_REQUEST_t;
typedef struct __RFC_STRUCT rfc_CMD_SET_CMD_START_IRQ_s rfc_CMD_SET_CMD_START_IRQ_t;

//! \addtogroup command
//! @{
struct __RFC_STRUCT rfc_command_s {
   uint16_t commandNo;                  //!<        The command ID number
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup radioOp
//! @{
//! Common definition for radio operation commands

struct __RFC_STRUCT rfc_radioOp_s {
   uint16_t commandNo;                  //!<        The command ID number
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

//! \addtogroup CMD_NOP
//! @{
#define CMD_NOP                                                 0x0801
//! No Operation Command
struct __RFC_STRUCT rfc_CMD_NOP_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0801
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

//! \addtogroup CMD_RADIO_SETUP
//! @{
#define CMD_RADIO_SETUP                                         0x0802
//! Radio Setup Command for Pre-Defined Schemes
struct __RFC_STRUCT rfc_CMD_RADIO_SETUP_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0802
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
   uint8_t mode;                        //!< \brief The main mode to use<br>
                                        //!<        0x00: BLE<br>
                                        //!<        0x01: IEEE 802.15.4<br>
                                        //!<        0x02: 2 Mbps GFSK<br>
                                        //!<        0x05: 5 Mbps coded 8-FSK<br>
                                        //!<        0xFF: Keep existing mode; update overrides only<br>
                                        //!<        Others: <i>Reserved</i>
   uint8_t loDivider;                   //!< \brief LO divider setting to use. Supported values: 0, 2, 4,
                                        //!<        5, 6, 10, 12, 15, and 30.
   struct {
      uint16_t frontEndMode:3;          //!< \brief 0x00: Differential mode<br>
                                        //!<        0x01: Single-ended mode RFP<br>
                                        //!<        0x02: Single-ended mode RFN<br>
                                        //!<        0x05 Single-ended mode RFP with external frontend control on RF pins (RFN and RXTX)<br>
                                        //!<        0x06 Single-ended mode RFN with external frontend control on RF pins (RFP and RXTX)<br>
                                        //!<        Others: <i>Reserved</i>
      uint16_t biasMode:1;              //!< \brief 0: Internal bias<br>
                                        //!<        1: External bias
      uint16_t analogCfgMode:6;         //!< \brief 0x00: Write analog configuration.<br>
                                        //!<        Required first time after boot and when changing frequency band
                                        //!<        or front-end configuration<br>
                                        //!<        0x2D: Keep analog configuration.<br>
                                        //!<        May be used after standby or when changing mode with the same frequency
                                        //!<        band and front-end configuration<br>
                                        //!<        Others: <i>Reserved</i>
      uint16_t bNoFsPowerUp:1;          //!< \brief 0: Power up frequency synth<br>
                                        //!<        1: Do not power up frequency synth
   } config;                            //!<        Configuration options
   uint16_t txPower;                    //!<        Transmit power
   uint32_t* pRegOverride;              //!< \brief Pointer to a list of hardware and configuration registers to override. If NULL, no
                                        //!<        override is used.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_FS
//! @{
#define CMD_FS                                                  0x0803
//! Frequency Synthesizer Programming Command
struct __RFC_STRUCT rfc_CMD_FS_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0803
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
   uint16_t frequency;                  //!<        The frequency in MHz to tune to
   uint16_t fractFreq;                  //!<        Fractional part of the frequency to tune to
   struct {
      uint8_t bTxMode:1;                //!< \brief 0: Start synth in RX mode<br>
                                        //!<        1: Start synth in TX mode
      uint8_t refFreq:6;                //!< \brief 0: Use default reference frequency<br>
                                        //!<        Others: Use reference frequency 48 MHz/<code>refFreq</code>
   } synthConf;
   uint8_t __dummy0;                    //!<        <i>Reserved</i>, always write 0
   uint8_t __dummy1;                    //!<        <i>Reserved</i>
   uint8_t __dummy2;                    //!<        <i>Reserved</i>
   uint16_t __dummy3;                   //!<        <i>Reserved</i>
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_FS_OFF
//! @{
#define CMD_FS_OFF                                              0x0804
//! Command for Turning off Frequency Synthesizer
struct __RFC_STRUCT rfc_CMD_FS_OFF_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0804
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

//! \addtogroup CMD_RX_TEST
//! @{
#define CMD_RX_TEST                                             0x0807
//! Receiver Test Command
struct __RFC_STRUCT rfc_CMD_RX_TEST_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0807
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
      uint8_t bEnaFifo:1;               //!< \brief 0: Do not enable FIFO in modem, so that received data is not available<br>
                                        //!<        1: Enable FIFO in modem -- the data must be read out by the application
      uint8_t bFsOff:1;                 //!< \brief 0: Keep frequency synth on after command<br>
                                        //!<        1: Turn frequency synth off after command
      uint8_t bNoSync:1;                //!< \brief 0: Run sync search as normal for the configured mode<br>
                                        //!<        1: Write correlation thresholds to the maximum value to avoid getting sync
   } config;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger classifier for ending the operation
   uint32_t syncWord;                   //!<        Sync word to use for receiver
   ratmr_t endTime;                     //!<        Time to end the operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_TX_TEST
//! @{
#define CMD_TX_TEST                                             0x0808
//! Transmitter Test Command
struct __RFC_STRUCT rfc_CMD_TX_TEST_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0808
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
      uint8_t bUseCw:1;                 //!< \brief 0: Send modulated signal<br>
                                        //!<        1: Send continuous wave
      uint8_t bFsOff:1;                 //!< \brief 0: Keep frequency synth on after command<br>
                                        //!<        1: Turn frequency synth off after command
      uint8_t whitenMode:2;             //!< \brief 0: No whitening<br>
                                        //!<        1: Default whitening<br>
                                        //!<        2: PRBS-15<br>
                                        //!<        3: PRBS-32
   } config;
   uint8_t __dummy0;
   uint16_t txWord;                     //!<        Value to send to the modem before whitening
   uint8_t __dummy1;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger classifier for ending the operation
   uint32_t syncWord;                   //!<        Sync word to use for transmitter
   ratmr_t endTime;                     //!<        Time to end the operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SYNC_STOP_RAT
//! @{
#define CMD_SYNC_STOP_RAT                                       0x0809
//! Synchronize and Stop Radio Timer Command
struct __RFC_STRUCT rfc_CMD_SYNC_STOP_RAT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0809
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
   uint16_t __dummy0;
   ratmr_t rat0;                        //!< \brief The returned RAT timer value corresponding to the value the RAT would have had when the
                                        //!<        RTC was zero
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SYNC_START_RAT
//! @{
#define CMD_SYNC_START_RAT                                      0x080A
//! Synchrously Start Radio Timer Command
struct __RFC_STRUCT rfc_CMD_SYNC_START_RAT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x080A
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
   uint16_t __dummy0;
   ratmr_t rat0;                        //!< \brief The desired RAT timer value corresponding to the value the RAT would have had when the
                                        //!<        RTC was zero. This parameter is returned by CMD_SYNC_STOP_RAT
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_RESYNC_RAT
//! @{
#define CMD_RESYNC_RAT                                          0x0816
//! Re-calculate rat0 value while RAT is running
struct __RFC_STRUCT rfc_CMD_RESYNC_RAT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0816
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
   uint16_t __dummy0;
   ratmr_t rat0;                        //!< \brief The desired RAT timer value corresponding to the value the RAT would have had when the
                                        //!<        RTC was zero
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_COUNT
//! @{
#define CMD_COUNT                                               0x080B
//! Counter Command
struct __RFC_STRUCT rfc_CMD_COUNT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x080B
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
   uint16_t counter;                    //!< \brief Counter. On start, the radio CPU decrements the value, and the end status of the operation
                                        //!<        differs if the result is zero
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_FS_POWERUP
//! @{
#define CMD_FS_POWERUP                                          0x080C
//! Power up Frequency Syntheszier Command
struct __RFC_STRUCT rfc_CMD_FS_POWERUP_s {
   uint16_t commandNo;                  //!<        The command ID number 0x080C
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
   uint16_t __dummy0;
   uint32_t* pRegOverride;              //!<        Pointer to a list of hardware and configuration registers to override. If NULL, no override is used.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_FS_POWERDOWN
//! @{
#define CMD_FS_POWERDOWN                                        0x080D
//! Power down Frequency Syntheszier Command
struct __RFC_STRUCT rfc_CMD_FS_POWERDOWN_s {
   uint16_t commandNo;                  //!<        The command ID number 0x080D
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

//! \addtogroup CMD_SCH_IMM
//! @{
#define CMD_SCH_IMM                                             0x0810
//! Run Immidiate Command as Radio Operation Command
struct __RFC_STRUCT rfc_CMD_SCH_IMM_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0810
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
   uint16_t __dummy0;
   uint32_t cmdrVal;                    //!<        Value as would be written to CMDR
   uint32_t cmdstaVal;                  //!<        Value as would be returned in CMDSTA
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_COUNT_BRANCH
//! @{
#define CMD_COUNT_BRANCH                                        0x0812
//! Counter Command with Branch of Command Chain
struct __RFC_STRUCT rfc_CMD_COUNT_BRANCH_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0812
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
   uint16_t counter;                    //!< \brief Counter. On start, the radio CPU decrements the value, and the end status of the operation
                                        //!<        differs if the result is zero
   rfc_radioOp_t *pNextOpIfOk;          //!<        Pointer to next operation if counter did not expire
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_PATTERN_CHECK
//! @{
#define CMD_PATTERN_CHECK                                       0x0813
//! Command for Checking a Value in Memory aginst a Pattern
struct __RFC_STRUCT rfc_CMD_PATTERN_CHECK_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0813
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
      uint16_t operation:2;             //!< \brief Operation to perform<br>
                                        //!<        0: True if value == <code>compareVal</code><br>
                                        //!<        1: True if value < <code>compareVal</code><br>
                                        //!<        2: True if value > <code>compareVal</code><br>
                                        //!<        3: <i>Reserved</i>
      uint16_t bByteRev:1;              //!< \brief If 1, interchange the four bytes of the value, so that they are read
                                        //!<        most-significant-byte-first.
      uint16_t bBitRev:1;               //!<        If 1, perform bit reversal of the value
      uint16_t signExtend:5;            //!< \brief 0: Treat value and <code>compareVal</code> as unsigned<br>
                                        //!<        1--31: Treat value and <code>compareVal</code> as signed, where the value
                                        //!<        gives the number of the most significant bit in the signed number.
      uint16_t bRxVal:1;                //!< \brief 0: Use <code>pValue</code> as a pointer<br>
                                        //!<        1: Use <code>pValue</code> as a signed offset to the start of the last
                                        //!<        committed RX entry element
   } patternOpt;                        //!<        Options for comparison
   rfc_radioOp_t *pNextOpIfOk;          //!<        Pointer to next operation if comparison result was true
   uint8_t* pValue;                     //!<        Pointer to read from, or offset from last RX entry if <code>patternOpt.bRxVal</code> == 1
   uint32_t mask;                       //!<        Bit mask to apply before comparison
   uint32_t compareVal;                 //!<        Value to compare to
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_RADIO_SETUP_PA
//! @{
//! Radio Setup Command for Pre-Defined Schemes with PA Switching Fields
struct __RFC_STRUCT rfc_CMD_RADIO_SETUP_PA_s {
   uint16_t commandNo;                  //!<        The command ID number
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
   uint8_t mode;                        //!< \brief The main mode to use<br>
                                        //!<        0x00: BLE<br>
                                        //!<        0x01: IEEE 802.15.4<br>
                                        //!<        0x02: 2 Mbps GFSK<br>
                                        //!<        0x05: 5 Mbps coded 8-FSK<br>
                                        //!<        0xFF: Keep existing mode; update overrides only<br>
                                        //!<        Others: <i>Reserved</i>
   uint8_t loDivider;                   //!< \brief LO divider setting to use. Supported values: 0, 2, 4,
                                        //!<        5, 6, 10, 12, 15, and 30.
   struct {
      uint16_t frontEndMode:3;          //!< \brief 0x00: Differential mode<br>
                                        //!<        0x01: Single-ended mode RFP<br>
                                        //!<        0x02: Single-ended mode RFN<br>
                                        //!<        0x05 Single-ended mode RFP with external frontend control on RF pins (RFN and RXTX)<br>
                                        //!<        0x06 Single-ended mode RFN with external frontend control on RF pins (RFP and RXTX)<br>
                                        //!<        Others: <i>Reserved</i>
      uint16_t biasMode:1;              //!< \brief 0: Internal bias<br>
                                        //!<        1: External bias
      uint16_t analogCfgMode:6;         //!< \brief 0x00: Write analog configuration.<br>
                                        //!<        Required first time after boot and when changing frequency band
                                        //!<        or front-end configuration<br>
                                        //!<        0x2D: Keep analog configuration.<br>
                                        //!<        May be used after standby or when changing mode with the same frequency
                                        //!<        band and front-end configuration<br>
                                        //!<        Others: <i>Reserved</i>
      uint16_t bNoFsPowerUp:1;          //!< \brief 0: Power up frequency synth<br>
                                        //!<        1: Do not power up frequency synth
   } config;                            //!<        Configuration options
   uint16_t txPower;                    //!<        Transmit power
   uint32_t* pRegOverride;              //!< \brief Pointer to a list of hardware and configuration registers to override. If NULL, no
                                        //!<        override is used.
   uint32_t* pRegOverrideTxStd;         //!< \brief Pointer to a list of hardware and configuration registers to override when switching to
                                        //!<        standard PA. Used by RF driver only, not radio CPU.
   uint32_t* pRegOverrideTx20;          //!< \brief Pointer to a list of hardware and configuration registers to override when switching to
                                        //!<        20-dBm PA. Used by RF driver only, not radio CPU.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_ABORT
//! @{
#define CMD_ABORT                                               0x0401
//! Abort Running Radio Operation Command
struct __RFC_STRUCT rfc_CMD_ABORT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0401
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_STOP
//! @{
#define CMD_STOP                                                0x0402
//! Stop Running Radio Operation Command Gracefully
struct __RFC_STRUCT rfc_CMD_STOP_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0402
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_GET_RSSI
//! @{
#define CMD_GET_RSSI                                            0x0403
//! Read RSSI Command
struct __RFC_STRUCT rfc_CMD_GET_RSSI_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0403
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_UPDATE_RADIO_SETUP
//! @{
#define CMD_UPDATE_RADIO_SETUP                                  0x0001
//! Update Radio Settings Command
struct __RFC_STRUCT rfc_CMD_UPDATE_RADIO_SETUP_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0001
   uint16_t __dummy0;
   uint32_t* pRegOverride;              //!<        Pointer to a list of hardware and configuration registers to override
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_TRIGGER
//! @{
#define CMD_TRIGGER                                             0x0404
//! Generate Command Trigger
struct __RFC_STRUCT rfc_CMD_TRIGGER_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0404
   uint8_t triggerNo;                   //!<        Command trigger number
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_GET_FW_INFO
//! @{
#define CMD_GET_FW_INFO                                         0x0002
//! Request Information on the RF Core ROM Firmware
struct __RFC_STRUCT rfc_CMD_GET_FW_INFO_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0002
   uint16_t versionNo;                  //!<        Firmware version number
   uint16_t startOffset;                //!<        The start of free RAM
   uint16_t freeRamSz;                  //!<        The size of free RAM
   uint16_t availRatCh;                 //!<        Bitmap of available RAT channels
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_START_RAT
//! @{
#define CMD_START_RAT                                           0x0405
//! Asynchronously Start Radio Timer Command
struct __RFC_STRUCT rfc_CMD_START_RAT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0405
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_PING
//! @{
#define CMD_PING                                                0x0406
//! Respond with Command ACK Only
struct __RFC_STRUCT rfc_CMD_PING_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0406
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_READ_RFREG
//! @{
#define CMD_READ_RFREG                                          0x0601
//! Read RF Core Hardware Register
struct __RFC_STRUCT rfc_CMD_READ_RFREG_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0601
   uint16_t address;                    //!<        The offset from the start of the RF core HW register bank (0x40040000)
   uint32_t value;                      //!<        Returned value of the register
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_ADD_DATA_ENTRY
//! @{
#define CMD_ADD_DATA_ENTRY                                      0x0005
//! Add Data Entry to Queue
struct __RFC_STRUCT rfc_CMD_ADD_DATA_ENTRY_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0005
   uint16_t __dummy0;
   dataQueue_t* pQueue;                 //!<        Pointer to the queue structure to which the entry will be added
   uint8_t* pEntry;                     //!<        Pointer to the entry
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_REMOVE_DATA_ENTRY
//! @{
#define CMD_REMOVE_DATA_ENTRY                                   0x0006
//! Remove First Data Entry from Queue
struct __RFC_STRUCT rfc_CMD_REMOVE_DATA_ENTRY_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0006
   uint16_t __dummy0;
   dataQueue_t* pQueue;                 //!<        Pointer to the queue structure from which the entry will be removed
   uint8_t* pEntry;                     //!<        Pointer to the entry that was removed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_FLUSH_QUEUE
//! @{
#define CMD_FLUSH_QUEUE                                         0x0007
//! Flush Data Queue
struct __RFC_STRUCT rfc_CMD_FLUSH_QUEUE_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0007
   uint16_t __dummy0;
   dataQueue_t* pQueue;                 //!<        Pointer to the queue structure to be flushed
   uint8_t* pFirstEntry;                //!<        Pointer to the first entry that was removed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_CLEAR_RX
//! @{
#define CMD_CLEAR_RX                                            0x0008
//! Clear all RX Queue Entries
struct __RFC_STRUCT rfc_CMD_CLEAR_RX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0008
   uint16_t __dummy0;
   dataQueue_t* pQueue;                 //!<        Pointer to the queue structure to be cleared
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_REMOVE_PENDING_ENTRIES
//! @{
#define CMD_REMOVE_PENDING_ENTRIES                              0x0009
//! Remove Pending Entries from Queue
struct __RFC_STRUCT rfc_CMD_REMOVE_PENDING_ENTRIES_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0009
   uint16_t __dummy0;
   dataQueue_t* pQueue;                 //!<        Pointer to the queue structure to be flushed
   uint8_t* pFirstEntry;                //!<        Pointer to the first entry that was removed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SET_RAT_CMP
//! @{
#define CMD_SET_RAT_CMP                                         0x000A
//! Set Radio Timer Channel in Compare Mode
struct __RFC_STRUCT rfc_CMD_SET_RAT_CMP_s {
   uint16_t commandNo;                  //!<        The command ID number 0x000A
   uint8_t ratCh;                       //!<        The radio timer channel number
   uint8_t __dummy0;
   ratmr_t compareTime;                 //!<        The time at which the compare occurs
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SET_RAT_CPT
//! @{
#define CMD_SET_RAT_CPT                                         0x0603
//! Set Radio Timer Channel in Capture Mode
struct __RFC_STRUCT rfc_CMD_SET_RAT_CPT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0603
   struct {
      uint16_t :3;
      uint16_t inputSrc:5;              //!<        Input source indicator
      uint16_t ratCh:4;                 //!<        The radio timer channel number
      uint16_t bRepeated:1;             //!< \brief 0: Single capture mode<br>
                                        //!<        1: Repeated capture mode
      uint16_t inputMode:2;             //!< \brief Input mode:<br>
                                        //!<        0: Capture on rising edge<br>
                                        //!<        1: Capture on falling edge<br>
                                        //!<        2: Capture on both edges<br>
                                        //!<        3: <i>Reserved</i>
   } config;
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_DISABLE_RAT_CH
//! @{
#define CMD_DISABLE_RAT_CH                                      0x0408
//! Disable Radio Timer Channel
struct __RFC_STRUCT rfc_CMD_DISABLE_RAT_CH_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0408
   uint8_t ratCh;                       //!<        The radio timer channel number
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SET_RAT_OUTPUT
//! @{
#define CMD_SET_RAT_OUTPUT                                      0x0604
//! Set Radio Timer Output to a Specified Mode
struct __RFC_STRUCT rfc_CMD_SET_RAT_OUTPUT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0604
   struct {
      uint16_t :2;
      uint16_t outputSel:3;             //!<        Output event indicator
      uint16_t outputMode:3;            //!< \brief 0: Set output line low as default; and pulse on event. Duration of pulse is one RF Core clock period (ca. 41.67 ns).<br>
                                        //!<        1: Set output line high on event<br>
                                        //!<        2: Set output line low on event<br>
                                        //!<        3: Toggle (invert) output line state on event<br>
                                        //!<        4: Immediately set output line to low (does not change upon event)<br>
                                        //!<        5: Immediately set output line to high (does not change upon event)<br>
                                        //!<        Others: <i>Reserved</i>
      uint16_t ratCh:4;                 //!<        The radio timer channel number
   } config;
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_ARM_RAT_CH
//! @{
#define CMD_ARM_RAT_CH                                          0x0409
//! Arm Radio Timer Channel
struct __RFC_STRUCT rfc_CMD_ARM_RAT_CH_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0409
   uint8_t ratCh;                       //!<        The radio timer channel number
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_DISARM_RAT_CH
//! @{
#define CMD_DISARM_RAT_CH                                       0x040A
//! Disarm Radio Timer Channel
struct __RFC_STRUCT rfc_CMD_DISARM_RAT_CH_s {
   uint16_t commandNo;                  //!<        The command ID number 0x040A
   uint8_t ratCh;                       //!<        The radio timer channel number
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SET_TX_POWER
//! @{
#define CMD_SET_TX_POWER                                        0x0010
//! Set Transmit Power
struct __RFC_STRUCT rfc_CMD_SET_TX_POWER_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0010
   uint16_t txPower;                    //!<        New TX power setting
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SET_TX20_POWER
//! @{
#define CMD_SET_TX20_POWER                                      0x0014
//! Set Transmit Power for 20-dBm PA
struct __RFC_STRUCT rfc_CMD_SET_TX20_POWER_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0014
   uint16_t __dummy0;
   uint32_t tx20Power;                  //!<        New TX power setting
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_CHANGE_PA
//! @{
#define CMD_CHANGE_PA                                           0x0015
//! Set TX power with possibility to switch between PAs
struct __RFC_STRUCT rfc_CMD_CHANGE_PA_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0015
   uint16_t __dummy0;
   uint32_t* pRegOverride;              //!< \brief Pointer to a list of hardware and configuration registers to override as part of the
                                        //!<        change, including new TX power
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_UPDATE_HPOSC_FREQ
//! @{
#define CMD_UPDATE_HPOSC_FREQ                                   0x0608
//! Set New Frequency Offset for HPOSC
struct __RFC_STRUCT rfc_CMD_UPDATE_HPOSC_FREQ_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0608
   int16_t freqOffset;                  //!<        Relative frequency offset, signed, scaled by 2<sup>-22</sup>
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_UPDATE_FS
//! @{
#define CMD_UPDATE_FS                                           0x0011
//! Set New Synthesizer Frequency without Recalibration (Deprecated; use CMD_MODIFY_FS)
struct __RFC_STRUCT rfc_CMD_UPDATE_FS_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0011
   uint16_t __dummy0;
   uint32_t __dummy1;
   uint32_t __dummy2;
   uint16_t __dummy3;
   uint16_t frequency;                  //!<        The frequency in MHz to tune to
   uint16_t fractFreq;                  //!<        Fractional part of the frequency to tune to
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_MODIFY_FS
//! @{
#define CMD_MODIFY_FS                                           0x0013
//! Set New Synthesizer Frequency without Recalibration
struct __RFC_STRUCT rfc_CMD_MODIFY_FS_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0013
   uint16_t frequency;                  //!<        The frequency in MHz to tune to
   uint16_t fractFreq;                  //!<        Fractional part of the frequency to tune to
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BUS_REQUEST
//! @{
#define CMD_BUS_REQUEST                                         0x040E
//! Request System Bus to be Availbale
struct __RFC_STRUCT rfc_CMD_BUS_REQUEST_s {
   uint16_t commandNo;                  //!<        The command ID number 0x040E
   uint8_t bSysBusNeeded;               //!< \brief 0: System bus may sleep<br>
                                        //!<        1: System bus access needed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_SET_CMD_START_IRQ
//! @{
#define CMD_SET_CMD_START_IRQ                                   0x0411
//! Enable or disable generation of IRQ when a radio operation command starts
struct __RFC_STRUCT rfc_CMD_SET_CMD_START_IRQ_s {
   uint16_t commandNo;                  //!<        The command ID number 0x0411
   uint8_t bEna;                        //!<        1 to enable interrupt generation; 0 to disable it
} __RFC_STRUCT_ATTR;

//! @}

//! @}
//! @}
#endif
