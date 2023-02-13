/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/settings_store.h"

DEFINE_FAKE_VALUE_FUNC(int, settings_delete, const char *);
DEFINE_FAKE_VALUE_FUNC(int, settings_save_one, const char *, const void *, size_t);
