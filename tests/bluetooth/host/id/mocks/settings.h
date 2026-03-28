/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define SETTINGS_FFF_FAKES_LIST(FAKE)                                                              \
	FAKE(bt_settings_store_id)                                                                 \
	FAKE(bt_settings_delete_id)                                                                \
	FAKE(bt_settings_store_irk)                                                                \
	FAKE(bt_settings_delete_irk)

DECLARE_FAKE_VOID_FUNC(bt_settings_store_id);
DECLARE_FAKE_VOID_FUNC(bt_settings_delete_id);
DECLARE_FAKE_VOID_FUNC(bt_settings_store_irk);
DECLARE_FAKE_VOID_FUNC(bt_settings_delete_irk);
