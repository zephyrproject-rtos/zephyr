/*
 * Copyright (c) 2017-2018, NXP Semiconductors, Inc.
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_CAU3_H_
#define _FSL_CAU3_H_

#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*!
 * @addtogroup cau3_driver
 * @{
 */
/*! @name Driver version */
/*@{*/
/*! @brief CAU3 driver version. Version 2.0.2.
 *
 * Current version: 2.0.2
 *
 * Change log:
 * - Version 2.0.0
 *   - Initial version
 * - Version 2.0.1
 *   - Replace static cau3_make_mems_private() with public CAU3_MakeMemsPrivate().
 *   - Remove the cau3_make_mems_private() from CAU3_Init to allow loading multiple images.
 * - Version 2.0.2
 *   - Add FSL_CAU3_USE_HW_SEMA compile time macro. When enabled, all CAU3 API functions
 *     lock hw semaphore on function entry and release the hw semaphore on function return.
 */
#define FSL_CAU3_DRIVER_VERSION (MAKE_VERSION(2, 0, 2))
/*@}*/

/*! @brief Hardware semaphore usage by driver functions.
 *  This macro can be enabled for mutual exclusive calls to CAU3 APIs
 *  from multiple CPUs. Note this does not lock against calls from multiple threads on one CPU.
 */

/* #define FSL_CAU3_USE_HW_SEMA 1 */

#if defined(FSL_CAU3_USE_HW_SEMA) && (FSL_CAU3_USE_HW_SEMA > 0)
/* Using SEMA42 hardware semaphore for multiprocessor locking.
 * Currently supporting only SEMA42 hardware semaphore.
 */
#if defined(FSL_FEATURE_SOC_SEMA42_COUNT) && (FSL_FEATURE_SOC_SEMA42_COUNT > 0)
#include "fsl_sema42.h"
#define FSL_CAU3_SEMA42_BASE SEMA420
#define FSL_CAU3_SEMA42_GATE (1)
#define FSL_CAU3_SEMA42_CLOCK_NAME kCLOCK_Sema420
#else
#error FSL_CAU3_USE_HW_SEMA requires SEMA42 semaphore.
#endif
#endif

/*! @brief CAU3 key slot selection. Current CryptoCore firmware supports 4 key slots inside CryptoCore's Private DMEM.
 *
 */
typedef enum _cau3_key_slot {
    kCAU3_KeySlot0 = 0x0U,    /*!< CAU3 key slot 0. */
    kCAU3_KeySlotNone = 0x0U, /*!< No key. */
    kCAU3_KeySlot1 = 0x1U,    /*!< CAU3 key slot 1. */
    kCAU3_KeySlot2 = 0x2U,    /*!< CAU3 key slot 2.*/
    kCAU3_KeySlot3 = 0x3U,    /*!< CAU3 key slot 3. */
} cau3_key_slot_t;

/*! @brief CAU3 task done selection. */
typedef enum _cau3_task_done {
    kCAU3_TaskDoneNull = 0x00000000U,      /*!< */
    kCAU3_TaskDonePoll = 0x00000000U,      /*!< Poll CAU3 status flag. */
    kCAU3_TaskDoneIrq = 0x00010000U,       /*!< Start operation and return. CAU3 asserts interrupt request when done. */
    kCAU3_TaskDoneEvent = 0x00020000U,     /*!< Call Wait-for-event opcode until CAU3 completes processing. */
    kCAU3_TaskDoneDmaRequest = 0x00040000, /*!< Start operation and return. CAU3 asserts DMA request when done. */
} cau3_task_done_t;

/*! @brief Specify CAU3's key resource and signalling to be used for an operation. */
typedef struct _cau3_handle
{
    cau3_task_done_t taskDone; /*!< Specify CAU3 task done signalling to Host CPU. */
    cau3_key_slot_t keySlot; /*!< For operations with key (such as AES encryption/decryption), specify CAU3 key slot. */
} cau3_handle_t;

/*! @} */

/*******************************************************************************
 * AES Definitions
 *******************************************************************************/

/*!
 * @addtogroup cau3_driver_aes
 * @{
 */

/*! AES block size in bytes */
#define CAU3_AES_BLOCK_SIZE 16

/*!
 *@}
 */ /* end of cau3_driver_aes */

/*******************************************************************************
 * HASH Definitions
 ******************************************************************************/
/*!
 * @addtogroup cau3_driver_hash
 * @{
 */

/*! @brief Supported cryptographic block cipher functions for HASH creation */
typedef enum _cau3_hash_algo_t {
    kCAU3_Sha1,   /*!< SHA_1 */
    kCAU3_Sha256, /*!< SHA_256 */
} cau3_hash_algo_t;

/*! @brief CAU3 HASH Context size. */
#define CAU3_SHA_BLOCK_SIZE 128                  /*!< internal buffer block size  */
#define CAU3_HASH_BLOCK_SIZE CAU3_SHA_BLOCK_SIZE /*!< CAU3 hash block size  */

/*! @brief CAU3 HASH Context size. */
#define CAU3_HASH_CTX_SIZE 58

/*! @brief Storage type used to save hash context. */
typedef struct _cau3_hash_ctx_t
{
    uint32_t x[CAU3_HASH_CTX_SIZE];
} cau3_hash_ctx_t;

/*!
 *@}
 */ /* end of cau3_driver_hash */

/*******************************************************************************
 * PKHA Definitions
 ******************************************************************************/
/*!
 * @addtogroup cau3_driver_pkha
 * @{
 */

/*! PKHA ECC point structure */
typedef struct _cau3_pkha_ecc_point_t
{
    uint8_t *X; /*!< X coordinate (affine) */
    uint8_t *Y; /*!< Y coordinate (affine) */
} cau3_pkha_ecc_point_t;

/*! @brief Use of timing equalized version of a PKHA function. */
typedef enum _cau3_pkha_timing_t {
    kCAU3_PKHA_NoTimingEqualized = 0U, /*!< Normal version of a PKHA operation */
    kCAU3_PKHA_TimingEqualized = 1U    /*!< Timing-equalized version of a PKHA operation  */
} cau3_pkha_timing_t;

/*! @brief Integer vs binary polynomial arithmetic selection. */
typedef enum _cau3_pkha_f2m_t {
    kCAU3_PKHA_IntegerArith = 0U, /*!< Use integer arithmetic */
    kCAU3_PKHA_F2mArith = 1U      /*!< Use binary polynomial arithmetic */
} cau3_pkha_f2m_t;

/*! @brief Montgomery or normal PKHA input format. */
typedef enum _cau3_pkha_montgomery_form_t {
    kCAU3_PKHA_NormalValue = 0U,     /*!< PKHA number is normal integer */
    kCAU3_PKHA_MontgomeryFormat = 1U /*!< PKHA number is in montgomery format */
} cau3_pkha_montgomery_form_t;

/*!
 *@}
 */ /* cau3_driver_pkha */

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @addtogroup cau3_driver
 * @{
 */

/*!
 * @brief   Enables clock for CAU3 and loads image to memory
 *
 * Enable CAU3 clock and loads image to CryptoCore.
 *
 * @param base CAU3 base address
 */
void CAU3_Init(CAU3_Type *base);

/*!
 * @brief   Execute a CAU3 null task to signal error termination
 *
 * Execute a null task to signal error termination.
 * The CryptoCore task executes one instruction - a "stop with error".
 *
 * @param base CAU3 base address
 * @param taskDone indicates completion signal
 *
 * @return status check from task completion
 */
status_t CAU3_ForceError(CAU3_Type *base, cau3_task_done_t taskDone);

/*!
 * @brief   Load special hardware "key context" into the CAU3's data memory
 *
 * Load the special hardware key context into the private DMEM. This only
 * includes the complete 256-bit key which is then specified with a size of
 * [8,16,24,32] bytes (for DES or AES-[128,256]).  It also loads the
 * default IV value specified in NIST/RFC2294 IV=0xa6a6a6a6a6a6a6a6. This operation typically
 * loads keySlot 0, which, by convention, is used for the system key encryption
 * key.
 *
 * See the GENERAL COMMENTS for more information on the keyContext structure.
 *
 * NOTE: This function also performs an AES key expansion if a keySize > 8
 * is specified.
 *
 * @param base CAU3 base address
 * @param   keySize is the logical key size in bytes [8,16,24,32]
 * @param   keySlot is the destination key slot number [0-3]
 * @param   taskDone indicates completion signal.
 *
 * @return  status check from task completion
 */
status_t CAU3_LoadSpecialKeyContext(CAU3_Type *base,
                                    size_t keySize,
                                    cau3_key_slot_t keySlot,
                                    cau3_task_done_t taskDone);

/*!
 * @brief   Invalidate a 64-byte "key context" in the CAU3's private data memory
 *
 * Clears the key context in the private DMEM. There is support for four "key
 * slots" with slot 0 typically used for the system key encryption key.
 *
 * @param base CAU3 base address
 * @param   keySlot is the key slot number [0-3] to invalidate
 * @param   taskDone indicates completion signal *
 * @return  status check from task completion
 */
status_t CAU3_ClearKeyContext(CAU3_Type *base, cau3_key_slot_t keySlot, cau3_task_done_t taskDone);

/*!
 * @brief   Load an initialization vector into a key context
 *
 * Loads a 16-byte initialization vector (iv) into the specified key slot.
 * There is support for a maximum of 4 key slots.
 * The function is used internally for loading AEAD_CHACHA20_POY1305 nonce.
 * It can be also used for Alternative Initial Values for A[0] in RFC 3394.
 *
 * @param base CAU3 base address
 * @param iv The initialization vector, ALIGNED ON A 0-MOD-4 ADDRESS.
 * @param keySlot is the destination key context
 * @param taskDone indicates completion signal
 *
 * @return  status check from task completion
 */
status_t CAU3_LoadKeyInitVector(CAU3_Type *base, const uint8_t *iv, cau3_key_slot_t keySlot, cau3_task_done_t taskDone);

/*!
 * @brief   Make the CAU3's local memories private
 *
 * Modify the CAU3's internal configuration so the local memories are private
 * and only accessible to the CAU3. This operation is typically performed after
 * the CAU_InitializeInstMemory(),
 *     CAU_InitializeDataMemory(), and
 *     CAU_InitializeReadOnlyDataMemory() functions have been performed.
 *
 * This configuration remains in effect until the next hardware reset.
 *
 * @param   taskDone indicates completion signal: CAU_[POLL, IRQ, EVENT, DMAREQ]
 *
 * @retval  status check from task completion: CAU_[OK, ERROR]
 */
status_t CAU3_MakeMemsPrivate(CAU3_Type *base, cau3_task_done_t taskDone);

/*!
 *@}
 */ /* end of cau3_driver */

/*******************************************************************************
 * AES API
 ******************************************************************************/

/*!
 * @addtogroup cau3_driver_aes
 * @{
 */

/*!
 * @brief Load AES key into CAU3 key slot.
 *
 * Load the key context into the private DMEM. This function also performs an AES key expansion.
 * For CAU3 AES encryption/decryption/cmac, users only need to call one of @ref CAU3_AES_SetKey and @ref
 * CAU3_LoadSpecialKeyContext.
 *
 * CAU3_AES_SetKey
 * @param   base CAU3 peripheral base address.
 * @param   handle Handle used for the request.
 * @param   key 0-mod-4 aligned pointer to AES key.
 * @param   keySize AES key size in bytes. Shall equal 16 or 32.
 * @return  status from set key operation
 */
status_t CAU3_AES_SetKey(CAU3_Type *base, cau3_handle_t *handle, const uint8_t *key, size_t keySize);

/*!
 * @brief Encrypts AES on one 128-bit block.
 *
 * Encrypts AES.
 * The source plaintext and destination ciphertext can overlap in system memory.
 *
 * @param base CAU3 peripheral base address
 * @param handle Handle used for this request.
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @return Status from encrypt operation
 */
status_t CAU3_AES_Encrypt(CAU3_Type *base, cau3_handle_t *handle, const uint8_t plaintext[16], uint8_t ciphertext[16]);

/*!
 * @brief Decrypts AES on one 128-bit block.
 *
 * Decrypts AES.
 * The source ciphertext and destination plaintext can overlap in system memory.
 *
 * @param base CAU3 peripheral base address
 * @param handle Handle used for this request.
 * @param ciphertext Input plain text to encrypt
 * @param[out] plaintext Output cipher text
 * @return Status from decrypt operation
 */
status_t CAU3_AES_Decrypt(CAU3_Type *base, cau3_handle_t *handle, const uint8_t ciphertext[16], uint8_t plaintext[16]);

/*!
 * @brief   Perform an AES-128 cipher-based authentication code (CMAC)
 *
 * Performs an AES-128 cipher-based authentication code (CMAC) on a
 * message. RFC 4493.
 *
 * @param base CAU3 peripheral base address
 * @param handle Handle used for this request.
 * @param message is source uint8_t array of data bytes, any alignment
 * @param size Number of bytes in the message.
 * @param mac is the output 16 bytes MAC, must be a 0-mod-4 aligned address
 * @return  status check from task completion
 */
status_t CAU3_AES_Cmac(CAU3_Type *base, cau3_handle_t *handle, const uint8_t *message, size_t size, uint8_t *mac);

/*!
 * @brief   Perform an AES key expansion for specified key slot
 *
 * Performs an AES key expansion (aka schedule) on the specified key slot. It
 * uses the keySize information in the context to determine whether the key
 * expansion applies to a 128- or 256-bit AES key.
 * This function is primarily intended to be called after
 * key blob has been unwrapped by @ref CAU3_KeyBlobUnwrap to destination key slot, so that the unwrapped key
 * can be used for AES encryption.
 *
 * @param base CAU3 base address
 * @param keySlot is the key context
 * @param taskDone indicates completion signal
 * @return status check from task completion
 */
status_t CAU3_AES_KeyExpansion(CAU3_Type *base, cau3_key_slot_t keySlot, cau3_task_done_t taskDone);

/*!
 *@}
 */ /* end of cau3_driver_aes */

/*******************************************************************************
 * DES API
 ******************************************************************************/
/*!
 * @addtogroup cau3_driver_des
 * @{
 */

/*!
 * @brief   Perform a 3DES key parity check
 *
 * Performs a 3DES key parity check on three 8-byte keys.
 * The function is blocking.
 *
 * @param base CAU3 peripheral base address
 * @param   keySlot defines the key context to be used in the parity check
 *
 * @return  status check from task completion
 */
status_t CAU3_TDES_CheckParity(CAU3_Type *base, cau3_key_slot_t keySlot);

/*!
 * @brief Load DES key into CAU3 key slot.
 *
 * Load the key context into the private DMEM.
 *
 * @param   base CAU3 peripheral base address.
 * @param   handle Handle used for the request.
 * @param   key 0-mod-4 aligned pointer to 3DES key.
 * @param   keySize 3DES key size in bytes. Shall equal 24.
 * @return  status from set key operation
 */
status_t CAU3_TDES_SetKey(CAU3_Type *base, cau3_handle_t *handle, const uint8_t *key, size_t keySize);

/*!
 * @brief   Perform a 3DES encryption
 *
 * Performs a 3DES "electronic code book" encryption on one 8-byte data block.
 * The source plaintext and destination ciphertext can overlap in system memory.
 * Supports both blocking and non-blocking task completion.
 *
 * @param   base CAU3 peripheral base address.
 * @param handle Handle used for this request.
 * @param   plaintext is source uint8_t array of data bytes, any alignment
 * @param   ciphertext is destination uint8_t array of data byte, any alignment
 *
 * @return  status check from task completion
 */
status_t CAU3_TDES_Encrypt(CAU3_Type *base, cau3_handle_t *handle, const uint8_t *plaintext, uint8_t *ciphertext);

/*!
 * @brief   Perform a 3DES decryption
 *
 * Performs a 3DES "electronic code book" decryption on one 8-byte data block.
 * The source ciphertext and destination plaintext can overlap in sysMemory.
 * Supports both blocking and non-blocking task completion.
 *
 * @param   base CAU3 peripheral base address.
 * @param handle Handle used for this request.
 * @param   ciphertext is destination uint8_t array of data byte, any alignment
 * @param   plaintext is source uint8_t array of data bytes, any alignment
 * @return  status check from task completion
 */
status_t CAU3_TDES_Decrypt(CAU3_Type *base, cau3_handle_t *handle, const uint8_t *ciphertext, uint8_t *plaintext);

/*!
 *@}
 */ /* end of cau3_driver_des */

/*******************************************************************************
 * HASH API
 ******************************************************************************/

/*!
 * @addtogroup cau3_driver_hash
 * @{
 */
/*!
 * @brief Initialize HASH context
 *
 * This function initializes the HASH.
 *
 * For blocking CAU3 HASH API, the HASH context contains all information required for context switch,
 * such as running hash.
 *
 * @param base CAU3 peripheral base address
 * @param[out] ctx Output hash context
 * @param algo Underlaying algorithm to use for hash computation.
 * @return Status of initialization
 */
status_t CAU3_HASH_Init(CAU3_Type *base, cau3_hash_ctx_t *ctx, cau3_hash_algo_t algo);

/*!
 * @brief Add data to current HASH
 *
 * Add data to current HASH. This can be called repeatedly with an arbitrary amount of data to be
 * hashed. The functions blocks. If it returns kStatus_Success, the running hash or mac
 * has been updated (CAU3 has processed the input data), so the memory at @ref input pointer
 * can be released back to system. The context is updated with the running hash or mac
 * and with all necessary information to support possible context switch.
 *
 * @param base CAU3 peripheral base address
 * @param[in,out] ctx HASH context
 * @param input Input data
 * @param inputSize Size of input data in bytes
 * @return Status of the hash update operation
 */
status_t CAU3_HASH_Update(CAU3_Type *base, cau3_hash_ctx_t *ctx, const uint8_t *input, size_t inputSize);

/*!
 * @brief Finalize hashing
 *
 * Outputs the final hash (computed by CAU3_HASH_Update()) and erases the context.
 *
 * @param[in,out] ctx Input hash context
 * @param[out] output Output hash data
 * @param[out] outputSize Output parameter storing the size of the output hash in bytes
 * @return Status of the hash finish operation
 */
status_t CAU3_HASH_Finish(CAU3_Type *base, cau3_hash_ctx_t *ctx, uint8_t *output, size_t *outputSize);

/*!
 * @brief Create HASH on given data
 *
 * Perform the full SHA in one function call. The function is blocking.
 *
 * @param base CAU3 peripheral base address
 * @param algo Underlaying algorithm to use for hash computation.
 * @param input Input data
 * @param inputSize Size of input data in bytes
 * @param[out] output Output hash data
 * @param[out] outputSize Output parameter storing the size of the output hash in bytes
 * @return Status of the one call hash operation.
 */
status_t CAU3_HASH(CAU3_Type *base,
                   cau3_hash_algo_t algo,
                   const uint8_t *input,
                   size_t inputSize,
                   uint8_t *output,
                   size_t *outputSize);

/*!
 *@}
 */ /* end of cau3_driver_hash */

/*******************************************************************************
 * AEAD API
 ******************************************************************************/

/*!
 * @addtogroup cau3_driver_chacha_poly
 * @{
 */

/*!
 * @brief Load 256-bit key into CAU3 key context (in key slot).
 *
 * Load the key context into the private DMEM for CHACHA20_POLY1305 AEAD.
 *
 * @param   base CAU3 peripheral base address.
 * @param   handle Handle used for the request.
 * @param   key 0-mod-4 aligned pointer to CHACHA20_POLY1305 256-bit key.
 * @param   keySize Size of the key in bytes. Shall be 32.
 * @return  status from set key operation
 */
status_t CAU3_CHACHA20_POLY1305_SetKey(CAU3_Type *base, cau3_handle_t *handle, const uint8_t *key, size_t keySize);

/*!
 * @brief   Perform ChaCha-Poly encryption/authentication
 *
 * Perform ChaCha encryption over a message of "n" bytes, and authentication
 * over the encrypted data plus an additional authenticated data,
 * returning encrypted data + a message digest.
 *
 * @param base CAU3 peripheral base address
 * @param handle Handle used for this request. The keySlot member specifies key context with key and IV.
 * @param plaintext The uint8_t source message to be encrypted, any alignment
 * @param[out] ciphertext is a pointer to the output encrypted message, any aligment
 * @param size The length of the plaintext and ciphertext in bytes
 * @param aad A pointer to the additional authenticated data, any alignment
 * @param aadLen Length of additional authenticated data in bytes
 * @param nonce 0-mod-4 aligned pointer to CHACHA20_POLY1305 96-bit nonce.
 * @param[out] tag A pointer to the 128-bit message digest output, any alignment
 *
 * @return  status check from task completion
 */
status_t CAU3_CHACHA20_POLY1305_Encrypt(CAU3_Type *base,
                                        cau3_handle_t *handle,
                                        const uint8_t *plaintext,
                                        uint8_t *ciphertext,
                                        size_t size,
                                        const uint8_t *aad,
                                        size_t aadLen,
                                        const uint8_t *nonce,
                                        uint8_t *tag);

/*!
 * @brief   Perform ChaCha-Poly decryption/authentication check
 *
 * Perform ChaCha decryption over a message of "n" bytes, and checks
 * authentication over the encrypted data plus an additional authenticated data,
 * returning decrypted data. IF the tag authentication fails, the task terminates with error and
 * the output is forced to zero.
 *
 * @param base CAU3 peripheral base address
 * @param handle Handle used for this request. The keySlot member specifies key context with key and IV.
 * @param ciphertext The uint8_t source msg to be decrypted, any alignment
 * @param[out] plaintext A pointer to the output decrypted message, any alignment
 * @param size Length of the plaintext and ciphertext in bytes
 * @param aad A pointer to the additional authenticated data, any alignment
 * @param aadLen Length of additional authenticated data in bytes
 * @param nonce 0-mod-4 aligned pointer to CHACHA20_POLY1305 96-bit nonce.
 * @param tag A pointer to the 128-bit msg digest input to be checked, any alignment
 *
 * @return  status check from task completion
 *
 */
status_t CAU3_CHACHA20_POLY1305_Decrypt(CAU3_Type *base,
                                        cau3_handle_t *handle,
                                        const uint8_t *ciphertext,
                                        uint8_t *plaintext,
                                        size_t size,
                                        const uint8_t *aad,
                                        size_t aadLen,
                                        const uint8_t *nonce,
                                        const uint8_t *tag);

/*!
 *@}
 */ /* end of cau3_driver_chacha_poly */

/*******************************************************************************
 * BLOB API
 ******************************************************************************/

/*!
 * @addtogroup cau3_driver_blob
 * @{
 */

/*!
 * @brief   Perform an RFC3394 key blob unwrap
 *
 * Perform an RFC3394 unwrap of an AES encrypted key blob. The unwrapped
 * key blob is loaded into the specified key slot [1-3]. The initial
 * special hardware KEK contained in key slot 0 is typically used for the
 * unwrapping operation. The destination context number must be different than
 * the keySlot used for unwrapping.
 * Implements the algorithm at RFC 3394 to AES key unwrap. The
 * current implementation allows to unwrap up to 512 bits,
 * with the restriction of nblocks=2 or =4 or n=8(means it
 * unwraps only 128bits, 256bits or two 256 bits keys (512)). It
 * is allowed input key of 128 and 256bits only (passed using
 * the keyslot). The function also assumes the
 * @ref CAU3_LoadSpecialKeyContext was called before.
 * It returns error and clear the destination context in case
 * parameters are not inside aceptable values.
 * In case n>4 && n!=8 it clears both destination contexts (the
 * dstContext and the adjacent/next context)
 * In case of n=8, the first unwraped key will be stored in the
 * dstContext slot, and the second key will be saved in the next
 * context (E.g: if dstContext=1, then first key goes to slot 1
 * and second key to slot 2. If dstContext=3 then first key goes
 * to slot 3 and second key goes to slot 1).
 * Examples of n usage.
 * E.g.: n = 2 means a unwraped key of 128 bits (2 * 64)
 * E.g.: n = 4 means a unwraped key of 256 bits (4 * 64)
 * E.g.: n = 8 means two unwraped keys of 256 bits (8 * 64)
 *
 * The function is blocking, it uses the polling task done signaling.
 *
 * @param base CAU3 peripheral base address
 * @param   keySlot is the key used to unwrap the key blob [0-3]
 * @param   keyBlob 0-mod-4 aligned pointer is the RFC3394 wrapped key blob.
 * @param   numberOfBlocks is the unwrapped keyBlob length as multiple of 64-bit blocks
 * @param   dstContext is the destination key context for unwrapped blob [0-3]
 * @retval  status check from task completion
 */
status_t CAU3_KeyBlobUnwrap(CAU3_Type *base,
                            cau3_key_slot_t keySlot,
                            const uint8_t *keyBlob,
                            uint32_t numberOfBlocks,
                            cau3_key_slot_t dstContext);

/*!
 *@}
 */ /* end of cau3_driver_blob */

/*******************************************************************************
 * PKHA API
 ******************************************************************************/

/*!
 * @addtogroup cau3_driver_pkha
 * @{
 */

int CAU3_PKHA_CompareBigNum(const uint8_t *a, size_t sizeA, const uint8_t *b, size_t sizeB);

/*!
 * @brief Converts from integer to Montgomery format.
 *
 * This function computes R2 mod N and optionally converts A or B into Montgomery format of A or B.
 *
 * @param base CAU3 peripheral base address
 * @param N modulus
 * @param sizeN size of N in bytes
 * @param[in,out] A The first input in non-Montgomery format. Output Montgomery format of the first input.
 * @param[in,out] sizeA pointer to size variable. On input it holds size of input A in bytes. On output it holds size of
 *                Montgomery format of A in bytes.
 * @param[in,out] B Second input in non-Montgomery format. Output Montgomery format of the second input.
 * @param[in,out] sizeB pointer to size variable. On input it holds size of input B in bytes. On output it holds size of
 *                Montgomery format of B in bytes.
 * @param[out] R2 Output Montgomery factor R2 mod N.
 * @param[out] sizeR2 pointer to size variable. On output it holds size of Montgomery factor R2 mod N in bytes.
 * @param equalTime Run the function time equalized or no timing equalization.
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @return Operation status.
 */
status_t CAU3_PKHA_NormalToMontgomery(CAU3_Type *base,
                                      const uint8_t *N,
                                      size_t sizeN,
                                      uint8_t *A,
                                      size_t *sizeA,
                                      uint8_t *B,
                                      size_t *sizeB,
                                      uint8_t *R2,
                                      size_t *sizeR2,
                                      cau3_pkha_timing_t equalTime,
                                      cau3_pkha_f2m_t arithType);

/*!
 * @brief Converts from Montgomery format to int.
 *
 * This function converts Montgomery format of A or B into int A or B.
 *
 * @param base CAU3 peripheral base address
 * @param N modulus.
 * @param sizeN size of N modulus in bytes.
 * @param[in,out] A Input first number in Montgomery format. Output is non-Montgomery format.
 * @param[in,out] sizeA pointer to size variable. On input it holds size of the input A in bytes. On output it holds
 * size of non-Montgomery A in bytes.
 * @param[in,out] B Input first number in Montgomery format. Output is non-Montgomery format.
 * @param[in,out] sizeB pointer to size variable. On input it holds size of the input B in bytes. On output it holds
 * size of non-Montgomery B in bytes.
 * @param equalTime Run the function time equalized or no timing equalization.
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @return Operation status.
 */
status_t CAU3_PKHA_MontgomeryToNormal(CAU3_Type *base,
                                      const uint8_t *N,
                                      size_t sizeN,
                                      uint8_t *A,
                                      size_t *sizeA,
                                      uint8_t *B,
                                      size_t *sizeB,
                                      cau3_pkha_timing_t equalTime,
                                      cau3_pkha_f2m_t arithType);

/*!
 * @brief Performs modular addition - (A + B) mod N.
 *
 * This function performs modular addition of (A + B) mod N, with either
 * integer or binary polynomial (F2m) inputs.  In the F2m form, this function is
 * equivalent to a bitwise XOR and it is functionally the same as subtraction.
 *
 * @param base CAU3 peripheral base address
 * @param A first addend (integer or binary polynomial)
 * @param sizeA Size of A in bytes
 * @param B second addend (integer or binary polynomial)
 * @param sizeB Size of B in bytes
 * @param N modulus.
 * @param sizeN Size of N in bytes.
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @return Operation status.
 */
status_t CAU3_PKHA_ModAdd(CAU3_Type *base,
                          const uint8_t *A,
                          size_t sizeA,
                          const uint8_t *B,
                          size_t sizeB,
                          const uint8_t *N,
                          size_t sizeN,
                          uint8_t *result,
                          size_t *resultSize,
                          cau3_pkha_f2m_t arithType);

/*!
 * @brief Performs modular subtraction - (A - B) mod N.
 *
 * This function performs modular subtraction of (A - B) mod N with
 * integer inputs.
 *
 * @param base CAU3 peripheral base address
 * @param A first addend (integer or binary polynomial)
 * @param sizeA Size of A in bytes
 * @param B second addend (integer or binary polynomial)
 * @param sizeB Size of B in bytes
 * @param N modulus
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @return Operation status.
 */
status_t CAU3_PKHA_ModSub1(CAU3_Type *base,
                           const uint8_t *A,
                           size_t sizeA,
                           const uint8_t *B,
                           size_t sizeB,
                           const uint8_t *N,
                           size_t sizeN,
                           uint8_t *result,
                           size_t *resultSize);

/*!
 * @brief Performs modular subtraction - (B - A) mod N.
 *
 * This function performs modular subtraction of (B - A) mod N,
 * with integer inputs.
 *
 * @param base CAU3 peripheral base address
 * @param A first addend (integer or binary polynomial)
 * @param sizeA Size of A in bytes
 * @param B second addend (integer or binary polynomial)
 * @param sizeB Size of B in bytes
 * @param N modulus
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @return Operation status.
 */
status_t CAU3_PKHA_ModSub2(CAU3_Type *base,
                           const uint8_t *A,
                           size_t sizeA,
                           const uint8_t *B,
                           size_t sizeB,
                           const uint8_t *N,
                           size_t sizeN,
                           uint8_t *result,
                           size_t *resultSize);

/*!
 * @brief Performs modular multiplication - (A x B) mod N.
 *
 * This function performs modular multiplication with either integer or
 * binary polynomial (F2m) inputs.  It can optionally specify whether inputs
 * and/or outputs will be in Montgomery form or not.
 *
 * @param base CAU3 peripheral base address
 * @param A first addend (integer or binary polynomial)
 * @param sizeA Size of A in bytes
 * @param B second addend (integer or binary polynomial)
 * @param sizeB Size of B in bytes
 * @param N modulus.
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @param montIn Format of inputs
 * @param montOut Format of output
 * @param equalTime Run the function time equalized or no timing equalization. This argument is ignored for F2m modular
 * multiplication.
 * @return Operation status.
 */
status_t CAU3_PKHA_ModMul(CAU3_Type *base,
                          const uint8_t *A,
                          size_t sizeA,
                          const uint8_t *B,
                          size_t sizeB,
                          const uint8_t *N,
                          size_t sizeN,
                          uint8_t *result,
                          size_t *resultSize,
                          cau3_pkha_f2m_t arithType,
                          cau3_pkha_montgomery_form_t montIn,
                          cau3_pkha_montgomery_form_t montOut,
                          cau3_pkha_timing_t equalTime);

/*!
 * @brief Performs modular exponentiation - (A^E) mod N.
 *
 * This function performs modular exponentiation with either integer or
 * binary polynomial (F2m) inputs.
 *
 * @param base CAU3 peripheral base address
 * @param A first addend (integer or binary polynomial)
 * @param sizeA Size of A in bytes
 * @param N modulus
 * @param sizeN Size of N in bytes
 * @param E exponent
 * @param sizeE Size of E in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @param montIn Format of A input (normal or Montgomery)
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @param equalTime Run the function time equalized or no timing equalization.
 * @return Operation status.
 */
status_t CAU3_PKHA_ModExp(CAU3_Type *base,
                          const uint8_t *A,
                          size_t sizeA,
                          const uint8_t *N,
                          size_t sizeN,
                          const uint8_t *E,
                          size_t sizeE,
                          uint8_t *result,
                          size_t *resultSize,
                          cau3_pkha_f2m_t arithType,
                          cau3_pkha_montgomery_form_t montIn,
                          cau3_pkha_timing_t equalTime);

/*!
 * @brief Performs Modular Square Root.
 *
 * This function performs modular square root with integer inputs.
 * The modular square root function computes output result B, such that ( B x B ) mod N = input A.
 * If no such B result exists, the result will be set to 0 and the PKHA "prime" flag
 * will be set. Input values A and B are limited to a maximum size of 128 bytes. Note that
 * two such square root values may exist. This algorithm will find either one of them, if any
 * exist. The second possible square root (B') can be found by calculating B' = N - B.
 *
 * @param base CAU3 peripheral base address
 * @param A input value, for which a square root is to be calculated
 * @param sizeA Size of A in bytes
 * @param N modulus
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @return Operation status.
 */
status_t CAU3_PKHA_ModSqrt(CAU3_Type *base,
                           const uint8_t *A,
                           size_t sizeA,
                           const uint8_t *N,
                           size_t sizeN,
                           uint8_t *result,
                           size_t *resultSize);

/*!
 * @brief Performs modular reduction - (A) mod N.
 *
 * This function performs modular reduction with either integer or
 * binary polynomial (F2m) inputs.
 *
 * @param base CAU3 peripheral base address
 * @param A first addend (integer or binary polynomial)
 * @param sizeA Size of A in bytes
 * @param N modulus
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @return Operation status.
 */
status_t CAU3_PKHA_ModRed(CAU3_Type *base,
                          const uint8_t *A,
                          size_t sizeA,
                          const uint8_t *N,
                          size_t sizeN,
                          uint8_t *result,
                          size_t *resultSize,
                          cau3_pkha_f2m_t arithType);

/*!
 * @brief Performs modular inversion - (A^-1) mod N.
 *
 * This function performs modular inversion with either integer or
 * binary polynomial (F2m) inputs.
 *
 * @param base CAU3 peripheral base address
 * @param A first addend (integer or binary polynomial)
 * @param sizeA Size of A in bytes
 * @param N modulus
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @return Operation status.
 */
status_t CAU3_PKHA_ModInv(CAU3_Type *base,
                          const uint8_t *A,
                          size_t sizeA,
                          const uint8_t *N,
                          size_t sizeN,
                          uint8_t *result,
                          size_t *resultSize,
                          cau3_pkha_f2m_t arithType);

/*!
 * @brief Computes integer Montgomery factor R^2 mod N.
 *
 * This function computes a constant to assist in converting operands
 * into the Montgomery residue system representation.
 *
 * @param base CAU3 peripheral base address
 * @param N modulus
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @return Operation status.
 */
status_t CAU3_PKHA_ModR2(
    CAU3_Type *base, const uint8_t *N, size_t sizeN, uint8_t *result, size_t *resultSize, cau3_pkha_f2m_t arithType);

/*!
 * @brief Performs Integer RERP mod P.
 *
 * This function is used to compute a constant to assist in converting operands into the
 * Montgomery residue system representation specifically for Chinese Remainder Theorem
 * while performing RSA with a CRT implementation where a modulus E=P x Q, and P and
 * Q are prime numbers. Although labeled RERP mod P, this routine (function) can also
 * compute RERQ mod Q.
 *
 * @param base CAU3 peripheral base address
 * @param P modulus P or Q of CRT, an odd integer
 * @param sizeP Size of P in bytes
 * @param sizeE Number of bytes of E = P x Q (this size must be given, though content of E itself is not used).
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @return Operation status.
 */
status_t CAU3_PKHA_ModRR(
    CAU3_Type *base, const uint8_t *P, size_t sizeP, size_t sizeE, uint8_t *result, size_t *resultSize);

/*!
 * @brief Calculates the greatest common divisor - GCD (A, N).
 *
 * This function calculates the greatest common divisor of two inputs with
 * either integer or binary polynomial (F2m) inputs.
 *
 * @param base CAU3 peripheral base address
 * @param A first value (must be smaller than or equal to N)
 * @param sizeA Size of A in bytes
 * @param N second value (must be non-zero)
 * @param sizeN Size of N in bytes
 * @param[out] result Output array to store result of operation
 * @param[out] resultSize Output size of operation in bytes
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @return Operation status.
 */
status_t CAU3_PKHA_ModGcd(CAU3_Type *base,
                          const uint8_t *A,
                          size_t sizeA,
                          const uint8_t *N,
                          size_t sizeN,
                          uint8_t *result,
                          size_t *resultSize,
                          cau3_pkha_f2m_t arithType);

/*!
 * @brief Executes Miller-Rabin primality test.
 *
 * This function calculates whether or not a candidate prime number is likely
 * to be a prime.
 *
 * @param base CAU3 peripheral base address
 * @param A initial random seed
 * @param sizeA Size of A in bytes
 * @param B number of trial runs
 * @param sizeB Size of B in bytes
 * @param N candidate prime integer
 * @param sizeN Size of N in bytes
 * @param[out] res True if the value is likely prime or false otherwise
 * @return Operation status.
 */
status_t CAU3_PKHA_PrimalityTest(CAU3_Type *base,
                                 const uint8_t *A,
                                 size_t sizeA,
                                 const uint8_t *B,
                                 size_t sizeB,
                                 const uint8_t *N,
                                 size_t sizeN,
                                 bool *res);

/*!
 * @brief Adds elliptic curve points - A + B.
 *
 * This function performs ECC point addition over a prime field (Fp) or binary field (F2m) using
 * affine coordinates.
 *
 * @param base CAU3 peripheral base address
 * @param A Left-hand point
 * @param B Right-hand point
 * @param N Prime modulus of the field
 * @param R2modN NULL (the function computes R2modN internally) or pointer to pre-computed R2modN (obtained from
 *               CAU3_PKHA_ModR2() function).
 * @param aCurveParam A parameter from curve equation
 * @param bCurveParam B parameter from curve equation (constant)
 * @param size Size in bytes of curve points and parameters
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @param[out] result Result point
 * @return Operation status.
 */
status_t CAU3_PKHA_ECC_PointAdd(CAU3_Type *base,
                                const cau3_pkha_ecc_point_t *A,
                                const cau3_pkha_ecc_point_t *B,
                                const uint8_t *N,
                                const uint8_t *R2modN,
                                const uint8_t *aCurveParam,
                                const uint8_t *bCurveParam,
                                size_t size,
                                cau3_pkha_f2m_t arithType,
                                cau3_pkha_ecc_point_t *result);

/*!
 * @brief Doubles elliptic curve points - B + B.
 *
 * This function performs ECC point doubling over a prime field (Fp) or binary field (F2m) using
 * affine coordinates.
 *
 * @param base CAU3 peripheral base address
 * @param B Point to double
 * @param N Prime modulus of the field
 * @param aCurveParam A parameter from curve equation
 * @param bCurveParam B parameter from curve equation (constant)
 * @param size Size in bytes of curve points and parameters
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @param[out] result Result point
 * @return Operation status.
 */
status_t CAU3_PKHA_ECC_PointDouble(CAU3_Type *base,
                                   const cau3_pkha_ecc_point_t *B,
                                   const uint8_t *N,
                                   const uint8_t *aCurveParam,
                                   const uint8_t *bCurveParam,
                                   size_t size,
                                   cau3_pkha_f2m_t arithType,
                                   cau3_pkha_ecc_point_t *result);

/*!
 * @brief Multiplies an elliptic curve point by a scalar - E x (A0, A1).
 *
 * This function performs ECC point multiplication to multiply an ECC point by
 * a scalar integer multiplier over a prime field (Fp) or a binary field (F2m).
 *
 * @param base CAU3 peripheral base address
 * @param A Point as multiplicand
 * @param E Scalar multiple
 * @param sizeE The size of E, in bytes
 * @param N Modulus, a prime number for the Fp field or Irreducible polynomial for F2m field.
 * @param R2modN NULL (the function computes R2modN internally) or pointer to pre-computed R2modN (obtained from
 *        CAU3_PKHA_ModR2() function).
 * @param aCurveParam A parameter from curve equation
 * @param bCurveParam B parameter from curve equation (C parameter for operation over F2m).
 * @param size Size in bytes of curve points and parameters
 * @param equalTime Run the function time equalized or no timing equalization.
 * @param arithType Type of arithmetic to perform (integer or F2m)
 * @param[out] result Result point
 * @return Operation status.
 */
status_t CAU3_PKHA_ECC_PointMul(CAU3_Type *base,
                                const cau3_pkha_ecc_point_t *A,
                                const uint8_t *E,
                                size_t sizeE,
                                const uint8_t *N,
                                const uint8_t *R2modN,
                                const uint8_t *aCurveParam,
                                const uint8_t *bCurveParam,
                                size_t size,
                                cau3_pkha_timing_t equalTime,
                                cau3_pkha_f2m_t arithType,
                                cau3_pkha_ecc_point_t *result);

/*!
 * @brief Computes scalar multiplication of a point on an elliptic curve in Montgomery form.
 *
 * This function computes the scalar multiplication of a point on an elliptic curve in
 * Montgomery form. The input and output are just the x coordinates of the points.
 * The points on a curve are defined by the equation E: B*y^2 = x^3 + A*x^2 + x mod p
 * This function computes a point multiplication on a Montgomery curve, using
 * Montgomery values, by means of a Montgomery ladder. At the end of the ladder, P2 = P3 + P1,
 * where P1 is the input and P3 is the result.
 *
 * @param base CAU3 peripheral base address
 * @param E Scalar multiplier, any integer
 * @param sizeE The size of E, in bytes
 * @param inputCoordinate Point as multiplicand, an input point's affine x coordinate
 * @param A24 elliptic curve a24 parameter, that is, (A+2)/4
 * @param N Modulus, a prime number.
 * @param R2modN NULL (the function computes R2modN internally) or pointer to pre-computed R2modN (obtained from
 *        @ref CAU3_PKHA_ModR2() function).
 * @param size Size in bytes of curve points and parameters
 * @param equalTime Run the function time equalized or no timing equalization.
 * @param[out] outputCoordinate Resulting poin's x affine coordinate.
 * @return Operation status.
 */
status_t CAU3_PKHA_ECM_PointMul(CAU3_Type *base,
                                const uint8_t *E,
                                size_t sizeE,
                                const uint8_t *inputCoordinate,
                                const uint8_t *A24,
                                const uint8_t *N,
                                const uint8_t *R2modN,
                                size_t size,
                                cau3_pkha_timing_t equalTime,
                                uint8_t *outputCoordinate);

/*!
 * @brief Multiplies an Edwards-form elliptic curve point by a scalar - E x (A0, A1).
 *
 * This function performs scalar multiplication of an Edwards-form elliptic curve point
 * in affine coordinates.
 * The points on a curve are defined by the equation E: a*X^2 + d^2 = 1 + D^2*X^2*Y^2 mod N
 *
 * @param base CAU3 peripheral base address
 * @param A Point as multiplicand
 * @param E Scalar multiple
 * @param sizeE The size of E, in bytes
 * @param N Modulus, a prime number for the Fp field.
 * @param R2modN NULL (the function computes R2modN internally) or pointer to pre-computed R2modN (obtained from
 *        @ref CAU3_PKHA_ModR2() function).
 * @param aCurveParam A parameter from curve equation
 * @param dCurveParam D parameter from curve equation.
 * @param size Size in bytes of curve points and parameters
 * @param equalTime Run the function time equalized or no timing equalization.
 * @param[out] result Result point
 * @return Operation status.
 */
status_t CAU3_PKHA_ECT_PointMul(CAU3_Type *base,
                                const cau3_pkha_ecc_point_t *A,
                                const uint8_t *E,
                                size_t sizeE,
                                const uint8_t *N,
                                const uint8_t *R2modN,
                                const uint8_t *aCurveParam,
                                const uint8_t *dCurveParam,
                                size_t size,
                                cau3_pkha_timing_t equalTime,
                                cau3_pkha_ecc_point_t *result);

/*!
 * @brief Adds an Edwards-form elliptic curve points - A + B.
 *
 * This function performs Edwards-form elliptic curve point addition over a prime field (Fp) using affine coordinates.
 * The points on a curve are defined by the equation E: a*X^2 + Y^2 = 1 + d^2*X^2*Y^2 mod N
 *
 * @param base CAU3 peripheral base address
 * @param A Left-hand point
 * @param B Right-hand point
 * @param N Prime modulus of the field
 * @param R2modN NULL (the function computes R2modN internally) or pointer to pre-computed R2modN (obtained from
 *               @ref CAU3_PKHA_ModR2() function).
 * @param aCurveParam A parameter from curve equation
 * @param dCurveParam D parameter from curve equation
 * @param size Size in bytes of curve points and parameters
 * @param[out] result Result point
 * @return Operation status.
 */
status_t CAU3_PKHA_ECT_PointAdd(CAU3_Type *base,
                                const cau3_pkha_ecc_point_t *A,
                                const cau3_pkha_ecc_point_t *B,
                                const uint8_t *N,
                                const uint8_t *R2modN,
                                const uint8_t *aCurveParam,
                                const uint8_t *dCurveParam,
                                size_t size,
                                cau3_pkha_ecc_point_t *result);

/*!
 *@}
 */ /* end of cau3_driver_pkha */

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_CAU3_H_ */
