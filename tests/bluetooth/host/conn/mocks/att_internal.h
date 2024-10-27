/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define ATT_INTERNAL_MOCKS_FFF_FAKES_LIST(FAKE) FAKE(bt_att_init)

DECLARE_FAKE_VOID_FUNC(bt_att_init);
