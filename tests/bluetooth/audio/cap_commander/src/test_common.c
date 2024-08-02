/* test_common.c - common procedures for unit test of CAP commander */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cap_commander.h"
#include "conn.h"
#include "cap_mocks.h"
#include "test_common.h"

void test_mocks_init(void)
{
	mock_cap_commander_init();
	mock_bt_aics_init();
	mock_bt_csip_init();
	mock_bt_micp_init();
	mock_bt_vcp_init();
	mock_bt_vocs_init();
}

void test_mocks_cleanup(void)
{
	mock_cap_commander_cleanup();
	mock_bt_aics_cleanup();
	mock_bt_csip_cleanup();
	mock_bt_micp_cleanup();
	mock_bt_vcp_cleanup();
	mock_bt_vocs_cleanup();
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
