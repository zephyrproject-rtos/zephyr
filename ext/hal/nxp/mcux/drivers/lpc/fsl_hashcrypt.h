/*
 * Copyright 2017-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_HASHCRYPT_H_
#define _FSL_HASHCRYPT_H_

#include "fsl_common.h"

/*! @brief HASHCRYPT status return codes. */
enum _hashcrypt_status
{
    kStatus_HASHCRYPT_Again =
        MAKE_STATUS(kStatusGroup_HASHCRYPT, 0), /*!< Non-blocking function shall be called again. */
};

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*!
 * @addtogroup hashcrypt_driver
 * @{
 */
/*! @name Driver version */
/*@{*/
/*! @brief HASHCRYPT driver version. Version 2.0.0.
 *
 * Current version: 2.0.0
 *
 * Change log:
 * - Version 2.0.0
 *   - Initial version
 */
#define FSL_HASHCRYPT_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*! @brief Algorithm used for Hashcrypt operation */
typedef enum _hashcrypt_algo_t
{
    kHASHCRYPT_Sha1 = 1,   /*!< SHA_1 */
    kHASHCRYPT_Sha256 = 2, /*!< SHA_256 */
    kHASHCRYPT_Sha512 = 3, /*!< SHA_512 */
    kHASHCRYPT_Aes = 4,    /*!< AES */
    kHASHCRYPT_AesIcb = 5, /*!< AES_ICB */
} hashcrypt_algo_t;

/*! @} */

/*******************************************************************************
 * AES Definitions
 *******************************************************************************/

/*!
 * @addtogroup hashcrypt_driver_aes
 * @{
 */

/*! AES block size in bytes */
#define HASHCRYPT_AES_BLOCK_SIZE 16
#define AES_ENCRYPT 0
#define AES_DECRYPT 1

/*! @brief AES mode */
typedef enum _hashcrypt_aes_mode_t
{
    kHASHCRYPT_AesEcb = 0U, /*!< AES ECB mode */
    kHASHCRYPT_AesCbc = 1U, /*!< AES CBC mode */
    kHASHCRYPT_AesCtr = 2U, /*!< AES CTR mode */
} hashcrypt_aes_mode_t;

/*! @brief Size of AES key */
typedef enum _hashcrypt_aes_keysize_t
{
    kHASHCRYPT_Aes128 = 0U,     /*!< AES 128 bit key */
    kHASHCRYPT_Aes192 = 1U,     /*!< AES 192 bit key */
    kHASHCRYPT_Aes256 = 2U,     /*!< AES 256 bit key */
    kHASHCRYPT_InvalidKey = 3U, /*!< AES invalid key */
} hashcrypt_aes_keysize_t;

/*! @brief HASHCRYPT key source selection.
 *
 */
typedef enum _hashcrypt_key
{
    kHASHCRYPT_UserKey = 0xc3c3U,   /*!< HASHCRYPT user key */
    kHASHCRYPT_SecretKey = 0x3c3cU, /*!< HASHCRYPT secret key (dedicated hw bus from PUF) */
} hashcrypt_key_t;

/*! @brief Specify HASHCRYPT's key resource. */
typedef struct _hashcrypt_handle
{
    uint32_t keyWord[8]; /*!< Copy of user key (set by HASHCRYPT_AES_SetKey(). */
    hashcrypt_aes_keysize_t keySize;
    hashcrypt_key_t keyType; /*!< For operations with key (such as AES encryption/decryption), specify key type. */
} hashcrypt_handle_t;

/*!
 *@}
 */ /* end of hashcrypt_driver_aes */

/*******************************************************************************
 * HASH Definitions
 ******************************************************************************/
/*!
 * @addtogroup hashcrypt_driver_hash
 * @{
 */

/*! @brief HASHCRYPT HASH Context size. */
#define HASHCRYPT_HASH_CTX_SIZE 22

/*! @brief Storage type used to save hash context. */
typedef struct _hashcrypt_hash_ctx_t
{
    uint32_t x[HASHCRYPT_HASH_CTX_SIZE]; /*!< storage */
} hashcrypt_hash_ctx_t;

/*! @brief HASHCRYPT background hash callback function. */
typedef void (*hashcrypt_callback_t)(HASHCRYPT_Type *base, hashcrypt_hash_ctx_t *ctx, status_t status, void *userData);

/*!
 *@}
 */ /* end of hashcrypt_driver_hash */

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @addtogroup hashcrypt_driver
 * @{
 */

/*!
 * @brief Enables clock and disables reset for HASHCRYPT peripheral.
 *
 * Enable clock and disable reset for HASHCRYPT.
 *
 * @param base HASHCRYPT base address
 */
void HASHCRYPT_Init(HASHCRYPT_Type *base);

/*!
 * @brief Disables clock for HASHCRYPT peripheral.
 *
 * Disable clock and enable reset.
 *
 * @param base HASHCRYPT base address
 */
void HASHCRYPT_Deinit(HASHCRYPT_Type *base);

/*!
 *@}
 */ /* end of hashcrypt_driver */

/*******************************************************************************
 * AES API
 ******************************************************************************/

/*!
 * @addtogroup hashcrypt_driver_aes
 * @{
 */

/*!
 * @brief Set AES key to hashcrypt_handle_t struct and optionally to HASHCRYPT.
 *
 * Sets the AES key for encryption/decryption with the hashcrypt_handle_t structure.
 * The hashcrypt_handle_t input argument specifies key source.
 *
 * @param   base HASHCRYPT peripheral base address.
 * @param   handle Handle used for the request.
 * @param   key 0-mod-4 aligned pointer to AES key.
 * @param   keySize AES key size in bytes. Shall equal 16, 24 or 32.
 * @return  status from set key operation
 */
status_t HASHCRYPT_AES_SetKey(HASHCRYPT_Type *base, hashcrypt_handle_t *handle, const uint8_t *key, size_t keySize);

/*!
 * @brief Encrypts AES on one or multiple 128-bit block(s).
 *
 * Encrypts AES.
 * The source plaintext and destination ciphertext can overlap in system memory.
 *
 * @param base HASHCRYPT peripheral base address
 * @param handle Handle used for this request.
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @return Status from encrypt operation
 */
status_t HASHCRYPT_AES_EncryptEcb(
    HASHCRYPT_Type *base, hashcrypt_handle_t *handle, const uint8_t *plaintext, uint8_t *ciphertext, size_t size);

/*!
 * @brief Decrypts AES on one or multiple 128-bit block(s).
 *
 * Decrypts AES.
 * The source ciphertext and destination plaintext can overlap in system memory.
 *
 * @param base HASHCRYPT peripheral base address
 * @param handle Handle used for this request.
 * @param ciphertext Input plain text to encrypt
 * @param[out] plaintext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @return Status from decrypt operation
 */
status_t HASHCRYPT_AES_DecryptEcb(
    HASHCRYPT_Type *base, hashcrypt_handle_t *handle, const uint8_t *ciphertext, uint8_t *plaintext, size_t size);

/*!
 * @brief Encrypts AES using CBC block mode.
 *
 * @param base HASHCRYPT peripheral base address
 * @param handle Handle used for this request.
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input initial vector to combine with the first input block.
 * @return Status from encrypt operation
 */
status_t HASHCRYPT_AES_EncryptCbc(HASHCRYPT_Type *base,
                                  hashcrypt_handle_t *handle,
                                  const uint8_t *plaintext,
                                  uint8_t *ciphertext,
                                  size_t size,
                                  const uint8_t iv[16]);

/*!
 * @brief Decrypts AES using CBC block mode.
 *
 * @param base HASHCRYPT peripheral base address
 * @param handle Handle used for this request.
 * @param ciphertext Input cipher text to decrypt
 * @param[out] plaintext Output plain text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input initial vector to combine with the first input block.
 * @return Status from decrypt operation
 */
status_t HASHCRYPT_AES_DecryptCbc(HASHCRYPT_Type *base,
                                  hashcrypt_handle_t *handle,
                                  const uint8_t *ciphertext,
                                  uint8_t *plaintext,
                                  size_t size,
                                  const uint8_t iv[16]);

/*!
 * @brief Encrypts or decrypts AES using CTR block mode.
 *
 * Encrypts or decrypts AES using CTR block mode.
 * AES CTR mode uses only forward AES cipher and same algorithm for encryption and decryption.
 * The only difference between encryption and decryption is that, for encryption, the input argument
 * is plain text and the output argument is cipher text. For decryption, the input argument is cipher text
 * and the output argument is plain text.
 *
 * @param base HASHCRYPT peripheral base address
 * @param handle Handle used for this request.
 * @param input Input data for CTR block mode
 * @param[out] output Output data for CTR block mode
 * @param size Size of input and output data in bytes
 * @param[in,out] counter Input counter (updates on return)
 * @param[out] counterlast Output cipher of last counter, for chained CTR calls (statefull encryption). NULL can be
 * passed if chained calls are
 * not used.
 * @param[out] szLeft Output number of bytes in left unused in counterlast block. NULL can be passed if chained calls
 * are not used.
 * @return Status from encrypt operation
 */
status_t HASHCRYPT_AES_CryptCtr(HASHCRYPT_Type *base,
                                hashcrypt_handle_t *handle,
                                const uint8_t *input,
                                uint8_t *output,
                                size_t size,
                                uint8_t counter[HASHCRYPT_AES_BLOCK_SIZE],
                                uint8_t counterlast[HASHCRYPT_AES_BLOCK_SIZE],
                                size_t *szLeft);

/*!
 *@}
 */ /* end of hashcrypt_driver_aes */

/*******************************************************************************
 * HASH API
 ******************************************************************************/

/*!
 * @addtogroup hashcrypt_driver_hash
 * @{
 */

/*!
 * @brief Create HASH on given data
 *
 * Perform the full SHA in one function call. The function is blocking.
 *
 * @param base HASHCRYPT peripheral base address
 * @param algo Underlaying algorithm to use for hash computation.
 * @param input Input data
 * @param inputSize Size of input data in bytes
 * @param[out] output Output hash data
 * @param[out] outputSize Output parameter storing the size of the output hash in bytes
 * @return Status of the one call hash operation.
 */
status_t HASHCRYPT_SHA(HASHCRYPT_Type *base,
                       hashcrypt_algo_t algo,
                       const uint8_t *input,
                       size_t inputSize,
                       uint8_t *output,
                       size_t *outputSize);

/*!
 * @brief Initialize HASH context
 *
 * This function initializes the HASH.
 *
 * @param base HASHCRYPT peripheral base address
 * @param[out] ctx Output hash context
 * @param algo Underlaying algorithm to use for hash computation.
 * @return Status of initialization
 */
status_t HASHCRYPT_SHA_Init(HASHCRYPT_Type *base, hashcrypt_hash_ctx_t *ctx, hashcrypt_algo_t algo);

/*!
 * @brief Add data to current HASH
 *
 * Add data to current HASH. This can be called repeatedly with an arbitrary amount of data to be
 * hashed. The functions blocks. If it returns kStatus_Success, the running hash
 * has been updated (HASHCRYPT has processed the input data), so the memory at \p input pointer
 * can be released back to system. The HASHCRYPT context buffer is updated with the running hash
 * and with all necessary information to support possible context switch.
 *
 * @param base HASHCRYPT peripheral base address
 * @param[in,out] ctx HASH context
 * @param input Input data
 * @param inputSize Size of input data in bytes
 * @return Status of the hash update operation
 */
status_t HASHCRYPT_SHA_Update(HASHCRYPT_Type *base, hashcrypt_hash_ctx_t *ctx, const uint8_t *input, size_t inputSize);

/*!
 * @brief Finalize hashing
 *
 * Outputs the final hash (computed by HASHCRYPT_HASH_Update()) and erases the context.
 *
 * @param base HASHCRYPT peripheral base address
 * @param[in,out] ctx Input hash context
 * @param[out] output Output hash data
 * @param[in,out] outputSize Optional parameter (can be passed as NULL). On function entry, it specifies the size of
 * output[] buffer. On function return, it stores the number of updated output bytes.
 * @return Status of the hash finish operation
 */
status_t HASHCRYPT_SHA_Finish(HASHCRYPT_Type *base, hashcrypt_hash_ctx_t *ctx, uint8_t *output, size_t *outputSize);

/*!
 *@}
 */ /* end of hashcrypt_driver_hash */

/*!
* @addtogroup hashcrypt_background_driver_hash
* @{
*/

/*!
 * @brief Initializes the HASHCRYPT handle for background hashing.
 *
 * This function initializes the hash context for background hashing
 * (Non-blocking) APIs. This is less typical interface to hash function, but can be used
 * for parallel processing, when main CPU has something else to do.
 * Example is digital signature RSASSA-PKCS1-V1_5-VERIFY((n,e),M,S) algorithm, where
 * background hashing of M can be started, then CPU can compute S^e mod n
 * (in parallel with background hashing) and once the digest becomes available,
 * CPU can proceed to comparison of EM with EM'.
 *
 * @param base HASHCRYPT peripheral base address.
 * @param[out] ctx Hash context.
 * @param callback Callback function.
 * @param userData User data (to be passed as an argument to callback function, once callback is invoked from isr).
 */
void HASHCRYPT_SHA_SetCallback(HASHCRYPT_Type *base,
                               hashcrypt_hash_ctx_t *ctx,
                               hashcrypt_callback_t callback,
                               void *userData);

/*!
* @brief Create running hash on given data.
*
* Configures the HASHCRYPT to compute new running hash as AHB master
* and returns immediately. HASHCRYPT AHB Master mode supports only aligned \p input
* address and can be called only once per continuous block of data. Every call to this function
* must be preceded with HASHCRYPT_SHA_Init() and finished with HASHCRYPT_SHA_Finish().
* Once callback function is invoked by HASHCRYPT isr, it should set a flag
* for the main application to finalize the hashing (padding) and to read out the final digest
* by calling HASHCRYPT_SHA_Finish().
*
* @param base HASHCRYPT peripheral base address
* @param ctx Specifies callback. Last incomplete 512-bit block of the input is copied into clear buffer for padding.
* @param input 32-bit word aligned pointer to Input data.
* @param inputSize Size of input data in bytes (must be word aligned)
* @return Status of the hash update operation.
*/
status_t HASHCRYPT_SHA_UpdateNonBlocking(HASHCRYPT_Type *base,
                                         hashcrypt_hash_ctx_t *ctx,
                                         const uint8_t *input,
                                         size_t inputSize);
/*!
 *@}
 */ /* end of hashcrypt_background_driver_hash */

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_HASHCRYPT_H_ */
