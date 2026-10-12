/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* The Host's asynchronous HCI command operations: how a result gets back to
 * the caller, what a cancel does in each state, and that nothing a caller
 * does with a result can hold up the commands of others.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "future.h"
#include "hci_cmd_op.h"
#include "hci_core.h"

#include "driver.h"

/* Stack of a thread that sends commands, logging included */
#define SENDER_STACK_SIZE 2048U

/* The operation that most cases send, with its future */
static struct bt_hci_cmd_op_fut req;

static int encode_refusal;
static unsigned int encode_count;

static struct bt_hci_cmd_op *encode_op;
static bool has_encode_run_unlocked;
static bool has_encode_run_irq_locked;

/* For a test that has something to do while the encoder runs */
static K_SEM_DEFINE(encode_sem, 0, 1);
static K_SEM_DEFINE(encode_gate, 0, 1);
static bool is_encode_gated;

static int encode(struct net_buf *buf, struct bt_hci_cmd_op *encoded)
{
	unsigned int key;

	encode_op = encoded;
	encode_count++;

	if (bt_dev.lock.owner != k_current_get()) {
		has_encode_run_unlocked = true;
	}

	key = irq_lock();
	if (!arch_irq_unlocked(key)) {
		has_encode_run_irq_locked = true;
	}

	irq_unlock(key);

	if (is_encode_gated) {
		is_encode_gated = false;
		k_sem_give(&encode_sem);
		/* Ended by the test, or by the reset of the gate in after() */
		(void)k_sem_take(&encode_gate, K_FOREVER);
	}

	if (encode_refusal != 0) {
		return encode_refusal;
	}

	/* One parameter byte, of any value */
	(void)net_buf_add_u8(buf, 0x5aU);

	return 0;
}

/* Sends its operation again with param_cmd, which names it */
static void op_cb(struct bt_hci_cmd_op *completed, int result);

/* The test command with a parameter byte, and without one. A send that is
 * waited for does not use the callback.
 */
static const struct bt_hci_cmd_desc param_cmd = {
	.opcode = TEST_OPCODE,
	.encode = encode,
	.done = op_cb,
};

static const struct bt_hci_cmd_desc plain_cmd = {
	.opcode = TEST_OPCODE,
	.done = op_cb,
};

/* More than the callbacks that any test has outstanding at once */
#define CB_SEM_MAX 10U

static K_SEM_DEFINE(cb_sem, 0, CB_SEM_MAX);
static K_SEM_DEFINE(cb_gate, 0, 1);
static unsigned int cb_count;
static int cb_result;
static struct bt_hci_cmd_op *cb_op;
static k_tid_t cb_thread;
static bool is_cb_resending;
static bool is_cb_gated;

static void op_cb(struct bt_hci_cmd_op *completed, int result)
{
	cb_count++;
	cb_result = result;
	cb_op = completed;
	cb_thread = k_current_get();

	if (is_cb_resending) {
		is_cb_resending = false;
		zassert_ok(bt_hci_cmd_send_cb(completed, &param_cmd),
			   "Sending the operation again from its callback failed");
	}

	k_sem_give(&cb_sem);

	if (is_cb_gated) {
		is_cb_gated = false;
		/* Ended by the test, or by the reset of the gate in after() */
		(void)k_sem_take(&cb_gate, K_FOREVER);
	}
}

/* The thread of send_from_ended_thread() */
static K_THREAD_STACK_DEFINE(sender_stack, SENDER_STACK_SIZE);
static struct k_thread sender_thread;
static k_tid_t sender_tid;
static int sender_err;

/* The work item of a test that sends from the system workqueue, and the end
 * of the wait for a sender of a test
 */
static struct k_work syswq_work;
static K_SEM_DEFINE(work_done, 0, 1);

/* Queues of the synchronous senders of test_mixed_load(), one each */
#define LOAD_SENDERS 2U

static K_THREAD_STACK_ARRAY_DEFINE(load_stacks, LOAD_SENDERS, SENDER_STACK_SIZE);
static struct k_work_q load_queues[LOAD_SENDERS];

/* The queue of a sender that is to block on the command pool */
static K_THREAD_STACK_DEFINE(alloc_stack, SENDER_STACK_SIZE);
static struct k_work_q alloc_queue;

static void *setup(void)
{
	/* Preemptible, below the cooperative test thread: a sender runs
	 * while the test waits.
	 */
	ARRAY_FOR_EACH(load_queues, i) {
		k_work_queue_init(&load_queues[i]);
		k_work_queue_start(&load_queues[i], load_stacks[i],
				   K_THREAD_STACK_SIZEOF(load_stacks[i]), K_PRIO_PREEMPT(1 + i),
				   NULL);
	}

	k_work_queue_init(&alloc_queue);
	k_work_queue_start(&alloc_queue, alloc_stack, K_THREAD_STACK_SIZEOF(alloc_stack),
			   K_PRIO_PREEMPT(1), NULL);

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Lets go of a callback or an encoder that a failed test left waiting */
	is_encode_gated = false;
	is_cb_gated = false;
	is_cb_resending = false;
	k_sem_reset(&encode_gate);
	k_sem_reset(&cb_gate);

	test_cmd_bufs_release();

	/* The disable completes whatever the senders still wait for. A test
	 * that failed in the middle of a recovery left Bluetooth disabled.
	 */
	if (bt_is_ready()) {
		test_driver_release();
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}

	if (sender_tid != NULL) {
		zassert_ok(k_thread_join(sender_tid, TEST_TIMEOUT), "The sender did not end");
		sender_tid = NULL;
	}

	test_workq_flush(&k_sys_work_q);
	ARRAY_FOR_EACH_PTR(load_queues, queue) {
		test_workq_flush(queue);
	}

	test_workq_flush(&alloc_queue);

	test_bt_workq_flush();

	encode_refusal = 0;
	encode_count = 0U;
	has_encode_run_unlocked = false;
	has_encode_run_irq_locked = false;
	cb_count = 0U;
	k_sem_reset(&encode_sem);
	k_sem_reset(&cb_sem);
	k_sem_reset(&work_done);
}

ZTEST_SUITE(cmd_op, test_with_reset, setup, before, after, NULL);

static void recover(void)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
}

/* The result of a command is the status that the controller responded with */
static ZTEST(cmd_op, test_future)
{
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The operation is pending before its use");

	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
	zassert_false(bt_hci_cmd_op_is_pending(&req.op),
		      "The operation is pending after its result");

	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(req.fut.result, BT_HCI_ERR_CMD_DISALLOWED,
		      "The result is %d", req.fut.result);
	zassert_equal(bt_hci_cmd_op_status(&req.op), BT_HCI_ERR_CMD_DISALLOWED, "Wrong status");

	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_equal(encode_count, 2U, "The encoder ran %u times", encode_count);
	zassert_equal_ptr(encode_op, &req.op, "The encoder got another operation");
	zassert_false(has_encode_run_unlocked, "The encoder ran without the host lock");
	zassert_false(has_encode_run_irq_locked, "The encoder ran with interrupts locked");
}

/* A callback runs on another thread than the sender's, gets the result and its
 * context, and may send the operation again.
 */
static ZTEST(cmd_op, test_callback)
{
	is_cb_resending = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");

	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not run");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not run again");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
	zassert_equal(cb_result, 0, "The result is %d", cb_result);
	zassert_equal_ptr(cb_op, &req.op, "The callback got another operation");
	zassert_not_equal(cb_thread, k_current_get(), "The callback ran in the sender's thread");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
}

/* A command whose description has no callback: nobody is told how it went */
static ZTEST(cmd_op, test_nobody_told)
{
	static const struct bt_hci_cmd_desc untold_cmd = {
		.opcode = TEST_OPCODE,
		.encode = encode,
	};

	zassert_ok(bt_hci_cmd_send_cb(&req.op, &untold_cmd), "Sending the operation failed");
	zassert_ok(test_rsp_wait(&test_cmd, 1U), "The controller did not respond");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The operation is pending");

	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &untold_cmd), "Sending the operation failed");
	zassert_ok(test_rsp_wait(&test_cmd, 2U), "The controller did not respond");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The failed operation is pending");
	test_bt_workq_flush();
	zassert_equal(cb_count, 0U, "A callback ran");
}

/* An object whose command has return parameters, as its owner has it: the
 * operation with its future, and what the handler of the return parameters
 * keeps and finds.
 */
struct keep_obj {
	struct bt_hci_cmd_op_fut cmd;
	/* The most that the handler takes: it refuses longer return parameters */
	uint16_t max;
	unsigned int count;
	uint16_t len;
	uint8_t rp[16];
	bool is_pending;
};

static int keep_rsp(struct bt_hci_cmd_op *op, struct net_buf *rp)
{
	struct keep_obj *obj = CONTAINER_OF(op, struct keep_obj, cmd.op);

	obj->count++;
	obj->len = rp->len;
	obj->is_pending = bt_hci_cmd_op_is_pending(op);

	if (rp->len > obj->max) {
		return -EMSGSIZE;
	}

	(void)memcpy(obj->rp, rp->data, rp->len);

	return 0;
}

static const struct bt_hci_cmd_desc keep_cmd = {
	.opcode = TEST_OPCODE,
	.encode = encode,
	.rsp = keep_rsp,
	.done = op_cb,
};

/* The handler of the return parameters gets them, the status first, before
 * the operation is handed back, and only from a controller that responded with
 * success. Return parameters that it refuses fail the operation.
 */
#define KEEP_RP_LEN 8U

static ZTEST(cmd_op, test_return_parameters)
{
	static struct keep_obj obj;

	obj.max = sizeof(obj.rp);
	test_cmd.rp_len = KEEP_RP_LEN;

	zassert_ok(bt_hci_cmd_send_async(&obj.cmd, &keep_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&obj.cmd.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(obj.cmd.fut.result, 0, "The result is %d", obj.cmd.fut.result);
	zassert_equal(obj.count, 1U, "The handler ran %u times", obj.count);
	zassert_true(obj.is_pending, "The handler ran after the hand-back");
	zassert_equal(obj.len, 1U + KEEP_RP_LEN, "The handler got %u bytes", obj.len);
	zassert_equal(obj.rp[0], BT_HCI_ERR_SUCCESS, "The status does not come first");

	/* The test controller counts the return parameters up from zero */
	zassert_equal(obj.rp[KEEP_RP_LEN], KEEP_RP_LEN - 1U, "Wrong last parameter byte");

	/* More return parameters than the handler can use */
	obj.max = KEEP_RP_LEN / 2U;
	zassert_ok(bt_hci_cmd_send_async(&obj.cmd, &keep_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&obj.cmd.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(obj.cmd.fut.result, -EMSGSIZE, "The result is %d", obj.cmd.fut.result);
	zassert_equal(bt_hci_cmd_op_status(&obj.cmd.op), BT_HCI_ERR_SUCCESS,
		      "The status of the controller was lost");

	/* The controller responds, but not with success */
	obj.count = 0U;
	test_cmd.status = BT_HCI_ERR_INVALID_PARAM;
	zassert_ok(bt_hci_cmd_send_async(&obj.cmd, &keep_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&obj.cmd.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(obj.cmd.fut.result, BT_HCI_ERR_INVALID_PARAM, "The result is %d",
		      obj.cmd.fut.result);
	zassert_equal(obj.count, 0U, "The handler got the response to a failed command");
	zassert_equal(bt_hci_cmd_op_status(&obj.cmd.op), BT_HCI_ERR_INVALID_PARAM,
		      "The status of the controller was lost");

	/* A command whose description has no handler */
	test_cmd.status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
}

/* Results that nobody picks up hold no command buffer: more commands than
 * there are buffers complete, and a synchronous command still goes through.
 */
static ZTEST(cmd_op, test_results_not_picked_up)
{
	static struct keep_obj kept;
	unsigned int buf_count = test_cmd_bufs_hold();

	test_cmd_bufs_release();
	kept.max = sizeof(kept.rp);
	test_cmd.rp_len = KEEP_RP_LEN;

	for (unsigned int i = 0U; i < buf_count + 1U; i++) {
		zassert_ok(bt_hci_cmd_send_async(&kept.cmd, &keep_cmd),
			   "Sending round %u failed", i);
		zassert_ok(test_rsp_wait(&test_cmd, i + 1U), "Operation %u got no response", i);
		zassert_true(bt_future_is_done(&kept.cmd.fut), "Operation %u did not complete", i);
		zassert_equal(kept.len, 1U + KEEP_RP_LEN, "Operation %u had %u bytes", i, kept.len);
	}

	zassert_equal(test_cmd_bufs_hold(), buf_count, "Completed operations hold command buffers");
	test_cmd_bufs_release();

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "A synchronous command failed");
}

/* An object that a command is about, as its owner has it: the operation with
 * its future, what the handler of the return parameters keeps, and what the
 * encoder and the callback need. The encoder sends the id of the object,
 * which the test controller repeats in the return parameters.
 */
#define PROBE_RP_LEN 4U

struct probe_obj {
	struct bt_hci_cmd_op_fut cmd;
	uint8_t id;
	/* Commands that the callback still sends, one after the other */
	uint8_t more;
	/* What the handler of the return parameters found */
	uint16_t len;
	uint8_t rp[1U + PROBE_RP_LEN];
	/* What the callback found */
	int result;
	k_tid_t thread;
};

static int probe_encode(struct net_buf *buf, struct bt_hci_cmd_op *encoded)
{
	const struct probe_obj *probe = CONTAINER_OF(encoded, struct probe_obj, cmd.op);

	(void)net_buf_add_u8(buf, probe->id);

	return 0;
}

static int probe_rsp(struct bt_hci_cmd_op *op, struct net_buf *rp)
{
	struct probe_obj *probe = CONTAINER_OF(op, struct probe_obj, cmd.op);

	probe->len = rp->len;
	if (rp->len < sizeof(probe->rp)) {
		return -EMSGSIZE;
	}

	(void)memcpy(probe->rp, rp->data, sizeof(probe->rp));

	return 0;
}

/* Sends the next command of its object with probe_cmd, which names it */
static void probe_cb(struct bt_hci_cmd_op *completed, int result);

static const struct bt_hci_cmd_desc probe_cmd = {
	.opcode = TEST_OPCODE,
	.encode = probe_encode,
	.rsp = probe_rsp,
	.done = probe_cb,
};

/* The callback of the first probe waits at cb_gate before it reads anything */
static bool is_probe_gated;

static void probe_cb(struct bt_hci_cmd_op *completed, int result)
{
	struct probe_obj *probe = CONTAINER_OF(completed, struct probe_obj, cmd.op);

	if (is_probe_gated) {
		is_probe_gated = false;
		k_sem_give(&cb_sem);
		/* Ended by the test, or by the reset of the gate in after() */
		(void)k_sem_take(&cb_gate, K_FOREVER);
	}

	probe->result = result;
	probe->thread = k_current_get();

	if (probe->more > 0U) {
		probe->more--;
		probe->id++;
		zassert_ok(bt_hci_cmd_send_cb(&probe->cmd.op, &probe_cmd),
			   "Sending the next command from the callback failed");
	}

	k_sem_give(&cb_sem);
}

/* The tests give their probes ids of their own, so that the return
 * parameters of one cannot pass for those of another.
 */
static void probe_init(struct probe_obj *probe, uint8_t id)
{
	(void)memset(probe, 0, sizeof(*probe));
	probe->id = id;
}

/* A thread that sends the command of an object and ends, so that nothing of
 * it is left when the controller responds.
 */
static void sender_cb_entry(void *p1, void *p2, void *p3)
{
	struct probe_obj *probe = p1;

	ARGS_UNUSED(p2, p3);

	sender_err = bt_hci_cmd_send_cb(&probe->cmd.op, &probe_cmd);
}

static void sender_future_entry(void *p1, void *p2, void *p3)
{
	struct probe_obj *probe = p1;

	ARGS_UNUSED(p2, p3);

	sender_err = bt_hci_cmd_send_async(&probe->cmd, &probe_cmd);
}

static void send_from_ended_thread(k_thread_entry_t entry, struct probe_obj *probe)
{
	sender_err = -EAGAIN;

	/* Below the cooperative test thread, so that it runs while the test
	 * waits for it
	 */
	sender_tid = k_thread_create(&sender_thread, sender_stack,
				     K_THREAD_STACK_SIZEOF(sender_stack), entry, probe, NULL, NULL,
				     K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(sender_tid, TEST_TIMEOUT), "The sender did not end");
	sender_tid = NULL;
	zassert_ok(sender_err, "Sending the operation failed");
}

/* The thread of the Bluetooth workqueue, which is where callbacks run */
static k_tid_t workq_thread;

static void workq_thread_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	workq_thread = k_current_get();
	k_sem_give(&cb_sem);
}

static K_WORK_DEFINE(workq_thread_work, workq_thread_handler);

/* A completion is dealt with elsewhere than where the command was sent: the
 * sending thread has ended when the controller responds, the handler keeps the
 * return parameters in the object, and the callback, on the Bluetooth
 * workqueue, finds them there and sends its next command from there.
 */
static ZTEST(cmd_op, test_response_read_by_callback)
{
	/* The command of the sender, and two that the callback sends */
	const uint8_t cmd_count = 3U;
	const uint8_t first_id = 0x10U;
	static struct probe_obj probe;

	zassert_true(bt_work_submit(&workq_thread_work) >= 0, "Submitting work failed");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The Bluetooth workqueue did not run");

	probe_init(&probe, first_id);
	probe.more = cmd_count - 1U;
	test_cmd.delay = K_FOREVER;
	test_cmd.rp_len = PROBE_RP_LEN;
	test_cmd.echo = true;

	send_from_ended_thread(sender_cb_entry, &probe);

	for (uint8_t i = 0U; i < cmd_count; i++) {
		zassert_ok(test_cmd_wait(&test_cmd, i + 1U), "Command %u was not sent", i);
		test_settle();
		zassert_equal(test_cmd.count, i + 1U, "The controller got %u commands",
			      test_cmd.count);
		zassert_equal(k_sem_count_get(&cb_sem), 0U, "Command %u completed early", i);

		test_cmd_respond(&test_cmd);
		zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "Command %u did not complete", i);
		zassert_equal(probe.result, 0, "Command %u gave %d", i, probe.result);
		zassert_equal_ptr(probe.thread, workq_thread,
				  "The callback did not run on the Bluetooth workqueue");
		zassert_equal(probe.len, sizeof(probe.rp), "Command %u left %u bytes", i,
			      probe.len);
		zassert_equal(probe.rp[0], BT_HCI_ERR_SUCCESS, "The status does not come first");
		zassert_equal(probe.rp[PROBE_RP_LEN], first_id + i,
			      "Command %u left the return parameters of another command", i);
	}

	zassert_false(bt_hci_cmd_op_is_pending(&probe.cmd.op), "The operation is pending");
}

/* The same with a future: another thread than the sender, which has ended,
 * collects the completion and finds the return parameters in the object.
 */
static ZTEST(cmd_op, test_response_collected_by_other_thread)
{
	static struct probe_obj probe;

	probe_init(&probe, 0x20U);
	test_cmd.delay = K_FOREVER;
	test_cmd.rp_len = PROBE_RP_LEN;
	test_cmd.echo = true;

	send_from_ended_thread(sender_future_entry, &probe);
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The command was not sent");
	zassert_false(bt_future_is_done(&probe.cmd.fut), "The future was resolved early");

	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&probe.cmd.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(probe.cmd.fut.result, 0, "The result is %d", probe.cmd.fut.result);
	zassert_equal(probe.len, sizeof(probe.rp), "The handler got %u bytes", probe.len);
	zassert_equal(probe.rp[0], BT_HCI_ERR_SUCCESS, "The status does not come first");
	zassert_equal(probe.rp[PROBE_RP_LEN], probe.id, "Wrong return parameters");
}

/* The handler of each object keeps the return parameters of its own command
 * as the response arrives, also while the callbacks are held up: all three
 * commands have got their responses while the first callback holds up the
 * other two.
 */
static ZTEST(cmd_op, test_responses_of_several_objects)
{
	static struct probe_obj probes[3];

	is_probe_gated = true;

	/* The responses come after the first callback has started to hold up
	 * the others
	 */
	test_cmd.delay = K_MSEC(5);
	test_cmd.rp_len = PROBE_RP_LEN;
	test_cmd.echo = true;

	ARRAY_FOR_EACH(probes, i) {
		probe_init(&probes[i], (uint8_t)(0x30U + i));
		zassert_ok(bt_hci_cmd_send_cb(&probes[i].cmd.op, &probe_cmd),
			   "Sending operation %zu failed", i);
	}

	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The first callback did not run");
	zassert_ok(test_rsp_wait(&test_cmd, ARRAY_SIZE(probes)), "A response is missing");
	zassert_equal(test_cmd.count, ARRAY_SIZE(probes), "The controller got %u commands",
		      test_cmd.count);

	k_sem_give(&cb_gate);
	ARRAY_FOR_EACH(probes, i) {
		zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "Completion %zu is missing", i);
	}

	ARRAY_FOR_EACH(probes, i) {
		zassert_equal(probes[i].result, 0, "Operation %zu gave %d", i, probes[i].result);
		zassert_equal(probes[i].len, sizeof(probes[i].rp), "Operation %zu left %u bytes",
			      i, probes[i].len);
		zassert_equal(probes[i].rp[1], probes[i].id,
			      "Object %zu has the return parameters of another one", i);
	}
}

/* The handler of the return parameters runs only for a command that the
 * controller responded to: not for one that was not sent, nor for one that
 * was canceled.
 */
static ZTEST(cmd_op, test_return_parameters_unanswered)
{
	static struct bt_hci_cmd_op_fut outstanding;
	static struct probe_obj probe;

	probe_init(&probe, 0x40U);
	test_cmd.rp_len = PROBE_RP_LEN;

	zassert_ok(bt_hci_cmd_send_async(&probe.cmd, &probe_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&probe.cmd.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(probe.len, sizeof(probe.rp), "A command with a response reached no handler");

	/* The driver fails to send the command */
	probe.len = 0U;
	test_cmd.send_err = -EIO;
	zassert_ok(bt_hci_cmd_send_async(&probe.cmd, &probe_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&probe.cmd.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(probe.cmd.fut.result, -EIO, "The result is %d", probe.cmd.fut.result);
	zassert_equal(probe.len, 0U, "An unsent command reached the handler");
	test_cmd.send_err = 0;

	zassert_ok(bt_hci_cmd_send_async(&probe.cmd, &probe_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&probe.cmd.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(probe.len, sizeof(probe.rp), "A command with a response reached no handler");

	/* Canceled while it waits behind a command that the controller keeps */
	probe.len = 0U;
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&outstanding, &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&probe.cmd, &probe_cmd), "Sending the operation failed");
	zassert_ok(test_cmd_wait(&test_cmd, 3U), "The outstanding command was not sent");
	zassert_ok(bt_hci_cmd_op_cancel(&probe.cmd.op), "Canceling the operation failed");
	zassert_equal(probe.cmd.fut.result, -ECANCELED, "The result is %d",
		      probe.cmd.fut.result);
	zassert_equal(probe.len, 0U, "A canceled command reached the handler");

	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&outstanding.fut, TEST_TIMEOUT), "The future was not resolved");
}

/* The one way to keep an operation on a stack: in a function that does not
 * return while the operation is pending. When its patience runs out it
 * cancels, and then waits for the hand-back whatever the cancel said: an
 * operation that has just completed is not pending any more, while its future
 * may not be resolved yet.
 */
struct stack_obj {
	struct bt_hci_cmd_op_fut cmd;
	uint16_t len;
};

static int stack_rsp(struct bt_hci_cmd_op *op, struct net_buf *rp)
{
	CONTAINER_OF(op, struct stack_obj, cmd.op)->len = rp->len;

	return 0;
}

static int send_and_wait(k_timeout_t patience, uint16_t *rp_len)
{
	static const struct bt_hci_cmd_desc stack_cmd = {
		.opcode = TEST_OPCODE,
		.rsp = stack_rsp,
	};
	struct stack_obj local = { 0 };
	int err;

	err = bt_hci_cmd_send_async(&local.cmd, &stack_cmd);
	if (err != 0) {
		return err;
	}

	if (bt_future_wait(&local.cmd.fut, patience) != 0) {
		/* Whatever the cancel gives, the operation is handed back, and
		 * the wait for that cannot fail
		 */
		(void)bt_hci_cmd_op_cancel(&local.cmd.op);
		(void)bt_future_wait(&local.cmd.fut, K_FOREVER);
	}

	*rp_len = local.len;

	return local.cmd.fut.result;
}

static ZTEST(cmd_op, test_stack_storage)
{
	static struct bt_hci_cmd_op_fut outstanding;
	uint16_t rp_len = UINT16_MAX;

	test_cmd.rp_len = PROBE_RP_LEN;

	/* The controller takes longer than the caller has patience for */
	test_cmd.delay = K_MSEC(50);
	zassert_ok(send_and_wait(K_MSEC(10), &rp_len), "The command failed");
	zassert_equal(rp_len, 1U + PROBE_RP_LEN, "%u bytes of return parameters", rp_len);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* The command has not been sent when the patience runs out */
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&outstanding, &plain_cmd), "Sending the operation failed");
	zassert_equal(send_and_wait(K_MSEC(10), &rp_len), -ECANCELED,
		      "A command behind an outstanding one was not canceled");
	zassert_equal(rp_len, 0U, "A canceled command left %u bytes", rp_len);

	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&outstanding.fut, TEST_TIMEOUT), "The future was not resolved");
	test_settle();
	zassert_equal(test_cmd.count, 2U, "A canceled command reached the controller");
}

/* A blocked callback holds up the callbacks behind it, but not the commands */
static ZTEST(cmd_op, test_callback_blocked)
{
	static struct bt_hci_cmd_op second;
	unsigned int buf_count;

	is_cb_gated = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not run");

	zassert_ok(bt_hci_cmd_send_cb(&second, &plain_cmd), "Sending the operation failed");

	buf_count = test_cmd_bufs_hold();
	test_cmd_bufs_release();

	for (unsigned int i = 0U; i < buf_count + 1U; i++) {
		zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
			   "A synchronous command failed behind a blocked callback");
	}

	zassert_equal(cb_count, 1U, "The second callback ran past the blocked one");
	zassert_equal(bt_hci_cmd_op_cancel(&second), -EINPROGRESS,
		      "A completed operation could be canceled");

	k_sem_give(&cb_gate);
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The second callback did not run");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
}

/* A call that fails changes nothing */
static ZTEST(cmd_op, test_busy)
{
	/* Pending for the calls below, as no time passes until the wait */
	test_cmd.delay = K_MSEC(100);
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_equal(bt_hci_cmd_send_async(&req, &param_cmd), -EBUSY,
		      "A pending operation was accepted again");
	zassert_equal(bt_hci_cmd_send_cb(&req.op, &param_cmd), -EBUSY,
		      "A pending operation was accepted again");

	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
	zassert_equal(cb_count, 0U, "The refused send got a callback");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
}

/* An operation that its encoder refuses never reaches the controller */
static ZTEST(cmd_op, test_encoder_refuses)
{
	encode_refusal = -ESTALE;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command buffer of the refused operation was not released");
}

/* A cancel in each state of an operation */
static ZTEST(cmd_op, test_cancel)
{
	static struct bt_hci_cmd_op_fut outstanding;
	unsigned int buf_count;

	zassert_equal(bt_hci_cmd_op_cancel(&req.op), -EALREADY, "An idle operation was canceled");

	/* Queued: no command buffer to be had */
	buf_count = test_cmd_bufs_hold();
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	test_settle();
	zassert_equal(encode_count, 0U, "The operation got a buffer");
	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling a queued operation failed");
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The canceled operation is pending");
	test_cmd_bufs_release();
	test_settle();
	zassert_equal(test_cmd.count, 0U, "A canceled command reached the controller");
	zassert_equal(cb_count, 0U, "A canceled operation got its callback");

	/* Encoded, but the controller takes no command: the TX processor
	 * gives the operations their buffers before it sends a command
	 */
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&outstanding, &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The first command was not sent");
	zassert_equal(encode_count, 1U, "The operation got no buffer");

	/* With the controller */
	zassert_equal(bt_hci_cmd_op_cancel(&outstanding.op), -EINPROGRESS,
		      "A command that is with the controller was canceled");

	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling an encoded operation failed");
	zassert_true(bt_future_is_done(&req.fut),
		     "The future of a canceled operation is unresolved");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);

	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&outstanding.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(outstanding.fut.result, 0, "The result is %d", outstanding.fut.result);
	test_settle();
	zassert_equal(test_cmd.count, 1U, "A canceled command reached the controller");

	/* The buffer of the canceled operation is free again, and so is the credit */
	zassert_equal(test_cmd_bufs_hold(), buf_count, "A command buffer is missing");
	test_cmd_bufs_release();
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command credit is missing");
}

/* A cancel that comes while the encoder runs is too late to get the operation
 * back, as the encoder may be using it, and early enough to keep the command
 * from the controller: the operation completes as canceled.
 */
static ZTEST(cmd_op, test_cancel_while_encoding)
{
	is_encode_gated = true;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&encode_sem, TEST_TIMEOUT), "The encoder did not run");

	zassert_equal(bt_hci_cmd_op_cancel(&req.op), -EINPROGRESS,
		      "An operation was handed back under its encoder");
	zassert_true(bt_hci_cmd_op_is_pending(&req.op), "The operation is not pending");
	zassert_false(bt_future_is_done(&req.fut), "The future was resolved under the encoder");

	k_sem_give(&encode_gate);
	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);
	zassert_equal(test_cmd.count, 0U, "A canceled command reached the controller");

	/* The same with a callback, which is told */
	is_encode_gated = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&encode_sem, TEST_TIMEOUT), "The encoder did not run");
	zassert_equal(bt_hci_cmd_op_cancel(&req.op), -EINPROGRESS,
		      "An operation was handed back under its encoder");
	k_sem_give(&encode_gate);
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback was not invoked");
	zassert_equal(cb_result, -ECANCELED, "The result is %d", cb_result);
	zassert_equal(test_cmd.count, 0U, "A canceled command reached the controller");

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command buffer of the canceled operation was not released");
}

/* An operation is either canceled, and then nothing of it is seen again, or
 * it completes. Never both, never neither. A cancel that is too late to hand
 * the operation back may still be in time to keep the command from the
 * controller, and the operation then completes as canceled.
 */
#define CANCEL_ROUNDS 200U

static ZTEST(cmd_op, test_cancel_or_complete)
{
	unsigned int canceled_cnt = 0U;
	unsigned int completed_cnt = 0U;
	unsigned int stopped_cnt = 0U;

	for (unsigned int i = 0U; i < CANCEL_ROUNDS; i++) {
		unsigned int before_count = test_cmd.count;
		int err;

		zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");

		/* Let the TX processor get further in three rounds out of four */
		if ((i % 4U) != 0U) {
			k_yield();
		}

		err = bt_hci_cmd_op_cancel(&req.op);
		zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");

		if (err == 0) {
			canceled_cnt++;
			zassert_equal(req.fut.result, -ECANCELED,
				      "Round %u: result %d", i, req.fut.result);
			test_settle();
			zassert_equal(test_cmd.count, before_count,
				      "Round %u: a canceled command reached the controller", i);
		} else if (req.fut.result == -ECANCELED) {
			/* Caught under its encoder, which another CPU can run
			 * during the cancel
			 */
			stopped_cnt++;
			zassert_equal(err, -EINPROGRESS, "Round %u: cancel gave %d", i, err);
			test_settle();
			zassert_equal(test_cmd.count, before_count,
				      "Round %u: a canceled command reached the controller", i);
		} else {
			completed_cnt++;
			zassert_equal(req.fut.result, 0, "Round %u: result %d", i, req.fut.result);
			zassert_equal(test_cmd.count, before_count + 1U,
				      "Round %u: the controller got no command", i);
		}
	}

	TC_PRINT("%u canceled, %u stopped under the encoder, %u completed\n", canceled_cnt,
		 stopped_cnt, completed_cnt);
	zassert_true(canceled_cnt > 0U, "No round was canceled");
	zassert_true(completed_cnt > 0U, "No round completed");
}

/* An operation that finds no command buffer, with nothing on its way that
 * would free one: the deadline ends its wait. The controller has done nothing
 * wrong, so the Host carries on.
 */
static ZTEST(cmd_op, test_no_buffer)
{
	int64_t start = k_uptime_get();
	int64_t waited;

	(void)test_cmd_bufs_hold();
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");

	zassert_ok(bt_future_wait(&req.fut, TEST_PAST_DEADLINE), "The future was not resolved");
	waited = k_uptime_get() - start;
	zassert_equal(req.fut.result, -ENOBUFS, "The result is %d", req.fut.result);
	zassert_within(waited, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The operation took %lld ms", waited);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");

	/* The next one is accepted, and waits like the first */
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd),
		   "The Host gave up on its controller over a missing buffer");
	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling the queued operation failed");

	test_cmd_bufs_release();
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	test_cmd_pool_check();
}

/* The Host keeps to one command at a time. A controller that grants a credit
 * while a command is outstanding gets the next command only after it has
 * responded, so that the response cannot be taken for that of the next one.
 */
static ZTEST(cmd_op, test_credit_while_command_outstanding)
{
	static struct bt_hci_cmd_op_fut second;

	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The first command was not sent");

	/* Another command with the same opcode, and room for it */
	test_credit_grant();
	zassert_ok(bt_hci_cmd_send_async(&second, &plain_cmd), "Sending the operation failed");
	test_settle();
	zassert_equal(test_cmd.count, 1U, "A command was sent while another was outstanding");

	/* The response is that of the first command, and lets the second go */
	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(req.fut.result, BT_HCI_ERR_CMD_DISALLOWED,
		      "The result is %d", req.fut.result);
	zassert_ok(test_cmd_wait(&test_cmd, 2U), "The second command was not sent");
	zassert_false(bt_future_is_done(&second.fut), "The second command completed early");

	test_cmd.status = BT_HCI_ERR_SUCCESS;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&second.fut, TEST_TIMEOUT), "The future was not resolved");
	zassert_equal(second.fut.result, 0, "The result is %d", second.fut.result);

	/* The credits of both responses have not added up to more than one */
	test_cmd.delay = K_NO_WAIT;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
}

/* An operation that has its buffer but no credit is canceled: its buffer is
 * free at once, and nothing is left that the deadline would hold against the
 * controller, however long the next credit takes.
 */
static ZTEST(cmd_op, test_cancel_encoded_without_credit)
{
	unsigned int buf_count;

	buf_count = test_cmd_bufs_hold();
	test_cmd_bufs_release();

	/* The controller responds, and takes no further command for now */
	test_cmd.ncmd = 0U;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command failed");

	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	test_settle();
	zassert_equal(encode_count, 1U, "The operation got no buffer");
	zassert_equal(test_cmd.count, 1U, "A command was sent without a credit");

	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling the encoded operation failed");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);
	zassert_equal(test_cmd_bufs_hold(), buf_count,
		      "The buffer of the canceled operation is not free");
	test_cmd_bufs_release();

	/* A deadline that is held against the controller expires only once
	 * its time has passed, and nothing signals the expiry.
	 */
	(void)k_sleep(TEST_PAST_DEADLINE);

	/* Still on speaking terms */
	test_cmd.ncmd = 1U;
	test_credit_grant();
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The Host gave up on a controller that owed it nothing");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
}

/* The operations reach the controller in the order in which they were sent */
struct sized_op {
	struct bt_hci_cmd_op_fut cmd;
	uint8_t len;
};

static int sized_encode(struct net_buf *buf, struct bt_hci_cmd_op *encoded)
{
	const struct sized_op *sized = CONTAINER_OF(encoded, struct sized_op, cmd.op);

	(void)net_buf_add(buf, sized->len);

	return 0;
}

static const struct bt_hci_cmd_desc sized_cmd = {
	.opcode = TEST_OPCODE,
	.encode = sized_encode,
};

static ZTEST(cmd_op, test_order)
{
	static struct sized_op sized[3];

	/* The responses take a moment, so that the operations queue up */
	test_cmd.delay = K_MSEC(5);

	ARRAY_FOR_EACH(sized, i) {
		sized[i].len = (uint8_t)(i + 1U);
		zassert_ok(bt_hci_cmd_send_async(&sized[i].cmd, &sized_cmd),
			   "Sending operation %zu failed", i);
	}

	ARRAY_FOR_EACH(sized, i) {
		zassert_ok(bt_future_wait(&sized[i].cmd.fut, TEST_TIMEOUT),
			   "Operation %zu did not complete", i);
		zassert_equal(sized[i].cmd.fut.result, 0, "Operation %zu gave %d", i,
			      sized[i].cmd.fut.result);
		zassert_equal(test_cmd_plen[i], sized[i].len, "Command %zu had %u parameter bytes",
			      i, test_cmd_plen[i]);
	}
}

/* Once its callback is invoked, an operation is not touched by the engine any
 * more, nor is what the handler of its return parameters keeps: the callback
 * may release the object that they are in.
 */
/* What the next owner of the memory of the object might write into it */
#define RELEASED_BYTE 0xa5U

static void release_cb(struct bt_hci_cmd_op *completed, int result)
{
	struct probe_obj *probe = CONTAINER_OF(completed, struct probe_obj, cmd.op);

	ARG_UNUSED(result);

	(void)memset(probe, RELEASED_BYTE, sizeof(*probe));
	k_sem_give(&cb_sem);
}

static ZTEST(cmd_op, test_callback_releases_object)
{
	static const struct bt_hci_cmd_desc release_cmd = {
		.opcode = TEST_OPCODE,
		.encode = probe_encode,
		.rsp = probe_rsp,
		.done = release_cb,
	};
	static struct probe_obj released;
	const uint8_t *bytes = (const uint8_t *)&released;

	probe_init(&released, 0U);
	test_cmd.rp_len = PROBE_RP_LEN;
	zassert_ok(bt_hci_cmd_send_cb(&released.cmd.op, &release_cmd),
		   "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not run");

	/* The workqueue that ran the callback has returned from it */
	test_bt_workq_flush();

	for (size_t i = 0U; i < sizeof(released); i++) {
		zassert_equal(bytes[i], RELEASED_BYTE, "Byte %zu of the object was written", i);
	}
}

/* A synchronous sender on the system workqueue sends the commands that are
 * ahead of its own by itself where the TX processor shares that queue. It
 * leaves the operations alone: their encoders rely on the host lock, which it
 * does not hold.
 */
static int syswq_send_err;
static int syswq_sync_err;
static unsigned int syswq_encode_count;

static void syswq_sender_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	syswq_send_err = bt_hci_cmd_send_async(&req, &param_cmd);
	syswq_sync_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	syswq_encode_count = encode_count;
	k_sem_give(&work_done);
}

static ZTEST(cmd_op, test_sender_on_system_workqueue)
{
	k_work_init(&syswq_work, syswq_sender_handler);
	zassert_true(k_work_submit(&syswq_work) >= 0, "Starting the sender failed");
	zassert_ok(k_sem_take(&work_done, TEST_TIMEOUT), "The sender did not finish");

	zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT), "The operation did not complete");
	zassert_ok(syswq_send_err, "Sending the operation failed");
	zassert_ok(syswq_sync_err, "The synchronous command failed");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_false(has_encode_run_unlocked, "The encoder ran without the host lock");

	if (!IS_ENABLED(CONFIG_BT_TX_PROCESSOR_THREAD)) {
		zassert_equal(syswq_encode_count, 0U,
			      "The operation was encoded by the synchronous sender");
		zassert_equal(test_cmd_plen[0], 0U, "The operation overtook the sender");
		zassert_equal(test_cmd_plen[1], 1U, "The operation had %u parameter bytes",
			      test_cmd_plen[1]);
	}
}

/* The controller takes the command credit back before anything has used it:
 * what wakes up a synchronous sender does not let it send, and its command is
 * sent when the next credit comes. Where the TX processor shares the system
 * workqueue, nothing else would send it.
 */
static int credit_err;

static void credit_sender_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	credit_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	k_sem_give(&work_done);
}

static ZTEST(cmd_op, test_sender_on_system_workqueue_credit_revoked)
{
	/* Taken back at once: the Host handles the event in this call */
	test_credit_revoke();

	k_work_init(&syswq_work, credit_sender_handler);
	zassert_true(k_work_submit(&syswq_work) >= 0, "Starting the sender failed");
	test_settle();
	zassert_equal(test_cmd.count, 0U, "The command was sent without a credit");

	test_credit_grant();
	zassert_ok(k_sem_take(&work_done, TEST_TIMEOUT), "The sender did not finish");
	zassert_ok(credit_err, "The synchronous command failed");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
}

/* A function that sends an operation from its stack and waits for it, as a
 * synchronous sender of the same command would be written.
 */
static int send_sync(void)
{
	struct bt_hci_cmd_op_fut local = { 0 };
	int err;

	err = bt_hci_cmd_send_async(&local, &param_cmd);
	if (err != 0) {
		return err;
	}

	return bt_hci_cmd_op_wait(&local.op);
}

/* A completion callback may wait for another operation: the callback runs on
 * the Bluetooth workqueue, and the response does not need that workqueue.
 */
static int wait_in_cb_err;

static void wait_in_cb(struct bt_hci_cmd_op *completed, int result)
{
	ARGS_UNUSED(completed, result);

	wait_in_cb_err = send_sync();
	k_sem_give(&cb_sem);
}

static const struct bt_hci_cmd_desc wait_in_cb_cmd = {
	.opcode = TEST_OPCODE,
	.done = wait_in_cb,
};

static ZTEST(cmd_op, test_wait_in_callback)
{
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &wait_in_cb_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not finish");
	zassert_ok(wait_in_cb_err, "The wait gave %d", wait_in_cb_err);
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
}

/* The wait returns what bt_hci_cmd_send_sync() returns for the same response
 * of the controller, and for a command that the driver did not take.
 */
static ZTEST(cmd_op, test_wait_returns_errno)
{
	static const struct {
		uint8_t status;
		int err;
	} responses[] = {
		{ BT_HCI_ERR_SUCCESS, 0 },
		{ BT_HCI_ERR_CONN_LIMIT_EXCEEDED, -ECONNREFUSED },
		{ BT_HCI_ERR_INSUFFICIENT_RESOURCES, -ENOMEM },
		{ BT_HCI_ERR_INVALID_PARAM, -EINVAL },
		{ BT_HCI_ERR_CMD_DISALLOWED, -EACCES },
		{ BT_HCI_ERR_UNKNOWN_CONN_ID, -EIO },
	};
	int err;

	ARRAY_FOR_EACH_PTR(responses, rsp) {
		test_cmd.status = rsp->status;

		err = send_sync();
		zassert_equal(err, rsp->err, "Status 0x%02x gave %d", rsp->status, err);

		err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
		zassert_equal(err, rsp->err, "The synchronous command gave %d", err);
	}

	test_cmd.status = BT_HCI_ERR_SUCCESS;
	test_cmd.send_err = -ENODEV;

	err = send_sync();
	zassert_equal(err, -EIO, "A failed send gave %d", err);

	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	zassert_equal(err, -EIO, "The synchronous command gave %d", err);
}

/* A command that its encoder refuses is not sent, which the waiter is told */
static ZTEST(cmd_op, test_wait_refused_by_encoder)
{
	int err;

	encode_refusal = -ENOTCONN;

	err = send_sync();
	zassert_equal(err, -ECANCELED, "The refused command gave %d", err);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");
}

/* Only an operation with a future can be waited for */
static ZTEST(cmd_op, test_wait_needs_future)
{
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_equal(bt_hci_cmd_op_wait(&req.op), -EINVAL, "A callback was waited for");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not run");
}

static int waiter_err;

static void waiter_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	waiter_err = send_sync();
	k_sem_give(&work_done);
}

/* The wait works on the system workqueue too, where the TX processor runs in
 * one scenario of this suite: the waiter then gives the operation its buffer
 * and sends the command by itself. Nothing is free for it at first: the
 * controller has a command, and the pool has no buffer.
 */
static ZTEST(cmd_op, test_wait_on_system_workqueue)
{
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&req, &plain_cmd), "Sending the operation failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The first command was not sent");

	(void)test_cmd_bufs_hold();
	k_work_init(&syswq_work, waiter_handler);
	zassert_true(k_work_submit(&syswq_work) >= 0, "Starting the waiter failed");
	test_settle();
	zassert_equal(k_sem_count_get(&work_done), 0U, "The waiter returned early");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* The waiter gets the buffer of the command that got the response */
	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);

	zassert_ok(k_sem_take(&work_done, TEST_TIMEOUT), "The waiter did not return");
	zassert_ok(waiter_err, "The waiter got %d", waiter_err);
	zassert_ok(bt_future_wait(&req.fut, K_NO_WAIT), "The first operation did not complete");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_equal(test_cmd_plen[1], 1U, "The command of the waiter had %u parameter bytes",
		      test_cmd_plen[1]);
	zassert_false(has_encode_run_unlocked, "The encoder ran without the host lock");
}

/* When the controller stops responding, every operation is handed back once,
 * whatever state it is in: with the controller, encoded, queued, or waiting
 * for its callback to run.
 */
static ZTEST(cmd_op, test_deadline_hands_back_all)
{
	static struct bt_hci_cmd_op_fut ops[3];
	static struct bt_hci_cmd_op cb_held_op;

	/* One whose callback cannot run yet, behind a blocked one */
	is_cb_gated = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not run");
	zassert_ok(bt_hci_cmd_send_cb(&cb_held_op, &plain_cmd), "Sending the operation failed");
	zassert_ok(test_rsp_wait(&test_cmd, 2U), "The second operation got no response");

	/* One with the controller, one encoded behind it, one without a buffer:
	 * the TX processor gives the operations their buffers before it sends
	 * a command
	 */
	test_cmd.delay = K_FOREVER;

	zassert_ok(bt_hci_cmd_send_async(&ops[0], &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&ops[1], &plain_cmd), "Sending the operation failed");
	zassert_ok(test_cmd_wait(&test_cmd, 3U), "The third command was not sent");
	(void)test_cmd_bufs_hold();
	zassert_ok(bt_hci_cmd_send_async(&ops[2], &plain_cmd), "Sending the operation failed");

	zassert_ok(bt_future_wait(&ops[0].fut, TEST_PAST_DEADLINE), "The future was not resolved");
	zassert_equal(ops[0].fut.result, -ETIMEDOUT, "The sent one gave %d", ops[0].fut.result);
	zassert_true(bt_future_is_done(&ops[1].fut), "The encoded one was not handed back");
	zassert_equal(ops[1].fut.result, -EHOSTDOWN, "The encoded one gave %d", ops[1].fut.result);
	zassert_true(bt_future_is_done(&ops[2].fut), "The queued one was not handed back");
	zassert_equal(ops[2].fut.result, -EHOSTDOWN, "The queued one gave %d", ops[2].fut.result);
	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	/* The one that had completed keeps its result */
	zassert_equal(cb_count, 1U, "The callback ran %u times", cb_count);
	k_sem_give(&cb_gate);
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The pending callback did not run");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
	zassert_equal(cb_result, 0, "The completed one gave %d", cb_result);

	ARRAY_FOR_EACH(ops, i) {
		zassert_false(bt_hci_cmd_op_is_pending(&ops[i].op), "Operation %zu is pending", i);
	}

	test_cmd_bufs_release();
	recover();
}

/* A command buffer that is freed goes to an operation that waits for one, not
 * to a thread that is blocked on the command pool.
 */
static int alloc_err;

static void alloc_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	/* A command without parameters */
	alloc_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);

	k_sem_give(&work_done);
}

static K_WORK_DEFINE(alloc_work, alloc_handler);

/* Each round checks the count of the pool after a buffer has changed hands */
#define HANDOVER_ROUNDS 5U

static ZTEST(cmd_op, test_freed_buffer_goes_to_operation)
{
	for (unsigned int round = 0U; round < HANDOVER_ROUNDS; round++) {
		test_driver_reset();
		(void)test_cmd_bufs_hold();

		/* A sender that blocks on the pool, then an operation, which
		 * has one parameter byte.
		 */
		zassert_true(k_work_submit_to_queue(&alloc_queue, &alloc_work) >= 0,
			     "Starting the sender failed");
		test_settle();
		zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
		test_settle();
		zassert_equal(test_cmd.count, 0U, "A command was sent without a buffer");

		net_buf_drop(&test_cmd_bufs[0]);

		zassert_ok(bt_future_wait(&req.fut, TEST_TIMEOUT),
			   "The operation did not complete");
		zassert_equal(test_cmd_plen[0], 1U, "The blocked sender got the freed buffer");

		test_cmd_bufs_release();
		zassert_ok(k_sem_take(&work_done, TEST_TIMEOUT),
			   "The blocked sender did not finish");
		zassert_ok(alloc_err, "The blocked sender got %d", alloc_err);
		zassert_equal(test_cmd.count, 2U, "The controller got %u commands",
			      test_cmd.count);

		/* A buffer that changed hands is still one buffer to the pool */
		test_cmd_pool_check();
	}
}

/* Nor do operations that keep coming starve a thread that is blocked on the
 * pool: the freed buffers alternate between the two. Three operations send
 * themselves again from their callbacks, so that an operation waits for a
 * buffer whenever one is freed.
 */
#define CHURN_OPS    3U
#define CHURN_ROUNDS 20U

/* Bound of the wait for the churn, which takes about 1.2 s: two ticks for each
 * of its 61 commands
 */
#define CHURN_TIMEOUT K_SECONDS(5)

static unsigned int churn_left;
static unsigned int churn_pending;
static unsigned int churn_failures;

/* Sends its operation again with churn_cmd, which names it */
static void churn_cb(struct bt_hci_cmd_op *completed, int result);

static const struct bt_hci_cmd_desc churn_cmd = {
	.opcode = TEST_OPCODE,
	.encode = encode,
	.done = churn_cb,
};

/* Counts its failures for the test to check, as it runs on the Bluetooth
 * workqueue
 */
static void churn_cb(struct bt_hci_cmd_op *completed, int result)
{
	if (result != 0) {
		churn_failures++;
	}

	if (churn_left > 0U) {
		churn_left--;
		if (bt_hci_cmd_send_cb(completed, &churn_cmd) == 0) {
			return;
		}

		churn_failures++;
	}

	churn_pending--;
	if (churn_pending == 0U) {
		k_sem_give(&cb_sem);
	}
}

static ZTEST(cmd_op, test_sender_not_starved_by_operations)
{
	static struct bt_hci_cmd_op churn[CHURN_OPS];
	bool is_sender_early = false;

	/* Each response comes a moment after its command, by when the
	 * callbacks of the others have sent their operations again
	 */
	test_cmd.delay = K_MSEC(1);

	/* A sender that blocks on the pool, then the operations */
	(void)test_cmd_bufs_hold();
	zassert_true(k_work_submit_to_queue(&alloc_queue, &alloc_work) >= 0,
		     "Starting the sender failed");
	test_settle();

	churn_left = CHURN_OPS * (CHURN_ROUNDS - 1U);
	churn_pending = CHURN_OPS;
	churn_failures = 0U;
	ARRAY_FOR_EACH(churn, i) {
		zassert_ok(bt_hci_cmd_send_cb(&churn[i], &churn_cmd),
			   "Sending operation %zu failed", i);
	}

	/* One buffer for all of them */
	net_buf_drop(&test_cmd_bufs[0]);

	zassert_ok(k_sem_take(&work_done, CHURN_TIMEOUT), "The sender never got a buffer");
	zassert_ok(alloc_err, "The sender got %d", alloc_err);
	zassert_ok(k_sem_take(&cb_sem, CHURN_TIMEOUT), "The operations did not all complete");
	zassert_equal(churn_failures, 0U, "The operations failed %u times", churn_failures);

	/* The command of the sender, the one without parameters, went out among
	 * the first commands rather than after the operations had all finished.
	 */
	ARRAY_FOR_EACH(test_cmd_plen, i) {
		if (test_cmd_plen[i] == 0U) {
			is_sender_early = true;
		}
	}

	zassert_true(is_sender_early, "The sender waited for the operations: lengths %u %u %u %u",
		     test_cmd_plen[0], test_cmd_plen[1], test_cmd_plen[2], test_cmd_plen[3]);
	zassert_equal(test_cmd.count, CHURN_OPS * CHURN_ROUNDS + 1U,
		      "The controller got %u commands", test_cmd.count);

	test_cmd_bufs_release();
	test_cmd_pool_check();
}

/* Synchronous commands from two threads and operations at once: every one of
 * them completes.
 */
#define LOAD_ROUNDS 50U

struct load_sender {
	struct k_work work;
	struct k_sem done;
	unsigned int failures;
};

static void load_handler(struct k_work *work)
{
	struct load_sender *load = CONTAINER_OF(work, struct load_sender, work);

	for (unsigned int i = 0U; i < LOAD_ROUNDS; i++) {
		if (bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL) != 0) {
			load->failures++;
		}
	}

	k_sem_give(&load->done);
}

/* Bound of the wait for the load, which takes about 3 s: two ticks for each of
 * its 150 commands
 */
#define LOAD_TIMEOUT K_SECONDS(5)

static ZTEST(cmd_op, test_mixed_load)
{
	static struct load_sender load[LOAD_SENDERS];

	/* Each response comes a moment after its command, so that the commands
	 * of the senders and the operations queue up
	 */
	test_cmd.delay = K_MSEC(1);

	ARRAY_FOR_EACH(load, i) {
		k_work_init(&load[i].work, load_handler);
		(void)k_sem_init(&load[i].done, 0U, 1U);
		load[i].failures = 0U;
		zassert_true(k_work_submit_to_queue(&load_queues[i], &load[i].work) >= 0,
			     "Starting a sender failed");
	}

	for (unsigned int i = 0U; i < LOAD_ROUNDS; i++) {
		zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending round %u failed", i);
		zassert_ok(bt_future_wait(&req.fut, LOAD_TIMEOUT), "Operation %u did not complete",
			   i);
		zassert_equal(req.fut.result, 0, "Operation %u gave %d", i, req.fut.result);
	}

	ARRAY_FOR_EACH(load, i) {
		zassert_ok(k_sem_take(&load[i].done, LOAD_TIMEOUT), "Sender %zu did not finish", i);
		zassert_equal(load[i].failures, 0U, "Sender %zu had %u failures", i,
			      load[i].failures);
	}

	/* The commands of the senders and of the operations */
	zassert_equal(test_cmd.count, (LOAD_SENDERS + 1U) * LOAD_ROUNDS,
		      "The controller got %u commands", test_cmd.count);
}

/* An operation that has its buffer and a command credit, but whose TX
 * processor does not run, here as a thread holds the host lock, is given up
 * by the deadline. Nothing was sent, so the controller is not.
 */
static ZTEST(cmd_op, test_deadline_gives_up_unsent)
{
	int64_t start;
	int64_t waited;
	int err;

	/* The operation gets its buffer behind a command that the controller
	 * has not responded to yet.
	 */
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send(TEST_OPCODE, NULL), "Sending a command failed");
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The first command was not sent");
	zassert_equal(encode_count, 1U, "The operation got no buffer");

	bt_dev_lock();
	start = k_uptime_get();
	test_cmd_respond(&test_cmd);
	err = bt_future_wait(&req.fut, TEST_PAST_DEADLINE);
	waited = k_uptime_get() - start;
	bt_dev_unlock();

	zassert_ok(err, "The operation was not handed back");
	zassert_equal(req.fut.result, -ENOBUFS, "The operation gave %d (!= %d)", req.fut.result,
		      -ENOBUFS);
	zassert_within(waited, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The operation took %lld ms", waited);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	test_cmd.delay = K_NO_WAIT;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	test_cmd_pool_check();
}
