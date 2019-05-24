/******************************************************************************
*  Filename:       rf_ble_cmd.h
*  Revised:        2018-07-31 20:13:42 +0200 (Tue, 31 Jul 2018)
*  Revision:       18572
*
*  Description:    CC13x2/CC26x2 API for Bluetooth Low Energy commands
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

#ifndef __BLE_CMD_H
#define __BLE_CMD_H

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

//! \addtogroup ble_cmd
//! @{

#include <stdint.h>
#include "rf_mailbox.h"
#include "rf_common_cmd.h"

typedef struct __RFC_STRUCT rfc_bleRadioOp_s rfc_bleRadioOp_t;
typedef struct __RFC_STRUCT rfc_ble5RadioOp_s rfc_ble5RadioOp_t;
typedef struct __RFC_STRUCT rfc_ble5Tx20RadioOp_s rfc_ble5Tx20RadioOp_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_SLAVE_s rfc_CMD_BLE_SLAVE_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_MASTER_s rfc_CMD_BLE_MASTER_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_ADV_s rfc_CMD_BLE_ADV_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_ADV_DIR_s rfc_CMD_BLE_ADV_DIR_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_ADV_NC_s rfc_CMD_BLE_ADV_NC_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_ADV_SCAN_s rfc_CMD_BLE_ADV_SCAN_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_SCANNER_s rfc_CMD_BLE_SCANNER_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_INITIATOR_s rfc_CMD_BLE_INITIATOR_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_GENERIC_RX_s rfc_CMD_BLE_GENERIC_RX_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_TX_TEST_s rfc_CMD_BLE_TX_TEST_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE_ADV_PAYLOAD_s rfc_CMD_BLE_ADV_PAYLOAD_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_RADIO_SETUP_s rfc_CMD_BLE5_RADIO_SETUP_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_SLAVE_s rfc_CMD_BLE5_SLAVE_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_MASTER_s rfc_CMD_BLE5_MASTER_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_ADV_EXT_s rfc_CMD_BLE5_ADV_EXT_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_ADV_AUX_s rfc_CMD_BLE5_ADV_AUX_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_SCANNER_s rfc_CMD_BLE5_SCANNER_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_INITIATOR_s rfc_CMD_BLE5_INITIATOR_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_GENERIC_RX_s rfc_CMD_BLE5_GENERIC_RX_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_TX_TEST_s rfc_CMD_BLE5_TX_TEST_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_ADV_s rfc_CMD_BLE5_ADV_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_ADV_DIR_s rfc_CMD_BLE5_ADV_DIR_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_ADV_NC_s rfc_CMD_BLE5_ADV_NC_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_ADV_SCAN_s rfc_CMD_BLE5_ADV_SCAN_t;
typedef struct __RFC_STRUCT rfc_CMD_BLE5_RADIO_SETUP_PA_s rfc_CMD_BLE5_RADIO_SETUP_PA_t;
typedef struct __RFC_STRUCT rfc_bleMasterSlavePar_s rfc_bleMasterSlavePar_t;
typedef struct __RFC_STRUCT rfc_bleSlavePar_s rfc_bleSlavePar_t;
typedef struct __RFC_STRUCT rfc_bleMasterPar_s rfc_bleMasterPar_t;
typedef struct __RFC_STRUCT rfc_bleAdvPar_s rfc_bleAdvPar_t;
typedef struct __RFC_STRUCT rfc_bleScannerPar_s rfc_bleScannerPar_t;
typedef struct __RFC_STRUCT rfc_bleInitiatorPar_s rfc_bleInitiatorPar_t;
typedef struct __RFC_STRUCT rfc_bleGenericRxPar_s rfc_bleGenericRxPar_t;
typedef struct __RFC_STRUCT rfc_bleTxTestPar_s rfc_bleTxTestPar_t;
typedef struct __RFC_STRUCT rfc_ble5SlavePar_s rfc_ble5SlavePar_t;
typedef struct __RFC_STRUCT rfc_ble5MasterPar_s rfc_ble5MasterPar_t;
typedef struct __RFC_STRUCT rfc_ble5AdvExtPar_s rfc_ble5AdvExtPar_t;
typedef struct __RFC_STRUCT rfc_ble5AdvAuxPar_s rfc_ble5AdvAuxPar_t;
typedef struct __RFC_STRUCT rfc_ble5AuxChRes_s rfc_ble5AuxChRes_t;
typedef struct __RFC_STRUCT rfc_ble5ScannerPar_s rfc_ble5ScannerPar_t;
typedef struct __RFC_STRUCT rfc_ble5InitiatorPar_s rfc_ble5InitiatorPar_t;
typedef struct __RFC_STRUCT rfc_bleMasterSlaveOutput_s rfc_bleMasterSlaveOutput_t;
typedef struct __RFC_STRUCT rfc_bleAdvOutput_s rfc_bleAdvOutput_t;
typedef struct __RFC_STRUCT rfc_bleScannerOutput_s rfc_bleScannerOutput_t;
typedef struct __RFC_STRUCT rfc_bleInitiatorOutput_s rfc_bleInitiatorOutput_t;
typedef struct __RFC_STRUCT rfc_ble5ScanInitOutput_s rfc_ble5ScanInitOutput_t;
typedef struct __RFC_STRUCT rfc_bleGenericRxOutput_s rfc_bleGenericRxOutput_t;
typedef struct __RFC_STRUCT rfc_bleTxTestOutput_s rfc_bleTxTestOutput_t;
typedef struct __RFC_STRUCT rfc_ble5ExtAdvEntry_s rfc_ble5ExtAdvEntry_t;
typedef struct __RFC_STRUCT rfc_bleWhiteListEntry_s rfc_bleWhiteListEntry_t;
typedef struct __RFC_STRUCT rfc_ble5AdiEntry_s rfc_ble5AdiEntry_t;
typedef struct __RFC_STRUCT rfc_bleRxStatus_s rfc_bleRxStatus_t;
typedef struct __RFC_STRUCT rfc_ble5RxStatus_s rfc_ble5RxStatus_t;

//! \addtogroup bleRadioOp
//! @{
struct __RFC_STRUCT rfc_bleRadioOp_s {
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   uint8_t* pParams;                    //!<        Pointer to command specific parameter structure
   uint8_t* pOutput;                    //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5RadioOp
//! @{
struct __RFC_STRUCT rfc_ble5RadioOp_s {
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   uint8_t* pParams;                    //!<        Pointer to command specific parameter structure
   uint8_t* pOutput;                    //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5Tx20RadioOp
//! @{
//! Command structure for Bluetooth commands which includes the optional field for 20-dBm PA TX power
struct __RFC_STRUCT rfc_ble5Tx20RadioOp_s {
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   uint8_t* pParams;                    //!<        Pointer to command specific parameter structure
   uint8_t* pOutput;                    //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_SLAVE
//! @{
#define CMD_BLE_SLAVE                                           0x1801
//! BLE Slave Command
struct __RFC_STRUCT rfc_CMD_BLE_SLAVE_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1801
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleSlavePar_t *pParams;          //!<        Pointer to command specific parameter structure
   rfc_bleMasterSlaveOutput_t *pOutput; //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_MASTER
//! @{
#define CMD_BLE_MASTER                                          0x1802
//! BLE Master Command
struct __RFC_STRUCT rfc_CMD_BLE_MASTER_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1802
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleMasterPar_t *pParams;         //!<        Pointer to command specific parameter structure
   rfc_bleMasterSlaveOutput_t *pOutput; //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_ADV
//! @{
#define CMD_BLE_ADV                                             0x1803
//! BLE Connectable Undirected Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE_ADV_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1803
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_ADV_DIR
//! @{
#define CMD_BLE_ADV_DIR                                         0x1804
//! BLE Connectable Directed Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE_ADV_DIR_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1804
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_ADV_NC
//! @{
#define CMD_BLE_ADV_NC                                          0x1805
//! BLE Non-Connectable Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE_ADV_NC_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1805
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_ADV_SCAN
//! @{
#define CMD_BLE_ADV_SCAN                                        0x1806
//! BLE Scannable Undirected Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE_ADV_SCAN_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1806
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_SCANNER
//! @{
#define CMD_BLE_SCANNER                                         0x1807
//! BLE Scanner Command
struct __RFC_STRUCT rfc_CMD_BLE_SCANNER_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1807
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleScannerPar_t *pParams;        //!<        Pointer to command specific parameter structure
   rfc_bleScannerOutput_t *pOutput;     //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_INITIATOR
//! @{
#define CMD_BLE_INITIATOR                                       0x1808
//! BLE Initiator Command
struct __RFC_STRUCT rfc_CMD_BLE_INITIATOR_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1808
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleInitiatorPar_t *pParams;      //!<        Pointer to command specific parameter structure
   rfc_bleInitiatorOutput_t *pOutput;   //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_GENERIC_RX
//! @{
#define CMD_BLE_GENERIC_RX                                      0x1809
//! BLE Generic Receiver Command
struct __RFC_STRUCT rfc_CMD_BLE_GENERIC_RX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1809
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleGenericRxPar_t *pParams;      //!<        Pointer to command specific parameter structure
   rfc_bleGenericRxOutput_t *pOutput;   //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_TX_TEST
//! @{
#define CMD_BLE_TX_TEST                                         0x180A
//! BLE PHY Test Transmitter Command
struct __RFC_STRUCT rfc_CMD_BLE_TX_TEST_s {
   uint16_t commandNo;                  //!<        The command ID number 0x180A
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   rfc_bleTxTestPar_t *pParams;         //!<        Pointer to command specific parameter structure
   rfc_bleTxTestOutput_t *pOutput;      //!<        Pointer to command specific output structure
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE_ADV_PAYLOAD
//! @{
#define CMD_BLE_ADV_PAYLOAD                                     0x1001
//! BLE Update Advertising Payload Command
struct __RFC_STRUCT rfc_CMD_BLE_ADV_PAYLOAD_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1001
   uint8_t payloadType;                 //!< \brief 0: Advertising data<br>
                                        //!<        1: Scan response data
   uint8_t newLen;                      //!<        Length of the new payload
   uint8_t* pNewData;                   //!<        Pointer to the buffer containing the new data
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to the parameter structure to update
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_RADIO_SETUP
//! @{
#define CMD_BLE5_RADIO_SETUP                                    0x1820
//! Bluetooth 5 Radio Setup Command for all PHYs
struct __RFC_STRUCT rfc_CMD_BLE5_RADIO_SETUP_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1820
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
      uint8_t mainMode:2;               //!< \brief PHY to use for non-BLE commands:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:1;                 //!< \brief Coding to use for TX if coded PHY is selected for non-BLE commands<br>
                                        //!<        0: S = 8 (125 kbps)<br>
                                        //!<        1: S = 2 (500 kbps)
   } defaultPhy;
   uint8_t loDivider;                   //!<        LO divider setting to use. Supported values: 0 or 2.
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
   uint16_t txPower;                    //!<        Default transmit power
   uint32_t* pRegOverrideCommon;        //!< \brief Pointer to a list of hardware and configuration registers to override during common
                                        //!<        initialization. If NULL, no override is used.
   uint32_t* pRegOverride1Mbps;         //!< \brief Pointer to a list of hardware and configuration registers to override when selecting
                                        //!<        1 Mbps PHY mode. If NULL, no override is used.
   uint32_t* pRegOverride2Mbps;         //!< \brief Pointer to a list of hardware and configuration registers to override when selecting
                                        //!<        2 Mbps PHY mode. If NULL, no override is used.
   uint32_t* pRegOverrideCoded;         //!< \brief Pointer to a list of hardware and configuration registers to override when selecting
                                        //!<        coded PHY mode. If NULL, no override is used.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_SLAVE
//! @{
#define CMD_BLE5_SLAVE                                          0x1821
//! Bluetooth 5 Slave Command
struct __RFC_STRUCT rfc_CMD_BLE5_SLAVE_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1821
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_ble5SlavePar_t *pParams;         //!<        Pointer to command specific parameter structure
   rfc_bleMasterSlaveOutput_t *pOutput; //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_MASTER
//! @{
#define CMD_BLE5_MASTER                                         0x1822
//! Bluetooth 5 Master Command
struct __RFC_STRUCT rfc_CMD_BLE5_MASTER_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1822
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_ble5MasterPar_t *pParams;        //!<        Pointer to command specific parameter structure
   rfc_bleMasterSlaveOutput_t *pOutput; //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_ADV_EXT
//! @{
#define CMD_BLE5_ADV_EXT                                        0x1823
//! Bluetooth 5 Extended Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE5_ADV_EXT_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1823
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_ble5AdvExtPar_t *pParams;        //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_ADV_AUX
//! @{
#define CMD_BLE5_ADV_AUX                                        0x1824
//! Bluetooth 5 Secondary Channel Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE5_ADV_AUX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1824
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_ble5AdvAuxPar_t *pParams;        //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_SCANNER
//! @{
#define CMD_BLE5_SCANNER                                        0x1827
//! Bluetooth 5 Scanner Command
struct __RFC_STRUCT rfc_CMD_BLE5_SCANNER_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1827
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_ble5ScannerPar_t *pParams;       //!<        Pointer to command specific parameter structure
   rfc_ble5ScanInitOutput_t *pOutput;   //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_INITIATOR
//! @{
#define CMD_BLE5_INITIATOR                                      0x1828
//! Bluetooth 5 Initiator Command
struct __RFC_STRUCT rfc_CMD_BLE5_INITIATOR_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1828
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_ble5InitiatorPar_t *pParams;     //!<        Pointer to command specific parameter structure
   rfc_ble5ScanInitOutput_t *pOutput;   //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_GENERIC_RX
//! @{
#define CMD_BLE5_GENERIC_RX                                     0x1829
//! Bluetooth 5 Generic Receiver Command
struct __RFC_STRUCT rfc_CMD_BLE5_GENERIC_RX_s {
   uint16_t commandNo;                  //!<        The command ID number 0x1829
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_bleGenericRxPar_t *pParams;      //!<        Pointer to command specific parameter structure
   rfc_bleGenericRxOutput_t *pOutput;   //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_TX_TEST
//! @{
#define CMD_BLE5_TX_TEST                                        0x182A
//! Bluetooth 5 PHY Test Transmitter Command
struct __RFC_STRUCT rfc_CMD_BLE5_TX_TEST_s {
   uint16_t commandNo;                  //!<        The command ID number 0x182A
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_bleTxTestPar_t *pParams;         //!<        Pointer to command specific parameter structure
   rfc_bleTxTestOutput_t *pOutput;      //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_ADV
//! @{
#define CMD_BLE5_ADV                                            0x182B
//! Bluetooth 5 Connectable Undirected Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE5_ADV_s {
   uint16_t commandNo;                  //!<        The command ID number 0x182B
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_ADV_DIR
//! @{
#define CMD_BLE5_ADV_DIR                                        0x182C
//! Bluetooth 5 Connectable Directed Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE5_ADV_DIR_s {
   uint16_t commandNo;                  //!<        The command ID number 0x182C
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_ADV_NC
//! @{
#define CMD_BLE5_ADV_NC                                         0x182D
//! Bluetooth 5 Non-Connectable Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE5_ADV_NC_s {
   uint16_t commandNo;                  //!<        The command ID number 0x182D
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_ADV_SCAN
//! @{
#define CMD_BLE5_ADV_SCAN                                       0x182E
//! Bluetooth 5 Scannable Undirected Advertiser Command
struct __RFC_STRUCT rfc_CMD_BLE5_ADV_SCAN_s {
   uint16_t commandNo;                  //!<        The command ID number 0x182E
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
   uint8_t channel;                     //!< \brief Channel to use<br>
                                        //!<        0--39: BLE advertising/data channel index<br>
                                        //!<        60--207: Custom frequency; (2300 + <code>channel</code>) MHz<br>
                                        //!<        255: Use existing frequency<br>
                                        //!<        Others: <i>Reserved</i>
   struct {
      uint8_t init:7;                   //!< \brief If <code>bOverride</code> = 1 or custom frequency is used:<br>
                                        //!<        0: Do not use whitening<br>
                                        //!<        Other value: Initialization for 7-bit LFSR whitener
      uint8_t bOverride:1;              //!< \brief 0: Use default whitening for BLE advertising/data channels<br>
                                        //!<        1: Override whitening initialization with value of init
   } whitening;
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:6;                 //!< \brief Coding to use for TX if coded PHY is selected.
                                        //!<        See the Technical Reference Manual for details.
   } phyMode;
   uint8_t rangeDelay;                  //!<        Number of RAT ticks to add to the listening time after T_IFS
   uint16_t txPower;                    //!< \brief Transmit power to use (overrides the one given in radio setup) <br>
                                        //!<        0x0000: Use default TX power<br>
                                        //!<        0xFFFF: 20-dBm PA only: Use TX power from <code>tx20Power</code> field (command
                                        //!<        structure that includes <code>tx20Power</code> must be used)
   rfc_bleAdvPar_t *pParams;            //!<        Pointer to command specific parameter structure
   rfc_bleAdvOutput_t *pOutput;         //!<        Pointer to command specific output structure
   uint32_t tx20Power;                  //!< \brief If <code>txPower</code> = 0xFFFF:<br>
                                        //!<        If <code>tx20Power</code> < 0x10000000: Transmit power to use for the 20-dBm PA;
                                        //!<        overrides the one given in radio setup for the duration of the command. <br>
                                        //!<        If <code>tx20Power</code> >= 0x10000000: Pointer to PA change override structure
                                        //!<        as for CMD_CHANGE_PA ; permanently changes the PA and PA power set in radio setup.<br>
                                        //!<        For other values of <code>txPower</code>, this field is not accessed by the radio
                                        //!<        CPU and may be omitted from the structure.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup CMD_BLE5_RADIO_SETUP_PA
//! @{
//! Bluetooth 5 Radio Setup Command for all PHYs with PA Switching Fields
struct __RFC_STRUCT rfc_CMD_BLE5_RADIO_SETUP_PA_s {
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
   struct {
      uint8_t mainMode:2;               //!< \brief PHY to use for non-BLE commands:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        3: <i>Reserved</i>
      uint8_t coding:1;                 //!< \brief Coding to use for TX if coded PHY is selected for non-BLE commands<br>
                                        //!<        0: S = 8 (125 kbps)<br>
                                        //!<        1: S = 2 (500 kbps)
   } defaultPhy;
   uint8_t loDivider;                   //!<        LO divider setting to use. Supported values: 0 or 2.
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
   uint16_t txPower;                    //!<        Default transmit power
   uint32_t* pRegOverrideCommon;        //!< \brief Pointer to a list of hardware and configuration registers to override during common
                                        //!<        initialization. If NULL, no override is used.
   uint32_t* pRegOverride1Mbps;         //!< \brief Pointer to a list of hardware and configuration registers to override when selecting
                                        //!<        1 Mbps PHY mode. If NULL, no override is used.
   uint32_t* pRegOverride2Mbps;         //!< \brief Pointer to a list of hardware and configuration registers to override when selecting
                                        //!<        2 Mbps PHY mode. If NULL, no override is used.
   uint32_t* pRegOverrideCoded;         //!< \brief Pointer to a list of hardware and configuration registers to override when selecting
                                        //!<        coded PHY mode. If NULL, no override is used.
   uint32_t* pRegOverrideTxStd;         //!< \brief Pointer to a list of hardware and configuration registers to override when switching to
                                        //!<        standard PA. Used by RF driver only, not radio CPU.
   uint32_t* pRegOverrideTx20;          //!< \brief Pointer to a list of hardware and configuration registers to override when switching to
                                        //!<        20-dBm PA. Used by RF driver only, not radio CPU.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleMasterSlavePar
//! @{
struct __RFC_STRUCT rfc_bleMasterSlavePar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   dataQueue_t* pTxQ;                   //!<        Pointer to transmit queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t lastRxSn:1;               //!<        The SN bit of the header of the last packet received with CRC OK
      uint8_t lastTxSn:1;               //!<        The SN bit of the header of the last transmitted packet
      uint8_t nextTxSn:1;               //!<        The SN bit of the header of the next packet to transmit
      uint8_t bFirstPkt:1;              //!<        For slave: 0 if a packet has been transmitted on the connection, 1 otherwise
      uint8_t bAutoEmpty:1;             //!<        1 if the last transmitted packet was an auto-empty packet
      uint8_t bLlCtrlTx:1;              //!<        1 if the last transmitted packet was an LL control packet (LLID = 11)
      uint8_t bLlCtrlAckRx:1;           //!<        1 if the last received packet was the ACK of an LL control packet
      uint8_t bLlCtrlAckPending:1;      //!<        1 if the last successfully received packet was an LL control packet which has not yet been ACK'ed
   } seqStat;                           //!<        Sequence number status
   uint8_t maxNack;                     //!<        Maximum number of NACKs received before operation ends. 0: No limit
   uint8_t maxPkt;                      //!<        Maximum number of packets transmitted in the operation before it ends. 0: No limit
   uint32_t accessAddress;              //!<        Access address used on the connection
   uint8_t crcInit0;                    //!<        CRC initialization value used on the connection -- least significant byte
   uint8_t crcInit1;                    //!<        CRC initialization value used on the connection -- middle byte
   uint8_t crcInit2;                    //!<        CRC initialization value used on the connection -- most significant byte
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleSlavePar
//! @{
//! Parameter structure for legacy slave (CMD_BLE_SLAVE)

struct __RFC_STRUCT rfc_bleSlavePar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   dataQueue_t* pTxQ;                   //!<        Pointer to transmit queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t lastRxSn:1;               //!<        The SN bit of the header of the last packet received with CRC OK
      uint8_t lastTxSn:1;               //!<        The SN bit of the header of the last transmitted packet
      uint8_t nextTxSn:1;               //!<        The SN bit of the header of the next packet to transmit
      uint8_t bFirstPkt:1;              //!<        For slave: 0 if a packet has been transmitted on the connection, 1 otherwise
      uint8_t bAutoEmpty:1;             //!<        1 if the last transmitted packet was an auto-empty packet
      uint8_t bLlCtrlTx:1;              //!<        1 if the last transmitted packet was an LL control packet (LLID = 11)
      uint8_t bLlCtrlAckRx:1;           //!<        1 if the last received packet was the ACK of an LL control packet
      uint8_t bLlCtrlAckPending:1;      //!<        1 if the last successfully received packet was an LL control packet which has not yet been ACK'ed
   } seqStat;                           //!<        Sequence number status
   uint8_t maxNack;                     //!<        Maximum number of NACKs received before operation ends. 0: No limit
   uint8_t maxPkt;                      //!<        Maximum number of packets transmitted in the operation before it ends. 0: No limit
   uint32_t accessAddress;              //!<        Access address used on the connection
   uint8_t crcInit0;                    //!<        CRC initialization value used on the connection -- least significant byte
   uint8_t crcInit1;                    //!<        CRC initialization value used on the connection -- middle byte
   uint8_t crcInit2;                    //!<        CRC initialization value used on the connection -- most significant byte
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } timeoutTrigger;                    //!<        Trigger that defines timeout of the first receive operation
   ratmr_t timeoutTime;                 //!< \brief Time used together with <code>timeoutTrigger</code> that defines timeout of the first
                                        //!<        receive operation
   uint16_t __dummy0;
   uint8_t __dummy1;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the connection event as soon as allowed
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        connection event as soon as allowed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleMasterPar
//! @{
//! Parameter structure for legacy master (CMD_BLE_MASTER)

struct __RFC_STRUCT rfc_bleMasterPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   dataQueue_t* pTxQ;                   //!<        Pointer to transmit queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t lastRxSn:1;               //!<        The SN bit of the header of the last packet received with CRC OK
      uint8_t lastTxSn:1;               //!<        The SN bit of the header of the last transmitted packet
      uint8_t nextTxSn:1;               //!<        The SN bit of the header of the next packet to transmit
      uint8_t bFirstPkt:1;              //!<        For slave: 0 if a packet has been transmitted on the connection, 1 otherwise
      uint8_t bAutoEmpty:1;             //!<        1 if the last transmitted packet was an auto-empty packet
      uint8_t bLlCtrlTx:1;              //!<        1 if the last transmitted packet was an LL control packet (LLID = 11)
      uint8_t bLlCtrlAckRx:1;           //!<        1 if the last received packet was the ACK of an LL control packet
      uint8_t bLlCtrlAckPending:1;      //!<        1 if the last successfully received packet was an LL control packet which has not yet been ACK'ed
   } seqStat;                           //!<        Sequence number status
   uint8_t maxNack;                     //!<        Maximum number of NACKs received before operation ends. 0: No limit
   uint8_t maxPkt;                      //!<        Maximum number of packets transmitted in the operation before it ends. 0: No limit
   uint32_t accessAddress;              //!<        Access address used on the connection
   uint8_t crcInit0;                    //!<        CRC initialization value used on the connection -- least significant byte
   uint8_t crcInit1;                    //!<        CRC initialization value used on the connection -- middle byte
   uint8_t crcInit2;                    //!<        CRC initialization value used on the connection -- most significant byte
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the connection event as soon as allowed
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        connection event as soon as allowed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleAdvPar
//! @{
//! Parameter structure for legacy advertiser (CMD_BLE_ADV* and CMD_BLE5_ADV*)

struct __RFC_STRUCT rfc_bleAdvPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t advFilterPolicy:2;        //!< \brief Advertiser filter policy<br>
                                        //!<        0: Process scan and connect requests from all devices<br>
                                        //!<        1: Process connect requests from all devices and only scan requests from
                                        //!<        devices that are in the white list<br>
                                        //!<        2: Process scan requests from all devices and only connect requests from
                                        //!<        devices that are in the white list<br>
                                        //!<        3: Process scan and connect requests only from devices in the white list
      uint8_t deviceAddrType:1;         //!<        The type of the device address -- public (0) or random (1)
      uint8_t peerAddrType:1;           //!<        Directed advertiser: The type of the peer address -- public (0) or random (1)
      uint8_t bStrictLenFilter:1;       //!< \brief 0: Accept any packet with a valid advertising packet length<br>
                                        //!<        1: Discard messages with illegal length for the given packet type
      uint8_t chSel:1;                  //!< \brief 0: Do not report support of Channel Selection Algorithm #2<br>
                                        //!<        1: Report support of Channel Selection Algorithm #2
      uint8_t privIgnMode:1;            //!< \brief 0: Filter on bPrivIgn only when white list is used
                                        //!<        1: Filter on bPrivIgn always
      uint8_t rpaMode:1;                //!< \brief Resolvable private address mode<br>
                                        //!<        0: Normal operation<br>
                                        //!<        1: Use white list for a received RPA regardless of filter policy
   } advConfig;
   uint8_t advLen;                      //!<        Size of advertiser data
   uint8_t scanRspLen;                  //!<        Size of scan response data
   uint8_t* pAdvData;                   //!<        Pointer to buffer containing ADV*_IND data
   uint8_t* pScanRspData;               //!<        Pointer to buffer containing SCAN_RSP data
   uint16_t* pDeviceAddress;            //!< \brief Pointer (with least significant bit set to 0) to device address used for this device.
                                        //!<        If least significant bit is 1, the address type given by
                                        //!<        <code>advConfig.deviceAddrType</code> is inverted.
   rfc_bleWhiteListEntry_t *pWhiteList; //!< \brief Pointer (with least significant bit set to 0)  to white list or peer address (directed
                                        //!<        advertiser). If least significant bit is 1, the address type given by
                                        //!<        <code>advConfig.peerAddrType</code> is inverted.
   struct {
      uint8_t scanRspEndType:1;         //!< \brief Command status at end if SCAN_RSP was sent:<br>
                                        //!<        0: End with BLE_DONE_OK and result True<br>
                                        //!<        1: End with BLE_DONE_SCAN_RSP and result False
   } behConfig;
   uint8_t __dummy0;
   uint8_t __dummy1;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the advertiser event as soon as allowed
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        advertiser event as soon as allowed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleScannerPar
//! @{
//! Parameter structure for legacy scanner (CMD_BLE_SCANNER)

struct __RFC_STRUCT rfc_bleScannerPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t scanFilterPolicy:1;       //!< \brief Scanning filter policy regarding advertiser address<br>
                                        //!<        0: Accept all advertisement packets<br>
                                        //!<        1: Accept only advertisement packets from devices where the advertiser's address
                                        //!<        is in the white list
      uint8_t bActiveScan:1;            //!< \brief 0: Passive scan<br>
                                        //!<        1: Active scan
      uint8_t deviceAddrType:1;         //!<        The type of the device address -- public (0) or random (1)
      uint8_t rpaFilterPolicy:1;        //!< \brief Filter policy for initA for ADV_DIRECT_IND messages<br>
                                        //!<        0: Accept only initA that matches own address<br>
                                        //!<        1: Also accept all resolvable private addresses
      uint8_t bStrictLenFilter:1;       //!< \brief 0: Accept any packet with a valid advertising packet length<br>
                                        //!<        1: Discard messages with illegal length for the given packet type
      uint8_t bAutoWlIgnore:1;          //!< \brief 0: Do not set ignore bit in white list from radio CPU<br>
                                        //!<        1: Automatically set ignore bit in white list
      uint8_t bEndOnRpt:1;              //!< \brief 0: Continue scanner operation after each reporting ADV*_IND or sending SCAN_RSP<br>
                                        //!<        1: End scanner operation after each reported ADV*_IND and potentially SCAN_RSP
      uint8_t rpaMode:1;                //!< \brief Resolvable private address mode<br>
                                        //!<        0: Normal operation<br>
                                        //!<        1: Use white list for a received RPA regardless of filter policy
   } scanConfig;
   uint16_t randomState;                //!<        State for pseudo-random number generation used in backoff procedure
   uint16_t backoffCount;               //!<        Parameter <i>backoffCount</i> used in backoff procedure, cf. Bluetooth spec
   struct {
      uint8_t logUpperLimit:4;          //!<        Binary logarithm of parameter upperLimit used in scanner backoff procedure
      uint8_t bLastSucceeded:1;         //!< \brief 1 if the last SCAN_RSP was successfully received and <code>upperLimit</code>
                                        //!<        not changed
      uint8_t bLastFailed:1;            //!< \brief 1 if reception of the last SCAN_RSP failed and <code>upperLimit</code> was not
                                        //!<        changed
   } backoffPar;
   uint8_t scanReqLen;                  //!<        Size of scan request data
   uint8_t* pScanReqData;               //!<        Pointer to buffer containing SCAN_REQ data
   uint16_t* pDeviceAddress;            //!< \brief Pointer (with least significant bit set to 0) to device address used for this device.
                                        //!<        If least significant bit is 1, the address type given by
                                        //!<        <code>scanConfig.deviceAddrType</code> is inverted.
   rfc_bleWhiteListEntry_t *pWhiteList; //!<        Pointer to white list
   uint16_t __dummy0;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } timeoutTrigger;                    //!<        Trigger that causes the device to stop receiving as soon as allowed
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to stop receiving as soon as allowed
   ratmr_t timeoutTime;                 //!< \brief Time used together with <code>timeoutTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_RXTIMEOUT
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_ENDED
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleInitiatorPar
//! @{
//! Parameter structure for legacy initiator (CMD_BLE_INITIATOR)

struct __RFC_STRUCT rfc_bleInitiatorPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t bUseWhiteList:1;          //!< \brief Initiator filter policy<br>
                                        //!<        0: Use specific peer address<br>
                                        //!<        1: Use white list
      uint8_t bDynamicWinOffset:1;      //!< \brief 0: No dynamic WinOffset insertion<br>
                                        //!<        1: Use dynamic WinOffset insertion
      uint8_t deviceAddrType:1;         //!<        The type of the device address -- public (0) or random (1)
      uint8_t peerAddrType:1;           //!<        The type of the peer address -- public (0) or random (1)
      uint8_t bStrictLenFilter:1;       //!< \brief 0: Accept any packet with a valid advertising packet length<br>
                                        //!<        1: Discard messages with illegal length for the given packet type
      uint8_t chSel:1;                  //!< \brief 0: Do not report support of Channel Selection Algorithm #2<br>
                                        //!<        1: Report support of Channel Selection Algorithm #2
   } initConfig;
   uint8_t __dummy0;
   uint8_t connectReqLen;               //!<        Size of connect request data
   uint8_t* pConnectReqData;            //!<        Pointer to buffer containing LLData to go in the CONNECT_IND (CONNECT_REQ)
   uint16_t* pDeviceAddress;            //!< \brief Pointer (with least significant bit set to 0) to device address used for this device.
                                        //!<        If least significant bit is 1, the address type given by
                                        //!<        <code>initConfig.deviceAddrType</code> is inverted.
   rfc_bleWhiteListEntry_t *pWhiteList; //!< \brief Pointer (with least significant bit set to 0)  to white list or peer address. If least
                                        //!<        significant bit is 1, the address type given by <code>initConfig.peerAddrType</code>
                                        //!<        is inverted.
   ratmr_t connectTime;                 //!< \brief Indication of timer value of the first possible start time of the first connection event.
                                        //!<        Set to the calculated value if a connection is made and to the next possible connection
                                        //!<        time if not.
   uint16_t __dummy1;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } timeoutTrigger;                    //!<        Trigger that causes the device to stop receiving as soon as allowed
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to stop receiving as soon as allowed
   ratmr_t timeoutTime;                 //!< \brief Time used together with <code>timeoutTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_RXTIMEOUT
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_ENDED
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleGenericRxPar
//! @{
//! Parameter structure for generic Rx (CMD_BLE_GENERIC_RX and CMD_BLE5_GENERIC_RX)

struct __RFC_STRUCT rfc_bleGenericRxPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue. May be NULL; if so, received packets are not stored
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   uint8_t bRepeat;                     //!< \brief 0: End operation after receiving a packet<br>
                                        //!<        1: Restart receiver after receiving a packet
   uint16_t __dummy0;
   uint32_t accessAddress;              //!<        Access address used on the connection
   uint8_t crcInit0;                    //!<        CRC initialization value used on the connection -- least significant byte
   uint8_t crcInit1;                    //!<        CRC initialization value used on the connection -- middle byte
   uint8_t crcInit2;                    //!<        CRC initialization value used on the connection -- most significant byte
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the Rx operation
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        Rx operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleTxTestPar
//! @{
//! Parameter structure for Tx test (CMD_BLE_TX_TEST and CMD_BLE5_TX_TEST)

struct __RFC_STRUCT rfc_bleTxTestPar_s {
   uint16_t numPackets;                 //!< \brief Number of packets to transmit<br>
                                        //!<        0: Transmit unlimited number of packets
   uint8_t payloadLength;               //!<        The number of payload bytes in each packet.
   uint8_t packetType;                  //!< \brief The packet type to be used, encoded according to the Bluetooth 5.0 spec, Volume 6, Part F,
                                        //!<        Section 4.1.4
   ratmr_t period;                      //!<        Number of radio timer cycles between the start of each packet
   struct {
      uint8_t bOverrideDefault:1;       //!< \brief 0: Use default packet encoding<br>
                                        //!<        1: Override packet contents
      uint8_t bUsePrbs9:1;              //!< \brief If <code>bOverride</code> is 1:<br>
                                        //!<        0: No PRBS9 encoding of packet<br>
                                        //!<        1: Use PRBS9 encoding of packet
      uint8_t bUsePrbs15:1;             //!< \brief If <code>bOverride</code> is 1:<br>
                                        //!<        0: No PRBS15 encoding of packet<br>
                                        //!<        1: Use PRBS15 encoding of packet
   } config;
   uint8_t byteVal;                     //!<        If <code>config.bOverride</code> is 1, value of each byte to be sent
   uint8_t __dummy0;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the Test Tx operation
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        Test Tx operation
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5SlavePar
//! @{
//! Parameter structure for Bluetooth 5 slave (CMD_BLE5_SLAVE)

struct __RFC_STRUCT rfc_ble5SlavePar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   dataQueue_t* pTxQ;                   //!<        Pointer to transmit queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t lastRxSn:1;               //!<        The SN bit of the header of the last packet received with CRC OK
      uint8_t lastTxSn:1;               //!<        The SN bit of the header of the last transmitted packet
      uint8_t nextTxSn:1;               //!<        The SN bit of the header of the next packet to transmit
      uint8_t bFirstPkt:1;              //!<        For slave: 0 if a packet has been transmitted on the connection, 1 otherwise
      uint8_t bAutoEmpty:1;             //!<        1 if the last transmitted packet was an auto-empty packet
      uint8_t bLlCtrlTx:1;              //!<        1 if the last transmitted packet was an LL control packet (LLID = 11)
      uint8_t bLlCtrlAckRx:1;           //!<        1 if the last received packet was the ACK of an LL control packet
      uint8_t bLlCtrlAckPending:1;      //!<        1 if the last successfully received packet was an LL control packet which has not yet been ACK'ed
   } seqStat;                           //!<        Sequence number status
   uint8_t maxNack;                     //!<        Maximum number of NACKs received before operation ends. 0: No limit
   uint8_t maxPkt;                      //!<        Maximum number of packets transmitted in the operation before it ends. 0: No limit
   uint32_t accessAddress;              //!<        Access address used on the connection
   uint8_t crcInit0;                    //!<        CRC initialization value used on the connection -- least significant byte
   uint8_t crcInit1;                    //!<        CRC initialization value used on the connection -- middle byte
   uint8_t crcInit2;                    //!<        CRC initialization value used on the connection -- most significant byte
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } timeoutTrigger;                    //!<        Trigger that defines timeout of the first receive operation
   ratmr_t timeoutTime;                 //!< \brief Time used together with <code>timeoutTrigger</code> that defines timeout of the first
                                        //!<        receive operation
   uint8_t maxRxPktLen;                 //!<        Maximum packet length currently allowed for received packets on the connection
   uint8_t maxLenLowRate;               //!<        Maximum packet length for which using S = 8 (125 kbps) is allowed when transmitting. 0: no limit.
   uint8_t __dummy0;
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the connection event as soon as allowed
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        connection event as soon as allowed
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5MasterPar
//! @{
//! Parameter structure for Bluetooth 5 master (CMD_BLE5_MASTER)

struct __RFC_STRUCT rfc_ble5MasterPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   dataQueue_t* pTxQ;                   //!<        Pointer to transmit queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t lastRxSn:1;               //!<        The SN bit of the header of the last packet received with CRC OK
      uint8_t lastTxSn:1;               //!<        The SN bit of the header of the last transmitted packet
      uint8_t nextTxSn:1;               //!<        The SN bit of the header of the next packet to transmit
      uint8_t bFirstPkt:1;              //!<        For slave: 0 if a packet has been transmitted on the connection, 1 otherwise
      uint8_t bAutoEmpty:1;             //!<        1 if the last transmitted packet was an auto-empty packet
      uint8_t bLlCtrlTx:1;              //!<        1 if the last transmitted packet was an LL control packet (LLID = 11)
      uint8_t bLlCtrlAckRx:1;           //!<        1 if the last received packet was the ACK of an LL control packet
      uint8_t bLlCtrlAckPending:1;      //!<        1 if the last successfully received packet was an LL control packet which has not yet been ACK'ed
   } seqStat;                           //!<        Sequence number status
   uint8_t maxNack;                     //!<        Maximum number of NACKs received before operation ends. 0: No limit
   uint8_t maxPkt;                      //!<        Maximum number of packets transmitted in the operation before it ends. 0: No limit
   uint32_t accessAddress;              //!<        Access address used on the connection
   uint8_t crcInit0;                    //!<        CRC initialization value used on the connection -- least significant byte
   uint8_t crcInit1;                    //!<        CRC initialization value used on the connection -- middle byte
   uint8_t crcInit2;                    //!<        CRC initialization value used on the connection -- most significant byte
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to end the connection event as soon as allowed
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to end the
                                        //!<        connection event as soon as allowed
   uint8_t maxRxPktLen;                 //!<        Maximum packet length currently allowed for received packets on the connection
   uint8_t maxLenLowRate;               //!<        Maximum packet length for which using S = 8 (125 kbps) is allowed when transmitting. 0: no limit.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5AdvExtPar
//! @{
//! Parameter structure for extended advertiser (CMD_BLE5_ADV_EXT)

struct __RFC_STRUCT rfc_ble5AdvExtPar_s {
   struct {
      uint8_t :2;
      uint8_t deviceAddrType:1;         //!<        The type of the device address -- public (0) or random (1)
   } advConfig;
   uint8_t __dummy0;
   uint8_t __dummy1;
   uint8_t auxPtrTargetType;            //!< \brief Number indicating reference for auxPtrTargetTime. Takes same values as trigger types,
                                        //!<        but only TRIG_ABSTIME and TRIG_REL_* are allowed
   ratmr_t auxPtrTargetTime;            //!<        Time of start of packet to which auxPtr points
   uint8_t* pAdvPkt;                    //!<        Pointer to extended advertising packet for the ADV_EXT_IND packet
   uint16_t* pDeviceAddress;            //!< \brief Pointer (with least significant bit set to 0) to device address used for this device.
                                        //!<        If least significant bit is 1, the address type given by
                                        //!<        <code>advConfig.deviceAddrType</code> is inverted.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5AdvAuxPar
//! @{
//! Parameter structure for secondary channel advertiser (CMD_BLE5_ADV_AUX)

struct __RFC_STRUCT rfc_ble5AdvAuxPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t advFilterPolicy:2;        //!< \brief Advertiser filter policy<br>
                                        //!<        0: Process scan and connect requests from all devices<br>
                                        //!<        1: Process connect requests from all devices and only scan requests from
                                        //!<        devices that are in the white list<br>
                                        //!<        2: Process scan requests from all devices and only connect requests from
                                        //!<        devices that are in the white list<br>
                                        //!<        3: Process scan and connect requests only from devices in the white list
      uint8_t deviceAddrType:1;         //!<        The type of the device address -- public (0) or random (1)
      uint8_t targetAddrType:1;         //!<        Directed secondary advertiser: The type of the target address -- public (0) or random (1)
      uint8_t bStrictLenFilter:1;       //!< \brief 0: Accept any packet with a valid advertising packet length<br>
                                        //!<        1: Discard messages with illegal length for the given packet type
      uint8_t bDirected:1;              //!< \brief 0: Advertiser is undirected: pWhiteList points to a white list
                                        //!<        1: Advertiser is directed: pWhiteList points to a single device address
      uint8_t privIgnMode:1;            //!< \brief 0: Filter on bPrivIgn only when white list is used
                                        //!<        1: Filter on bPrivIgn always
      uint8_t rpaMode:1;                //!< \brief Resolvable private address mode<br>
                                        //!<        0: Normal operation<br>
                                        //!<        1: Use white list for a received RPA regardless of filter policy
   } advConfig;
   struct {
      uint8_t scanRspEndType:1;         //!< \brief Command status at end if AUX_SCAN_RSP was sent:<br>
                                        //!<        0: End with BLE_DONE_OK and result True<br>
                                        //!<        1: End with BLE_DONE_SCAN_RSP and result False
   } behConfig;
   uint8_t auxPtrTargetType;            //!< \brief Number indicating reference for auxPtrTargetTime. Takes same values as trigger types,
                                        //!<        but only TRIG_ABSTIME and TRIG_REL_* are allowed
   ratmr_t auxPtrTargetTime;            //!<        Time of start of packet to which auxPtr points
   uint8_t* pAdvPkt;                    //!<        Pointer to extended advertising packet for the ADV_AUX_IND packet
   uint8_t* pRspPkt;                    //!< \brief Pointer to extended advertising packet for the AUX_SCAN_RSP or AUX_CONNECT_RSP packet
                                        //!<        (may be NULL if not applicable)
   uint16_t* pDeviceAddress;            //!< \brief Pointer (with least significant bit set to 0) to device address used for this device.
                                        //!<        If least significant bit is 1, the address type given by
                                        //!<        <code>advConfig.deviceAddrType</code> is inverted.
   rfc_bleWhiteListEntry_t *pWhiteList; //!< \brief Pointer (with least significant bit set to 0)  to white list or peer address (directed
                                        //!<        advertiser). If least significant bit is 1, the address type given by
                                        //!<        <code>advConfig.peerAddrType</code> is inverted.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5AuxChRes
//! @{
struct __RFC_STRUCT rfc_ble5AuxChRes_s {
   ratmr_t rxStartTime;                 //!<        The time needed to start RX in order to receive the packet
   uint16_t rxListenTime;               //!<        The time needed to listen in order to receive the packet. 0: No AUX packet
   uint8_t channelNo;                   //!<        The channel index used for secondary advertising
   uint8_t phyMode;                     //!< \brief PHY to use on secondary channel:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        Others: <i>Reserved</i>
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5ScannerPar
//! @{
//! Parameter structure for Bluetooth 5 scanner (CMD_BLE5_SCANNER)

struct __RFC_STRUCT rfc_ble5ScannerPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t scanFilterPolicy:1;       //!< \brief Scanning filter policy regarding advertiser address<br>
                                        //!<        0: Accept all advertisement packets<br>
                                        //!<        1: Accept only advertisement packets from devices where the advertiser's address
                                        //!<        is in the White list.
      uint8_t bActiveScan:1;            //!< \brief 0: Passive scan<br>
                                        //!<        1: Active scan
      uint8_t deviceAddrType:1;         //!<        The type of the device address -- public (0) or random (1)
      uint8_t rpaFilterPolicy:1;        //!< \brief Filter policy for initA of ADV_DIRECT_IND messages<br>
                                        //!<        0: Accept only initA that matches own address<br>
                                        //!<        1: Also accept all resolvable private addresses
      uint8_t bStrictLenFilter:1;       //!< \brief 0: Accept any packet with a valid advertising packet length<br>
                                        //!<        1: Discard messages with illegal length for the given packet type
      uint8_t bAutoWlIgnore:1;          //!< \brief 0: Do not set ignore bit in white list from radio CPU for legacy packets<br>
                                        //!<        1: Automatically set ignore bit in white list for legacy packets
      uint8_t bEndOnRpt:1;              //!< \brief 0: Continue scanner operation after each reporting ADV*_IND or sending SCAN_RSP<br>
                                        //!<        1: End scanner operation after each reported ADV*_IND and potentially SCAN_RSP
      uint8_t rpaMode:1;                //!< \brief Resolvable private address mode<br>
                                        //!<        0: Normal operation<br>
                                        //!<        1: Use white list for a received RPA regardless of filter policy
   } scanConfig;
   uint16_t randomState;                //!<        State for pseudo-random number generation used in backoff procedure
   uint16_t backoffCount;               //!<        Parameter <i>backoffCount</i> used in backoff procedure, cf. Bluetooth spec
   struct {
      uint8_t logUpperLimit:4;          //!<        Binary logarithm of parameter upperLimit used in scanner backoff procedure
      uint8_t bLastSucceeded:1;         //!< \brief 1 if the last SCAN_RSP was successfully received and <code>upperLimit</code>
                                        //!<        not changed
      uint8_t bLastFailed:1;            //!< \brief 1 if reception of the last SCAN_RSP failed and <code>upperLimit</code> was not
                                        //!<        changed
   } backoffPar;
   struct {
      uint8_t bCheckAdi:1;              //!< \brief 0: Do not perform ADI filtering<br>
                                        //!<        1: Perform ADI filtering on packets where ADI is present
      uint8_t bAutoAdiUpdate:1;         //!< \brief 0: Do not update ADI entries in radio CPU using legacy mode (recommended)<br>
                                        //!<        1: Legacy mode: Automatically update ADI entry for received packets with
                                        //!<        AdvDataInfo after first occurrence
      uint8_t bApplyDuplicateFiltering:1;//!< \brief 0: Do not apply duplicate filtering based on device address for extended
                                        //!<        advertiser packets (recommended)<br>
                                        //!<        1: Apply duplicate filtering based on device address for extended advertiser
                                        //!<        packets with no ADI field
      uint8_t bAutoWlIgnore:1;          //!< \brief 0: Do not set ignore bit in white list from radio CPU for extended advertising packets<br>
                                        //!<        1: Automatically set ignore bit in white list for extended advertising packets
      uint8_t bAutoAdiProcess:1;        //!< \brief 0: Do not use automatic ADI processing<br>
                                        //!<        1: Automatically update ADI entry for received packets so that only the same
                                        //!<        ADI is accepted for the rest of the chain and the SID/DID combination is
                                        //!<        ignored after the entire chain is received.
      uint8_t bExclusiveSid:1;          //!< \brief 0: Set <code>adiStatus.state</code> to 0 when command starts so that all
                                        //!<        valid SIDs are accepted<br>
                                        //!<        1: Do not modify adiStatus.state when command starts<br>
   } extFilterConfig;
   struct {
      uint8_t lastAcceptedSid:4;        //!<        Indication of SID of last successfully received packet that was not ignored
      uint8_t state:3;                  //!< \brief 0: No extended packet received, or last extended packet didn't have an ADI;
                                        //!<        <code>lastAcceptedSid</code> field is not valid<br>
                                        //!<        1: A message with ADI has been received, but no chain is under reception;
                                        //!<        ADI filtering to be performed normally<br>
                                        //!<        2: A message with SID as given in <code>lastAcceptedSid</code> has been
                                        //!<        received, and chained messages are still pending. Messages without this
                                        //!<        SID will be ignored<br>
                                        //!<        3: An AUX_SCAN_RSP message has been received after receiving messages with SID
                                        //!<        as given in <code>lastAcceptedSid</code>, and chained messages are
                                        //!<        pending. Messages with an ADI field will be ignored.<br>
                                        //!<        4: A message with no ADI has been received, and chained messages are still
                                        //!<        pending. Messages with an ADI field will be ignored.<br>
                                        //!<        Others: <i>Reserved</i>
   } adiStatus;
   uint8_t __dummy0;
   uint16_t __dummy1;
   uint16_t* pDeviceAddress;            //!< \brief Pointer (with least significant bit set to 0) to device address used for this device.
                                        //!<        If least significant bit is 1, the address type given by
                                        //!<        <code>scanConfig.deviceAddrType</code> is inverted.
   rfc_bleWhiteListEntry_t *pWhiteList; //!<        Pointer to white list
   rfc_ble5AdiEntry_t *pAdiList;        //!<        Pointer to advDataInfo list
   uint16_t maxWaitTimeForAuxCh;        //!< \brief Maximum wait time for switching to secondary scanning withing the command. If the time
                                        //!<        to the start of the event is greater than this, the command will end with BLE_DONE_AUX.
                                        //!<        If it is smaller, the radio will automatically switch to the correct channel and PHY.
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } timeoutTrigger;                    //!<        Trigger that causes the device to stop receiving as soon as allowed
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to stop receiving as soon as allowed
   ratmr_t timeoutTime;                 //!< \brief Time used together with <code>timeoutTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_RXTIMEOUT
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_ENDED
   ratmr_t rxStartTime;                 //!<        The time needed to start RX in order to receive the packet
   uint16_t rxListenTime;               //!<        The time needed to listen in order to receive the packet. 0: No AUX packet
   uint8_t channelNo;                   //!<        The channel index used for secondary advertising
   uint8_t phyMode;                     //!< \brief PHY to use on secondary channel:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        Others: <i>Reserved</i>
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5InitiatorPar
//! @{
//! Parameter structure for Bluetooth 5 initiator (CMD_BLE5_INITIATOR)

struct __RFC_STRUCT rfc_ble5InitiatorPar_s {
   dataQueue_t* pRxQ;                   //!<        Pointer to receive queue
   struct {
      uint8_t bAutoFlushIgnored:1;      //!<        If 1, automatically remove ignored packets from Rx queue
      uint8_t bAutoFlushCrcErr:1;       //!<        If 1, automatically remove packets with CRC error from Rx queue
      uint8_t bAutoFlushEmpty:1;        //!<        If 1, automatically remove empty packets from Rx queue
      uint8_t bIncludeLenByte:1;        //!<        If 1, include the received length byte in the stored packet; otherwise discard it
      uint8_t bIncludeCrc:1;            //!<        If 1, include the received CRC field in the stored packet; otherwise discard it
      uint8_t bAppendRssi:1;            //!<        If 1, append an RSSI byte to the packet in the Rx queue
      uint8_t bAppendStatus:1;          //!<        If 1, append a status word to the packet in the Rx queue
      uint8_t bAppendTimestamp:1;       //!<        If 1, append a timestamp to the packet in the Rx queue
   } rxConfig;                          //!<        Configuration bits for the receive queue entries
   struct {
      uint8_t bUseWhiteList:1;          //!< \brief Initiator filter policy<br>
                                        //!<        0: Use specific peer address<br>
                                        //!<        1: Use white list
      uint8_t bDynamicWinOffset:1;      //!<        1: Use dynamic WinOffset insertion
      uint8_t deviceAddrType:1;         //!<        The type of the device address -- public (0) or random (1)
      uint8_t peerAddrType:1;           //!<        The type of the peer address -- public (0) or random (1)
      uint8_t bStrictLenFilter:1;       //!< \brief 0: Accept any packet with a valid advertising packet length<br>
                                        //!<        1: Discard messages with illegal length for the given packet type
      uint8_t chSel:1;                  //!< \brief 0: Do not report support of Channel Selection Algorithm #2 in CONNECT_IND<br>
                                        //!<        1: Report support of Channel Selection Algorithm #2 in CONNECT_IND
   } initConfig;
   uint16_t randomState;                //!<        State for pseudo-random number generation used in backoff procedure
   uint16_t backoffCount;               //!<        Parameter <i>backoffCount</i> used in backoff procedure, cf. Bluetooth spec
   struct {
      uint8_t logUpperLimit:4;          //!<        Binary logarithm of parameter upperLimit used in scanner backoff procedure
      uint8_t bLastSucceeded:1;         //!< \brief 1 if the last SCAN_RSP was successfully received and <code>upperLimit</code>
                                        //!<        not changed
      uint8_t bLastFailed:1;            //!< \brief 1 if reception of the last SCAN_RSP failed and <code>upperLimit</code> was not
                                        //!<        changed
   } backoffPar;
   uint8_t connectReqLen;               //!<        Size of connect request data
   uint8_t* pConnectReqData;            //!<        Pointer to buffer containing LLData to go in the CONNECT_IND or AUX_CONNECT_REQ packet
   uint16_t* pDeviceAddress;            //!< \brief Pointer (with least significant bit set to 0) to device address used for this device.
                                        //!<        If least significant bit is 1, the address type given by
                                        //!<        <code>initConfig.deviceAddrType</code> is inverted.
   rfc_bleWhiteListEntry_t *pWhiteList; //!< \brief Pointer (with least significant bit set to 0)  to white list or peer address. If least
                                        //!<        significant bit is 1, the address type given by <code>initConfig.peerAddrType</code>
                                        //!<        is inverted.
   ratmr_t connectTime;                 //!< \brief Indication of timer value of the first possible start time of the first connection event.
                                        //!<        Set to the calculated value if a connection is made and to the next possible connection
                                        //!<        time if not.
   uint16_t maxWaitTimeForAuxCh;        //!< \brief Maximum wait time for switching to secondary scanning withing the command. If the time
                                        //!<        to the start of the event is greater than this, the command will end with BLE_DONE_AUX.
                                        //!<        If it is smaller, the radio will automatically switch to the correct channel and PHY.
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } timeoutTrigger;                    //!<        Trigger that causes the device to stop receiving as soon as allowed
   struct {
      uint8_t triggerType:4;            //!<        The type of trigger
      uint8_t bEnaCmd:1;                //!< \brief 0: No alternative trigger command<br>
                                        //!<        1: CMD_TRIGGER can be used as an alternative trigger
      uint8_t triggerNo:2;              //!<        The trigger number of the CMD_TRIGGER command that triggers this action
      uint8_t pastTrig:1;               //!< \brief 0: A trigger in the past is never triggered, or for start of commands, give an error<br>
                                        //!<        1: A trigger in the past is triggered as soon as possible
   } endTrigger;                        //!<        Trigger that causes the device to stop receiving as soon as allowed
   ratmr_t timeoutTime;                 //!< \brief Time used together with <code>timeoutTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_RXTIMEOUT
   ratmr_t endTime;                     //!< \brief Time used together with <code>endTrigger</code> that causes the device to stop
                                        //!<        receiving as soon as allowed, ending with BLE_DONE_ENDED
   ratmr_t rxStartTime;                 //!<        The time needed to start RX in order to receive the packet
   uint16_t rxListenTime;               //!<        The time needed to listen in order to receive the packet. 0: No AUX packet
   uint8_t channelNo;                   //!<        The channel index used for secondary advertising
   uint8_t phyMode;                     //!< \brief PHY to use on secondary channel:<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded<br>
                                        //!<        Others: <i>Reserved</i>
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleMasterSlaveOutput
//! @{
//! Output structure for master and slave (CMD_BLE_MASTER/CMD_BLE_SLAVE/CMD_BLE5_MASTER/CMD_BLE5_SLAVE)

struct __RFC_STRUCT rfc_bleMasterSlaveOutput_s {
   uint8_t nTx;                         //!< \brief Total number of packets (including auto-empty and retransmissions) that have been
                                        //!<        transmitted
   uint8_t nTxAck;                      //!<        Total number of transmitted packets (including auto-empty) that have been ACK'ed
   uint8_t nTxCtrl;                     //!<        Number of unique LL control packets from the Tx queue that have been transmitted
   uint8_t nTxCtrlAck;                  //!<        Number of LL control packets from the Tx queue that have been finished (ACK'ed)
   uint8_t nTxCtrlAckAck;               //!< \brief Number of LL control packets that have been ACK'ed and where an ACK has been sent in
                                        //!<        response
   uint8_t nTxRetrans;                  //!<        Number of retransmissions that has been done
   uint8_t nTxEntryDone;                //!<        Number of packets from the Tx queue that have been finished (ACK'ed)
   uint8_t nRxOk;                       //!<        Number of packets that have been received with payload, CRC OK and not ignored
   uint8_t nRxCtrl;                     //!<        Number of LL control packets that have been received with CRC OK and not ignored
   uint8_t nRxCtrlAck;                  //!< \brief Number of LL control packets that have been received with CRC OK and not ignored, and
                                        //!<        then ACK'ed
   uint8_t nRxNok;                      //!<        Number of packets that have been received with CRC error
   uint8_t nRxIgnored;                  //!< \brief Number of packets that have been received with CRC OK and ignored due to repeated
                                        //!<        sequence number
   uint8_t nRxEmpty;                    //!<        Number of packets that have been received with CRC OK and no payload
   uint8_t nRxBufFull;                  //!<        Number of packets that have been received and discarded due to lack of buffer space
   int8_t lastRssi;                     //!<        RSSI of last received packet (signed)
   struct {
      uint8_t bTimeStampValid:1;        //!<        1 if a valid time stamp has been written to timeStamp; 0 otherwise
      uint8_t bLastCrcErr:1;            //!<        1 if the last received packet had CRC error; 0 otherwise
      uint8_t bLastIgnored:1;           //!<        1 if the last received packet with CRC OK was ignored; 0 otherwise
      uint8_t bLastEmpty:1;             //!<        1 if the last received packet with CRC OK was empty; 0 otherwise
      uint8_t bLastCtrl:1;              //!<        1 if the last received packet with CRC OK was an LL control packet; 0 otherwise
      uint8_t bLastMd:1;                //!<        1 if the last received packet with CRC OK had MD = 1; 0 otherwise
      uint8_t bLastAck:1;               //!< \brief 1 if the last received packet with CRC OK was an ACK of a transmitted packet;
                                        //!<        0 otherwise
   } pktStatus;                         //!<        Status of received packets
   ratmr_t timeStamp;                   //!<        Slave operation: Time stamp of first received packet
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleAdvOutput
//! @{
//! Output structure for advertiser (CMD_BLE_ADV* and CMD_BLE5_ADV*)

struct __RFC_STRUCT rfc_bleAdvOutput_s {
   uint16_t nTxAdvInd;                  //!<        Number of ADV*_IND packets completely transmitted
   uint8_t nTxScanRsp;                  //!<        Number of  AUX_SCAN_RSP or SCAN_RSP packets transmitted
   uint8_t nRxScanReq;                  //!<        Number of AUX_SCAN_REQ or SCAN_REQ packets received OK and not ignored
   uint8_t nRxConnectReq;               //!<        Number of AUX_CONNECT_REQ or CONNECT_IND (CONNECT_REQ) packets received OK and not ignored
   uint8_t nTxConnectRsp;               //!<        Number of  AUX_CONNECT_RSP packets transmitted
   uint16_t nRxNok;                     //!<        Number of packets received with CRC error
   uint16_t nRxIgnored;                 //!<        Number of packets received with CRC OK, but ignored
   uint8_t nRxBufFull;                  //!<        Number of packets received that did not fit in Rx queue
   int8_t lastRssi;                     //!<        The RSSI of the last received packet (signed)
   ratmr_t timeStamp;                   //!<        Time stamp of the last received packet
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleScannerOutput
//! @{
//! Output structure for legacy scanner (CMD_BLE_SCANNER)

struct __RFC_STRUCT rfc_bleScannerOutput_s {
   uint16_t nTxScanReq;                 //!<        Number of transmitted SCAN_REQ packets
   uint16_t nBackedOffScanReq;          //!<        Number of SCAN_REQ packets not sent due to backoff procedure
   uint16_t nRxAdvOk;                   //!<        Number of ADV*_IND packets received with CRC OK and not ignored
   uint16_t nRxAdvIgnored;              //!<        Number of ADV*_IND packets received with CRC OK, but ignored
   uint16_t nRxAdvNok;                  //!<        Number of ADV*_IND packets received with CRC error
   uint16_t nRxScanRspOk;               //!<        Number of SCAN_RSP packets received with CRC OK and not ignored
   uint16_t nRxScanRspIgnored;          //!<        Number of SCAN_RSP packets received with CRC OK, but ignored
   uint16_t nRxScanRspNok;              //!<        Number of SCAN_RSP packets received with CRC error
   uint8_t nRxAdvBufFull;               //!<        Number of ADV*_IND packets received that did not fit in Rx queue
   uint8_t nRxScanRspBufFull;           //!<        Number of SCAN_RSP packets received that did not fit in Rx queue
   int8_t lastRssi;                     //!<        The RSSI of the last received packet (signed)
   uint8_t __dummy0;
   ratmr_t timeStamp;                   //!<        Time stamp of the last successfully received ADV*_IND packet that was not ignored
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleInitiatorOutput
//! @{
//! Output structure for legacy initiator (CMD_BLE_INITIATOR)

struct __RFC_STRUCT rfc_bleInitiatorOutput_s {
   uint8_t nTxConnectReq;               //!<        Number of transmitted CONNECT_IND (CONNECT_REQ) packets
   uint8_t nRxAdvOk;                    //!<        Number of ADV*_IND packets received with CRC OK and not ignored
   uint16_t nRxAdvIgnored;              //!<        Number of ADV*_IND packets received with CRC OK, but ignored
   uint16_t nRxAdvNok;                  //!<        Number of ADV*_IND packets received with CRC error
   uint8_t nRxAdvBufFull;               //!<        Number of ADV*_IND packets received that did not fit in Rx queue
   int8_t lastRssi;                     //!<        The RSSI of the last received packet (signed)
   ratmr_t timeStamp;                   //!<        Time stamp of the received ADV*_IND packet that caused transmission of CONNECT_IND (CONNECT_REQ)
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5ScanInitOutput
//! @{
//! Output structure for BLE scanner and initiator (CMD_BLE5_SCANNER and CMD_BLE5_INITIATOR)

struct __RFC_STRUCT rfc_ble5ScanInitOutput_s {
   uint16_t nTxReq;                     //!<        Number of transmitted AUX_SCAN_REQ, SCAN_REQ, AUX_CONNECT_REQ, or CONNECT_IND packets
   uint16_t nBackedOffReq;              //!<        Number of AUX_SCAN_REQ, SCAN_REQ, or AUX_CONNECT_REQ packets not sent due to backoff procedure
   uint16_t nRxAdvOk;                   //!<        Number of ADV*_IND packets received with CRC OK and not ignored
   uint16_t nRxAdvIgnored;              //!<        Number of ADV*_IND packets received with CRC OK, but ignored
   uint16_t nRxAdvNok;                  //!<        Number of ADV*_IND packets received with CRC error
   uint16_t nRxRspOk;                   //!<        Number of AUX_SCAN_RSP, SCAN_RSP, or AUX_CONNECT_RSP packets received with CRC OK and not ignored
   uint16_t nRxRspIgnored;              //!<        Number of AUX_SCAN_RSP, SCAN_RSP, or AUX_CONNECT_RSP packets received with CRC OK, but ignored
   uint16_t nRxRspNok;                  //!<        Number of AUX_SCAN_RSP, SCAN_RSP, or AUX_CONNECT_RSP packets received with CRC error
   uint8_t nRxAdvBufFull;               //!<        Number of ADV*_IND packets received that did not fit in Rx queue
   uint8_t nRxRspBufFull;               //!<        Number of AUX_SCAN_RSP, SCAN_RSP, or AUX_CONNECT_RSP packets received that did not fit in Rx queue
   int8_t lastRssi;                     //!<        The RSSI of the last received packet (signed)
   uint8_t __dummy0;
   ratmr_t timeStamp;                   //!<        Time stamp of the last successfully received *ADV*_IND packet that was not ignored
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleGenericRxOutput
//! @{
//! Output structure for generic Rx (CMD_BLE_GENERIC_RX and CMD_BLE5_GENERIC_RX)

struct __RFC_STRUCT rfc_bleGenericRxOutput_s {
   uint16_t nRxOk;                      //!<        Number of packets received with CRC OK
   uint16_t nRxNok;                     //!<        Number of packets received with CRC error
   uint16_t nRxBufFull;                 //!<        Number of packets that have been received and discarded due to lack of buffer space
   int8_t lastRssi;                     //!<        The RSSI of the last received packet (signed)
   uint8_t __dummy0;
   ratmr_t timeStamp;                   //!<        Time stamp of the last received packet
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleTxTestOutput
//! @{
//! Output structure for Tx test (CMD_BLE_TX_TEST and CMD_BLE5_TX_TEST)

struct __RFC_STRUCT rfc_bleTxTestOutput_s {
   uint16_t nTx;                        //!<        Number of packets transmitted
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5ExtAdvEntry
//! @{
//! Common Extended Packet Entry Format

struct __RFC_STRUCT rfc_ble5ExtAdvEntry_s {
   struct {
      uint8_t length:6;                 //!<        Extended header length
      uint8_t advMode:2;                //!< \brief Advertiser mode as defined in BLE:<br>
                                        //!<        0: Non-connectable, non-scannable<br>
                                        //!<        1: Connectable, non-scannable<br>
                                        //!<        2: Non-connectable, scannable<br>
                                        //!<        3: <i>Reserved</i>
   } extHdrInfo;
   uint8_t extHdrFlags;                 //!<        Extended header flags as defined in BLE
   struct {
      uint8_t bSkipAdvA:1;              //!< \brief 0: AdvA is present in extended payload if configured in
                                        //!<        <code>extHdrFlags</code><br>
                                        //!<        1: AdvA is inserted automatically from command structure if configured in
                                        //!<        <code>extHdrFlags</code> and is omitted from extended header
      uint8_t bSkipTargetA:1;           //!< \brief 0: TargetA is present in extended payload if configured in
                                        //!<        <code>extHdrFlags</code>. For response messages, the value is replaced
                                        //!<        by the received address when sending<br>
                                        //!<        1: TargetA is inserted automatically from command structure or received
                                        //!<        address if configured in <code>extHdrFlags</code> and is omitted from
                                        //!<        extended header. Not supported with CMD_BLE5_ADV_EXT.
      uint8_t deviceAddrType:1;         //!< \brief If <code>bSkipAdvA</code> = 0: The type of the device address in extended
                                        //!<        header buffer -- public (0) or random (1)
      uint8_t targetAddrType:1;         //!< \brief If <code>bSkipAdvA</code> = 0: The type of the target address in extended
                                        //!<        header buffer -- public (0) or random (1)
   } extHdrConfig;
   uint8_t advDataLen;                  //!<        Size of payload buffer
   uint8_t* pExtHeader;                 //!< \brief Pointer to buffer containing extended header. If no fields except extended
                                        //!<        header flags, automatic advertiser address, or automatic target address are
                                        //!<        present, pointer may be NULL.
   uint8_t* pAdvData;                   //!< \brief Pointer to buffer containing advData. If <code>advDataLen</code> = 0,
                                        //!<        pointer may be NULL.
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleWhiteListEntry
//! @{
//! White list entry structure

struct __RFC_STRUCT rfc_bleWhiteListEntry_s {
   uint8_t size;                        //!<        Number of while list entries. Used in the first entry of the list only
   struct {
      uint8_t bEnable:1;                //!<        1 if the entry is in use, 0 if the entry is not in use
      uint8_t addrType:1;               //!<        The type address in the entry -- public (0) or random (1)
      uint8_t bWlIgn:1;                 //!< \brief 1 if the entry is to be ignored by a scanner if the AdvDataInfo
                                        //!<        field is not present, 0 otherwise. Used to mask out entries that
                                        //!<        have already been scanned and reported.
      uint8_t :1;
      uint8_t bPrivIgn:1;               //!< \brief 1 if the entry is to be ignored as part of a privacy algorithm,
                                        //!<        0 otherwise
   } conf;
   uint16_t address;                    //!<        Least significant 16 bits of the address contained in the entry
   uint32_t addressHi;                  //!<        Most significant 32 bits of the address contained in the entry
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5AdiEntry
//! @{
//! AdvDataInfo list entry structure

struct __RFC_STRUCT rfc_ble5AdiEntry_s {
   struct {
      uint16_t advDataId:12;            //!< \brief If <code>bValid</code> = 1: Last Advertising Data ID (DID) for the
                                        //!<        Advertising Set ID (SID) corresponding to the entry number in the array
      uint16_t mode:2;                  //!< \brief 0: Entry is invalid (always receive packet with the given SID)<br>
                                        //!<        1: Entry is valid (ignore packets with the given SID where DID equals
                                        //!<        <code>advDataId</code>)<br>
                                        //!<        2: Entry is blocked (always ignore packet with the given SID)<br>
                                        //!<        3: <i>Reserved</i>
   } advDataInfo;
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup bleRxStatus
//! @{
//! Receive status byte that may be appended to message in receive buffer for legacy commands

struct __RFC_STRUCT rfc_bleRxStatus_s {
   struct {
      uint8_t channel:6;                //!< \brief The channel on which the packet was received, provided channel is in the range
                                        //!<        0--39; otherwise 0x3F
      uint8_t bIgnore:1;                //!<        1 if the packet is marked as ignored, 0 otherwise
      uint8_t bCrcErr:1;                //!<        1 if the packet was received with CRC error, 0 otherwise
   } status;
} __RFC_STRUCT_ATTR;

//! @}

//! \addtogroup ble5RxStatus
//! @{
//! Receive status field that may be appended to message in receive buffer for Bluetooth 5 commands

struct __RFC_STRUCT rfc_ble5RxStatus_s {
   struct {
      uint16_t channel:6;               //!< \brief The channel on which the packet was received, provided channel is in the range
                                        //!<        0--39; otherwise 0x3F
      uint16_t bIgnore:1;               //!<        1 if the packet is marked as ignored, 0 otherwise
      uint16_t bCrcErr:1;               //!<        1 if the packet was received with CRC error, 0 otherwise
      uint16_t phyMode:2;               //!< \brief The PHY on which the packet was received<br>
                                        //!<        0: 1 Mbps<br>
                                        //!<        1: 2 Mbps<br>
                                        //!<        2: Coded, S = 8 (125 kbps)<br>
                                        //!<        3: Coded, S = 2 (500 kbps)
   } status;
} __RFC_STRUCT_ATTR;

//! @}

//! @}
//! @}
#endif
