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
#ifndef AES256_H_
#define AES256_H_

//*****************************************************************************
//
//! \addtogroup aes256_api
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

/* Module Defines and macro for easy access */
#define AES256_CMSIS(x) ((AES256_Type *) x)

//*****************************************************************************
//
// The following are deprecated values. Please refer to documentation for the
// correct values to use.
//
//*****************************************************************************
#define Key_128BIT                                                          128
#define Key_192BIT                                                          192
#define Key_256BIT                                                          256

//*****************************************************************************
//
// The following are values that can be passed to the keyLength parameter for
// functions: AES256_setCipherKey(), AES256_setDecipherKey(), and
// AES256_startSetDecipherKey().
//
//*****************************************************************************
#define AES256_KEYLENGTH_128BIT                                             128
#define AES256_KEYLENGTH_192BIT                                             192
#define AES256_KEYLENGTH_256BIT                                             256

//*****************************************************************************
//
// The following are values that can be passed toThe following are values that
// can be returned by the AES256_getErrorFlagStatus() function.
//
//*****************************************************************************
#define AES256_ERROR_OCCURRED                                 AES256_CTL0_ERRFG
#define AES256_NO_ERROR                                                    0x00

//*****************************************************************************
//
// The following are values that can be passed toThe following are values that
// can be returned by the AES256_isBusy() function.
//
//*****************************************************************************
#define AES256_BUSY                                            AES256_STAT_BUSY
#define AES256_NOT_BUSY                                                    0x00

//*****************************************************************************
//
// The following are values that can be passed toThe following are values that
// can be returned by the AES256_getInterruptFlagStatus() function.
//
//*****************************************************************************
#define AES256_READY_INTERRUPT                                             0x01
#define AES256_NOTREADY_INTERRUPT                                          0x00

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Loads a 128, 192 or 256 bit cipher key to AES256 module.
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param cipherKey is a pointer to an uint8_t array with a length of 16 bytes
//!        that contains a 128 bit cipher key.
//! \param keyLength is the length of the key.
//!        Valid values are:
//!        - \b AES256_KEYLENGTH_128BIT
//!        - \b AES256_KEYLENGTH_192BIT
//!        - \b AES256_KEYLENGTH_256BIT
//!
//! \return true if set correctly, false otherwise
//
//*****************************************************************************
extern bool AES256_setCipherKey(uint32_t moduleInstance,
        const uint8_t *cipherKey, uint_fast16_t keyLength);

//*****************************************************************************
//
//! \brief Encrypts a block of data using the AES256 module.
//!
//! The cipher key that is used for encryption should be loaded in advance by
//! using function AES256_setCipherKey()
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param data is a pointer to an uint8_t array with a length of 16 bytes that
//!        contains data to be encrypted.
//! \param encryptedData is a pointer to an uint8_t array with a length of 16
//!        bytes in that the encrypted data will be written.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_encryptData(uint32_t moduleInstance, const uint8_t *data,
        uint8_t *encryptedData);

//*****************************************************************************
//
//! \brief Decrypts a block of data using the AES256 module.
//!
//! This function requires a pregenerated decryption key. A key can be loaded
//! and pregenerated by using function AES256_setDecipherKey() or
//! AES256_startSetDecipherKey(). The decryption takes 167 MCLK.
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param data is a pointer to an uint8_t array with a length of 16 bytes that
//!        contains encrypted data to be decrypted.
//! \param decryptedData is a pointer to an uint8_t array with a length of 16
//!        bytes in that the decrypted data will be written.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_decryptData(uint32_t moduleInstance, const uint8_t *data,
        uint8_t *decryptedData);

//*****************************************************************************
//
//! \brief Sets the decipher key.
//!
//! The API AES256_startSetDecipherKey or AES256_setDecipherKey must be invoked
//! before invoking AES256_startDecryptData.
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param cipherKey is a pointer to an uint8_t array with a length of 16 bytes
//!        that contains a 128 bit cipher key.
//! \param keyLength is the length of the key.
//!        Valid values are:
//!        - \b AES256_KEYLENGTH_128BIT
//!        - \b AES256_KEYLENGTH_192BIT
//!        - \b AES256_KEYLENGTH_256BIT
//!
//! \return true if set, false otherwise
//
//*****************************************************************************
extern bool AES256_setDecipherKey(uint32_t moduleInstance,
        const uint8_t *cipherKey, uint_fast16_t keyLength);

//*****************************************************************************
//
//! \brief Clears the AES256 ready interrupt flag.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! Modified bits are \b AESRDYIFG of \b AESACTL0 register.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_clearInterruptFlag(uint32_t moduleInstance);

//*****************************************************************************
//
//! \brief Gets the AES256 ready interrupt flag status.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! \return One of the following:
//!         - \b AES256_READY_INTERRUPT
//!         - \b AES256_NOTREADY_INTERRUPT
//!         \n indicating the status of the AES256 ready status
//
//*****************************************************************************
extern uint32_t AES256_getInterruptFlagStatus(uint32_t moduleInstance);

//*****************************************************************************
//
//! \brief Enables AES256 ready interrupt.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! Modified bits are \b AESRDYIE of \b AESACTL0 register.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_enableInterrupt(uint32_t moduleInstance);

//*****************************************************************************
//
//! \brief Disables AES256 ready interrupt.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! Modified bits are \b AESRDYIE of \b AESACTL0 register.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_disableInterrupt(uint32_t moduleInstance);

//*****************************************************************************
//
//! \brief Resets AES256 Module immediately.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! Modified bits are \b AESSWRST of \b AESACTL0 register.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_reset(uint32_t moduleInstance);

//*****************************************************************************
//
//! \brief Starts an encryption process on the AES256 module.
//!
//! The cipher key that is used for decryption should be loaded in advance by
//! using function AES256_setCipherKey(). This is a non-blocking equivalent pf
//! AES256_encryptData(). It is recommended to use the interrupt functionality
//! to check for procedure completion then use the AES256_getDataOut() API to
//! retrieve the encrypted data.
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param data is a pointer to an uint8_t array with a length of 16 bytes that
//!        contains data to be encrypted.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_startEncryptData(uint32_t moduleInstance,
        const uint8_t *data);

//*****************************************************************************
//
//! \brief Decypts a block of data using the AES256 module.
//!
//! This is the non-blocking equivalant of AES256_decryptData(). This function
//! requires a pregenerated decryption key. A key can be loaded and
//! pregenerated by using function AES256_setDecipherKey() or
//! AES256_startSetDecipherKey(). The decryption takes 167 MCLK. It is
//! recommended to use interrupt to check for procedure completion then use the
//! AES256_getDataOut() API to retrieve the decrypted data.
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param data is a pointer to an uint8_t array with a length of 16 bytes that
//!        contains encrypted data to be decrypted.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_startDecryptData(uint32_t moduleInstance,
        const uint8_t *data);

//*****************************************************************************
//
//! \brief Sets the decipher key
//!
//! The API AES256_startSetDecipherKey() or AES256_setDecipherKey() must be
//! invoked before invoking AES256_startDecryptData.
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param cipherKey is a pointer to an uint8_t array with a length of 16 bytes
//!        that contains a 128 bit cipher key.
//! \param keyLength is the length of the key.
//!        Valid values are:
//!        - \b AES256_KEYLENGTH_128BIT
//!        - \b AES256_KEYLENGTH_192BIT
//!        - \b AES256_KEYLENGTH_256BIT
//!
//! \return true if set correctly, false otherwise
//
//*****************************************************************************
extern bool AES256_startSetDecipherKey(uint32_t moduleInstance,
        const uint8_t *cipherKey, uint_fast16_t keyLength);

//*****************************************************************************
//
//! \brief Reads back the output data from AES256 module.
//!
//! This function is meant to use after an encryption or decryption process
//! that was started and finished by initiating an interrupt by use of
//! AES256_startEncryptData or AES256_startDecryptData functions.
//!
//! \param moduleInstance is the base address of the AES256 module.
//! \param outputData is a pointer to an uint8_t array with a length of 16
//!        bytes in that the data will be written.
//!
//! \return true if data is valid, otherwise false
//
//*****************************************************************************
extern bool AES256_getDataOut(uint32_t moduleInstance,
        uint8_t *outputData);

//*****************************************************************************
//
//! \brief Gets the AES256 module busy status.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! \return true if busy, false otherwise
//
//*****************************************************************************
extern bool AES256_isBusy(uint32_t moduleInstance);

//*****************************************************************************
//
//! \brief Clears the AES256 error flag.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! Modified bits are \b AESERRFG of \b AESACTL0 register.
//!
//! \return None
//
//*****************************************************************************
extern void AES256_clearErrorFlag(uint32_t moduleInstance);

//*****************************************************************************
//
//! \brief Gets the AES256 error flag status.
//!
//! \param moduleInstance is the base address of the AES256 module.
//!
//! \return One of the following:
//!         - \b AES256_ERROR_OCCURRED
//!         - \b AES256_NO_ERROR
//!         \n indicating the error flag status
//
//*****************************************************************************
extern uint32_t AES256_getErrorFlagStatus(uint32_t moduleInstance);

//*****************************************************************************
//
//! Registers an interrupt handler for the AES interrupt.
//!
//! \param moduleInstance Instance of the AES256 module
//!
//! \param intHandler is a pointer to the function to be called when the
//! AES interrupt occurs.
//!
//! This function registers the handler to be called when a AES
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific AES interrupts must be enabled
//! via AES256_enableInterrupt().  It is the interrupt handler's responsibility
//! to clear the interrupt source via AES256_clearInterrupt().
//!
//! \return None.
//
//*****************************************************************************
extern void AES256_registerInterrupt(uint32_t moduleInstance,
        void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the AES interrupt
//!
//! \param moduleInstance Instance of the AES256 module
//!
//! This function unregisters the handler to be called when AES
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void AES256_unregisterInterrupt(uint32_t moduleInstance);

//*****************************************************************************
//
//! Returns the current interrupt flag for the peripheral.
//!
//! \param moduleInstance Instance of the AES256 module
//!
//! \return The currently triggered interrupt flag for the module.
//
//*****************************************************************************
extern uint32_t AES256_getInterruptStatus(uint32_t moduleInstance);

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

#endif /* AES256_H_ */

