/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/settings.h"

#include <zephyr/kernel.h>

DEFINE_FAKE_VOID_FUNC(bt_settings_store_id);
DEFINE_FAKE_VOID_FUNC(bt_settings_delete_id);
DEFINE_FAKE_VOID_FUNC(bt_settings_store_irk);
DEFINE_FAKE_VOID_FUNC(bt_settings_delete_irk);
