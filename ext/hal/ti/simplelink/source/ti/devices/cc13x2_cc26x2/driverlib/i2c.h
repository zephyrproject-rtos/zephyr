/******************************************************************************
*  Filename:       i2c.h
*  Revised:        2017-05-23 12:08:52 +0200 (Tue, 23 May 2017)
*  Revision:       49048
*
*  Description:    Defines and prototypes for the I2C.
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

//*****************************************************************************
//
//! \addtogroup peripheral_group
//! @{
//! \addtogroup i2c_api
//! @{
//
//*****************************************************************************

#ifndef __I2C_H__
#define __I2C_H__

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

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_i2c.h"
#include "../inc/hw_sysctl.h"
#include "debug.h"
#include "interrupt.h"
#include "cpu.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define I2CMasterInitExpClk             NOROM_I2CMasterInitExpClk
    #define I2CMasterErr                    NOROM_I2CMasterErr
    #define I2CIntRegister                  NOROM_I2CIntRegister
    #define I2CIntUnregister                NOROM_I2CIntUnregister
#endif

//*****************************************************************************
//
// I2C Master commands
//
//*****************************************************************************
#define I2C_MASTER_CMD_SINGLE_SEND                                            \
                                0x00000007
#define I2C_MASTER_CMD_SINGLE_RECEIVE                                         \
                                0x00000007
#define I2C_MASTER_CMD_BURST_SEND_START                                       \
                                0x00000003
#define I2C_MASTER_CMD_BURST_SEND_CONT                                        \
                                0x00000001
#define I2C_MASTER_CMD_BURST_SEND_FINISH                                      \
                                0x00000005
#define I2C_MASTER_CMD_BURST_SEND_ERROR_STOP                                  \
                                0x00000004
#define I2C_MASTER_CMD_BURST_RECEIVE_START                                    \
                                0x0000000b
#define I2C_MASTER_CMD_BURST_RECEIVE_CONT                                     \
                                0x00000009
#define I2C_MASTER_CMD_BURST_RECEIVE_FINISH                                   \
                                0x00000005
#define I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP                               \
                                0x00000004

//*****************************************************************************
//
// I2C Master error status
//
//*****************************************************************************
#define I2C_MASTER_ERR_NONE     0
#define I2C_MASTER_ERR_ADDR_ACK 0x00000004
#define I2C_MASTER_ERR_DATA_ACK 0x00000008
#define I2C_MASTER_ERR_ARB_LOST 0x00000010

//*****************************************************************************
//
// I2C Slave action requests
//
//*****************************************************************************
#define I2C_SLAVE_ACT_NONE      0
#define I2C_SLAVE_ACT_RREQ      0x00000001  // Master has sent data
#define I2C_SLAVE_ACT_TREQ      0x00000002  // Master has requested data
#define I2C_SLAVE_ACT_RREQ_FBR  0x00000005  // Master has sent first byte

//*****************************************************************************
//
// I2C Slave interrupts
//
//*****************************************************************************
#define I2C_SLAVE_INT_STOP      0x00000004  // Stop Condition Interrupt.
#define I2C_SLAVE_INT_START     0x00000002  // Start Condition Interrupt.
#define I2C_SLAVE_INT_DATA      0x00000001  // Data Interrupt.

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

#ifdef DRIVERLIB_DEBUG
//*****************************************************************************
//
//! \internal
//!
//! \brief Checks an I2C base address.
//!
//! This function determines if a I2C port base address is valid.
//!
//! \param ui32Base is the base address of the I2C port.
//!
//! \return Returns \c true if the base address is valid and \c false
//! otherwise
//
//*****************************************************************************
static bool
I2CBaseValid(uint32_t ui32Base)
{
    return(ui32Base == I2C0_BASE);
}
#endif

//*****************************************************************************
//
//! \brief Initializes the I2C Master block.
//!
//! This function initializes operation of the I2C Master block. Upon
//! successful initialization of the I2C block, this function will have set the
//! bus speed for the master, and will have enabled the I2C Master block.
//!
//! If the parameter \c bFast is \c true, then the master block will be set up
//! to transfer data at 400 kbps; otherwise, it will be set up to transfer data
//! at 100 kbps.
//!
//!
//! \param ui32Base is the base address of the I2C module.
//! \param ui32I2CClk is the rate of the clock supplied to the I2C module.
//! \param bFast set up for fast data transfers.
//!
//! \return None
//
//*****************************************************************************
extern void I2CMasterInitExpClk(uint32_t ui32Base, uint32_t ui32I2CClk,
                                bool bFast);

//*****************************************************************************
//
//! \brief Controls the state of the I2C Master module.
//!
//! This function is used to control the state of the Master module send and
//! receive operations.
//!
//! \param ui32Base is the base address of the I2C module.
//! \param ui32Cmd is the command to be issued by the I2C Master module
//! The parameter can be one of the following values:
//! - \ref I2C_MASTER_CMD_SINGLE_SEND
//! - \ref I2C_MASTER_CMD_SINGLE_RECEIVE
//! - \ref I2C_MASTER_CMD_BURST_SEND_START
//! - \ref I2C_MASTER_CMD_BURST_SEND_CONT
//! - \ref I2C_MASTER_CMD_BURST_SEND_FINISH
//! - \ref I2C_MASTER_CMD_BURST_SEND_ERROR_STOP
//! - \ref I2C_MASTER_CMD_BURST_RECEIVE_START
//! - \ref I2C_MASTER_CMD_BURST_RECEIVE_CONT
//! - \ref I2C_MASTER_CMD_BURST_RECEIVE_FINISH
//! - \ref I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterControl(uint32_t ui32Base, uint32_t ui32Cmd)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));
    ASSERT((ui32Cmd == I2C_MASTER_CMD_SINGLE_SEND) ||
    //     (ui32Cmd == I2C_MASTER_CMD_SINGLE_RECEIVE) || -> Equal to SINGLE_SEND
           (ui32Cmd == I2C_MASTER_CMD_BURST_SEND_START) ||
           (ui32Cmd == I2C_MASTER_CMD_BURST_SEND_CONT) ||
           (ui32Cmd == I2C_MASTER_CMD_BURST_SEND_FINISH) ||
           (ui32Cmd == I2C_MASTER_CMD_BURST_SEND_ERROR_STOP) ||
           (ui32Cmd == I2C_MASTER_CMD_BURST_RECEIVE_START) ||
           (ui32Cmd == I2C_MASTER_CMD_BURST_RECEIVE_CONT) ||
           (ui32Cmd == I2C_MASTER_CMD_BURST_RECEIVE_FINISH) ||
           (ui32Cmd == I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP));

    // Send the command.
    HWREG(I2C0_BASE + I2C_O_MCTRL) = ui32Cmd;

    // Delay minimum four cycles in order to ensure that the I2C_O_MSTAT
    // register has been correctly updated before function exit
    CPUdelay(2);
}

//*****************************************************************************
//
//! \brief Sets the address that the I2C Master will place on the bus.
//!
//! This function will set the address that the I2C Master will place on the
//! bus when initiating a transaction. When the \e bReceive parameter is set
//! to \b true, the address will indicate that the I2C Master is initiating a
//! read from the slave; otherwise the address will indicate that the I2C
//! Master is initiating a write to the slave.
//!
//! \param ui32Base is the base address of the I2C module.
//! \param ui8SlaveAddr is a 7-bit slave address
//! \param bReceive flag indicates the type of communication with the slave.
//! - \c true  : I2C Master is initiating a read from the slave.
//! - \c false : I2C Master is initiating a write to the slave.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterSlaveAddrSet(uint32_t ui32Base, uint8_t ui8SlaveAddr,
                      bool bReceive)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));
    ASSERT(!(ui8SlaveAddr & 0x80));

    // Set the address of the slave with which the master will communicate.
    HWREG(I2C0_BASE + I2C_O_MSA) = (ui8SlaveAddr << 1) | bReceive;
}

//*****************************************************************************
//
//! \brief Enables the I2C Master block.
//!
//! This will enable operation of the I2C Master block.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterEnable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Enable the clock for the master.
    HWREGBITW(I2C0_BASE + I2C_O_MCR, I2C_MCR_MFE_BITN) = 1;

    // Enable the master block.
    HWREG(I2C0_BASE + I2C_O_MCTRL) = I2C_MCTRL_RUN;
}

//*****************************************************************************
//
//! \brief Disables the I2C master block.
//!
//! This will disable operation of the I2C master block.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterDisable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Disable the master block.
    HWREG(I2C0_BASE + I2C_O_MCTRL) = 0;

    // Disable the clock for the master.
    HWREGBITW(I2C0_BASE + I2C_O_MCR, I2C_MCR_MFE_BITN) = 0;
}

//*****************************************************************************
//
//! \brief Indicates whether or not the I2C Master is busy.
//!
//! This function returns an indication of whether or not the I2C Master is
//! busy transmitting or receiving data.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return Returns status of I2C Master:
//! - \c true  : I2C Master is busy.
//! - \c false : I2C Master is not busy.
//
//*****************************************************************************
__STATIC_INLINE bool
I2CMasterBusy(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Return the busy status.
    if(HWREG(I2C0_BASE + I2C_O_MSTAT) & I2C_MSTAT_BUSY)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

//*****************************************************************************
//
//! \brief Indicates whether or not the I2C bus is busy.
//!
//! This function returns an indication of whether or not the I2C bus is busy.
//! This function can be used in a multi-master environment to determine if
//! another master is currently using the bus.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return Returns status of the I2C bus:
//! - \c true  : I2C bus is busy.
//! - \c false : I2C bus is not busy.
//
//*****************************************************************************
__STATIC_INLINE bool
I2CMasterBusBusy(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Return the bus busy status.
    if(HWREG(I2C0_BASE + I2C_O_MSTAT) & I2C_MSTAT_BUSBSY)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

//*****************************************************************************
//
//! \brief Receives a byte that has been sent to the I2C Master.
//!
//! This function reads a byte of data from the I2C Master Data Register.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return Returns the byte received from by the I2C Master, cast as an
//! uint32_t.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
I2CMasterDataGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Read a byte.
    return(HWREG(I2C0_BASE + I2C_O_MDR));
}

//*****************************************************************************
//
//! \brief Transmits a byte from the I2C Master.
//!
//! This function will place the supplied data into I2C Master Data Register.
//!
//! \param ui32Base is the base address of the I2C module.
//! \param ui8Data is the data to be transmitted by the I2C Master
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterDataPut(uint32_t ui32Base, uint8_t ui8Data)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Write the byte.
    HWREG(I2C0_BASE + I2C_O_MDR) = ui8Data;
}

//*****************************************************************************
//
//! \brief Gets the error status of the I2C Master module.
//!
//! This function is used to obtain the error status of the Master module send
//! and receive operations.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return Returns the error status of the Master module:
//! - \ref I2C_MASTER_ERR_NONE
//! - \ref I2C_MASTER_ERR_ADDR_ACK
//! - \ref I2C_MASTER_ERR_DATA_ACK
//! - \ref I2C_MASTER_ERR_ARB_LOST
//
//*****************************************************************************
extern uint32_t I2CMasterErr(uint32_t ui32Base);

//*****************************************************************************
//
//! \brief Enables the I2C Master interrupt.
//!
//! Enables the I2C Master interrupt source.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterIntEnable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Enable the master interrupt.
    HWREG(I2C0_BASE + I2C_O_MIMR) = I2C_MIMR_IM;
}

//*****************************************************************************
//
//! \brief Disables the I2C Master interrupt.
//!
//! Disables the I2C Master interrupt source.
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterIntDisable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Disable the master interrupt.
    HWREG(I2C0_BASE + I2C_O_MIMR) = 0;
}

//*****************************************************************************
//
//! \brief Clears I2C Master interrupt sources.
//!
//! The I2C Master interrupt source is cleared, so that it no longer asserts.
//! This must be done in the interrupt handler to keep it from being called
//! again immediately upon exit.
//!
//! \note Due to write buffers and synchronizers in the system it may take several
//! clock cycles from a register write clearing an event in a module and until the
//! event is actually cleared in the NVIC of the system CPU. It is recommended to
//! clear the event source early in the interrupt service routine (ISR) to allow
//! the event clear to propagate to the NVIC before returning from the ISR.
//! At the same time, an early event clear allows new events of the same type to be
//! pended instead of ignored if the event is cleared later in the ISR.
//! It is the responsibility of the programmer to make sure that enough time has passed
//! before returning from the ISR to avoid false re-triggering of the cleared event.
//! A simple, although not necessarily optimal, way of clearing an event before
//! returning from the ISR is:
//! -# Write to clear event (interrupt source). (buffered write)
//! -# Dummy read from the event source module. (making sure the write has propagated)
//! -# Wait two system CPU clock cycles (user code or two NOPs). (allowing cleared event to propagate through any synchronizers)
//!
//! \param ui32Base is the base address of the I2C module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CMasterIntClear(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Clear the I2C master interrupt source.
    HWREG(I2C0_BASE + I2C_O_MICR) = I2C_MICR_IC;
}

//*****************************************************************************
//
//! \brief Gets the current I2C Master interrupt status.
//!
//! This returns the interrupt status for the I2C Master module. Either the
//! raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \param ui32Base is the base address of the I2C Master module.
//! \param bMasked selects either raw or masked interrupt status.
//! - \c false : Raw interrupt status is requested.
//! - \c true  : Masked interrupt status is requested.
//!
//! \return Returns the current interrupt status.
//! - \c true  : Active.
//! - \c false : Not active.
//
//*****************************************************************************
__STATIC_INLINE bool
I2CMasterIntStatus(uint32_t ui32Base, bool bMasked)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Return either the interrupt status or the raw interrupt status as
    // requested.
    if(bMasked)
    {
        return((HWREG(I2C0_BASE + I2C_O_MMIS)) ? true : false);
    }
    else
    {
        return((HWREG(I2C0_BASE + I2C_O_MRIS)) ? true : false);
    }
}

//*****************************************************************************
//
//! \brief Enables the I2C Slave block.
//!
//! This will enable operation of the I2C Slave block.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveEnable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Enable the clock to the slave block.
    HWREGBITW(I2C0_BASE + I2C_O_MCR, I2C_MCR_SFE_BITN) = 1;

    // Enable the slave.
    HWREG(I2C0_BASE + I2C_O_SCTL) = I2C_SCTL_DA;
}

//*****************************************************************************
//
//! \brief Initializes the I2C Slave block.
//!
//! This function initializes operation of the I2C Slave block. Upon
//! successful initialization of the I2C blocks, this function will have set
//! the slave address and have enabled the I2C Slave block.
//!
//! The parameter \c ui8SlaveAddr is the value that will be compared against the
//! slave address sent by an I2C master.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//! \param ui8SlaveAddr is the 7-bit slave address.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveInit(uint32_t ui32Base, uint8_t ui8SlaveAddr)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));
    ASSERT(!(ui8SlaveAddr & 0x80));

    // Must enable the device before doing anything else.
    I2CSlaveEnable(I2C0_BASE);

    // Set up the slave address.
    HWREG(I2C0_BASE + I2C_O_SOAR) = ui8SlaveAddr;
}

//*****************************************************************************
//
//! \brief Sets the I2C slave address.
//!
//! This function writes the specified slave address.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//! \param ui8SlaveAddr is the 7-bit slave address
//!
//! \return None.
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveAddressSet(uint32_t ui32Base, uint8_t ui8SlaveAddr)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));
    ASSERT(!(ui8SlaveAddr & 0x80));

    // Set up the primary slave address.
    HWREG(I2C0_BASE + I2C_O_SOAR) = ui8SlaveAddr;
}

//*****************************************************************************
//
//! \brief Disables the I2C slave block.
//!
//! This will disable operation of the I2C slave block.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveDisable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Disable the slave.
    HWREG(I2C0_BASE + I2C_O_SCTL) = 0x0;

    // Disable the clock to the slave block.
    HWREGBITW(I2C0_BASE + I2C_O_MCR, I2C_MCR_SFE_BITN) = 0;
}

//*****************************************************************************
//
//! \brief Gets the I2C Slave module status.
//!
//! This function will return the action requested from a master, if any.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//!
//! \return Returns the status of the I2C Slave module:
//! - \ref I2C_SLAVE_ACT_NONE : No action has been requested of the I2C Slave module.
//! - \ref I2C_SLAVE_ACT_RREQ : An I2C master has sent data to the I2C Slave module.
//! - \ref I2C_SLAVE_ACT_TREQ : An I2C master has requested that the I2C Slave module send data.
//! - \ref I2C_SLAVE_ACT_RREQ_FBR : An I2C master has sent data to the I2C slave
//! and the first byte following the slave's own address has been received.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
I2CSlaveStatus(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Return the slave status.
    return(HWREG(I2C0_BASE + I2C_O_SSTAT));
}

//*****************************************************************************
//
//! \brief Receives a byte that has been sent to the I2C Slave.
//!
//! This function reads a byte of data from the I2C Slave Data Register.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//!
//! \return Returns the byte received from by the I2C Slave, cast as an
//! uint32_t.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
I2CSlaveDataGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Read a byte.
    return(HWREG(I2C0_BASE + I2C_O_SDR));
}

//*****************************************************************************
//
//! \brief Transmits a byte from the I2C Slave.
//!
//! This function will place the supplied data into I2C Slave Data Register.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//! \param ui8Data data to be transmitted from the I2C Slave.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveDataPut(uint32_t ui32Base, uint8_t ui8Data)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Write the byte.
    HWREG(I2C0_BASE + I2C_O_SDR) = ui8Data;
}

//*****************************************************************************
//
//! \brief Enables individual I2C Slave interrupt sources.
//!
//! Enables the indicated I2C Slave interrupt sources. Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! \param ui32Base is the base address of the I2C module.
//! \param ui32IntFlags is the bit mask of the slave interrupt sources to be enabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref I2C_SLAVE_INT_STOP
//! - \ref I2C_SLAVE_INT_START
//! - \ref I2C_SLAVE_INT_DATA
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveIntEnable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    uint32_t ui32Val;

    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));
    ASSERT(ui32IntFlags & (I2C_SLAVE_INT_STOP | I2C_SLAVE_INT_START |
                           I2C_SLAVE_INT_DATA));

    // Enable the slave interrupt.
    ui32Val = HWREG(I2C0_BASE + I2C_O_SIMR);
    ui32Val |= ui32IntFlags;
    HWREG(I2C0_BASE + I2C_O_SIMR) = ui32Val;
}

//*****************************************************************************
//
//! \brief Disables individual I2C Slave interrupt sources.
//!
//! Disables the indicated I2C Slave interrupt sources. Only the sources that
//! are enabled can be reflected to the processor interrupt; disabled sources
//! have no effect on the processor.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//! \param ui32IntFlags is the bit mask of the interrupt sources to be disabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref I2C_SLAVE_INT_STOP
//! - \ref I2C_SLAVE_INT_START
//! - \ref I2C_SLAVE_INT_DATA
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveIntDisable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    uint32_t ui32Val;

    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));
    ASSERT(ui32IntFlags & (I2C_SLAVE_INT_STOP | I2C_SLAVE_INT_START |
                           I2C_SLAVE_INT_DATA));

    // Disable the slave interrupt.
    ui32Val = HWREG(I2C0_BASE + I2C_O_SIMR);
    ui32Val &= ~ui32IntFlags;
    HWREG(I2C0_BASE + I2C_O_SIMR) = ui32Val;
}

//*****************************************************************************
//
//! \brief Clears I2C Slave interrupt sources.
//!
//! The specified I2C Slave interrupt sources are cleared, so that they no
//! longer assert. This must be done in the interrupt handler to keep it from
//! being called again immediately upon exit.
//!
//! \note Due to write buffers and synchronizers in the system it may take several
//! clock cycles from a register write clearing an event in a module and until the
//! event is actually cleared in the NVIC of the system CPU. It is recommended to
//! clear the event source early in the interrupt service routine (ISR) to allow
//! the event clear to propagate to the NVIC before returning from the ISR.
//! At the same time, an early event clear allows new events of the same type to be
//! pended instead of ignored if the event is cleared later in the ISR.
//! It is the responsibility of the programmer to make sure that enough time has passed
//! before returning from the ISR to avoid false re-triggering of the cleared event.
//! A simple, although not necessarily optimal, way of clearing an event before
//! returning from the ISR is:
//! -# Write to clear event (interrupt source). (buffered write)
//! -# Dummy read from the event source module. (making sure the write has propagated)
//! -# Wait two system CPU clock cycles (user code or two NOPs). (allowing cleared event to propagate through any synchronizers)
//!
//! \param ui32Base is the base address of the I2C module.
//! \param ui32IntFlags is a bit mask of the interrupt sources to be cleared.
//! The parameter is the bitwise OR of any of the following:
//! - \ref I2C_SLAVE_INT_STOP
//! - \ref I2C_SLAVE_INT_START
//! - \ref I2C_SLAVE_INT_DATA
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2CSlaveIntClear(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Clear the I2C slave interrupt source.
    HWREG(I2C0_BASE + I2C_O_SICR) = ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Gets the current I2C Slave interrupt status.
//!
//! This returns the interrupt status for the I2C Slave module. Either the raw
//! interrupt status or the status of interrupts that are allowed to reflect to
//! the processor can be returned.
//!
//! \param ui32Base is the base address of the I2C Slave module.
//! \param bMasked selects either raw or masked interrupt status.
//! - \c false : Raw interrupt status is requested.
//! - \c true  : Masked interrupt status is requested.
//!
//! \return Returns the current interrupt status as an OR'ed combination of:
//! - \ref I2C_SLAVE_INT_STOP
//! - \ref I2C_SLAVE_INT_START
//! - \ref I2C_SLAVE_INT_DATA
//
//*****************************************************************************
__STATIC_INLINE uint32_t
I2CSlaveIntStatus(uint32_t ui32Base, bool bMasked)
{
    // Check the arguments.
    ASSERT(I2CBaseValid(ui32Base));

    // Return either the interrupt status or the raw interrupt status as
    // requested.
    if(bMasked)
    {
        return(HWREG(I2C0_BASE + I2C_O_SMIS));
    }
    else
    {
        return(HWREG(I2C0_BASE + I2C_O_SRIS));
    }
}

//*****************************************************************************
//
//! \brief Registers an interrupt handler for the I2C module in the dynamic interrupt table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific I2C interrupts must be enabled via \ref I2CMasterIntEnable() and
//! \ref I2CSlaveIntEnable(). If necessary, it is the interrupt handler's
//! responsibility to clear the interrupt source via \ref I2CMasterIntClear() and
//! \ref I2CSlaveIntClear().
//!
//! \param ui32Base is the base address of the I2C Master module.
//! \param pfnHandler is a pointer to the function to be called when the
//! I2C interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
extern void I2CIntRegister(uint32_t ui32Base, void (*pfnHandler)(void));

//*****************************************************************************
//
//! \brief Unregisters an interrupt handler for the I2C module in the dynamic interrupt table.
//!
//! This function will clear the handler to be called when an I2C interrupt
//! occurs. This will also mask off the interrupt in the interrupt controller
//! so that the interrupt handler no longer is called.
//!
//! \param ui32Base is the base address of the I2C Master module.
//!
//! \return None
//!
//! \sa \brief IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
extern void I2CIntUnregister(uint32_t ui32Base);

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_I2CMasterInitExpClk
        #undef  I2CMasterInitExpClk
        #define I2CMasterInitExpClk             ROM_I2CMasterInitExpClk
    #endif
    #ifdef ROM_I2CMasterErr
        #undef  I2CMasterErr
        #define I2CMasterErr                    ROM_I2CMasterErr
    #endif
    #ifdef ROM_I2CIntRegister
        #undef  I2CIntRegister
        #define I2CIntRegister                  ROM_I2CIntRegister
    #endif
    #ifdef ROM_I2CIntUnregister
        #undef  I2CIntUnregister
        #define I2CIntUnregister                ROM_I2CIntUnregister
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __I2C_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
