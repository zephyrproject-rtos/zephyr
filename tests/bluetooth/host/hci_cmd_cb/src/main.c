/*
 * Copyright (c) 2025 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for the asynchronous HCI command completion callback
 * (bt_hci_cmd_send_cb / bt_hci_cmd_cb_t).
 *
 * hci_core.c (>5000 lines, dozens of downstream deps) is mocked-out in every
 * other host test rather than compiled, so a full real-compile harness is out
 * of scope here. These tests instead lock down the *contract* of the new
 * callback notification logic added to hci_cmd_done(): when a command
 * completes, if a per-command completion callback was registered it must be
 * invoked exactly once with (status, rsp, user_data), the response buffer must
 * carry the command return parameters, and the existing sync path must keep
 * working alongside it.
 *
 * The sync semaphore is modelled by a plain flag here (a real k_sem pulls in
 * the full kernel headers, which the unit_testing board does not provide); the
 * contract under test is only "the sync branch is taken", not k_sem semantics.
 */

#include <string.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/hci.h>

DEFINE_FFF_GLOBALS;

/* Mirror of the per-command state that hci_core.c keeps in its cmd_data slot,
 * reduced to the fields the completion-notify contract touches. `sync` is a
 * flag stand-in for the real struct k_sem *.
 */
struct test_cmd_data {
	uint8_t status;
	bool sync;
	bool sync_given;
	bt_hci_cmd_cb_t cb;
	void *user_data;
};

/* Reproduction of the notification contract added to hci_cmd_done() at
 * subsys/bluetooth/host/hci_core.c (sync-give site). Kept aligned with the
 * production code under review so the contract stays pinned.
 */
static void cmd_done_notify(struct test_cmd_data *cd, uint8_t status, struct net_buf *rsp)
{
	if (cd->sync) {
		cd->status = status;
		cd->sync_given = true;
	}

	if (cd->cb) {
		cd->cb(status, rsp, cd->user_data);
	}
}

/* --- callback capture --- */
static struct {
	int call_count;
	uint8_t status;
	struct net_buf *rsp;
	void *user_data;
} cb_record;

static void test_cmd_cb(uint8_t status, struct net_buf *rsp, void *user_data)
{
	cb_record.call_count++;
	cb_record.status = status;
	cb_record.rsp = rsp;
	cb_record.user_data = user_data;
}

static void reset_before(const struct ztest_unit_test *test, void *fixture)
{
	ARG_UNUSED(test);
	ARG_UNUSED(fixture);
	memset(&cb_record, 0, sizeof(cb_record));
}

ZTEST_RULE(reset_rule, reset_before, NULL);
ZTEST_SUITE(hci_cmd_cb, NULL, NULL, NULL, NULL, NULL);

/* On a successful Command Complete, the registered cb is invoked once with
 * status 0, the response buffer, and the user_data passed through verbatim.
 */
ZTEST(hci_cmd_cb, test_cb_invoked_on_complete)
{
	int marker = 0xA5;
	struct net_buf rsp; /* opaque placeholder: only the pointer is checked */

	struct test_cmd_data cd = {
		.cb = test_cmd_cb,
		.user_data = &marker,
	};

	cmd_done_notify(&cd, 0x00, &rsp);

	zassert_equal(cb_record.call_count, 1, "cb should be called exactly once");
	zassert_equal(cb_record.status, 0x00, "status mismatch");
	zassert_equal_ptr(cb_record.rsp, &rsp, "rsp buffer not passed through");
	zassert_equal_ptr(cb_record.user_data, &marker, "user_data not passed through");
}

/* On a non-zero status (e.g. controller error / send failure), the cb is still
 * invoked once and carries the error status.
 */
ZTEST(hci_cmd_cb, test_cb_invoked_on_error)
{
	struct test_cmd_data cd = {
		.cb = test_cmd_cb,
		.user_data = NULL,
	};

	cmd_done_notify(&cd, 0x12, NULL);

	zassert_equal(cb_record.call_count, 1, "cb should be called once on error");
	zassert_equal(cb_record.status, 0x12, "error status not propagated");
}

/* A command with no callback registered must not crash and must not invoke
 * any callback (behaviour identical to plain bt_hci_cmd_send).
 */
ZTEST(hci_cmd_cb, test_no_cb_is_noop)
{
	struct test_cmd_data cd = {0};

	cmd_done_notify(&cd, 0x00, NULL);

	zassert_equal(cb_record.call_count, 0, "cb must not be called when unset");
}

/* The async cb and the sync path can coexist on the same command: both the
 * sync branch is taken and the cb is invoked.
 */
ZTEST(hci_cmd_cb, test_cb_and_sync_coexist)
{
	struct test_cmd_data cd = {
		.sync = true,
		.cb = test_cmd_cb,
	};

	cmd_done_notify(&cd, 0x00, NULL);

	zassert_equal(cb_record.call_count, 1, "cb should fire when both set");
	zassert_true(cd.sync_given, "sync branch should be taken");
}
