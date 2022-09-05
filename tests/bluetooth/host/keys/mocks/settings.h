/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/addr.h>

/* List of fakes used by this unit tester */
#define SETTINGS_FFF_FAKES_LIST(FAKE)      \
		FAKE(bt_settings_encode_key)       \
		FAKE(bt_settings_decode_key)       \
		FAKE(settings_name_next)           \

DECLARE_FAKE_VOID_FUNC(bt_settings_encode_key, char *, size_t, const char *,
			    const bt_addr_le_t *, const char *);
DECLARE_FAKE_VALUE_FUNC(int, bt_settings_decode_key, const char *, bt_addr_le_t *);
DECLARE_FAKE_VALUE_FUNC(int, settings_name_next, const char *, const char **);
