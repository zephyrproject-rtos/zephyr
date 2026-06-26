/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "lifo_usage.h"
#include <zephyr/kernel.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 2

struct k_lifo lifo, plifo;
static ldata_t lifo_data[LIST_LEN];
struct k_lifo timeout_order_lifo;

static struct k_thread tdata, tdata1;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack1, STACK_SIZE);

static struct k_sem start_sema, wait_sema;
void test_thread_pend_and_timeout(void *p1, void *p2, void *p3);

struct scratch_lifo_packet {
	void *link_in_lifo;
	void *data_if_needed;
};

struct reply_packet {
	void *link_in_lifo;
	int32_t reply;
};

struct timeout_order_data {
	void *link_in_lifo;
	struct k_lifo *klifo;
	int32_t timeout;
	int32_t timeout_order;
	int32_t q_order;
};

static struct k_lifo lifo_timeout[2];

struct timeout_order_data timeout_order_data[] = {
	{0, &lifo_timeout[0], 200, 2, 0},
	{0, &lifo_timeout[0], 400, 4, 1},
	{0, &lifo_timeout[0],   0, 0, 2},
	{0, &lifo_timeout[0], 100, 1, 3},
	{0, &lifo_timeout[0], 300, 3, 4},
};

struct timeout_order_data timeout_order_data_mult_lifo[] = {
	{0, &lifo_timeout[1],   0, 0, 0},
	{0, &lifo_timeout[0], 300, 3, 1},
	{0, &lifo_timeout[0], 500, 5, 2},
	{0, &lifo_timeout[1], 800, 8, 3},
	{0, &lifo_timeout[1], 700, 7, 4},
	{0, &lifo_timeout[0], 100, 1, 5},
	{0, &lifo_timeout[0], 600, 6, 6},
	{0, &lifo_timeout[0], 200, 2, 7},
	{0, &lifo_timeout[1], 400, 4, 8},
};

#define NUM_SCRATCH_LIFO_PACKETS 20
#define TIMEOUT_ORDER_NUM_THREADS	ARRAY_SIZE(timeout_order_data_mult_lifo)
#define TSTACK_SIZE			(1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIFO_THREAD_PRIO		-5

struct scratch_lifo_packet scratch_lifo_packets[NUM_SCRATCH_LIFO_PACKETS];

struct k_lifo scratch_lifo_packets_lifo;

static k_tid_t to_ord_tid[TIMEOUT_ORDER_NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(ttstack,
		TIMEOUT_ORDER_NUM_THREADS, TSTACK_SIZE);
static struct k_thread ttdata[TIMEOUT_ORDER_NUM_THREADS];

static void *get_scratch_packet(void)
{
	void *packet = k_lifo_get(&scratch_lifo_packets_lifo, K_NO_WAIT);

	zassert_true(packet != NULL);
	return packet;
}

static void put_scratch_packet(void *packet)
{
	k_lifo_put(&scratch_lifo_packets_lifo, packet);
}

static void thread_entry_nowait(void *p1, void *p2, void *p3)
{
	void *ret;

	ret = k_lifo_get((struct k_lifo *)p1, K_FOREVER);

	/* data pushed at last should be read first */
	zassert_equal(ret, (void *)&lifo_data[1]);

	ret = k_lifo_get((struct k_lifo *)p1, K_FOREVER);

	zassert_equal(ret, (void *)&lifo_data[0]);

	k_sem_give(&start_sema);
}

static bool is_timeout_in_range(uint32_t start_time, uint32_t timeout)
{
	uint32_t stop_time, diff;

	stop_time = k_cycle_get_32();
	diff = k_cyc_to_ms_floor32(stop_time - start_time);
	return timeout <= diff;
}

static int test_multiple_threads_pending(struct timeout_order_data *test_data,
					 int test_data_size)
{
	int ii;

	for (ii = 0; ii < test_data_size; ii++) {
		to_ord_tid[ii] = k_thread_create(&ttdata[ii], ttstack[ii], TSTACK_SIZE,
						 test_thread_pend_and_timeout,
						 &test_data[ii], NULL, NULL,
						 LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);
	}

	for (ii = 0; ii < test_data_size; ii++) {
		struct timeout_order_data *data =
			k_lifo_get(&timeout_order_lifo, K_FOREVER);

		if (data->timeout_order == ii) {
			TC_PRINT(" thread (q order: %d, t/o: %d, lifo %p)\n",
				 data->q_order, (int) data->timeout,
				 data->klifo);
		} else {
			zassert_equal(data->timeout_order, ii, " *** thread %d "
				      "woke up, expected %d\n",
				      data->timeout_order, ii);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

static void thread_entry_wait(void *p1, void *p2, void *p3)
{
	k_lifo_put((struct k_lifo *)p1, (void *)&lifo_data[0]);

	k_lifo_put((struct k_lifo *)p1, (void *)&lifo_data[1]);
	k_sem_give(&wait_sema);
}

/**
 * @brief LIFO kernel object tests
 * @defgroup tests_kernel_lifo LIFO tests
 * @ingroup all_tests
 * @{
 * @}
 */

/**
 * @addtogroup tests_kernel_lifo
 * @{
 */

/* Helper: get from lifo_timeout[0] with K_NO_WAIT and report whether data was
 * present back via timeout_order_lifo.
 */
static void test_thread_timeout_reply_values(void *p1, void *p2, void *p3)
{
	struct reply_packet *reply_packet = (struct reply_packet *)p1;

	reply_packet->reply =
		!!k_lifo_get(&lifo_timeout[0], K_NO_WAIT);

	k_lifo_put(&timeout_order_lifo, reply_packet);
}

/* Helper: get from lifo_timeout[0] with K_FOREVER and report the result back
 * via timeout_order_lifo.
 */
static void test_thread_timeout_reply_values_wfe(void *p1, void *p2, void *p3)
{
	struct reply_packet *reply_packet = (struct reply_packet *)p1;

	reply_packet->reply =
		!!k_lifo_get(&lifo_timeout[0], K_FOREVER);

	k_lifo_put(&timeout_order_lifo, reply_packet);
}

/* Helper: sleep for the given timeout then put a packet on the lifo. */
static void test_thread_put_timeout(void *p1, void *p2, void *p3)
{
	uint32_t timeout = *((uint32_t *)p2);

	k_msleep(timeout);
	k_lifo_put((struct k_lifo *)p1, get_scratch_packet());
}

/**
 * @brief Verify k_lifo_get() returns items in last-in first-out order.
 *
 * @details
 * The defining property of a LIFO is that the most recently put item is
 * retrieved first. Two items are placed on the LIFO around the creation of a
 * reader thread, which must receive them in reverse insertion order.
 *
 * Test steps:
 * - Put item 0, start a reader thread, then put item 1.
 * - The reader gets two items and verifies item 1 is returned before item 0.
 *
 * Expected result:
 * - Items are returned newest-first (item 1, then item 0).
 *
 * @see k_lifo_put()
 * @see k_lifo_get()
 */
ZTEST(lifo_usage, test_lifo_nowait)
{
	k_lifo_init(&lifo);

	k_sem_init(&start_sema, 0, 1);

	/* put some data on lifo */
	k_lifo_put(&lifo, (void *)&lifo_data[0]);

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_nowait, &lifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_lifo_put(&lifo, (void *)&lifo_data[1]);

	/* Allow another thread to read lifo */
	k_sem_take(&start_sema, K_FOREVER);
	k_thread_abort(tid);
}

/**
 * @brief Verify a thread blocked on an empty LIFO is woken by k_lifo_put().
 *
 * @details
 * A reader blocks on an empty LIFO with K_FOREVER. A second thread then puts two
 * items; the pending reader must be woken with the first item and retrieve the
 * second on its next get, confirming put correctly hands data to a waiter.
 *
 * Test steps:
 * - Start a thread that puts item 0 then item 1 on the LIFO.
 * - In the main thread, block in k_lifo_get(K_FOREVER) and verify it returns
 *   item 0.
 * - Get again and verify it returns item 1.
 *
 * Expected result:
 * - The pending reader wakes and receives both items in order.
 *
 * @see k_lifo_get()
 * @see k_lifo_put()
 */
ZTEST(lifo_usage_1cpu, test_lifo_wait)
{
	int *ret;

	k_lifo_init(&plifo);
	k_sem_init(&wait_sema, 0, 1);

	k_tid_t tid = k_thread_create(&tdata1, tstack1, STACK_SIZE,
				      thread_entry_wait, &plifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	ret = k_lifo_get(&plifo, K_FOREVER);

	zassert_equal(ret, (void *)&lifo_data[0]);

	k_sem_take(&wait_sema, K_FOREVER);

	ret = k_lifo_get(&plifo, K_FOREVER);

	zassert_equal(ret, (void *)&lifo_data[1]);

	k_thread_abort(tid);
}

/**
 * @brief Verify k_lifo_get() timeout behavior on an empty LIFO.
 *
 * @details
 * On an empty LIFO, k_lifo_get() with a finite timeout must block for at least
 * the requested duration and then return NULL, while K_NO_WAIT must return NULL
 * immediately. The elapsed time is measured to confirm the full timeout elapsed.
 *
 * Test steps:
 * - Call k_lifo_get() on an empty LIFO with a 100 ms timeout; measure the wait.
 * - Verify it returned NULL and that at least the timeout elapsed.
 * - Call k_lifo_get() with K_NO_WAIT and verify it returns NULL.
 *
 * Expected result:
 * - The timed get returns NULL after the timeout; K_NO_WAIT returns NULL at once.
 *
 * @see k_lifo_get()
 */
ZTEST(lifo_usage_1cpu, test_timeout_empty_lifo)
{
	void *packet;

	uint32_t start_time, timeout;

	timeout = 100U;

	start_time = k_cycle_get_32();

	packet = k_lifo_get(&lifo_timeout[0], K_MSEC(timeout));

	zassert_is_null(packet);

	zassert_true(is_timeout_in_range(start_time, timeout));

	/* Test empty lifo with timeout of K_NO_WAIT */
	packet = k_lifo_get(&lifo_timeout[0], K_NO_WAIT);
	zassert_is_null(packet);
}

/**
 * @brief Verify k_lifo_get() returns queued data without waiting.
 *
 * @details
 * When an item is already on the LIFO, k_lifo_get() must return it immediately
 * regardless of the timeout argument. The contract is checked with both
 * K_NO_WAIT and K_FOREVER, neither of which should block when data is present.
 *
 * Test steps:
 * - Put an item, then k_lifo_get() with K_NO_WAIT and verify a non-NULL return.
 * - Put an item, then k_lifo_get() with K_FOREVER and verify a non-NULL return.
 *
 * Expected result:
 * - Both gets return the queued item without blocking.
 *
 * @see k_lifo_get()
 * @see k_lifo_put()
 */
ZTEST(lifo_usage, test_timeout_non_empty_lifo)
{
	void *packet, *scratch_packet;

	/* Test k_lifo_get with K_NO_WAIT */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);
	packet = k_lifo_get(&lifo_timeout[0], K_NO_WAIT);
	zassert_true(packet != NULL);
	put_scratch_packet(scratch_packet);

	 /* Test k_lifo_get with K_FOREVER */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);
	packet = k_lifo_get(&lifo_timeout[0], K_FOREVER);
	zassert_true(packet != NULL);
}

/**
 * @brief Verify k_lifo_get() wakes when a thread supplies data within timeout.
 *
 * @details
 * A getter pending with a timeout must be woken as soon as a producer thread
 * puts data, returning that data rather than timing out. The test also covers
 * the K_NO_WAIT and K_FOREVER variants where a child thread conditionally
 * supplies data based on availability on a second LIFO, exercising the
 * interaction between blocking gets and producer timing.
 *
 * Test steps:
 * - Pend on k_lifo_get() with a timeout while a child puts data in time; verify
 *   the data is returned within the timeout window.
 * - With K_NO_WAIT, verify the child reports no data when the LIFO is empty and
 *   reports data when it has been pre-filled.
 * - With K_FOREVER, verify the child retrieves pre-filled data.
 *
 * Expected result:
 * - The getter receives data supplied in time; the reply flags reflect LIFO
 *   availability for each timeout variant.
 *
 * @see k_lifo_get()
 * @see k_lifo_put()
 */
ZTEST(lifo_usage_1cpu, test_timeout_lifo_thread)
{
	void *packet, *scratch_packet;
	static volatile struct reply_packet reply_packet;
	uint32_t start_time, timeout;

	/*
	 * Test lifo with some timeout and child thread that puts
	 * data on the lifo on time
	 */
	timeout = 10U;
	start_time = k_cycle_get_32();

	to_ord_tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
					test_thread_put_timeout, &lifo_timeout[0],
					&timeout, NULL,
					LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	packet = k_lifo_get(&lifo_timeout[0], K_MSEC(timeout + 10));
	zassert_true(packet != NULL);
	zassert_true(is_timeout_in_range(start_time, timeout));
	put_scratch_packet(packet);

	/*
	 * Test k_lifo_get with timeout of K_NO_WAIT and the lifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another lifo. In this test child
	 * thread does not find data on lifo.
	 */
	to_ord_tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
					test_thread_timeout_reply_values,
					(void *)&reply_packet, NULL, NULL,
					LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();
	packet = k_lifo_get(&timeout_order_lifo, K_NO_WAIT);
	zassert_true(packet != NULL);
	zassert_false(reply_packet.reply);

	/*
	 * Test k_lifo_get with timeout of K_NO_WAIT and the lifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another lifo. In this test child
	 * thread does find data on lifo.
	 */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);

	to_ord_tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
					test_thread_timeout_reply_values,
					(void *)&reply_packet, NULL, NULL,
					LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();
	packet = k_lifo_get(&timeout_order_lifo, K_NO_WAIT);
	zassert_true(packet != NULL);
	zassert_true(reply_packet.reply);
	put_scratch_packet(scratch_packet);

	/*
	 * Test k_lifo_get with timeout of K_FOREVER and the lifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another lifo. In this test child
	 * thread does find data on lifo.
	 */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);

	to_ord_tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
					test_thread_timeout_reply_values_wfe,
					(void *)&reply_packet, NULL, NULL,
					LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	packet = k_lifo_get(&timeout_order_lifo, K_FOREVER);
	zassert_true(packet != NULL);
	zassert_true(reply_packet.reply);
	put_scratch_packet(scratch_packet);
}

/* Helper: pend on the thread's lifo with its timeout, confirm it times out with
 * NULL in range, then report back via timeout_order_lifo.
 */
void test_thread_pend_and_timeout(void *p1, void *p2, void *p3)
{
	struct timeout_order_data *d = (struct timeout_order_data *)p1;
	uint32_t start_time;
	void *packet;

	start_time = k_cycle_get_32();
	packet = k_lifo_get(d->klifo, K_MSEC(d->timeout));
	zassert_true(packet == NULL);
	zassert_true(is_timeout_in_range(start_time, d->timeout));

	k_lifo_put(&timeout_order_lifo, d);
}


/**
 * @brief Verify multiple threads pending on one LIFO time out in timeout order.
 *
 * @details
 * When several threads block on the same LIFO with different timeouts and no
 * data arrives, each must time out in ascending-timeout order regardless of the
 * order in which the threads were queued, validating the timeout queue ordering.
 *
 * Test steps:
 * - Spawn several threads that pend on the same LIFO with distinct timeouts.
 * - Each reports back as it times out; verify the reported order matches the
 *   ascending-timeout order.
 *
 * Expected result:
 * - Threads time out strictly in increasing-timeout order.
 *
 * @see k_lifo_get()
 */
ZTEST(lifo_usage_1cpu, test_timeout_threads_pend_on_lifo)
{
	int32_t rv, test_data_size;

	/*
	 * Test multiple threads pending on the same
	 * lifo with different timeouts
	 */
	test_data_size = ARRAY_SIZE(timeout_order_data);
	rv = test_multiple_threads_pending(timeout_order_data, test_data_size);
	zassert_equal(rv, TC_PASS);
}

/**
 * @}
 */

/* Suite setup helper: initialize the lifos used by the tests, pre-fill the
 * scratch packet pool, and seed the lifo_data payloads.
 */
static void test_para_init(void)
{
	intptr_t ii;

	/* Init  kernel objects*/
	k_lifo_init(&lifo_timeout[0]);

	k_lifo_init(&lifo_timeout[0]);

	k_lifo_init(&timeout_order_lifo);

	k_lifo_init(&scratch_lifo_packets_lifo);

	/* Fill Scratch LIFO*/

	for (ii = 0; ii < NUM_SCRATCH_LIFO_PACKETS; ii++) {
		scratch_lifo_packets[ii].data_if_needed = (void *)ii;
		k_lifo_put(&scratch_lifo_packets_lifo,
				(void *)&scratch_lifo_packets[ii]);
	}

	for (int i = 0; i < LIST_LEN; i++) {
		lifo_data[i].data = i + 1;
	}
}

/* Suite setup entry point. */
void *lifo_usage_setup(void)
{
	test_para_init();

	return NULL;
}


ZTEST_SUITE(lifo_usage_1cpu, NULL, lifo_usage_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

ZTEST_SUITE(lifo_usage, NULL, lifo_usage_setup, NULL, NULL, NULL);
