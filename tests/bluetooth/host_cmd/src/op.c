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

static struct bt_hci_cmd_op op;
static struct bt_future fut;

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

static int encode(struct net_buf *buf, struct bt_hci_cmd_op *encoded)
{
	encode_op = encoded;
	encode_count++;

	if (bt_dev.lock.owner != k_current_get()) {
		encode_unlocked = true;
	}

	if (encode_refusal != 0) {
		return encode_refusal;
	}

	net_buf_add_u8(buf, 0x5a);

	return 0;
}

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
		zassert_ok(bt_hci_cmd_send_cb(completed, NULL, op_cb),
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
	cb_count = 0U;
	cb_resend = false;
	cb_block = false;
	k_sem_reset(&cb_sem);
	k_sem_reset(&cb_gate);

	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	bt_hci_cmd_op_init(&op, TEST_OPCODE, encode);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	release_buffers();
}

ZTEST_SUITE(cmd_op, test_with_reset, NULL, before, after, NULL);

/* The result of a command is the status that the controller answered with */
static ZTEST(cmd_op, test_future)
{
	zassert_false(bt_hci_cmd_op_is_pending(&op), "The operation is pending before its use");

	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(fut.result, 0, "The result is %d", fut.result);
	zassert_false(bt_hci_cmd_op_is_pending(&op), "The operation is pending after its result");

	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(fut.result, BT_HCI_ERR_CMD_DISALLOWED, "The result is %d", fut.result);
	zassert_equal(bt_hci_cmd_op_status(&op), BT_HCI_ERR_CMD_DISALLOWED, "Wrong status");

	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_equal(encode_count, 2U, "The encoder ran %u times", encode_count);
	zassert_equal_ptr(encode_op, &op, "The encoder got another operation");
	zassert_false(encode_unlocked, "The encoder ran without the host lock");
}

/* A callback runs on another thread than the sender's, gets the result and its
 * context, and may send the operation again.
 */
static ZTEST(cmd_op, test_callback)
{
	cb_resend = true;
	zassert_ok(bt_hci_cmd_send_cb(&op, NULL, op_cb), "Sending the operation failed");

	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run again");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
	zassert_equal(cb_result, 0, "The result is %d", cb_result);
	zassert_equal_ptr(cb_op, &op, "The callback got another operation");
	zassert_not_equal(cb_thread, k_current_get(), "The callback ran in the sender's thread");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);

	zassert_equal(bt_hci_cmd_send_cb(&op, NULL, NULL), -EINVAL,
		      "A missing callback was accepted");
}

/* The return parameters are copied to the caller. What does not fit is cut off
 * and reported, with the status that the controller gave.
 */
static ZTEST(cmd_op, test_response_storage)
{
	NET_BUF_SIMPLE_DEFINE(rsp, 16);
	NET_BUF_SIMPLE_DEFINE(small, 4);

	test_cmd.rp_len = 8U;

	zassert_ok(bt_hci_cmd_send_async(&op, &rsp, &fut), "Sending the operation failed");
	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(fut.result, 0, "The result is %d", fut.result);
	zassert_equal(rsp.len, 9U, "%u bytes of return parameters", rsp.len);
	zassert_equal(bt_hci_cmd_op_rsp_len(&op), 9U, "Wrong length reported");
	zassert_equal(rsp.data[0], BT_HCI_ERR_SUCCESS, "The status does not come first");
	zassert_equal(rsp.data[8], 7U, "Wrong last parameter byte");

	test_cmd.status = BT_HCI_ERR_INVALID_PARAM;
	zassert_ok(bt_hci_cmd_send_async(&op, &small, &fut), "Sending the operation failed");
	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(fut.result, -EMSGSIZE, "The result is %d", fut.result);
	zassert_equal(small.len, 4U, "%u bytes of return parameters", small.len);
	zassert_equal(small.data[3], 2U, "Wrong last parameter byte");
	zassert_equal(bt_hci_cmd_op_rsp_len(&op), 9U, "Wrong length reported");
	zassert_equal(bt_hci_cmd_op_status(&op), BT_HCI_ERR_INVALID_PARAM,
		      "The status of the controller was lost");
}

/* Results that nobody picks up hold no command buffer: more commands than
 * there are buffers complete, and a synchronous command still goes through.
 */
static ZTEST(cmd_op, test_results_not_picked_up)
{
	NET_BUF_SIMPLE_DEFINE(rsp, 8);
	unsigned int buffers = hold_buffers();

	release_buffers();
	test_cmd.rp_len = 4U;

	for (unsigned int i = 0U; i < buffers + 1U; i++) {
		zassert_ok(bt_hci_cmd_send_async(&op, &rsp, &fut), "Sending round %u failed", i);
		k_sleep(K_MSEC(1));
		zassert_true(bt_future_is_done(&fut), "Operation %u did not complete", i);
		zassert_equal(rsp.len, 5U, "Operation %u has %u bytes", i, rsp.len);
	}

	zassert_equal(hold_buffers(), buffers, "Completed operations hold command buffers");
	release_buffers();

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "A synchronous command failed");
}

/* A blocked callback holds up the callbacks behind it, but not the commands */
static ZTEST(cmd_op, test_callback_blocked)
{
	static struct bt_hci_cmd_op second;
	unsigned int buffers;

	cb_block = true;
	zassert_ok(bt_hci_cmd_send_cb(&op, NULL, op_cb), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");

	bt_hci_cmd_op_init(&second, TEST_OPCODE, NULL);
	zassert_ok(bt_hci_cmd_send_cb(&second, NULL, op_cb), "Sending the operation failed");

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
	static struct bt_future other;

	test_cmd.delay = K_MSEC(100);
	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
	zassert_equal(bt_hci_cmd_send_async(&op, NULL, &other), -EBUSY,
		      "A pending operation was accepted again");
	zassert_equal(bt_hci_cmd_send_cb(&op, NULL, op_cb), -EBUSY,
		      "A pending operation was accepted again");

	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(fut.result, 0, "The result is %d", fut.result);
	zassert_equal(cb_count, 0U, "The refused send got a callback");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
}

/* An operation that its encoder refuses never reaches the controller */
static ZTEST(cmd_op, test_encoder_refuses)
{
	encode_refusal = -ESTALE;
	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(fut.result, -ECANCELED, "The result is %d", fut.result);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command buffer of the refused operation was not released");
}

/* A cancel in each state of an operation */
static ZTEST(cmd_op, test_cancel)
{
	static struct bt_hci_cmd_op outstanding;
	static struct bt_future outstanding_fut;
	unsigned int all;

	zassert_equal(bt_hci_cmd_op_cancel(&op), -EALREADY, "An idle operation was canceled");

	/* Queued: no command buffer to be had */
	all = hold_buffers();
	zassert_ok(bt_hci_cmd_send_cb(&op, NULL, op_cb), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(encode_count, 0U, "The operation got a buffer");
	zassert_ok(bt_hci_cmd_op_cancel(&op), "Canceling a queued operation failed");
	zassert_false(bt_hci_cmd_op_is_pending(&op), "The canceled operation is pending");
	release_buffers();
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 0U, "A canceled command reached the controller");
	zassert_equal(cb_count, 0U, "A canceled operation got its callback");

	/* Encoded, but the controller takes no command */
	bt_hci_cmd_op_init(&outstanding, TEST_OPCODE, NULL);
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&outstanding, NULL, &outstanding_fut),
		   "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(encode_count, 1U, "The operation got no buffer");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* With the controller */
	zassert_equal(bt_hci_cmd_op_cancel(&outstanding), -EINPROGRESS,
		      "A command that is with the controller was canceled");

	zassert_ok(bt_hci_cmd_op_cancel(&op), "Canceling an encoded operation failed");
	zassert_true(bt_future_is_done(&fut), "The future of a canceled operation is unresolved");
	zassert_equal(fut.result, -ECANCELED, "The result is %d", fut.result);

	test_cmd.delay = K_NO_WAIT;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&outstanding_fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(outstanding_fut.result, 0, "The result is %d", outstanding_fut.result);
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "A canceled command reached the controller");

	/* The buffer of the canceled operation is free again, and so is the credit */
	zassert_equal(hold_buffers(), all, "A command buffer is missing");
	release_buffers();
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command credit is missing");
}

/* An operation is either canceled, and then nothing of it is seen again, or
 * it completes. Never both, never neither.
 */
static ZTEST(cmd_op, test_cancel_or_complete)
{
	unsigned int canceled = 0U;
	unsigned int completed = 0U;

	for (unsigned int i = 0U; i < 200U; i++) {
		unsigned int before_count = test_cmd.count;
		int err;

		zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");

		/* Let the TX processor get further every few rounds */
		if ((i % 4U) != 0U) {
			k_yield();
		}

		err = bt_hci_cmd_op_cancel(&op);
		zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");

		if (err == 0) {
			canceled++;
			zassert_equal(fut.result, -ECANCELED, "Round %u: result %d", i, fut.result);
			k_sleep(K_MSEC(1));
			zassert_equal(test_cmd.count, before_count,
				      "Round %u: a canceled command reached the controller", i);
		} else {
			completed++;
			zassert_equal(fut.result, 0, "Round %u: result %d", i, fut.result);
			zassert_equal(test_cmd.count, before_count + 1U,
				      "Round %u: the controller got no command", i);
		}
	}

	TC_PRINT("%u canceled, %u completed\n", canceled, completed);
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
	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");

	zassert_ok(bt_future_wait(&fut, K_MSEC(DEADLINE_MS + MSEC_PER_SEC)),
		   "The future was not resolved");
	waited = k_uptime_get() - start;
	zassert_equal(fut.result, -ENOBUFS, "The result is %d", fut.result);
	zassert_within(waited, DEADLINE_MS, 100, "The operation took %lld ms", waited);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");

	/* The next one is accepted, and waits like the first */
	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut),
		   "The Host gave up on its controller over a missing buffer");
	zassert_ok(bt_hci_cmd_op_cancel(&op), "Canceling the queued operation failed");

	release_buffers();
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	check_pool();
}

/* The Host keeps to one command at a time. A controller that grants a credit
 * while a command is unanswered gets the next command only after it has
 * answered, so that the answer cannot be taken for that of the next one.
 */
static ZTEST(cmd_op, test_credit_while_command_outstanding)
{
	static struct bt_hci_cmd_op second;
	static struct bt_future second_fut;

	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* Another command with the same opcode, and room for it */
	test_credit_grant();
	bt_hci_cmd_op_init(&second, TEST_OPCODE, NULL);
	zassert_ok(bt_hci_cmd_send_async(&second, NULL, &second_fut),
		   "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "A command was sent while another was unanswered");

	/* The answer is that of the first command, and lets the second go */
	test_cmd.status = BT_HCI_ERR_CMD_DISALLOWED;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(fut.result, BT_HCI_ERR_CMD_DISALLOWED, "The result is %d", fut.result);
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
	zassert_false(bt_future_is_done(&second_fut), "The second command was answered early");

	test_cmd.status = BT_HCI_ERR_SUCCESS;
	test_cmd_respond(&test_cmd);
	zassert_ok(bt_future_wait(&second_fut, K_SECONDS(1)), "The future was not resolved");
	zassert_equal(second_fut.result, 0, "The result is %d", second_fut.result);

	/* The credits of both answers have not added up to more than one */
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

	/* The controller answers, and takes no further command for now */
	test_cmd.ncmd = 0U;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command failed");

	zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(encode_count, 1U, "The operation got no buffer");
	zassert_equal(test_cmd.count, 1U, "A command was sent without a credit");

	zassert_ok(bt_hci_cmd_op_cancel(&op), "Canceling the encoded operation failed");
	zassert_equal(fut.result, -ECANCELED, "The result is %d", fut.result);
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
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	uint8_t len;
};

static int sized_encode(struct net_buf *buf, struct bt_hci_cmd_op *encoded)
{
	const struct sized_op *sized = CONTAINER_OF(encoded, struct sized_op, op);

	(void)net_buf_add(buf, sized->len);

	return 0;
}

static ZTEST(cmd_op, test_order)
{
	static struct sized_op sized[3];

	test_cmd.delay = K_MSEC(5);

	ARRAY_FOR_EACH(sized, i) {
		sized[i].len = (uint8_t)(i + 1U);
		bt_hci_cmd_op_init(&sized[i].op, TEST_OPCODE, sized_encode);
		zassert_ok(bt_hci_cmd_send_async(&sized[i].op, NULL, &sized[i].fut),
			   "Sending operation %zu failed", i);
	}

	ARRAY_FOR_EACH(sized, i) {
		zassert_ok(bt_future_wait(&sized[i].fut, K_SECONDS(1)),
			   "Operation %zu did not complete", i);
		zassert_equal(sized[i].fut.result, 0, "Operation %zu gave %d", i,
			      sized[i].fut.result);
		zassert_equal(test_cmd_plen[i], sized[i].len, "Command %zu had %u parameter bytes",
			      i, test_cmd_plen[i]);
	}
}

/* Once its callback is invoked, an operation is not touched by the engine any
 * more: the callback may release the memory that it is in.
 */
static void release_cb(struct bt_hci_cmd_op *completed, int result)
{
	ARG_UNUSED(result);

	/* What the next owner of the memory might do with it */
	(void)memset(completed, 0xa5, sizeof(*completed));
	k_sem_give(&cb_sem);
}

static ZTEST(cmd_op, test_callback_releases_operation)
{
	static struct bt_hci_cmd_op released;
	const uint8_t *bytes = (const uint8_t *)&released;

	bt_hci_cmd_op_init(&released, TEST_OPCODE, NULL);
	zassert_ok(bt_hci_cmd_send_cb(&released, NULL, release_cb), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");

	/* Whatever ran the callback has returned from it by now */
	k_sleep(K_MSEC(10));

	for (size_t i = 0U; i < sizeof(released); i++) {
		zassert_equal(bytes[i], 0xa5, "Byte %zu of the operation was written", i);
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

	syswq_send_err = bt_hci_cmd_send_async(&op, NULL, &fut);
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

	zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The operation did not complete");
	zassert_ok(syswq_send_err, "Sending the operation failed");
	zassert_ok(syswq_sync_err, "The synchronous command failed");
	zassert_equal(fut.result, 0, "The result is %d", fut.result);
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

/* When the controller stops answering, every operation is handed back once,
 * whatever state it is in: with the controller, encoded, queued, or waiting
 * for its callback to run.
 */
static ZTEST(cmd_op, test_deadline_hands_back_all)
{
	static struct bt_hci_cmd_op ops[3];
	static struct bt_future futs[3];
	static struct bt_hci_cmd_op done;

	/* One whose callback cannot run yet, behind a blocked one */
	cb_block = true;
	zassert_ok(bt_hci_cmd_send_cb(&op, NULL, op_cb), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");
	bt_hci_cmd_op_init(&done, TEST_OPCODE, NULL);
	zassert_ok(bt_hci_cmd_send_cb(&done, NULL, op_cb), "Sending the operation failed");
	k_sleep(K_MSEC(10));

	/* One with the controller, one encoded behind it, one without a buffer */
	test_cmd.delay = K_FOREVER;
	ARRAY_FOR_EACH(ops, i) {
		bt_hci_cmd_op_init(&ops[i], TEST_OPCODE, NULL);
	}

	zassert_ok(bt_hci_cmd_send_async(&ops[0], NULL, &futs[0]), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&ops[1], NULL, &futs[1]), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	hold_buffers();
	zassert_ok(bt_hci_cmd_send_async(&ops[2], NULL, &futs[2]), "Sending the operation failed");

	zassert_ok(bt_future_wait(&futs[0], K_MSEC(DEADLINE_MS + MSEC_PER_SEC)),
		   "The future was not resolved");
	zassert_equal(futs[0].result, -ETIMEDOUT, "The sent one gave %d", futs[0].result);
	zassert_true(bt_future_is_done(&futs[1]), "The encoded one was not handed back");
	zassert_equal(futs[1].result, -EHOSTDOWN, "The encoded one gave %d", futs[1].result);
	zassert_true(bt_future_is_done(&futs[2]), "The queued one was not handed back");
	zassert_equal(futs[2].result, -EHOSTDOWN, "The queued one gave %d", futs[2].result);
	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	/* The one that had completed keeps its result */
	zassert_equal(cb_count, 1U, "The callback ran %u times", cb_count);
	k_sem_give(&cb_gate);
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The pending callback did not run");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
	zassert_equal(cb_result, 0, "The completed one gave %d", cb_result);

	ARRAY_FOR_EACH(ops, i) {
		zassert_false(bt_hci_cmd_op_is_pending(&ops[i]), "Operation %zu is pending", i);
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
		zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending the operation failed");
		k_sleep(K_MSEC(10));
		zassert_equal(test_cmd.count, 0U, "A command was sent without a buffer");

		buf = held[0];
		held[0] = NULL;
		net_buf_unref(buf);

		zassert_ok(bt_future_wait(&fut, K_SECONDS(1)), "The operation did not complete");
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
		zassert_ok(bt_hci_cmd_send_async(&op, NULL, &fut), "Sending round %u failed", i);
		zassert_ok(bt_future_wait(&fut, K_SECONDS(5)), "Operation %u did not complete", i);
		zassert_equal(fut.result, 0, "Operation %u gave %d", i, fut.result);
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
 * bt_disable() would not get past a command that is unanswered.
 */
ZTEST_SUITE(cmd_op_no_reset, test_without_reset, NULL, before, after, NULL);

static ZTEST(cmd_op_no_reset, test_disable_hands_back_all)
{
	static struct bt_hci_cmd_op ops[3];
	static struct bt_future futs[3];
	static struct bt_hci_cmd_op done;

	/* One whose callback cannot run yet, behind a blocked one */
	cb_block = true;
	zassert_ok(bt_hci_cmd_send_cb(&op, NULL, op_cb), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, K_SECONDS(1)), "The callback did not run");
	bt_hci_cmd_op_init(&done, TEST_OPCODE, NULL);
	zassert_ok(bt_hci_cmd_send_cb(&done, NULL, op_cb), "Sending the operation failed");
	k_sleep(K_MSEC(10));

	/* One with the controller, one encoded behind it, one without a buffer */
	test_cmd.delay = K_FOREVER;
	ARRAY_FOR_EACH(ops, i) {
		bt_hci_cmd_op_init(&ops[i], TEST_OPCODE, NULL);
	}

	zassert_ok(bt_hci_cmd_send_async(&ops[0], NULL, &futs[0]), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&ops[1], NULL, &futs[1]), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	hold_buffers();
	zassert_ok(bt_hci_cmd_send_async(&ops[2], NULL, &futs[2]), "Sending the operation failed");
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	ARRAY_FOR_EACH(ops, i) {
		zassert_true(bt_future_is_done(&futs[i]), "Operation %zu was not handed back", i);
		zassert_equal(futs[i].result, -EHOSTDOWN, "Operation %zu gave %d", i,
			      futs[i].result);
		zassert_false(bt_hci_cmd_op_is_pending(&ops[i]), "Operation %zu is pending", i);
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
