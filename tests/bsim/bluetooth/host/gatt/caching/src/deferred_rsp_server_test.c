/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>

#include "host/att_internal.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "deferred_rsp.h"

DEFINE_FLAG_STATIC(flag_is_connected);
DEFINE_FLAG_STATIC(flag_is_encrypted);
DEFINE_FLAG_STATIC(flag_test_done);

static struct bt_conn *g_conn;

static uint8_t chrc_value[VALUE_SIZE];
static uint8_t chrc2_value[VALUE2_SIZE];
static uint8_t sync_value[SYNC_VALUE_SIZE];
static uint8_t fill_value[FILL_VALUE_SIZE];

/* Synchronous value written by its own method, prepared alongside a deferred
 * one so an Execute Write queue holds both (STAGE_EXEC_MIXED).
 */
static uint8_t mixed_value[WRITE_SIZE];

static void fill_values(void)
{
	for (size_t i = 0U; i < sizeof(chrc_value); i++) {
		chrc_value[i] = (uint8_t)i;
	}

	for (size_t i = 0U; i < sizeof(chrc2_value); i++) {
		chrc2_value[i] = (uint8_t)(0x80U + i);
	}

	for (size_t i = 0U; i < sizeof(sync_value); i++) {
		sync_value[i] = (uint8_t)(0xf0U + i);
	}

	for (size_t i = 0U; i < sizeof(fill_value); i++) {
		fill_value[i] = (uint8_t)(0x40U + i);
	}
}

static enum test_stage stage;
static bool command_seen;
static bool execute_seen;
static bool mixed_seen;
static bool use_eatt;

/* Values of the Execute Write in STAGE_EXEC and STAGE_EXEC_ERR seen so far */
static uint8_t exec_cnt;

/* Set from the call for one value of an Execute Write until it is responded to */
static bool exec_pending;

/* Set while a Prepare Write is deferred, so that an Execute Write applying its
 * value before the application has accepted it is caught
 */
static bool prep_deferred;

/* The request is only valid during the callback, so it is copied */
static struct bt_gatt_req pending_req;
static const uint8_t *pending_value;
static uint16_t pending_len;
static uint8_t pending_err;
static bool pending_is_read;

static void answer_work_handler(struct k_work *work)
{
	/* Responding to one value of a Read Multiple can ask for the next one
	 * before returning, which overwrites the pending state
	 */
	struct bt_gatt_req req = pending_req;
	const uint8_t *value = pending_value;
	uint16_t len = pending_len;
	uint8_t att_err = pending_err;
	bool is_read = pending_is_read;
	int err;

	if (att_err != 0U) {
		exec_pending = false;
		err = bt_gatt_error_rsp(&req, BT_GATT_ERR(att_err));
	} else if (is_read) {
		err = bt_gatt_read_rsp(&req, value, len);
	} else {
		/* Accepted, so an Execute Write may now apply the value */
		prep_deferred = false;
		exec_pending = false;
		err = bt_gatt_write_rsp(&req);
	}

	if (err != 0) {
		TEST_FAIL("Deferred response failed (err %d)", err);

		return;
	}

	/* The request is done, the same token must not respond again */
	err = bt_gatt_error_rsp(&req, BT_GATT_ERR(TEST_DEFERRED_ERR));
	if (err != -EINVAL) {
		TEST_FAIL("Stale request token accepted (err %d)", err);
	}
}

static K_WORK_DELAYABLE_DEFINE(answer_work, answer_work_handler);

/* STAGE_BEARERS defers a read on every bearer before responding to any of them */
static struct bt_gatt_req bearer_req[BEARER_SLOTS];
static uint16_t bearer_offset[BEARER_SLOTS];

static void bearer_work_handler(struct k_work *work)
{
	/* Responded to in reverse, so that a response cannot be the one the bearer
	 * that asked last is waiting for.
	 */
	for (uint8_t i = BEARER_SLOTS; i > 0U; i--) {
		uint16_t offset = bearer_offset[i - 1];
		int err;

		err = bt_gatt_read_rsp(&bearer_req[i - 1], &chrc_value[offset],
				       sizeof(chrc_value) - offset);
		if (err != 0) {
			TEST_FAIL("Deferred response %u failed (err %d)", i - 1, err);

			return;
		}

		/* That request is done, its token must not respond again */
		err = bt_gatt_error_rsp(&bearer_req[i - 1], BT_GATT_ERR(TEST_DEFERRED_ERR));
		if (err != -EINVAL) {
			TEST_FAIL("Stale request token %u accepted (err %d)", i - 1, err);

			return;
		}
	}
}

static K_WORK_DELAYABLE_DEFINE(bearer_work, bearer_work_handler);

/* STAGE_PREP_REORDER defers a Prepare Write on every bearer before accepting
 * any of them, so that the host holds more than one unaccepted prepare of the
 * same handle at once, each numbered on arrival.
 */
static struct bt_gatt_req prep_req[PREP_SLOTS];
static uint16_t prep_offset[PREP_SLOTS];
static uint8_t prep_cnt;
static bool reorder_exec_seen;

static void prep_reorder_work_handler(struct k_work *work)
{
	/* Accepted in reverse arrival order: the value that arrived last is
	 * accepted first. The queue must still execute in arrival order, so
	 * the fragment at offset 0 is applied before the one after it.
	 */
	for (uint8_t i = PREP_SLOTS; i > 0U; i--) {
		int err;

		err = bt_gatt_write_rsp(&prep_req[i - 1]);
		if (err != 0) {
			TEST_FAIL("Prepare accept %u failed (err %d)", i - 1, err);

			return;
		}

		/* That request is done, its token must not respond again */
		err = bt_gatt_error_rsp(&prep_req[i - 1], BT_GATT_ERR(TEST_DEFERRED_ERR));
		if (err != -EINVAL) {
			TEST_FAIL("Stale prepare token %u accepted (err %d)", i - 1, err);

			return;
		}
	}

	/* Both accepted, so the Execute Write may now apply their values */
	prep_deferred = false;
}

static K_WORK_DELAYABLE_DEFINE(prep_reorder_work, prep_reorder_work_handler);

/* More than att_pool holds in this image */
#define HOLD_MAX 16U

/* STAGE_MULT_SYNC leaves a single ATT buffer free, which the response of a
 * synchronous Read Multiple takes, so it must not need another one
 */
static struct net_buf *held[HOLD_MAX];
static size_t held_cnt;

static void hold_work_handler(struct k_work *work)
{
	struct net_buf *buf;

	/* This runs on the system work queue, where the allocation never waits */
	while (held_cnt < ARRAY_SIZE(held)) {
		buf = bt_att_create_pdu(g_conn, BT_ATT_OP_NOTIFY, 0);
		if (buf == NULL) {
			break;
		}

		held[held_cnt] = buf;
		held_cnt++;
	}

	if ((held_cnt == 0U) || (held_cnt == ARRAY_SIZE(held))) {
		TEST_FAIL("Holding %zu ATT buffers", held_cnt);

		return;
	}

	held_cnt--;
	net_buf_unref(held[held_cnt]);
}

static K_WORK_DELAYABLE_DEFINE(hold_work, hold_work_handler);

static void release_held(void)
{
	while (held_cnt > 0U) {
		held_cnt--;
		net_buf_unref(held[held_cnt]);
	}
}

static int read_deferred_chrc(const struct bt_gatt_req *req, const struct bt_gatt_attr *attr,
			   uint16_t offset, uint16_t max_len)
{
	printk("deferred read offset %u max_len %u\n", offset, max_len);

	if (offset > sizeof(chrc_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (stage == STAGE_BEARERS) {
		static uint8_t bearer_cnt;

		if (bearer_cnt >= BEARER_SLOTS) {
			TEST_FAIL("More than %u requests deferred at once", BEARER_SLOTS);

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		bearer_req[bearer_cnt] = *req;
		bearer_offset[bearer_cnt] = offset;
		bearer_cnt++;

		/* Held until every bearer has one, which only works if the
		 * host keeps the deferred state per bearer.
		 */
		if (bearer_cnt == BEARER_SLOTS) {
			k_work_schedule(&bearer_work, K_MSEC(10));
		}

		return 0;
	}

	if (stage == STAGE_MULT_INLINE_ERR) {
		/* Responded to from inside the callback */
		if (bt_gatt_error_rsp(req, BT_GATT_ERR(TEST_DEFERRED_ERR)) != 0) {
			TEST_FAIL("Inline response failed");

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		return 0;
	}

	if (stage == STAGE_READ) {
		/* Not BT_GATT_ERR() of an ATT error, and the request stays open */
		if ((bt_gatt_error_rsp(req, TEST_DEFERRED_ERR) != -EINVAL) ||
		    (bt_gatt_error_rsp(req, 0) != -EINVAL) ||
		    (bt_gatt_error_rsp(req, -0x100) != -EINVAL)) {
			TEST_FAIL("An invalid error code was accepted");

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	}

	pending_req = *req;
	pending_value = &chrc_value[offset];
	pending_len = sizeof(chrc_value) - offset;
	pending_is_read = true;
	pending_err = (stage == STAGE_ERROR || stage == STAGE_MULT_ERROR) ? TEST_DEFERRED_ERR : 0U;

	if ((stage == STAGE_PIPELINE) || (stage == STAGE_PIPELINE_EATT) || (stage == STAGE_PREP0)) {
		/* Left unanswered: the bearer goes away first */
		return 0;
	}

	k_work_schedule(&answer_work, K_MSEC(10));

	return 0;
}

static int read_deferred_chrc2(const struct bt_gatt_req *req, const struct bt_gatt_attr *attr,
			    uint16_t offset, uint16_t max_len)
{
	printk("deferred read 2 offset %u max_len %u\n", offset, max_len);

	if (offset > sizeof(chrc2_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	pending_req = *req;
	pending_value = &chrc2_value[offset];
	pending_len = sizeof(chrc2_value) - offset;
	pending_is_read = true;
	pending_err = 0U;

	k_work_schedule(&answer_work, K_MSEC(10));

	return 0;
}

static ssize_t read_sync_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, sync_value, sizeof(sync_value));
}

static ssize_t read_fill_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, fill_value, sizeof(fill_value));
}

/* One value of an Execute Write, after the previous one has been responded to */
static int exec_value(const struct bt_gatt_req *req, const struct bt_gatt_attr *attr,
		      uint16_t len, uint16_t offset)
{
	const struct bt_uuid *expected = TEST_DEFERRED_CHRC_UUID;

	if (exec_pending) {
		TEST_FAIL("Next execute write value before the previous was responded to");

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if ((offset != 0U) || (len != WRITE_SIZE)) {
		TEST_FAIL("Unexpected execute write len %u offset %u", len, offset);

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	pending_req = *req;
	pending_is_read = false;
	pending_err = 0U;

	if (stage == STAGE_EXEC_EMPTY) {
		TEST_FAIL("A value survived the failed execute write");

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if ((stage == STAGE_EXEC) || (stage == STAGE_EXEC_ERR)) {
		/* In the order the client prepared them */
		if (exec_cnt == 1U) {
			expected = TEST_DEFERRED2_CHRC_UUID;
		}

		if ((exec_cnt > 1U) || (bt_uuid_cmp(attr->uuid, expected) != 0)) {
			TEST_FAIL("Execute write value %u is for the wrong attribute", exec_cnt);

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		if ((stage == STAGE_EXEC_ERR) && (exec_cnt == 1U)) {
			pending_err = TEST_DEFERRED_ERR;
		}

		exec_cnt++;
	}

	execute_seen = true;
	exec_pending = true;
	k_work_schedule(&answer_work, K_MSEC(10));

	return 0;
}

static int write_deferred_chrc(const struct bt_gatt_req *req, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset,
			    enum bt_gatt_attr_write_flag flags)
{
	printk("deferred write len %u offset %u flags 0x%02x id %u\n", len, offset, flags, req->id);

	if (req->conn != g_conn) {
		TEST_FAIL("Write for an unknown connection");

		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if ((flags & BT_GATT_WRITE_FLAG_CMD) != 0U) {
		/* No response to send, so the request cannot be responded to */
		if ((req->id != 0U) || (bt_gatt_write_rsp(req) != -EINVAL)) {
			TEST_FAIL("A write without a response can be responded to");

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		command_seen = true;

		return 0;
	}

	if (req->id == 0U) {
		/* An Execute Write whose queue also holds a synchronous value
		 * is not deferred, so the deferred attribute's method is called
		 * with id 0 and must write before it returns, like a Command.
		 */
		if ((flags & BT_GATT_WRITE_FLAG_EXECUTE) == 0U) {
			TEST_FAIL("A request with a response has no id");

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		if (bt_gatt_write_rsp(req) != -EINVAL) {
			TEST_FAIL("An id 0 Execute Write can be responded to");

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		if (stage == STAGE_EXEC_MIXED_ERR) {
			return BT_GATT_ERR(TEST_DEFERRED_ERR);
		}

		if ((offset != 0U) || (len != WRITE_SIZE)) {
			TEST_FAIL("Mixed execute write len %u offset %u", len, offset);

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		/* Write the deferred value of the mixed Execute into its own
		 * store, so the shared chrc_value stays as the other stages
		 * expect it.
		 */
		memcpy(mixed_value, buf, len);
		mixed_seen = true;

		return 0;
	}

	if ((flags & BT_GATT_WRITE_FLAG_EXECUTE) != 0U) {
		if (prep_deferred) {
			TEST_FAIL("Execute write applied an unaccepted prepare");

			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		if (stage == STAGE_PREP_REORDER) {
			const uint8_t *value = buf;

			/* One reassembled write of both fragments, in the order
			 * the client sent them, whatever order they were
			 * accepted in. The old code queued the accepted value,
			 * so reverse acceptance reversed the fragments.
			 */
			if ((offset != 0U) || (len != (2U * WRITE_SIZE))) {
				TEST_FAIL("Reordered execute len %u offset %u", len, offset);

				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			}

			for (uint16_t i = 0U; i < WRITE_SIZE; i++) {
				if ((value[i] != (uint8_t)(REORDER_FIRST_BYTE + i)) ||
				    (value[WRITE_SIZE + i] !=
				     (uint8_t)(REORDER_SECOND_BYTE + i))) {
					TEST_FAIL("Reordered execute byte %u is 0x%02x 0x%02x", i,
						  value[i], value[WRITE_SIZE + i]);

					return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
				}
			}

			reorder_exec_seen = true;
			pending_req = *req;
			pending_is_read = false;
			pending_err = 0U;
			k_work_schedule(&answer_work, K_MSEC(10));

			return 0;
		}

		return exec_value(req, attr, len, offset);
	}

	if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
		prep_deferred = true;

		if (stage == STAGE_PREP_REORDER) {
			if (prep_cnt >= PREP_SLOTS) {
				TEST_FAIL("More than %u prepares deferred at once", PREP_SLOTS);

				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			}

			prep_req[prep_cnt] = *req;
			prep_offset[prep_cnt] = offset;
			prep_cnt++;

			/* Held until both bearers have one, so the host carries
			 * two unaccepted prepares of this handle at the same
			 * time, then accepted in reverse arrival order.
			 */
			if (prep_cnt == PREP_SLOTS) {
				k_work_schedule(&prep_reorder_work, K_MSEC(10));
			}

			return 0;
		}
	}

	pending_req = *req;
	pending_is_read = false;
	pending_err = 0U;

	/* STAGE_PREP_EXEC needs this one to still be deferred when the Execute
	 * Write the client sends on the other bearer arrives
	 */
	k_work_schedule(&answer_work, (stage == STAGE_PREP_EXEC) ? K_MSEC(200) : K_MSEC(10));

	return 0;
}

static ssize_t write_syncprep_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);

	if ((flags & BT_GATT_WRITE_FLAG_PREPARE) != 0U) {
		/* Only queued now; the value is written on the Execute Write */
		return 0;
	}

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(mixed_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* The host writes this synchronous value itself, so there is nothing
	 * to defer even when it shares an Execute Write with a deferred one.
	 */
	memcpy(mixed_value, buf, len);

	return len;
}

static ssize_t write_cp_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *value = buf;
	int err;

	if (len != 1U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	stage = value[0];
	exec_cnt = 0U;
	printk("stage %u\n", stage);

	release_held();
	if (stage == STAGE_MULT_SYNC) {
		/* Once the response to this write has been sent */
		k_work_schedule(&hold_work, K_MSEC(300));
	}

	if ((stage == STAGE_PIPELINE) && use_eatt) {
		/* Only the enhanced bearer went, and its request with it */
		err = bt_gatt_read_rsp(&pending_req, chrc_value, sizeof(chrc_value));
		if ((bt_eatt_count(conn) != 0U) || (err != -EINVAL)) {
			TEST_FAIL("%u enhanced bearers, late rsp %d", bt_eatt_count(conn), err);
		}
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(test_deferred_svc, BT_GATT_PRIMARY_SERVICE(TEST_DEFERRED_SERVICE_UUID),
		       BT_GATT_CHARACTERISTIC_DEFERRED(TEST_DEFERRED_CHRC_UUID,
						    BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
						    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
							    BT_GATT_PERM_PREPARE_WRITE,
						    read_deferred_chrc, write_deferred_chrc, NULL),
		       BT_GATT_CHARACTERISTIC(TEST_CP_CHRC_UUID, BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE, NULL, write_cp_chrc, NULL),
		       BT_GATT_CHARACTERISTIC_DEFERRED(TEST_DEFERRED2_CHRC_UUID,
						    BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
						    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
							    BT_GATT_PERM_PREPARE_WRITE,
						    read_deferred_chrc2, write_deferred_chrc, NULL),
		       BT_GATT_CHARACTERISTIC(TEST_SYNC_CHRC_UUID, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_sync_chrc, NULL, NULL),
		       BT_GATT_CHARACTERISTIC(TEST_SYNCPREP_CHRC_UUID, BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
					      NULL, write_syncprep_chrc, NULL),
		       BT_GATT_CHARACTERISTIC(TEST_FILL_CHRC_UUID, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_fill_chrc, NULL, NULL));

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);

		return;
	}

	g_conn = bt_conn_ref(conn);

	SET_FLAG(flag_is_connected);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err security_err)
{
	if (security_err == BT_SECURITY_ERR_SUCCESS && level > BT_SECURITY_L1) {
		SET_FLAG(flag_is_encrypted);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;

	printk("Disconnected: %s (reason 0x%02x)\n", bt_conn_dst_str(conn), reason);

	bt_conn_drop(&g_conn);

	if (stage == STAGE_PREP0) {
		/* The enhanced bearer was dropped for the pipelined prepare;
		 * the client ends the test by dropping the connection
		 */
		SET_FLAG(flag_test_done);

		return;
	}

	if (stage != STAGE_PIPELINE) {
		TEST_FAIL("Disconnected in stage %u", stage);

		return;
	}

	/* The bearer the request arrived on is gone */
	err = bt_gatt_read_rsp(&pending_req, chrc_value, sizeof(chrc_value));
	if (err != -ENOTCONN) {
		TEST_FAIL("Deferred response accepted after disconnect (err %d)", err);

		return;
	}

	if (!command_seen) {
		TEST_FAIL("Write command was never seen");

		return;
	}

	if (!execute_seen) {
		TEST_FAIL("Execute write was never seen");

		return;
	}

	SET_FLAG(flag_test_done);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void test_main_common(bool connect_eatt)
{
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};
	int err;

	fill_values();
	use_eatt = connect_eatt;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err != 0) {
		TEST_FAIL("Bluetooth init failed (err %d)", err);

		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		TEST_FAIL("Advertising failed to start (err %d)", err);

		return;
	}

	if (connect_eatt) {
		WAIT_FOR_FLAG(flag_is_connected);
		WAIT_FOR_FLAG(flag_is_encrypted);

		err = bt_eatt_connect(g_conn, CONFIG_BT_EATT_MAX);
		if (err != 0) {
			TEST_FAIL("Failed to connect EATT channels (err %d)", err);

			return;
		}
	}

	WAIT_FOR_FLAG(flag_test_done);

	TEST_PASS("GATT server passed");
}

static void test_main_eatt(void)
{
	test_main_common(true);
}

static void test_main_no_eatt(void)
{
	test_main_common(false);
}

static const struct bst_test_instance test_deferred_server[] = {
	{
		.test_id = "gatt_server_deferred_eatt",
		.test_main_f = test_main_eatt,
	},
	{
		.test_id = "gatt_server_deferred_no_eatt",
		.test_main_f = test_main_no_eatt,
	},
	{
		/* Prepare-count-0 image: a deferred read, then a pipelined
		 * Prepare Write whose handler is compiled out
		 */
		.test_id = "gatt_server_prep0",
		.test_main_f = test_main_no_eatt,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_gatt_deferred_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_deferred_server);
}
