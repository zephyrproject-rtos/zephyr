/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ITS_H
#define SECURE_STORAGE_ITS_H

/** @file zephyr/secure_storage/its.h The secure storage ITS implementation.
 *
 * The functions declared in this header implement the PSA ITS API
 * when the secure storage subsystem is enabled.
 * They must not be called directly, and this header must not be included other than when
 * providing a custom implementation (@kconfig{CONFIG_SECURE_STORAGE_ITS_IMPLEMENTATION_CUSTOM}).
 */
#include "its/common.h"

/** @brief See `psa_its_set()`, to which this function is analogous. */
psa_status_t secure_storage_its_set(secure_storage_its_uid_t uid, size_t data_length,
				    const void *p_data, psa_storage_create_flags_t create_flags);

/** @brief See `psa_its_get()`, to which this function is analogous. */
psa_status_t secure_storage_its_get(secure_storage_its_uid_t uid, size_t data_offset,
				    size_t data_size, void *p_data, size_t *p_data_length);

/** @brief See `psa_its_get_info()`, to which this function is analogous. */
psa_status_t secure_storage_its_get_info(secure_storage_its_uid_t uid,
					 struct psa_storage_info_t *p_info);

/** @brief See `psa_its_remove()`, to which this function is analogous. */
psa_status_t secure_storage_its_remove(secure_storage_its_uid_t uid);

#endif
