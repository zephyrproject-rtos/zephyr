/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "addr_internal.h"

DEFINE_FAKE_VOID_FUNC(bt_addr_le_copy_resolved, bt_addr_le_t *, const bt_addr_le_t *);
DEFINE_FAKE_VALUE_FUNC(bool, bt_addr_le_is_resolved, const bt_addr_le_t *);
