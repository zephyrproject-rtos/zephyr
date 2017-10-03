/*
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
 */
/** ============================================================================
 *  @file       CryptoKeyPlaintext.h
 *
 * @warning     This is a beta API. It may change in future releases.
 *
 * # Overview #
 * This file contains the APIs to initialize and access plaintext CryptoKeys.
 * Plaintext CryptoKeys point to keying material stored in flash or RAM and
 * are not subject to enforced usage restrictions. That only means that calling
 * a function that requires an assymmetric public key with a symmetric key will
 * not return an error. It will likely not yield the desired results.
 *
 * # Usage #
 *
 * Plaintext keys are the simplest of the CryptoKeys. All they do is store the length
 * of and a pointer to the keying material. Their use is hence simple as well. After
 * calling the initialization function, the CryptoKey may be used in any of the
 * crypto operation APIs that take a CryptoKey as an input.
 *
 * @code
 *
 * uint8_t keyingMaterial[16];
 * CryptoKey cryptoKey;
 *
 * // Initialise the CryptoKey
 * CryptoKeyPlaintext_initKey(&cryptoKey, keyingMaterial, sizeof(keyingMaterial));
 *
 * // Use the CryptoKey in another crypto operation
 *
 * @endcode
 *
 */

#ifndef ti_drivers_cryptoutils_cyptokey_CryptoKeyPlaintext__include
#define ti_drivers_cryptoutils_cyptokey_CryptoKeyPlaintext__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <ti/drivers/cryptoutils/cryptokey/CryptoKey.h>

/*!
 *  @brief Initializes a CryptoKey type
 *
 *  @param [in]     keyHandle   Pointer to a CryptoKey which will be initialized to type CryptoKey_PLAINTEXT
 *                              and ready for use
 *  @param [in]     key         Pointer to keying material
 *
 *  @param [in]     keyLength   Length of keying material in bytes
 *
 *  @return Returns a status code from CryptoKey.h
 */
int_fast16_t CryptoKeyPlaintext_initKey(CryptoKey *keyHandle, uint8_t *key, size_t keyLength);


/*!
 *  @brief Initializes an empty plaintext CryptoKey type
 *
 *  @param [in]     keyHandle       Pointer to a CryptoKey which will be initialized to type
 *                                  CryptoKey_BLANK_PLAINTEXT
 *
 *  @param [in]     keyLocation     Pointer to location where plaintext keying material can be stored
 *
 *  @param [in]     keyLength       Length of keying material, in bytes
 *
 *  @return Returns a status code from CryptoKey.h
 */
int_fast16_t CryptoKeyPlaintext_initBlankKey(CryptoKey *keyHandle, uint8_t *keyLocation, size_t keyLength);

/*!
 *  @brief Gets the length of a plaintext key
 *
 *  @param [in]      keyHandle  Pointer to a plaintext CryptoKey
 *
 *  @param [out]     length     Length of the keying material, in bytes
 *
 *  @return Returns a status code from CryptoKey.h
 */
int_fast16_t CryptoKeyPlaintext_getKeyLength(CryptoKey *keyHandle, size_t *length);

/*!
 * @brief Sets the CryptoKey keyMaterial pointer
 *
 *  Updates the key location for a plaintext CryptoKey.
 *  Does not modify data at the pointer location.
 *
 *  @param [in]     keyHandle   Pointer to a plaintext CryptoKey who's key data pointer will be modified
 *
 *  @param [in]     location    Pointer to key data location
 *
 *  @return Returns a status code from CryptoKey.h
 */
int_fast16_t CryptoKeyPlaintext_setKeyLocation(CryptoKey *keyHandle, uint8_t *location);


/*!
 *  @brief Gets the length of a plaintext key
 *
 *  @param [in]      keyHandle   Pointer to a CryptoKey
 *
 *  @param [out]     length      Length value will be updated to CryptoKey length, in bytes
 *
 *  @return Returns a status code from CryptoKey.h
 */
int_fast16_t CryptoKeyPlaintext_getKeyLength(CryptoKey *keyHandle, size_t *length);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_cryptoutils_cyptoKey_CryptoKeyPlaintext__include */
