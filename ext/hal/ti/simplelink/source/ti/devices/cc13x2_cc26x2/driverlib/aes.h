/******************************************************************************
*  Filename:       aes.h
*  Revised:        2019-01-25 14:45:16 +0100 (Fri, 25 Jan 2019)
*  Revision:       54287
*
*  Description:    AES header file.
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
//! \addtogroup aes_api
//! @{
//
//*****************************************************************************

#ifndef __AES_H__
#define __AES_H__

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
#include <string.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_crypto.h"
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
    #define AESStartDMAOperation            NOROM_AESStartDMAOperation
    #define AESSetInitializationVector      NOROM_AESSetInitializationVector
    #define AESWriteCCMInitializationVector NOROM_AESWriteCCMInitializationVector
    #define AESReadTag                      NOROM_AESReadTag
    #define AESVerifyTag                    NOROM_AESVerifyTag
    #define AESWriteToKeyStore              NOROM_AESWriteToKeyStore
    #define AESReadFromKeyStore             NOROM_AESReadFromKeyStore
    #define AESWaitForIRQFlags              NOROM_AESWaitForIRQFlags
    #define AESConfigureCCMCtrl             NOROM_AESConfigureCCMCtrl
#endif


//*****************************************************************************
//
// Values that can be passed to AESIntEnable, AESIntDisable, and AESIntClear
// as the intFlags parameter, and returned from AESIntStatus.
// Only AES_DMA_IN_DONE and AES_RESULT_RDY are routed to the NVIC. Check each
// function to see if it supports other interrupt status flags.
//
//*****************************************************************************
#define AES_DMA_IN_DONE                 CRYPTO_IRQEN_DMA_IN_DONE_M
#define AES_RESULT_RDY                  CRYPTO_IRQEN_RESULT_AVAIL_M
#define AES_DMA_BUS_ERR                 CRYPTO_IRQCLR_DMA_BUS_ERR_M
#define AES_KEY_ST_WR_ERR               CRYPTO_IRQCLR_KEY_ST_WR_ERR_M
#define AES_KEY_ST_RD_ERR               CRYPTO_IRQCLR_KEY_ST_RD_ERR_M


//*****************************************************************************
//
//  General constants
//
//*****************************************************************************

// AES module return codes
#define AES_SUCCESS                     0
#define AES_KEYSTORE_ERROR              1
#define AES_KEYSTORE_AREA_INVALID       2
#define AES_DMA_BUSY                    3
#define AES_DMA_ERROR                   4
#define AES_TAG_NOT_READY               5
#define AES_TAG_VERIFICATION_FAILED     6

// Key store module defines
#define AES_IV_LENGTH_BYTES             16
#define AES_TAG_LENGTH_BYTES            16
#define AES_128_KEY_LENGTH_BYTES        (128 / 8)
#define AES_192_KEY_LENGTH_BYTES        (192 / 8)
#define AES_256_KEY_LENGTH_BYTES        (256 / 8)

#define AES_BLOCK_SIZE                  16

// DMA status codes
#define AES_DMA_CHANNEL0_ACTIVE         CRYPTO_DMASTAT_CH0_ACT_M
#define AES_DMA_CHANNEL1_ACTIVE         CRYPTO_DMASTAT_CH1_ACT_M
#define AES_DMA_PORT_ERROR              CRYPTO_DMASTAT_PORT_ERR_M

// Crypto module operation types
#define AES_ALGSEL_AES                  CRYPTO_ALGSEL_AES_M
#define AES_ALGSEL_KEY_STORE            CRYPTO_ALGSEL_KEY_STORE_M
#define AES_ALGSEL_TAG                  CRYPTO_ALGSEL_TAG_M


//*****************************************************************************
//
// For 128-bit keys, all 8 key area locations from 0 to 7 are valid.
// A 256-bit key requires two consecutive Key Area locations. The base key area
// may be odd. Do not attempt to write a 256-bit key to AES_KEY_AREA_7.
//
//*****************************************************************************
#define AES_KEY_AREA_0          0
#define AES_KEY_AREA_1          1
#define AES_KEY_AREA_2          2
#define AES_KEY_AREA_3          3
#define AES_KEY_AREA_4          4
#define AES_KEY_AREA_5          5
#define AES_KEY_AREA_6          6
#define AES_KEY_AREA_7          7

//*****************************************************************************
//
// Defines for the AES-CTR mode counter width
//
//*****************************************************************************
#define AES_CTR_WIDTH_32        0x0
#define AES_CTR_WIDTH_64        0x1
#define AES_CTR_WIDTH_96        0x2
#define AES_CTR_WIDTH_128       0x3

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
//! \param [in] channel0Addr A pointer to the address channel 0 shall use.
//!
//! \param [in] channel0Length Length of the data in bytes to be read from or
//!                            written to at channel0Addr. Set to 0 to not set up
//!                            this channel. Permitted ranges are mode dependent
//!                            and displayed below.
//!                            - ECB:        [16]
//!                            - CBC:        [1, sizeof(RAM)]
//!                            - CBC-MAC:    [1, sizeof(RAM)]
//!                            - CCM:        [1, sizeof(RAM)]
//!
//! \param [out] channel1Addr A pointer to the address channel 1 shall use.
//!
//! \param [in] channel1Length Length of the data in bytes to be read from or
//!                            written to at channel1Addr. Set to 0 to not set up
//!                            this channel.Permitted ranges are mode dependent
//!                            and displayed below.
//!                            - ECB:        [16]
//!                            - CBC:        [1, sizeof(RAM)]
//!                            - CBC-MAC:    [1, sizeof(RAM)]
//!                            - CCM:        [1, sizeof(RAM)]
//!
//! \return None
//
//*****************************************************************************
extern void AESStartDMAOperation(const uint8_t *channel0Addr, uint32_t channel0Length,  uint8_t *channel1Addr, uint32_t channel1Length);

//*****************************************************************************
//
//! \brief Write the initialization vector (IV) to the crypto module.
//!
//!         Depending on the mode of operation, the tag must be constructed
//!         differently:
//!             - CBC:      No special care must be taken. Any 128-bit IV
//!                         (initialization vector) will suffice.
//!             - CBC-MAC:  IV's must be all 0's.
//!             - CCM:      Only 12 and 13 byte IV's are permitted. See code
//!                         below for formatting.
//! \code
//!                         uint8_t initVectorLength = 12;  // Could also be 13
//!
//!                         union {
//!                             uint32_t word[4];
//!                             uint8_t byte[16];
//!                         } initVector;
//!
//!                         uint8_t initVectorUnformatted[initVectorLength];
//!
//!                         // This is the same field length value that is written to the ctrl register
//!                         initVector.byte[0] = L - 1;
//!
//!                         memcpy(&initVector.byte[1], initVectorUnformatted, initVectorLength);
//!
//!                         // Fill the remaining bytes with zeros
//!                         for (initVectorLength++; initVectorLength < sizeof(initVector.byte); initVectorLength++) {
//!                             initVector.byte[initVectorLength] = 0;
//!                         }
//! \endcode
//!
//! \param [in] initializationVector Pointer to an array with four 32-bit elements
//!                                  to be used as initialization vector.
//!                                  Elements of array must be word aligned in memory.
//!
//! \return None
//
//*****************************************************************************
extern void AESSetInitializationVector(const uint32_t *initializationVector);

//*****************************************************************************
//
//! \brief Generate and load the initialization vector for a CCM operation.
//!
//!
//! \param [in] nonce Pointer to a nonce of length \c nonceLength.
//!
//! \param [in] nonceLength Number of bytes to copy from \c nonce when creating
//!                         the CCM IV. The L-value is also derived from it.
//!
//! \return None
//
//*****************************************************************************
extern void AESWriteCCMInitializationVector(const uint8_t *nonce, uint32_t nonceLength);

//*****************************************************************************
//
//! \brief Read the tag out from the crypto module.
//!
//! This function copies the \c tagLength bytes from the tag calculated by the
//! crypto module in CCM, GCM, or CBC-MAC mode to \c tag.
//!
//! \param [out] tag Pointer to an array of \c tagLength bytes.
//!
//! \param [in] tagLength Number of bytes to copy to \c tag.
//!
//! \return Returns a status code depending on the result of the transfer.
//! - \ref AES_TAG_NOT_READY if the tag is not ready yet
//! - \ref AES_SUCCESS otherwise
//
//*****************************************************************************
extern uint32_t AESReadTag(uint8_t *tag, uint32_t tagLength);

//*****************************************************************************
//
//! \brief Verifies the provided \c tag against calculated one
//!
//! This function compares the provided tag against the tag calculated by the
//! crypto module during the last CCM, GCM, or CBC-MAC
//!
//! This function copies the \c tagLength bytes from the tag calculated by the
//! crypto module in CCM, GCM, or CBC-MAC mode to \c tag.
//!
//! \param [in] tag Pointer to an array of \c tagLength bytes.
//!
//! \param [in] tagLength Number of bytes to compare.
//!
//! \return Returns a status code depending on the result of the transfer.
//! - \ref AES_TAG_VERIFICATION_FAILED if the verification failed
//! - \ref AES_SUCCESS otherwise
//
//*****************************************************************************
extern uint32_t AESVerifyTag(const uint8_t *tag, uint32_t tagLength);

//*****************************************************************************
//
//! \brief Transfer a key from main memory to a key area within the key store.
//!
//!     The crypto DMA transfers the key and function does not return until
//!     the operation completes.
//!     The keyStore can only contain valid keys of one \c aesKeyLength at
//!     any one point in time. The keyStore cannot contain both 128-bit and
//!     256-bit keys simultaneously. When a key of a different \c aesKeyLength
//!     from the previous \c aesKeyLength is loaded, all previous keys are
//!     invalidated.
//!
//! \param [in] aesKey Pointer to key. Does not need to be word-aligned.
//!
//! \param [in] aesKeyLength The key size in bytes. Currently, 128-bit, 192-bit,
//!                          and 256-bit keys are supported.
//! - \ref AES_128_KEY_LENGTH_BYTES
//! - \ref AES_192_KEY_LENGTH_BYTES
//! - \ref AES_256_KEY_LENGTH_BYTES
//!
//! \param [in] keyStoreArea The key store area to transfer the key to.
//!                          When using 128-bit keys, only the specified key store
//!                          area will be occupied.
//!                          When using 256-bit or 192-bit keys, two consecutive key areas
//!                          are used to store the key.
//! - \ref AES_KEY_AREA_0
//! - \ref AES_KEY_AREA_1
//! - \ref AES_KEY_AREA_2
//! - \ref AES_KEY_AREA_3
//! - \ref AES_KEY_AREA_4
//! - \ref AES_KEY_AREA_5
//! - \ref AES_KEY_AREA_6
//! - \ref AES_KEY_AREA_7
//!
//!     When using 256-bit or 192-bit keys, the 8 \c keyStoreArea's are
//!     split into four sets of two. Selecting any \c keyStoreArea automatically
//!     occupies the second \c keyStoreArea of the tuples below:
//!
//! - (\ref AES_KEY_AREA_0, \ref AES_KEY_AREA_1)
//! - (\ref AES_KEY_AREA_2, \ref AES_KEY_AREA_3)
//! - (\ref AES_KEY_AREA_4, \ref AES_KEY_AREA_5)
//! - (\ref AES_KEY_AREA_6, \ref AES_KEY_AREA_7)
//!
//!     For example: if \c keyStoreArea == \ref AES_KEY_AREA_2,
//!     both \ref AES_KEY_AREA_2 and \ref AES_KEY_AREA_3 are occupied.
//!     If \c keyStoreArea == \ref AES_KEY_AREA_5, both \ref AES_KEY_AREA_4 and \ref AES_KEY_AREA_5 are occupied.
//!
//! \return Returns a status code depending on the result of the transfer.
//!         If there was an error in the read process itself, an error is
//!         returned.
//!         Otherwise, a success code is returned.
//! - \ref AES_KEYSTORE_ERROR
//! - \ref AES_SUCCESS
//!
//! \sa AESReadFromKeyStore
//
//*****************************************************************************
extern uint32_t AESWriteToKeyStore(const uint8_t *aesKey, uint32_t aesKeyLength, uint32_t keyStoreArea);

//*****************************************************************************
//
//! \brief Transfer a key from key store area to the internal buffers within
//!        the hardware module.
//!
//!     The function polls until the transfer is complete.
//!
//! \param [in] keyStoreArea The key store area to transfer the key from. When using
//!                          256-bit keys, either of the occupied key areas may be
//!                          specified to load the key. There is no need to specify
//!                          the length of the key here as the key store keeps track
//!                          of how long a key associated with any valid key area is
//!                          and where is starts.
//! - \ref AES_KEY_AREA_0
//! - \ref AES_KEY_AREA_1
//! - \ref AES_KEY_AREA_2
//! - \ref AES_KEY_AREA_3
//! - \ref AES_KEY_AREA_4
//! - \ref AES_KEY_AREA_5
//! - \ref AES_KEY_AREA_6
//! - \ref AES_KEY_AREA_7
//!
//! \return Returns a status code depending on the result of the transfer.
//!         When specifying a \c keyStoreArea value without a valid key in it an
//!         error is returned.
//!         If there was an error in the read process itself, an error is
//!         returned.
//!         Otherwise, a success code is returned.
//! - \ref AES_KEYSTORE_AREA_INVALID
//! - \ref AES_KEYSTORE_ERROR
//! - \ref AES_SUCCESS
//!
//! \sa AESWriteToKeyStore
//
//*****************************************************************************
extern uint32_t AESReadFromKeyStore(uint32_t keyStoreArea);


//*****************************************************************************
//
//! \brief Poll the interrupt status register and clear when done.
//!
//!        This function polls until one of the bits in the \c irqFlags is
//!        asserted. Only \ref AES_DMA_IN_DONE and \ref AES_RESULT_RDY can actually
//!        trigger the interrupt line. That means that one of those should
//!        always be included in \c irqFlags and will always be returned together
//!        with any error codes.
//!
//! \param [in] irqFlags IRQ flags to poll and mask that the status register will be
//!                      masked with. May consist of any bitwise OR of the flags
//!                      below that includes at least one of
//!                      \ref AES_DMA_IN_DONE or \ref AES_RESULT_RDY :
//! - \ref AES_DMA_IN_DONE
//! - \ref AES_RESULT_RDY
//! - \ref AES_DMA_BUS_ERR
//! - \ref AES_KEY_ST_WR_ERR
//! - \ref AES_KEY_ST_RD_ERR
//!
//! \return Returns the IRQ status register masked with \c irqFlags. May be any
//!         bitwise OR of the following masks:
//! - \ref AES_DMA_IN_DONE
//! - \ref AES_RESULT_RDY
//! - \ref AES_DMA_BUS_ERR
//! - \ref AES_KEY_ST_WR_ERR
//! - \ref AES_KEY_ST_RD_ERR
//
//*****************************************************************************
extern uint32_t AESWaitForIRQFlags(uint32_t irqFlags);

//*****************************************************************************
//
//! \brief Configure AES engine for CCM operation.
//!
//! \param [in] nonceLength Length of the nonce. Must be <= 14.
//!
//! \param [in] macLength Length of the MAC. Must be <= 16.
//!
//! \param [in] encrypt Whether to set up an encrypt or decrypt operation.
//! - true: encrypt
//! - false: decrypt
//!
//! \return None
//
//*****************************************************************************
extern void AESConfigureCCMCtrl(uint32_t nonceLength, uint32_t macLength, bool encrypt);

//*****************************************************************************
//
//! \brief Invalidate a key in the key store
//!
//! \param [in] keyStoreArea is the entry in the key store to invalidate. This
//!                          permanently deletes the key from the key store.
//! - \ref AES_KEY_AREA_0
//! - \ref AES_KEY_AREA_1
//! - \ref AES_KEY_AREA_2
//! - \ref AES_KEY_AREA_3
//! - \ref AES_KEY_AREA_4
//! - \ref AES_KEY_AREA_5
//! - \ref AES_KEY_AREA_6
//! - \ref AES_KEY_AREA_7
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void AESInvalidateKey(uint32_t keyStoreArea)
{
    ASSERT((keyStoreArea == AES_KEY_AREA_0) ||
           (keyStoreArea == AES_KEY_AREA_1) ||
           (keyStoreArea == AES_KEY_AREA_2) ||
           (keyStoreArea == AES_KEY_AREA_3) ||
           (keyStoreArea == AES_KEY_AREA_4) ||
           (keyStoreArea == AES_KEY_AREA_5) ||
           (keyStoreArea == AES_KEY_AREA_6) ||
           (keyStoreArea == AES_KEY_AREA_7));

    // Clear any previously written key at the key location
    HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) = (0x00000001 << keyStoreArea);
}

//*****************************************************************************
//
//! \brief Select type of operation
//!
//! \param [in] algorithm Flags that specify which type of operation the crypto
//!                       module shall perform. The flags are mutually exclusive.
//! - 0 : Reset the module
//! - \ref AES_ALGSEL_AES
//! - \ref AES_ALGSEL_TAG
//! - \ref AES_ALGSEL_KEY_STORE
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void AESSelectAlgorithm(uint32_t algorithm)
{
    ASSERT((algorithm == AES_ALGSEL_AES) ||
           (algorithm == AES_ALGSEL_AES | AES_ALGSEL_TAG) ||
           (algorithm == AES_ALGSEL_KEY_STORE));

    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = algorithm;
}

//*****************************************************************************
//
//! \brief Set up the next crypto module operation.
//!
//! The function uses a bitwise OR of the fields within the CRYPTO_O_AESCTL
//! register. The relevant field names have the format:
//! - CRYPTO_AESCTL_[field name]
//!
//! \param [in] ctrlMask Specifies which register fields shall be set.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void AESSetCtrl(uint32_t ctrlMask)
{
    HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = ctrlMask;
}

//*****************************************************************************
//
//! \brief Specify length of the crypto operation.
//!
//!        Despite specifying it here, the crypto DMA must still be
//!        set up with the correct data length.
//!
//! \param [in] length Data length in bytes. If this
//!                    value is set to 0, only authentication of the AAD is
//!                    performed in CCM-mode and AESWriteAuthLength() must be set to
//!                    >0.
//!                    Range depends on the mode:
//!                      - ECB:        [16]
//!                      - CBC:        [1, sizeof(RAM)]
//!                      - CBC-MAC:    [1, sizeof(RAM)]
//!                      - CCM:        [0, sizeof(RAM)]
//!
//! \return None
//!
//! \sa AESWriteAuthLength
//
//*****************************************************************************
__STATIC_INLINE void AESSetDataLength(uint32_t length)
{
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN0) = length;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN1) = 0;
}

//*****************************************************************************
//
//! \brief Specify the length of the additional authentication data (AAD).
//!
//!        Despite specifying it here, the crypto DMA must still be set up with
//!        the correct AAD length.
//!
//! \param [in] length Specifies how long the AAD is in a CCM operation. In CCM mode,
//!                    set this to 0 if no AAD is required. If set to 0,
//!                    AESWriteDataLength() must be set to >0.
//!                    Range depends on the mode:
//!                      - ECB:        Do not call.
//!                      - CBC:        [0]
//!                      - CBC-MAC:    [0]
//!                      - CCM:        [0, sizeof(RAM)]
//!
//! \return None
//!
//! \sa AESWriteDataLength
//
//*****************************************************************************
__STATIC_INLINE void AESSetAuthLength(uint32_t length)
{
    HWREG(CRYPTO_BASE + CRYPTO_O_AESAUTHLEN) = length;
}

//*****************************************************************************
//
//! \brief Reset the accelerator and cancel ongoing operations
//!
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void AESReset(void)
{
    HWREG(CRYPTO_BASE + CRYPTO_O_SWRESET) = 0x00000001;
}

//*****************************************************************************
//
//! \brief Enable individual crypto interrupt sources.
//!
//! This function enables the indicated crypto interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt.
//! Disabled sources have no effect on the processor.
//!
//! \param [in] intFlags is the bitwise OR of the interrupt sources to be enabled.
//! - \ref AES_DMA_IN_DONE
//! - \ref AES_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void AESIntEnable(uint32_t intFlags)
{
    // Check the arguments.
    ASSERT((intFlags & AES_DMA_IN_DONE) ||
           (intFlags & AES_RESULT_RDY));

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
//! \param [in] intFlags is the bitwise OR of the interrupt sources to be enabled.
//! - \ref AES_DMA_IN_DONE
//! - \ref AES_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void AESIntDisable(uint32_t intFlags)
{
    // Check the arguments.
    ASSERT((intFlags & AES_DMA_IN_DONE) ||
           (intFlags & AES_RESULT_RDY));

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
//! - \ref AES_DMA_IN_DONE
//! - \ref AES_RESULT_RDY
//
//*****************************************************************************
__STATIC_INLINE uint32_t AESIntStatusMasked(void)
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
//! - \ref AES_DMA_IN_DONE
//! - \ref AES_RESULT_RDY
//! - \ref AES_DMA_BUS_ERR
//! - \ref AES_KEY_ST_WR_ERR
//! - \ref AES_KEY_ST_RD_ERR
//
//*****************************************************************************
__STATIC_INLINE uint32_t AESIntStatusRaw(void)
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
//! \param [in] intFlags is a bit mask of the interrupt sources to be cleared.
//! - \ref AES_DMA_IN_DONE
//! - \ref AES_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void AESIntClear(uint32_t intFlags)
{
    // Check the arguments.
    ASSERT((intFlags & AES_DMA_IN_DONE) ||
           (intFlags & AES_RESULT_RDY));

    // Clear the requested interrupt sources,
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = intFlags;
}

//*****************************************************************************
//
//! \brief Register an interrupt handler for a crypto interrupt.
//!
//! This function does the actual registering of the interrupt handler.  This
//! function enables the global interrupt in the interrupt controller; specific
//! crypto interrupts must be enabled via \ref AESIntEnable(). It is the interrupt
//! handler's responsibility to clear the interrupt source.
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
__STATIC_INLINE void AESIntRegister(void (*handlerFxn)(void))
{
    // Register the interrupt handler.
    IntRegister(INT_CRYPTO_RESULT_AVAIL_IRQ, handlerFxn);

    // Enable the crypto interrupt.
    IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);
}

//*****************************************************************************
//
//! \brief Unregister an interrupt handler for a crypto interrupt.
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
__STATIC_INLINE void AESIntUnregister(void)
{
    //
    // Disable the interrupt.
    //
    IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);

    //
    // Unregister the interrupt handler.
    //
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
    #ifdef ROM_AESStartDMAOperation
        #undef  AESStartDMAOperation
        #define AESStartDMAOperation            ROM_AESStartDMAOperation
    #endif
    #ifdef ROM_AESSetInitializationVector
        #undef  AESSetInitializationVector
        #define AESSetInitializationVector      ROM_AESSetInitializationVector
    #endif
    #ifdef ROM_AESWriteCCMInitializationVector
        #undef  AESWriteCCMInitializationVector
        #define AESWriteCCMInitializationVector ROM_AESWriteCCMInitializationVector
    #endif
    #ifdef ROM_AESReadTag
        #undef  AESReadTag
        #define AESReadTag                      ROM_AESReadTag
    #endif
    #ifdef ROM_AESVerifyTag
        #undef  AESVerifyTag
        #define AESVerifyTag                    ROM_AESVerifyTag
    #endif
    #ifdef ROM_AESWriteToKeyStore
        #undef  AESWriteToKeyStore
        #define AESWriteToKeyStore              ROM_AESWriteToKeyStore
    #endif
    #ifdef ROM_AESReadFromKeyStore
        #undef  AESReadFromKeyStore
        #define AESReadFromKeyStore             ROM_AESReadFromKeyStore
    #endif
    #ifdef ROM_AESWaitForIRQFlags
        #undef  AESWaitForIRQFlags
        #define AESWaitForIRQFlags              ROM_AESWaitForIRQFlags
    #endif
    #ifdef ROM_AESConfigureCCMCtrl
        #undef  AESConfigureCCMCtrl
        #define AESConfigureCCMCtrl             ROM_AESConfigureCCMCtrl
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

#endif  // __AES_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
