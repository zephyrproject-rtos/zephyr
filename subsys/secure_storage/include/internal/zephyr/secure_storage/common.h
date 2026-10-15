/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_COMMON_H
#define SECURE_STORAGE_COMMON_H

/** @file zephyr/secure_storage/common.h Common definitions of the secure storage subsystem. */
#include <stdint.h>

/* A size-optimized version of `psa_storage_create_flags_t`. Used for storing the `create_flags`. */
typedef uint8_t secure_storage_packed_create_flags_t;

#define SECURE_STORAGE_ALL_CREATE_FLAGS \
	(PSA_STORAGE_FLAG_NONE | \
	 PSA_STORAGE_FLAG_WRITE_ONCE | \
	 PSA_STORAGE_FLAG_NO_CONFIDENTIALITY | \
	 PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION)

/* For logging a `psa_storage_uid_t`, whose used width depends on the configuration. */
#ifdef CONFIG_SECURE_STORAGE_64_BIT_UID
/** Format specifier for logging a `psa_storage_uid_t`. */
#define PSA_UID_FMT           "%#llx"
/** Argument for logging a `psa_storage_uid_t` with @ref PSA_UID_FMT. */
#define PSA_UID_ARGS(psa_uid) (unsigned long long)(psa_uid)
#else
/** Format specifier for logging a `psa_storage_uid_t`. */
#define PSA_UID_FMT           "%#lx"
/** Argument for logging a `psa_storage_uid_t` with @ref PSA_UID_FMT. */
#define PSA_UID_ARGS(psa_uid) (unsigned long)(psa_uid)
#endif

#endif
