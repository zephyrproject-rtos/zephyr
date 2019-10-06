/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "lifo_usage.h"
#include <kernel.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LIST_LEN 2

struct k_lifo lifo, plifo;
static ldata_t data[LIST_LEN];
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
	s32_t reply;
};

struct timeout_order_data {
	void *link_in_lifo;
	struct k_lifo *klifo;
	u32_t timeout;
	s32_t timeout_order;
	s32_t q_order;
};

static struct k_lifo lifo_timeout[2];

struct timeout_order_data timeout_order_data[] = {
	{0, &lifo_timeout[0], 20, 2, 0},
	{0, &lifo_timeout[0], 40, 4, 1},
	{0, &lifo_timeout[0], 0, 0, 2},
	{0, &lifo_timeout[0], 10, 1, 3},
	{0, &lifo_timeout[0], 30, 3, 4},
};

struct timeout_order_data timeout_order_data_mult_lifo[] = {
	{0, &lifo_timeout[1], 0, 0, 0},
	{0, &lifo_timeout[0], 30, 3, 1},
	{0, &lifo_timeout[0], 50, 5, 2},
	{0, &lifo_timeout[1], 80, 8, 3},
	{0, &lifo_timeout[1], 70, 7, 4},
	{0, &lifo_timeout[0], 10, 1, 5},
	{0, &lifo_timeout[0], 60, 6, 6},
	{0, &lifo_timeout[0], 20, 2, 7},
	{0, &lifo_timeout[1], 40, 4, 8},
};

#define NUM_SCRATCH_LIFO_PACKETS 20
#define TIMEOUT_ORDER_NUM_THREADS	ARRAY_SIZE(timeout_order_data_mult_lifo)
#define TSTACK_SIZE			(1024 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LIFO_THREAD_PRIO		-5

struct scratch_lifo_packet scratch_lifo_packets[NUM_SCRATCH_LIFO_PACKETS];

struct k_lifo scratch_lifo_packets_lifo;

static k_tid_t tid[TIMEOUT_ORDER_NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(ttstack,
		TIMEOUT_ORDER_NUM_THREADS, TSTACK_SIZE);
static struct k_thread ttdata[TIMEOUT_ORDER_NUM_THREADS];

static void *get_scratch_packet(void)
{
	void *packet = k_lifo_get(&scratch_lifo_packets_lifo, K_NO_WAIT);

	zassert_true(packet != NULL, NULL);
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
	zassert_equal(ret, (void *)&data[1], NULL);

	ret = k_lifo_get((struct k_lifo *)p1, K_FOREVER);

	zassert_equal(ret, (void *)&data[0], NULL);

	k_sem_give(&start_sema);
}

static bool is_timeout_in_range(u32_t start_time, u32_t timeout)
{
	u32_t stop_time, diff;

	stop_time = k_cycle_get_32();
	diff = SYS_CLOCK_HW_CYCLES_TO_NS(stop_time -
					start_time) / NSEC_PER_USEC;
	diff = diff / USEC_PER_MSEC;
	return timeout <= diff;
}

static int test_multiple_threads_pending(struct timeout_order_data *test_data,
					 int test_data_size)
{
	int ii;

	for (ii = 0; ii < test_data_size; ii++) {
		tid[ii] = k_thread_create(&ttdata[ii], ttstack[ii], TSTACK_SIZE,
				test_thread_pend_and_timeout,
				&test_data[ii], NULL, NULL,
				LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);
	}

	for (ii = 0; ii < test_data_size; ii++) {
		struct timeout_order_data *data =
			k_lifo_get(&timeout_order_lifo, K_FOREVER);

		if (data->timeout_order == ii) {
			TC_PRINT(" thread (q order: %d, t/o: %d, lifo %p)\n",
				data->q_order, data->timeout, data->klifo);
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
	k_lifo_put((struct k_lifo *)p1, (void *)&data[0]);

	k_lifo_put((struct k_lifo *)p1, (void *)&data[1]);
	k_sem_give(&wait_sema);
}


/**
 * @addtogroup kernel_lifo_tests
 * @{
 */

/**
 * @brief try getting data on lifo with special timeout value,
 * return result in lifo
 *
 *
 * @see k_lifo_put()
 */
static void test_thread_timeout_reply_values(void *p1, void *p2, void *p3)
{
	struct reply_packet *reply_packet = (struct reply_packet *)p1;

	reply_packet->reply =
		!!k_lifo_get(&lifo_timeout[0], K_NO_WAIT);

	k_lifo_put(&timeout_order_lifo, reply_packet);
}

/**
 * @see k_lifo_put()
 */
static void test_thread_timeout_reply_values_wfe(void *p1, void *p2, void *p3)
{
	struct reply_packet *reply_packet = (struct reply_packet *)p1;

	reply_packet->reply =
		!!k_lifo_get(&lifo_timeout[0], K_FOREVER);

	k_lifo_put(&timeout_order_lifo, reply_packet);
}

/**
 * @brief A thread sleeps then puts data on the lifo
 *
 * @see k_lifo_put()
 */
static void test_thread_put_timeout(void *p1, void *p2, void *p3)
{
	u32_t timeout = *((u32_t *)p2);

	k_sleep(timeout);
	k_lifo_put((struct k_lifo *)p1, get_scratch_packet());
}

/**
 * @brief Test last in, first out queue using LIFO
 * @see k_sem_init(), k_lifo_put(), k_lifo_get()
 */
static void test_lifo_nowait(void)
{
	k_lifo_init(&lifo);

	k_sem_init(&start_sema, 0, 1);

	/* put some data on lifo */
	k_lifo_put(&lifo, (void *)&data[0]);

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_nowait, &lifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_lifo_put(&lifo, (void *)&data[1]);

	/* Allow another thread to read lifo */
	k_sem_take(&start_sema, K_FOREVER);
	k_thread_abort(tid);
}

/**
 * @brief Test pending reader in LIFO
 * @see k_lifo_init(), k_lifo_get(), k_lifo_put()
 */
static void test_lifo_wait(void)
{
	int *ret;

	k_lifo_init(&plifo);
	k_sem_init(&wait_sema, 0, 1);

	k_tid_t tid = k_thread_create(&tdata1, tstack1, STACK_SIZE,
				      thread_entry_wait, &plifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	ret = k_lifo_get(&plifo, K_FOREVER);

	zassert_equal(ret, (void *)&data[0], NULL);

	k_sem_take(&wait_sema, K_FOREVER);

	ret = k_lifo_get(&plifo, K_FOREVER);

	zassert_equal(ret, (void *)&data[1], NULL);

	k_thread_abort(tid);
}

/**
 * @brief Test reading empty LIFO
 * @see k_lifo_get()
 */
static void test_timeout_empty_lifo(void)
{
	void *packet;

	u32_t start_time, timeout;

	timeout = 10U;

	start_time = k_cycle_get_32();

	packet = k_lifo_get(&lifo_timeout[0], timeout);

	zassert_equal(packet, NULL, NULL);

	zassert_true(is_timeout_in_range(start_time, timeout), NULL);

	/* Test empty lifo with timeout of K_NO_WAIT */
	packet = k_lifo_get(&lifo_timeout[0], K_NO_WAIT);
	zassert_equal(packet, NULL, NULL);
}

/**
 * @brief Test read and write operation in LIFO with timeout
 * @see k_lifo_put(), k_lifo_get()
 */
static void test_timeout_non_empty_lifo(void)
{
	void *packet, *scratch_packet;

	/* Test k_lifo_get with K_NO_WAIT */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);
	packet = k_lifo_get(&lifo_timeout[0], K_NO_WAIT);
	zassert_true(packet != NULL, NULL);
	put_scratch_packet(scratch_packet);

	 /* Test k_lifo_get with K_FOREVER */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);
	packet = k_lifo_get(&lifo_timeout[0], K_FOREVER);
	zassert_true(packet != NULL, NULL);
}

/**
 * @brief Test LIFO with timeout
 * @see k_lifo_put(), k_lifo_get()
 */
static void test_timeout_lifo_thread(void)
{
	void *packet, *scratch_packet;
	struct reply_packet reply_packet;
	u32_t start_time, timeout;

	/*
	 * Test lifo with some timeout and child thread that puts
	 * data on the lifo on time
	 */
	timeout = 10U;
	start_time = k_cycle_get_32();

	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_put_timeout, &lifo_timeout[0],
				&timeout, NULL,
				LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	packet = k_lifo_get(&lifo_timeout[0], timeout + 10);
	zassert_true(packet != NULL, NULL);
	zassert_true(is_timeout_in_range(start_time, timeout), NULL);
	put_scratch_packet(packet);

	/*
	 * Test k_lifo_get with timeout of K_NO_WAIT and the lifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another lifo. In this test child
	 * thread does not find data on lifo.
	 */
	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_timeout_reply_values,
				&reply_packet, NULL, NULL,
				LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();
	packet = k_lifo_get(&timeout_order_lifo, K_NO_WAIT);
	zassert_true(packet != NULL, NULL);
	zassert_false(reply_packet.reply, NULL);

	/*
	 * Test k_lifo_get with timeout of K_NO_WAIT and the lifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another lifo. In this test child
	 * thread does find data on lifo.
	 */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);

	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_timeout_reply_values,
				&reply_packet, NULL, NULL,
				LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();
	packet = k_lifo_get(&timeout_order_lifo, K_NO_WAIT);
	zassert_true(packet != NULL, NULL);
	zassert_true(reply_packet.reply, NULL);
	put_scratch_packet(scratch_packet);

	/*
	 * Test k_lifo_get with timeout of K_FOREVER and the lifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another lifo. In this test child
	 * thread does find data on lifo.
	 */
	scratch_packet = get_scratch_packet();
	k_lifo_put(&lifo_timeout[0], scratch_packet);

	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_timeout_reply_values_wfe,
				&reply_packet, NULL, NULL,
				LIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	packet = k_lifo_get(&timeout_order_lifo, K_FOREVER);
	zassert_true(packet != NULL, NULL);
	zassert_true(reply_packet.reply, NULL);
	put_scratch_packet(scratch_packet);
}

/**
 * @brief a thread pends on a lifo then times out
 * @see k_lifo_put(), k_lifo_get()
 */
void test_thread_pend_and_timeout(void *p1, void *p2, void *p3)
{
	struct timeout_order_data *d = (struct timeout_order_data *)p1;
	u32_t start_time;
	void *packet;

	start_time = k_cycle_get_32();
	packet = k_lifo_get(d->klifo, d->timeout);
	zassert_true(packet == NULL, NULL);
	zassert_true(is_timeout_in_range(start_time, d->timeout), NULL);

	k_lifo_put(&timeout_order_lifo, d);
}


/**
 * @brief Test multiple pending readers in LIFO
 * @details test multiple threads pending on the same lifo
 * with different timeouts
 * @see k_lifo_get()
 */
static void test_timeout_threads_pend_on_lifo(void)
{
	s32_t rv, test_data_size;

	/*
	 * Test multiple threads pending on the same
	 * lifo with different timeouts
	 */
	test_data_size = ARRAY_SIZE(timeout_order_data);
	rv = test_multiple_threads_pending(timeout_order_data, test_data_size);
	zassert_equal(rv, TC_PASS, NULL);
}

/**
 * @brief Test LIFO initialization with various parameters
 * @see k_lifo_init(), k_lifo_put()
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
		data[i].data = i + 1;
	}
}

/**
 * @}
 */

/** test case main entry */
void test_main(void)
{
	test_para_init();

	ztest_test_suite(test_lifo_usage,
			 ztest_unit_test(test_lifo_nowait),
			 ztest_1cpu_unit_test(test_lifo_wait),
			 ztest_1cpu_unit_test(test_timeout_empty_lifo),
			 ztest_unit_test(test_timeout_non_empty_lifo),
			 ztest_1cpu_unit_test(test_timeout_lifo_thread),
			 ztest_1cpu_unit_test(test_timeout_threads_pend_on_lifo));
	ztest_run_test_suite(test_lifo_usage);
}
