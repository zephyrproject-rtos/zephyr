/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef I2C_H_
#define I2C_H_

//*****************************************************************************
//
//! \addtogroup i2c_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/eusci.h>

#define EUSCI_B_I2C_NO_AUTO_STOP                           EUSCI_B_CTLW1_ASTP_0
#define EUSCI_B_I2C_SET_BYTECOUNT_THRESHOLD_FLAG           EUSCI_B_CTLW1_ASTP_1
#define EUSCI_B_I2C_SEND_STOP_AUTOMATICALLY_ON_BYTECOUNT_THRESHOLD  \
                                                           EUSCI_B_CTLW1_ASTP_2

#define EUSCI_B_I2C_SET_DATA_RATE_1MBPS                                 1000000
#define EUSCI_B_I2C_SET_DATA_RATE_400KBPS                                400000
#define EUSCI_B_I2C_SET_DATA_RATE_100KBPS                                100000

#define EUSCI_B_I2C_CLOCKSOURCE_ACLK                   EUSCI_B_CTLW0_SSEL__ACLK
#define EUSCI_B_I2C_CLOCKSOURCE_SMCLK                 EUSCI_B_CTLW0_SSEL__SMCLK

#define EUSCI_B_I2C_OWN_ADDRESS_OFFSET0                                    0x00
#define EUSCI_B_I2C_OWN_ADDRESS_OFFSET1                                    0x02
#define EUSCI_B_I2C_OWN_ADDRESS_OFFSET2                                    0x04
#define EUSCI_B_I2C_OWN_ADDRESS_OFFSET3                                    0x06

#define EUSCI_B_I2C_OWN_ADDRESS_DISABLE                                    0x00
#define EUSCI_B_I2C_OWN_ADDRESS_ENABLE                      EUSCI_B_I2COA0_OAEN

#define EUSCI_B_I2C_TRANSMIT_MODE                              EUSCI_B_CTLW0_TR
#define EUSCI_B_I2C_RECEIVE_MODE                                           0x00

#define EUSCI_B_I2C_NAK_INTERRUPT                             EUSCI_B_IE_NACKIE
#define EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT                   EUSCI_B_IE_ALIE
#define EUSCI_B_I2C_STOP_INTERRUPT                             EUSCI_B_IE_STPIE
#define EUSCI_B_I2C_START_INTERRUPT                            EUSCI_B_IE_STTIE
#define EUSCI_B_I2C_TRANSMIT_INTERRUPT0                        EUSCI_B_IE_TXIE0
#define EUSCI_B_I2C_TRANSMIT_INTERRUPT1                        EUSCI_B_IE_TXIE1
#define EUSCI_B_I2C_TRANSMIT_INTERRUPT2                        EUSCI_B_IE_TXIE2
#define EUSCI_B_I2C_TRANSMIT_INTERRUPT3                        EUSCI_B_IE_TXIE3
#define EUSCI_B_I2C_RECEIVE_INTERRUPT0                         EUSCI_B_IE_RXIE0
#define EUSCI_B_I2C_RECEIVE_INTERRUPT1                         EUSCI_B_IE_RXIE1
#define EUSCI_B_I2C_RECEIVE_INTERRUPT2                         EUSCI_B_IE_RXIE2
#define EUSCI_B_I2C_RECEIVE_INTERRUPT3                         EUSCI_B_IE_RXIE3
#define EUSCI_B_I2C_BIT9_POSITION_INTERRUPT                   EUSCI_B_IE_BIT9IE
#define EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT               EUSCI_B_IE_CLTOIE
#define EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT                    EUSCI_B_IE_BCNTIE

#define EUSCI_B_I2C_BUS_BUSY                                EUSCI_B_STATW_BBUSY
#define EUSCI_B_I2C_BUS_NOT_BUSY                                           0x00

#define EUSCI_B_I2C_STOP_SEND_COMPLETE                                     0x00
#define EUSCI_B_I2C_SENDING_STOP                            EUSCI_B_CTLW0_TXSTP

#define EUSCI_B_I2C_START_SEND_COMPLETE                                    0x00
#define EUSCI_B_I2C_SENDING_START                           EUSCI_B_CTLW0_TXSTT

//*****************************************************************************
//
//!     ypedef eUSCI_I2C_MasterConfig
//! \brief Type definition for \link _eUSCI_I2C_MasterConfig \endlink structure
//!
//! \struct _eUSCI_I2C_MasterConfig
//! \brief Configuration structure for master mode in the \b I2C module. See
//!          \link I2C_initMaster \endlink for parameter documentation.
//
//*****************************************************************************
typedef struct
{
    uint_fast8_t selectClockSource;
    uint32_t i2cClk;
    uint32_t dataRate;
    uint_fast8_t byteCounterThreshold;
    uint_fast8_t autoSTOPGeneration;
} eUSCI_I2C_MasterConfig;


//*****************************************************************************
//
//! Initializes the I2C Master block.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//! \param config Configuration structure for I2C master mode
//!
//! <hr>
//! <b>Configuration options for \link eUSCI_I2C_MasterConfig \endlink structure.</b>
//! <hr>
//!
//! \param selectClockSource is the clock source.
//!         Valid values are
//!         - \b EUSCI_B_I2C_CLOCKSOURCE_ACLK
//!         - \b EUSCI_B_I2C_CLOCKSOURCE_SMCLK
//! \param i2cClk is the rate of the clock supplied to the I2C module
//!                   (the frequency in Hz of the clock source specified in
//!                     selectClockSource).
//! \param dataRate set up for selecting data transfer rate.
//!         Valid values are
//!         - \b EUSCI_B_I2C_SET_DATA_RATE_1MBPS
//!         - \b EUSCI_B_I2C_SET_DATA_RATE_400KBPS
//!         - \b EUSCI_B_I2C_SET_DATA_RATE_100KBPS
//! \param byteCounterThreshold sets threshold for automatic STOP or UCSTPIFG
//! \param autoSTOPGeneration sets up the STOP condition generation.
//!         Valid values are
//!         - \b EUSCI_B_I2C_NO_AUTO_STOP
//!         - \b EUSCI_B_I2C_SET_BYTECOUNT_THRESHOLD_FLAG
//!         - \b EUSCI_B_I2C_SEND_STOP_AUTOMATICALLY_ON_BYTECOUNT_THRESHOLD
//!
//! This function initializes operation of the I2C Master block.  Upon
//! successful initialization of the I2C block, this function will have set the
//! bus speed for the master; however I2C module is still disabled till
//! I2C_enableModule is invoked
//!
//! Modified bits are \b UCMST,UCMODE_3,\b UCSYNC of \b UCBxCTL0 register
//!                   \b UCSSELx, \b UCSWRST, of \b UCBxCTL1 register
//!                   \b UCBxBR0 and \b UCBxBR1 registers
//! \return None.
//
//*****************************************************************************
extern void I2C_initMaster(uint32_t moduleInstance,
        const eUSCI_I2C_MasterConfig *config);

//*****************************************************************************
//
//! Initializes the I2C Slave block.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param slaveAddress 7-bit or 10-bit slave address
//! \param slaveAddressOffset Own address Offset referred to- 'x' value of
//!     UCBxI2COAx. Valid values are:
//!                  - \b EUSCI_B_I2C_OWN_ADDRESS_OFFSET0,
//!                  - \b EUSCI_B_I2C_OWN_ADDRESS_OFFSET1,
//!                  - \b EUSCI_B_I2C_OWN_ADDRESS_OFFSET2,
//!                  - \b EUSCI_B_I2C_OWN_ADDRESS_OFFSET3
//! \param slaveOwnAddressEnable selects if the specified address is enabled
//!     or disabled. Valid values are:
//!                     - \b EUSCI_B_I2C_OWN_ADDRESS_DISABLE,
//!                     - \b EUSCI_B_I2C_OWN_ADDRESS_ENABLE
//!
//! This function initializes operation of the I2C as a Slave mode.  Upon
//! successful initialization of the I2C blocks, this function will have set
//! the slave address but the I2C module is still disabled till
//! I2C_enableModule is invoked.
//!
//! The parameter slaveAddress is the value that will be compared against the
//! slave address sent by an I2C master.
//!
//! Modified bits are \b UCMODE_3, \b UCSYNC of \b UCBxCTL0 register
//!                   \b UCSWRST of \b UCBxCTL1 register
//!                   \b UCBxI2COA register
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_initSlave(uint32_t moduleInstance, uint_fast16_t slaveAddress,
        uint_fast8_t slaveAddressOffset, uint32_t slaveOwnAddressEnable);

//*****************************************************************************
//
//! Enables the I2C block.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! This will enable operation of the I2C block.
//! Modified bits are \b UCSWRST of \b UCBxCTL1 register.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_enableModule(uint32_t moduleInstance);

//*****************************************************************************
//
//! Disables the I2C block.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! This will disable operation of the I2C block.
//! Modified bits are \b UCSWRST of \b UCBxCTL1 register.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_disableModule(uint32_t moduleInstance);

//*****************************************************************************
//
//! Sets the address that the I2C Master will place on the bus.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param slaveAddress 7-bit or 10-bit slave address
//!
//! This function will set the address that the I2C Master will place on the
//! bus when initiating a transaction.
//! Modified register is  \b UCBxI2CSA register
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_setSlaveAddress(uint32_t moduleInstance,
        uint_fast16_t slaveAddress);

//*****************************************************************************
//
//! Sets the mode of the I2C device
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param mode indicates whether module is in transmit/receive mode
//!     - \b EUSCI_B_I2C_TRANSMIT_MODE
//!     - \b EUSCI_B_I2C_RECEIVE_MODE [Default value]
//!
//! Modified bits are \b UCTR of \b UCBxCTL1 register
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_setMode(uint32_t moduleInstance, uint_fast8_t mode);

//*****************************************************************************
//
//! \brief Gets the mode of the I2C device
//!
//! Current I2C transmit/receive mode.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! Modified bits are \b UCTR of \b UCBxCTL1 register.
//!
//! \return None
//!         Return one of the following:
//!         - \b EUSCI_B_I2C_TRANSMIT_MODE
//!         - \b EUSCI_B_I2C_RECEIVE_MODE
//!         \n indicating the current mode
//
//*****************************************************************************
extern uint_fast8_t I2C_getMode(uint32_t moduleInstance);

//*****************************************************************************
//
//! Transmits a byte from the I2C Module.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param transmitData data to be transmitted from the I2C module
//!
//! This function will place the supplied data into I2C transmit data register
//! to start transmission
//! Modified register is \b UCBxTXBUF register
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_slavePutData(uint32_t moduleInstance, uint8_t transmitData);

//*****************************************************************************
//
//! Receives a byte that has been sent to the I2C Module.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! This function reads a byte of data from the I2C receive data Register.
//!
//! \return Returns the byte received from by the I2C module, cast as an
//! uint8_t.
//! Modified bit is \b UCBxRXBUF register
//
//*****************************************************************************
extern uint8_t I2C_slaveGetData(uint32_t moduleInstance);

//*****************************************************************************
//
//! Indicates whether or not the I2C bus is busy.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function returns an indication of whether or not the I2C bus is
//! busy.This function checks the status of the bus via UCBBUSY bit in
//! UCBxSTAT register.
//!
//! \return Returns EUSCI_B_I2C_BUS_BUSY if the I2C Master is busy; otherwise,
//! returns EUSCI_B_I2C_BUS_NOT_BUSY.
//
//*****************************************************************************
extern uint8_t I2C_isBusBusy(uint32_t moduleInstance);

//*****************************************************************************
//
//! Does single byte transmission from Master to Slave
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the data byte to be transmitted
//!
//! This function is used by the Master module to send a single byte.
//! This function
//! - Sends START
//! - Transmits the byte to the Slave
//! - Sends STOP
//!
//! Modified registers are \b UCBxIE, \b UCBxCTL1, \b UCBxIFG, \b UCBxTXBUF,
//! \b UCBxIE
//!
//! \return none
//
//*****************************************************************************
extern void I2C_masterSendSingleByte(uint32_t moduleInstance, uint8_t txData);

//*****************************************************************************
//
//! Does single byte transmission from Master to Slave with timeout
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the data byte to be transmitted
//! \param timeout is the amount of time to wait until giving up
//!
//! This function is used by the Master module to send a single byte.
//! This function
//! - Sends START
//! - Transmits the byte to the Slave
//! - Sends STOP
//!
//! Modified registers are \b UCBxIE, \b UCBxCTL1, \b UCBxIFG, \b UCBxTXBUF,
//! \b UCBxIE
//!
//! \return 0x01 or 0x00URE of the transmission process.
//
//*****************************************************************************
extern bool I2C_masterSendSingleByteWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout);

//*****************************************************************************
//
//! Starts multi-byte transmission from Master to Slave
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the first data byte to be transmitted
//!
//! This function is used by the Master module to send a single byte.
//! This function
//! - Sends START
//! - Transmits the first data byte of a multi-byte transmission to the Slave
//!
//! Modified registers are \b UCBxIE, \b UCBxCTL1, \b UCBxIFG, \b UCBxTXBUF,
//! \b UCBxIE
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_masterSendMultiByteStart(uint32_t moduleInstance,
        uint8_t txData);

//*****************************************************************************
//
//! Starts multi-byte transmission from Master to Slave with timeout
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the first data byte to be transmitted
//! \param timeout is the amount of time to wait until giving up
//!
//! This function is used by the Master module to send a single byte.
//! This function
//! - Sends START
//! - Transmits the first data byte of a multi-byte transmission to the Slave
//!
//! Modified registers are \b UCBxIE, \b UCBxCTL1, \b UCBxIFG, \b UCBxTXBUF,
//! \b UCBxIE
//!
//! \return 0x01 or 0x00URE of the transmission process.
//
//*****************************************************************************
extern bool I2C_masterSendMultiByteStartWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout);

//*****************************************************************************
//
//! Continues multi-byte transmission from Master to Slave
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the next data byte to be transmitted
//!
//! This function is used by the Master module continue each byte of a
//! multi-byte trasmission. This function
//! - Transmits each data byte of a multi-byte transmission to the Slave
//!
//! Modified registers are \b UCBxTXBUF
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_masterSendMultiByteNext(uint32_t moduleInstance,
        uint8_t txData);

//*****************************************************************************
//
//! Continues multi-byte transmission from Master to Slave with timeout
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the next data byte to be transmitted
//!
//! \param timeout is the amount of time to wait until giving up
//!
//! This function is used by the Master module continue each byte of a
//! multi-byte transmission. This function
//! - Transmits each data byte of a multi-byte transmission to the Slave
//!
//! Modified registers are \b UCBxTXBUF
//!
//! \return 0x01 or 0x00URE of the transmission process.
//
//*****************************************************************************
extern bool I2C_masterSendMultiByteNextWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout);

//*****************************************************************************
//
//! Finishes multi-byte transmission from Master to Slave
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the last data byte to be transmitted in a multi-byte
//! transmsission
//!
//! This function is used by the Master module to send the last byte and STOP.
//! This function
//! - Transmits the last data byte of a multi-byte transmission to the Slave
//! - Sends STOP
//!
//! Modified registers are \b UCBxTXBUF and \b UCBxCTL1.
//!
//! \return false if NAK occurred, false otherwise
//
//*****************************************************************************
extern bool I2C_masterSendMultiByteFinish(uint32_t moduleInstance,
        uint8_t txData);

//*****************************************************************************
//
//! Finishes multi-byte transmission from Master to Slave with timeout
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is the last data byte to be transmitted in a multi-byte
//! transmission
//! \param timeout is the amount of time to wait until giving up
//!
//! This function is used by the Master module to send the last byte and STOP.
//! This function
//! - Transmits the last data byte of a multi-byte transmission to the Slave
//! - Sends STOP
//!
//! Modified registers are \b UCBxTXBUF and \b UCBxCTL1.
//!
//! \return 0x01 or 0x00URE of the transmission process.
//
//*****************************************************************************
extern bool I2C_masterSendMultiByteFinishWithTimeout(uint32_t moduleInstance,
        uint8_t txData, uint32_t timeout);

//*****************************************************************************
//
//! Send STOP byte at the end of a multi-byte transmission from Master to Slave
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function is used by the Master module send STOP at the end of a
//! multi-byte transmission
//!
//! This function
//! - Send a STOP after current transmission is complete
//!
//! Modified bits are \b UCTXSTP bit of \b UCBxCTL1.
//! \return None.
//
//*****************************************************************************
extern void I2C_masterSendMultiByteStop(uint32_t moduleInstance);

//*****************************************************************************
//
//! Send STOP byte at the end of a multi-byte transmission from Master to Slave
//! with timeout
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param timeout is the amount of time to wait until giving up
//!
//! This function is used by the Master module send STOP at the end of a
//! multi-byte transmission
//!
//! This function
//! - Send a STOP after current transmission is complete
//!
//! Modified bits are \b UCTXSTP bit of \b UCBxCTL1.
//! \return 0x01 or 0x00URE of the transmission process.
//
//*****************************************************************************
extern bool I2C_masterSendMultiByteStopWithTimeout(uint32_t moduleInstance,
        uint32_t timeout);

//*****************************************************************************
//
//! Starts reception at the Master end
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function is used by the Master module initiate reception of a single
//! byte. This function
//! - Sends START
//!
//! Modified bits are \b UCTXSTT bit of \b UCBxCTL1.
//! \return None.
//
//*****************************************************************************
extern void I2C_masterReceiveStart(uint32_t moduleInstance);

//*****************************************************************************
//
//! Starts multi-byte reception at the Master end one byte at a time
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function is used by the Master module to receive each byte of a
//! multi-byte reception
//! This function reads currently received byte
//!
//! Modified register is \b UCBxRXBUF.
//! \return Received byte at Master end.
//
//*****************************************************************************
extern uint8_t I2C_masterReceiveMultiByteNext(uint32_t moduleInstance);

//*****************************************************************************
//
//! Finishes multi-byte reception at the Master end
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function is used by the Master module to initiate completion of a
//! multi-byte reception
//! This function
//! - Receives the current byte and initiates the STOP from Master to Slave
//!
//! Modified bits are \b UCTXSTP bit of \b UCBxCTL1.
//!
//! \return Received byte at Master end.
//
//*****************************************************************************
extern uint8_t I2C_masterReceiveMultiByteFinish(uint32_t moduleInstance);

//*****************************************************************************
//
//! Finishes multi-byte reception at the Master end with timeout
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param txData is a pointer to the location to store the received byte at
//!     master end
//! \param timeout is the amount of time to wait until giving up
//!
//! This function is used by the Master module to initiate completion of a
//! multi-byte reception
//! This function
//! - Receives the current byte and initiates the STOP from Master to Slave
//!
//! Modified bits are \b UCTXSTP bit of \b UCBxCTL1.
//!
//! \return 0x01 or 0x00URE of the transmission process.
//
//*****************************************************************************
extern bool I2C_masterReceiveMultiByteFinishWithTimeout(uint32_t moduleInstance,
        uint8_t *txData, uint32_t timeout);

//*****************************************************************************
//
//! Sends the STOP at the end of a multi-byte reception at the Master end
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function is used by the Master module to initiate STOP
//!
//! Modified bits are UCTXSTP bit of UCBxCTL1.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_masterReceiveMultiByteStop(uint32_t moduleInstance);

//*****************************************************************************
//
//! Does single byte reception from the slave
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! This function is used by the Master module to receive a single byte.
//! This function:
//! - Sends START and STOP
//! - Waits for data reception
//! - Receives one byte from the Slave
//!
//! Modified registers are \b UCBxIE, \b UCBxCTL1, \b UCBxIFG, \b UCBxTXBUF,
//! \b UCBxIE
//!
//! \return The byte that has been received from the slave
//
//*****************************************************************************
extern uint8_t I2C_masterReceiveSingleByte(uint32_t moduleInstance);

//*****************************************************************************
//
//! Receives a byte that has been sent to the I2C Master Module.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function reads a byte of data from the I2C receive data Register.
//!
//! \return Returns the byte received from by the I2C module, cast as an
//! uint8_t.
//
//*****************************************************************************
extern uint8_t I2C_masterReceiveSingle(uint32_t moduleInstance);

//*****************************************************************************
//
//! Returns the address of the RX Buffer of the I2C for the DMA module.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! Returns the address of the I2C RX Buffer. This can be used in conjunction
//! with the DMA to store the received data directly to memory.
//!
//! \return NONE
//
//*****************************************************************************
extern uint32_t I2C_getReceiveBufferAddressForDMA(uint32_t moduleInstance);

//*****************************************************************************
//
//! Returns the address of the TX Buffer of the I2C for the DMA module.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! Returns the address of the I2C TX Buffer. This can be used in conjunction
//! with the DMA to obtain transmitted data directly from memory.
//!
//! \return NONE
//
//*****************************************************************************
extern uint32_t I2C_getTransmitBufferAddressForDMA(uint32_t moduleInstance);

//*****************************************************************************
//
//! Indicates whether STOP got sent.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function returns an indication of whether or not STOP got sent
//! This function checks the status of the bus via UCTXSTP bit in
//! UCBxCTL1 register.
//!
//! \return Returns EUSCI_B_I2C_STOP_SEND_COMPLETE if the I2C Master
//!         finished sending STOP; otherwise, returns EUSCI_B_I2C_SENDING_STOP.
//
//*****************************************************************************
extern uint8_t I2C_masterIsStopSent(uint32_t moduleInstance);

//*****************************************************************************
//
//! Indicates whether Start got sent.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function returns an indication of whether or not Start got sent
//! This function checks the status of the bus via UCTXSTT bit in
//! UCBxCTL1 register.
//!
//! \return Returns true if the START has been sent, false if it is sending
//
//*****************************************************************************
extern bool I2C_masterIsStartSent(uint32_t moduleInstance);

//*****************************************************************************
//
//! This function is used by the Master module to initiate START
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! This function is used by the Master module to initiate STOP
//!
//! Modified bits are UCTXSTT bit of UCBxCTLW0.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_masterSendStart(uint32_t moduleInstance);

//*****************************************************************************
//
//! Enables Multi Master Mode
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! At the end of this function, the I2C module is still disabled till
//! I2C_enableModule is invoked
//!
//! Modified bits are \b UCSWRST of \b OFS_UCBxCTLW0, \b UCMM bit of
//! \b UCBxCTLW0
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_enableMultiMasterMode(uint32_t moduleInstance);

//*****************************************************************************
//
//! Disables Multi Master Mode
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//!
//! At the end of this function, the I2C module is still disabled till
//! I2C_enableModule is invoked
//!
//! Modified bits are \b UCSWRST of \b OFS_UCBxCTLW0, \b UCMM bit of
//! \b UCBxCTLW0
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_disableMultiMasterMode(uint32_t moduleInstance);

//*****************************************************************************
//
//! Enables individual I2C interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param mask is the bit mask of the interrupt sources to
//!                          be enabled.
//!
//! Enables the indicated I2C interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! The mask parameter is the logical OR of any of the following:
//!
//! - \b EUSCI_B_I2C_STOP_INTERRUPT - STOP condition interrupt
//! - \b EUSCI_B_I2C_START_INTERRUPT - START condition interrupt
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT0 - Transmit interrupt0
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT1 - Transmit interrupt1
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT2 - Transmit interrupt2
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT3 - Transmit interrupt3
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT0 - Receive interrupt0
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT1 - Receive interrupt1
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT2 - Receive interrupt2
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT3 - Receive interrupt3
//! - \b EUSCI_B_I2C_NAK_INTERRUPT - Not-acknowledge interrupt
//! - \b EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT - Arbitration lost interrupt
//! - \b EUSCI_B_I2C_BIT9_POSITION_INTERRUPT - Bit position 9 interrupt enable
//! - \b EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT - Clock low timeout interrupt
//!                                                 enable
//! - \b EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT - Byte counter interrupt enable
//!
//! Modified registers are UCBxIFG and OFS_UCBxIE.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_enableInterrupt(uint32_t moduleInstance, uint_fast16_t mask);

//*****************************************************************************
//
//! Disables individual I2C interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param mask is the bit mask of the interrupt sources to be
//! disabled.
//!
//! Disables the indicated I2C interrupt sources.  Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! The mask parameter is the logical OR of any of the following:
//!
//! - \b EUSCI_B_I2C_STOP_INTERRUPT - STOP condition interrupt
//! - \b EUSCI_B_I2C_START_INTERRUPT - START condition interrupt
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT0 - Transmit interrupt0
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT1 - Transmit interrupt1
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT2 - Transmit interrupt2
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT3 - Transmit interrupt3
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT0 - Receive interrupt0
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT1 - Receive interrupt1
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT2 - Receive interrupt2
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT3 - Receive interrupt3
//! - \b EUSCI_B_I2C_NAK_INTERRUPT - Not-acknowledge interrupt
//! - \b EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT - Arbitration lost interrupt
//! - \b EUSCI_B_I2C_BIT9_POSITION_INTERRUPT - Bit position 9 interrupt enable
//! - \b EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT - Clock low timeout interrupt
//!                                                enable
//! - \b EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT - Byte counter interrupt enable
//!
//! Modified register is \b UCBxIE.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_disableInterrupt(uint32_t moduleInstance, uint_fast16_t mask);

//*****************************************************************************
//
//! Clears I2C interrupt sources.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param mask is a bit mask of the interrupt sources to be cleared.
//!
//! The I2C interrupt source is cleared, so that it no longer asserts.
//! The highest interrupt flag is automatically cleared when an interrupt vector
//! generator is used.
//!
//! The mask parameter has the same definition as the mask
//! parameter to I2C_enableInterrupt().
//!
//! Modified register is \b UCBxIFG.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_clearInterruptFlag(uint32_t moduleInstance, uint_fast16_t mask);

//*****************************************************************************
//
//! Gets the current I2C interrupt status.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//! \param mask is the masked interrupt flag status to be returned.
//!        Mask value is the logical OR of any of the following:
//!        - \b EUSCI_B_I2C_NAK_INTERRUPT - Not-acknowledge interrupt
//!        - \b EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT - Arbitration lost
//!           interrupt
//!        - \b EUSCI_B_I2C_STOP_INTERRUPT - STOP condition interrupt
//!        - \b EUSCI_B_I2C_START_INTERRUPT - START condition interrupt
//!        - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT0 - Transmit interrupt0
//!        - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT1 - Transmit interrupt1
//!        - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT2 - Transmit interrupt2
//!        - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT3 - Transmit interrupt3
//!        - \b EUSCI_B_I2C_RECEIVE_INTERRUPT0 - Receive interrupt0
//!        - \b EUSCI_B_I2C_RECEIVE_INTERRUPT1 - Receive interrupt1
//!        - \b EUSCI_B_I2C_RECEIVE_INTERRUPT2 - Receive interrupt2
//!        - \b EUSCI_B_I2C_RECEIVE_INTERRUPT3 - Receive interrupt3
//!        - \b EUSCI_B_I2C_BIT9_POSITION_INTERRUPT - Bit position 9 interrupt
//!        - \b EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT - Clock low timeout
//!           interrupt enable
//!        - \b EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT - Byte counter interrupt
//!           enable
//!
//! \return the masked status of the interrupt flag
//! - \b EUSCI_B_I2C_STOP_INTERRUPT - STOP condition interrupt
//! - \b EUSCI_B_I2C_START_INTERRUPT - START condition interrupt
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT0 - Transmit interrupt0
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT1 - Transmit interrupt1
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT2 - Transmit interrupt2
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT3 - Transmit interrupt3
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT0 - Receive interrupt0
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT1 - Receive interrupt1
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT2 - Receive interrupt2
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT3 - Receive interrupt3
//! - \b EUSCI_B_I2C_NAK_INTERRUPT - Not-acknowledge interrupt
//! - \b EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT - Arbitration lost interrupt
//! - \b EUSCI_B_I2C_BIT9_POSITION_INTERRUPT - Bit position 9 interrupt enable
//! - \b EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT - Clock low timeout interrupt
//!                                                enable
//! - \b EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT - Byte counter interrupt enable
//
//*****************************************************************************
extern uint_fast16_t I2C_getInterruptStatus(uint32_t moduleInstance, uint16_t mask);

//*****************************************************************************
//
//! Gets the current I2C interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list of pending interrupts
//! that are actually enabled and could have caused the ISR.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \return the masked status of the interrupt flag
//! - \b EUSCI_B_I2C_STOP_INTERRUPT - STOP condition interrupt
//! - \b EUSCI_B_I2C_START_INTERRUPT - START condition interrupt
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT0 - Transmit interrupt0
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT1 - Transmit interrupt1
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT2 - Transmit interrupt2
//! - \b EUSCI_B_I2C_TRANSMIT_INTERRUPT3 - Transmit interrupt3
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT0 - Receive interrupt0
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT1 - Receive interrupt1
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT2 - Receive interrupt2
//! - \b EUSCI_B_I2C_RECEIVE_INTERRUPT3 - Receive interrupt3
//! - \b EUSCI_B_I2C_NAK_INTERRUPT - Not-acknowledge interrupt
//! - \b EUSCI_B_I2C_ARBITRATIONLOST_INTERRUPT - Arbitration lost interrupt
//! - \b EUSCI_B_I2C_BIT9_POSITION_INTERRUPT - Bit position 9 interrupt enable
//! - \b EUSCI_B_I2C_CLOCK_LOW_TIMEOUT_INTERRUPT - Clock low timeout interrupt
//!                                                enable
//! - \b EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT - Byte counter interrupt enable
//
//*****************************************************************************
extern uint_fast16_t I2C_getEnabledInterruptStatus(uint32_t moduleInstance);

//*****************************************************************************
//
//! Registers an interrupt handler for I2C interrupts.
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \param intHandler is a pointer to the function to be called when the
//! timer capture compare interrupt occurs.
//!
//! This function registers the handler to be called when an I2C
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific I2C interrupts must be enabled
//! via I2C_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via I2C_clearInterruptFlag().
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_registerInterrupt(uint32_t moduleInstance,
        void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the timer
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! This function unregisters the handler to be called when timer
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_unregisterInterrupt(uint32_t moduleInstance);


//*****************************************************************************
//
//! This function is used by the slave to send a NAK out over the I2C line
//!
//! \param moduleInstance is the instance of the eUSCI B (I2C) module. Valid
//! parameters vary from part to part, but can include:
//!         - \b EUSCI_B0_BASE
//!         - \b EUSCI_B1_BASE
//!         - \b EUSCI_B2_BASE
//!         - \b EUSCI_B3_BASE
//!  <br>It is important to note that for eUSCI modules, only "B" modules such as
//!  EUSCI_B0 can be used. "A" modules such as EUSCI_A0 do not support the
//!  I2C mode.
//!
//! \return None.
//
//*****************************************************************************
extern void I2C_slaveSendNAK(uint32_t moduleInstance);

/* Backwards Compatibility Layer */
#define EUSCI_B_I2C_slaveInit I2C_initSlave
#define EUSCI_B_I2C_enable I2C_enableModule
#define EUSCI_B_I2C_disable I2C_disableModule
#define EUSCI_B_I2C_setSlaveAddress I2C_setSlaveAddress
#define EUSCI_B_I2C_setMode I2C_setMode
#define EUSCI_B_I2C_getMode I2C_getMode
#define EUSCI_B_I2C_slaveDataPut I2C_slavePutData
#define EUSCI_B_I2C_slaveDataGet I2C_slaveGetData
#define EUSCI_B_I2C_isBusBusy I2C_isBusBusy
#define EUSCI_B_I2C_masterIsStopSent I2C_masterIsStopSent
#define EUSCI_B_I2C_masterIsStartSent I2C_masterIsStartSent
#define EUSCI_B_I2C_enableInterrupt I2C_enableInterrupt
#define EUSCI_B_I2C_disableInterrupt I2C_disableInterrupt
#define EUSCI_B_I2C_clearInterruptFlag I2C_clearInterruptFlag
#define EUSCI_B_I2C_getInterruptStatus  I2C_getEnabledInterruptStatus
#define EUSCI_B_I2C_masterSendSingleByte I2C_masterSendSingleByte
#define EUSCI_B_I2C_masterReceiveSingleByte I2C_masterReceiveSingleByte
#define EUSCI_B_I2C_masterSendSingleByteWithTimeout I2C_masterSendSingleByteWithTimeout
#define EUSCI_B_I2C_masterMultiByteSendStart I2C_masterSendMultiByteStart
#define EUSCI_B_I2C_masterMultiByteSendStartWithTimeout I2C_masterSendMultiByteStartWithTimeout
#define EUSCI_B_I2C_masterMultiByteSendNext I2C_masterSendMultiByteNext
#define EUSCI_B_I2C_masterMultiByteSendNextWithTimeout I2C_masterSendMultiByteNextWithTimeout
#define EUSCI_B_I2C_masterMultiByteSendFinish I2C_masterSendMultiByteFinish
#define EUSCI_B_I2C_masterMultiByteSendFinishWithTimeout I2C_masterSendMultiByteFinishWithTimeout
#define EUSCI_B_I2C_masterSendStart I2C_masterSendStart
#define EUSCI_B_I2C_masterMultiByteSendStop I2C_masterSendMultiByteStop
#define EUSCI_B_I2C_masterMultiByteSendStopWithTimeout I2C_masterSendMultiByteStopWithTimeout
#define EUSCI_B_I2C_masterReceiveStart I2C_masterReceiveStart
#define EUSCI_B_I2C_masterMultiByteReceiveNext I2C_masterReceiveMultiByteNext
#define EUSCI_B_I2C_masterMultiByteReceiveFinish I2C_masterReceiveMultiByteFinish
#define EUSCI_B_I2C_masterMultiByteReceiveFinishWithTimeout I2C_masterReceiveMultiByteFinishWithTimeout
#define EUSCI_B_I2C_masterMultiByteReceiveStop I2C_masterReceiveMultiByteStop
#define EUSCI_B_I2C_enableMultiMasterMode I2C_enableMultiMasterMode
#define EUSCI_B_I2C_disableMultiMasterMode I2C_disableMultiMasterMode
#define EUSCI_B_I2C_masterSingleReceive I2C_masterReceiveSingle
#define EUSCI_B_I2C_getReceiveBufferAddressForDMA I2C_getReceiveBufferAddressForDMA
#define EUSCI_B_I2C_getTransmitBufferAddressForDMA I2C_getTransmitBufferAddressForDMA


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif /* I2C_H_ */

