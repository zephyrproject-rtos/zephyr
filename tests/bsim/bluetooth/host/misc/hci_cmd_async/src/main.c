/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
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
struct random_addr_req {
	struct bt_hci_cmd_op_fut cmd;
	bt_addr_t addr;
};

static int random_addr_encode(struct net_buf *buf, struct bt_hci_cmd_op *op)
{
	const struct random_addr_req *req = CONTAINER_OF(op, struct random_addr_req, cmd.op);

	net_buf_add_mem(buf, &req->addr, sizeof(req->addr));

	return 0;
}

/* The commands of the tests. A send that is waited for does not use the
 * callback.
 */
static const struct bt_hci_cmd_desc read_bd_addr_cmd = {
	.opcode = BT_HCI_OP_READ_BD_ADDR,
	.done = cmd_cb,
};

static const struct bt_hci_cmd_desc set_random_addr_cmd = {
	.opcode = BT_HCI_OP_LE_SET_RANDOM_ADDRESS,
	.encode = random_addr_encode,
};

static const struct bt_hci_cmd_desc unknown_cmd = {
	.opcode = UNKNOWN_OPCODE,
};

/* The return parameters go to a handler, which keeps them in the object */
struct read_bd_addr_req {
	struct bt_hci_cmd_op_fut cmd;
	uint16_t len;
	bt_addr_t addr;
};

static int read_bd_addr_rsp(struct bt_hci_cmd_op *op, struct net_buf *rp)
{
	struct read_bd_addr_req *req = CONTAINER_OF(op, struct read_bd_addr_req, cmd.op);
	const struct bt_hci_rp_read_bd_addr *bd = (const void *)rp->data;

	req->len = rp->len;
	if (rp->len < sizeof(*bd)) {
		return -EMSGSIZE;
	}

	bt_addr_copy(&req->addr, &bd->bdaddr);

	return 0;
}

static void test_future(void)
{
	static const struct bt_hci_cmd_desc read_cmd = {
		.opcode = BT_HCI_OP_READ_BD_ADDR,
		.rsp = read_bd_addr_rsp,
	};
	static struct read_bd_addr_req req;
	int err;

	err = bt_hci_cmd_send_async(&req.cmd, &read_cmd);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&req.cmd.fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(req.cmd.fut.result == 0, "Unexpected result %d", req.cmd.fut.result);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&req.cmd.op), "Operation still pending");
	TEST_ASSERT(req.len == sizeof(struct bt_hci_rp_read_bd_addr),
		    "Unexpected response length %u", req.len);
}

static void test_encoder(void)
{
	static struct random_addr_req req = {
		.addr = {{0x11, 0x22, 0x33, 0x44, 0x55, 0xc0}},
	};
	int err;

	err = bt_hci_cmd_send_async(&req.cmd, &set_random_addr_cmd);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&req.cmd.fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(req.cmd.fut.result == 0, "Unexpected result %d", req.cmd.fut.result);
}

static void test_failure_status(void)
{
	static struct bt_hci_cmd_op_fut req;
	int err;

	err = bt_hci_cmd_send_async(&req, &unknown_cmd);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&req.fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(req.fut.result == BT_HCI_ERR_UNKNOWN_CMD, "Unexpected result %d",
		    req.fut.result);
	TEST_ASSERT(bt_hci_cmd_op_status(&req.op) == BT_HCI_ERR_UNKNOWN_CMD, "Unexpected status");
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&req.op), "Operation still pending");
}

static void test_fire_and_forget(void)
{
	static const struct bt_hci_cmd_desc untold_cmd = {
		.opcode = BT_HCI_OP_READ_BD_ADDR,
	};
	static struct bt_hci_cmd_op op;
	int err;

	err = bt_hci_cmd_send_cb(&op, &untold_cmd);
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

	err = bt_hci_cmd_send_cb(&op, &read_bd_addr_cmd);
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
	static struct bt_hci_cmd_op_fut req;
	int err;

	err = bt_disable();
	TEST_ASSERT(err == 0, "Disable failed (err %d)", err);

	err = bt_hci_cmd_send_async(&req, &read_bd_addr_cmd);
	TEST_ASSERT(err == -EHOSTDOWN, "Unexpected result %d with Bluetooth disabled", err);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&req.op), "Operation pending after a failed send");

	err = bt_hci_cmd_send_cb(&req.op, &read_bd_addr_cmd);
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
