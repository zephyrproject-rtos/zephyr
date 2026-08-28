/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file srtp_crypto.h
 * @brief Internal PSA Crypto wrappers for SRTP (RFC 3711 / RFC 7714)
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_RTP_SRTP_CRYPTO_H_
#define ZEPHYR_SUBSYS_NET_LIB_RTP_SRTP_CRYPTO_H_

#include <psa/crypto.h>
#include <zephyr/net/srtp.h>

/** KDF labels as per RFC 3711 section 4.3.1 */
#define SRTP_KDF_LABEL_ENCRYPTION 0x00
#define SRTP_KDF_LABEL_AUTH       0x01
#define SRTP_KDF_LABEL_SALT       0x02

/**
 * @brief Scrub key material from memory.
 *
 * The volatile accesses prevent the compiler from eliding the stores as
 * dead when the buffer goes out of scope right after.
 *
 * @param buf Buffer to zeroize.
 * @param len Length of @p buf in bytes.
 */
static inline void srtp_crypto_zeroize(void *buf, size_t len)
{
	volatile uint8_t *p = buf;

	while (len-- > 0) {
		*p++ = 0U;
	}
}

/**
 * @brief Import an SRTP master key for use with the key derivation function.
 *
 * @param key     Master key.
 * @param key_len Length of @p key in bytes.
 * @param key_id  Location to store the PSA key identifier.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_master_key_import(const uint8_t *key, size_t key_len, psa_key_id_t *key_id);

/**
 * @brief Derive an SRTP session key (RFC 3711 section 4.3.1).
 *
 * Computes AES-CM keystream over the block x = key_id XOR master_salt, where
 * key_id = label || index DIV kdr.
 *
 * @param master_key    PSA key identifier of the master key.
 * @param master_salt   Master salt, zero padded to 14 bytes when shorter.
 * @param label         KDF label.
 * @param index_div_kdr Packet index divided by the key derivation rate; 0 when
 *                      the key derivation rate is 0.
 * @param out           Output buffer for the derived key material.
 * @param out_len       Number of bytes to derive.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_kdf(psa_key_id_t master_key, const uint8_t master_salt[SRTP_MASTER_SALT_LEN],
		    uint8_t label, uint64_t index_div_kdr, uint8_t *out, size_t out_len);

/**
 * @brief Import a derived AES-CM session encryption key.
 *
 * @param key    16-byte session key.
 * @param key_id Location to store the PSA key identifier.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_cm_key_import(const uint8_t key[SRTP_AES_128_KEY_LEN], psa_key_id_t *key_id);

/**
 * @brief Import a derived AES-GCM session encryption key.
 *
 * @param key     Session key.
 * @param key_len Length of @p key in bytes; 16 (AES-128) or 32 (AES-256).
 * @param key_id  Location to store the PSA key identifier.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_gcm_key_import(const uint8_t *key, size_t key_len, psa_key_id_t *key_id);

/**
 * @brief Import a derived HMAC-SHA1 session authentication key.
 *
 * @param key     20-byte session authentication key.
 * @param tag_len Truncated tag length in bytes (10 or 4).
 * @param key_id  Location to store the PSA key identifier.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_auth_key_import(const uint8_t key[SRTP_HMAC_SHA1_KEY_LEN], size_t tag_len,
				psa_key_id_t *key_id);

/**
 * @brief Encrypt or decrypt data in place with AES-CM (RFC 3711 section 4.1.1).
 *
 * AES counter mode is symmetric; the same operation is used in both
 * directions.
 *
 * @param key  PSA key identifier of the session encryption key.
 * @param iv   16-byte initial counter block.
 * @param data Data to transform in place.
 * @param len  Length of @p data in bytes.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_cm_crypt(psa_key_id_t key, const uint8_t iv[16], uint8_t *data, size_t len);

/**
 * @brief Compute the truncated HMAC-SHA1 tag over data followed by the ROC.
 *
 * @param key     PSA key identifier of the session authentication key.
 * @param tag_len Truncated tag length in bytes (10 or 4).
 * @param data    Authenticated portion of the packet.
 * @param len     Length of @p data in bytes.
 * @param roc     Rollover counter, appended big endian (RFC 3711 section 4.2).
 * @param tag     Output buffer of @p tag_len bytes.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_auth_compute(psa_key_id_t key, size_t tag_len, const uint8_t *data, size_t len,
			     uint32_t roc, uint8_t *tag);

/**
 * @brief Verify the truncated HMAC-SHA1 tag over data followed by the ROC.
 *
 * @param key     PSA key identifier of the session authentication key.
 * @param tag_len Truncated tag length in bytes (10 or 4).
 * @param data    Authenticated portion of the packet.
 * @param len     Length of @p data in bytes.
 * @param roc     Rollover counter, appended big endian (RFC 3711 section 4.2).
 * @param tag     Expected tag of @p tag_len bytes.
 *
 * @retval 0        On success.
 * @retval -EBADMSG When the tag does not match.
 * @retval negative Other errno value on failure.
 */
int srtp_crypto_auth_verify(psa_key_id_t key, size_t tag_len, const uint8_t *data, size_t len,
			    uint32_t roc, const uint8_t *tag);

/**
 * @brief Encrypt data in place with AES-GCM (RFC 7714 section 8.1).
 *
 * The 16-byte authentication tag is written directly after the ciphertext,
 * so @p data must have room for @p len + @ref SRTP_AES_GCM_TAG_LEN bytes.
 *
 * @param key     PSA key identifier of the session encryption key.
 * @param iv      12-byte initialization vector.
 * @param aad     Additional authenticated data (the RTP header).
 * @param aad_len Length of @p aad in bytes.
 * @param data    Plaintext to encrypt in place.
 * @param len     Length of the plaintext in bytes.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_crypto_gcm_encrypt(psa_key_id_t key, const uint8_t iv[SRTP_AES_GCM_IV_LEN],
			    const uint8_t *aad, size_t aad_len, uint8_t *data, size_t len);

/**
 * @brief Decrypt data in place with AES-GCM (RFC 7714 section 8.2).
 *
 * @param key     PSA key identifier of the session encryption key.
 * @param iv      12-byte initialization vector.
 * @param aad     Additional authenticated data (the RTP header).
 * @param aad_len Length of @p aad in bytes.
 * @param data    Ciphertext followed by the 16-byte tag, decrypted in place.
 * @param len     Length of @p data including the tag, in bytes.
 *
 * @retval 0        On success.
 * @retval -EBADMSG When authentication fails.
 * @retval negative Other errno value on failure.
 */
int srtp_crypto_gcm_decrypt(psa_key_id_t key, const uint8_t iv[SRTP_AES_GCM_IV_LEN],
			    const uint8_t *aad, size_t aad_len, uint8_t *data, size_t len);

/**
 * @brief Destroy a PSA key and reset the identifier.
 *
 * @param key_id Key identifier to destroy; ignored when PSA_KEY_ID_NULL.
 */
void srtp_crypto_key_destroy(psa_key_id_t *key_id);

#endif /* ZEPHYR_SUBSYS_NET_LIB_RTP_SRTP_CRYPTO_H_ */
