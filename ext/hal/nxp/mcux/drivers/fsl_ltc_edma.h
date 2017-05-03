/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_LTC_EDMA_H_
#define _FSL_LTC_EDMA_H_

#include "fsl_common.h"

#include "fsl_ltc.h"
#include "fsl_dmamux.h"
#include "fsl_edma.h"

/*!
 * @addtogroup ltc_edma_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* @brief The LTC eDMA handle type. */
typedef struct _ltc_edma_handle ltc_edma_handle_t;

/*! @brief LTC eDMA callback function. */
typedef void (*ltc_edma_callback_t)(LTC_Type *base, ltc_edma_handle_t *handle, status_t status, void *userData);

/*! @brief LTC eDMA state machine function. It is defined only for private usage inside LTC eDMA driver. */
typedef status_t (*ltc_edma_state_machine_t)(LTC_Type *base, ltc_edma_handle_t *handle);

/*!
* @brief LTC eDMA handle. It is defined only for private usage inside LTC eDMA driver.
*/
struct _ltc_edma_handle
{
    ltc_edma_callback_t callback; /*!< Callback function. */
    void *userData;               /*!< LTC callback function parameter.*/

    edma_handle_t *inputFifoEdmaHandle;  /*!< The eDMA TX channel used. */
    edma_handle_t *outputFifoEdmaHandle; /*!< The eDMA RX channel used. */

    ltc_edma_state_machine_t state_machine; /*!< State machine. */
    uint32_t state;                         /*!< Internal state. */
    const uint8_t *inData;                  /*!< Input data. */
    uint8_t *outData;                       /*!< Output data. */
    uint32_t size;                          /*!< Size of input and output data in bytes.*/
    uint32_t modeReg;                       /*!< LTC mode register.*/
    /* Used by AES CTR*/
    uint8_t *counter;   /*!< Input counter (updates on return)*/
    const uint8_t *key; /*!< Input key to use for forward AES cipher*/
    uint32_t keySize;   /*!< Size of the input key, in bytes. Must be 16, 24, or 32.*/
    uint8_t
        *counterlast; /*!< Output cipher of last counter, for chained CTR calls. NULL can be passed if chained calls are
                         not used.*/
    uint32_t *szLeft; /*!< Output number of bytes in left unused in counterlast block. NULL can be passed if chained
                         calls are not used.*/
    uint32_t lastSize; /*!< Last size.*/
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Init the LTC eDMA handle which is used in transcational functions
 * @param base      LTC module base address
 * @param handle    Pointer to ltc_edma_handle_t structure
 * @param callback  Callback function, NULL means no callback.
 * @param userData  Callback function parameter.
 * @param inputFifoEdmaHandle User requested eDMA handle for Input FIFO eDMA.
 * @param outputFifoEdmaHandle User requested eDMA handle for Output FIFO eDMA.
 */
void LTC_CreateHandleEDMA(LTC_Type *base,
                          ltc_edma_handle_t *handle,
                          ltc_edma_callback_t callback,
                          void *userData,
                          edma_handle_t *inputFifoEdmaHandle,
                          edma_handle_t *outputFifoEdmaHandle);

/*! @}*/

/*******************************************************************************
 * AES API
 ******************************************************************************/

/*!
 * @addtogroup ltc_edma_driver_aes
 * @{
 */

/*!
 * @brief Encrypts AES using the ECB block mode.
 *
 * Encrypts AES using the ECB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param key Input key to use for encryption
 * @param keySize Size of the input key, in bytes. Must be 16, 24, or 32.
 * @return Status from encrypt operation
 */
status_t LTC_AES_EncryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t *key,
                                uint32_t keySize);

/*!
 * @brief Decrypts AES using ECB block mode.
 *
 * Decrypts AES using ECB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input cipher text to decrypt
 * @param[out] plaintext Output plain text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param key Input key.
 * @param keySize Size of the input key, in bytes. Must be 16, 24, or 32.
 * @param keyType Input type of the key (allows to directly load decrypt key for AES ECB decrypt operation.)
 * @return Status from decrypt operation
 */
status_t LTC_AES_DecryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t *key,
                                uint32_t keySize,
                                ltc_aes_key_t keyType);

/*!
 * @brief Encrypts AES using CBC block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plain text to encrypt
 * @param[out] ciphertext Output cipher text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input initial vector to combine with the first input block.
 * @param key Input key to use for encryption
 * @param keySize Size of the input key, in bytes. Must be 16, 24, or 32.
 * @return Status from encrypt operation
 */
status_t LTC_AES_EncryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_AES_IV_SIZE],
                                const uint8_t *key,
                                uint32_t keySize);

/*!
 * @brief Decrypts AES using CBC block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input cipher text to decrypt
 * @param[out] plaintext Output plain text
 * @param size Size of input and output data in bytes. Must be multiple of 16 bytes.
 * @param iv Input initial vector to combine with the first input block.
 * @param key Input key to use for decryption
 * @param keySize Size of the input key, in bytes. Must be 16, 24, or 32.
 * @param keyType Input type of the key (allows to directly load decrypt key for AES CBC decrypt operation.)
 * @return Status from decrypt operation
 */
status_t LTC_AES_DecryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_AES_IV_SIZE],
                                const uint8_t *key,
                                uint32_t keySize,
                                ltc_aes_key_t keyType);

/*!
 * @brief Encrypts or decrypts AES using CTR block mode.
 *
 * Encrypts or decrypts AES using CTR block mode.
 * AES CTR mode uses only forward AES cipher and same algorithm for encryption and decryption.
 * The only difference between encryption and decryption is that, for encryption, the input argument
 * is plain text and the output argument is cipher text. For decryption, the input argument is cipher text
 * and the output argument is plain text.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param input Input data for CTR block mode
 * @param[out] output Output data for CTR block mode
 * @param size Size of input and output data in bytes
 * @param[in,out] counter Input counter (updates on return)
 * @param key Input key to use for forward AES cipher
 * @param keySize Size of the input key, in bytes. Must be 16, 24, or 32.
 * @param[out] counterlast Output cipher of last counter, for chained CTR calls. NULL can be passed if chained calls are
 * not used.
 * @param[out] szLeft Output number of bytes in left unused in counterlast block. NULL can be passed if chained calls
 * are not used.
 * @return Status from encrypt operation
 */
status_t LTC_AES_CryptCtrEDMA(LTC_Type *base,
                              ltc_edma_handle_t *handle,
                              const uint8_t *input,
                              uint8_t *output,
                              uint32_t size,
                              uint8_t counter[LTC_AES_BLOCK_SIZE],
                              const uint8_t *key,
                              uint32_t keySize,
                              uint8_t counterlast[LTC_AES_BLOCK_SIZE],
                              uint32_t *szLeft);

/*! AES CTR decrypt is mapped to the AES CTR generic operation */
#define LTC_AES_DecryptCtrEDMA(base, handle, input, output, size, counter, key, keySize, counterlast, szLeft) \
    LTC_AES_CryptCtrEDMA(base, handle, input, output, size, counter, key, keySize, counterlast, szLeft)

/*! AES CTR encrypt is mapped to the AES CTR generic operation */
#define LTC_AES_EncryptCtrEDMA(base, handle, input, output, size, counter, key, keySize, counterlast, szLeft) \
    LTC_AES_CryptCtrEDMA(base, handle, input, output, size, counter, key, keySize, counterlast, szLeft)

/*!
 *@}
 */

/*******************************************************************************
 * DES API
 ******************************************************************************/
/*!
 * @addtogroup ltc_edma_driver_des
 * @{
 */
/*!
 * @brief Encrypts DES using ECB block mode.
 *
 * Encrypts DES using ECB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes. Must be multiple of 8 bytes.
 * @param key Input key to use for encryption
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_EncryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts DES using ECB block mode.
 *
 * Decrypts DES using ECB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes. Must be multiple of 8 bytes.
 * @param key Input key to use for decryption
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_DecryptEcbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts DES using CBC block mode.
 *
 * Encrypts DES using CBC block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Ouput ciphertext
 * @param size Size of input and output data in bytes
 * @param iv Input initial vector to combine with the first plaintext block.
 *           The iv does not need to be secret, but it must be unpredictable.
 * @param key Input key to use for encryption
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_EncryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts DES using CBC block mode.
 *
 * Decrypts DES using CBC block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input data in bytes
 * @param iv Input initial vector to combine with the first plaintext block.
 *           The iv does not need to be secret, but it must be unpredictable.
 * @param key Input key to use for decryption
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_DecryptCbcEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts DES using CFB block mode.
 *
 * Encrypts DES using CFB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param size Size of input data in bytes
 * @param iv Input initial block.
 * @param key Input key to use for encryption
 * @param[out] ciphertext Output ciphertext
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_EncryptCfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts DES using CFB block mode.
 *
 * Decrypts DES using CFB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes
 * @param iv Input initial block.
 * @param key Input key to use for decryption
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_DecryptCfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts DES using OFB block mode.
 *
 * Encrypts DES using OFB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes
 * @param iv Input unique input vector. The OFB mode requires that the IV be unique
 *           for each execution of the mode under the given key.
 * @param key Input key to use for encryption
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_EncryptOfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *plaintext,
                                uint8_t *ciphertext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts DES using OFB block mode.
 *
 * Decrypts DES using OFB block mode.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes. Must be multiple of 8 bytes.
 * @param iv Input unique input vector. The OFB mode requires that the IV be unique
 *           for each execution of the mode under the given key.
 * @param key Input key to use for decryption
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES_DecryptOfbEDMA(LTC_Type *base,
                                ltc_edma_handle_t *handle,
                                const uint8_t *ciphertext,
                                uint8_t *plaintext,
                                uint32_t size,
                                const uint8_t iv[LTC_DES_IV_SIZE],
                                const uint8_t key[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using ECB block mode with two keys.
 *
 * Encrypts triple DES using ECB block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes. Must be multiple of 8 bytes.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_EncryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using ECB block mode with two keys.
 *
 * Decrypts triple DES using ECB block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes. Must be multiple of 8 bytes.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_DecryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using CBC block mode with two keys.
 *
 * Encrypts triple DES using CBC block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes
 * @param iv Input initial vector to combine with the first plaintext block.
 *           The iv does not need to be secret, but it must be unpredictable.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_EncryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using CBC block mode with two keys.
 *
 * Decrypts triple DES using CBC block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes
 * @param iv Input initial vector to combine with the first plaintext block.
 *           The iv does not need to be secret, but it must be unpredictable.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_DecryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using CFB block mode with two keys.
 *
 * Encrypts triple DES using CFB block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes
 * @param iv Input initial block.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_EncryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using CFB block mode with two keys.
 *
 * Decrypts triple DES using CFB block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes
 * @param iv Input initial block.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_DecryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using OFB block mode with two keys.
 *
 * Encrypts triple DES using OFB block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes
 * @param iv Input unique input vector. The OFB mode requires that the IV be unique
 *           for each execution of the mode under the given key.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_EncryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using OFB block mode with two keys.
 *
 * Decrypts triple DES using OFB block mode with two keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes
 * @param iv Input unique input vector. The OFB mode requires that the IV be unique
 *           for each execution of the mode under the given key.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES2_DecryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using ECB block mode with three keys.
 *
 * Encrypts triple DES using ECB block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes. Must be multiple of 8 bytes.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_EncryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using ECB block mode with three keys.
 *
 * Decrypts triple DES using ECB block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes. Must be multiple of 8 bytes.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_DecryptEcbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using CBC block mode with three keys.
 *
 * Encrypts triple DES using CBC block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input data in bytes
 * @param iv Input initial vector to combine with the first plaintext block.
 *           The iv does not need to be secret, but it must be unpredictable.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_EncryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using CBC block mode with three keys.
 *
 * Decrypts triple DES using CBC block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes
 * @param iv Input initial vector to combine with the first plaintext block.
 *           The iv does not need to be secret, but it must be unpredictable.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_DecryptCbcEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using CFB block mode with three keys.
 *
 * Encrypts triple DES using CFB block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and ouput data in bytes
 * @param iv Input initial block.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_EncryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using CFB block mode with three keys.
 *
 * Decrypts triple DES using CFB block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input data in bytes
 * @param iv Input initial block.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_DecryptCfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 * @brief Encrypts triple DES using OFB block mode with three keys.
 *
 * Encrypts triple DES using OFB block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param plaintext Input plaintext to encrypt
 * @param[out] ciphertext Output ciphertext
 * @param size Size of input and output data in bytes
 * @param iv Input unique input vector. The OFB mode requires that the IV be unique
 *           for each execution of the mode under the given key.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_EncryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *plaintext,
                                 uint8_t *ciphertext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 * @brief Decrypts triple DES using OFB block mode with three keys.
 *
 * Decrypts triple DES using OFB block mode with three keys.
 *
 * @param base LTC peripheral base address
 * @param handle pointer to ltc_edma_handle_t structure which stores the transaction state.
 * @param ciphertext Input ciphertext to decrypt
 * @param[out] plaintext Output plaintext
 * @param size Size of input and output data in bytes
 * @param iv Input unique input vector. The OFB mode requires that the IV be unique
 *           for each execution of the mode under the given key.
 * @param key1 First input key for key bundle
 * @param key2 Second input key for key bundle
 * @param key3 Third input key for key bundle
 * @return Status from encrypt/decrypt operation
 */
status_t LTC_DES3_DecryptOfbEDMA(LTC_Type *base,
                                 ltc_edma_handle_t *handle,
                                 const uint8_t *ciphertext,
                                 uint8_t *plaintext,
                                 uint32_t size,
                                 const uint8_t iv[LTC_DES_IV_SIZE],
                                 const uint8_t key1[LTC_DES_KEY_SIZE],
                                 const uint8_t key2[LTC_DES_KEY_SIZE],
                                 const uint8_t key3[LTC_DES_KEY_SIZE]);

/*!
 *@}
 */

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_LTC_EDMA_H_ */
