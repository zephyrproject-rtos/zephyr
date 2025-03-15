/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_CONN_H_
#define MOCKS_CONN_H_
#include <stdint.h>

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>

/* List of fakes used by this unit tester */
#define CONN_FFF_FAKES_LIST(FAKE)                                                                  \
	FAKE(bt_conn_foreach)                                                                      \
	FAKE(bt_conn_get_dst)                                                                      \
	FAKE(bt_foreach_bond)

typedef void (*bt_conn_foreach_cb)(struct bt_conn *, void *);
typedef void (*bt_foreach_bond_cb)(const struct bt_bond_info *, void *);
DECLARE_FAKE_VOID_FUNC(bt_conn_foreach, enum bt_conn_type, bt_conn_foreach_cb, void *);
DECLARE_FAKE_VALUE_FUNC(const bt_addr_le_t *, bt_conn_get_dst, const struct bt_conn *);
DECLARE_FAKE_VOID_FUNC(bt_foreach_bond, uint8_t, bt_foreach_bond_cb, void *);

struct bt_conn {
	uint8_t index;
	struct bt_conn_info info;
	struct bt_iso_chan *chan;
};

void mock_bt_conn_connected(struct bt_conn *conn, uint8_t err);
void mock_bt_conn_disconnected(struct bt_conn *conn, uint8_t err);

#endif /* MOCKS_CONN_H_ */
