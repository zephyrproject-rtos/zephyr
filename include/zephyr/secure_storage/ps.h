/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file zephyr/secure_storage/ps.h
 * @brief The secure storage PS API.
 *
 * The functions declared in this header implement the PSA PS API
 * when the secure storage subsystem is enabled.
 * They must not be called directly, and this header must not be included other than when
 * providing a custom PS implementation (CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_CUSTOM).
 */
#ifndef SECURE_STORAGE_PS_H
#define SECURE_STORAGE_PS_H

#include <psa/storage_common.h>

/** @brief See `psa_ps_set()`, to which this function is analogous.
 *  @see psa_ps_set()
 */
psa_status_t secure_storage_ps_set(const psa_storage_uid_t uid, size_t data_length,
				   const void *p_data, psa_storage_create_flags_t create_flags);

/** @brief See `psa_ps_get()`, to which this function is analogous.
 *  @see psa_ps_get()
 */
psa_status_t secure_storage_ps_get(const psa_storage_uid_t uid, size_t data_offset,
				   size_t data_length, void *p_data, size_t *p_data_length);

/** @brief See `psa_ps_get_info()`, to which this function is analogous.
 *  @see psa_ps_get_info()
 */
psa_status_t secure_storage_ps_get_info(const psa_storage_uid_t uid,
					struct psa_storage_info_t *p_info);

/** @brief See `psa_ps_remove()`, to which this function is analogous.
 *  @see psa_ps_remove()
 */
psa_status_t secure_storage_ps_remove(const psa_storage_uid_t uid);

#ifdef CONFIG_SECURE_STORAGE_PS_SUPPORTS_SET_EXTENDED

/** @brief See `psa_ps_create()`, to which this function is analogous.
 *  @see psa_ps_create()
 */
psa_status_t secure_storage_ps_create(psa_storage_uid_t uid, size_t capacity,
				      psa_storage_create_flags_t create_flags);

/** @brief See `psa_ps_set_extended()`, to which this function is analogous.
 *  @see psa_ps_set_extended()
 */
psa_status_t secure_storage_ps_set_extended(psa_storage_uid_t uid, size_t data_offset,
					    size_t data_length, const void *p_data);

#endif /* CONFIG_SECURE_STORAGE_PS_SUPPORTS_SET_EXTENDED */

#endif
