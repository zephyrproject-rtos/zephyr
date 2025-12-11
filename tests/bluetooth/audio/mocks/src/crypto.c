/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>

#include <zephyr/fff.h>

#include "crypto.h"

DEFINE_FAKE_VALUE_FUNC(int, bt_rand, void *, size_t);
