/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_COMMON_H
#define SECURE_STORAGE_COMMON_H

/** @file zephyr/secure_storage/common.h Common definitions of the secure storage subsystem. */
#include <stdint.h>
#include <zephyr/toolchain.h>
#include <psa/storage_common.h>

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

#ifdef CONFIG_SECURE_STORAGE_64_BIT_UID

/** The UID (caller + entry IDs) of an ITS entry. */
typedef struct __packed {
	psa_storage_uid_t uid;
	secure_storage_caller_id_t caller_id;
} secure_storage_uid_t;

#else

#define SECURE_STORAGE_UID_BIT_SIZE 30
#define SECURE_STORAGE_CALLER_ID_BIT_SIZE 2

/** @brief The UID (caller + entry IDs) of an ITS entry.
 * This is a packed, 32-bit version of `psa_storage_uid_t` which allows storing
 * smaller IDs compared to the 64-bit ones that PSA Secure Storage specifies.
 * Zephyr defines ranges of IDs to be used by different users of the API (subsystems, application)
 * which guarantees 1. no collisions and 2. that the IDs used fit within `uid`.
 * @see @ref zephyr/psa/key_ids.h and the other header files under `zephyr/psa`.
 */
typedef struct {
	psa_storage_uid_t uid : SECURE_STORAGE_UID_BIT_SIZE;
	secure_storage_caller_id_t caller_id : SECURE_STORAGE_CALLER_ID_BIT_SIZE;
} secure_storage_uid_t;

#endif /* CONFIG_SECURE_STORAGE_64_BIT_UID */

#endif
