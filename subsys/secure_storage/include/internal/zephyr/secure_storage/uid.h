/*
 * Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_UID_H
#define SECURE_STORAGE_UID_H

/** @file zephyr/secure_storage/uid.h UID definitions of the secure storage subsystem. */
#include <stdint.h>
#include <zephyr/secure_storage/common.h>
#include <psa/storage_common.h>

#ifdef CONFIG_SECURE_STORAGE_64_BIT_UID

/** The UID (caller + entry IDs) of an ITS/PS entry. */
typedef struct __packed {
	/** The entry ID. */
	psa_storage_uid_t uid;
	/** The ID of the caller. */
	secure_storage_caller_id_t caller_id;
} secure_storage_uid_t;

#else

/** Width, in bits, of the entry ID in a 32-bit `secure_storage_uid_t`. */
#define SECURE_STORAGE_UID_BIT_SIZE 30
/** Width, in bits, of the caller ID in a 32-bit `secure_storage_uid_t`. */
#define SECURE_STORAGE_CALLER_ID_BIT_SIZE 2

/** @brief The UID (caller + entry IDs) of an ITS/PS entry.
 * This is a packed, 32-bit version of `psa_storage_uid_t` which allows storing
 * smaller IDs compared to the 64-bit ones that PSA Secure Storage specifies.
 * Zephyr defines ranges of IDs to be used by different users of the API (subsystems, application)
 * which guarantees 1. no collisions and 2. that the IDs used fit within `uid`.
 * @see @ref zephyr/psa/key_ids.h and the other header files under `zephyr/psa`.
 */
typedef struct {
	/** The entry ID. */
	psa_storage_uid_t uid : SECURE_STORAGE_UID_BIT_SIZE;
	/** The ID of the caller. */
	secure_storage_caller_id_t caller_id : SECURE_STORAGE_CALLER_ID_BIT_SIZE;
} secure_storage_uid_t;

#endif /* CONFIG_SECURE_STORAGE_64_BIT_UID */

/* For logging a `secure_storage_uid_t`, whose width depends on the configuration. */
#ifdef CONFIG_SECURE_STORAGE_64_BIT_UID
/** Format specifier for logging a `psa_storage_uid_t`. */
#define PSA_UID_FMT             "%#llx"
/** Argument for logging a `psa_storage_uid_t` with @ref PSA_UID_FMT. */
#define PSA_UID_ARGS(psa_uid)    (unsigned long long)psa_uid
/** Format specifier for logging a `secure_storage_uid_t`. */
#define SS_UID_FMT              "%u/"PSA_UID_FMT
/** Arguments for logging a `secure_storage_uid_t` with @ref SS_UID_FMT. */
#define SS_UID_ARGS(ss_uid)     (ss_uid).caller_id, PSA_UID_ARGS(ss_uid.uid)
#else
/** Format specifier for logging a `psa_storage_uid_t`. */
#define PSA_UID_FMT             "%#lx"
/** Argument for logging a `psa_storage_uid_t` with @ref PSA_UID_FMT. */
#define PSA_UID_ARGS(psa_uid)    (unsigned long)psa_uid
/** Format specifier for logging a `secure_storage_uid_t`. */
#define SS_UID_FMT              "%u/"PSA_UID_FMT
/** Arguments for logging a `secure_storage_uid_t` with @ref SS_UID_FMT. */
#define SS_UID_ARGS(ss_uid)     (ss_uid).caller_id, PSA_UID_ARGS(ss_uid.uid)
#endif

/** @brief Build the ITS/PS UID of an entry from its caller ID and entry ID.
 *
 * @param caller_id The ID of the caller.
 * @param uid The entry ID.
 * @param[out] out_uid The resulting UID.
 *
 * @retval PSA_SUCCESS The UID was built.
 * @retval PSA_ERROR_INVALID_ARGUMENT The entry ID is zero or does not fit in
 *                                    `SECURE_STORAGE_UID_BIT_SIZE` bits.
 */
psa_status_t secure_storage_make_uid(secure_storage_caller_id_t caller_id,
				     psa_storage_uid_t uid,
				     secure_storage_uid_t *out_uid);

#endif /* SECURE_STORAGE_UID_H */
