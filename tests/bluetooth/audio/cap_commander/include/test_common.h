/* test_common.h */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>

#define BROADCAST_CODE "BroadcastCode"
#define RANDOM_SRC_ID  0x55

void test_mocks_init(void);
void test_mocks_cleanup(void);

void test_conn_init(struct bt_conn *conn);
