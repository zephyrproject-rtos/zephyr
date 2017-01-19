/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <misc/__assert.h>
#include <misc/util.h>

/* timeout tests
 *
 * Test the nano_xxx_fifo_wait_timeout() APIs.
 *
 * First, the task waits with a timeout and times out. Then it wait with a
 * timeout, but gets the data in time.
 *
 * Then, multiple timeout tests are done for the fibers, to test the ordering
 * of queueing/dequeueing when timeout occurs, first on one fifo, then on
 * multiple fifos.
 *
 * Finally, multiple fibers pend on one fifo, and they all get the
 * data in time, except the last one: this tests that the timeout is
 * recomputed correctly when timeouts are aborted.
 */

#include <tc_nano_timeout_common.h>

#define FIBER_PRIORITY 5

#if defined(CONFIG_DEBUG) && defined(CONFIG_ASSERT)
#define FIBER_STACKSIZE 512
#else
#define FIBER_STACKSIZE 384
#endif

struct scratch_fifo_packet {
	void *link_in_fifo;
	void *data_if_needed;
};

struct reply_packet {
	void *link_in_fifo;
	int reply;
};

#define NUM_SCRATCH_FIFO_PACKETS 20
struct scratch_fifo_packet scratch_fifo_packets[NUM_SCRATCH_FIFO_PACKETS];

struct nano_fifo scratch_fifo_packets_fifo;

void *get_scratch_packet(void)
{
	void *packet = nano_fifo_get(&scratch_fifo_packets_fifo, TICKS_NONE);

	__ASSERT_NO_MSG(packet);

	return packet;
}

void put_scratch_packet(void *packet)
{
	nano_fifo_put(&scratch_fifo_packets_fifo, packet);
}

static struct nano_fifo fifo_timeout[2];
struct nano_fifo timeout_order_fifo;

struct timeout_order_data {
	void *link_in_fifo;
	struct nano_fifo *fifo;
	int32_t timeout;
	int timeout_order;
	int q_order;
};


struct timeout_order_data timeout_order_data[] = {
	{0, &fifo_timeout[0], TIMEOUT(2), 2, 0},
	{0, &fifo_timeout[0], TIMEOUT(4), 4, 1},
	{0, &fifo_timeout[0], TIMEOUT(0), 0, 2},
	{0, &fifo_timeout[0], TIMEOUT(1), 1, 3},
	{0, &fifo_timeout[0], TIMEOUT(3), 3, 4},
};

struct timeout_order_data timeout_order_data_mult_fifo[] = {
	{0, &fifo_timeout[1], TIMEOUT(0), 0, 0},
	{0, &fifo_timeout[0], TIMEOUT(3), 3, 1},
	{0, &fifo_timeout[0], TIMEOUT(5), 5, 2},
	{0, &fifo_timeout[1], TIMEOUT(8), 8, 3},
	{0, &fifo_timeout[1], TIMEOUT(7), 7, 4},
	{0, &fifo_timeout[0], TIMEOUT(1), 1, 5},
	{0, &fifo_timeout[0], TIMEOUT(6), 6, 6},
	{0, &fifo_timeout[0], TIMEOUT(2), 2, 7},
	{0, &fifo_timeout[1], TIMEOUT(4), 4, 8},
};

#define TIMEOUT_ORDER_NUM_FIBERS ARRAY_SIZE(timeout_order_data_mult_fifo)
static char __stack timeout_stacks[TIMEOUT_ORDER_NUM_FIBERS][FIBER_STACKSIZE];

/* a fiber sleeps then puts data on the fifo */
static void test_fiber_put_timeout(int fifo, int timeout)
{
	fiber_sleep((int32_t)timeout);
	nano_fiber_fifo_put((struct nano_fifo *)fifo, get_scratch_packet());
}

/* a fiber pends on a fifo then times out */
static void test_fiber_pend_and_timeout(int data, int unused)
{
	struct timeout_order_data *d = (void *)data;
	int32_t orig_ticks = sys_tick_get();
	void *packet;

	ARG_UNUSED(unused);

	packet = nano_fiber_fifo_get(d->fifo, d->timeout);
	if (packet) {
		TC_ERROR(" *** timeout of %d did not time out.\n",
					d->timeout);
		return;
	}
	if (!is_timeout_in_range(orig_ticks, d->timeout)) {
		return;
	}

	nano_fiber_fifo_put(&timeout_order_fifo, d);
}

/* the task spins several fibers that pend and timeout on fifos */
static int test_multiple_fibers_pending(struct timeout_order_data *test_data,
										int test_data_size)
{
	int ii;

	for (ii = 0; ii < test_data_size; ii++) {
		task_fiber_start(timeout_stacks[ii], FIBER_STACKSIZE,
							test_fiber_pend_and_timeout,
							(int)&test_data[ii], 0,
							FIBER_PRIORITY, 0);
	}

	for (ii = 0; ii < test_data_size; ii++) {
		struct timeout_order_data *data =
			nano_task_fifo_get(&timeout_order_fifo, TICKS_UNLIMITED);

		if (data->timeout_order == ii) {
			TC_PRINT(" got fiber (q order: %d, t/o: %d, fifo %p) as expected\n",
						data->q_order, data->timeout, data->fifo);
		} else {
			TC_ERROR(" *** fiber %d woke up, expected %d\n",
						data->timeout_order, ii);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/* a fiber pends on a fifo with a timeout and gets the data in time */
static void test_fiber_pend_and_get_data(int data, int unused)
{
	struct timeout_order_data *d = (void *)data;
	void *packet;

	ARG_UNUSED(unused);

	packet = nano_fiber_fifo_get(d->fifo, d->timeout);
	if (!packet) {
		TC_PRINT(" *** fiber (q order: %d, t/o: %d, fifo %p) timed out!\n",
						d->q_order, d->timeout, d->fifo);
		return;
	}

	put_scratch_packet(packet);
	nano_fiber_fifo_put(&timeout_order_fifo, d);
}

/* the task spins fibers that get fifo data in time, except the last one */
static int test_multiple_fibers_get_data(struct timeout_order_data *test_data,
					int test_data_size)
{
	struct timeout_order_data *data;
	int ii;

	for (ii = 0; ii < test_data_size-1; ii++) {
		task_fiber_start(timeout_stacks[ii], FIBER_STACKSIZE,
							test_fiber_pend_and_get_data,
							(int)&test_data[ii], 0,
							FIBER_PRIORITY, 0);
	}
	task_fiber_start(timeout_stacks[ii], FIBER_STACKSIZE,
						test_fiber_pend_and_timeout,
						(int)&test_data[ii], 0,
						FIBER_PRIORITY, 0);

	for (ii = 0; ii < test_data_size-1; ii++) {

		nano_task_fifo_put(test_data[ii].fifo, get_scratch_packet());

		data = nano_task_fifo_get(&timeout_order_fifo, TICKS_UNLIMITED);

		if (data->q_order == ii) {
			TC_PRINT(" got fiber (q order: %d, t/o: %d, fifo %p) as expected\n",
						data->q_order, data->timeout, data->fifo);
		} else {
			TC_ERROR(" *** fiber %d woke up, expected %d\n",
						data->q_order, ii);
			return TC_FAIL;
		}
	}

	data = nano_task_fifo_get(&timeout_order_fifo, TICKS_UNLIMITED);
	if (data->q_order == ii) {
		TC_PRINT(" got fiber (q order: %d, t/o: %d, fifo %p) as expected\n",
					data->q_order, data->timeout, data->fifo);
	} else {
		TC_ERROR(" *** fiber %d woke up, expected %d\n",
					data->timeout_order, ii);
		return TC_FAIL;
	}

	return TC_PASS;
}

/* try getting data on fifo with special timeout value, return result in fifo */
static void test_fiber_ticks_special_values(int packet, int special_value)
{
	struct reply_packet *reply_packet = (void *)packet;

	reply_packet->reply =
		!!nano_fiber_fifo_get(&fifo_timeout[0], special_value);

	nano_fiber_fifo_put(&timeout_order_fifo, reply_packet);
}

/* the timeout test entry point */
int test_fifo_timeout(void)
{
	int64_t orig_ticks;
	int32_t timeout;
	int rv;
	void *packet, *scratch_packet;
	int test_data_size;
	int ii;
	struct reply_packet reply_packet;

	nano_fifo_init(&fifo_timeout[0]);
	nano_fifo_init(&fifo_timeout[1]);
	nano_fifo_init(&timeout_order_fifo);
	nano_fifo_init(&scratch_fifo_packets_fifo);

	for (ii = 0; ii < NUM_SCRATCH_FIFO_PACKETS; ii++) {
		scratch_fifo_packets[ii].data_if_needed = (void *)ii;
		nano_task_fifo_put(&scratch_fifo_packets_fifo,
							&scratch_fifo_packets[ii]);
	}

	/* test nano_task_fifo_get() with timeout */
	timeout = 10;
	orig_ticks = sys_tick_get();
	packet = nano_task_fifo_get(&fifo_timeout[0], timeout);
	if (packet) {
		TC_ERROR(" *** timeout of %d did not time out.\n", timeout);
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}
	if ((sys_tick_get() - orig_ticks) < timeout) {
		TC_ERROR(" *** task did not wait long enough on timeout of %d.\n",
					timeout);
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	/* test nano_task_fifo_get with timeout of 0 */

	packet = nano_task_fifo_get(&fifo_timeout[0], 0);
	if (packet) {
		TC_ERROR(" *** timeout of 0 did not time out.\n");
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	/* test nano_task_fifo_get with timeout > 0 */

	TC_PRINT("test nano_task_fifo_get with timeout > 0\n");

	timeout = 3;
	orig_ticks = sys_tick_get();

	packet = nano_task_fifo_get(&fifo_timeout[0], timeout);

	if (packet) {
		TC_ERROR(" *** timeout of %d did not time out.\n",
				timeout);
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	if (!is_timeout_in_range(orig_ticks, timeout)) {
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	TC_PRINT("nano_task_fifo_get timed out as expected\n");

	/*
	 * test nano_task_fifo_get with a timeout and fiber that puts
	 * data on the fifo on time
	 */

	timeout = 5;
	orig_ticks = sys_tick_get();

	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_put_timeout, (int)&fifo_timeout[0],
						timeout,
						FIBER_PRIORITY, 0);

	packet = nano_task_fifo_get(&fifo_timeout[0], (int)(timeout + 5));
	if (!packet) {
		TC_ERROR(" *** data put in time did not return valid pointer.\n");
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	put_scratch_packet(packet);

	if (!is_timeout_in_range(orig_ticks, timeout)) {
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	TC_PRINT("nano_task_fifo_get got fifo in time, as expected\n");

	/*
	 * test nano_task_fifo_get with TICKS_NONE and no data
	 * unavailable.
	 */

	if (nano_task_fifo_get(&fifo_timeout[0], TICKS_NONE)) {
		TC_ERROR("task with TICKS_NONE got data, but shouldn't have\n");
		return TC_FAIL;
	}

	TC_PRINT("task with TICKS_NONE did not get data, as expected\n");

	/*
	 * test nano_task_fifo_get with TICKS_NONE and some data
	 * available.
	 */

	scratch_packet = get_scratch_packet();
	nano_task_fifo_put(&fifo_timeout[0], scratch_packet);
	if (!nano_task_fifo_get(&fifo_timeout[0], TICKS_NONE)) {
		TC_ERROR("task with TICKS_NONE did not get available data\n");
		return TC_FAIL;
	}
	put_scratch_packet(scratch_packet);

	TC_PRINT("task with TICKS_NONE got available data, as expected\n");

	/*
	 * test nano_task_fifo_get with TICKS_UNLIMITED and the
	 * data available.
	 */

	TC_PRINT("Trying to take available data with TICKS_UNLIMITED:\n"
			 " will hang the test if it fails.\n");

	scratch_packet = get_scratch_packet();
	nano_task_fifo_put(&fifo_timeout[0], scratch_packet);
	if (!nano_task_fifo_get(&fifo_timeout[0], TICKS_UNLIMITED)) {
		TC_ERROR(" *** This will never be hit!!! .\n");
		return TC_FAIL;
	}
	put_scratch_packet(scratch_packet);

	TC_PRINT("task with TICKS_UNLIMITED got available data, as expected\n");

	/* test fiber with timeout of TICKS_NONE not getting data on empty fifo */

	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_ticks_special_values,
						(int)&reply_packet, TICKS_NONE, FIBER_PRIORITY, 0);

	if (!nano_task_fifo_get(&timeout_order_fifo, TICKS_NONE)) {
		TC_ERROR(" *** fiber should have run and filled the fifo.\n");
		return TC_FAIL;
	}

	if (reply_packet.reply != 0) {
		TC_ERROR(" *** fiber should not have obtained the data.\n");
		return TC_FAIL;
	}

	TC_PRINT("fiber with TICKS_NONE did not get data, as expected\n");

	/* test fiber with timeout of TICKS_NONE getting data when available */

	scratch_packet = get_scratch_packet();
	nano_task_fifo_put(&fifo_timeout[0], scratch_packet);
	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_ticks_special_values,
						(int)&reply_packet, TICKS_NONE, FIBER_PRIORITY, 0);
	put_scratch_packet(scratch_packet);

	if (!nano_task_fifo_get(&timeout_order_fifo, TICKS_NONE)) {
		TC_ERROR(" *** fiber should have run and filled the fifo.\n");
		return TC_FAIL;
	}

	if (reply_packet.reply != 1) {
		TC_ERROR(" *** fiber should have obtained the data.\n");
		return TC_FAIL;
	}

	TC_PRINT("fiber with TICKS_NONE got available data, as expected\n");

	/* test fiber with TICKS_UNLIMITED timeout getting data when availalble */

	scratch_packet = get_scratch_packet();
	nano_task_fifo_put(&fifo_timeout[0], scratch_packet);
	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
			test_fiber_ticks_special_values,
			(int)&reply_packet, TICKS_UNLIMITED, FIBER_PRIORITY, 0);
	put_scratch_packet(scratch_packet);

	if (!nano_task_fifo_get(&timeout_order_fifo, TICKS_NONE)) {
		TC_ERROR(" *** fiber should have run and filled the fifo.\n");
		return TC_FAIL;
	}

	if (reply_packet.reply != 1) {
		TC_ERROR(" *** fiber should have obtained the data.\n");
		return TC_FAIL;
	}

	TC_PRINT("fiber with TICKS_UNLIMITED got available data, as expected\n");

	/* test multiple fibers pending on the same fifo with different timeouts */

	test_data_size = ARRAY_SIZE(timeout_order_data);

	TC_PRINT("testing timeouts of %d fibers on same fifo\n", test_data_size);

	rv = test_multiple_fibers_pending(timeout_order_data, test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not time out in the right order\n");
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	/* test mult. fibers pending on different fifos with different timeouts */

	test_data_size = ARRAY_SIZE(timeout_order_data_mult_fifo);

	TC_PRINT("testing timeouts of %d fibers on different fifos\n",
				test_data_size);

	rv = test_multiple_fibers_pending(timeout_order_data_mult_fifo,
										test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not time out in the right order\n");
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	/*
	 * test multiple fibers pending on same fifo with different timeouts, but
	 * getting the data in time, except the last one.
	 */

	test_data_size = ARRAY_SIZE(timeout_order_data);

	TC_PRINT("testing %d fibers timing out, but obtaining the data in time\n"
				"(except the last one, which times out)\n",
				test_data_size);

	rv = test_multiple_fibers_get_data(timeout_order_data, test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not get the data in the right order\n");
		TC_END_RESULT(TC_FAIL);
		return TC_FAIL;
	}

	TC_END_RESULT(TC_PASS);
	return TC_PASS;
}
