/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/adv.h>

typedef void (*bt_le_ext_adv_foreach_cb)(struct bt_le_ext_adv *adv, void *data);

/* List of fakes used by this unit tester */
#define ADV_FFF_FAKES_LIST(FAKE)                                                                   \
	FAKE(bt_le_adv_set_enable)                                                                 \
	FAKE(bt_le_adv_lookup_legacy)                                                              \
	FAKE(bt_le_ext_adv_get_index)                                                              \
	FAKE(bt_le_adv_set_enable_ext)                                                             \
	FAKE(bt_le_ext_adv_foreach)

DECLARE_FAKE_VALUE_FUNC(int, bt_le_adv_set_enable, struct bt_le_ext_adv *, bool);
DECLARE_FAKE_VALUE_FUNC(struct bt_le_ext_adv *, bt_le_adv_lookup_legacy);
DECLARE_FAKE_VALUE_FUNC(uint8_t, bt_le_ext_adv_get_index, struct bt_le_ext_adv *);
DECLARE_FAKE_VALUE_FUNC(int, bt_le_adv_set_enable_legacy, struct bt_le_ext_adv *, bool);
DECLARE_FAKE_VALUE_FUNC(int, bt_le_adv_set_enable_ext, struct bt_le_ext_adv *, bool,
			const struct bt_le_ext_adv_start_param *);
DECLARE_FAKE_VOID_FUNC(bt_le_ext_adv_foreach, bt_le_ext_adv_foreach_cb, void *);
