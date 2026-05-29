/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef PSA_ERROR_H
#define PSA_ERROR_H
/**
 * @file psa/error.h Return values of the PSA Secure Storage API.
 * @ingroup psa_secure_storage
 * @{
 */
#include <stdint.h>

/** Function return status. Either `PSA_SUCCESS` or one of the defined `PSA_ERROR_*` values. */
typedef int32_t psa_status_t;

/** The operation completed successfully. */
#define PSA_SUCCESS ((psa_status_t)0)

/** An unspecified internal failure happened. */
#define PSA_ERROR_GENERIC_ERROR        ((psa_status_t)-132)

/** An entry associated with the provided `uid` already
 *  exists and was created with `PSA_STORAGE_FLAG_WRITE_ONCE`.
 */
#define PSA_ERROR_NOT_PERMITTED        ((psa_status_t)-133)

/** The function is not supported or one or more of the flags
 *  provided in `create_flags` are not supported or not valid.
 */
#define PSA_ERROR_NOT_SUPPORTED        ((psa_status_t)-134)

/** One or more arguments other than `create_flags` are invalid. */
#define PSA_ERROR_INVALID_ARGUMENT     ((psa_status_t)-135)

/** An entry with the provided `uid` already exists. */
#define PSA_ERROR_ALREADY_EXISTS       ((psa_status_t)-139)

/** The provided `uid` was not found in the storage. */
#define PSA_ERROR_DOES_NOT_EXIST       ((psa_status_t)-140)

/** There is insufficient space on the storage medium. */
#define PSA_ERROR_INSUFFICIENT_STORAGE ((psa_status_t)-142)

/** The physical storage has failed (fatal error). */
#define PSA_ERROR_STORAGE_FAILURE      ((psa_status_t)-146)

/** The data associated with `uid` failed authentication. */
#define PSA_ERROR_INVALID_SIGNATURE    ((psa_status_t)-149)

/** The data associated with `uid` is corrupt. */
#define PSA_ERROR_DATA_CORRUPT         ((psa_status_t)-152)

/** @} */
#endif
