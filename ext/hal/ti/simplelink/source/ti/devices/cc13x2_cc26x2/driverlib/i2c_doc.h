/******************************************************************************
*  Filename:       i2c_doc.h
*  Revised:        2016-03-30 13:03:59 +0200 (Wed, 30 Mar 2016)
*  Revision:       45971
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
//! \addtogroup i2c_api
//! @{
//! \section sec_i2c Introduction
//!
//! The Inter-Integrated Circuit (\i2c) API provides a set of functions for using
//! the \ti_device \i2c master and slave module. Functions are provided to perform
//! the following actions:
//! - Initialize the \i2c module.
//! - Send and receive data.
//! - Obtain status.
//! - Manage interrupts for the \i2c module.
//!
//! The \i2c master and slave module provide the ability to communicate to other IC
//! devices over an \i2c bus. The \i2c bus is specified to support devices that can
//! both transmit and receive (write and read) data. Also, devices on the \i2c bus
//! can be designated as either a master or a slave. The \ti_device \i2c module
//! supports both sending and receiving data as either a master or a slave, and also
//! support the simultaneous operation as both a master and a slave. Finally, the
//! \ti_device \i2c module can operate at two speeds: standard (100 kb/s) and fast
//! (400 kb/s).
//!
//! The master and slave \i2c module can generate interrupts. The \i2c master
//! module generates interrupts when a transmit or receive operation
//! completes (or aborts due to an error).
//! The \i2c slave module can generate interrupts when data is
//! sent or requested by a master and when a START or STOP condition is present.
//!
//! \section sec_i2c_master Master Operations
//!
//! When using this API to drive the \i2c master module, the user must first
//! initialize the \i2c master module with a call to \ref I2CMasterInitExpClk(). This
//! function sets the bus speed and enables the master module.
//!
//! The user may transmit or receive data after the successful initialization of
//! the \i2c master module. Data is transferred by first setting the slave address
//! using \ref I2CMasterSlaveAddrSet(). This function is also used to define whether
//! the transfer is a send (a write to the slave from the master) or a receive (a
//! read from the slave by the master). Then, if connected to an \i2c bus that has
//! multiple masters, the \ti_device \i2c master must first call \ref I2CMasterBusBusy()
//! before trying to initiate the desired transaction. After determining that
//! the bus is not busy, if trying to send data, the user must call the
//! \ref I2CMasterDataPut() function. The transaction can then be initiated on the bus
//! by calling the \ref I2CMasterControl() function with any of the following commands:
//!  - \ref I2C_MASTER_CMD_SINGLE_SEND
//!  - \ref I2C_MASTER_CMD_SINGLE_RECEIVE
//!  - \ref I2C_MASTER_CMD_BURST_SEND_START
//!  - \ref I2C_MASTER_CMD_BURST_RECEIVE_START
//!
//! Any of these commands result in the master arbitrating for the bus,
//! driving the start sequence onto the bus, and sending the slave address and
//! direction bit across the bus. The remainder of the transaction can then be
//! driven using either a polling or interrupt-driven method.
//!
//! For the single send and receive cases, the polling method involves looping
//! on the return from \ref I2CMasterBusy(). Once the function indicates that the \i2c
//! master is no longer busy, the bus transaction is complete and can be
//! checked for errors using \ref I2CMasterErr(). If there are no errors, then the data
//! has been sent or is ready to be read using \ref I2CMasterDataGet(). For the burst
//! send and receive cases, the polling method also involves calling the
//! \ref I2CMasterControl() function for each byte transmitted or received
//! (using either the \ref I2C_MASTER_CMD_BURST_SEND_CONT or \ref I2C_MASTER_CMD_BURST_RECEIVE_CONT
//! commands), and for the last byte sent or received (using either the
//! \ref I2C_MASTER_CMD_BURST_SEND_FINISH or \ref I2C_MASTER_CMD_BURST_RECEIVE_FINISH
//! commands).
//!
//! If any error is detected during the burst transfer,
//! the appropriate stop command (\ref I2C_MASTER_CMD_BURST_SEND_ERROR_STOP or
//! \ref I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP) should be used to call the
//! \ref I2CMasterControl() function.
//!
//! For the interrupt-driven transaction, the user must register an interrupt
//! handler for the \i2c devices and enable the \i2c master interrupt; the interrupt
//! occurs when the master is no longer busy.
//!
//! \section sec_i2c_slave Slave Operations
//!
//! When using this API to drive the \i2c slave module, the user must first
//! initialize the \i2c slave module with a call to \ref I2CSlaveInit(). This function
//! enables the \i2c slave module and initializes the address of the slave. After the
//! initialization completes, the user may poll the slave status using
//! \ref I2CSlaveStatus() to determine if a master requested a send or receive
//! operation. Depending on the type of operation requested, the user can call
//! \ref I2CSlaveDataPut() or \ref I2CSlaveDataGet() to complete the transaction.
//! Alternatively, the \i2c slave can handle transactions using an interrupt handler
//! registered with \ref I2CIntRegister(), and by enabling the \i2c slave interrupt.
//!
//! \section sec_i2c_api API
//!
//! The \i2c API is broken into three groups of functions:
//! those that handle status and initialization, those that
//! deal with sending and receiving data, and those that deal with
//! interrupts.
//!
//! Status and initialization functions for the \i2c module are:
//! - \ref I2CMasterInitExpClk()
//! - \ref I2CMasterEnable()
//! - \ref I2CMasterDisable()
//! - \ref I2CMasterBusBusy()
//! - \ref I2CMasterBusy()
//! - \ref I2CMasterErr()
//! - \ref I2CSlaveInit()
//! - \ref I2CSlaveEnable()
//! - \ref I2CSlaveDisable()
//! - \ref I2CSlaveStatus()
//!
//! Sending and receiving data from the \i2c module is handled by the following functions:
//! - \ref I2CMasterSlaveAddrSet()
//! - \ref I2CSlaveAddressSet()
//! - \ref I2CMasterControl()
//! - \ref I2CMasterDataGet()
//! - \ref I2CMasterDataPut()
//! - \ref I2CSlaveDataGet()
//! - \ref I2CSlaveDataPut()
//!
//! The \i2c master and slave interrupts are handled by the following functions:
//! - \ref I2CIntRegister()
//! - \ref I2CIntUnregister()
//! - \ref I2CMasterIntEnable()
//! - \ref I2CMasterIntDisable()
//! - \ref I2CMasterIntClear()
//! - \ref I2CMasterIntStatus()
//! - \ref I2CSlaveIntEnable()
//! - \ref I2CSlaveIntDisable()
//! - \ref I2CSlaveIntClear()
//! - \ref I2CSlaveIntStatus()
//!
//! @}
