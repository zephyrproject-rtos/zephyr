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
 *  @file       CryptoKey.h
 *
 * @brief The CryptoKey type is an opaque representation of a cryptographic key.
 *
 * @warning This is a beta API. It may change in future releases.
 *
 * Cryptographic keying material may be stored on an embedded system multiple ways.
 *  - plaintext: in plaintext in flash or RAM
 *  - keyblob: in encrypted form in flash or RAM
 *  - key store: in a dedicated hardware database whose entries can not be directly
 *    read out.
 *
 * Each storage option requires different approaches to handling the keying material
 * when performing a crypto operation. In order to separate these concerns from
 * the API of the various crypto drivers available with TI-RTOS, the CryptoKey
 * type abstracts away from these details. It does not contain any cryptographic
 * keying material itself but instead contains the details necessary for drivers to use the
 * keying material. The driver implementation handles preparing and moving the keying
 * material as necessary to perform the desired crypto operation.
 *
 * The same CryptoKey may be passed to crypto APIs of different modes subject to
 * restrictions placed on the key by their storage types. Plaintext keys may be used
 * without restriction while key store and keyblob keys have their permitted uses
 * restricted when the keying material is loaded or the keyblob is encrypted respectively.
 * These restrictions are specified in a CryptoKey_SecurityPolicy that is device-specific
 * and depends on the hardware capability of the device.
 *
 * An application should never access a field within a CryptoKey struct itself.
 * Where needed, helper functions are provided to do so.
 *
 * Before using a CryptoKey in another crypto API call, it must be initialized
 * with a call to one of the initialization functions.
 *  - CryptoKeyPlaintext_initKey()
 *  - CryptoKeyPlaintext_initBlankKey()
 *  - CryptoKeyKeyStore_initKey()
 *  - CryptoKeyKeyStore_initBlankKey()
 *  - CryptoKeyKeyBlob_initKey()
 *  - CryptoKeyKeyBlob_initBlankKey()
 *
 * The keyblob and keystore CryptoKeys may be used to create a keyblob or
 * load a key into a key store after their respective _init call.
 *
 * CryptoKeys can be initialized "blank", without keying material but with an empty buffer
 * or key store entry, to encode the destination of a key to be created in the
 * future. This way, keys may be generated securely within a key store
 * for example and never even be stored in RAM temporarily.
 *
 * Not all devices support all CryptoKey functionality. This is hardware-dependent.
 *
 */

#ifndef ti_drivers_cryptoutils_cyptokey_CryptoKey__include
#define ti_drivers_cryptoutils_cyptokey_CryptoKey__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*!

 */

    /**
 *  @defgroup CryptoKey_CONTROL Status codes
 *  These CryptoKey macros are reservations for CryptoKey.h
 *  @{
 */


/*!
 * Common CryptoKey_control status code reservation offset.
 * CryptoKey driver implementations should offset status codes with
 * CryptoKey_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define CryptoKeyXYZ_STATUS_ERROR0    CryptoKey_STATUS_RESERVED - 0
 * #define CryptoKeyXYZ_STATUS_ERROR1    CryptoKey_STATUS_RESERVED - 1
 * #define CryptoKeyXYZ_STATUS_ERROR2    CryptoKey_STATUS_RESERVED - 2
 * @endcode
 */
#define CryptoKey_STATUS_RESERVED        (-32)

/**
 *  @defgroup CryptoKey_STATUS Status Codes
 *  CryptoKey_STATUS_* macros are general status codes returned by CryptoKey_control()
 *  @{
 *  @ingroup CryptoKey_CONTROL
 */

/*!
 * @brief   Successful status code
 *
 * CryptoKey_control() returns CryptoKey_STATUS_SUCCESS if the control code was executed
 * successfully.
 */
#define CryptoKey_STATUS_SUCCESS         (0)

/*!
 * @brief   Generic error status code
 *
 * CryptoKey_control() returns CryptoKey_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define CryptoKey_STATUS_ERROR           (-1)

/*!
 * @brief   Returned if the encoding of a CryptoKey is not a CryptoKey_Encoding value
 *
 * CryptoKey_control() returns CryptoKey_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define CryptoKey_STATUS_UNDEFINED_ENCODING    (-2)


/** @}*/

/** @}*/

/*!
 *  @brief  List of the different types of CryptoKey.
 *
 */
typedef enum CryptoKey_Encoding_ {
    CryptoKey_PLAINTEXT             = 1 << 1,
    CryptoKey_BLANK_PLAINTEXT       = 1 << 2,
    CryptoKey_KEYSTORE              = 1 << 3,
    CryptoKey_BLANK_KEYSTORE        = 1 << 4,
    CryptoKey_KEYBLOB               = 1 << 5,
    CryptoKey_BLANK_KEYBLOB         = 1 << 6,
} CryptoKey_Encoding;

/*!
 *  @brief  Plaintext CryptoKey datastructure.
 *
 * This structure contains all the information necessary to access keying material stored
 * in plaintext form in flash or RAM.
 */
typedef struct CryptoKey_Plaintext_ {
    uint8_t *keyMaterial;
    uint16_t keyLength;
} CryptoKey_Plaintext;

/*!
 *  @brief  Key store CryptoKey datastructure.
 *
 * This structure contains all the information necessary to access keying material stored
 * in a dedicated key store or key database with memory access controls.
 */
typedef struct CryptoKey_KeyStore_ {
    void* keyStore;
    uint16_t keyLength;
    uint32_t keyIndex;
} CryptoKey_KeyStore;

/*!
 *  @brief  Keyblob CryptoKey datastructure.
 *
 * This structure contains all the information necessary to access keying material stored
 * in an encrypted structure in flash or RAM.
 */
typedef struct CryptoKey_KeyBlob_ {
    uint8_t *keyBlob;
    uint32_t keyBlobLength;
} CryptoKey_KeyBlob;

/*!
 * @brief  CryptoKey datastructure.
 *
 * This structure contains a CryptoKey_Encoding and one of
 * - CryptoKey_Plaintext
 * - CryptoKey_KeyStore
 * - CryptoKey_KeyBlob
 */
typedef struct CryptoKey_ {
    CryptoKey_Encoding encoding;
    union {
        CryptoKey_Plaintext plaintext;
        CryptoKey_KeyStore keyStore;
        CryptoKey_KeyBlob keyBlob;
    } u;
} CryptoKey;


/*!
 * @brief  Structure that specifies the restrictions on a CryptoKey
 *
 * This structure is device-specific and declared here in incomplete form.
 * The structure is fully defined in CryptoKeyDEVICE.h. This creates a link-time binding
 * when using the structure with key store or keyblob functions. If the instance
 * of the CryptoKey_SecurityPolicy is kept in a device-specific application-file,
 * the gernic application code may still use references to it despite being
 * an incomplete type in the generic application file at compile time.
 */
typedef struct CryptoKey_SecurityPolicy_ CryptoKey_SecurityPolicy;

/*!
 *  @brief Gets the key type of the CryptoKey
 *
 *  @param [in]     keyHandle   Pointer to a CryptoKey
 *  @param [out]    keyType     Type of the CryptoKey
 *
 *  @return Returns a status code
 */
int_fast16_t CryptoKey_getCryptoKeyType(CryptoKey *keyHandle, CryptoKey_Encoding *keyType);

/*!
 *  @brief Wheather the CryptoKey is 'blank' or represents valid keying material
 *
 *  @param [in]     keyHandle   Pointer to a CryptoKey
 *  @param [out]    isBlank     Wheather the CryptoKey is 'blank' or not
 *
 *  @return Returns a status code
 */
int_fast16_t CryptoKey_isBlank(CryptoKey *keyHandle, bool *isBlank);

/*!
 *  @brief Marks a CryptoKey as 'blank'.
 *
 *  The CryptoKey will be unlinked from any previously connected keying material
 *
 *  @param [in]     keyHandle   Pointer to a CryptoKey
 *
 *  @return Returns a status code
 */
int_fast16_t CryptoKey_markAsBlank(CryptoKey *keyHandle);

/*!
 *  @brief Function to initialize the CryptoKey_SecurityPolicy struct to its defaults
 *
 *  This will zero-out all fields that cannot be set to safe defaults
 *
 *  @param [in]     policy   Pointer to a CryptoKey_SecurityPolicy
 *
 *  @return Returns a status code
 */
int_fast16_t CryptoKey_initSecurityPolicy(CryptoKey_SecurityPolicy *policy);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_cryptoutils_cyptokey_CryptoKey__include */
