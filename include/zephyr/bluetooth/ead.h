/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_EAD_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_EAD_H_

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Encrypted Advertising Data (EAD)
 * @defgroup bt_ead Encrypted Advertising Data (EAD)
 * @ingroup bluetooth
 * @{
 */

/** Randomizer size in bytes */
#define BT_EAD_RANDOMIZER_SIZE 5
/** Key size in bytes */
#define BT_EAD_KEY_SIZE	       16
/** Initialisation Vector size in bytes */
#define BT_EAD_IV_SIZE	       8
/** MIC size in bytes */
#define BT_EAD_MIC_SIZE	       4

/** Get the size (in bytes) of the encrypted advertising data for a given
 *  payload size in bytes.
 */
#define BT_EAD_ENCRYPTED_PAYLOAD_SIZE(payload_size)                                                \
	((payload_size) + BT_EAD_RANDOMIZER_SIZE + BT_EAD_MIC_SIZE)

/** Get the size (in bytes) of the decrypted payload for a given payload size in
 *  bytes.
 */
#define BT_EAD_DECRYPTED_PAYLOAD_SIZE(encrypted_payload_size)                                      \
	((encrypted_payload_size) - (BT_EAD_RANDOMIZER_SIZE + BT_EAD_MIC_SIZE))

/**
 * @brief Encrypt and authenticate the given advertising data.
 *
 * The resulting data in @p encrypted_payload will look like that:
 * - Randomizer is added in the @ref BT_EAD_RANDOMIZER_SIZE first bytes;
 * - Encrypted payload is added ( @p payload_size bytes);
 * - MIC is added in the last @ref BT_EAD_MIC_SIZE bytes.
 *
 * @attention The function must be called each time the RPA is updated or the
 * data are modified.
 *
 * @note The term `advertising structure` is used to describe the advertising
 *       data with the advertising type and the length of those two.
 *
 * @param[in]  session_key Key of @ref BT_EAD_KEY_SIZE bytes used for the
 *             encryption.
 * @param[in]  iv Initialisation Vector used to generate the nonce. It must be
 *             changed each time the Session Key changes.
 * @param[in]  payload Advertising Data to encrypt. Can be multiple advertising
 *             structures that are concatenated.
 * @param[in]  payload_size Size of the Advertising Data to encrypt.
 * @param[out] encrypted_payload Encrypted Ad Data including the Randomizer and
 *             the MIC. Size must be at least @ref BT_EAD_RANDOMIZER_SIZE + @p
 *             payload_size + @ref BT_EAD_MIC_SIZE. Use @ref
 *             BT_EAD_ENCRYPTED_PAYLOAD_SIZE to get the right size.
 *
 * @retval 0 Data have been correctly encrypted and authenticated.
 * @retval -EIO Error occurred during the encryption or the authentication.
 * @retval -EINVAL One of the argument is a NULL pointer.
 * @retval -ECANCELED Error occurred during the random number generation.
 */
int bt_ead_encrypt(const uint8_t session_key[BT_EAD_KEY_SIZE], const uint8_t iv[BT_EAD_IV_SIZE],
		   const uint8_t *payload, size_t payload_size, uint8_t *encrypted_payload);

/**
 * @brief Decrypt and authenticate the given encrypted advertising data.
 *
 * @note The term `advertising structure` is used to describe the advertising
 *       data with the advertising type and the length of those two.
 *
 * @param[in]  session_key Key of 16 bytes used for the encryption.
 * @param[in]  iv Initialisation Vector used to generate the `nonce`.
 * @param[in]  encrypted_payload Encrypted Advertising Data received. This
 *             should only contain the advertising data from the received
 *             advertising structure, not the length nor the type.
 * @param[in]  encrypted_payload_size Size of the received advertising data in
 *             bytes. Should be equal to the length field of the received
 *             advertising structure, minus the size of the type (1 byte).
 * @param[out] payload Decrypted advertising payload. Use @ref
 *             BT_EAD_DECRYPTED_PAYLOAD_SIZE to get the right size.
 *
 * @retval 0 Data have been correctly decrypted and authenticated.
 * @retval -EIO Error occurred during the decryption or the authentication.
 * @retval -EINVAL One of the argument is a NULL pointer or @p
 *                 encrypted_payload_size is less than @ref
 *                 BT_EAD_RANDOMIZER_SIZE + @ref BT_EAD_MIC_SIZE.
 */
int bt_ead_decrypt(const uint8_t session_key[BT_EAD_KEY_SIZE], const uint8_t iv[BT_EAD_IV_SIZE],
		   const uint8_t *encrypted_payload, size_t encrypted_payload_size,
		   uint8_t *payload);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_EAD_H_ */
