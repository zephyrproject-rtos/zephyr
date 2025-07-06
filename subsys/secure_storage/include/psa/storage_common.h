/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef PSA_STORAGE_COMMON_H
#define PSA_STORAGE_COMMON_H
/**
 * @defgroup psa_secure_storage PSA Secure Storage API
 * @ingroup os_services
 * @details For more information on the PSA Secure Storage API, see the
 * [PSA Certified Secure Storage API](https://arm-software.github.io/psa-api/storage/1.0/)
 * specification.
 */
/**
 * @file psa/storage_common.h
 * @ingroup psa_secure_storage
 * @brief Common definitions of the PSA Secure Storage API.
 * @{
 */
#include <psa/error.h>
#include <stddef.h>

/** UID type for identifying entries. */
typedef uint64_t psa_storage_uid_t;

/** Flags used when creating an entry. */
typedef uint32_t psa_storage_create_flags_t;

/** No flag to pass. */
#define PSA_STORAGE_FLAG_NONE                 0u
/** The data associated with the UID will not be able to be modified or deleted. */
#define PSA_STORAGE_FLAG_WRITE_ONCE           (1u << 0)
/** The data associated with the UID is public, requiring only integrity. */
#define PSA_STORAGE_FLAG_NO_CONFIDENTIALITY   (1u << 1)
/** The data associated with the UID does not require replay protection. */
#define PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION (1u << 2)

/** Metadata associated with a specific entry. */
struct psa_storage_info_t {
	/** The allocated capacity of the storage associated with an entry. */
	size_t capacity;
	/** The size of an entry's data. */
	size_t size;
	/** The flags used when the entry was created. */
	psa_storage_create_flags_t flags;
};

/** Flag indicating that @ref psa_ps_create() and @ref psa_ps_set_extended() are supported. */
#define PSA_STORAGE_SUPPORT_SET_EXTENDED (1u << 0)

/** @} */
#endif
