/******************************************************************************
*  Filename:       crypto.h
*  Revised:        2018-01-12 18:46:31 +0100 (Fri, 12 Jan 2018)
*  Revision:       51161
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
//! \addtogroup crypto_api
//! @{
//
//*****************************************************************************

#ifndef __CRYPTO_H__
#define __CRYPTO_H__

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
    #define CRYPTOAesLoadKey                NOROM_CRYPTOAesLoadKey
    #define CRYPTOAesCbc                    NOROM_CRYPTOAesCbc
    #define CRYPTOAesCbcStatus              NOROM_CRYPTOAesCbcStatus
    #define CRYPTOAesEcb                    NOROM_CRYPTOAesEcb
    #define CRYPTOAesEcbStatus              NOROM_CRYPTOAesEcbStatus
    #define CRYPTOCcmAuthEncrypt            NOROM_CRYPTOCcmAuthEncrypt
    #define CRYPTOCcmAuthEncryptStatus      NOROM_CRYPTOCcmAuthEncryptStatus
    #define CRYPTOCcmAuthEncryptResultGet   NOROM_CRYPTOCcmAuthEncryptResultGet
    #define CRYPTOCcmInvAuthDecrypt         NOROM_CRYPTOCcmInvAuthDecrypt
    #define CRYPTOCcmInvAuthDecryptStatus   NOROM_CRYPTOCcmInvAuthDecryptStatus
    #define CRYPTOCcmInvAuthDecryptResultGet NOROM_CRYPTOCcmInvAuthDecryptResultGet
    #define CRYPTODmaEnable                 NOROM_CRYPTODmaEnable
    #define CRYPTODmaDisable                NOROM_CRYPTODmaDisable
#endif

//*****************************************************************************
//
// Length of AES Electronic Code Book (ECB) block in bytes
//
//*****************************************************************************
#define AES_ECB_LENGTH  16

//*****************************************************************************
//
// Values that can be passed to CryptoIntEnable, CryptoIntDisable, and CryptoIntClear
// as the ui32IntFlags parameter, and returned from CryptoIntStatus.
//
//*****************************************************************************
#define CRYPTO_DMA_IN_DONE      0x00000002  // DMA done interrupt mask
#define CRYPTO_RESULT_RDY       0x00000001  // Result ready interrupt mask
#define CRYPTO_DMA_BUS_ERR      0x80000000  // DMA Bus error
#define CRYPTO_KEY_ST_WR_ERR    0x40000000  // Key Store Write failed
#define CRYPTO_KEY_ST_RD_ERR    0x20000000  // Key Store Read failed

#define CRYPTO_IRQTYPE_LEVEL    0x00000001  // Crypto Level interrupt enabled
#define CRYPTO_IRQTYPE_PULSE    0x00000000  // Crypto pulse interrupt enabled

#define CRYPTO_DMA_CHAN0        0x00000001  // Crypto DMA Channel 0
#define CRYPTO_DMA_CHAN1        0x00000002  // Crypto DMA Channel 1

#define CRYPTO_AES128_ENCRYPT   0x0000000C  //
#define CRYPTO_AES128_DECRYPT   0x00000008  //

#define CRYPTO_DMA_READY        0x00000000  // DMA ready
#define CRYPTO_DMA_BSY          0x00000003  // DMA busy
#define CRYPTO_DMA_BUS_ERROR    0x00020000  // DMA encountered bus error

//*****************************************************************************
//
//  General constants
//
//*****************************************************************************

// AES module return codes
#define AES_SUCCESS                      0
#define AES_KEYSTORE_READ_ERROR          1
#define AES_KEYSTORE_WRITE_ERROR         2
#define AES_DMA_BUS_ERROR                3
#define CCM_AUTHENTICATION_FAILED        4
#define AES_ECB_TEST_ERROR               8
#define AES_NULL_ERROR                   9
#define AES_CCM_TEST_ERROR              10
#define AES_DMA_BSY                     11

// Key store module defines
#define STATE_BLENGTH                   16  // Number of bytes in State
#define KEY_BLENGTH                     16  // Number of bytes in Key
#define KEY_EXP_LENGTH                 176  // Nb * (Nr+1) * 4

#define KEY_STORE_SIZE_128      0x00000001
#define KEY_STORE_SIZE_192      0x00000002
#define KEY_STORE_SIZE_256      0x00000003
#define KEY_STORE_SIZE_BITS     0x00000003

//*****************************************************************************
//
// For 128 bit key all 8 Key Area locations from 0 to 8 are valid
// However for 192 bit and 256 bit keys, only even Key Areas 0, 2, 4, 6
// are valid.
//
//*****************************************************************************
#define CRYPTO_KEY_AREA_0       0
#define CRYPTO_KEY_AREA_1       1
#define CRYPTO_KEY_AREA_2       2
#define CRYPTO_KEY_AREA_3       3
#define CRYPTO_KEY_AREA_4       4
#define CRYPTO_KEY_AREA_5       5
#define CRYPTO_KEY_AREA_6       6
#define CRYPTO_KEY_AREA_7       7

//*****************************************************************************
//
// Defines for the current AES operation
//
//*****************************************************************************
#define CRYPTO_AES_NONE         0
#define CRYPTO_AES_KEYL0AD      1
#define CRYPTO_AES_ECB          2
#define CRYPTO_AES_CCM          3
#define CRYPTO_AES_RNG          4
#define CRYPTO_AES_CBC          5

//*****************************************************************************
//
// Defines for the AES-CTR mode counter width
//
//*****************************************************************************
#define CRYPTO_AES_CTR_32       0x0
#define CRYPTO_AES_CTR_64       0x1
#define CRYPTO_AES_CTR_96       0x2
#define CRYPTO_AES_CTR_128      0x3

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Write the key into the Key Ram.
//!
//! The \c ui32KeyLocation parameter is an enumerated type which specifies
//! the Key Ram location in which the key is stored.
//!
//! The pointer \c pui8AesKey has the address where the Key is stored.
//!
//! \param pui32AesKey is a pointer to an AES Key.
//! \param ui32KeyLocation is the location of the key in Key RAM.
//! This parameter can have any of the following values:
//! - \ref CRYPTO_KEY_AREA_0
//! - \ref CRYPTO_KEY_AREA_1
//! - \ref CRYPTO_KEY_AREA_2
//! - \ref CRYPTO_KEY_AREA_3
//! - \ref CRYPTO_KEY_AREA_4
//! - \ref CRYPTO_KEY_AREA_5
//! - \ref CRYPTO_KEY_AREA_6
//! - \ref CRYPTO_KEY_AREA_7
//!
//! \return Returns status of the function:
//! - \ref AES_SUCCESS
//! - \ref AES_KEYSTORE_READ_ERROR
//
//*****************************************************************************
extern uint32_t CRYPTOAesLoadKey(uint32_t *pui32AesKey,
                                 uint32_t ui32KeyLocation);

//*****************************************************************************
//
//! \brief Start an AES-CBC operation (encryption or decryption).
//!
//! The function starts an AES CBC mode encrypt or decrypt operation.
//! End operation can be detected by enabling interrupt or by polling
//! CRYPTOAesCbcStatus(). Result of operation is returned by CRYPTOAesCbcStatus().
//!
//! \param pui32MsgIn is a pointer to the input data.
//! \param pui32MsgOut is a pointer to the output data.
//! \param ui32MsgLength is the length in bytes of the input data.
//! \param pui32Nonce is a pointer to 16-byte Nonce.
//! \param ui32KeyLocation is the location of the key in Key RAM.
//! This parameter can have any of the following values:
//! - \ref CRYPTO_KEY_AREA_0
//! - \ref CRYPTO_KEY_AREA_1
//! - \ref CRYPTO_KEY_AREA_2
//! - \ref CRYPTO_KEY_AREA_3
//! - \ref CRYPTO_KEY_AREA_4
//! - \ref CRYPTO_KEY_AREA_5
//! - \ref CRYPTO_KEY_AREA_6
//! - \ref CRYPTO_KEY_AREA_7
//! \param bEncrypt is set \c true to encrypt or set \c false to decrypt.
//! \param bIntEnable is set \c true to enable Crypto interrupts or \c false to
//! disable Crypto interrupt.
//!
//! \return Returns status of the AES-CBC operation:
//! - \ref AES_SUCCESS
//! - \ref AES_KEYSTORE_READ_ERROR
//!
//! \sa \ref CRYPTOAesCbcStatus()
//
//*****************************************************************************
extern uint32_t CRYPTOAesCbc(uint32_t *pui32MsgIn, uint32_t *pui32MsgOut,
                             uint32_t ui32MsgLength, uint32_t *pui32Nonce,
                             uint32_t ui32KeyLocation, bool bEncrypt,
                             bool bIntEnable);

//*****************************************************************************
//
//! \brief Check the result of an AES CBC operation.
//!
//! This function should be called after \ref CRYPTOAesCbc() function to
//! check if the AES CBC operation was successful.
//!
//! \return Returns the status of the AES CBC operation:
//! - \ref AES_SUCCESS       : Successful.
//! - \ref AES_DMA_BUS_ERROR : Failed.
//! - \ref AES_DMA_BSY       : Operation is ongoing.
//!
//! \sa \ref CRYPTOAesCbc()
//
//*****************************************************************************
extern uint32_t CRYPTOAesCbcStatus(void);

//*****************************************************************************
//
//! \brief Start an AES-ECB operation (encryption or decryption).
//!
//! The \c ui32KeyLocation parameter is an enumerated type which specifies
//! the Key Ram location in which the key is stored.
//!
//! \param pui32MsgIn is a pointer to the input data.
//! \param pui32MsgOut is a pointer to the output data.
//! \param ui32KeyLocation is the location of the key in Key RAM.
//! This parameter can have any of the following values:
//! - \ref CRYPTO_KEY_AREA_0
//! - \ref CRYPTO_KEY_AREA_1
//! - \ref CRYPTO_KEY_AREA_2
//! - \ref CRYPTO_KEY_AREA_3
//! - \ref CRYPTO_KEY_AREA_4
//! - \ref CRYPTO_KEY_AREA_5
//! - \ref CRYPTO_KEY_AREA_6
//! - \ref CRYPTO_KEY_AREA_7
//! \param bEncrypt is set \c true to encrypt or set \c false to decrypt.
//! \param bIntEnable is set \c true to enable Crypto interrupts or \c false to
//! disable Crypto interrupt.
//!
//! \return Returns status of the AES-ECB operation:
//! - \ref AES_SUCCESS
//! - \ref AES_KEYSTORE_READ_ERROR
//!
//! \sa \ref CRYPTOAesEcbStatus()
//
//*****************************************************************************
extern uint32_t CRYPTOAesEcb(uint32_t *pui32MsgIn, uint32_t *pui32MsgOut,
                             uint32_t ui32KeyLocation, bool bEncrypt,
                             bool bIntEnable);

//*****************************************************************************
//
//! \brief Check the result of an AES ECB operation.
//!
//! This function should be called after \ref CRYPTOAesEcb() function to
//! check if the AES ECB operation was successful.
//!
//! \return Returns the status of the AES ECB operation:
//! - \ref AES_SUCCESS       : Successful.
//! - \ref AES_DMA_BUS_ERROR : Failed.
//! - \ref AES_DMA_BSY       : Operation is ongoing.
//!
//! \sa \ref CRYPTOAesEcb()
//
//*****************************************************************************
extern uint32_t CRYPTOAesEcbStatus(void);

//*****************************************************************************
//
//! \brief Finish the encryption operation by resetting the operation mode.
//!
//! This function should be called after \ref CRYPTOAesEcbStatus() has reported
//! that the operation is finished successfully.
//!
//! \return None
//!
//! \sa \ref CRYPTOAesEcbStatus()
//
//*****************************************************************************
__STATIC_INLINE void
CRYPTOAesEcbFinish(void)
{
    // Result has already been copied to the output buffer by DMA.
    // Disable master control/DMA clock and clear the operating mode.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = 0x00000000;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = 0x00000000;
}

//*****************************************************************************
//
//! \brief Finish the encryption operation by resetting the operation mode.
//!
//! This function should be called after \ref CRYPTOAesCbcStatus() has reported
//! that the operation is finished successfully.
//!
//! \return None
//!
//! \sa \ref CRYPTOAesCbcStatus()
//
//*****************************************************************************
__STATIC_INLINE void
CRYPTOAesCbcFinish(void)
{
    // Result has already been copied to the output buffer by DMA.
    // Disable master control/DMA clock and clear the operating mode.
    HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = 0x00000000;
    HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = 0x00000000;
}

//*****************************************************************************
//
//! \brief Start CCM operation.
//!
//! The \c ui32KeyLocation is an enumerated type which specifies the Key Ram
//! location in which the key is stored.
//!
//! \param bEncrypt determines whether to run encryption or not.
//! \param ui32AuthLength is the length of the authentication field -
//! 0, 2, 4, 6, 8, 10, 12, 14 or 16 octets.
//! \param pui32Nonce is a pointer to 13-byte or 12-byte Nonce (Number used once).
//! \param pui32PlainText is a pointer to the octet string input message.
//! \param ui32PlainTextLength is the length of the message.
//! \param pui32Header is the length of the header (Additional Authentication
//! Data or AAD).
//! \param ui32HeaderLength is the length of the header in octets.
//! \param ui32KeyLocation is the location in Key RAM where the key is stored.
//! This parameter can have any of the following values:
//! - \ref CRYPTO_KEY_AREA_0
//! - \ref CRYPTO_KEY_AREA_1
//! - \ref CRYPTO_KEY_AREA_2
//! - \ref CRYPTO_KEY_AREA_3
//! - \ref CRYPTO_KEY_AREA_4
//! - \ref CRYPTO_KEY_AREA_5
//! - \ref CRYPTO_KEY_AREA_6
//! - \ref CRYPTO_KEY_AREA_7
//! \param ui32FieldLength is the size of the length field (2 or 3).
//! \param bIntEnable enables interrupts.
//!
//! \return Returns the status of the CCM operation
//! - \ref AES_SUCCESS
//! - \ref AES_KEYSTORE_READ_ERROR
//! - \ref AES_DMA_BUS_ERROR
//!
//! \sa \ref CRYPTOCcmAuthEncryptStatus()
//
//*****************************************************************************
extern uint32_t CRYPTOCcmAuthEncrypt(bool bEncrypt, uint32_t ui32AuthLength,
                                     uint32_t *pui32Nonce,
                                     uint32_t *pui32PlainText,
                                     uint32_t ui32PlainTextLength,
                                     uint32_t *pui32Header,
                                     uint32_t ui32HeaderLength,
                                     uint32_t ui32KeyLocation,
                                     uint32_t ui32FieldLength,
                                     bool bIntEnable);

//*****************************************************************************
//
//! \brief Check the result of an AES CCM operation.
//!
//! This function should be called after \ref CRYPTOCcmAuthEncrypt() function to check
//! if the AES CCM operation was successful.
//!
//! \return Returns the status of the AES CCM operation:
//! - \ref AES_SUCCESS       : Successful.
//! - \ref AES_DMA_BUS_ERROR : Failed.
//! - \ref AES_DMA_BSY       : Operation is ongoing.
//!
//! \sa \ref CRYPTOCcmAuthEncrypt()
//
//*****************************************************************************
extern uint32_t CRYPTOCcmAuthEncryptStatus(void);

//*****************************************************************************
//
//! \brief Get the result of an AES CCM operation.
//!
//! This function should be called after \ref CRYPTOCcmAuthEncryptStatus().
//!
//! \param ui32TagLength is length of the Tag.
//! \param pui32CcmTag is the location of the authentication Tag.
//!
//! \return Returns \ref AES_SUCCESS if successful.
//!
//! \sa \ref CRYPTOCcmAuthEncryptStatus()
//
//*****************************************************************************
extern uint32_t CRYPTOCcmAuthEncryptResultGet(uint32_t ui32TagLength,
                                              uint32_t *pui32CcmTag);

//*****************************************************************************
//
//! \brief Start a CCM Decryption and Inverse Authentication operation.
//!
//! The \c ui32KeyLocation is an enumerated type which specifies the Key Ram
//! location in which the key is stored.
//!
//! \param bDecrypt determines whether to run decryption or not.
//! \param ui32AuthLength is the length of the authentication field -
//! 0, 2, 4, 6, 8, 10, 12, 14 or 16 octets.
//! \param pui32Nonce is a pointer to 13-byte or 12-byte Nonce (Number used once).
//! \param pui32CipherText is a pointer to the octet string encrypted message.
//! \param ui32CipherTextLength is the length of the encrypted message.
//! \param pui32Header is the length of the header (Additional Authentication
//! Data or AAD).
//! \param ui32HeaderLength is the length of the header in octets.
//! \param ui32KeyLocation is the location in Key RAM where the key is stored.
//! This parameter can have any of the following values:
//! - \ref CRYPTO_KEY_AREA_0
//! - \ref CRYPTO_KEY_AREA_1
//! - \ref CRYPTO_KEY_AREA_2
//! - \ref CRYPTO_KEY_AREA_3
//! - \ref CRYPTO_KEY_AREA_4
//! - \ref CRYPTO_KEY_AREA_5
//! - \ref CRYPTO_KEY_AREA_6
//! - \ref CRYPTO_KEY_AREA_7
//! \param ui32FieldLength is the size of the length field (2 or 3).
//! \param bIntEnable enables interrupts.
//!
//! \return Returns the status of the operation:
//! - \ref AES_SUCCESS
//! - \ref AES_KEYSTORE_READ_ERROR
//! - \ref AES_DMA_BUS_ERROR
//
//*****************************************************************************
extern uint32_t CRYPTOCcmInvAuthDecrypt(bool bDecrypt, uint32_t ui32AuthLength,
                                        uint32_t *pui32Nonce,
                                        uint32_t *pui32CipherText,
                                        uint32_t ui32CipherTextLength,
                                        uint32_t *pui32Header,
                                        uint32_t ui32HeaderLength,
                                        uint32_t ui32KeyLocation,
                                        uint32_t ui32FieldLength,
                                        bool bIntEnable);

//*****************************************************************************
//
//! \brief Checks CCM decrypt and Inverse Authentication result.
//!
//! \return Returns status of operation:
//! - \ref AES_SUCCESS       : Operation was successful.
//! - \ref AES_DMA_BSY       : Operation is busy.
//! - \ref AES_DMA_BUS_ERROR : An error is encountered.
//
//*****************************************************************************
extern uint32_t CRYPTOCcmInvAuthDecryptStatus(void);

//*****************************************************************************
//
//! \brief Get the result of the CCM operation.
//!
//! \param ui32AuthLength is the length of the authentication field -
//! 0, 2, 4, 6, 8, 10, 12, 14 or 16 octets.
//! \param pui32CipherText is a pointer to the octet string encrypted message.
//! \param ui32CipherTextLength is the length of the encrypted message.
//! \param pui32CcmTag is the location of the authentication Tag.
//!
//! \return Returns AES_SUCCESS if successful.
//
//*****************************************************************************
extern uint32_t CRYPTOCcmInvAuthDecryptResultGet(uint32_t ui32AuthLength,
                                                 uint32_t *pui32CipherText,
                                                 uint32_t ui32CipherTextLength,
                                                 uint32_t *pui32CcmTag);

//*****************************************************************************
//
//! \brief Get the current status of the Crypto DMA controller.
//!
//! This function is used to poll the Crypto DMA controller to check if it is
//! ready for a new operation or if an error has occurred.
//!
//! The \ref CRYPTO_DMA_BUS_ERROR can also be caught using the crypto event
//! handler.
//!
//! \return Returns the current status of the DMA controller:
//! - \ref CRYPTO_DMA_READY     : DMA ready for a new operation
//! - \ref CRYPTO_DMA_BSY       : DMA is busy
//! - \ref CRYPTO_DMA_BUS_ERROR : DMA Bus error
//
//*****************************************************************************
__STATIC_INLINE uint32_t
CRYPTODmaStatus(void)
{
    // Return the value of the status register.
    return (HWREG(CRYPTO_BASE + CRYPTO_O_DMASTAT));
}

//*****************************************************************************
//
//! \brief Enable Crypto DMA operation.
//!
//! The specified Crypto DMA channels are enabled.
//!
//! \param ui32Channels is a bitwise OR of the channels to enable.
//! - \ref CRYPTO_DMA_CHAN0
//! - \ref CRYPTO_DMA_CHAN1
//!
//! \return None
//
//*****************************************************************************
extern void CRYPTODmaEnable(uint32_t ui32Channels);

//*****************************************************************************
//
//! \brief Disable Crypto DMA operation.
//!
//! The specified Crypto DMA channels are disabled.
//!
//! \param ui32Channels is a bitwise OR of the channels to disable.
//! - \ref CRYPTO_DMA_CHAN0
//! - \ref CRYPTO_DMA_CHAN1
//!
//! \return None
//
//*****************************************************************************
extern void CRYPTODmaDisable(uint32_t ui32Channels);

//*****************************************************************************
//
//! \brief Enables individual Crypto interrupt sources.
//!
//! This function enables the indicated Crypto interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt.
//! Disabled sources have no effect on the processor.
//!
//! \param ui32IntFlags is the bitwise OR of the interrupt sources to be enabled.
//! - \ref CRYPTO_DMA_IN_DONE
//! - \ref CRYPTO_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
CRYPTOIntEnable(uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT((ui32IntFlags & CRYPTO_DMA_IN_DONE) |
           (ui32IntFlags & CRYPTO_RESULT_RDY));

    // Using level interrupt.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;

    // Enable the specified interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) |= ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Disables individual CRYPTO interrupt sources.
//!
//! This function disables the indicated Crypto interrupt sources. Only the
//! sources that are enabled can be reflected to the processor interrupt.
//! Disabled sources have no effect on the processor.
//!
//! \param ui32IntFlags is the bitwise OR of the interrupt sources to be enabled.
//! - \ref CRYPTO_DMA_IN_DONE
//! - \ref CRYPTO_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
CRYPTOIntDisable(uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT((ui32IntFlags & CRYPTO_DMA_IN_DONE) |
           (ui32IntFlags & CRYPTO_RESULT_RDY));

    // Disable the specified interrupts.
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) &= ~ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Gets the current interrupt status.
//!
//! This function returns the interrupt status for the specified Crypto. Either
//! the raw interrupt status or the status of interrupts that are allowed to
//! reflect to the processor can be returned.
//!
//! \param bMasked whether to use raw or masked interrupt status:
//! - \c false : Raw interrupt status is required.
//! - \c true  : Masked interrupt status is required.
//!
//! \return Returns the current interrupt status:
//! - \ref CRYPTO_DMA_IN_DONE
//! - \ref CRYPTO_RESULT_RDY
//
//*****************************************************************************
__STATIC_INLINE uint32_t
CRYPTOIntStatus(bool bMasked)
{
    uint32_t ui32Mask;

    // Return either the interrupt status or the raw interrupt status as
    // requested.
    if(bMasked)
    {
        ui32Mask = HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN);
        return(ui32Mask & HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT));
    }
    else
    {
        return(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & 0x00000003);
    }
}

//*****************************************************************************
//
//! \brief Clears Crypto interrupt sources.
//!
//! The specified Crypto interrupt sources are cleared, so that they no longer
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
//! \param ui32IntFlags is a bit mask of the interrupt sources to be cleared.
//! - \ref CRYPTO_DMA_IN_DONE
//! - \ref CRYPTO_RESULT_RDY
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
CRYPTOIntClear(uint32_t ui32IntFlags)
{
    // Check the arguments.
    ASSERT((ui32IntFlags & CRYPTO_DMA_IN_DONE) |
           (ui32IntFlags & CRYPTO_RESULT_RDY));

    // Clear the requested interrupt sources,
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = ui32IntFlags;
}

//*****************************************************************************
//
//! \brief Registers an interrupt handler for a Crypto interrupt in the dynamic interrupt table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function registers a function as the interrupt handler for a specific
//! interrupt and enables the corresponding interrupt in the interrupt controller.
//!
//! Specific UART interrupts must be enabled via \ref CRYPTOIntEnable(). It is the
//! interrupt handler's responsibility to clear the interrupt source.
//!
//! \param pfnHandler is a pointer to the function to be called when the
//! UART interrupt occurs.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void
CRYPTOIntRegister(void (*pfnHandler)(void))
{
    // Register the interrupt handler.
    IntRegister(INT_CRYPTO_RESULT_AVAIL_IRQ, pfnHandler);

    // Enable the UART interrupt.
    IntEnable(INT_CRYPTO_RESULT_AVAIL_IRQ);
}

//*****************************************************************************
//
//! \brief Unregisters an interrupt handler for a Crypto interrupt in the dynamic interrupt table.
//!
//! This function does the actual unregistering of the interrupt handler. It
//! clears the handler to be called when a Crypto interrupt occurs. This
//! function also masks off the interrupt in the interrupt controller so that
//! the interrupt handler no longer is called.
//!
//! \return None
//!
//! \sa \ref IntRegister() for important information about registering interrupt
//! handlers.
//
//*****************************************************************************
__STATIC_INLINE void
CRYPTOIntUnregister(void)
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
    #ifdef ROM_CRYPTOAesLoadKey
        #undef  CRYPTOAesLoadKey
        #define CRYPTOAesLoadKey                ROM_CRYPTOAesLoadKey
    #endif
    #ifdef ROM_CRYPTOAesCbc
        #undef  CRYPTOAesCbc
        #define CRYPTOAesCbc                    ROM_CRYPTOAesCbc
    #endif
    #ifdef ROM_CRYPTOAesCbcStatus
        #undef  CRYPTOAesCbcStatus
        #define CRYPTOAesCbcStatus              ROM_CRYPTOAesCbcStatus
    #endif
    #ifdef ROM_CRYPTOAesEcb
        #undef  CRYPTOAesEcb
        #define CRYPTOAesEcb                    ROM_CRYPTOAesEcb
    #endif
    #ifdef ROM_CRYPTOAesEcbStatus
        #undef  CRYPTOAesEcbStatus
        #define CRYPTOAesEcbStatus              ROM_CRYPTOAesEcbStatus
    #endif
    #ifdef ROM_CRYPTOCcmAuthEncrypt
        #undef  CRYPTOCcmAuthEncrypt
        #define CRYPTOCcmAuthEncrypt            ROM_CRYPTOCcmAuthEncrypt
    #endif
    #ifdef ROM_CRYPTOCcmAuthEncryptStatus
        #undef  CRYPTOCcmAuthEncryptStatus
        #define CRYPTOCcmAuthEncryptStatus      ROM_CRYPTOCcmAuthEncryptStatus
    #endif
    #ifdef ROM_CRYPTOCcmAuthEncryptResultGet
        #undef  CRYPTOCcmAuthEncryptResultGet
        #define CRYPTOCcmAuthEncryptResultGet   ROM_CRYPTOCcmAuthEncryptResultGet
    #endif
    #ifdef ROM_CRYPTOCcmInvAuthDecrypt
        #undef  CRYPTOCcmInvAuthDecrypt
        #define CRYPTOCcmInvAuthDecrypt         ROM_CRYPTOCcmInvAuthDecrypt
    #endif
    #ifdef ROM_CRYPTOCcmInvAuthDecryptStatus
        #undef  CRYPTOCcmInvAuthDecryptStatus
        #define CRYPTOCcmInvAuthDecryptStatus   ROM_CRYPTOCcmInvAuthDecryptStatus
    #endif
    #ifdef ROM_CRYPTOCcmInvAuthDecryptResultGet
        #undef  CRYPTOCcmInvAuthDecryptResultGet
        #define CRYPTOCcmInvAuthDecryptResultGet ROM_CRYPTOCcmInvAuthDecryptResultGet
    #endif
    #ifdef ROM_CRYPTODmaEnable
        #undef  CRYPTODmaEnable
        #define CRYPTODmaEnable                 ROM_CRYPTODmaEnable
    #endif
    #ifdef ROM_CRYPTODmaDisable
        #undef  CRYPTODmaDisable
        #define CRYPTODmaDisable                ROM_CRYPTODmaDisable
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

#endif  // __CRYPTO_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
