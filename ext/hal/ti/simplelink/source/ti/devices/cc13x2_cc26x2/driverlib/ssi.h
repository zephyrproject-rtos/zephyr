/******************************************************************************
*  Filename:       ssi.h
*  Revised:        2017-05-23 12:08:52 +0200 (Tue, 23 May 2017)
*  Revision:       49048
*
*  Description:    Defines and macros for the SSI.
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
//! \addtogroup ssi_api
//! @{
//
//*****************************************************************************

#ifndef __SSI_H__
#define __SSI_H__

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
#include "../inc/hw_ints.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_types.h"
#include "../inc/hw_ssi.h"
#include "debug.h"
#include "interrupt.h"

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
    #define SSIConfigSetExpClk              NOROM_SSIConfigSetExpClk
    #define SSIDataPut                      NOROM_SSIDataPut
    #define SSIDataPutNonBlocking           NOROM_SSIDataPutNonBlocking
    #define SSIDataGet                      NOROM_SSIDataGet
    #define SSIDataGetNonBlocking           NOROM_SSIDataGetNonBlocking
    #define SSIIntRegister                  NOROM_SSIIntRegister
    #define SSIIntUnregister                NOROM_SSIIntUnregister
#endif

//*****************************************************************************
//
// Values that can be passed to SSIIntEnable, SSIIntDisable, and SSIIntClear
// as the ui32IntFlags parameter, and returned by SSIIntStatus.
//
//*****************************************************************************
#define SSI_TXFF                0x00000008  // TX FIFO half full or less
#define SSI_RXFF                0x00000004  // RX FIFO half full or more
#define SSI_RXTO                0x00000002  // RX timeout
#define SSI_RXOR                0x00000001  // RX overrun

//*****************************************************************************
//
// Values that are returned from SSIStatus
//
//*****************************************************************************
#define SSI_RX_FULL             0x00000008  // Receive FIFO full
#define SSI_RX_NOT_EMPTY        0x00000004  // Receive FIFO not empty
#define SSI_TX_NOT_FULL         0x00000002  // Transmit FIFO not full
#define SSI_TX_EMPTY            0x00000001  // Transmit FIFO empty
#define SSI_STATUS_MASK         0x0000000F

//*****************************************************************************
//
// Values that can be passed to SSIConfigSetExpClk.
//
//*****************************************************************************
#define SSI_FRF_MOTO_MODE_0     0x00000000  // Moto fmt, polarity 0, phase 0
#define SSI_FRF_MOTO_MODE_1     0x00000002  // Moto fmt, polarity 0, phase 1
#define SSI_FRF_MOTO_MODE_2     0x00000001  // Moto fmt, polarity 1, phase 0
#define SSI_FRF_MOTO_MODE_3     0x00000003  // Moto fmt, polarity 1, phase 1
#define SSI_FRF_TI              0x00000010  // TI frame format
#define SSI_FRF_NMW             0x00000020  // National MicroWire frame format

#define SSI_MODE_MASTER         0x00000000  // SSI master
#define SSI_MODE_SLAVE          0x00000001  // SSI slave
#define SSI_MODE_SLAVE_OD       0x00000002  // SSI slave with output disabled

//*****************************************************************************
//
// Values that can be passed to SSIDMAEnable() and SSIDMADisable().
//
//*****************************************************************************
#define SSI_DMA_TX              0x00000002  // Enable DMA for transmit
#define SSI_DMA_RX              0x00000001  // Enable DMA for receive

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
//! \brief Checks an SSI base address.
//!
//! This function determines if an SSI module base address is valid.
//!
//! \param ui32Base specifies the SSI module base address.
//!
//! \return Returns \c true if the base address is valid and \c false
//! otherwise.
//
//*****************************************************************************
static bool
SSIBaseValid(uint32_t ui32Base)
{
    return(ui32Base == SSI0_BASE || ui32Base == SSI1_BASE);
}
#endif

//*****************************************************************************
//
//! \brief Configures the synchronous serial port.
//!
//! This function configures the synchronous serial port.  It sets
//! the SSI protocol, mode of operation, bit rate, and data width.
//!
//! The \c ui32Protocol parameter defines the data frame format. The Motorola
//! frame formats imply the following polarity and phase configurations:
//!
//! <pre>
//! Polarity Phase       Mode
//!   0       0   SSI_FRF_MOTO_MODE_0
//!   0       1   SSI_FRF_MOTO_MODE_1
//!   1       0   SSI_FRF_MOTO_MODE_2
//!   1       1   SSI_FRF_MOTO_MODE_3
//! </pre>
//!
//! The \c ui32Mode parameter defines the operating mode of the SSI module.
//! The SSI module can operate as a master or slave; if a slave, the SSI can be
//! configured to disable output on its serial output line.
//!
//! The \c ui32BitRate parameter defines the bit rate for the SSI. This bit
//! rate must satisfy the following clock ratio criteria:
//! - Master mode : FSSI >= 2 * bit rate
//! - Slave mode  : FSSI >= 12 * bit rate
//!
//! where FSSI is the frequency of the clock supplied to the SSI module.
//!
//! The \c ui32DataWidth parameter defines the width of the data transfers, and
//! can be a value between 4 and 16, inclusive.
//!
//! \note The peripheral clock is not necessarily the same as the processor clock.
//! The frequency of the peripheral clock is set by the system control.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param ui32SSIClk is the rate of the clock supplied to the SSI module.
//! \param ui32Protocol specifies the data transfer protocol.
//! The parameter can be one of the following values:
//! - \ref SSI_FRF_MOTO_MODE_0
//! - \ref SSI_FRF_MOTO_MODE_1
//! - \ref SSI_FRF_MOTO_MODE_2
//! - \ref SSI_FRF_MOTO_MODE_3
//! - \ref SSI_FRF_TI
//! - \ref SSI_FRF_NMW.
//! \param ui32Mode specifies the mode of operation.
//! The parameter can be one of the following values:
//! - \ref SSI_MODE_MASTER
//! - \ref SSI_MODE_SLAVE
//! - \ref SSI_MODE_SLAVE_OD
//! \param ui32BitRate specifies the clock rate.
//! \param ui32DataWidth specifies number of bits transferred per frame.
//! Must be a value between 4 and 16, both included.
//!
//! \return None
//
//*****************************************************************************
extern void SSIConfigSetExpClk(uint32_t ui32Base, uint32_t ui32SSIClk,
                               uint32_t ui32Protocol, uint32_t ui32Mode,
                               uint32_t ui32BitRate, uint32_t ui32DataWidth);

//*****************************************************************************
//
//! \brief Enables the synchronous serial port.
//!
//! This function enables operation of the synchronous serial port.  The
//! synchronous serial port must be configured before it is enabled.
//!
//! \param ui32Base specifies the SSI module base address.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SSIEnable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Read-modify-write the enable bit.
    HWREG(ui32Base + SSI_O_CR1) |= SSI_CR1_SSE;
}

//*****************************************************************************
//
//! \brief Disables the synchronous serial port.
//!
//! This function disables operation of the synchronous serial port.
//!
//! \param ui32Base specifies the SSI module base address.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SSIDisable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Read-modify-write the enable bit.
    HWREG(ui32Base + SSI_O_CR1) &= ~(SSI_CR1_SSE);
}

//*****************************************************************************
//
//! \brief Puts a data element into the SSI transmit FIFO.
//!
//! This function places the supplied data into the transmit FIFO of the
//! specified SSI module.
//!
//! \note The upper 32 - N bits of the \c ui32Data are discarded by the
//! hardware, where N is the data width as configured by \ref SSIConfigSetExpClk().
//! For example, if the interface is configured for 8-bit data width, the upper
//! 24 bits of \c ui32Data are discarded.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param ui32Data is the data to be transmitted over the SSI interface.
//!
//! \return None
//
//*****************************************************************************
extern void SSIDataPut(uint32_t ui32Base, uint32_t ui32Data);

//*****************************************************************************
//
//! \brief Puts a data element into the SSI transmit FIFO.
//!
//! This function places the supplied data into the transmit FIFO of the
//! specified SSI module. If there is no space in the FIFO, then this function
//! returns a zero.
//!
//! \note The upper 32 - N bits of the \c ui32Data are discarded by the hardware,
//! where N is the data width as configured by \ref SSIConfigSetExpClk(). For
//! example, if the interface is configured for 8-bit data width, the upper 24
//! bits of \c ui32Data are discarded.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param ui32Data is the data to be transmitted over the SSI interface.
//!
//! \return Returns the number of elements written to the SSI transmit FIFO.
//
//*****************************************************************************
extern int32_t SSIDataPutNonBlocking(uint32_t ui32Base, uint32_t ui32Data);

//*****************************************************************************
//
//! \brief Gets a data element from the SSI receive FIFO.
//!
//! This function gets received data from the receive FIFO of the specified
//! SSI module and places that data into the location specified by the
//! \c pui32Data parameter.
//!
//! \note Only the lower N bits of the value written to \c pui32Data contain
//! valid data, where N is the data width as configured by
//! \ref SSIConfigSetExpClk(). For example, if the interface is configured for
//! 8-bit data width, only the lower 8 bits of the value written to
//! \c pui32Data contain valid data.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param pui32Data is a pointer to a storage location for data that was
//! received over the SSI interface.
//!
//! \return None
//
//*****************************************************************************
extern void SSIDataGet(uint32_t ui32Base, uint32_t *pui32Data);

//*****************************************************************************
//
//! \brief Gets a data element from the SSI receive FIFO.
//!
//! This function gets received data from the receive FIFO of the specified SSI
//! module and places that data into the location specified by the \c ui32Data
//! parameter. If there is no data in the FIFO, then this function returns a
//! zero.
//!
//! \note Only the lower N bits of the value written to \c pui32Data contain
//! valid data, where N is the data width as configured by
//! \ref SSIConfigSetExpClk(). For example, if the interface is configured for
//! 8-bit data width, only the lower 8 bits of the value written to \c pui32Data
//! contain valid data.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param pui32Data is a pointer to a storage location for data that was
//! received over the SSI interface.
//!
//! \return Returns the number of elements read from the SSI receive FIFO.
//
//*****************************************************************************
extern int32_t SSIDataGetNonBlocking(uint32_t ui32Base, uint32_t *pui32Data);

//*****************************************************************************
//
//! \brief Determines whether the SSI transmitter is busy or not.
//!
//! Allows the caller to determine whether all transmitted bytes have cleared
//! the transmitter hardware. If \c false is returned, then the transmit FIFO
//! is empty and all bits of the last transmitted word have left the hardware
//! shift register.
//!
//! \param ui32Base is the base address of the SSI port.
//!
//! \return Returns status of the SSI transmit buffer.
//! - \c true  : SSI is transmitting.
//! - \c false : SSI transmissions are complete.
//
//*****************************************************************************
__STATIC_INLINE bool
SSIBusy(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Determine if the SSI is busy.
    return((HWREG(ui32Base + SSI_O_SR) & SSI_SR_BSY) ? true : false);
}

//*****************************************************************************
//
//! \brief Get the status of the SSI data buffers.
//!
//! This function is used to poll the status of the internal FIFOs in the SSI
//! module. The status of both TX and RX FIFO is returned.
//!
//! \param ui32Base specifies the SSI module base address.
//!
//! \return Returns the current status of the internal SSI data buffers.
//! The status is a bitwise OR'ed combination of:
//! - \ref SSI_RX_FULL        : Receive FIFO full.
//! - \ref SSI_RX_NOT_EMPTY   : Receive FIFO not empty.
//! - \ref SSI_TX_NOT_FULL    : Transmit FIFO not full.
//! - \ref SSI_TX_EMPTY       : Transmit FIFO empty.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
SSIStatus(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Return the status
    return (HWREG(ui32Base + SSI_O_SR) & SSI_STATUS_MASK);
}

//*****************************************************************************
//
//! \brief Registers an interrupt handler for the Synchronous Serial Interface in the dynamic interrupt table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific SSI interrupts must be enabled via \ref SSIIntEnable(). If necessary,
//! it is the interrupt handler's responsibility to clear the interrupt source
//! via \ref SSIIntClear().
//!
//! \param ui32Base specifies the SSI module base address.
//! \param pfnHandler is a pointer to the function to be called when the
//! synchronous serial port interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
extern void SSIIntRegister(uint32_t ui32Base, void (*pfnHandler)(void));

//*****************************************************************************
//
//! \brief Unregisters an interrupt handler for the Synchronous Serial Interface in the dynamic interrupt table.
//!
//! This function will clear the handler to be called when a SSI
//! interrupt occurs. This will also mask off the interrupt in the interrupt
//! controller so that the interrupt handler no longer is called.
//!
//! \param ui32Base specifies the SSI module base address.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
extern void SSIIntUnregister(uint32_t ui32Base);

//*****************************************************************************
//
//! \brief Enables individual SSI interrupt sources.
//!
//! Enables the indicated SSI interrupt sources. Only the sources that are
//! enabled can be reflected to the processor interrupt; disabled sources have
//! no effect on the processor.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param ui32IntFlags is a bit mask of the interrupt sources to be enabled.
//! - \ref SSI_TXFF
//! - \ref SSI_RXFF
//! - \ref SSI_RXTO
//! - \ref SSI_RXOR
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SSIIntEnable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Enable the specified interrupts.
    HWREG(ui32Base + SSI_O_IMSC) |= ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Disables individual SSI interrupt sources.
//!
//! Disables the indicated SSI interrupt sources.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param ui32IntFlags is a bit mask of the interrupt sources to be disabled.
//! - \ref SSI_TXFF
//! - \ref SSI_RXFF
//! - \ref SSI_RXTO
//! - \ref SSI_RXOR
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SSIIntDisable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Disable the specified interrupts.
    HWREG(ui32Base + SSI_O_IMSC) &= ~(ui32IntFlags);
}

//*****************************************************************************
//
//! \brief Clears SSI interrupt sources.
//!
//! The specified SSI interrupt sources are cleared so that they no longer
//! assert. This function must be called in the interrupt handler to keep the
//! interrupts from being recognized again immediately upon exit.
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
//! \param ui32Base specifies the SSI module base address.
//! \param ui32IntFlags is a bit mask of the interrupt sources to be cleared.
//! The parameter can consist of either or both of:
//! - \ref SSI_RXTO : Timeout interrupt.
//! - \ref SSI_RXOR : Overrun interrupt.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SSIIntClear(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Clear the requested interrupt sources.
    HWREG(ui32Base + SSI_O_ICR) = ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Gets the current interrupt status.
//!
//! This function returns the interrupt status for the SSI module.  Either the
//! raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \param ui32Base specifies the SSI module base address.
//! \param bMasked selects either raw or masked interrupt.
//! \c false : Raw interrupt status is required.
//! \c true  : Masked interrupt status is required.
//!
//! \return Returns the current interrupt status as an OR'ed combination of:
//! - \ref SSI_TXFF
//! - \ref SSI_RXFF
//! - \ref SSI_RXTO
//! - \ref SSI_RXOR
//
//*****************************************************************************
__STATIC_INLINE uint32_t
SSIIntStatus(uint32_t ui32Base, bool bMasked)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Return either the interrupt status or the raw interrupt status as
    // requested.
    if(bMasked)
    {
        return(HWREG(ui32Base + SSI_O_MIS));
    }
    else
    {
        return(HWREG(ui32Base + SSI_O_RIS));
    }
}

//*****************************************************************************
//
//! \brief Enable SSI DMA operation.
//!
//! The specified SSI DMA features are enabled. The SSI can be
//! configured to use DMA for transmit and/or receive data transfers.
//!
//! \note The uDMA controller must also be set up before DMA can be used
//! with the SSI.
//!
//! \param ui32Base is the base address of the SSI port.
//! \param ui32DMAFlags is a bit mask of the DMA features to enable.
//! The parameter is the bitwise OR of any of the following values:
//! - \ref SSI_DMA_RX : Enable DMA for receive.
//! - \ref SSI_DMA_TX : Enable DMA for transmit.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SSIDMAEnable(uint32_t ui32Base, uint32_t ui32DMAFlags)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Set the requested bits in the SSI DMA control register.
    HWREG(ui32Base + SSI_O_DMACR) |= ui32DMAFlags;
}

//*****************************************************************************
//
//! \brief Disable SSI DMA operation.
//!
//! This function is used to disable SSI DMA features that were enabled
//! by \ref SSIDMAEnable(). The specified SSI DMA features are disabled.
//!
//! \param ui32Base is the base address of the SSI port.
//! \param ui32DMAFlags is a bit mask of the DMA features to disable.
//! The parameter is the bitwise OR of any of the following values:
//! - \ref SSI_DMA_RX : Disable DMA for receive.
//! - \ref SSI_DMA_TX : Disable DMA for transmit.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
SSIDMADisable(uint32_t ui32Base, uint32_t ui32DMAFlags)
{
    // Check the arguments.
    ASSERT(SSIBaseValid(ui32Base));

    // Clear the requested bits in the SSI DMA control register.
    HWREG(ui32Base + SSI_O_DMACR) &= ~ui32DMAFlags;
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_SSIConfigSetExpClk
        #undef  SSIConfigSetExpClk
        #define SSIConfigSetExpClk              ROM_SSIConfigSetExpClk
    #endif
    #ifdef ROM_SSIDataPut
        #undef  SSIDataPut
        #define SSIDataPut                      ROM_SSIDataPut
    #endif
    #ifdef ROM_SSIDataPutNonBlocking
        #undef  SSIDataPutNonBlocking
        #define SSIDataPutNonBlocking           ROM_SSIDataPutNonBlocking
    #endif
    #ifdef ROM_SSIDataGet
        #undef  SSIDataGet
        #define SSIDataGet                      ROM_SSIDataGet
    #endif
    #ifdef ROM_SSIDataGetNonBlocking
        #undef  SSIDataGetNonBlocking
        #define SSIDataGetNonBlocking           ROM_SSIDataGetNonBlocking
    #endif
    #ifdef ROM_SSIIntRegister
        #undef  SSIIntRegister
        #define SSIIntRegister                  ROM_SSIIntRegister
    #endif
    #ifdef ROM_SSIIntUnregister
        #undef  SSIIntUnregister
        #define SSIIntUnregister                ROM_SSIIntUnregister
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

#endif // __SSI_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
