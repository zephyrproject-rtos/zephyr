/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_PS_STORE_SETTINGS_H
#define SECURE_STORAGE_PS_STORE_SETTINGS_H

/** @file zephyr/secure_storage/ps/store/settings.h The settings PS store module API.
 *
 * The functions declared in this header allow customization
 * of the settings implementation of the PS store module.
 * They are not meant to be called directly other than by the settings PS store module.
 * This header file may and must be included when providing a custom implementation of one
 * or more of these functions (@kconfig_regex{CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_.*_CUSTOM}).
 */
#include <zephyr/secure_storage/ps/common.h>

/** @brief PS store settings name buffer size */
#define SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE                    \
		(CONFIG_SECURE_STORAGE_PS_STORE_SETTINGS_NAME_MAX_LEN + 1)

/** @brief Returns the setting name to use for a PS entry.
 *
 * @param[in]  uid  The UID of the PS entry for which the setting name is used.
 * @param[out] name The setting name.
 */
void secure_storage_ps_store_settings_get_name(
	psa_storage_uid_t uid,
	char name[static SECURE_STORAGE_PS_STORE_SETTINGS_NAME_BUF_SIZE]);

#endif
