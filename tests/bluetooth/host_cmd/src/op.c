/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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

#define DEADLINE_MS (10 * MSEC_PER_SEC)

/* The operation that most cases send, with its future */
static struct bt_hci_cmd_op_fut req;

/* More than the Host has command buffers in any configuration of this suite */
#define CMD_BUF_MAX 32U

/* Command buffers that a test keeps to itself */
static struct net_buf *held[CMD_BUF_MAX];

/* Take every free command buffer */
static unsigned int hold_buffers(void)
{
	unsigned int count = 0U;

	ARRAY_FOR_EACH(held, i) {
		held[i] = bt_hci_cmd_alloc(K_NO_WAIT);
		if (held[i] == NULL) {
			break;
		}

		count++;
	}

	zassert_true(count < ARRAY_SIZE(held), "The Host has more command buffers than expected");

	return count;
}

static void release_buffers(void)
{
	ARRAY_FOR_EACH(held, i) {
		if (held[i] != NULL) {
			net_buf_unref(held[i]);
			held[i] = NULL;
		}
	}
}

/* The command pool has all of its buffers, and knows it */
static void check_pool(void)
{
	struct net_buf_pool *pool;
	unsigned int count;

	count = hold_buffers();
	zassert_true(count > 0U, "No command buffer is free");
	pool = net_buf_pool_get(held[0]->pool_id);
	zassert_equal(count, pool->buf_count, "%u of %u command buffers are free", count,
		      pool->buf_count);
	zassert_equal(atomic_get(&pool->avail_count), 0, "The pool counts %ld free buffers",
		      atomic_get(&pool->avail_count));

	release_buffers();
	zassert_equal(atomic_get(&pool->avail_count), pool->buf_count,
		      "The pool counts %ld free buffers", atomic_get(&pool->avail_count));
}

static int encode_refusal;
static unsigned int encode_count;

static struct bt_hci_cmd_op *encode_op;
static bool encode_unlocked;
static bool encode_irq_locked;

/* For a test that has something to do while the encoder runs */
static K_SEM_DEFINE(encode_sem, 0, 1);
static K_SEM_DEFINE(encode_gate, 0, 1);
static bool encode_block;

static int encode(struct net_buf *buf, struct bt_hci_cmd_op *encoded)
{
	unsigned int key;

	encode_op = encoded;
	encode_count++;

	if (bt_dev.lock.owner != k_current_get()) {
		encode_unlocked = true;
	}

	key = irq_lock();
	if (!arch_irq_unlocked(key)) {
		encode_irq_locked = true;
	}

	irq_unlock(key);

	if (encode_block) {
		encode_block = false;
		k_sem_give(&encode_sem);
		(void)k_sem_take(&encode_gate, K_FOREVER);
	}

	if (encode_refusal != 0) {
		return encode_refusal;
	}

	net_buf_add_u8(buf, 0x5a);

	return 0;
}

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

static K_SEM_DEFINE(cb_sem, 0, 10);
static K_SEM_DEFINE(cb_gate, 0, 1);
static unsigned int cb_count;
static int cb_result;
static struct bt_hci_cmd_op *cb_op;
static k_tid_t cb_thread;
static bool cb_resend;
static bool cb_block;

static void op_cb(struct bt_hci_cmd_op *completed, int result)
{
	cb_count++;
	cb_result = result;
	cb_op = completed;
	cb_thread = k_current_get();

	if (cb_resend) {
		cb_resend = false;
		zassert_ok(bt_hci_cmd_send_cb(completed, &param_cmd),
			   "Sending the operation again from its callback failed");
	}

	k_sem_give(&cb_sem);

	if (cb_block) {
		cb_block = false;
		(void)k_sem_take(&cb_gate, K_FOREVER);
	}
}

static void recover(void)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_driver_reset();
	encode_refusal = 0;
	encode_count = 0U;
	encode_unlocked = false;
	encode_irq_locked = false;
	encode_block = false;
	k_sem_reset(&encode_sem);
	k_sem_reset(&encode_gate);
	cb_count = 0U;
	cb_resend = false;
	cb_block = false;
	k_sem_reset(&cb_sem);
	k_sem_reset(&cb_gate);

	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	release_buffers();
}

ZTEST_SUITE(cmd_op, test_with_reset, NULL, before, after, NULL);

/* The result of a command is the status that the controller responded with */
static ZTEST(cmd_op, test_future)
{
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The operation is pending before its use");

	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
	zassert_false(bt_hci_cmd_op_is_pending(&req.op),
		      "The operation is pending after its result");

	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(req.fut.result, BT_HCI_ERR_CMD_DISALLOWED,
		      "The result is %d", req.fut.result);
	zassert_equal(bt_hci_cmd_op_status(&req.op), BT_HCI_ERR_CMD_DISALLOWED, "Wrong status");

	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_equal(encode_count, 2U, "The encoder ran %u times", encode_count);
	zassert_equal_ptr(encode_op, &req.op, "The encoder got another operation");
	zassert_false(encode_unlocked, "The encoder ran without the host lock");
	zassert_false(encode_irq_locked, "The encoder ran with interrupts locked");
}

/* A callback runs on another thread than the sender's, gets the result and its
 * context, and may send the operation again.
 */
static ZTEST(cmd_op, test_callback)
{
	cb_resend = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");

	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run again");
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
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The operation is pending");

	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &untold_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The failed operation is pending");
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
	bool pending;
};

static int keep_rsp(struct bt_hci_cmd_op *op, struct net_buf *rp)
{
	struct keep_obj *obj = CONTAINER_OF(op, struct keep_obj, cmd.op);

	obj->count++;
	obj->len = rp->len;
	obj->pending = bt_hci_cmd_op_is_pending(op);

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
static ZTEST(cmd_op, test_return_parameters)
{
	static struct keep_obj obj;

	obj.max = sizeof(obj.rp);
	test_cmd.rp_len = 8U;

	zassert_ok(bt_hci_cmd_send_async(&obj.cmd, &keep_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&obj.cmd.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(obj.cmd.fut.result, 0, "The result is %d", obj.cmd.fut.result);
	zassert_equal(obj.count, 1U, "The handler ran %u times", obj.count);
	zassert_true(obj.pending, "The handler ran after the hand-back");
	zassert_equal(obj.len, 9U, "The handler got %u bytes", obj.len);
	zassert_equal(obj.rp[0], BT_HCI_ERR_SUCCESS, "The status does not come first");
	zassert_equal(obj.rp[8], 7U, "Wrong last parameter byte");

	/* More return parameters than the handler can use */
	obj.max = 4U;
	zassert_ok(bt_hci_cmd_send_async(&obj.cmd, &keep_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&obj.cmd.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(obj.cmd.fut.result, -EMSGSIZE, "The result is %d", obj.cmd.fut.result);
	zassert_equal(bt_hci_cmd_op_status(&obj.cmd.op), BT_HCI_ERR_SUCCESS,
		      "The status of the controller was lost");

	/* The controller responds, but not with success */
	obj.count = 0U;
	test_cmd.status = BT_HCI_ERR_INVALID_PARAM;
	zassert_ok(bt_hci_cmd_send_async(&obj.cmd, &keep_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&obj.cmd.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(obj.cmd.fut.result, BT_HCI_ERR_INVALID_PARAM, "The result is %d",
		      obj.cmd.fut.result);
	zassert_equal(obj.count, 0U, "The handler got the response to a failed command");
	zassert_equal(bt_hci_cmd_op_status(&obj.cmd.op), BT_HCI_ERR_INVALID_PARAM,
		      "The status of the controller was lost");

	/* A command whose description has no handler */
	test_cmd.status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
}

/* Results that nobody picks up hold no command buffer: more commands than
 * there are buffers complete, and a synchronous command still goes through.
 */
static ZTEST(cmd_op, test_results_not_picked_up)
{
	static struct keep_obj kept;
	unsigned int buffers = hold_buffers();

	release_buffers();
	kept.max = sizeof(kept.rp);
	test_cmd.rp_len = 4U;

	for (unsigned int i = 0U; i < buffers + 1U; i++) {
		zassert_ok(bt_hci_cmd_send_async(&kept.cmd, &keep_cmd),
			   "Sending round %u failed", i);
		k_sleep(K_MSEC(1));
		zassert_true(bt_future_is_done(&kept.cmd.fut), "Operation %u did not complete", i);
		zassert_equal(kept.len, 5U, "Operation %u had %u bytes", i, kept.len);
	}

	zassert_equal(hold_buffers(), buffers, "Completed operations hold command buffers");
	release_buffers();

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

	net_buf_add_u8(buf, probe->id);

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

static void probe_cb(struct bt_hci_cmd_op *completed, int result);

static const struct bt_hci_cmd_desc probe_cmd = {
	.opcode = TEST_OPCODE,
	.encode = probe_encode,
	.rsp = probe_rsp,
	.done = probe_cb,
};

/* The callback of the first probe waits here before it reads anything */
static bool probe_hold;

static void probe_cb(struct bt_hci_cmd_op *completed, int result)
{
	struct probe_obj *probe = CONTAINER_OF(completed, struct probe_obj, cmd.op);

	if (probe_hold) {
		probe_hold = false;
		k_sem_give(&cb_sem);
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

static void probe_init(struct probe_obj *probe, uint8_t id)
{
	(void)memset(probe, 0, sizeof(*probe));
	probe->id = id;
}

/* A thread that sends the command of an object and ends, so that nothing of
 * it is left when the controller responds.
 */
static K_THREAD_STACK_DEFINE(sender_stack, 2048);
static struct k_thread sender_thread;
static int sender_err;

static void sender_cb_entry(void *p1, void *p2, void *p3)
{
	struct probe_obj *probe = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sender_err = bt_hci_cmd_send_cb(&probe->cmd.op, &probe_cmd);
}

static void sender_future_entry(void *p1, void *p2, void *p3)
{
	struct probe_obj *probe = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sender_err = bt_hci_cmd_send_async(&probe->cmd, &probe_cmd);
}

static void send_from_ended_thread(k_thread_entry_t entry, struct probe_obj *probe)
{
	sender_err = -EAGAIN;
	(void)k_thread_create(&sender_thread, sender_stack, K_THREAD_STACK_SIZEOF(sender_stack),
			      entry, probe, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&sender_thread, K_SECONDS(1)), "The sender did not end");
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
	static struct probe_obj probe;
	const uint8_t first_id = 0x10U;

	zassert_true(bt_work_submit(&workq_thread_work) >= 0, "Submitting work failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The Bluetooth workqueue did not run");

	probe_init(&probe, first_id);
	probe.more = 2U;
	test_cmd.delay = K_FOREVER;
	test_cmd.rp_len = PROBE_RP_LEN;
	test_cmd.echo = true;

	send_from_ended_thread(sender_cb_entry, &probe);

	for (uint8_t i = 0U; i < 3U; i++) {
		k_sleep(K_MSEC(10));
		zassert_equal(test_cmd.count, i + 1U, "The controller got %u commands",
			      test_cmd.count);
		zassert_equal(k_sem_count_get(&cb_sem), 0U, "Command %u completed early", i);

		test_cmd_respond(&test_cmd);
		zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "Command %u did not complete", i);
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
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
	zassert_false(bt_future_is_done(&probe.cmd.fut), "The future was resolved early");

	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&probe.cmd.fut, K_SECONDS(1)), "The future was not resolved");
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

	probe_hold = true;
	test_cmd.delay = K_MSEC(5);
	test_cmd.rp_len = PROBE_RP_LEN;
	test_cmd.echo = true;

	ARRAY_FOR_EACH(probes, i) {
		probe_init(&probes[i], (uint8_t)(0x30U + i));
		zassert_ok(bt_hci_cmd_send_cb(&probes[i].cmd.op, &probe_cmd),
			   "Sending operation %zu failed", i);
	}

	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The first callback did not run");
	k_sleep(K_MSEC(50));
	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	k_sem_give(&cb_gate);
	ARRAY_FOR_EACH(probes, i) {
		zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "Completion %zu is missing", i);
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
	zassert_ok(bt_future_wait(&probe.cmd.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(probe.len, sizeof(probe.rp), "A command with a response reached no handler");

	/* The driver fails to send the command */
	probe.len = 0U;
	test_cmd.send_err = -EIO;
	zassert_ok(bt_hci_cmd_send_async(&probe.cmd, &probe_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&probe.cmd.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(probe.cmd.fut.result, -EIO, "The result is %d", probe.cmd.fut.result);
	zassert_equal(probe.len, 0U, "An unsent command reached the handler");
	test_cmd.send_err = 0;

	zassert_ok(bt_hci_cmd_send_async(&probe.cmd, &probe_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&probe.cmd.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(probe.len, sizeof(probe.rp), "A command with a response reached no handler");

	/* Canceled while it waits behind a command that the controller keeps */
	probe.len = 0U;
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&outstanding, &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&probe.cmd, &probe_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_ok(bt_hci_cmd_op_cancel(&probe.cmd.op), "Canceling the operation failed");
	zassert_equal(probe.cmd.fut.result, -ECANCELED, "The result is %d",
		      probe.cmd.fut.result);
	zassert_equal(probe.len, 0U, "A canceled command reached the handler");

	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&outstanding.fut, K_SECONDS(1)), "The future was not resolved");
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
	zassert_ok(bt_future_wait(&outstanding.fut, K_SECONDS(1)), "The future was not resolved");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 2U, "A canceled command reached the controller");
}

/* A blocked callback holds up the callbacks behind it, but not the commands */
static ZTEST(cmd_op, test_callback_blocked)
{
	static struct bt_hci_cmd_op second;
	unsigned int buffers;

	cb_block = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");

	zassert_ok(bt_hci_cmd_send_cb(&second, &plain_cmd), "Sending the operation failed");

	buffers = hold_buffers();
	release_buffers();

	for (unsigned int i = 0U; i < buffers + 1U; i++) {
		zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
			   "A synchronous command failed behind a blocked callback");
	}

	zassert_equal(cb_count, 1U, "The second callback ran past the blocked one");
	zassert_equal(bt_hci_cmd_op_cancel(&second), -EINPROGRESS,
		      "A completed operation could be canceled");

	k_sem_give(&cb_gate);
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The second callback did not run");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
}

/* A call that fails changes nothing */
static ZTEST(cmd_op, test_busy)
{
	test_cmd.delay = K_MSEC(100);
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_equal(bt_hci_cmd_send_async(&req, &param_cmd), -EBUSY,
		      "A pending operation was accepted again");
	zassert_equal(bt_hci_cmd_send_cb(&req.op, &param_cmd), -EBUSY,
		      "A pending operation was accepted again");

	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
	zassert_equal(cb_count, 0U, "The refused send got a callback");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
}

/* An operation that its encoder refuses never reaches the controller */
static ZTEST(cmd_op, test_encoder_refuses)
{
	encode_refusal = -ESTALE;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command buffer of the refused operation was not released");
}

/* A cancel in each state of an operation */
static ZTEST(cmd_op, test_cancel)
{
	static struct bt_hci_cmd_op_fut outstanding;
	unsigned int all;

	zassert_equal(bt_hci_cmd_op_cancel(&req.op), -EALREADY, "An idle operation was canceled");

	/* Queued: no command buffer to be had */
	all = hold_buffers();
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(encode_count, 0U, "The operation got a buffer");
	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling a queued operation failed");
	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The canceled operation is pending");
	release_buffers();
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 0U, "A canceled command reached the controller");
	zassert_equal(cb_count, 0U, "A canceled operation got its callback");

	/* Encoded, but the controller takes no command */
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&outstanding, &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(encode_count, 1U, "The operation got no buffer");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* With the controller */
	zassert_equal(bt_hci_cmd_op_cancel(&outstanding.op), -EINPROGRESS,
		      "A command that is with the controller was canceled");

	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling an encoded operation failed");
	zassert_true(bt_future_is_done(&req.fut),
		     "The future of a canceled operation is unresolved");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);

	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&outstanding.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(outstanding.fut.result, 0, "The result is %d", outstanding.fut.result);
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "A canceled command reached the controller");

	/* The buffer of the canceled operation is free again, and so is the credit */
	zassert_equal(hold_buffers(), all, "A command buffer is missing");
	release_buffers();
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command credit is missing");
}

/* A cancel that comes while the encoder runs is too late to get the operation
 * back, as the encoder may be using it, and early enough to keep the command
 * from the controller: the operation completes as canceled.
 */
static ZTEST(cmd_op, test_cancel_while_encoding)
{
	encode_block = true;
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&encode_sem, K_SECONDS(1)), "The encoder did not run");

	zassert_equal(bt_hci_cmd_op_cancel(&req.op), -EINPROGRESS,
		      "An operation was handed back under its encoder");
	zassert_true(bt_hci_cmd_op_is_pending(&req.op), "The operation is not pending");
	zassert_false(bt_future_is_done(&req.fut), "The future was resolved under the encoder");

	k_sem_give(&encode_gate);
	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);
	zassert_equal(test_cmd.count, 0U, "A canceled command reached the controller");

	/* The same with a callback, which is told */
	encode_block = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&encode_sem, K_SECONDS(1)), "The encoder did not run");
	zassert_equal(bt_hci_cmd_op_cancel(&req.op), -EINPROGRESS,
		      "An operation was handed back under its encoder");
	k_sem_give(&encode_gate);
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback was not invoked");
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
static ZTEST(cmd_op, test_cancel_or_complete)
{
	unsigned int canceled = 0U;
	unsigned int completed = 0U;
	unsigned int stopped = 0U;

	for (unsigned int i = 0U; i < 200U; i++) {
		unsigned int before_count = test_cmd.count;
		int err;

		zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");

		/* Let the TX processor get further every few rounds */
		if ((i % 4U) != 0U) {
			k_yield();
		}

		err = bt_hci_cmd_op_cancel(&req.op);
		zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");

		if (err == 0) {
			canceled++;
			zassert_equal(req.fut.result, -ECANCELED,
				      "Round %u: result %d", i, req.fut.result);
			k_sleep(K_MSEC(1));
			zassert_equal(test_cmd.count, before_count,
				      "Round %u: a canceled command reached the controller", i);
		} else if (req.fut.result == -ECANCELED) {
			/* Caught under its encoder, which another CPU can run
			 * during the cancel
			 */
			stopped++;
			zassert_equal(err, -EINPROGRESS, "Round %u: cancel gave %d", i, err);
			k_sleep(K_MSEC(1));
			zassert_equal(test_cmd.count, before_count,
				      "Round %u: a canceled command reached the controller", i);
		} else {
			completed++;
			zassert_equal(req.fut.result, 0, "Round %u: result %d", i, req.fut.result);
			zassert_equal(test_cmd.count, before_count + 1U,
				      "Round %u: the controller got no command", i);
		}
	}

	TC_PRINT("%u canceled, %u stopped under the encoder, %u completed\n", canceled, stopped,
		 completed);
	zassert_true(canceled > 0U, "No round was canceled");
	zassert_true(completed > 0U, "No round completed");
}

/* An operation that finds no command buffer, with nothing on its way that
 * would free one: the deadline ends its wait. The controller has done nothing
 * wrong, so the Host carries on.
 */
static ZTEST(cmd_op, test_no_buffer)
{
	int64_t start = k_uptime_get();
	int64_t waited;

	hold_buffers();
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");

	zassert_ok(bt_future_wait(&req.fut, K_MSEC(DEADLINE_MS + MSEC_PER_SEC)),
		   "The future was not resolved");
	waited = k_uptime_get() - start;
	zassert_equal(req.fut.result, -ENOBUFS, "The result is %d", req.fut.result);
	zassert_within(waited, DEADLINE_MS, 100, "The operation took %lld ms", waited);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");

	/* The next one is accepted, and waits like the first */
	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd),
		   "The Host gave up on its controller over a missing buffer");
	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling the queued operation failed");

	release_buffers();
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	check_pool();
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
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* Another command with the same opcode, and room for it */
	test_credit_grant();
	zassert_ok(bt_hci_cmd_send_async(&second, &plain_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "A command was sent while another was outstanding");

	/* The response is that of the first command, and lets the second go */
	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(req.fut.result, BT_HCI_ERR_CMD_DISALLOWED,
		      "The result is %d", req.fut.result);
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_false(bt_future_is_done(&second.fut), "The second command completed early");

	test_cmd.status = BT_HCI_ERR_SUCCESS;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&second.fut, K_SECONDS(1)), "The future was not resolved");
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
	unsigned int all;

	all = hold_buffers();
	release_buffers();

	/* The controller responds, and takes no further command for now */
	test_cmd.ncmd = 0U;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command failed");

	zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(encode_count, 1U, "The operation got no buffer");
	zassert_equal(test_cmd.count, 1U, "A command was sent without a credit");

	zassert_ok(bt_hci_cmd_op_cancel(&req.op), "Canceling the encoded operation failed");
	zassert_equal(req.fut.result, -ECANCELED, "The result is %d", req.fut.result);
	zassert_equal(hold_buffers(), all, "The buffer of the canceled operation is not free");
	release_buffers();

	k_sleep(K_MSEC(DEADLINE_MS + MSEC_PER_SEC));

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

	test_cmd.delay = K_MSEC(5);

	ARRAY_FOR_EACH(sized, i) {
		sized[i].len = (uint8_t)(i + 1U);
		zassert_ok(bt_hci_cmd_send_async(&sized[i].cmd, &sized_cmd),
			   "Sending operation %zu failed", i);
	}

	ARRAY_FOR_EACH(sized, i) {
		zassert_ok(bt_future_wait(&sized[i].cmd.fut, K_SECONDS(1)),
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
static void release_cb(struct bt_hci_cmd_op *completed, int result)
{
	struct probe_obj *probe = CONTAINER_OF(completed, struct probe_obj, cmd.op);

	ARG_UNUSED(result);

	/* What the next owner of the memory might do with it */
	(void)memset(probe, 0xa5, sizeof(*probe));
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
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");

	/* Whatever ran the callback has returned from it by now */
	k_sleep(K_MSEC(10));

	for (size_t i = 0U; i < sizeof(released); i++) {
		zassert_equal(bytes[i], 0xa5, "Byte %zu of the object was written", i);
	}
}

/* A synchronous sender on the system workqueue sends the commands that are
 * ahead of its own by itself where the TX processor shares that queue. It
 * leaves the operations alone: their encoders rely on the host lock, which it
 * does not hold.
 */
static K_SEM_DEFINE(syswq_done, 0, 1);
static int syswq_send_err;
static int syswq_sync_err;
static unsigned int syswq_encode_count;

static void syswq_sender_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	syswq_send_err = bt_hci_cmd_send_async(&req, &param_cmd);
	syswq_sync_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	syswq_encode_count = encode_count;
	k_sem_give(&syswq_done);
}

static K_WORK_DEFINE(syswq_sender_work, syswq_sender_handler);

static ZTEST(cmd_op, test_sender_on_system_workqueue)
{
	k_sem_reset(&syswq_done);
	zassert_true(k_work_submit(&syswq_sender_work) >= 0, "Starting the sender failed");
	zassert_ok(k_sem_take(&syswq_done, K_SECONDS(1)), "The sender did not finish");

	zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)), "The operation did not complete");
	zassert_ok(syswq_send_err, "Sending the operation failed");
	zassert_ok(syswq_sync_err, "The synchronous command failed");
	zassert_equal(req.fut.result, 0, "The result is %d", req.fut.result);
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_false(encode_unlocked, "The encoder ran without the host lock");

	if (!IS_ENABLED(CONFIG_BT_TX_PROCESSOR_THREAD)) {
		zassert_equal(syswq_encode_count, 0U,
			      "The operation was encoded by the synchronous sender");
		zassert_equal(test_cmd_plen[0], 0U, "The operation overtook the sender");
		zassert_equal(test_cmd_plen[1], 1U, "The operation had %u parameter bytes",
			      test_cmd_plen[1]);
	}
}

/* A function that sends an operation from its stack and waits for it, as a
 * synchronous sender of the same command would be written.
 */
static int send_sync(void)
{
	struct bt_hci_cmd_op_fut local = {0};
	int err;

	err = bt_hci_cmd_send_async(&local, &param_cmd);
	if (err != 0) {
		return err;
	}

	return bt_hci_cmd_op_wait(&local.op);
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
		{BT_HCI_ERR_SUCCESS, 0},
		{BT_HCI_ERR_CONN_LIMIT_EXCEEDED, -ECONNREFUSED},
		{BT_HCI_ERR_INSUFFICIENT_RESOURCES, -ENOMEM},
		{BT_HCI_ERR_INVALID_PARAM, -EINVAL},
		{BT_HCI_ERR_CMD_DISALLOWED, -EACCES},
		{BT_HCI_ERR_UNKNOWN_CONN_ID, -EIO},
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
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");
}

static K_SEM_DEFINE(waiter_done, 0, 1);
static int waiter_err;

static void waiter_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	waiter_err = send_sync();
	k_sem_give(&waiter_done);
}

static K_WORK_DEFINE(waiter_work, waiter_handler);

/* The wait works on the system workqueue too, where the TX processor runs in
 * one scenario of this suite: the waiter then gives the operation its buffer
 * and sends the command by itself. Nothing is free for it at first: the
 * controller has a command, and the pool has no buffer.
 */
static ZTEST(cmd_op, test_wait_on_system_workqueue)
{
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&req, &plain_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	(void)hold_buffers();
	k_sem_reset(&waiter_done);
	zassert_true(k_work_submit(&waiter_work) >= 0, "Starting the waiter failed");
	k_sleep(K_MSEC(20));
	zassert_equal(k_sem_count_get(&waiter_done), 0U, "The waiter returned early");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* The waiter gets the buffer of the command that got the response */
	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);

	zassert_ok(k_sem_take(&waiter_done, K_SECONDS(1)), "The waiter did not return");
	zassert_ok(waiter_err, "The waiter got %d", waiter_err);
	zassert_ok(bt_future_wait(&req.fut, K_NO_WAIT), "The first operation did not complete");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_equal(test_cmd_plen[1], 1U, "The command of the waiter had %u parameter bytes",
		      test_cmd_plen[1]);
	zassert_false(encode_unlocked, "The encoder ran without the host lock");
}

/* When the controller stops responding, every operation is handed back once,
 * whatever state it is in: with the controller, encoded, queued, or waiting
 * for its callback to run.
 */
static ZTEST(cmd_op, test_deadline_hands_back_all)
{
	static struct bt_hci_cmd_op_fut ops[3];
	static struct bt_hci_cmd_op done;

	/* One whose callback cannot run yet, behind a blocked one */
	cb_block = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");
	zassert_ok(bt_hci_cmd_send_cb(&done, &plain_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));

	/* One with the controller, one encoded behind it, one without a buffer */
	test_cmd.delay = K_FOREVER;

	zassert_ok(bt_hci_cmd_send_async(&ops[0], &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&ops[1], &plain_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	hold_buffers();
	zassert_ok(bt_hci_cmd_send_async(&ops[2], &plain_cmd), "Sending the operation failed");

	zassert_ok(bt_future_wait(&ops[0].fut, K_MSEC(DEADLINE_MS + MSEC_PER_SEC)),
		   "The future was not resolved");
	zassert_equal(ops[0].fut.result, -ETIMEDOUT, "The sent one gave %d", ops[0].fut.result);
	zassert_true(bt_future_is_done(&ops[1].fut), "The encoded one was not handed back");
	zassert_equal(ops[1].fut.result, -EHOSTDOWN, "The encoded one gave %d", ops[1].fut.result);
	zassert_true(bt_future_is_done(&ops[2].fut), "The queued one was not handed back");
	zassert_equal(ops[2].fut.result, -EHOSTDOWN, "The queued one gave %d", ops[2].fut.result);
	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	/* The one that had completed keeps its result */
	zassert_equal(cb_count, 1U, "The callback ran %u times", cb_count);
	k_sem_give(&cb_gate);
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The pending callback did not run");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
	zassert_equal(cb_result, 0, "The completed one gave %d", cb_result);

	ARRAY_FOR_EACH(ops, i) {
		zassert_false(bt_hci_cmd_op_is_pending(&ops[i].op), "Operation %zu is pending", i);
	}

	release_buffers();
	recover();
}

/* A command buffer that is freed goes to an operation that waits for one, not
 * to a thread that is blocked on the command pool.
 */
static void alloc_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	/* A command without parameters */
	(void)bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
}

static K_WORK_DEFINE(alloc_work, alloc_handler);
static K_THREAD_STACK_DEFINE(alloc_stack, 2048);
static struct k_work_q alloc_queue;

static ZTEST(cmd_op, test_freed_buffer_goes_to_operation)
{
	static bool started;
	struct net_buf *buf;

	if (!started) {
		k_work_queue_init(&alloc_queue);
		k_work_queue_start(&alloc_queue, alloc_stack, K_THREAD_STACK_SIZEOF(alloc_stack),
				   K_PRIO_PREEMPT(1), NULL);
		started = true;
	}

	for (unsigned int round = 0U; round < 5U; round++) {
		test_driver_reset();
		(void)hold_buffers();

		/* A sender that blocks on the pool, then an operation, which
		 * has one parameter byte.
		 */
		zassert_true(k_work_submit_to_queue(&alloc_queue, &alloc_work) >= 0,
			     "Starting the sender failed");
		k_sleep(K_MSEC(10));
		zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending the operation failed");
		k_sleep(K_MSEC(10));
		zassert_equal(test_cmd.count, 0U, "A command was sent without a buffer");

		buf = held[0];
		held[0] = NULL;
		net_buf_unref(buf);

		zassert_ok(bt_future_wait(&req.fut, K_SECONDS(1)),
			   "The operation did not complete");
		zassert_equal(test_cmd_plen[0], 1U, "The blocked sender got the freed buffer");

		release_buffers();
		k_sleep(K_MSEC(10));
		zassert_equal(test_cmd.count, 2U, "The controller got %u commands",
			      test_cmd.count);

		/* A buffer that changed hands is still one buffer to the pool */
		check_pool();
	}
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

static K_THREAD_STACK_ARRAY_DEFINE(load_stacks, 2, 2048);
static struct k_work_q load_queues[2];

static ZTEST(cmd_op, test_mixed_load)
{
	static struct load_sender load[2];
	static bool started;

	if (!started) {
		ARRAY_FOR_EACH(load_queues, i) {
			k_work_queue_init(&load_queues[i]);
			k_work_queue_start(&load_queues[i], load_stacks[i],
					   K_THREAD_STACK_SIZEOF(load_stacks[i]),
					   K_PRIO_PREEMPT(1 + i), NULL);
		}

		started = true;
	}

	test_cmd.delay = K_MSEC(1);

	ARRAY_FOR_EACH(load, i) {
		k_work_init(&load[i].work, load_handler);
		k_sem_init(&load[i].done, 0, 1);
		load[i].failures = 0U;
		zassert_true(k_work_submit_to_queue(&load_queues[i], &load[i].work) >= 0,
			     "Starting a sender failed");
	}

	for (unsigned int i = 0U; i < LOAD_ROUNDS; i++) {
		zassert_ok(bt_hci_cmd_send_async(&req, &param_cmd), "Sending round %u failed", i);
		zassert_ok(bt_future_wait(&req.fut, K_SECONDS(5)),
			   "Operation %u did not complete", i);
		zassert_equal(req.fut.result, 0, "Operation %u gave %d", i, req.fut.result);
	}

	ARRAY_FOR_EACH(load, i) {
		zassert_ok(k_sem_take(&load[i].done, K_SECONDS(5)), "Sender %zu did not finish", i);
		zassert_equal(load[i].failures, 0U, "Sender %zu had %u failures", i,
			      load[i].failures);
	}

	zassert_equal(test_cmd.count, 3U * LOAD_ROUNDS, "The controller got %u commands",
		      test_cmd.count);
}

/* bt_disable() hands every operation back once, whatever state it is in: with
 * the controller, encoded, queued, or waiting for its callback to run. This
 * needs a controller that the Host does not reset, as the reset of
 * bt_disable() would not get past a command that is outstanding.
 */
ZTEST_SUITE(cmd_op_no_reset, test_without_reset, NULL, before, after, NULL);

static ZTEST(cmd_op_no_reset, test_disable_hands_back_all)
{
	static struct bt_hci_cmd_op_fut ops[3];
	static struct bt_hci_cmd_op done;

	/* One whose callback cannot run yet, behind a blocked one */
	cb_block = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");
	zassert_ok(bt_hci_cmd_send_cb(&done, &plain_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));

	/* One with the controller, one encoded behind it, one without a buffer */
	test_cmd.delay = K_FOREVER;

	zassert_ok(bt_hci_cmd_send_async(&ops[0], &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&ops[1], &plain_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	hold_buffers();
	zassert_ok(bt_hci_cmd_send_async(&ops[2], &plain_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	ARRAY_FOR_EACH(ops, i) {
		zassert_true(bt_future_is_done(&ops[i].fut),
			     "Operation %zu was not handed back", i);
		zassert_equal(ops[i].fut.result, -EHOSTDOWN, "Operation %zu gave %d", i,
			      ops[i].fut.result);
		zassert_false(bt_hci_cmd_op_is_pending(&ops[i].op), "Operation %zu is pending", i);
	}

	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	/* The one that had completed keeps its result */
	zassert_equal(cb_count, 1U, "The callback ran %u times", cb_count);
	k_sem_give(&cb_gate);
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The pending callback did not run");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
	zassert_equal(cb_result, 0, "The completed one gave %d", cb_result);

	release_buffers();
	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	check_pool();
}

/* bt_disable() purges an operation that has its command buffer and waits for
 * a command credit, and a thread of higher priority cancels it meanwhile: the
 * purge itself wakes that thread up when it completes the synchronous command
 * queued ahead of the operation. The operation is handed back once, by the
 * cancel or by the purge, never by both and never by neither.
 */
static K_THREAD_STACK_ARRAY_DEFINE(race_stacks, 2, 2048);
static struct k_thread race_threads[2];
static int race_sync_err;
static int race_cancel_err;
static int race_disable_err;

static void race_cancel_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	race_sync_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	race_cancel_err = bt_hci_cmd_op_cancel(&req.op);
}

static void race_disable_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	race_disable_err = bt_disable();
}

static ZTEST(cmd_op_no_reset, test_disable_races_cancel)
{
	unsigned int handbacks;

	/* The controller responds, and takes no further command */
	test_cmd.ncmd = 0U;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command failed");

	k_thread_create(&race_threads[0], race_stacks[0], K_THREAD_STACK_SIZEOF(race_stacks[0]),
			race_cancel_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(encode_count, 1U, "The operation got no buffer");
	zassert_equal(test_cmd.count, 1U, "A command was sent without a credit");

	k_thread_create(&race_threads[1], race_stacks[1], K_THREAD_STACK_SIZEOF(race_stacks[1]),
			race_disable_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&race_threads[1], K_SECONDS(1)), "bt_disable() did not return");
	zassert_ok(k_thread_join(&race_threads[0], K_SECONDS(1)), "The cancel did not return");
	zassert_ok(race_disable_err, "Bluetooth disable failed");
	zassert_equal(race_sync_err, -EHOSTDOWN, "The synchronous command gave %d (!= %d)",
		      race_sync_err, -EHOSTDOWN);

	/* A callback that is on its way has run by now */
	(void)k_sem_take(&cb_sem, K_MSEC(100));

	handbacks = cb_count + ((race_cancel_err == 0) ? 1U : 0U);
	TC_PRINT("Cancel gave %d, %u callbacks\n", race_cancel_err, cb_count);
	zassert_equal(handbacks, 1U, "The operation was handed back %u times", handbacks);
	if (cb_count > 0U) {
		zassert_equal(cb_result, -EHOSTDOWN, "The result is %d", cb_result);
	}

	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The operation is pending");

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	check_pool();
}
