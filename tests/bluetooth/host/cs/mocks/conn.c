/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/conn.h"

#include <zephyr/kernel.h>

DEFINE_FAKE_VOID_FUNC(bt_conn_unref, struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(struct bt_conn *, bt_conn_lookup_handle, uint16_t, enum bt_conn_type);
DEFINE_FAKE_VOID_FUNC(bt_conn_notify_remote_cs_capabilities, struct bt_conn *,
		      uint8_t, struct bt_conn_le_cs_capabilities *);
DEFINE_FAKE_VOID_FUNC(bt_conn_notify_remote_cs_fae_table, struct bt_conn *,
		      uint8_t, struct bt_conn_le_cs_fae_table *);
DEFINE_FAKE_VOID_FUNC(bt_conn_notify_cs_config_created, struct bt_conn *,
		      uint8_t, struct bt_conn_le_cs_config *);
DEFINE_FAKE_VOID_FUNC(bt_conn_notify_cs_config_removed, struct bt_conn *, uint8_t);
DEFINE_FAKE_VOID_FUNC(bt_conn_notify_cs_subevent_result, struct bt_conn *,
		      struct bt_conn_le_cs_subevent_result *);
DEFINE_FAKE_VOID_FUNC(bt_conn_notify_cs_security_enable_available, struct bt_conn *, uint8_t);
DEFINE_FAKE_VOID_FUNC(bt_conn_notify_cs_procedure_enable_available, struct bt_conn *,
		      uint8_t, struct bt_conn_le_cs_procedure_enable_complete *);
