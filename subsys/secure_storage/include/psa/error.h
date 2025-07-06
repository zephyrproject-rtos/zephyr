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

typedef int32_t psa_status_t;

#define PSA_SUCCESS ((psa_status_t)0)

#define PSA_ERROR_GENERIC_ERROR        ((psa_status_t)-132)
#define PSA_ERROR_NOT_PERMITTED        ((psa_status_t)-133)
#define PSA_ERROR_NOT_SUPPORTED        ((psa_status_t)-134)
#define PSA_ERROR_INVALID_ARGUMENT     ((psa_status_t)-135)
#define PSA_ERROR_ALREADY_EXISTS       ((psa_status_t)-139)
#define PSA_ERROR_DOES_NOT_EXIST       ((psa_status_t)-140)
#define PSA_ERROR_INSUFFICIENT_STORAGE ((psa_status_t)-142)
#define PSA_ERROR_STORAGE_FAILURE      ((psa_status_t)-146)
#define PSA_ERROR_INVALID_SIGNATURE    ((psa_status_t)-149)
#define PSA_ERROR_DATA_CORRUPT         ((psa_status_t)-152)

/** @} */
#endif
