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
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/sync.h"
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
	ARG_UNUSED(attr);

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

/* Prepare Write Requests the write method of TEST_PREP_CHRC_UUID accepted */
static atomic_t prepared;

static ssize_t write_prep_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			       const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(attr);
	ARG_UNUSED(buf);

	LOG_INF("Write from %s: len %u offset %u flags 0x%02x", bt_conn_dst_str(conn), len, offset,
		flags);

	if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
		(void)atomic_inc(&prepared);

		return 0;
	}

	return BT_GATT_ERR(EXEC_FAIL_ERR);
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_CHRC_UUID, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE,
			       NULL, write_chrc, NULL),
	BT_GATT_CHARACTERISTIC(TEST_PREP_CHRC_UUID, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
			       NULL, write_prep_chrc, NULL),
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

/* Execute Write Requests reported, by outcome */
static atomic_t ended[BT_GATT_EXECUTE_WRITE_FAILED + 1];
static atomic_t ended_err;

static void execute_write_ended(struct bt_conn *conn, enum bt_gatt_execute_write_outcome outcome,
				uint8_t att_err)
{
	LOG_INF("Execute Write of %s ended: outcome %d err 0x%02x", bt_conn_dst_str(conn), outcome,
		att_err);

	TEST_ASSERT(outcome < ARRAY_SIZE(ended), "Unknown outcome %d", outcome);
	/* The error of a failure is checked by the step that causes it */
	TEST_ASSERT((outcome == BT_GATT_EXECUTE_WRITE_FAILED) || (att_err == 0U),
		    "Outcome %d came with ATT error 0x%02x", outcome, att_err);

	(void)atomic_set(&ended_err, att_err);
	(void)atomic_inc(&ended[outcome]);
}

/* Waits for the client to finish a step, checks how many Execute Write Requests
 * ended with each outcome so far, and lets the client go on
 */
static void check_ended(const char *step, long written, long cancelled, long failed)
{
	bk_sync_wait();

	TEST_ASSERT((atomic_get(&ended[BT_GATT_EXECUTE_WRITE_WRITTEN]) == written) &&
		    (atomic_get(&ended[BT_GATT_EXECUTE_WRITE_CANCELLED]) == cancelled) &&
		    (atomic_get(&ended[BT_GATT_EXECUTE_WRITE_FAILED]) == failed),
		    "After %s: written %ld cancelled %ld failed %ld, expected %ld %ld %ld", step,
		    (long)atomic_get(&ended[BT_GATT_EXECUTE_WRITE_WRITTEN]),
		    (long)atomic_get(&ended[BT_GATT_EXECUTE_WRITE_CANCELLED]),
		    (long)atomic_get(&ended[BT_GATT_EXECUTE_WRITE_FAILED]), written, cancelled,
		    failed);

	bk_sync_send();
}

void execute_server_procedure(void)
{
	static struct bt_gatt_cb gatt_cb = {
		.execute_write_ended = execute_write_ended,
	};
	struct bt_conn *conn;
	uint8_t expected[CHRC_SIZE];
	int err;

	TEST_START("execute_server");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Sync init failed");

	bt_gatt_cb_register(&gatt_cb);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, SERVER_NAME);
	TEST_ASSERT(err == 0, "Advertising failed (err %d)", err);

	/* Each outcome is reported before the response, which the client has
	 * when it signals
	 */
	check_ended("the cancel", 0, 1, 0);
	check_ended("the cancel of an empty queue", 0, 1, 0);

	check_ended("the gap between the offsets", 0, 1, 1);
	TEST_ASSERT(atomic_get(&ended_err) == BT_ATT_ERR_INVALID_OFFSET,
		    "The gap failed with 0x%02lx", (long)atomic_get(&ended_err));

	check_ended("the failing write method", 0, 1, 2);
	TEST_ASSERT(atomic_get(&ended_err) == EXEC_FAIL_ERR,
		    "The failing write method failed with 0x%02lx", (long)atomic_get(&ended_err));
	TEST_ASSERT(!IS_FLAG_SET(flag_written), "A value after the failing one was written");

	/* The method is not told of the value the queue has no room for */
	check_ended("the full queue", 0, 2, 2);
	TEST_ASSERT(atomic_get(&prepared) == 1 + PREP_QUOTA,
		    "The write method accepted %ld prepared values, expected %d",
		    (long)atomic_get(&prepared), 1 + PREP_QUOTA);

	err = bt_testlib_wait_disconnected(conn);
	TEST_ASSERT(err == 0, "Waiting for the client to disconnect failed (err %d)", err);

	/* The long write executed last was written */
	TEST_ASSERT((atomic_get(&ended[BT_GATT_EXECUTE_WRITE_WRITTEN]) == 1) &&
		    (atomic_get(&ended[BT_GATT_EXECUTE_WRITE_CANCELLED]) == 2) &&
		    (atomic_get(&ended[BT_GATT_EXECUTE_WRITE_FAILED]) == 2),
		    "The executed long write was not reported once as written");
	TEST_ASSERT(IS_FLAG_SET(flag_written), "The executed long write was not written");

	writer_value_fill(expected, sizeof(expected));
	TEST_ASSERT(memcmp(chrc_value, expected, sizeof(expected)) == 0,
		    "Written value differs from the client's value");

	TEST_PASS("execute_server");
}
