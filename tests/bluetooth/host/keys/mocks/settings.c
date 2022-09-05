/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/settings.h"

DEFINE_FAKE_VOID_FUNC(bt_settings_encode_key, char *, size_t, const char *,
			    const bt_addr_le_t *, const char *);
DEFINE_FAKE_VALUE_FUNC(int, bt_settings_decode_key, const char *, bt_addr_le_t *);
DEFINE_FAKE_VALUE_FUNC(int, settings_name_next, const char *, const char **);
