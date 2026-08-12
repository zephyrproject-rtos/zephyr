/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/random/random.h>

#include "net.h"
#include "access.h"
#include "delayable_msg.h"

#define SRC_ADDR 0x0002
#define RX_ADDR  0xc000

static void start_cb(uint16_t duration, int err, void *cb_data);

struct bt_mesh_net bt_mesh;

static struct bt_mesh_msg_ctx gctx = {.net_idx = 0,
				      .app_idx = 0,
				      .addr = 0,
				      .recv_dst = RX_ADDR,
				      .uuid = NULL,
				      .recv_rssi = 0,
				      .recv_ttl = 0x05,
				      .send_rel = false,
				      .rnd_delay = true,
				      .send_ttl = 0x06};

static struct bt_mesh_send_cb send_cb = {
	.start = start_cb,
};

static bool is_fake_random;
static bool check_expectations;
static bool accum_mask;
static bool do_not_call_cb;
static bool yield_in_send;
static uint16_t fake_random;
static uint8_t *buf_data;
static size_t buf_data_size;
static uint16_t id_mask;
static int cb_err_status;
static atomic_t send_call_count;

K_SEM_DEFINE(delayed_msg_sent, 0, 1);

/**** Mocked functions ****/
int bt_mesh_access_send(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf, uint16_t src_addr,
			const struct bt_mesh_send_cb *cb, void *cb_data)
{
	atomic_inc(&send_call_count);

	if (check_expectations) {
		gctx.rnd_delay = false;
		ztest_check_expected_data(ctx, sizeof(struct bt_mesh_msg_ctx));
		gctx.rnd_delay = true;
		ztest_check_expected_value(src_addr);
		ztest_check_expected_data(cb, sizeof(struct bt_mesh_send_cb));
		ztest_check_expected_data(cb_data, sizeof(uint32_t));
		zexpect_mem_equal(buf->data, buf_data, buf_data_size, "Buffer data corrupted");
	}

	if (cb && !do_not_call_cb) {
		cb->start(0x0, cb_err_status, cb_data);
	}

	/* Models the caller blocking inside the transport: the delayable msg has already been
	 * detached from busy_ctx at this point.
	 */
	if (yield_in_send) {
		k_yield();
	}

	return cb_err_status;
}

int bt_rand(void *buf, size_t len)
{
	if (is_fake_random) {
		*(uint16_t *)buf = fake_random;
	} else {
		sys_rand_get(buf, len);
	}
	return 0;
}
/**** Mocked functions ****/

static void start_cb(uint16_t duration, int err, void *cb_data)
{

	zassert_equal(err, cb_err_status, "err: %d, cb_err_status: %d", err, cb_err_status);

	if (accum_mask) {
		id_mask |= 1 << *(uint16_t *)cb_data;
	} else {
		k_sem_give(&delayed_msg_sent);
	}
}

static void set_expectation(struct net_buf_simple *buf, uint32_t *buf_id)
{
	ztest_expect_data(bt_mesh_access_send, ctx, &gctx);
	ztest_expect_value(bt_mesh_access_send, src_addr, SRC_ADDR);
	ztest_expect_data(bt_mesh_access_send, cb, &send_cb);
	ztest_expect_data(bt_mesh_access_send, cb_data, buf_id);
	buf_data = buf->__buf;
	buf_data_size = buf->size;
	check_expectations = true;
}

static void tc_setup(void *fixture)
{
	is_fake_random = false;
	check_expectations = false;
	accum_mask = false;
	id_mask = 0;
	do_not_call_cb = false;
	yield_in_send = false;
	cb_err_status = 0;
	atomic_set(&send_call_count, 0);
	k_sem_reset(&delayed_msg_sent);
	/* The module only queues messages for a provisioned, running node. */
	atomic_clear_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
	atomic_set_bit(bt_mesh.flags, BT_MESH_VALID);
	bt_mesh_delayable_msg_init();
}

static void tc_teardown(void *fixture)
{
	zassert_equal(gctx.net_idx, 0);
	zassert_equal(gctx.app_idx, 0);
	zassert_equal(gctx.addr, 0);
	zassert_equal(gctx.recv_dst, RX_ADDR);
	zassert_is_null(gctx.uuid);
	zassert_equal(gctx.recv_rssi, 0);
	zassert_equal(gctx.recv_ttl, 0x05);
	zassert_false(gctx.send_rel);
	zassert_true(gctx.rnd_delay);
	zassert_equal(gctx.send_ttl, 0x06);
}

ZTEST_SUITE(bt_mesh_delayable_msg, NULL, NULL, tc_setup, tc_teardown, NULL);

/* Simple single message sending with full size.
 */
ZTEST(bt_mesh_delayable_msg, test_single_sending)
{
	uint32_t buf_id = 0x55aa55aa;
	uint8_t *payload;

	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_TX_SDU_MAX);

	payload = net_buf_simple_add(&buf, BT_MESH_TX_SDU_MAX);
	for (int i = 0; i < BT_MESH_TX_SDU_MAX; i++) {
		payload[i] = i;
	}

	set_expectation(&buf, &buf_id);
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id));

	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been sent.");
}

/* The test checks that the delayed message mechanism sorts
 * the incoming messages according to the transmission start timestamp.
 */
ZTEST(bt_mesh_delayable_msg, test_self_sorting)
{
	uint8_t tx_data[20];
	uint32_t buf1_id = 1;
	uint32_t buf2_id = 2;
	uint32_t buf3_id = 3;
	uint32_t buf4_id = 4;

	NET_BUF_SIMPLE_DEFINE(buf1, 20);
	NET_BUF_SIMPLE_DEFINE(buf2, 20);
	NET_BUF_SIMPLE_DEFINE(buf3, 20);
	NET_BUF_SIMPLE_DEFINE(buf4, 20);

	memset(tx_data, 1, 20);
	net_buf_simple_add_mem(&buf1, tx_data, 20);
	memset(tx_data, 2, 20);
	net_buf_simple_add_mem(&buf2, tx_data, 20);
	memset(tx_data, 3, 20);
	net_buf_simple_add_mem(&buf3, tx_data, 20);
	memset(tx_data, 4, 20);
	net_buf_simple_add_mem(&buf4, tx_data, 20);

	is_fake_random = true;
	fake_random = 30;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf1_id));
	fake_random = 10;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf2, SRC_ADDR, &send_cb, &buf2_id));
	fake_random = 20;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf3, SRC_ADDR, &send_cb, &buf3_id));
	fake_random = 40;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf4, SRC_ADDR, &send_cb, &buf4_id));

	set_expectation(&buf2, &buf2_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
	set_expectation(&buf3, &buf3_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
	set_expectation(&buf1, &buf1_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
	set_expectation(&buf4, &buf4_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
}

/* The test checks that the delayed msg mechanism can allocate new context
 * if all contexts are in use by sending the message, that is the closest to
 * the tx time.
 */
ZTEST(bt_mesh_delayable_msg, test_ctx_reallocation)
{
	uint8_t tx_data[20];
	uint32_t buf_id1 = 0;
	uint32_t buf_id2 = 1;
	uint32_t buf_id3 = 2;
	uint32_t buf_id4 = 3;
	uint32_t buf_id5 = 4;

	NET_BUF_SIMPLE_DEFINE(buf, 20);

	memset(tx_data, 1, 20);
	net_buf_simple_add_mem(&buf, tx_data, 20);

	accum_mask = true;
	is_fake_random = true;
	fake_random = 10;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id1));
	fake_random = 30;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id2));
	fake_random = 20;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id3));
	fake_random = 40;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id4));
	fake_random = 40;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id5));
	zassert_equal(id_mask, 0x0001, "Delayed message context reallocation was broken");
	k_sleep(K_MSEC(500));
	zassert_equal(id_mask, 0x001F);
}

/* The test checks that the delayed msg mechanism can allocate new chunks
 * if all chunks are in use by sending the other messages.
 */
ZTEST(bt_mesh_delayable_msg, test_chunk_reallocation)
{
	uint8_t tx_data[BT_MESH_TX_SDU_MAX];
	uint32_t buf_id1 = 0;
	uint32_t buf_id2 = 1;
	uint32_t buf_id3 = 2;
	uint32_t buf_id4 = 3;

	NET_BUF_SIMPLE_DEFINE(buf1, 20);
	NET_BUF_SIMPLE_DEFINE(buf2, BT_MESH_TX_SDU_MAX);

	memset(tx_data, 1, BT_MESH_TX_SDU_MAX);
	net_buf_simple_add_mem(&buf2, tx_data, BT_MESH_TX_SDU_MAX);

	accum_mask = true;
	net_buf_simple_add_mem(&buf1, tx_data, 20);
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id1));
	net_buf_simple_reset(&buf1);
	net_buf_simple_add_mem(&buf1, tx_data, 20);
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id2));
	net_buf_simple_reset(&buf1);
	net_buf_simple_add_mem(&buf1, tx_data, 20);
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id3));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf2, SRC_ADDR, &send_cb, &buf_id4));
	zassert_equal(id_mask, 0x0007, "Delayed message chunks reallocation was broken");
	k_sleep(K_MSEC(500));
	zassert_equal(id_mask, 0x000F);
}

/* The test checks that the delayed msg mechanism can reschedule access messages
 * transport layes doesn't have enough memory or buffers at the moment.
 * Also it checks that the delayed msg mechanism can handle the other transport
 * layer errors without rescheduling appropriate access messages.
 */
ZTEST(bt_mesh_delayable_msg, test_cb_error_status)
{
	uint32_t buf_id = 0x55aa55aa;
	uint8_t tx_data[BT_MESH_TX_SDU_MAX];

	NET_BUF_SIMPLE_DEFINE(buf1, 20);
	NET_BUF_SIMPLE_DEFINE(buf2, 20);
	NET_BUF_SIMPLE_DEFINE(buf3, 20);

	memset(tx_data, 1, BT_MESH_TX_SDU_MAX);
	net_buf_simple_add_mem(&buf1, tx_data, 20);
	net_buf_simple_add_mem(&buf2, tx_data, 20);
	net_buf_simple_add_mem(&buf3, tx_data, 20);

	cb_err_status = -ENOBUFS;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id));
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been handled.");
	cb_err_status = 0;
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been sent.");

	cb_err_status = -EBUSY;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf2, SRC_ADDR, &send_cb, &buf_id));
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been handled.");
	cb_err_status = 0;
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been sent.");

	cb_err_status = -EINVAL;
	do_not_call_cb = true;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf3, SRC_ADDR, &send_cb, &buf_id));
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been handled.");
	cb_err_status = 0;
	zexpect_not_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		       "Delayed message has not been handled.");
}

/* The test checks that the delayed msg mechanism raises
 * the model message callback with the appropriate error code after
 * stopping the functionality.
 */
ZTEST(bt_mesh_delayable_msg, test_stop_handler)
{
	uint8_t tx_data[20];
	uint32_t buf_id1 = 0;
	uint32_t buf_id2 = 1;
	uint32_t buf_id3 = 2;
	uint32_t buf_id4 = 3;

	NET_BUF_SIMPLE_DEFINE(buf, 20);

	memset(tx_data, 1, 20);
	net_buf_simple_add_mem(&buf, tx_data, 20);

	accum_mask = true;
	cb_err_status = -ENODEV;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id1));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id2));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id3));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id4));
	bt_mesh_delayable_msg_stop();
	/* Long enough for any timer that survived the stop to fire. */
	k_sleep(K_SECONDS(1));
	zassert_equal(atomic_get(&send_call_count), 0,
		      "Delayed message has been sent after stopping.");
	zassert_equal(id_mask, 0x000F, "Not all scheduled messages were handled after stopping");
}

/* The test checks that chunk allocation is based on actual data length (buf->len)
 * and not on buffer capacity (buf->size). Four messages with small payloads in
 * large-capacity buffers should consume exactly the full chunk pool without error
 * or eviction.
 *
 * With CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE=20,
 * CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT=20 and data_len=100:
 *   DIV_ROUND_UP(100, 20) = 5 chunks per msg, 4 msgs * 5 = 20 chunks (full pool)
 */
ZTEST(bt_mesh_delayable_msg, test_chunk_alloc_by_data_len)
{
	uint8_t tx_data[100];
	uint32_t buf_id1 = 0;
	uint32_t buf_id2 = 1;
	uint32_t buf_id3 = 2;
	uint32_t buf_id4 = 3;

	NET_BUF_SIMPLE_DEFINE(buf1, BT_MESH_TX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(buf2, BT_MESH_TX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(buf3, BT_MESH_TX_SDU_MAX);
	NET_BUF_SIMPLE_DEFINE(buf4, BT_MESH_TX_SDU_MAX);

	memset(tx_data, 1, sizeof(tx_data));
	net_buf_simple_add_mem(&buf1, tx_data, sizeof(tx_data));
	net_buf_simple_add_mem(&buf2, tx_data, sizeof(tx_data));
	net_buf_simple_add_mem(&buf3, tx_data, sizeof(tx_data));
	net_buf_simple_add_mem(&buf4, tx_data, sizeof(tx_data));

	accum_mask = true;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id1));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf2, SRC_ADDR, &send_cb, &buf_id2));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf3, SRC_ADDR, &send_cb, &buf_id3));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf4, SRC_ADDR, &send_cb, &buf_id4));

	/* All 4 messages should coexist using the full chunk pool (5 chunks each)
	 * without any eviction.
	 */
	zassert_equal(id_mask, 0,
		      "Messages were evicted unexpectedly (id_mask: 0x%04x)", id_mask);

	k_sleep(K_MSEC(500));
	zassert_equal(id_mask, 0x000F, "Not all messages were sent (id_mask: 0x%04x)", id_mask);
}

#define STRESS_ITER    40
#define STRESS_BUF_LEN 20

/* Cooperative priority for the main test thread and the helper: strictly higher (more negative)
 * than the sysworkq default of -1, mirroring the production model where the enqueuing contexts
 * (BT RX WQ) run cooperatively above the sysworkq that runs delayable_msg_handler().
 */
#define STRESS_COOP_PRIO K_PRIO_COOP(6)

K_THREAD_STACK_DEFINE(stress_helper_stack, 4096);
static struct k_thread stress_helper_thread;

/* Per-msg callback counters. Each manage() call passes a pointer to one entry as cb_data, and a
 * correct implementation increments every scheduled entry exactly once. Counting per entry catches
 * both a msg sent twice and a msg silently dropped, which the aggregate counters can hide.
 */
static atomic_t stress_main_cb_count[STRESS_ITER];
static atomic_t stress_helper_cb_count[STRESS_ITER];

/* Aggregate counts kept for a quick sanity check and diagnostic output. */
static atomic_t stress_scheduled;
static atomic_t stress_cb_count;

/* cb_data pointers passed to manage(). Only the slots that were actually scheduled fire. */
static atomic_t *stress_main_slot[STRESS_ITER];
static atomic_t *stress_helper_slot[STRESS_ITER];

static void stress_cb_start(uint16_t duration, int err, void *cb_data)
{
	ARG_UNUSED(duration);
	ARG_UNUSED(err);

	atomic_inc(&stress_cb_count);
	/* cb_data points directly at the per-slot counter for this msg. */
	atomic_inc((atomic_t *)cb_data);
	/* The mock calls this synchronously from inside bt_mesh_access_send(), that is, while
	 * push_msg_from_delayable_msgs() is still working on the detached msg. Yielding here hands
	 * the CPU to the other cooperative enqueuer at exactly that point.
	 */
	k_yield();
}

static const struct bt_mesh_send_cb stress_send_cb = {
	.start = stress_cb_start,
};

static void stress_helper_fn(void *a, void *b, void *c)
{
	uint8_t data[STRESS_BUF_LEN];

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	memset(data, 0x5A, sizeof(data));

	for (int i = 0; i < STRESS_ITER; i++) {
		NET_BUF_SIMPLE_DEFINE(buf, STRESS_BUF_LEN);
		int err;

		net_buf_simple_add_mem(&buf, data, STRESS_BUF_LEN);
		stress_helper_slot[i] = &stress_helper_cb_count[i];
		err = bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &stress_send_cb,
						   stress_helper_slot[i]);
		if (err == 0) {
			atomic_inc(&stress_scheduled);
		} else {
			/* Not queued: mark this slot as "not expected". */
			stress_helper_slot[i] = NULL;
		}
		/* Cooperative round-robin: hand the CPU to the other same-priority enqueuer.
		 * Neither thread can be preempted, so the interleaving points are exactly the
		 * yield sites.
		 */
		k_yield();
	}
}

/* Regression test for unsynchronized cross-thread access to the delayable msg lists.
 *
 * Two cooperative threads at the same priority (the ZTEST main thread, temporarily raised, and a
 * helper) call bt_mesh_delayable_msg_manage() alternately via k_yield(), and the mocked send yields
 * as well, so a second enqueuer enters push_msg_from_delayable_msgs() while the first is still
 * working on the head msg. The sysworkq handler drains the queue during the final k_sleep().
 *
 * Callbacks are counted per cb_data slot: a msg sent twice and a msg silently dropped cancel each
 * other out in the aggregate counters, but not per slot.
 */
ZTEST(bt_mesh_delayable_msg, test_concurrent_manage)
{
	uint8_t data[STRESS_BUF_LEN];
	atomic_t final_cb_slot = ATOMIC_INIT(0);
	int orig_prio;
	int join_result;
	int post_manage_result;
	int scheduled_snapshot;
	int completed_snapshot;
	int per_slot_failures = 0;
	int final_completed;

	memset(data, 0xA5, sizeof(data));

	atomic_set(&stress_scheduled, 0);
	atomic_set(&stress_cb_count, 0);
	for (int i = 0; i < STRESS_ITER; i++) {
		atomic_set(&stress_main_cb_count[i], 0);
		atomic_set(&stress_helper_cb_count[i], 0);
		stress_main_slot[i] = NULL;
		stress_helper_slot[i] = NULL;
	}

	/* This thread and the helper run at the same cooperative priority, above the sysworkq, so
	 * neither can be preempted and every scheduling transition happens at a k_yield()/k_sleep.
	 */
	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), STRESS_COOP_PRIO);

	k_thread_create(&stress_helper_thread, stress_helper_stack,
			K_THREAD_STACK_SIZEOF(stress_helper_stack), stress_helper_fn, NULL,
			NULL, NULL, STRESS_COOP_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&stress_helper_thread, "stress_helper");

	for (int i = 0; i < STRESS_ITER; i++) {
		NET_BUF_SIMPLE_DEFINE(buf, STRESS_BUF_LEN);
		int err;

		net_buf_simple_add_mem(&buf, data, STRESS_BUF_LEN);
		stress_main_slot[i] = &stress_main_cb_count[i];
		err = bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &stress_send_cb,
						   stress_main_slot[i]);
		if (err == 0) {
			atomic_inc(&stress_scheduled);
		} else {
			stress_main_slot[i] = NULL;
		}
		k_yield();
	}

	/* Capture the join result before touching the priority or asserting, as zassert may
	 * long-jump out of the test.
	 */
	join_result = k_thread_join(&stress_helper_thread, K_SECONDS(5));

	/* Drain: main blocks, so the sysworkq becomes the only runnable cooperative thread and
	 * fires the handler until busy_ctx is empty. The random delay is up to ~500 ms per msg for
	 * group addresses, so give a generous margin.
	 */
	k_sleep(K_SECONDS(2));

	scheduled_snapshot = atomic_get(&stress_scheduled);
	completed_snapshot = atomic_get(&stress_cb_count);

	/* Every scheduled slot must have received exactly one callback, every unscheduled slot
	 * none.
	 */
	for (int i = 0; i < STRESS_ITER; i++) {
		int m = atomic_get(&stress_main_cb_count[i]);
		int h = atomic_get(&stress_helper_cb_count[i]);
		int m_expected = stress_main_slot[i] ? 1 : 0;
		int h_expected = stress_helper_slot[i] ? 1 : 0;

		if (m != m_expected || h != h_expected) {
			per_slot_failures++;
		}
	}

	/* Scheduling one more msg proves that the free and busy lists are consistent and that
	 * nothing was leaked out of both of them.
	 */
	NET_BUF_SIMPLE_DEFINE(final_buf, STRESS_BUF_LEN);

	net_buf_simple_add_mem(&final_buf, data, STRESS_BUF_LEN);
	post_manage_result = bt_mesh_delayable_msg_manage(&gctx, &final_buf, SRC_ADDR,
							  &stress_send_cb, &final_cb_slot);

	k_sleep(K_MSEC(700));
	final_completed = atomic_get(&stress_cb_count);

	/* Restore the priority before any zassert that could long-jump out of the test, so the
	 * remaining test cases run with the expected one.
	 */
	k_thread_priority_set(k_current_get(), orig_prio);

	zassert_ok(join_result, "Helper thread did not terminate (join=%d)", join_result);
	zassert_equal(per_slot_failures, 0,
		      "Race detected: %d slots received a wrong callback count "
		      "(exactly-once delivery contract violated). "
		      "scheduled=%d completed=%d",
		      per_slot_failures, scheduled_snapshot, completed_snapshot);
	zassert_equal(scheduled_snapshot, completed_snapshot,
		      "Race detected (aggregate): scheduled=%d completed=%d",
		      scheduled_snapshot, completed_snapshot);
	zassert_ok(post_manage_result,
		   "Post-stress manage() failed (%d): indicates leak or list corruption",
		   post_manage_result);
	zassert_equal(atomic_get(&final_cb_slot), 1,
		      "Post-stress callback did not fire exactly once (got %d)",
		      (int)atomic_get(&final_cb_slot));
	zassert_equal(final_completed, completed_snapshot + 1,
		      "Post-stress aggregate cb count off: final=%d snapshot=%d",
		      final_completed, completed_snapshot);
}

#define STOP_RACE_MSGS 4
#define STOP_RACE_LEN  20

struct stop_race_slot {
	atomic_t count;
	int err;
};

static struct stop_race_slot stop_race_slots[STOP_RACE_MSGS];
static struct stop_race_slot stop_race_extra_slot;

static void stop_race_cb_start(uint16_t duration, int err, void *cb_data)
{
	struct stop_race_slot *slot = cb_data;

	ARG_UNUSED(duration);

	slot->err = err;
	atomic_inc(&slot->count);
}

static const struct bt_mesh_send_cb stop_race_send_cb = {
	.start = stop_race_cb_start,
};

K_THREAD_STACK_DEFINE(stop_race_stack, 2048);
static struct k_thread stop_race_thread;

static void stop_race_stopper_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	/* Mirrors bt_mesh_suspend(): the flag is set before the queue is drained. */
	atomic_set_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
	bt_mesh_delayable_msg_stop();
}

/* Regression test for a message left in flight across bt_mesh_delayable_msg_stop().
 *
 * push_msg_from_delayable_msgs() detaches the head from busy_ctx before the blocking send, so a
 * concurrent stop() cannot see that context and cannot complete it. If the send then reports
 * -EBUSY or -ENOBUFS, the context must not be queued again on the stopped stack.
 *
 * The interleaving is forced deterministically: the ctx pool is filled, so the next manage() takes
 * the purge path and calls push_msg_from_delayable_msgs() itself; the mocked send yields, handing
 * the CPU to a same-priority cooperative thread that suspends and stops the module; the send then
 * reports -ENOBUFS, so the message is queued again behind the drain. It must not reach the
 * transport after that, and the message that manage() was building must not be accepted either.
 */
ZTEST(bt_mesh_delayable_msg, test_stop_during_in_flight_send)
{
	uint8_t tx_data[STOP_RACE_LEN];
	int orig_prio;
	int join_result;
	int manage_result;
	int sends_after_stop;

	NET_BUF_SIMPLE_DEFINE(buf, STOP_RACE_LEN);

	memset(tx_data, 1, sizeof(tx_data));

	for (int i = 0; i < STOP_RACE_MSGS; i++) {
		atomic_set(&stop_race_slots[i].count, 0);
		stop_race_slots[i].err = 0;
	}
	atomic_set(&stop_race_extra_slot.count, 0);
	stop_race_extra_slot.err = 0;

	/* Cooperative priority above the sysworkq, so every context switch happens at an explicit
	 * yield site.
	 */
	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), STRESS_COOP_PRIO);

	for (int i = 0; i < STOP_RACE_MSGS; i++) {
		net_buf_simple_reset(&buf);
		net_buf_simple_add_mem(&buf, tx_data, sizeof(tx_data));
		zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &stop_race_send_cb,
							&stop_race_slots[i]));
	}

	k_thread_create(&stop_race_thread, stop_race_stack,
			K_THREAD_STACK_SIZEOF(stop_race_stack), stop_race_stopper_fn, NULL, NULL,
			NULL, STRESS_COOP_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&stop_race_thread, "stop_race_stopper");

	/* The transport reports no buffers and does not raise the callback itself, exactly as it
	 * does while the advertiser is suspended.
	 */
	cb_err_status = -ENOBUFS;
	do_not_call_cb = true;
	yield_in_send = true;

	net_buf_simple_reset(&buf);
	net_buf_simple_add_mem(&buf, tx_data, sizeof(tx_data));
	manage_result = bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &stop_race_send_cb,
						     &stop_race_extra_slot);

	yield_in_send = false;
	do_not_call_cb = false;
	cb_err_status = 0;

	join_result = k_thread_join(&stop_race_thread, K_SECONDS(5));

	sends_after_stop = atomic_get(&send_call_count);
	/* Long enough for a resurrected message to fire (backoff is 10 ms). */
	k_sleep(K_SECONDS(1));

	k_thread_priority_set(k_current_get(), orig_prio);
	atomic_clear_bit(bt_mesh.flags, BT_MESH_SUSPENDED);

	zassert_ok(join_result, "Stopper thread did not terminate (join=%d)", join_result);
	zassert_equal(atomic_get(&send_call_count), sends_after_stop,
		      "Message was sent after stopping (%d sends, expected %d)",
		      (int)atomic_get(&send_call_count), sends_after_stop);
	/* -ENODEV when the queueing itself is refused, -ENOMEM when no context can be freed for it;
	 * either way a message built before the stop must not be accepted.
	 */
	zassert_not_ok(manage_result, "manage() accepted a message once stopped");
	zassert_equal(atomic_get(&stop_race_extra_slot.count), 0,
		      "Refused message must not raise the callback");

	for (int i = 0; i < STOP_RACE_MSGS; i++) {
		zassert_equal(atomic_get(&stop_race_slots[i].count), 1,
			      "Message %d got %d callbacks, expected exactly one", i,
			      (int)atomic_get(&stop_race_slots[i].count));
		zassert_equal(stop_race_slots[i].err, -ENODEV,
			      "Message %d completed with %d, expected -ENODEV", i,
			      stop_race_slots[i].err);
	}
}

#define STALE_BUF_LEN 20

/* Regression test for a stale work invocation sending the next message before it is due.
 *
 * k_work_reschedule() does not withdraw a submission that already happened, so the handler can run
 * for an expiry that another context has already consumed. Popping the head unconditionally then
 * transmits a message that is not due, truncating the random delay the Access layer transmission
 * rules require.
 *
 * The interleaving is built from a cooperative thread above the sysworkq priority: k_busy_wait()
 * lets the first deadline expire and its work be submitted without giving the sysworkq a chance to
 * run, and the purge path then consumes that same message, leaving the submission stale.
 */
ZTEST(bt_mesh_delayable_msg, test_stale_handler_no_early_send)
{
	/* bt_rand() is mocked, so these become delays of 20, 420, 430 and 440 ms. */
	static const uint16_t fake_delays[] = {0, 400, 410, 420};
	uint8_t tx_data[STALE_BUF_LEN];
	int orig_prio;
	int sends_after_purge;
	int sends_at_end;

	NET_BUF_SIMPLE_DEFINE(buf, STALE_BUF_LEN);

	memset(tx_data, 3, sizeof(tx_data));

	orig_prio = k_thread_priority_get(k_current_get());
	k_thread_priority_set(k_current_get(), STRESS_COOP_PRIO);
	is_fake_random = true;

	for (int i = 0; i < ARRAY_SIZE(fake_delays); i++) {
		fake_random = fake_delays[i];
		net_buf_simple_reset(&buf);
		net_buf_simple_add_mem(&buf, tx_data, sizeof(tx_data));
		zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, NULL, NULL));
	}

	/* Let the first deadline expire and its work be submitted. Busy-waiting keeps the CPU, so
	 * the cooperative sysworkq cannot run the handler yet.
	 */
	k_busy_wait(40 * USEC_PER_MSEC);

	/* The ctx pool is full, so this call purges the first message itself. The submission made
	 * on its behalf is now stale.
	 */
	fake_random = 440;
	net_buf_simple_reset(&buf);
	net_buf_simple_add_mem(&buf, tx_data, sizeof(tx_data));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, NULL, NULL));

	/* Hand the CPU over so the stale invocation runs while nothing is due. */
	k_sleep(K_MSEC(50));
	sends_after_purge = atomic_get(&send_call_count);

	k_sleep(K_MSEC(600));
	sends_at_end = atomic_get(&send_call_count);

	k_thread_priority_set(k_current_get(), orig_prio);
	is_fake_random = false;

	zassert_equal(sends_after_purge, 1, "Message sent before it was due (%d sends, expected 1)",
		      sends_after_purge);
	zassert_equal(sends_at_end, 5, "Not every message was sent (%d of 5)", sends_at_end);
}
