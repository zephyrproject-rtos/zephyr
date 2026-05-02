/*
 * Copyright (c) 2019-2025 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>

struct setting_entry {
	char name[SETTINGS_MAX_NAME_LEN];
	char value[SETTINGS_MAX_VAL_LEN];
	size_t val_len;
};
