/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The Host's asynchronous HCI command operations against the controller: the
 * path through a real command and its response. What the Host does with a
 * controller that misbehaves is covered by tests/bluetooth/host_cmd.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/* Host-internal API under test */
#include "future.h"
#include "hci_cmd_op.h"

#include "babblekit/testcase.h"
#include "bstests.h"

#define WAIT_TIMEOUT K_SECONDS(2)

/* An opcode the controller does not implement */
#define UNKNOWN_OPCODE BT_OP(BT_OGF_LE, 0x3ff)

static K_SEM_DEFINE(cb_sem, 0, 1);
static struct bt_hci_cmd_op *cb_op;
static int cb_result;
static k_tid_t cb_thread;

static void cmd_cb(struct bt_hci_cmd_op *op, int result)
{
	cb_op = op;
	cb_result = result;
	cb_thread = k_current_get();

	k_sem_give(&cb_sem);
}

/* An operation finds its context through the object that it is part of */
struct random_addr_cmd {
	struct bt_hci_cmd_op op;
	bt_addr_t addr;
};

static int random_addr_encode(struct net_buf *buf, struct bt_hci_cmd_op *op)
{
	const struct random_addr_cmd *cmd = CONTAINER_OF(op, struct random_addr_cmd, op);

	net_buf_add_mem(buf, &cmd->addr, sizeof(cmd->addr));

	return 0;
}

static void test_future(void)
{
	NET_BUF_SIMPLE_DEFINE(rsp, sizeof(struct bt_hci_rp_read_bd_addr));
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	int err;

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL);

	err = bt_hci_cmd_send_async(&op, &rsp, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(fut.result == 0, "Unexpected result %d", fut.result);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op), "Operation still pending");
	TEST_ASSERT(rsp.len == sizeof(struct bt_hci_rp_read_bd_addr),
		    "Unexpected response length %u", rsp.len);
	TEST_ASSERT(rsp.data[0] == BT_HCI_ERR_SUCCESS, "Unexpected status 0x%02x", rsp.data[0]);
}

static void test_encoder(void)
{
	struct random_addr_cmd cmd = {
		.addr = {{0x11, 0x22, 0x33, 0x44, 0x55, 0xc0}},
	};
	struct bt_future fut;
	int err;

	bt_hci_cmd_op_init(&cmd.op, BT_HCI_OP_LE_SET_RANDOM_ADDRESS, random_addr_encode);

	err = bt_hci_cmd_send_async(&cmd.op, NULL, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(fut.result == 0, "Unexpected result %d", fut.result);
}

static void test_failure_status(void)
{
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	int err;

	bt_hci_cmd_op_init(&op, UNKNOWN_OPCODE, NULL);

	err = bt_hci_cmd_send_async(&op, NULL, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(fut.result == BT_HCI_ERR_UNKNOWN_CMD, "Unexpected result %d", fut.result);
	TEST_ASSERT(bt_hci_cmd_op_status(&op) == BT_HCI_ERR_UNKNOWN_CMD, "Unexpected status");
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op), "Operation still pending");
}

static void test_fire_and_forget(void)
{
	struct bt_hci_cmd_op op;
	int err;

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL);

	err = bt_hci_cmd_send_async(&op, NULL, NULL);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	/* The synchronous command is queued behind the operation */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR, NULL, NULL);
	TEST_ASSERT(err == 0, "Synchronous command failed (err %d)", err);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op), "Operation still pending");
}

static void test_callback(void)
{
	static struct bt_hci_cmd_op op;
	int err;

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL);

	err = bt_hci_cmd_send_cb(&op, NULL, cmd_cb);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = k_sem_take(&cb_sem, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Callback not invoked");
	TEST_ASSERT(cb_op == &op, "The callback got another operation");
	TEST_ASSERT(cb_result == 0, "Unexpected result %d", cb_result);
	TEST_ASSERT(cb_thread != k_current_get(), "Callback ran in the sender's thread");
	TEST_ASSERT(bt_hci_cmd_op_cancel(&op) == -EALREADY, "A completed operation was canceled");
}

static void test_host_down(void)
{
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	int err;

	err = bt_disable();
	TEST_ASSERT(err == 0, "Disable failed (err %d)", err);

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL);

	err = bt_hci_cmd_send_async(&op, NULL, &fut);
	TEST_ASSERT(err == -EHOSTDOWN, "Unexpected result %d with Bluetooth disabled", err);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op), "Operation pending after a failed send");

	err = bt_hci_cmd_send_cb(&op, NULL, cmd_cb);
	TEST_ASSERT(err == -EHOSTDOWN, "Unexpected result %d with Bluetooth disabled", err);
}

static void test_main(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Enable failed (err %d)", err);

	test_future();
	test_encoder();
	test_failure_status();
	test_fire_and_forget();
	test_callback();
	test_host_down();

	TEST_PASS_AND_EXIT("HCI command async test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "hci_cmd_async",
		.test_descr = "Asynchronous HCI command operations",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

static struct bst_test_list *test_hci_cmd_async_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_hci_cmd_async_install,
	NULL
};

int main(void)
{
	bst_main();

	return 0;
}
