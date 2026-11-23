/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_COMMON_H
#define SECURE_STORAGE_COMMON_H

/** @file zephyr/secure_storage/common.h Common definitions of the secure storage subsystem. */
#include <stdint.h>

/** A size-optimized version of `psa_storage_create_flags_t`. Used for storing `create_flags`. */
typedef uint8_t secure_storage_packed_create_flags_t;

#define SECURE_STORAGE_ALL_CREATE_FLAGS \
	(PSA_STORAGE_FLAG_NONE | \
	 PSA_STORAGE_FLAG_WRITE_ONCE | \
	 PSA_STORAGE_FLAG_NO_CONFIDENTIALITY | \
	 PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION)

#if defined(CONFIG_TF_PSA_CRYPTO_BUILTIN) || defined(CONFIG_TF_PSA_CRYPTO_CUSTOM)
#include <mbedtls/platform_util.h>
#define secure_storage_zeroize mbedtls_platform_zeroize
#else
#include <stddef.h>
/** Zeroizes a buffer. */
void secure_storage_zeroize(void *buf, size_t len);
#endif

#endif
