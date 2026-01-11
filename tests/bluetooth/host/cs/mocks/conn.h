/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/conn_internal.h>

/* List of fakes used by this unit tester */
#define CONN_FFF_FAKES_LIST(FAKE)                                                                  \
	FAKE(bt_conn_unref)                                                                        \
	FAKE(bt_conn_lookup_handle)                                                                \
	FAKE(bt_conn_notify_remote_cs_capabilities)                                                \
	FAKE(bt_conn_notify_cs_config_created)                                                     \
	FAKE(bt_conn_notify_cs_config_removed)                                                     \
	FAKE(bt_conn_notify_cs_subevent_result)                                                    \
	FAKE(bt_conn_notify_cs_security_enable_available)                                          \
	FAKE(bt_conn_notify_cs_procedure_enable_available)                                         \
	FAKE(bt_conn_notify_remote_cs_fae_table)

DECLARE_FAKE_VOID_FUNC(bt_conn_unref, struct bt_conn *);
DECLARE_FAKE_VALUE_FUNC(struct bt_conn *, bt_conn_lookup_handle, uint16_t, enum bt_conn_type);
DECLARE_FAKE_VOID_FUNC(bt_conn_notify_remote_cs_capabilities, struct bt_conn *,
		       uint8_t, struct bt_conn_le_cs_capabilities *);
DECLARE_FAKE_VOID_FUNC(bt_conn_notify_remote_cs_fae_table, struct bt_conn *,
		       uint8_t, struct bt_conn_le_cs_fae_table *);
DECLARE_FAKE_VOID_FUNC(bt_conn_notify_cs_config_created, struct bt_conn *,
		       uint8_t, struct bt_conn_le_cs_config *);
DECLARE_FAKE_VOID_FUNC(bt_conn_notify_cs_config_removed, struct bt_conn *, uint8_t);
DECLARE_FAKE_VOID_FUNC(bt_conn_notify_cs_subevent_result, struct bt_conn *,
		       struct bt_conn_le_cs_subevent_result *);
DECLARE_FAKE_VOID_FUNC(bt_conn_notify_cs_security_enable_available, struct bt_conn *, uint8_t);
DECLARE_FAKE_VOID_FUNC(bt_conn_notify_cs_procedure_enable_available, struct bt_conn *,
		       uint8_t, struct bt_conn_le_cs_procedure_enable_complete *);
