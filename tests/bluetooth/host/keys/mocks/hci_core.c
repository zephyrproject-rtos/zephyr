/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/hci_core.h"

DEFINE_FAKE_VALUE_FUNC(int, bt_unpair, uint8_t, const bt_addr_le_t *);
DEFINE_FAKE_VOID_FUNC(bt_id_add, struct bt_keys *);
