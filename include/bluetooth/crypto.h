/** @file
 *  @brief Bluetooth subsystem crypto APIs.
 */

/*
 * Copyright (c) 2017-2020 Nordic Semiconductor ASA
 * Copyright (c) 2015-2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CRYPTO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CRYPTO_H_

/**
 * @brief Cryptography
 * @defgroup bt_crypto Cryptography
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Generate random data.
 *
 *  A random number generation helper which utilizes the Bluetooth
 *  controller's own RNG.
 *
 *  @param buf Buffer to insert the random data
 *  @param len Length of random data to generate
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_rand(void *buf, size_t len);

/** @brief AES encrypt little-endian data.
 *
 *  An AES encrypt helper is used to request the Bluetooth controller's own
 *  hardware to encrypt the plaintext using the key and returns the encrypted
 *  data.
 *
 *  @param key 128 bit LS byte first key for the encryption of the plaintext
 *  @param plaintext 128 bit LS byte first plaintext data block to be encrypted
 *  @param enc_data 128 bit LS byte first encrypted data block
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16]);

/** @brief AES encrypt big-endian data.
 *
 *  An AES encrypt helper is used to request the Bluetooth controller's own
 *  hardware to encrypt the plaintext using the key and returns the encrypted
 *  data.
 *
 *  @param key 128 bit MS byte first key for the encryption of the plaintext
 *  @param plaintext 128 bit MS byte first plaintext data block to be encrypted
 *  @param enc_data 128 bit MS byte first encrypted data block
 *
 *  @return Zero on success or error code otherwise.
 */
int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16]);


/** @brief Decrypt big-endian data with AES-CCM.
 *
 *  Decrypts and authorizes @c enc_data with AES-CCM, as described in
 *  https://tools.ietf.org/html/rfc3610.
 *
 *  Assumes that the MIC follows directly after the encrypted data.
 *
 *  @param key       128 bit MS byte first key
 *  @param nonce     13 byte MS byte first nonce
 *  @param enc_data  Encrypted data
 *  @param len       Length of the encrypted data
 *  @param aad       Additional input data
 *  @param aad_len   Additional input data length
 *  @param plaintext Plaintext buffer to place result in
 *  @param mic_size  Size of the trailing MIC (in bytes)
 *
 *  @retval 0        Successfully decrypted the data.
 *  @retval -EINVAL  Invalid parameters.
 *  @retval -EBADMSG Authentication failed.
 */
int bt_ccm_decrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *enc_data,
		   size_t len, const uint8_t *aad, size_t aad_len,
		   uint8_t *plaintext, size_t mic_size);


/** @brief Encrypt big-endian data with AES-CCM.
 *
 *  Encrypts and generates a MIC from @c plaintext with AES-CCM, as described in
 *  https://tools.ietf.org/html/rfc3610.
 *
 *  Places the MIC directly after the encrypted data.
 *
 *  @param key       128 bit MS byte first key
 *  @param nonce     13 byte MS byte first nonce
 *  @param enc_data  Buffer to place encrypted data in
 *  @param len       Length of the encrypted data
 *  @param aad       Additional input data
 *  @param aad_len   Additional input data length
 *  @param plaintext Plaintext buffer to encrypt
 *  @param mic_size  Size of the trailing MIC (in bytes)
 *
 *  @retval 0        Successfully encrypted the data.
 *  @retval -EINVAL  Invalid parameters.
 */
int bt_ccm_encrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *enc_data,
		   size_t len, const uint8_t *aad, size_t aad_len,
		   uint8_t *plaintext, size_t mic_size);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CRYPTO_H_ */
