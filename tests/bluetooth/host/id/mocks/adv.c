/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/adv.h"

#include <zephyr/kernel.h>

DEFINE_FAKE_VALUE_FUNC(int, bt_le_adv_set_enable, struct bt_le_ext_adv *, bool);
DEFINE_FAKE_VALUE_FUNC(struct bt_le_ext_adv *, bt_le_adv_lookup_legacy);
DEFINE_FAKE_VALUE_FUNC(uint8_t, bt_le_ext_adv_get_index, struct bt_le_ext_adv *);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_adv_set_enable_legacy, struct bt_le_ext_adv *, bool);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_adv_set_enable_ext, struct bt_le_ext_adv *, bool,
		       const struct bt_le_ext_adv_start_param *);
DEFINE_FAKE_VOID_FUNC(bt_le_ext_adv_foreach, bt_le_ext_adv_foreach_cb, void *);
