/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <testlib/att_read.h>
#include <testlib/conn.h>
#include <testlib/scan.h>
#include <testlib/security.h>

#include "babblekit/testcase.h"
#include "../common_defs.h"

LOG_MODULE_REGISTER(client, LOG_LEVEL_DBG);

#define BT_LOCAL_ATT_MTU_EATT MIN(BT_L2CAP_SDU_RX_MTU, BT_L2CAP_SDU_TX_MTU)
#define BT_LOCAL_ATT_MTU_UATT MIN(BT_L2CAP_RX_MTU, BT_L2CAP_TX_MTU)
#define BT_ATT_BUF_SIZE       MAX(BT_LOCAL_ATT_MTU_UATT, BT_LOCAL_ATT_MTU_EATT)

void test_long_read(struct bt_conn *conn, enum bt_att_chan_opt bearer)
{
	/* The handle will be discovered in the first switch case. */
	uint16_t handle = 0;

	/* Loop over the switch cases. */
	for (int i = 0; i < 3; i++) {
		int err;
		uint16_t actual_read_len;
		uint16_t remote_read_send_len;

		NET_BUF_SIMPLE_DEFINE(attr_value, sizeof(remote_read_send_len));

		/* Exercise various ATT read operations. */
		switch (i) {
		case 0:
			LOG_INF("ATT_READ_BY_TYPE");
			/* Aka. "read by uuid". */
			err = bt_testlib_att_read_by_type_sync(&attr_value, &actual_read_len,
							       &handle, NULL, conn, bearer,
							       MTU_VALIDATION_CHRC, 1, 0xffff);
			break;
		case 1:
			LOG_INF("ATT_READ");
			/* Arg `offset == 0`: the stack should choose ATT_READ PDU. */
			err = bt_testlib_att_read_by_handle_sync(&attr_value, &actual_read_len,
								 NULL, conn, bearer, handle, 0);
			break;
		case 2:
			LOG_INF("ATT_READ_BLOB");
			/* Arg `offset != 0`: the stack should choose ATT_READ_BLOB PDU. */
			err = bt_testlib_att_read_by_handle_sync(&attr_value, &actual_read_len,
								 NULL, conn, bearer, handle, 1);
			break;
		}

		__ASSERT_NO_MSG(!err);
		__ASSERT(attr_value.len >= sizeof(remote_read_send_len),
			 "Remote sent too little data.");
		remote_read_send_len = net_buf_simple_pull_le16(&attr_value);
		__ASSERT(remote_read_send_len == actual_read_len, "Length mismatch. %u %u",
			 remote_read_send_len, actual_read_len);
	}
}

static void test_cli_main(void)
{
	bt_addr_le_t scan_result;
	int err;
	struct bt_conn *conn = NULL;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_scan_find_name(&scan_result, "d1");
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_connect(&scan_result, &conn);
	__ASSERT_NO_MSG(!err);

	/* Establish EATT bearers. */
	err = bt_testlib_secure(conn, BT_SECURITY_L2);
	__ASSERT_NO_MSG(!err);

	while (bt_eatt_count(conn) == 0) {
		LOG_DBG("E..");
		k_msleep(100);
	};
	LOG_DBG("EATT");

	test_long_read(conn, BT_ATT_CHAN_OPT_UNENHANCED_ONLY);
	test_long_read(conn, BT_ATT_CHAN_OPT_ENHANCED_ONLY);

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	__ASSERT_NO_MSG(!err);

	bt_conn_unref(conn);
	conn = NULL;

	TEST_PASS("PASS");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "cli",
		.test_main_f = test_cli_main,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
};

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();

	return 0;
}
