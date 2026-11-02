/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_H_
#define ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_H_

/**
 * @file it51xxx_crypto.h
 *
 * Public API for IT51XXX Crypto driver
 */

#include "psa/crypto.h"

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Size of the shared hardware crypto DLM region in bytes. */
#define IT51XXX_HWCRYPTO_DLM_SIZE KB(4)

/** SHA256 hash length in 32-bit words. */
#define SHA_SHA256_HASH_LEN_WORDS (PSA_HASH_LENGTH(PSA_ALG_SHA_256) / sizeof(uint32_t))

/**
 * @brief Maximum input length in bytes per SHA hardware operation.
 *
 * Larger messages are processed in chunks of up to 1024 bytes.
 * For example, a 10 KiB message requires 10 hardware operations.
 */
#define SHA_HW_MAX_INPUT_LEN       1024
/** Maximum input length in 32-bit words per SHA hardware operation. */
#define SHA_HW_MAX_INPUT_LEN_WORDS (SHA_HW_MAX_INPUT_LEN / sizeof(uint32_t))

/** Minimum RSA operand length in bytes. */
#define IT51XXX_HWCRYPTO_RSA_MIN_BYTE_LEN 64
/** Maximum RSA operand length in bytes. */
#define IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN 512 /* 512 bytes (4096 bits) */

/**
 * @brief SHA256 DLM data.
 *
 * Must reside within the first 4k-byte of RAM and be aligned to a
 * 256-byte boundary.
 */
struct it51xxx_sha256_dlm {
	/** SHA input buffer accessible as 32-bit words or bytes. */
	union {
		/** SHA input buffer accessed as 32-bit words. */
		uint32_t input_words[SHA_HW_MAX_INPUT_LEN_WORDS];
		/** SHA input buffer accessed as bytes. */
		uint8_t input_bytes[SHA_HW_MAX_INPUT_LEN];
	};
	/** SHA hash state words H[0] through H[7]. */
	uint32_t hash[SHA_SHA256_HASH_LEN_WORDS];
	/** SHA initialization state. */
	uint32_t sha_init;
	/** Current index into the input buffer. */
	uint32_t input_bytes_index;
	/** Total input message length in bytes. */
	uint32_t total_len;
} __aligned(256);

/**
 * @brief RSA DLM layout.
 *
 * Field offsets correspond to the hardware-defined layout within
 * the 4 KiB DLM region.
 */
struct it51xxx_rsa_dlm {
	/** Public key data at offsets 0x000–0x1FF. */
	uint8_t key_public[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	/** Reserved region at offsets 0x200–0x3FF. */
	uint8_t reserved_1[512];
	/** Message data at offsets 0x400–0x5FF. */
	uint8_t messages[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	/** Reserved region at offsets 0x600–0x9FF. */
	uint8_t reserved_2[1024];
	/** Private key data at offsets 0xA00–0xBFF. */
	uint8_t key_private[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
	/** Reserved region at offsets 0xC00–0xFFF. */
	uint8_t reserved_3[1024];
};

/**
 * @brief Shared DLM storage for it51xxx hardware crypto algorithms.
 *
 * SHA, RSA, and other crypto algorithms share the same 4 KiB DLM
 * memory region. Union members use the same underlying storage,
 * so access must be serialized.
 */
union hwcrypto_dlm_block {
	/** SHA256 DLM data. */
	struct it51xxx_sha256_dlm sha256;
	/** RSA DLM data. */
	struct it51xxx_rsa_dlm rsa;
	/** Raw access to the entire shared DLM region. */
	uint8_t raw_data[IT51XXX_HWCRYPTO_DLM_SIZE];
};

/**
 * @brief Shared DLM instance used by it51xxx hardware crypto algorithms.
 */
extern union hwcrypto_dlm_block hwcrypto_dlm;

/**
 *@brief Get the hardware crypto register base address.
 *
 *@return Register base address obtained from devicetree.
 */
mm_reg_t it51xxx_crypto_get_base(void);

/**
 *@brief Lock the shared hardware crypto resource.
 *
 *@param[in] timeout Maximum time to wait for the mutex.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
int it51xxx_crypto_lock(k_timeout_t timeout);

/**
 *@brief Unlock the shared hardware crypto resource.
 *
 * Must be called by the thread that holds the mutex.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
void it51xxx_crypto_unlock(void);

/**
 *@brief Initialize shared hardware crypto resources.
 *
 * Call before starting any hardware crypto operation.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t it51xxx_psa_crypto_init(void);

/**
 *@brief Release shared hardware crypto resources.
 *
 * Call only after all hardware crypto operations have completed.
 *
 * @return  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t it51xxx_psa_crypto_free(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_H_ */
