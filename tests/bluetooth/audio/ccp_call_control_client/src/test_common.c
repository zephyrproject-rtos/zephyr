/* common.c - Common functions */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/ztest_test.h>

#include "ccp_call_control_client.h"
#include "conn.h"

void test_mocks_init(const struct ztest_unit_test *test, void *fixture)
{
	mock_ccp_call_control_client_init();
}

void test_mocks_cleanup(const struct ztest_unit_test *test, void *fixture)
{
	mock_ccp_call_control_client_cleanup();
}

void test_conn_init(struct bt_conn *conn)
{
	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_CENTRAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;

	mock_bt_conn_connected(conn, BT_HCI_ERR_SUCCESS);
}
