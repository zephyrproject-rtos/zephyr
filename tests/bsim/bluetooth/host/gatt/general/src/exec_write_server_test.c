/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "common.h"
#include "testlib/adv.h"

DEFINE_FLAG_STATIC(flag_test_done);

static uint8_t first_exec_cnt;
static uint8_t second_exec_cnt;
static bool fail_first_exec;

static ssize_t write_first_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if ((flags & BT_GATT_WRITE_FLAG_EXECUTE) != 0U) {
		first_exec_cnt++;

		if (fail_first_exec) {
			return BT_GATT_ERR(EXEC_WRITE_APP_ERR);
		}
	}

	return len;
}

static ssize_t write_second_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	if ((flags & BT_GATT_WRITE_FLAG_EXECUTE) != 0U) {
		second_exec_cnt++;
	}

	return len;
}

static ssize_t write_cp_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *stage = buf;

	if (len != 1U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	switch (stage[0]) {
	case EXEC_WRITE_STAGE_CHECK_CANCEL:
		TEST_ASSERT(first_exec_cnt == 0U, "Cancel wrote to the first characteristic");
		TEST_ASSERT(second_exec_cnt == 0U, "Cancel wrote to the second characteristic");

		fail_first_exec = true;

		break;
	case EXEC_WRITE_STAGE_CHECK_ERROR:
		TEST_ASSERT(first_exec_cnt == 1U, "Error write skipped the first characteristic");
		TEST_ASSERT(second_exec_cnt == 0U, "Error write reached the second characteristic");

		SET_FLAG(flag_test_done);

		break;
	default:
		TEST_FAIL("Invalid stage %u", stage[0]);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(exec_write_svc,
		       BT_GATT_PRIMARY_SERVICE(EXEC_WRITE_SVC_UUID),
		       BT_GATT_CHARACTERISTIC(EXEC_WRITE_FIRST_CHRC_UUID,
					      BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE, NULL,
					      write_first_chrc, NULL),
		       BT_GATT_CHARACTERISTIC(EXEC_WRITE_SECOND_CHRC_UUID,
					      BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE, NULL,
					      write_second_chrc, NULL),
		       BT_GATT_CHARACTERISTIC(EXEC_WRITE_CP_CHRC_UUID,
					      BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE, NULL,
					      write_cp_chrc, NULL));

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);

		return;
	}

	err = bt_testlib_adv_conn(NULL, BT_ID_DEFAULT, bt_get_name());
	if (err != 0) {
		TEST_FAIL("Could not start advertising: %d", err);

		return;
	}

	WAIT_FOR_FLAG(flag_test_done);

	TEST_PASS("GATT server passed");
}

static const struct bst_test_instance test_exec_write_server[] = {
	{
		.test_id = "exec_write_server",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_exec_write_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_exec_write_server);
}
