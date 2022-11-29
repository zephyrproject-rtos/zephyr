/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define SETTINGS_FFF_FAKES_LIST(FAKE) FAKE(bt_settings_save_id)

DECLARE_FAKE_VOID_FUNC(bt_settings_save_id);
