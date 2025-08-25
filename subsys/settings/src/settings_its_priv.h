/*
 * Copyright (c) 2019 Laczen
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/settings/settings.h>

struct setting_entry {
	char name[SETTINGS_MAX_NAME_LEN];
	char value[SETTINGS_MAX_VAL_LEN];
	size_t val_len;
};

static struct setting_entry entries[CONFIG_SETTINGS_TFM_ITS_NUM_ENTRIES];
