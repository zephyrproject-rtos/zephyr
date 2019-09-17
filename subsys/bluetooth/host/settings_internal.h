/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Max settings key length (with all components) */
#define BT_SETTINGS_KEY_MAX 36

/* Base64-encoded string buffer size of in_size bytes */
#define BT_SETTINGS_SIZE(in_size) ((((((in_size) - 1) / 3) * 4) + 4) + 1)

void bt_settings_save_id(void);

int bt_settings_init(void);
