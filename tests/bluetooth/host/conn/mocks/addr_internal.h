/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/addr.h>

/* List of fakes used by this unit tester */
#define ADDR_INTERNAL_MOCKS_FFF_FAKES_LIST(FAKE)                                                   \
	FAKE(bt_addr_le_copy_resolved)                                                             \
	FAKE(bt_addr_le_is_resolved)

DECLARE_FAKE_VOID_FUNC(bt_addr_le_copy_resolved, bt_addr_le_t *, const bt_addr_le_t *);
DECLARE_FAKE_VALUE_FUNC(bool, bt_addr_le_is_resolved, const bt_addr_le_t *);
