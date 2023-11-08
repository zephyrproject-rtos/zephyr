/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/has.h>

#include "conn.h"
#include "has_client.h"

#define LOG_LEVEL CONFIG_BT_HAS_CLIENT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_has_client);

DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_init, const struct bt_has_client_cb *);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_bind, struct bt_conn *, struct bt_has_client **);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_unbind, struct bt_has_client *);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_presets_read, struct bt_has_client *, uint8_t,
		       uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_set, struct bt_has_client *, uint8_t, bool);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_next, struct bt_has_client *, bool);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_prev, struct bt_has_client *, bool);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_write, struct bt_has_client *, uint8_t,
		       const char *);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_info_get, const struct bt_has_client *,
		       struct bt_has_client_info *);
DEFINE_FAKE_VALUE_FUNC(int, bt_has_client_conn_get, const struct bt_has_client *,
		       struct bt_conn **);
