/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

/*
 * @file
 * @brief Test fifo APIs timeout
 *
 * This module tests following fifo timeout scenarios
 *
 * First, the thread waits with a timeout and times out. Then it wait with a
 * timeout, but gets the data in time.
 *
 * Then, multiple timeout tests are done for the threads, to test the ordering
 * of queueing/dequeueing when timeout occurs, first on one fifo, then on
 * multiple fifos.
 *
 * Finally, multiple threads pend on one fifo, and they all get the
 * data in time, except the last one: this tests that the timeout is
 * recomputed correctly when timeouts are aborted.
 */

struct scratch_fifo_packet {
	void *link_in_fifo;
	void *data_if_needed;
};

struct reply_packet {
	void *link_in_fifo;
	int32_t reply;
};

struct timeout_order_data {
	void *link_in_fifo;
	struct k_fifo *fifo;
	uint32_t timeout;
	int32_t timeout_order;
	int32_t q_order;
};


#define NUM_SCRATCH_FIFO_PACKETS 20
struct scratch_fifo_packet scratch_fifo_packets[NUM_SCRATCH_FIFO_PACKETS];

struct k_fifo scratch_fifo_packets_fifo;

static struct k_fifo fifo_timeout[2];
struct k_fifo timeout_order_fifo;

struct timeout_order_data timeout_order_data[] = {
	{0, &fifo_timeout[0], 20, 2, 0},
	{0, &fifo_timeout[0], 40, 4, 1},
	{0, &fifo_timeout[0], 0, 0, 2},
	{0, &fifo_timeout[0], 10, 1, 3},
	{0, &fifo_timeout[0], 30, 3, 4},
};

struct timeout_order_data timeout_order_data_mult_fifo[] = {
	{0, &fifo_timeout[1], 0, 0, 0},
	{0, &fifo_timeout[0], 30, 3, 1},
	{0, &fifo_timeout[0], 50, 5, 2},
	{0, &fifo_timeout[1], 80, 8, 3},
	{0, &fifo_timeout[1], 70, 7, 4},
	{0, &fifo_timeout[0], 10, 1, 5},
	{0, &fifo_timeout[0], 60, 6, 6},
	{0, &fifo_timeout[0], 20, 2, 7},
	{0, &fifo_timeout[1], 40, 4, 8},
};

#define TIMEOUT_ORDER_NUM_THREADS	ARRAY_SIZE(timeout_order_data_mult_fifo)
#define TSTACK_SIZE			(512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define FIFO_THREAD_PRIO		-5

static K_THREAD_STACK_ARRAY_DEFINE(ttstack,
		TIMEOUT_ORDER_NUM_THREADS, TSTACK_SIZE);
static struct k_thread ttdata[TIMEOUT_ORDER_NUM_THREADS];
static k_tid_t tid[TIMEOUT_ORDER_NUM_THREADS];

static void *get_scratch_packet(void)
{
	void *packet = k_fifo_get(&scratch_fifo_packets_fifo, K_NO_WAIT);

	zassert_true(packet != NULL);
	return packet;
}

static void put_scratch_packet(void *packet)
{
	k_fifo_put(&scratch_fifo_packets_fifo, packet);
}

static bool is_timeout_in_range(uint32_t start_time, uint32_t timeout)
{
	uint32_t stop_time, diff;

	stop_time = k_cycle_get_32();
	diff = (uint32_t)k_cyc_to_ns_floor64(stop_time -
			start_time) / NSEC_PER_USEC;
	diff = diff / USEC_PER_MSEC;
	return timeout <= diff;
}

/* a thread sleeps then puts data on the fifo */
static void test_thread_put_timeout(void *p1, void *p2, void *p3)
{
	uint32_t timeout = *((uint32_t *)p2);

	k_msleep(timeout);
	k_fifo_put((struct k_fifo *)p1, get_scratch_packet());
}

/* a thread pends on a fifo then times out */
static void test_thread_pend_and_timeout(void *p1, void *p2, void *p3)
{
	struct timeout_order_data *d = (struct timeout_order_data *)p1;
	uint32_t start_time;
	void *packet;

	k_msleep(1); /* Align to ticks */

	start_time = k_cycle_get_32();
	packet = k_fifo_get(d->fifo, K_MSEC(d->timeout));
	zassert_true(packet == NULL);
	zassert_true(is_timeout_in_range(start_time, d->timeout));

	k_fifo_put(&timeout_order_fifo, d);
}
/* Spins several threads that pend and timeout on fifos */
static int test_multiple_threads_pending(struct timeout_order_data *test_data,
					 int test_data_size)
{
	int ii, j;
	uint32_t diff_ms;

	for (ii = 0; ii < test_data_size; ii++) {
		tid[ii] = k_thread_create(&ttdata[ii], ttstack[ii], TSTACK_SIZE,
				test_thread_pend_and_timeout,
				&test_data[ii], NULL, NULL,
				FIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* In general, there is no guarantee of wakeup order when multiple
	 * threads are woken up on the same tick. This can especially happen
	 * when the system is loaded. However, in this particular test, we
	 * are controlling the system state and hence we can make a reasonable
	 * estimation of a timeout occurring with the max deviation of an
	 * additional tick. Hence the timeout order may slightly be different
	 * from what we normally expect.
	 */
	for (ii = 0; ii < test_data_size; ii++) {
		struct timeout_order_data *data =
			k_fifo_get(&timeout_order_fifo, K_FOREVER);

		zassert_not_null(data, NULL);
		if (data->timeout_order == ii) {
			TC_PRINT(" thread (q order: %d, t/o: %d, fifo %p)\n",
				data->q_order, data->timeout, data->fifo);
		} else {
			/* Get the index of the thread which should have
			 * actually timed out.
			 */
			for (j = 0; j < test_data_size; j++) {
				if (test_data[j].timeout_order == ii) {
					break;
				}
			}

			if (data->timeout > test_data[j].timeout) {
				diff_ms = data->timeout - test_data[j].timeout;
			} else {
				diff_ms = test_data[j].timeout - data->timeout;
			}

			if (k_ms_to_ticks_ceil32(diff_ms) == 1) {
				TC_PRINT(
				" thread (q order: %d, t/o: %d, fifo %p)\n",
				data->q_order, data->timeout, data->fifo);
			} else {
				TC_ERROR(
				" *** thread %d woke up, expected %d\n",
				data->timeout_order, ii);
				return TC_FAIL;
			}
		}
	}

	return TC_PASS;
}

/* a thread pends on a fifo with a timeout and gets the data in time */
static void test_thread_pend_and_get_data(void *p1, void *p2, void *p3)
{
	struct timeout_order_data *d = (struct timeout_order_data *)p1;
	void *packet;

	packet = k_fifo_get(d->fifo, K_MSEC(d->timeout));
	zassert_true(packet != NULL);

	put_scratch_packet(packet);
	k_fifo_put(&timeout_order_fifo, d);
}

/* Spins child threads that get fifo data in time, except the last one */
static int test_multiple_threads_get_data(struct timeout_order_data *test_data,
					 int test_data_size)
{
	struct timeout_order_data *data;
	int ii;

	for (ii = 0; ii < test_data_size-1; ii++) {
		tid[ii] = k_thread_create(&ttdata[ii], ttstack[ii], TSTACK_SIZE,
				test_thread_pend_and_get_data,
				&test_data[ii], NULL, NULL,
				K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);
	}

	tid[ii] = k_thread_create(&ttdata[ii], ttstack[ii], TSTACK_SIZE,
				test_thread_pend_and_timeout,
				&test_data[ii], NULL, NULL,
				K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);

	for (ii = 0; ii < test_data_size-1; ii++) {
		k_fifo_put(test_data[ii].fifo, get_scratch_packet());

		data = k_fifo_get(&timeout_order_fifo, K_FOREVER);
		if (!data) {
			TC_ERROR("thread %d got NULL value from fifo\n", ii);
			return TC_FAIL;
		}

		if (data->q_order != ii) {
			TC_ERROR(" *** thread %d woke up, expected %d\n",
				 data->q_order, ii);
			return TC_FAIL;
		}

		if (data->q_order == ii) {
			TC_PRINT(" thread (q order: %d, t/o: %d, fifo %p)\n",
				data->q_order, data->timeout, data->fifo);
		}
	}

	data = k_fifo_get(&timeout_order_fifo, K_FOREVER);
	if (!data) {
		TC_ERROR("thread %d got NULL value from fifo\n", ii);
		return TC_FAIL;
	}

	if (data->q_order != ii) {
		TC_ERROR(" *** thread %d woke up, expected %d\n",
			 data->q_order, ii);
		return TC_FAIL;
	}

	TC_PRINT(" thread (q order: %d, t/o: %d, fifo %p)\n",
		 data->q_order, data->timeout, data->fifo);

	return TC_PASS;
}

/* try getting data on fifo with special timeout value, return result in fifo */
static void test_thread_timeout_reply_values(void *p1, void *p2, void *p3)
{
	struct reply_packet *reply_packet = (struct reply_packet *)p1;

	reply_packet->reply =
		!!k_fifo_get(&fifo_timeout[0], K_NO_WAIT);

	k_fifo_put(&timeout_order_fifo, reply_packet);
}

static void test_thread_timeout_reply_values_wfe(void *p1, void *p2, void *p3)
{
	struct reply_packet *reply_packet = (struct reply_packet *)p1;

	reply_packet->reply =
		!!k_fifo_get(&fifo_timeout[0], K_FOREVER);

	k_fifo_put(&timeout_order_fifo, reply_packet);
}

/**
 * @addtogroup kernel_fifo_tests
 * @{
 */

/**
 * @brief Test empty fifo with timeout and K_NO_WAIT
 * @see k_fifo_get()
 */
ZTEST(fifo_timeout_1cpu, test_timeout_empty_fifo)
{
	void *packet;
	uint32_t start_time, timeout;

	k_msleep(1); /* Align to ticks */

	/* Test empty fifo with timeout */
	timeout = 10U;
	start_time = k_cycle_get_32();
	packet = k_fifo_get(&fifo_timeout[0], K_MSEC(timeout));
	zassert_true(packet == NULL);
	zassert_true(is_timeout_in_range(start_time, timeout));

	/* Test empty fifo with timeout of K_NO_WAIT */
	packet = k_fifo_get(&fifo_timeout[0], K_NO_WAIT);
	zassert_true(packet == NULL);
}

/**
 * @brief Test non empty fifo with timeout and K_NO_WAIT
 * @see k_fifo_get(), k_fifo_put()
 */
ZTEST(fifo_timeout, test_timeout_non_empty_fifo)
{
	void *packet, *scratch_packet;

	 /* Test k_fifo_get with K_NO_WAIT */
	scratch_packet = get_scratch_packet();
	k_fifo_put(&fifo_timeout[0], scratch_packet);
	packet = k_fifo_get(&fifo_timeout[0], K_NO_WAIT);
	zassert_true(packet != NULL);
	put_scratch_packet(scratch_packet);

	 /* Test k_fifo_get with K_FOREVER */
	scratch_packet = get_scratch_packet();
	k_fifo_put(&fifo_timeout[0], scratch_packet);
	packet = k_fifo_get(&fifo_timeout[0], K_FOREVER);
	zassert_true(packet != NULL);
	put_scratch_packet(scratch_packet);
}

/**
 * @brief Test fifo with timeout and K_NO_WAIT
 * @details In first scenario test fifo with some timeout where child thread
 * puts data on the fifo on time. In second scenario test k_fifo_get with
 * timeout of K_NO_WAIT and the fifo should be filled by the child thread
 * based on the data availability on another fifo. In third scenario test
 * k_fifo_get with timeout of K_FOREVER and the fifo should be filled by
 * the child thread based on the data availability on another fifo.
 * @see k_fifo_get(), k_fifo_put()
 */
ZTEST(fifo_timeout_1cpu, test_timeout_fifo_thread)
{
	void *packet, *scratch_packet;
	struct reply_packet reply_packet;
	uint32_t start_time, timeout;

	k_msleep(1); /* Align to ticks */

	/*
	 * Test fifo with some timeout and child thread that puts
	 * data on the fifo on time
	 */
	timeout = 10U;
	start_time = k_cycle_get_32();

	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_put_timeout, &fifo_timeout[0],
				&timeout, NULL,
				FIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	packet = k_fifo_get(&fifo_timeout[0], K_MSEC(timeout + 10));
	zassert_true(packet != NULL);
	zassert_true(is_timeout_in_range(start_time, timeout));
	put_scratch_packet(packet);

	/*
	 * Test k_fifo_get with timeout of K_NO_WAIT and the fifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another fifo. In this test child
	 * thread does not find data on fifo.
	 */
	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_timeout_reply_values,
				&reply_packet, NULL, NULL,
				FIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();
	packet = k_fifo_get(&timeout_order_fifo, K_NO_WAIT);
	zassert_true(packet != NULL);
	zassert_false(reply_packet.reply);

	/*
	 * Test k_fifo_get with timeout of K_NO_WAIT and the fifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another fifo. In this test child
	 * thread does find data on fifo.
	 */
	scratch_packet = get_scratch_packet();
	k_fifo_put(&fifo_timeout[0], scratch_packet);

	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_timeout_reply_values,
				&reply_packet, NULL, NULL,
				FIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();
	packet = k_fifo_get(&timeout_order_fifo, K_NO_WAIT);
	zassert_true(packet != NULL);
	zassert_true(reply_packet.reply);
	put_scratch_packet(scratch_packet);

	/*
	 * Test k_fifo_get with timeout of K_FOREVER and the fifo
	 * should be filled be filled by the child thread based on
	 * the data availability on another fifo. In this test child
	 * thread does find data on fifo.
	 */
	scratch_packet = get_scratch_packet();
	k_fifo_put(&fifo_timeout[0], scratch_packet);

	tid[0] = k_thread_create(&ttdata[0], ttstack[0], TSTACK_SIZE,
				test_thread_timeout_reply_values_wfe,
				&reply_packet, NULL, NULL,
				FIFO_THREAD_PRIO, K_INHERIT_PERMS, K_NO_WAIT);

	packet = k_fifo_get(&timeout_order_fifo, K_FOREVER);
	zassert_true(packet != NULL);
	zassert_true(reply_packet.reply);
	put_scratch_packet(scratch_packet);
}

/**
 * @brief Test fifo with different timeouts
 * @details test multiple threads pending on the same fifo with
 * different timeouts
 * @see k_fifo_get(), k_fifo_put()
 */
ZTEST(fifo_timeout_1cpu, test_timeout_threads_pend_on_fifo)
{
	int32_t rv, test_data_size;

	/*
	 * Test multiple threads pending on the same
	 * fifo with different timeouts
	 */
	test_data_size = ARRAY_SIZE(timeout_order_data);
	rv = test_multiple_threads_pending(timeout_order_data, test_data_size);
	zassert_equal(rv, TC_PASS);
}

/**
 * @brief Test multiple fifos with different timeouts
 * @details test multiple threads pending on different fifos
 * with different timeouts
 * @see k_fifo_get(), k_fifo_put()
 */
ZTEST(fifo_timeout_1cpu, test_timeout_threads_pend_on_dual_fifos)
{
	int32_t rv, test_data_size;

	/*
	 * Test multiple threads pending on different
	 * fifos with different timeouts
	 */
	test_data_size = ARRAY_SIZE(timeout_order_data_mult_fifo);
	rv = test_multiple_threads_pending(timeout_order_data_mult_fifo,
							test_data_size);
	zassert_equal(rv, TC_PASS);

}

/**
 * @brief Test same fifo with different timeouts
 * @details test multiple threads pending on the same fifo with
 * different timeouts but getting the data in time
 * @see k_fifo_get(), k_fifo_put()
 */
ZTEST(fifo_timeout_1cpu, test_timeout_threads_pend_fail_on_fifo)
{
	int32_t rv, test_data_size;

	/*
	 * Test multiple threads pending on same
	 * fifo with different timeouts, but getting
	 * the data in time, except the last one.
	 */
	test_data_size = ARRAY_SIZE(timeout_order_data);
	rv = test_multiple_threads_get_data(timeout_order_data, test_data_size);
	zassert_equal(rv, TC_PASS);
}

/**
 * @brief Test fifo init
 * @see k_fifo_init(), k_fifo_put()
 */
static void *test_timeout_setup(void)
{
	intptr_t ii;

	/* Init kernel objects */
	k_fifo_init(&fifo_timeout[0]);
	k_fifo_init(&fifo_timeout[1]);
	k_fifo_init(&timeout_order_fifo);
	k_fifo_init(&scratch_fifo_packets_fifo);

	/* Fill scratch fifo */
	for (ii = 0; ii < NUM_SCRATCH_FIFO_PACKETS; ii++) {
		scratch_fifo_packets[ii].data_if_needed = (void *)ii;
		k_fifo_put(&scratch_fifo_packets_fifo,
				(void *)&scratch_fifo_packets[ii]);
	}

	return NULL;
}
/**
 * @}
 */

ZTEST_SUITE(fifo_timeout, NULL, test_timeout_setup, NULL, NULL, NULL);

ZTEST_SUITE(fifo_timeout_1cpu, NULL, test_timeout_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
