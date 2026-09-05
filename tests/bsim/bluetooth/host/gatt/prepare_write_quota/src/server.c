/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "testlib/adv.h"
#include "testlib/conn.h"

#include "common.h"

LOG_MODULE_REGISTER(server, LOG_LEVEL_INF);

DEFINE_FLAG_STATIC(flag_written);

static uint8_t chrc_value[CHRC_SIZE];

static ssize_t write_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_INF("Write from %s: len %u offset %u flags 0x%02x", bt_conn_dst_str(conn), len, offset,
		flags);

	/* Without BT_GATT_PERM_PREPARE_WRITE only executed writes get here, and
	 * the writer's value arrives reassembled into a single write.
	 */
	TEST_ASSERT(offset == 0 && len == sizeof(chrc_value),
		    "Unexpected write of %u octets at offset %u", len, offset);

	(void)memcpy(chrc_value, buf, len);
	SET_FLAG(flag_written);

	return len;
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE,
			       NULL, write_chrc, NULL),
);

void server_procedure(void)
{
	struct bt_conn *conns[CONFIG_BT_MAX_CONN] = {};
	uint8_t expected[CHRC_SIZE];
	int err;

	TEST_START("server");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	/* Advertise until both clients are connected: the holder first, then the writer */
	ARRAY_FOR_EACH_PTR(conns, conn) {
		err = bt_testlib_adv_conn(conn, BT_ID_DEFAULT, SERVER_NAME);
		TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

		LOG_INF("Connected to %s", bt_conn_dst_str(*conn));
	}

	/* The writer disconnects once its write attempt is over */
	err = bt_testlib_wait_disconnected(conns[ARRAY_SIZE(conns) - 1]);
	TEST_ASSERT(err == 0, "Waiting for the writer to disconnect failed (err %d)", err);

	TEST_ASSERT(IS_FLAG_SET(flag_written) == WRITER_EXPECT_SUCCESS,
		    "The writer's value %s written", IS_FLAG_SET(flag_written) ? "was" : "was not");

	if (WRITER_EXPECT_SUCCESS) {
		writer_value_fill(expected, sizeof(expected));
		TEST_ASSERT(memcmp(chrc_value, expected, sizeof(expected)) == 0,
			    "Written value differs from the writer's value");
	}

	TEST_PASS("server");
}
