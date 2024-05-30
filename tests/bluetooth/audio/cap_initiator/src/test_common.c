/* test_common.c - common procedures for unit test of CAP initiator */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cap_initiator.h"
#include "conn.h"
#include "test_common.h"

void test_mocks_init(void)
{
	mock_cap_initiator_init();
}

void test_mocks_cleanup(void)
{
	mock_cap_initiator_cleanup();
}

void test_conn_init(struct bt_conn *conn)
{
	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_PERIPHERAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;
}
