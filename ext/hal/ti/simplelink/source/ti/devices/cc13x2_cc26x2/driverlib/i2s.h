/******************************************************************************
*  Filename:       i2s.h
*  Revised:        2018-11-16 11:16:53 +0100 (Fri, 16 Nov 2018)
*  Revision:       53356
*
*  Description:    Defines and prototypes for the I2S.
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

//****************************************************************************
//
//! \addtogroup peripheral_group
//! @{
//! \addtogroup i2s_api
//! @{
//
//****************************************************************************

#ifndef __I2S_H__
#define __I2S_H__

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
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_i2s.h"
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
    #define I2SEnable                       NOROM_I2SEnable
    #define I2SAudioFormatConfigure         NOROM_I2SAudioFormatConfigure
    #define I2SChannelConfigure             NOROM_I2SChannelConfigure
    #define I2SBufferConfig                 NOROM_I2SBufferConfig
    #define I2SPointerUpdate                NOROM_I2SPointerUpdate
    #define I2SPointerSet                   NOROM_I2SPointerSet
    #define I2SSampleStampConfigure         NOROM_I2SSampleStampConfigure
    #define I2SSampleStampGet               NOROM_I2SSampleStampGet
#endif

//*****************************************************************************
//
//! \brief A structure that defines an audio control table. Note: Memory for this
//! structure \b must be initialized by user application. See detailed description!
//!
//! \deprecated This structure will be removed in a future release.
//!
//! These fields are used by the I2S and normally it is not necessary for
//! software to directly read or write fields in the table.
//!
//! \note The control table must be defined by the user as a global variable and
//! the global pointer must then be assigned the address of the control table
//! inside a user function (but before calling any I2S-function).
//!
/*!
\verbatim
 I2SControlTable g_controlTable;    // Define global
 g_pControlTable = &g_controlTable; // Assign pointer (inside a function)
\endverbatim
*/
//!
//
//*****************************************************************************
#ifndef DEPRECATED
typedef struct
{
    uint16_t ui16DMABufSize;        //!< Size of DMA buffer in number of samples.
    uint16_t ui16ChBufSize;         //!< Size of Channel buffer.
    uint8_t ui8InChan;              //!< Input Channel.
    uint8_t ui8OutChan;             //!< Output Channel.
    uint16_t ui16MemLen;            //!< Length of the audio words stored in memory.
    uint32_t ui32InBase;            //!< Base address of the input buffer.
    uint32_t ui32InOffset;          //!< Value of the current input pointer offset.
    uint32_t ui32OutBase;           //!< Base address of the output buffer.
    uint32_t ui32OutOffset;         //!< Value of the current output pointer offset.
} I2SControlTable;
#endif

//*****************************************************************************
//
// Declare global pointer to the I2S data structure.
//
// The control table must be defined by the user as a global variable and the
// global pointer must then be assigned the address of the control table:
//
// I2SControlTable g_controlTable;
// g_pControlTable = &g_controlTable;
//
//*****************************************************************************
#ifndef DEPRECATED
extern I2SControlTable *g_pControlTable;
#endif

//*****************************************************************************
//
// Defines for the I2S DMA buffer sizes
//
//*****************************************************************************
#ifndef DEPRECATED
#define I2S_DMA_BUF_SIZE_64     0x00000040
#define I2S_DMA_BUF_SIZE_128    0x00000080
#define I2S_DMA_BUF_SIZE_256    0x00000100
#endif

//*****************************************************************************
//
// Defines for the I2S audio clock configuration
//
//*****************************************************************************
#ifndef DEPRECATED
#define I2S_EXT_WCLK            0x00000001
#define I2S_INT_WCLK            0x00000002
#define I2S_INVERT_WCLK         0x00000004
#define I2S_NORMAL_WCLK         0x00000000
#endif

//*****************************************************************************
//
// Defines for the audio data line input/output configuration
//
//*****************************************************************************
#ifndef DEPRECATED
#define I2S_LINE_UNUSED         0x00000000
#define I2S_LINE_INPUT          0x00000001
#define I2S_LINE_OUTPUT         0x00000002
#define I2S_LINE_MASK           0x00000003
#endif

//*****************************************************************************
//
// Defines for activating an audio channel.
//
//*****************************************************************************
#ifndef DEPRECATED
#define I2S_CHAN0_ACT           0x00000100
#define I2S_CHAN1_ACT           0x00000200
#define I2S_CHAN2_ACT           0x00000400
#define I2S_CHAN3_ACT           0x00000800
#define I2S_CHAN4_ACT           0x00001000
#define I2S_CHAN5_ACT           0x00002000
#define I2S_CHAN6_ACT           0x00004000
#define I2S_CHAN7_ACT           0x00008000
#define I2S_MONO_MODE           0x00000100
#define I2S_STEREO_MODE         0x00000300
#define I2S_CHAN_CFG_MASK       0x0000FF00
#endif

#define I2S_CHAN0_MASK          0x00000001
#define I2S_CHAN1_MASK          0x00000002
#define I2S_CHAN2_MASK          0x00000004
#define I2S_CHAN3_MASK          0x00000008
#define I2S_CHAN4_MASK          0x00000010
#define I2S_CHAN5_MASK          0x00000020
#define I2S_CHAN6_MASK          0x00000040
#define I2S_CHAN7_MASK          0x00000080

//*****************************************************************************
//
// Defines for the audio format configuration
//
//*****************************************************************************
#define I2S_MEM_LENGTH_16       0x00000000  // 16 bit size of word in memory
#define I2S_MEM_LENGTH_24       0x00000080  // 24 bit size of word in memory
#define I2S_POS_EDGE            0x00000040  // Sample on positive edge
#define I2S_NEG_EDGE            0x00000000  // Sample on negative edge
#define I2S_DUAL_PHASE_FMT      0x00000020  // Dual Phased audio format
#define I2S_SINGLE_PHASE_FMT    0x00000000  // Single Phased audio format
#define I2S_WORD_LENGTH_8       0x00000008  // Word length is 8 bits
#define I2S_WORD_LENGTH_16      0x00000010  // Word length is 16 bits
#define I2S_WORD_LENGTH_24      0x00000018  // Word length is 24 bits

//*****************************************************************************
//
// Defines for the sample stamp counters
//
//*****************************************************************************
#ifndef DEPRECATED
#define I2S_STMP0               0x00000001  // Sample stamp counter channel 0
#define I2S_STMP1               0x00000002  // Sample stamp counter channel 1
#endif
#define I2S_STMP_SATURATION     0x0000FFFF  // The saturation value used when
                                            // calculating the sample stamp

//*****************************************************************************
//
// Defines for the interrupt
//
//*****************************************************************************
#define I2S_INT_DMA_IN          0x00000020  // DMA output buffer full interrupt
#define I2S_INT_DMA_OUT         0x00000010  // DMA input buffer empty interrupt
#define I2S_INT_TIMEOUT         0x00000008  // Word Clock Timeout
#define I2S_INT_BUS_ERR         0x00000004  // DMA Bus error
#define I2S_INT_WCLK_ERR        0x00000002  // Word Clock error
#define I2S_INT_PTR_ERR         0x00000001  // Data pointer error (DMA data was not updated in time).
#define I2S_INT_ALL             0x0000003F  // All interrupts

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
//! \brief Checks an I2S base address.
//!
//! This function determines if an I2S port base address is valid.
//!
//! \param ui32Base is the base address of the I2S port.
//!
//! \return Returns \c true if the base address is valid and \c false
//! otherwise.
//
//*****************************************************************************
static bool
I2SBaseValid(uint32_t ui32Base)
{
    return(ui32Base == I2S0_BASE);
}
#endif

//*****************************************************************************
//
//! \brief Enables the I2S module for operation.
//!
//! \deprecated This function will be removed in a future release.
//!
//! \note The module should only be enabled after configuration. When the
//! module is disabled, no data or clocks will be generated on the I2S signals.
//!
//! \note Immediately after enabling the module the programmer should update
//! the DMA data pointer registers using \ref I2SPointerUpdate() to ensure a new
//! pointer is written before the DMA transfer completes. Failure to update
//! the pointer in time will result in an \ref I2S_INT_PTR_ERR.
//!
//! \param ui32Base is the I2S module base address.
//!
//! \return None
//
//*****************************************************************************
#ifndef DEPRECATED
extern void I2SEnable(uint32_t ui32Base);
#endif

//*****************************************************************************
//
//! \brief Disables the I2S module for operation.
//!
//! \deprecated This function will be removed in a future release.
//!
//! This function will immediately disable the I2S module. To ensure that
//! all buffer operations are completed before shutting down, the correct
//! procedure is:
//! 1. Do not update the data pointers using \ref I2SPointerUpdate().
//! 2. Await next interrupt resulting in \ref I2S_INT_PTR_ERR.
//! 3. Disable the I2S using \ref I2SDisable() and clear the pointer error using
//!    \ref I2SIntClear().
//! 4. Disable bit clock source (done externally).
//!
//! \param ui32Base is the I2S module base address.
//!
//! \return None
//
//*****************************************************************************
#ifndef DEPRECATED
__STATIC_INLINE void
I2SDisable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Disable the I2S module.
    HWREG(I2S0_BASE + I2S_O_AIFDMACFG) = 0x0;
}
#endif

//*****************************************************************************
//
//! \brief Configures the I2S module.
//!
//! \deprecated This function will be removed in a future release.
//!
//! The word length defines the size of the word transmitted on the data
//! lines. For single phased formats \c I2S_WORD_LENGTH_x is the exact number
//! of bits per word. In dual phased format this is the maximum number of bits
//! per word. The size is set using \ref I2S_WORD_LENGTH_8,
//! \ref I2S_WORD_LENGTH_16 or \ref I2S_WORD_LENGTH_24.
//!
//! \param ui32Base is the I2S module base address.
//! \param ui32FmtCfg is the bitwise OR of several options:
//! - Sample size:
//!   - \ref I2S_MEM_LENGTH_16
//!   - \ref I2S_MEM_LENGTH_24
//! - Clock edge sampling:
//!   - \ref I2S_POS_EDGE
//!   - \ref I2S_NEG_EDGE
//! - Phase:
//!   - \ref I2S_DUAL_PHASE_FMT
//!   - \ref I2S_SINGLE_PHASE_FMT
//! - Word length:
//!   - \ref I2S_WORD_LENGTH_8
//!   - \ref I2S_WORD_LENGTH_16
//!   - \ref I2S_WORD_LENGTH_24
//! \param ui32BitClkDelay defines the bit clock delay by setting the number of bit clock periods between the
//! positive word clock edge and the MSB of the first word in a phase. The bit
//! clock delay is determined by the ratio between the bit clock and the frame
//! clock and the chosen audio format. The bit clock delay \b must be configured
//! depending on the chosen audio format:
//! - 0     : Left Justified Format (LJF).
//! - 1     : I2S and DSP format.
//! - 2-255 : Right Justified format (RJF).
//!
//! \return None
//!
//! \sa \ref I2SChannelConfigure()
//
//*****************************************************************************
#ifndef DEPRECATED
extern void I2SAudioFormatConfigure(uint32_t ui32Base, uint32_t ui32FmtCfg,
                                    uint32_t ui32BitClkDelay);
#endif

//****************************************************************************
//
//! \brief Setup the audio channel configuration.
//!
//! \deprecated This function will be removed in a future release.
//!
//! The channel configuration is a bitwise OR of the input/output mode of each
//! data line and the active audio channels within a specific audio frame.
//!
//! Setting up the input/output mode use one of:
//! - \ref I2S_LINE_UNUSED
//! - \ref I2S_LINE_INPUT
//! - \ref I2S_LINE_OUTPUT
//!
//! For dual phased audio (LJF,RJF,I2S) only mono and stereo modes are allowed.
//! For single phased audio format (DSP) up to 8 active channels are allowed
//! on a single data line. For setting up the active channels in a frame use:
//! - Single phased, use a bitwise OR'ed combination of:
//!   - \ref I2S_CHAN0_ACT
//!   - \ref I2S_CHAN1_ACT
//!   - \ref I2S_CHAN2_ACT
//!   - \ref I2S_CHAN3_ACT
//!   - \ref I2S_CHAN4_ACT
//!   - \ref I2S_CHAN5_ACT
//!   - \ref I2S_CHAN6_ACT
//!   - \ref I2S_CHAN7_ACT
//! - Dual phased, use one of:
//!   - \ref I2S_MONO_MODE (same as \ref I2S_CHAN0_ACT)
//!   - \ref I2S_STEREO_MODE (same as \ref I2S_CHAN0_ACT | \ref I2S_CHAN1_ACT)
//!
//! \note The audio format and the clock configuration should be set using
//! \ref I2SAudioFormatConfigure()
//!
//! \param ui32Base is base address of the I2S module.
//! \param ui32Chan0Cfg defines the channel configuration for data line 0.
//! \param ui32Chan1Cfg defines the channel configuration for data line 1.
//!
//! \return None
//!
//! \sa \ref I2SAudioFormatConfigure()
//
//****************************************************************************
#ifndef DEPRECATED
extern void I2SChannelConfigure(uint32_t ui32Base, uint32_t ui32Chan0Cfg,
                                uint32_t ui32Chan1Cfg);
#endif

//****************************************************************************
//
//! \brief Configure the I2S frame clock.
//!
//! \deprecated This function will be removed in a future release.
//!
//! Configure I2S clock to be either internal or external and either normal
//! or inverted.
//!
//! \note The bit clock configuration is done externally, but the internal/
//! external setting must match what is chosen internally in the I2S module
//! for the frame clock.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param ui32ClkConfig is the clock configuration parameter. Bitwise OR'ed
//! combination of clock source and clock polarity:
//! - Clock source:
//!   - \ref I2S_EXT_WCLK : External clock.
//!   - \ref I2S_INT_WCLK : Internal clock.
//! - Clock polarity:
//!   - \ref I2S_NORMAL_WCLK : Normal clock.
//!   - \ref I2S_INVERT_WCLK : Inverted clock.
//!
//! \return None
//
//****************************************************************************
#ifndef DEPRECATED
__STATIC_INLINE void
I2SClockConfigure(uint32_t ui32Base, uint32_t ui32ClkConfig)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Setup register WCLK Source.
    HWREG(I2S0_BASE + I2S_O_AIFWCLKSRC) = ui32ClkConfig &
                                         (I2S_AIFWCLKSRC_WCLK_INV_M |
                                          I2S_AIFWCLKSRC_WCLK_SRC_M);
}
#endif

//****************************************************************************
//
//! \brief Set the input buffer pointers.
//!
//! \deprecated This function will be removed in a future release.
//!
//! The next pointer should always be written while the DMA is using the
//! previous written pointer. If not written in time an \ref I2S_INT_PTR_ERR will
//! occur and all outputs will be disabled.
//!
//! \note At startup the next data pointer should be
//! written just before and just after calling the \ref I2SEnable().
//!
//! \param ui32Base is the base address of the I2S module.
//! \param ui32InBufBase is the address of the input buffer.
//! \param ui32OutBufBase is the address of the output  buffer.
//! \param ui16DMABufSize is the size of the DMA buffers. Must be greater than 0!
//! \param ui16ChanBufSize is the size of the channel buffers.
//!
//! \return None
//
//****************************************************************************
#ifndef DEPRECATED
extern void I2SBufferConfig(uint32_t ui32Base, uint32_t ui32InBufBase,
                            uint32_t ui32OutBufBase, uint16_t ui16DMABufSize,
                            uint16_t ui16ChanBufSize);
#endif

//****************************************************************************
//
//! \brief Update the buffer pointers.
//!
//! \deprecated This function will be removed in a future release.
//!
//! The next pointer should always be written while the DMA is using the
//! previous written pointer. If not written in time an \ref I2S_INT_PTR_ERR will occur
//! and all outputs will be disabled. Nothing is preventing the pointers from
//! being identical, but this function relies on both pointers (input or
//! output pointers) are pointing to a valid address.
//!
//! \note It is recommended that the pointer update is done in an interrupt context
//! to ensure that the update is performed before the buffer is full.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param bInput determines whether to update input or output pointer.
//! - \c true  : Update input pointer.
//! - \c false : Update output pointer
//!
//! \return None
//!
//! \sa \ref I2SPointerSet()
//
//****************************************************************************
#ifndef DEPRECATED
extern void I2SPointerUpdate(uint32_t ui32Base, bool bInput);
#endif

//****************************************************************************
//
//! \brief Set a buffer pointer (input or output) directly.
//!
//! \deprecated This function will be removed in a future release.
//!
//! This function allows bypassing of the pointers in the global control table.
//!
//! The next pointer should always be written while the DMA is using the
//! previous written pointer. If not written in time an \ref I2S_INT_PTR_ERR will occur
//! and all outputs will be disabled. Nothing is preventing the pointers from
//! being identical, but this function relies on both pointers (input or
//! output pointers) are pointing to a valid address.
//!
//! \note It is recommended that the pointer update is done in an interrupt context
//! to ensure that the update is performed before the buffer is full.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param bInput determines whether to update input or output pointer.
//! - \c true  : Update input pointer.
//! - \c false : Update output pointer
//! \param pNextPointer is a void pointer to user defined buffer.
//!
//! \return None
//!
//! \sa \ref I2SPointerUpdate()
//
//****************************************************************************
#ifndef DEPRECATED
extern void I2SPointerSet(uint32_t ui32Base, bool bInput, void * pNextPointer);
#endif

//*****************************************************************************
//
//! \brief Registers an interrupt handler for an I2S interrupt in the dynamic interrupt table.
//!
//! \deprecated This function will be removed in a future release.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific I2S interrupts must be enabled via \ref I2SIntEnable(). It is the interrupt
//! handler's responsibility to clear the interrupt source.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param pfnHandler is a pointer to the function to be called when the
//! I2S interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
#ifndef DEPRECATED
__STATIC_INLINE void
I2SIntRegister(uint32_t ui32Base, void (*pfnHandler)(void))
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Register the interrupt handler.
    IntRegister(INT_I2S_IRQ, pfnHandler);

    // Enable the I2S interrupt.
    IntEnable(INT_I2S_IRQ);
}
#endif

//*****************************************************************************
//
//! \brief Unregisters an interrupt handler for a I2S interrupt in the dynamic interrupt table.
//!
//! \deprecated This function will be removed in a future release.
//!
//! This function does the actual unregistering of the interrupt handler. It
//! clears the handler to be called when an I2S interrupt occurs.  This
//! function also masks off the interrupt in the interrupt controller so that
//! the interrupt handler no longer is called.
//!
//! \param ui32Base is the base address of the I2S port.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
#ifndef DEPRECATED
__STATIC_INLINE void
I2SIntUnregister(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Disable the interrupt.
    IntDisable(INT_I2S_IRQ);

    // Unregister the interrupt handler.
    IntUnregister(INT_I2S_IRQ);
}
#endif

//*****************************************************************************
//
//! \brief Configure the sample stamp generator.
//!
//! \deprecated This function will be removed in a future release.
//!
//! Use this function to configure the sample stamp generator.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param bInput enables triggering of the sample stamp generator on input.
//! \param bOutput enables triggering of the sample stamp generator on output.
//!
//! \return None
//
//*****************************************************************************
#ifndef DEPRECATED
extern void I2SSampleStampConfigure(uint32_t ui32Base, bool bInput,
                                    bool bOutput);
#endif

//*****************************************************************************
//
//! \brief Enables individual I2S interrupt sources.
//!
//! This function enables the indicated I2S interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \param ui32Base is the base address of the I2S port.
//! \param ui32IntFlags is the bit mask of the interrupt sources to be enabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref I2S_INT_DMA_IN
//! - \ref I2S_INT_DMA_OUT
//! - \ref I2S_INT_TIMEOUT
//! - \ref I2S_INT_BUS_ERR
//! - \ref I2S_INT_WCLK_ERR
//! - \ref I2S_INT_PTR_ERR
//! - \ref I2S_INT_ALL (covers all the above)
//!
//! \return None.
//
//*****************************************************************************
__STATIC_INLINE void
I2SIntEnable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Enable the specified interrupts.
    HWREG(I2S0_BASE + I2S_O_IRQMASK) |= ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Disables individual I2S interrupt sources.
//!
//! This function disables the indicated I2S interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \param ui32Base is the base address of the I2S port.
//! \param ui32IntFlags is the bit mask of the interrupt sources to be disabled.
//! The parameter is the bitwise OR of any of the following:
//! - \ref I2S_INT_DMA_IN
//! - \ref I2S_INT_DMA_OUT
//! - \ref I2S_INT_TIMEOUT
//! - \ref I2S_INT_BUS_ERR
//! - \ref I2S_INT_WCLK_ERR
//! - \ref I2S_INT_PTR_ERR
//! - \ref I2S_INT_ALL (covers all the above)
//!
//! \return None.
//
//*****************************************************************************
__STATIC_INLINE void
I2SIntDisable(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Disable the specified interrupts.
    HWREG(I2S0_BASE + I2S_O_IRQMASK) &= ~ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Gets the current interrupt status.
//!
//! This function returns the interrupt status for the specified I2S. Either
//! the raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \param ui32Base is the base address of the I2S port
//! \param bMasked selects between raw and masked interrupt status:
//! - \c false : Raw interrupt status is required.
//! - \c true  : Masked interrupt status is required.
//!
//! \return Returns the current interrupt status as a vector of:
//! - \ref I2S_INT_DMA_IN
//! - \ref I2S_INT_DMA_OUT
//! - \ref I2S_INT_TIMEOUT
//! - \ref I2S_INT_BUS_ERR
//! - \ref I2S_INT_WCLK_ERR
//! - \ref I2S_INT_PTR_ERR
//
//*****************************************************************************
__STATIC_INLINE uint32_t
I2SIntStatus(uint32_t ui32Base, bool bMasked)
{
    uint32_t ui32Mask;

    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Return either the interrupt status or the raw interrupt status as
    // requested.
    if(bMasked)
    {
        ui32Mask = HWREG(I2S0_BASE + I2S_O_IRQFLAGS);
        return(ui32Mask & HWREG(I2S0_BASE + I2S_O_IRQMASK));
    }
    else
    {
        return(HWREG(I2S0_BASE + I2S_O_IRQFLAGS));
    }
}

//*****************************************************************************
//
//! \brief Clears I2S interrupt sources.
//!
//! The specified I2S interrupt sources are cleared, so that they no longer
//! assert. This function must be called in the interrupt handler to keep the
//! interrupt from being recognized again immediately upon exit.
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
//! \param ui32Base is the base address of the I2S port.
//! \param ui32IntFlags is a bit mask of the interrupt sources to be cleared.
//! The parameter is the bitwise OR of any of the following:
//! - \ref I2S_INT_DMA_IN
//! - \ref I2S_INT_DMA_OUT
//! - \ref I2S_INT_TIMEOUT
//! - \ref I2S_INT_BUS_ERR
//! - \ref I2S_INT_WCLK_ERR
//! - \ref I2S_INT_PTR_ERR
//! - \ref I2S_INT_ALL (covers all the above)
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2SIntClear(uint32_t ui32Base, uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Clear the requested interrupt sources.
    HWREG(I2S0_BASE + I2S_O_IRQCLR) = ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Enable the Sample Stamp generator.
//!
//! Use this function to enable the sample stamp generators.
//!
//! \note It is the user's responsibility to ensure that the sample stamp
//! generator is properly configured before it is enabled. It is the setting
//! of the Input and Output triggers configured using \ref I2SSampleStampConfigure()
//! that triggers the start point of the audio streams.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2SSampleStampEnable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Set the enable bit.
    HWREG(I2S0_BASE + I2S_O_STMPCTL) = I2S_STMPCTL_STMP_EN;
}

//*****************************************************************************
//
//! \brief Disable the Sample Stamp generator.
//!
//! Use this function to disable the sample stamp generators. When the sample
//! stamp generator is disabled, the clock counters are automatically cleared.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2SSampleStampDisable(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Clear the enable bit.
    HWREG(I2S0_BASE + I2S_O_STMPCTL) = 0;

}

//*****************************************************************************
//
//! \brief Get the current value of a sample stamp counter.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param ui32Channel is the sample stamp counter to sample
//!
//! \return Returns the current value of the selected sample stamp channel.
//
//*****************************************************************************
extern uint32_t I2SSampleStampGet(uint32_t ui32Base, uint32_t ui32Channel);

//*****************************************************************************
//
//! \brief Starts the I2S.
//!
//! I2S must be configured before it is started.
//!
//! \note Immediately after enabling the module the programmer must update
//! the DMA data pointer registers using \ref I2SInPointerSet() and
//! \ref I2SOutPointerSet() to ensure a new pointer is written before the DMA
//! transfer completes. Failure to update the pointer in time will result in
//! an \ref I2S_INT_PTR_ERR.
//!
//! \param ui32Base is the I2S module base address.
//! \param ui8FixDMALength is the length of the DMA buffer: this will allow
//!        the DMA to read ui8FixDMALength between to pointer refreshes.
//!
//! \return None
//!
//! \sa \ref I2SStop()
//
//*****************************************************************************
__STATIC_INLINE void I2SStart(uint32_t ui32Base, uint8_t ui8FixDMALength)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Enable the I2S module.
    HWREG(I2S0_BASE + I2S_O_AIFDMACFG) = ui8FixDMALength;
}

//*****************************************************************************
//
//! \brief Stops the I2S module for operation.
//!
//! This function will immediately disable the I2S module. To ensure that
//! all buffer operations are completed before shutting down, the correct
//! procedure is:
//! 1. Do not update the data pointers using \ref I2SInPointerSet() and
//!    \ref I2SOutPointerSet().
//! 2. Await that values returned by \ref I2SInPointerNextGet(),
//!    \ref I2SOutPointerNextGet(), \ref I2SInPointerGet() and \ref I2SOutPointerGet()
//!    are zero.
//! 3. Disable the I2S using \ref I2SStop() and clear the pointer
//!    error using \ref I2SIntClear().
//! 4. Disable bit clock source (done externally).
//!
//! \param ui32Base is the I2S module base address.
//!
//! \return None
//!
//! \sa \ref I2SStart()
//
//*****************************************************************************
__STATIC_INLINE void I2SStop(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Disable the I2S module.
    HWREG(I2S0_BASE + I2S_O_AIFDMACFG) = 0x00;
}

//*****************************************************************************
//
//! \brief Configure the serial format of the I2S module.
//!
//! The word length defines the size of the word transmitted on the data
//! lines. For single phased formats \c ui8BitsPerSample is the exact number
//! of bits per word. In dual phased format this is the maximum number of bits
//! per word.
//!
//! \param ui32Base is the I2S module base address.
//! \param ui8iDataDelay is the number of BCLK periods between the first WCLK
//!         edge and the MSB of the first audio channel data transferred during
//!         the phase.
//! \param ui8iMemory24Bits selects if the samples in memory are coded on 16 bits
//!        or 24 bits. Possible values are:
//!        - \ref I2S_MEM_LENGTH_16
//!        - \ref I2S_MEM_LENGTH_24
//! \param ui8iSamplingEdge selects if sampling on falling or rising edges.
//!        Possible values are:
//!        - \ref I2S_NEG_EDGE
//!        - \ref I2S_POS_EDGE
//! \param boolDualPhase must be set to true for dual phase and to false for
//!        single phase and user-defined phase.
//! \param ui8BitsPerSample is the number of bits transmitted for each sample.
//!        If this number does not match with the memory length selected
//!        (16 bits or24 bits), samples will be truncated or padded.
//! \param ui16transmissionDelay is the number of WCLK periods before the first
//!        transmission.
//!
//! \return None
//!
//! \sa \ref I2SFrameConfigure()
//
//*****************************************************************************
__STATIC_INLINE void
I2SFormatConfigure(uint32_t ui32Base,
                   uint8_t  ui8iDataDelay,
                   uint8_t  ui8iMemory24Bits,
                   uint8_t  ui8iSamplingEdge,
                   bool     boolDualPhase,
                   uint8_t  ui8BitsPerSample,
                   uint16_t ui16transmissionDelay)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));
    ASSERT(ui8BitsPerSample <= I2S_AIFFMTCFG_WORD_LEN_MAX);
    ASSERT(ui8BitsPerSample >= I2S_AIFFMTCFG_WORD_LEN_MIN);

    // Setup register AIFFMTCFG Source.
    HWREGH(I2S0_BASE + I2S_O_AIFFMTCFG) =
                                (ui8iDataDelay      << I2S_AIFFMTCFG_DATA_DELAY_S) |
                                (ui8iMemory24Bits   << I2S_AIFFMTCFG_MEM_LEN_24_S) |
                                (ui8iSamplingEdge   << I2S_AIFFMTCFG_SMPL_EDGE_S ) |
                                (boolDualPhase      << I2S_AIFFMTCFG_DUAL_PHASE_S) |
                                (ui8BitsPerSample   << I2S_AIFFMTCFG_WORD_LEN_S  );

    // Number of WCLK periods before the first read / write
    HWREGH(I2S0_BASE + I2S_O_STMPWPER) = ui16transmissionDelay;
}

//****************************************************************************
//
//! \brief Setup the two interfaces SD0 and SD1 (also called AD0 and AD1).
//!
//! This function sets interface's direction and activated channels.
//!
//! \param ui32Base is base address of the I2S module.
//! \param ui8StatusAD0 defines the usage of AD0
//!         0x00: AD0 is disabled
//!         0x01, AD0 is an input
//!         0x02, AD0 is an output
//! \param ui8ChanAD0 defines the channel mask for AD0.
//!        Use a bitwise OR'ed combination of:
//!         - \ref I2S_CHAN0_MASK
//!         - \ref I2S_CHAN1_MASK
//!         - \ref I2S_CHAN2_MASK
//!         - \ref I2S_CHAN3_MASK
//!         - \ref I2S_CHAN4_MASK
//!         - \ref I2S_CHAN5_MASK
//!         - \ref I2S_CHAN6_MASK
//!         - \ref I2S_CHAN7_MASK
//! \param ui8StatusAD1 defines the usage of AD1
//!         0x00: AD1 is disabled
//!         0x10, AD1 is an input
//!         0x20, AD1 is an output
//! \param ui8ChanAD1 defines the channel mask for AD1.
//!        Use a bitwise OR'ed combination of:
//!         - \ref I2S_CHAN0_MASK
//!         - \ref I2S_CHAN1_MASK
//!         - \ref I2S_CHAN2_MASK
//!         - \ref I2S_CHAN3_MASK
//!         - \ref I2S_CHAN4_MASK
//!         - \ref I2S_CHAN5_MASK
//!         - \ref I2S_CHAN6_MASK
//!         - \ref I2S_CHAN7_MASK
//!
//! \return None
//!
//! \sa \ref I2SFormatConfigure()
//
//****************************************************************************
__STATIC_INLINE void
I2SFrameConfigure(uint32_t ui32Base,
                  uint8_t  ui8StatusAD0, uint8_t  ui8ChanAD0,
                  uint8_t  ui8StatusAD1, uint8_t  ui8ChanAD1)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Configure input/output channels.
    HWREGB(I2S0_BASE + I2S_O_AIFDIRCFG) = (ui8StatusAD0 | ui8StatusAD1);

    // Configure the valid channel mask.
    HWREGB(I2S0_BASE + I2S_O_AIFWMASK0) = ui8ChanAD0;
    HWREGB(I2S0_BASE + I2S_O_AIFWMASK1) = ui8ChanAD1;
}

//****************************************************************************
//
//! \brief Configure the I2S frame clock (also called WCLK or WS).
//!
//! Configure WCLK clock to be either internal (master) or external (slave).
//! Configure WCLK clock either normal or inverted.
//!
//! \note The bit clock configuration is done externally, but the internal/
//! external setting must match what is chosen internally in the I2S module
//! for the frame clock.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param boolMaster false: the device is a slave (external clock)
//!                   true: the device is a master (internal clock)
//! \param boolWCLKInvert false: WCLK is not inverted
//!                       true: WCLK is internally inverted
//!
//! \return None
//
//****************************************************************************
__STATIC_INLINE void
I2SWclkConfigure(uint32_t ui32Base,
                 bool     boolMaster,
                 bool     boolWCLKInvert)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));
    ASSERT(ui8ClkSource < I2S_AIFWCLKSRC_WCLK_SRC_RESERVED);

    // if(boolMaster == 0) then ui8ClkSource = 1
    // if(boolMaster == 1) then ui8ClkSource = 2
    uint8_t ui8ClkSource = (uint8_t)boolMaster + 0x01;

    // Setup register WCLK Source.
    HWREGB(I2S0_BASE + I2S_O_AIFWCLKSRC) =
                               ((ui8ClkSource       << I2S_AIFWCLKSRC_WCLK_SRC_S) |
                                (boolWCLKInvert     << I2S_AIFWCLKSRC_WCLK_INV_S ));
}

//****************************************************************************
//
//! \brief Set the input buffer pointer.
//!
//! The next pointer should always be written while the DMA is using the
//! previous written pointer. If not written in time an \ref I2S_INT_PTR_ERR
//! will occur and all inputs and outputs will be disabled.
//! This function relies on pointer is pointing to a valid address.
//!
//! \note It is recommended that the pointer update is done in an interrupt context
//! to ensure that the update is performed before the buffer is full.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param ui32NextPointer is the adress of the data
//!
//! \return None
//!
//! \sa \ref I2SOutPointerSet()
//
//****************************************************************************
__STATIC_INLINE void
I2SInPointerSet(uint32_t ui32Base, uint32_t ui32NextPointer)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    HWREG(I2S0_BASE + I2S_O_AIFINPTRNEXT) = ui32NextPointer;
}

//****************************************************************************
//
//! \brief Set the output buffer pointer.
//!
//! The next pointer should always be written while the DMA is using the
//! previous written pointer. If not written in time an \ref I2S_INT_PTR_ERR
//! will occur and all inputs and outputs will be disabled.
//! This function relies on pointer is pointing to a valid address.
//!
//! \note It is recommended that the pointer update is done in an interrupt context
//! to ensure that the update is performed before the buffer is full.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param ui32NextPointer is the adress of the data
//!
//! \return None
//!
//! \sa \ref I2SInPointerSet()
//
//****************************************************************************
__STATIC_INLINE void
I2SOutPointerSet(uint32_t ui32Base, uint32_t ui32NextPointer)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    HWREG(I2S0_BASE + I2S_O_AIFOUTPTRNEXT) = ui32NextPointer;
}

//****************************************************************************
//
//! \brief Get value stored in PTR NEXT IN register
//!
//! \param ui32Base is the base address of the I2S module.
//!
//! \return the value of PTR NEXT IN.
//
//****************************************************************************
__STATIC_INLINE uint32_t
I2SInPointerNextGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    return (HWREG(I2S0_BASE + I2S_O_AIFINPTRNEXT));
}


//****************************************************************************
//
//! \brief Get value stored in PTR NEXT OUT register
//!
//! \param ui32Base is the base address of the I2S module.
//!
//! \return the value of PTR NEXT OUT.
//
//****************************************************************************
__STATIC_INLINE uint32_t
I2SOutPointerNextGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    return (HWREG(I2S0_BASE + I2S_O_AIFOUTPTRNEXT));
}

//****************************************************************************
//
//! \brief Get value stored in PTR IN register
//!
//! \param ui32Base is the base address of the I2S module.
//!
//! \return the value of PTR IN.
//
//****************************************************************************
__STATIC_INLINE uint32_t
I2SInPointerGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    return (HWREG(I2S0_BASE + I2S_O_AIFINPTR));
}

//****************************************************************************
//
//! \brief Get value stored in PTR OUT register
//!
//! \param ui32Base is the base address of the I2S module.
//!
//! \return the value of PTR OUT.
//
//****************************************************************************
__STATIC_INLINE uint32_t
I2SOutPointerGet(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    return (HWREG(I2S0_BASE + I2S_O_AIFOUTPTR));
}

//*****************************************************************************
//
//! \brief Configure the IN sample stamp generator.
//!
//! Use this function to configure the sample stamp generator.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param ui16TrigValue value used to set the trigger.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2SSampleStampInConfigure(uint32_t ui32Base, uint16_t ui16TrigValue)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Setup the sample stamp trigger for input streams.
    HWREGH(I2S0_BASE + I2S_O_STMPINTRIG) = ui16TrigValue;
}

//*****************************************************************************
//
//! \brief Configure the OUT sample stamp generator.
//!
//! Use this function to configure the sample stamp generator.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param ui16TrigValue value used to set the trigger.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2SSampleStampOutConfigure(uint32_t ui32Base, uint16_t ui16TrigValue)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    // Setup the sample stamp trigger for output streams.
    HWREGH(I2S0_BASE + I2S_O_STMPOUTTRIG) = ui16TrigValue;
}

//*****************************************************************************
//
//! \brief Add the specified value to the WCLK count.
//!
//! \param ui32Base is the base address of the I2S module.
//! \param i16Value is the offset to add to the counter (this value can be negative)
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2SWclkCounterConfigure(uint32_t ui32Base, int16_t i16Value)
{
    uint16_t ui16MinusValue;

    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    if (i16Value >= 0)
    {
        HWREGH(I2S0_BASE + I2S_O_STMPWADD) = i16Value;
    }
    else
    {
        ui16MinusValue = (uint16_t)(-i16Value);
        HWREGH(I2S0_BASE + I2S_O_STMPWADD) = HWREGH(I2S0_BASE + I2S_O_STMPWPER) - ui16MinusValue;
    }
}

//*****************************************************************************
//
//! \brief Reset the WCLK count.
//!
//! \param ui32Base is the base address of the I2S module.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
I2SWclkCounterReset(uint32_t ui32Base)
{
    // Check the arguments.
    ASSERT(I2SBaseValid(ui32Base));

    HWREGH(I2S0_BASE + I2S_O_STMPWSET) = 0;
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_I2SEnable
        #undef  I2SEnable
        #define I2SEnable                       ROM_I2SEnable
    #endif
    #ifdef ROM_I2SAudioFormatConfigure
        #undef  I2SAudioFormatConfigure
        #define I2SAudioFormatConfigure         ROM_I2SAudioFormatConfigure
    #endif
    #ifdef ROM_I2SChannelConfigure
        #undef  I2SChannelConfigure
        #define I2SChannelConfigure             ROM_I2SChannelConfigure
    #endif
    #ifdef ROM_I2SBufferConfig
        #undef  I2SBufferConfig
        #define I2SBufferConfig                 ROM_I2SBufferConfig
    #endif
    #ifdef ROM_I2SPointerUpdate
        #undef  I2SPointerUpdate
        #define I2SPointerUpdate                ROM_I2SPointerUpdate
    #endif
    #ifdef ROM_I2SPointerSet
        #undef  I2SPointerSet
        #define I2SPointerSet                   ROM_I2SPointerSet
    #endif
    #ifdef ROM_I2SSampleStampConfigure
        #undef  I2SSampleStampConfigure
        #define I2SSampleStampConfigure         ROM_I2SSampleStampConfigure
    #endif
    #ifdef ROM_I2SSampleStampGet
        #undef  I2SSampleStampGet
        #define I2SSampleStampGet               ROM_I2SSampleStampGet
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

#endif //  __I2S_H__

//****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//****************************************************************************
