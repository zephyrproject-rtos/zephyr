/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ITS_COMMON_H
#define SECURE_STORAGE_ITS_COMMON_H

/** @file zephyr/secure_storage/its/common.h
 * @brief Common definitions of the secure storage subsystem's ITS APIs.
 */
#include "../common.h"
#include <zephyr/toolchain.h>
#include <psa/storage_common.h>

/** @brief The ID of the caller from which the ITS API call originates.
 * This is used to prevent ID collisions between different callers that are not aware
 * of each other and so might use the same numerical IDs, e.g. PSA Crypto and PSA ITS.
 */
typedef enum {
	SECURE_STORAGE_ITS_CALLER_PSA_ITS,
	SECURE_STORAGE_ITS_CALLER_PSA_PS,
	SECURE_STORAGE_ITS_CALLER_MBEDTLS,
	SECURE_STORAGE_ITS_CALLER_COUNT
} secure_storage_its_caller_id_t;

/** The UID (caller + entry IDs) of an ITS entry. */
typedef struct {
	psa_storage_uid_t uid;
	secure_storage_its_caller_id_t caller_id;
} __packed secure_storage_its_uid_t;

#endif
