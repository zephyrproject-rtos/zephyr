/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/addr.h>
#include <host/keys.h>

/* List of fakes used by this unit tester */
#define ID_FFF_FAKES_LIST(FAKE)                     \
		FAKE(bt_id_del)                     \
		FAKE(bt_id_pending_keys_update)     \
		FAKE(bt_id_pending_keys_update_set) \
		FAKE(k_work_submit)

DECLARE_FAKE_VOID_FUNC(bt_id_del, struct bt_keys *);
DECLARE_FAKE_VOID_FUNC(bt_id_pending_keys_update);
DECLARE_FAKE_VOID_FUNC(bt_id_pending_keys_update_set, struct bt_keys *, uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, k_work_submit, struct k_work *);
