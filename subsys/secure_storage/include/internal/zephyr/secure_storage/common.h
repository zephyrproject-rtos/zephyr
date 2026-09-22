/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_COMMON_H
#define SECURE_STORAGE_COMMON_H

/** @file zephyr/secure_storage/common.h Common definitions of the secure storage subsystem. */
#include <stdint.h>
#include <zephyr/toolchain.h>
#include <psa/error.h>

/* A size-optimized version of `psa_storage_create_flags_t`. Used for storing the `create_flags`. */
typedef uint8_t secure_storage_packed_create_flags_t;

#define SECURE_STORAGE_ALL_CREATE_FLAGS \
	(PSA_STORAGE_FLAG_NONE | \
	 PSA_STORAGE_FLAG_WRITE_ONCE | \
	 PSA_STORAGE_FLAG_NO_CONFIDENTIALITY | \
	 PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION)

/** @brief The ID of the caller from which the ITS/PS API call originates.
 * This is used to namespace the different callers and possibly treat them differently.
 */
typedef enum {
	/** When the call comes from a psa_its_*() function. */
	SECURE_STORAGE_CALLER_PSA_ITS,
	/** When the call comes from a psa_ps_*() function. */
	SECURE_STORAGE_CALLER_PSA_PS,
	/** When the call comes from the PSA Crypto implementation. */
	SECURE_STORAGE_CALLER_PSA_CRYPTO,
	/** Number of caller IDs. */
	SECURE_STORAGE_CALLER_COUNT
} secure_storage_caller_id_t;

#endif
