/* test_common.h */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void test_mocks_init(void);
void test_mocks_cleanup(void);

void test_conn_init(struct bt_conn *conn);
