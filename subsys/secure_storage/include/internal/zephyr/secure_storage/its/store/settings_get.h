/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_ITS_STORE_SETTINGS_GET_H
#define SECURE_STORAGE_ITS_STORE_SETTINGS_GET_H

/** @file zephyr/secure_storage/its/store/settings_get.h The settings ITS store module API.
 *
 * The functions declared in this header allow customization
 * of the settings implementation of the ITS store module.
 * They are not meant to be called directly other than by the settings ITS store module.
 * This header file may and must be included when providing a custom implementation of one
 * or more of these functions (@kconfig{CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_*_CUSTOM}).
 */
#include <zephyr/secure_storage/its/common.h>

enum { SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE
	= CONFIG_SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_MAX_LEN + 1 };

/** @brief Returns the setting name to use for an ITS entry.
 *
 * @param[in]  uid  The UID of the ITS entry for which the setting name is used.
 * @param[out] name The setting name.
 */
void secure_storage_its_store_settings_get_name(
	secure_storage_its_uid_t uid,
	char name[static SECURE_STORAGE_ITS_STORE_SETTINGS_NAME_BUF_SIZE]);

#endif
