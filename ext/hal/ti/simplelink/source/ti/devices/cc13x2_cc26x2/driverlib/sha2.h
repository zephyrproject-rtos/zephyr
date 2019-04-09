/******************************************************************************
*  Filename:       sha2.h
*  Revised:        2018-04-17 16:04:03 +0200 (Tue, 17 Apr 2018)
*  Revision:       51893
*
*  Description:    SHA-2 header file.
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
//! \addtogroup sha2_api
//! @{
//
//*****************************************************************************

#ifndef __SHA2_H__
#define __SHA2_H__

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
#include <stddef.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_crypto.h"
#include "../inc/hw_ccfg.h"
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
    #define SHA2StartDMAOperation           NOROM_SHA2StartDMAOperation
    #define SHA2WaitForIRQFlags             NOROM_SHA2WaitForIRQFlags
    #define SHA2ComputeInitialHash          NOROM_SHA2ComputeInitialHash
    #define SHA2ComputeIntermediateHash     NOROM_SHA2ComputeIntermediateHash
    #define SHA2ComputeFinalHash            NOROM_SHA2ComputeFinalHash
    #define SHA2ComputeHash                 NOROM_SHA2ComputeHash
#endif

//*****************************************************************************
//
// Values that can be passed to SHA2IntEnable, SHA2IntDisable, and SHA2IntClear
// as the intFlags parameter, and returned from SHA2IntStatus.
// Only SHA2_DMA_IN_DONE and SHA2_RESULT_RDY are routed to the NVIC. Check each
// function to see if it supports other interrupt status flags.
//
//*****************************************************************************
#define SHA2_DMA_IN_DONE                (CRYPTO_IRQEN_DMA_IN_DONE_M)
#define SHA2_RESULT_RDY                 (CRYPTO_IRQEN_RESULT_AVAIL_M)
#define SHA2_DMA_BUS_ERR                (CRYPTO_IRQCLR_DMA_BUS_ERR_M)


//*****************************************************************************
//
//  General constants
//
//*****************************************************************************

// SHA-2 module return codes
#define SHA2_SUCCESS                        0
#define SHA2_INVALID_ALGORITHM              1
#define SHA2_DMA_BUSY                       3
#define SHA2_DMA_ERROR                      4
#define SHA2_DIGEST_NOT_READY               5
#define SHA2_OLD_DIGEST_NOT_READ            6

// SHA-2 output digest lengths in bytes.
#define SHA2_SHA224_DIGEST_LENGTH_BYTES     (224 / 8)
#define SHA2_SHA256_DIGEST_LENGTH_BYTES     (256 / 8)
#define SHA2_SHA384_DIGEST_LENGTH_BYTES     (384 / 8)
#define SHA2_SHA512_DIGEST_LENGTH_BYTES     (512 / 8)

//Selectable SHA-2 modes. They determine the algorithm used and if initial
//values will be set to the default constants or not
#define SHA2_MODE_SELECT_SHA224             (CRYPTO_HASHMODE_SHA224_MODE_M)
#define SHA2_MODE_SELECT_SHA256             (CRYPTO_HASHMODE_SHA256_MODE_M)
#define SHA2_MODE_SELECT_SHA384             (CRYPTO_HASHMODE_SHA384_MODE_M)
#define SHA2_MODE_SELECT_SHA512             (CRYPTO_HASHMODE_SHA512_MODE_M)
#define SHA2_MODE_SELECT_NEW_HASH           (CRYPTO_HASHMODE_NEW_HASH_M)

// SHA-2 block lengths. When hashing block-wise, they define the size of each
// block provided to the new and intermediate hash functions.
#define SHA2_SHA224_BLOCK_SIZE_BYTES        (512 / 8)
#define SHA2_SHA256_BLOCK_SIZE_BYTES        (512 / 8)
#define SHA2_SHA384_BLOCK_SIZE_BYTES        (1024 / 8)
#define SHA2_SHA512_BLOCK_SIZE_BYTES        (1024 / 8)

// DMA status codes
#define SHA2_DMA_CHANNEL0_ACTIVE            (CRYPTO_DMASTAT_CH0_ACT_M)
#define SHA2_DMA_CHANNEL1_ACTIVE            (CRYPTO_DMASTAT_CH1_ACT_M)
#define SHA2_DMA_PORT_ERROR                 (CRYPTO_DMASTAT_PORT_ERR_M)

// Crypto module DMA operation types
#define SHA2_ALGSEL_SHA256                  0x04
#define SHA2_ALGSEL_SHA512                  0x08
#define SHA2_ALGSEL_TAG                     (CRYPTO_ALGSEL_TAG_M)



//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Start a crypto DMA operation
//!
//!        Enable the crypto DMA channels, configure the channel addresses,
//!        and set the length of the data transfer.
//!        Setting the length of the data transfer automatically starts the
//!        transfer. It is also used by the hardware module as a signal to
//!        begin the encryption, decryption, or MAC operation.
//!
//! \param [in] channel0Addr
//!        A pointer to the address channel 0 shall use.
//!
//! \param [in] channel0Length
//!        Length of the data in bytes to be read from or written to at
//!        \c channel0Addr. Set to 0 to not set up this channel.
//!
//! \param [out] channel1Addr
//!        A pointer to the address channel 1 shall use.
//!
//! \param [in] channel1Length
//!        Length of the data in bytes to be read from or written to at
//!        \c channel1Addr. Set to 0 to not set up this channel.
//!
//! \return None
//
//*****************************************************************************
extern void SHA2StartDMAOperation(uint8_t *channel0Addr, uint32_t channel0Length,  uint8_t *channel1Addr, uint32_t channel1Length);

//*****************************************************************************
//
//! \brief Poll the interrupt status register and clear when done.
//!
//!        This function polls until one of the bits in the \c irqFlags is
//!        asserted. Only \ref SHA2_DMA_IN_DONE and \ref SHA2_RESULT_RDY can actually
//!        trigger the interrupt line. That means that one of those should
//!        always be included in \c irqFlags and will always be returned together
//!        with any error codes.
//!
//! \param [in] irqFlags
//!        IRQ flags to poll and mask that the status register will be
//!        masked with. Consists of any bitwise OR of the flags
//!        below that includes at least one of
//!        \ref SHA2_DMA_IN_DONE or \ref SHA2_RESULT_RDY :
//! - \ref SHA2_DMA_IN_DONE
//! - \ref SHA2_RESULT_RDY
//! - \ref SHA2_DMA_BUS_ERR
//!
//! \return Returns the IRQ status register masked with \c irqFlags. May be any
//!         bitwise OR of the following masks:
//! - \ref SHA2_DMA_IN_DONE
//! - \ref SHA2_RESULT_RDY
//! - \ref SHA2_DMA_BUS_ERR
//
//*****************************************************************************
extern uint32_t SHA2WaitForIRQFlags(uint32_t irqFlags);

//*****************************************************************************
//
//! \brief Start a new SHA-2 hash operation.
//!
//!        This function begins a new piecewise hash operation.
//!
//!        Call this function when starting a new hash operation and the
//!        entire message is not yet available.
//!
//!        Call SHA2ComputeIntermediateHash() or SHA2ComputeFinalHash()
//!        after this call.
//!
//!        If the device shall go into standby in between calls to this
//!        function and either SHA2ComputeIntermediateHash() or
//!        SHA2ComputeFinalHash(), the intermediate digest must be saved in
//!        system RAM.
//!
//! \param [in] message
//!        Byte array containing the start of the message to hash.
//!        Must be exactly as long as the block length of the selected
//!        algorithm.
//! - \ref SHA2_SHA224_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA256_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA384_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA512_BLOCK_SIZE_BYTES
//!
//! \param [out] intermediateDigest
//!        Pointer to intermediate digest.
//! - NULL The intermediate digest will be stored in the internal
//!        registers of the SHA module.
//! - Not NULL Specifies the location the intermediate digest will
//!        be written to.
//!
//!        Must be of a length equal to the digest length of the selected
//!        hash algorithm.
//!        Must be 32-bit aligned. \c intermediateDigest is copied into the
//!        registers through the AHB slave interface in
//!        SHA2ComputeIntermediateHash() and SHA2ComputeFinalHash().
//!        This can only be done word-by-word.
//!
//! \param [in] hashAlgorithm Selects the hash algorithm to use. One of:
//! - \ref SHA2_MODE_SELECT_SHA224
//! - \ref SHA2_MODE_SELECT_SHA256
//! - \ref SHA2_MODE_SELECT_SHA384
//! - \ref SHA2_MODE_SELECT_SHA512
//!
//! \param [in] initialMessageLength The length in bytes of the first
//!             section of the message to process. Must be a multiple of the
//!             block size.
//!
//! \return Returns a SHA-2 return code.
//! - \ref SHA2_SUCCESS
//! - \ref SHA2_INVALID_ALGORITHM
//!
//! \sa SHA2ComputeIntermediateHash()
//! \sa SHA2ComputeFinalHash()
//
//*****************************************************************************
extern uint32_t SHA2ComputeInitialHash(const uint8_t *message, uint32_t *intermediateDigest, uint32_t hashAlgorithm, uint32_t initialMessageLength);

//*****************************************************************************
//
//! \brief Resume a SHA-2 hash operation but do not finalize it.
//!
//!        This function resumes a previous hash operation.
//!
//!        Call this function when continuing a hash operation and the
//!        message is not yet complete.
//!
//!        Call this function again or SHA2ComputeFinalHash()
//!        after this call.
//!
//!        If the device shall go into standby in between calls to this
//!        function and SHA2ComputeFinalHash(), the intermediate
//!        digest must be saved in system RAM.
//!
//! \param [in] message
//!        Byte array containing the start of the current block of the
//!        message to hash.
//!        Must be exactly as long as the block length of the selected
//!        algorithm.
//! - \ref SHA2_SHA224_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA256_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA384_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA512_BLOCK_SIZE_BYTES
//!
//! \param [in, out] intermediateDigest
//!        Pointer to intermediate digest.
//! - NULL The intermediate digest will be sourced from the internal
//!        registers of the SHA module and stored there after the
//!        operation completes.
//! - Not NULL Specifies the location the intermediate digest will
//!        be read from and written to.
//!
//!        Must be of a length equal to the digest length of the selected
//!        hash algorithm.
//!        Must be 32-bit aligned. \c intermediateDigest is copied from and
//!        to the registers through the AHB slave interface.
//!        This can only be done word-by-word.
//!
//! \param [in] hashAlgorithm Selects the hash algorithm to use. One of:
//! - \ref SHA2_MODE_SELECT_SHA224
//! - \ref SHA2_MODE_SELECT_SHA256
//! - \ref SHA2_MODE_SELECT_SHA384
//! - \ref SHA2_MODE_SELECT_SHA512
//!
//! \param [in] intermediateMessageLength The length in bytes of this
//!             section of the message to process. Must be a multiple of the
//!             block size.
//!
//! \return Returns a SHA-2 return code.
//! - \ref SHA2_SUCCESS
//! - \ref SHA2_INVALID_ALGORITHM
//!
//! \sa SHA2ComputeInitialHash()
//! \sa SHA2ComputeFinalHash()
//
//*****************************************************************************
extern uint32_t SHA2ComputeIntermediateHash(const uint8_t *message, uint32_t *intermediateDigest, uint32_t hashAlgorithm, uint32_t intermediateMessageLength);

//*****************************************************************************
//
//! \brief Resume a SHA-2 hash operation and finalize it.
//!
//!        This function resumes a previous hash session.
//!
//!        Call this function when continuing a hash operation and the
//!        message is complete.
//!
//! \param [in] message
//!        Byte array containing the final block of the message to hash.
//!        Any length <= the block size is acceptable.
//!        The DMA finalize the message as necessary.
//! - \ref SHA2_SHA224_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA256_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA384_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA512_BLOCK_SIZE_BYTES
//!
//! \param [out] resultDigest
//!        Byte array that the final digest will be written to. Must be of
//!        a length equal to the digest length of the selected hash algorithm.
//!
//! \param [in] intermediateDigest
//!        Pointer to intermediate digest.
//! - NULL The intermediate digest will be sourced from the internal
//!        registers of the SHA module.
//! - Not NULL Specifies the location the intermediate digest will
//!        be read from.
//!        Must be of a length equal to the digest length of the selected
//!        hash algorithm.
//!        Must be 32-bit aligned. \c intermediateDigest is copied from and
//!        to the registers through the AHB slave interface.
//!        This can only be done word-by-word.
//!
//! \param [in] totalMsgLength
//!        The length in bytes of the entire \c message including the sections
//!        passed to previous calls to SHA2ComputeInitialHash() and
//!        SHA2ComputeIntermediateHash().
//!
//! \param [in] messageLength The length in bytes of the last
//!             section of the message to process. Does not need to be
//!             a multiple of the block size.
//!
//! \param [in] hashAlgorithm Selects the hash algorithm to use. One of:
//! - \ref SHA2_MODE_SELECT_SHA224
//! - \ref SHA2_MODE_SELECT_SHA256
//! - \ref SHA2_MODE_SELECT_SHA384
//! - \ref SHA2_MODE_SELECT_SHA512
//!
//! \return Returns a SHA-2 return code.
//! - \ref SHA2_SUCCESS
//! - \ref SHA2_INVALID_ALGORITHM
//!
//! \sa SHA2ComputeInitialHash()
//! \sa SHA2ComputeIntermediateHash()
//
//*****************************************************************************
extern uint32_t SHA2ComputeFinalHash(const uint8_t *message, uint8_t *resultDigest, uint32_t *intermediateDigest, uint32_t totalMsgLength, uint32_t messageLength, uint32_t hashAlgorithm);

//*****************************************************************************
//
//! \brief Start a SHA-2 hash operation and return the finalized digest.
//!
//!        This function starts a hash operation and returns the finalized
//!        digest.
//!
//!        Use this function if the entire message is available when starting
//!        the hash.
//!
//! \param [in] message
//!        Byte array containing the message that will be hashed.
//!        Any length <= the block size is acceptable.
//!        The DMA will finalize the message as necessary.
//! - \ref SHA2_SHA224_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA256_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA384_BLOCK_SIZE_BYTES
//! - \ref SHA2_SHA512_BLOCK_SIZE_BYTES
//!
//! \param [out] resultDigest
//!        Byte array that the final digest will be written to. Must be of a
//!        length equal to the digest length of the selected hash algorithm.
//!
//! \param [in] totalMsgLength
//!        The length in bytes of the entire \c message.
//!
//! \param [in] hashAlgorithm Selects the hash algorithm to use. One of:
//! - \ref SHA2_MODE_SELECT_SHA224
//! - \ref SHA2_MODE_SELECT_SHA256
//! - \ref SHA2_MODE_SELECT_SHA384
//! - \ref SHA2_MODE_SELECT_SHA512
//!
//! \return Returns a SHA-2 return code.
//! - \ref SHA2_SUCCESS
//! - \ref SHA2_INVALID_ALGORITHM
//!
//
//*****************************************************************************
extern uint32_t SHA2ComputeHash(const uint8_t *message, uint8_t *resultDigest, uint32_t totalMsgLength, uint32_t hashAlgorithm);

//*****************************************************************************
//
//! \brief Configure the crypto DMA for a particular operation.
//!
//! \param algorithm
//!        Configures the crypto DMA for a particular operation.
//!        It also powers on the respective part of the system.
//!        \ref SHA2_ALGSEL_TAG may be combined with another flag. All other
//!        flags are mutually exclusive.
//! - 0 : Reset the module and turn off all sub-modules.
//! - \ref SHA2_ALGSEL_SHA256 Configure for a SHA224 or SHA256 operation.
//! - \ref SHA2_ALGSEL_SHA512 Configure for a SHA384 or SHA512 operation.
//! - \ref SHA2_ALGSEL_TAG Read out hash via DMA rather than the slave interface
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void SHA2SelectAlgorithm(uint32_t algorithm)
{
    ASSERT((algorithm == SHA2_ALGSEL_SHA256) ||
           (algorithm == SHA2_ALGSEL_SHA512) ||
           (algorithm == SHA2_ALGSEL_SHA256 | SHA2_ALGSEL_TAG) ||
           (algorithm == SHA2_ALGSEL_SHA512 | SHA2_ALGSEL_TAG));

    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = algorithm;
}



//*****************************************************************************
//
//! \brief Specify the total length of the message.
//!
//!        Despite specifying it here, the crypto DMA must still be
//!        set up with the correct data length.
//!
//!        Call this function only when setting up the final hash operation to
//!        enable finalization.
//!
//! \param length Total message length in bits.
//!
//! \return None
//!
//! \sa SHA2StartDMAOperation()
//
//*****************************************************************************
__STATIC_INLINE void SHA2SetMessageLength(uint32_t length)
{
    HWREG(CRYPTO_BASE + CRYPTO_O_HASHINLENL) = length;
    // CRYPTO_O_HASHINLENH is automatically set to 0. No need for the extra write.
}

//*****************************************************************************
//
//! \brief Load an intermediate digest.
//!
//! \param [in] digestLength
//!        Length of the digest in bytes. Must be one of:
//! - \ref SHA2_SHA224_DIGEST_LENGTH_BYTES
//! - \ref SHA2_SHA256_DIGEST_LENGTH_BYTES
//! - \ref SHA2_SHA384_DIGEST_LENGTH_BYTES
//! - \ref SHA2_SHA512_DIGEST_LENGTH_BYTES
//!
//! \param [in] digest
//!        Pointer to an intermediate digest. Must be 32-bit aligned.
//!
//
//*****************************************************************************
__STATIC_INLINE void SHA2SetDigest(uint32_t *digest, uint8_t digestLength)
{
    // Check the arguments.
    ASSERT(!(digest == NULL) && !((uint32_t)digest & 0x03));
    ASSERT((digestLength == SHA2_SHA224_DIGEST_LENGTH_BYTES) ||
           (digestLength == SHA2_SHA256_DIGEST_LENGTH_BYTES) ||
           (digestLength == SHA2_SHA384_DIGEST_LENGTH_BYTES) ||
           (digestLength == SHA2_SHA512_DIGEST_LENGTH_BYTES));

    // Write digest
    uint32_t i = 0;
    for (i = 0; i < (digestLength / sizeof(uint32_t)); i++) {
        HWREG(CRYPTO_BASE + CRYPTO_O_HASHDIGESTA + (i * sizeof(uint32_t))) = digest[i];
    }

}

//*****************************************************************************
//
//! \brief Read the intermediate or final digest.
//!
//! \param [in] digestLength Length of the digest in bytes. Must be one of:
//! - ref SHA2_SHA224_DIGEST_LENGTH_BYTES
//! - ref SHA2_SHA256_DIGEST_LENGTH_BYTES
//! - ref SHA2_SHA384_DIGEST_LENGTH_BYTES
//! - ref SHA2_SHA512_DIGEST_LENGTH_BYTES
//!
//! \param [out] digest
//!        Pointer to an intermediate digest. Must be 32-bit aligned.
//!
//! \return Returns a status code.
//! - \ref SHA2_OLD_DIGEST_NOT_READ
//! - \ref SHA2_SUCCESS
//
//*****************************************************************************
__STATIC_INLINE uint32_t SHA2GetDigest(uint32_t *digest, uint8_t digestLength)
{
    // Check the arguments.
    ASSERT(!(digest == NULL) && !((uint32_t)digest & 0x03));
    ASSERT((digestLength == SHA2_SHA224_DIGEST_LENGTH_BYTES) ||
           (digestLength == SHA2_SHA256_DIGEST_LENGTH_BYTES) ||
           (digestLength == SHA2_SHA384_DIGEST_LENGTH_BYTES) ||
           (digestLength == SHA2_SHA512_DIGEST_LENGTH_BYTES));

    if (HWREG(CRYPTO_BASE + CRYPTO_O_HASHIOBUFCTRL) & CRYPTO_HASHIOBUFCTRL_OUTPUT_FULL_M) {
        return SHA2_OLD_DIGEST_NOT_READ;
    }
    else {
         // Read digest
        uint32_t i = 0;
        for (i = 0; i < (digestLength / sizeof(uint32_t)); i++) {
            digest[i] = HWREG(CRYPTO_BASE + CRYPTO_O_HASHDIGESTA + (i * sizeof(uint32_t)));
        }
        return SHA2_SUCCESS;
    }
}

//*****************************************************************************
//
//! \brief Confirm digest was read.
//
//*****************************************************************************
__STATIC_INLINE void SHA2ClearDigestAvailableFlag(void)
{
    HWREG(CRYPTO_BASE + CRYPTO_O_HASHIOBUFCTRL) = HWREG(CRYPTO_BASE + CRYPTO_O_HASHIOBUFCTRL);
}

//*****************************************************************************
//
//! \brief Enable individual crypto interrupt sources.
//!
//! This function enables the indicated crypto interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt.
//! Disabled sources have no effect on the processor.
//!
//! \param intFlags is the bitwise OR of the interrupt sources to be enabled.
//! - \ref SHA2_DMA_IN_DONE
//! - \ref SHA2_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void SHA2IntEnable(uint32_t intFlags)
{
    // Check the arguments.
    ASSERT((intFlags & SHA2_DMA_IN_DONE) ||
           (intFlags & SHA2_RESULT_RDY));

    // Using level interrupt.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL_M;

    // Enable the specified interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) |= intFlags;
}

//*****************************************************************************
//
//! \brief Disable individual crypto interrupt sources.
//!
//! This function disables the indicated crypto interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt.
//! Disabled sources have no effect on the processor.
//!
//! \param intFlags is the bitwise OR of the interrupt sources to be enabled.
//! - \ref SHA2_DMA_IN_DONE
//! - \ref SHA2_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void SHA2IntDisable(uint32_t intFlags)
{
    // Check the arguments.
    ASSERT((intFlags & SHA2_DMA_IN_DONE) ||
           (intFlags & SHA2_RESULT_RDY));

    // Disable the specified interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) &= ~intFlags;
}

//*****************************************************************************
//
//! \brief Get the current masked interrupt status.
//!
//! This function returns the masked interrupt status of the crypto module.
//!
//! \return Returns the status of the masked lines when enabled:
//! - \ref SHA2_DMA_IN_DONE
//! - \ref SHA2_RESULT_RDY
//
//*****************************************************************************
__STATIC_INLINE uint32_t SHA2IntStatusMasked(void)
{
    uint32_t mask;

    // Return the masked interrupt status
    mask = HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN);
    return(mask & HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT));
}

//*****************************************************************************
//
//! \brief Get the current raw interrupt status.
//!
//! This function returns the raw interrupt status of the crypto module.
//! It returns both the status of the lines routed to the NVIC as well as the
//! error flags.
//!
//! \return Returns the raw interrupt status:
//! - \ref SHA2_DMA_IN_DONE
//! - \ref SHA2_RESULT_RDY
//! - \ref SHA2_DMA_BUS_ERR
//
//*****************************************************************************
__STATIC_INLINE uint32_t SHA2IntStatusRaw(void)
{
    // Return either the raw interrupt status
    return(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT));
}

//*****************************************************************************
//
//! \brief Clear crypto interrupt sources.
//!
//! The specified crypto interrupt sources are cleared, so that they no longer
//! assert. This function must be called in the interrupt handler to keep the
//! interrupt from being recognized again immediately upon exit.
//!
//! \note Due to write buffers and synchronizers in the system it may take several
//! clock cycles from a register write clearing an event in the module until the
//! event is actually cleared in the NVIC of the system CPU. It is recommended to
//! clear the event source early in the interrupt service routine (ISR) to allow
//! the event clear to propagate to the NVIC before returning from the ISR.
//!
//! \param intFlags is a bit mask of the interrupt sources to be cleared.
//! - \ref SHA2_DMA_IN_DONE
//! - \ref SHA2_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void SHA2IntClear(uint32_t intFlags)
{
    // Check the arguments.
    ASSERT((intFlags & SHA2_DMA_IN_DONE) ||
           (intFlags & SHA2_RESULT_RDY));

    // Clear the requested interrupt sources,
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = intFlags;
}

//*****************************************************************************
//
//! \brief Register an interrupt handler for a crypto interrupt in the dynamic interrupt table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific crypto interrupts must be enabled via \ref SHA2IntEnable(). It is the
//! interrupt handler's responsibility to clear the interrupt source.
//!
//! \param handlerFxn is a pointer to the function to be called when the
//! crypto interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void SHA2IntRegister(void (*handlerFxn)(void))
{
    // Register the interrupt handler.
    IntRegister(INT_CRYPTO_RESULT_AVAIL_IRQ, handlerFxn);

    // Enable the crypto interrupt.
    IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);
}

//*****************************************************************************
//
//! \brief Unregister an interrupt handler for a crypto interrupt in the dynamic interrupt table.
//!
//! This function does the actual unregistering of the interrupt handler. It
//! clears the handler called when a crypto interrupt occurs. This
//! function also masks off the interrupt in the interrupt controller so that
//! the interrupt handler no longer is called.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void SHA2IntUnregister(void)
{
    // Disable the interrupt.
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);

    // Unregister the interrupt handler.
    IntUnregister(INT_CRYPTO_RESULT_AVAIL_IRQ);
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_SHA2StartDMAOperation
        #undef  SHA2StartDMAOperation
        #define SHA2StartDMAOperation           ROM_SHA2StartDMAOperation
    #endif
    #ifdef ROM_SHA2WaitForIRQFlags
        #undef  SHA2WaitForIRQFlags
        #define SHA2WaitForIRQFlags             ROM_SHA2WaitForIRQFlags
    #endif
    #ifdef ROM_SHA2ComputeInitialHash
        #undef  SHA2ComputeInitialHash
        #define SHA2ComputeInitialHash          ROM_SHA2ComputeInitialHash
    #endif
    #ifdef ROM_SHA2ComputeIntermediateHash
        #undef  SHA2ComputeIntermediateHash
        #define SHA2ComputeIntermediateHash     ROM_SHA2ComputeIntermediateHash
    #endif
    #ifdef ROM_SHA2ComputeFinalHash
        #undef  SHA2ComputeFinalHash
        #define SHA2ComputeFinalHash            ROM_SHA2ComputeFinalHash
    #endif
    #ifdef ROM_SHA2ComputeHash
        #undef  SHA2ComputeHash
        #define SHA2ComputeHash                 ROM_SHA2ComputeHash
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

#endif  // __SHA2_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
