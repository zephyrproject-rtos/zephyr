/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_SHA_H_
#define ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_SHA_H_

/**
 * @file it51xxx_crypto_sha.h
 * @brief Provide hash computation and SHA initialization.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute hash using the SHA hardware.
 *
 * @param alg PSA hash algorithm. Only PSA_ALG_SHA_256 is supported.
 * @param input Input data to hash.
 * @param input_length Length of the input data in bytes.
 * @param hash Buffer that receives the hash.
 * @param hash_size Size of the hash buffer in bytes.
 * @param hash_length Receives the number of hash bytes written on success.
 *
 * @retval PSA_SUCCESS The hash was computed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The requested algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The hash buffer is NULL or too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT The input length exceeds the supported limit.
 */
psa_status_t ite_it51xxx_hash_compute(psa_algorithm_t alg, const uint8_t *input,
				      size_t input_length, uint8_t *hash, size_t hash_size,
				      size_t *hash_length);

/**
 * @brief Initialize SHA hardware
 *
 * @retval PSA_SUCCESS on success.
 */
psa_status_t it51xxx_crypto_sha_init(void);

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_DRIVERS_MISC_IT51XXX_PSA_CRYPTO_IT51XXX_CRYPTO_SHA_H_ */
