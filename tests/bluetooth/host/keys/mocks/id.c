/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/id.h"

DEFINE_FAKE_VOID_FUNC(bt_id_del, struct bt_keys *);
DEFINE_FAKE_VOID_FUNC(bt_id_pending_keys_update);
DEFINE_FAKE_VOID_FUNC(bt_id_pending_keys_update_set, struct bt_keys *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, k_work_submit, struct k_work *);
