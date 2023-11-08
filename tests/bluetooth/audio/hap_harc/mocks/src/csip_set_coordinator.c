/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/csip.h>

#include "conn.h"
#include "csip.h"

#define LOG_LEVEL CONFIG_BT_CSIP_SET_COORDINATOR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_csip_set_coordinator);

DEFINE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_register_cb,
		       struct bt_csip_set_coordinator_cb *);
DEFINE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_discover, struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_ordered_access,
		       const struct bt_csip_set_coordinator_set_member **, uint8_t,
		       const struct bt_csip_set_coordinator_set_info *,
		       bt_csip_set_coordinator_ordered_access_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_lock,
		       const struct bt_csip_set_coordinator_set_member **, uint8_t,
		       const struct bt_csip_set_coordinator_set_info *);
DEFINE_FAKE_VALUE_FUNC(int, bt_csip_set_coordinator_release,
		       const struct bt_csip_set_coordinator_set_member **, uint8_t,
		       const struct bt_csip_set_coordinator_set_info *);
DEFINE_FAKE_VOID_FUNC(bt_csip_set_coordinator_set_members_sort_by_rank,
		      const struct bt_csip_set_coordinator_set_member **, size_t,
		      const struct bt_csip_set_coordinator_set_info *, bool);
