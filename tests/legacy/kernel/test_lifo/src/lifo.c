/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file
 * @brief Test nanokernel LIFO APIs
 *
 * DESCRIPTION
 * This module tests four basic scenarios with the usage of the following LIFO
 * routines:
 *
 * nano_isr_lifo_get, nano_isr_lifo_put
 * nano_fiber_lifo_get, nano_fiber_lifo_put
 * nano_task_lifo_get, nano_task_lifo_put
 *
 * Scenario #1
 * Getting (and waiting for an object) from an empty LIFO.  Both fibers and
 * tasks can wait on a LIFO, but an ISR can not.
 *
 * Scenario #2
 * Getting objects from a non-empty LIFO.  Fibers, tasks and ISRs are all
 * allowed to get an object from a non-empty LIFO.
 *
 * Scenario #3:
 * Multiple fibers pend on the same LIFO.
 *
 * Scenario #4:
 * Timeout scenarios with multiple LIFOs and fibers.
 *
 * These scenarios will be tested using a combinations of tasks, fibers and
 * ISRs.
 */

#include <zephyr.h>
#include <tc_util.h>
#include <misc/util.h>
#include <misc/__assert.h>
#include <irq_offload.h>

/* test uses 2 software IRQs */
#define NUM_SW_IRQS 2

#include <util_test_common.h>

#define FIBER_STACKSIZE    384
#define FIBER_PRIORITY     4

typedef struct {
	struct nano_lifo *lifo_ptr;  /* LIFO */
	void *data;     /* pointer to data to add */
} ISR_LIFO_INFO;

typedef struct {
	uint32_t   link;     /* 32-bit word for LIFO to use as a link */
	uint32_t   data;     /* miscellaneous data put on LIFO (not important) */
} LIFO_ITEM;

/* Items to be added/removed from LIFO during the test */
static LIFO_ITEM  lifoItem[4] = {
		{0, 1},
		{0, 2},
		{0, 3},
		{0, 4},
	};

static struct nano_lifo      test_lifo;           /* LIFO used in test */
static struct nano_sem       taskWaitSem;         /* task waits on this semaphore */
static struct nano_sem       fiberWaitSem;        /* fiber waits on this semaphore */
static struct nano_timer     timer;
static void *timerData[1];
static ISR_LIFO_INFO  isrLifoInfo = {&test_lifo, NULL};

static volatile int  fiberDetectedFailure = 0; /* non-zero on failure */

static char __stack fiberStack[FIBER_STACKSIZE];

static struct nano_lifo multi_waiters;
static struct nano_sem reply_multi_waiters;

/**
 *
 * @brief Add an item to a LIFO
 *
 * This routine is the ISR handler for _trigger_nano_isr_lifo_put().  It adds
 * an item to the LIFO in the context of an ISR.
 *
 * @param data    pointer to ISR handler parameter
 *
 * @return N/A
 */

void isr_lifo_put(void *data)
{
	ISR_LIFO_INFO *pInfo = (ISR_LIFO_INFO *) data;

	nano_isr_lifo_put(pInfo->lifo_ptr, pInfo->data);
}

static void _trigger_nano_isr_lifo_put(void)
{
	irq_offload(isr_lifo_put, &isrLifoInfo);
}


/**
 *
 * @brief Get an item from a LIFO
 *
 * This routine is the ISR handler for _trigger_nano_isr_lifo_get().  It gets
 * an item from the LIFO in the context of an ISR.
 *
 * @param data    pointer to ISR handler parameter
 *
 * @return N/A
 */

void isr_lifo_get(void *data)
{
	ISR_LIFO_INFO *pInfo = (ISR_LIFO_INFO *) data;

	pInfo->data = nano_isr_lifo_get(pInfo->lifo_ptr, TICKS_NONE);
}

static void _trigger_nano_isr_lifo_get(void)
{
	irq_offload(isr_lifo_get, &isrLifoInfo);
}


/**
 *
 * @brief Fiber portion of test that waits on a LIFO
 *
 * This routine works with taskLifoWaitTest() to test the addition and removal
 * of items to/from a LIFO.  The cases covered will have a fiber or task waiting
 * on an empty LIFO.
 *
 * @return 0 on success, -1 on failure
 */

int fiberLifoWaitTest(void)
{
	void *data;     /* ptr to data retrieved from LIFO */

	/*
	 * The LIFO is empty; wait for an item to be added to the LIFO
	 * from the task.
	 */

	TC_PRINT("Fiber waiting on an empty LIFO\n");
	nano_fiber_sem_give(&taskWaitSem);
	data = nano_fiber_lifo_get(&test_lifo, TICKS_UNLIMITED);
	if (data != &lifoItem[0]) {
		fiberDetectedFailure = 1;
		return -1;
	}

	nano_fiber_sem_take(&fiberWaitSem, TICKS_UNLIMITED);
	data = nano_fiber_lifo_get(&test_lifo, TICKS_UNLIMITED);
	if (data != &lifoItem[2]) {
		fiberDetectedFailure = 1;
		return -1;
	}

	/*
	 * Give the task some time to check the results.  Ideally, this would
	 * be waiting for a semaphore instead of a using a delay, but if the
	 * main task wakes the fiber before it blocks on the LIFO, the fiber
	 * will add the item to the LIFO too soon.  Obviously, a semaphore could
	 * not be given if the task is blocked on the LIFO; hence the delay.
	 */

	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_test(&timer, TICKS_UNLIMITED);

	/* The task is waiting on an empty LIFO.  Wake it up. */
	nano_fiber_lifo_put(&test_lifo, &lifoItem[3]);
	nano_fiber_lifo_put(&test_lifo, &lifoItem[2]);
	nano_fiber_lifo_put(&test_lifo, &lifoItem[1]);

	/*
	 * Wait for the task to check the results.  If the results pass, then the
	 * the task will wake the fiber.  If the results do not pass, then the
	 * fiber will wait forever.
	 */

	nano_fiber_sem_take(&fiberWaitSem, TICKS_UNLIMITED);

	return 0;
}

/**
 *
 * @brief Fiber portion of test that does not wait on a LIFO
 *
 * This routine works with fiberLifoNonWaitTest() to test the addition and
 * removal of items from a LIFO without having to wait.
 *
 * @return 0 on success, -1 on failure
 */

int fiberLifoNonWaitTest(void)
{
	void *data;    /* pointer to data retrieved from LIFO */

	/* The LIFO has two items in it; retrieve them both */

	data = nano_fiber_lifo_get(&test_lifo, TICKS_NONE);
	if (data != (void *) &lifoItem[3]) {
		goto errorReturn;
	}

	data = nano_fiber_lifo_get(&test_lifo, TICKS_NONE);
	if (data != (void *) &lifoItem[2]) {
		goto errorReturn;
	}

	/* LIFO should be empty--verify. */
	data = nano_fiber_lifo_get(&test_lifo, TICKS_NONE);
	if (data != NULL) {
		goto errorReturn;
	}

	/*
	 * The LIFO is now empty.  Add two items to the LIFO and then wait
	 * for the semaphore so that the task can retrieve them.
	 */

	TC_PRINT("Task to get LIFO items without waiting\n");
	nano_fiber_lifo_put(&test_lifo, &lifoItem[0]);
	nano_fiber_lifo_put(&test_lifo, &lifoItem[1]);
	nano_fiber_sem_give(&taskWaitSem);       /* Wake the task (if blocked) */

	/*
	 * Wait for the task to get the items and then trigger an ISR to populate
	 * the LIFO.
	 */

	nano_fiber_sem_take(&fiberWaitSem, TICKS_UNLIMITED);

	/*
	 * The task retrieved the two items from the LIFO and then triggered
	 * two interrupts to add two other items to the LIFO.  The fiber will
	 * now trigger two interrupts to read the two items.
	 */

	_trigger_nano_isr_lifo_get();
	if (isrLifoInfo.data != &lifoItem[1]) {
		goto errorReturn;
	}

	_trigger_nano_isr_lifo_get();
	if (isrLifoInfo.data != &lifoItem[3]) {
		goto errorReturn;
	}

	/* The LIFO should now be empty--verify */
	_trigger_nano_isr_lifo_get();
	if (isrLifoInfo.data != NULL) {
		goto errorReturn;
	}

	return 0;

errorReturn:
	fiberDetectedFailure = 1;
	return -1;
}

/**
 *
 * @brief Entry point for the fiber portion of the LIFO tests
 *
 * NOTE: The fiber portion of the tests have higher priority than the task
 * portion of the tests.
 *
 * @param arg1    unused
 * @param arg2    unused
 *
 * @return N/A
 */

static void fiberEntry(int arg1, int arg2)
{
	int  rv;      /* return value from a test */

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	rv = fiberLifoWaitTest();

	if (rv == 0) {
		fiberLifoNonWaitTest();
	}

}

/**
 *
 * @brief Task portion of test that waits on a LIFO
 *
 * This routine works with fiberLifoWaitTest() to test the addition and removal
 * of items to/from a LIFO.  The cases covered will have a fiber or task waiting
 * on an empty LIFO.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int taskLifoWaitTest(void)
{
	void *data;    /* ptr to data retrieved from LIFO */
	int i;

	/*
	 * the first item sent by the fiber is given directly to the waiting
	 * task, which then ceases waiting (but doesn't get to execute yet);
	 * the two remaining items then get queued internally by the LIFO,
	 * and are later retrieved by the task in LIFO order
	 */

	int expected_item[3] = { 3, 1, 2 };

	/* Wait on <taskWaitSem> in case fiber's print message blocked */
	nano_fiber_sem_take(&taskWaitSem, TICKS_UNLIMITED);

	/* The fiber is waiting on the LIFO.  Wake it. */
	nano_task_lifo_put(&test_lifo, &lifoItem[0]);

	/*
	 * The fiber ran, but is now blocked on the semaphore.  Add an item to the
	 * LIFO before giving the semaphore that wakes the fiber so that we can
	 * cover the path of nano_fiber_lifo_get(TICKS_UNLIMITED) not waiting on
	 * the LIFO.
	 */

	nano_task_lifo_put(&test_lifo, &lifoItem[2]);
	nano_task_sem_give(&fiberWaitSem);

	/* Check that the fiber got the correct item (lifoItem[0]) */

	if (fiberDetectedFailure) {
		TC_ERROR(" *** nano_task_lifo_put()/nano_fiber_lifo_get() failure\n");
		return TC_FAIL;
	}

	/* The LIFO is empty.  This time the task will wait for the 3 items. */

	TC_PRINT("Task waiting on an empty LIFO\n");
	for (i = 0; i < 3; i++) {
		data = nano_task_lifo_get(&test_lifo, TICKS_UNLIMITED);
		if (data != (void *) &lifoItem[expected_item[i]]) {
			TC_ERROR(" *** nano_task_lifo_get()/nano_fiber_lifo_put() failure\n");
			return TC_FAIL;
		}
	}

	/* Waiting on an empty LIFO passed for both fiber and task. */

	return TC_PASS;
}

/**
 *
 * @brief Task portion of test that does not wait on a LIFO
 *
 * This routine works with fiberLifoNonWaitTest() to test the addition and
 * removal of items from a LIFO without having to wait.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int taskLifoNonWaitTest(void)
{
	void *data;    /* ptr to data retrieved from LIFO */

	/*
	 * The fiber is presently waiting for <fiberWaitSem>.  Populate the LIFO
	 * before waking the fiber.
	 */

	TC_PRINT("Fiber to get LIFO items without waiting\n");
	nano_task_lifo_put(&test_lifo, &lifoItem[2]);
	nano_task_lifo_put(&test_lifo, &lifoItem[3]);
	nano_task_sem_give(&fiberWaitSem);    /* Wake the fiber */

	/* Check that fiber received the items correctly */
	if (fiberDetectedFailure) {
		TC_ERROR(" *** nano_task_lifo_put()/nano_fiber_lifo_get() failure\n");
		return TC_FAIL;
	}

	/* Wait for the fiber to be ready */
	nano_task_sem_take(&taskWaitSem, TICKS_UNLIMITED);

	data = nano_task_lifo_get(&test_lifo, TICKS_NONE);
	if (data != (void *) &lifoItem[1]) {
		TC_ERROR(" *** nano_task_lifo_get()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
	}

	data = nano_task_lifo_get(&test_lifo, TICKS_NONE);
	if (data != (void *) &lifoItem[0]) {
		TC_ERROR(" *** nano_task_lifo_get()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
	}

	data = nano_task_lifo_get(&test_lifo, TICKS_NONE);
	if (data != NULL) {
		TC_ERROR(" *** nano_task_lifo_get()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
	}

	/*
	 * Software interrupts have been configured so that when invoked,
	 * the ISR will add an item to the LIFO.  The fiber (when unblocked)
	 * trigger software interrupts to get the items from the LIFO from
	 * within an ISR.
	 *
	 * Populate the LIFO.
	 */

	TC_PRINT("ISR to get LIFO items without waiting\n");
	isrLifoInfo.data = &lifoItem[3];
	_trigger_nano_isr_lifo_put();
	isrLifoInfo.data = &lifoItem[1];
	_trigger_nano_isr_lifo_put();

	isrLifoInfo.data = NULL;    /* Force NULL to ensure [data] changes */

	nano_task_sem_give(&fiberWaitSem);    /* Wake the fiber */

	if (fiberDetectedFailure) {
		TC_ERROR(" *** nano_isr_lifo_put()/nano_isr_lifo_get() failure\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Initialize nanokernel objects
 *
 * This routine initializes the nanokernel objects used in the LIFO tests.
 *
 * @return N/A
 */

void initNanoObjects(void)
{
	nano_lifo_init(&test_lifo);   /* Initialize the LIFO */
	nano_sem_init(&taskWaitSem);   /* Initialize the task waiting semaphore */
	nano_sem_init(&fiberWaitSem);  /* Initialize the fiber waiting semaphore */
	nano_timer_init(&timer, timerData);

	nano_lifo_init(&multi_waiters);
	nano_sem_init(&reply_multi_waiters);

	TC_PRINT("Nano objects initialized\n");
}

/*
 * Multiple-waiters test
 *
 * NUM_WAITERS fibers pend on the multi_waiters LIFO, then the task puts data
 * on the LIFO NUM_WAITERS times. Each time, the first fiber in the queue wakes
 * up, is context-switched to, verifies the data is the one expected, and gives
 * the reply_multi_waiters semaphore, for a total of NUM_WAITERS times. The
 * task finally must be able to obtain the reply_multi_waiters semaphore
 * NUM_WAITERS times.
 */
#define NUM_WAITERS 3
static char __stack fiber_multi_waiters_stacks[NUM_WAITERS][FIBER_STACKSIZE];
static LIFO_ITEM multi_waiters_items[NUM_WAITERS] = {
	[0 ...(NUM_WAITERS-1)].link = 0,
	[0 ...(NUM_WAITERS-1)].data = 0xabad1dea,
};

/**
 *
 * @brief Fiber entry point for multiple-waiters test
 *
 * @return N/A
 */

static void fiber_multi_waiters(int arg1, int arg2)
{
	void *item;

	TC_PRINT("multiple-waiter fiber %d receiving item...\n", arg1);
	item = nano_fiber_lifo_get(&multi_waiters, TICKS_UNLIMITED);
	if (item != &multi_waiters_items[arg1]) {
		TC_ERROR(" *** fiber %d did not receive correct item\n", arg1);
		TC_ERROR(" *** received %p instead of %p.\n",
				 item, &multi_waiters_items[arg1]);

		/* do NOT give the semaphore, signifying an error */
		return;
	}
	TC_PRINT("multiple-waiter fiber %d got correct item, giving semaphore\n",
			 arg1);
	nano_fiber_sem_give(&reply_multi_waiters);
}

/**
 *
 * @brief Task part of multiple-waiter test, repeatable
 *
 * @return N/A
 */

static int do_test_multiple_waiters(void)
{
	int ii;

	/* pend all fibers one the same lifo */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		task_fiber_start(fiber_multi_waiters_stacks[ii], FIBER_STACKSIZE,
						 fiber_multi_waiters, ii, 0, FIBER_PRIORITY, 0);
	}

	/* wake up all the fibers: the task is preempted each time */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		nano_task_lifo_put(&multi_waiters, &multi_waiters_items[ii]);
	}

	/* reply_multi_waiters will have been given once for each fiber */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		if (!nano_task_sem_take(&reply_multi_waiters, TICKS_NONE)) {
			TC_ERROR(" *** Cannot take sem supposedly given by waiters.\n");
			return TC_FAIL;
		}
	}

	TC_PRINT("Task took multi-waiter reply semaphore %d times, as expected.\n",
			 NUM_WAITERS);

	if (nano_task_lifo_get(&multi_waiters, TICKS_NONE)) {
		TC_ERROR(" *** multi_waiters should have been empty.\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Entry point for multiple-waiters test
 *
 * @return N/A
 */

static int test_multiple_waiters(void)
{
	TC_PRINT("First pass\n");
	if (do_test_multiple_waiters() == TC_FAIL) {
		TC_ERROR(" *** First pass test failed.\n");
		return TC_FAIL;
	}

	/*
	 * Verity a wait q that has been emptied has been reset correctly, so
	 * redo the test. This time, send one message before starting the fibers.
	 */

	TC_PRINT("Second pass\n");
	if (do_test_multiple_waiters() == TC_FAIL) {
		TC_ERROR(" *** Second pass test failed.\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/* timeout tests
 *
 * Test the nano_xxx_lifo_wait_timeout() APIs.
 *
 * First, the task waits with a timeout and times out. Then it wait with a
 * timeout, but gets the data in time.
 *
 * Then, multiple timeout tests are done for the fibers, to test the ordering
 * of queueing/dequeueing when timeout occurs, first on one lifo, then on
 * multiple lifos.
 *
 * Finally, multiple fibers pend on one lifo, and they all get the
 * data in time, except the last one: this tests that the timeout is
 * recomputed correctly when timeouts are aborted.
 */

#include <tc_nano_timeout_common.h>

struct scratch_q_packet {
	void *link_in_q;
	void *data_if_needed;
};

struct reply_packet {
	void *link_in_fifo;
	int reply;
};

#define NUM_SCRATCH_Q_PACKETS 20
struct scratch_q_packet scratch_q_packets[NUM_SCRATCH_Q_PACKETS];

struct nano_fifo scratch_q_packets_fifo;

void *get_scratch_packet(void)
{
	void *packet = nano_fifo_get(&scratch_q_packets_fifo, TICKS_NONE);

	__ASSERT_NO_MSG(packet);

	return packet;
}

void put_scratch_packet(void *packet)
{
	nano_fifo_put(&scratch_q_packets_fifo, packet);
}

static struct nano_lifo lifo_timeout[2];
struct nano_fifo timeout_order_fifo;

struct timeout_order_data {
	void *link_in_lifo;
	struct nano_lifo *lifo;
	int32_t timeout;
	int timeout_order;
	int q_order;
};

struct timeout_order_data timeout_order_data[] = {
	{0, &lifo_timeout[0], TIMEOUT(2), 2, 0},
	{0, &lifo_timeout[0], TIMEOUT(4), 4, 1},
	{0, &lifo_timeout[0], TIMEOUT(0), 0, 2},
	{0, &lifo_timeout[0], TIMEOUT(1), 1, 3},
	{0, &lifo_timeout[0], TIMEOUT(3), 3, 4},
};

struct timeout_order_data timeout_order_data_mult_lifo[] = {
	{0, &lifo_timeout[1], TIMEOUT(0), 0, 0},
	{0, &lifo_timeout[0], TIMEOUT(3), 3, 1},
	{0, &lifo_timeout[0], TIMEOUT(5), 5, 2},
	{0, &lifo_timeout[1], TIMEOUT(8), 8, 3},
	{0, &lifo_timeout[1], TIMEOUT(7), 7, 4},
	{0, &lifo_timeout[0], TIMEOUT(1), 1, 5},
	{0, &lifo_timeout[0], TIMEOUT(6), 6, 6},
	{0, &lifo_timeout[0], TIMEOUT(2), 2, 7},
	{0, &lifo_timeout[1], TIMEOUT(4), 4, 8},
};

#define TIMEOUT_ORDER_NUM_FIBERS ARRAY_SIZE(timeout_order_data_mult_lifo)
static char __stack timeout_stacks[TIMEOUT_ORDER_NUM_FIBERS][FIBER_STACKSIZE];

/* a fiber sleeps then puts data on the lifo */
static void test_fiber_put_timeout(int lifo, int timeout)
{
	fiber_sleep((int32_t)timeout);
	nano_fiber_lifo_put((struct nano_lifo *)lifo, get_scratch_packet());
}

/* a fiber pends on a lifo then times out */
static void test_fiber_pend_and_timeout(int data, int unused)
{
	struct timeout_order_data *d = (void *)data;
	int32_t orig_ticks = sys_tick_get();
	void *packet;

	ARG_UNUSED(unused);

	packet = nano_fiber_lifo_get(d->lifo, d->timeout);
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

/* the task spins several fibers that pend and timeout on lifos */
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
			TC_PRINT(" got fiber (q order: %d, t/o: %d, lifo %p) as expected\n",
						data->q_order, data->timeout, data->lifo);
		} else {
			TC_ERROR(" *** fiber %d woke up, expected %d\n",
						data->timeout_order, ii);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/* a fiber pends on a lifo with a timeout and gets the data in time */
static void test_fiber_pend_and_get_data(int data, int unused)
{
	struct timeout_order_data *d = (void *)data;
	void *packet;

	ARG_UNUSED(unused);

	packet = nano_fiber_lifo_get(d->lifo, d->timeout);
	if (!packet) {
		TC_PRINT(" *** fiber (q order: %d, t/o: %d, lifo %p) timed out!\n",
						d->q_order, d->timeout, d->lifo);
		return;
	}

	put_scratch_packet(packet);
	nano_fiber_fifo_put(&timeout_order_fifo, d);
}

/* the task spins fibers that get lifo data in time, except the last one */
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

		nano_task_lifo_put(test_data[ii].lifo, get_scratch_packet());

		data = nano_task_fifo_get(&timeout_order_fifo, TICKS_UNLIMITED);

		if (data->q_order == ii) {
			TC_PRINT(" got fiber (q order: %d, t/o: %d, lifo %p) as expected\n",
						data->q_order, data->timeout, data->lifo);
		} else {
			TC_ERROR(" *** fiber %d woke up, expected %d\n",
						data->q_order, ii);
			return TC_FAIL;
		}
	}

	data = nano_task_fifo_get(&timeout_order_fifo, TICKS_UNLIMITED);
	if (data->q_order == ii) {
		TC_PRINT(" got fiber (q order: %d, t/o: %d, lifo %p) as expected\n",
					data->q_order, data->timeout, data->lifo);
	} else {
		TC_ERROR(" *** fiber %d woke up, expected %d\n",
					data->timeout_order, ii);
		return TC_FAIL;
	}

	return TC_PASS;
}

/* try getting data on lifo with special timeout value, return result in fifo */
static void test_fiber_ticks_special_values(int packet, int special_value)
{
	struct reply_packet *reply_packet = (void *)packet;

	reply_packet->reply =
		!!nano_fiber_lifo_get(&lifo_timeout[0], special_value);

	nano_fiber_fifo_put(&timeout_order_fifo, reply_packet);
}

/* the timeout test entry point */
static int test_timeout(void)
{
	int64_t orig_ticks;
	int32_t timeout;
	int rv;
	void *packet, *scratch_packet;
	int test_data_size;
	int ii;
	struct reply_packet reply_packet;

	nano_lifo_init(&lifo_timeout[0]);
	nano_lifo_init(&lifo_timeout[1]);
	nano_fifo_init(&timeout_order_fifo);
	nano_fifo_init(&scratch_q_packets_fifo);

	for (ii = 0; ii < NUM_SCRATCH_Q_PACKETS; ii++) {
		scratch_q_packets[ii].data_if_needed = (void *)ii;
		nano_task_fifo_put(&scratch_q_packets_fifo,
							&scratch_q_packets[ii]);
	}

	/* test nano_task_lifo_get() with timeout */
	timeout = 10;
	orig_ticks = sys_tick_get();
	packet = nano_task_lifo_get(&lifo_timeout[0], timeout);
	if (packet) {
		TC_ERROR(" *** timeout of %d did not time out.\n", timeout);
		return TC_FAIL;
	}
	if ((sys_tick_get() - orig_ticks) < timeout) {
		TC_ERROR(" *** task did not wait long enough on timeout of %d.\n",
					timeout);
		return TC_FAIL;
	}

	/* test nano_task_lifo_get() with timeout of 0 */

	packet = nano_task_lifo_get(&lifo_timeout[0], 0);
	if (packet) {
		TC_ERROR(" *** timeout of 0 did not time out.\n");
		return TC_FAIL;
	}

	/* test nano_task_lifo_get() with timeout > 0 */

	TC_PRINT("test nano_task_lifo_get() with timeout > 0\n");

	timeout = 3;
	orig_ticks = sys_tick_get();

	packet = nano_task_lifo_get(&lifo_timeout[0], timeout);

	if (packet) {
		TC_ERROR(" *** timeout of %d did not time out.\n",
				timeout);
		return TC_FAIL;
	}

	if (!is_timeout_in_range(orig_ticks, timeout)) {
		return TC_FAIL;
	}

	TC_PRINT("nano_task_lifo_get() timed out as expected\n");

	/*
	 * test nano_task_lifo_get() with a timeout and fiber that puts
	 * data on the lifo on time
	 */

	timeout = 5;
	orig_ticks = sys_tick_get();

	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_put_timeout, (int)&lifo_timeout[0],
						timeout,
						FIBER_PRIORITY, 0);

	packet = nano_task_lifo_get(&lifo_timeout[0],
												(int)(timeout + 5));
	if (!packet) {
		TC_ERROR(" *** data put in time did not return valid pointer.\n");
		return TC_FAIL;
	}

	put_scratch_packet(packet);

	if (!is_timeout_in_range(orig_ticks, timeout)) {
		return TC_FAIL;
	}

	TC_PRINT("nano_task_lifo_get() got lifo in time, as expected\n");

	/*
	 * test nano_task_lifo_get() with TICKS_NONE and no data
	 * unavailable.
	 */

	if (nano_task_lifo_get(&lifo_timeout[0], TICKS_NONE)) {
		TC_ERROR("task with TICKS_NONE got data, but shouldn't have\n");
		return TC_FAIL;
	}

	TC_PRINT("task with TICKS_NONE did not get data, as expected\n");

	/*
	 * test nano_task_lifo_get() with TICKS_NONE and some data
	 * available.
	 */

	scratch_packet = get_scratch_packet();
	nano_task_lifo_put(&lifo_timeout[0], scratch_packet);
	if (!nano_task_lifo_get(&lifo_timeout[0], TICKS_NONE)) {
		TC_ERROR("task with TICKS_NONE did not get available data\n");
		return TC_FAIL;
	}
	put_scratch_packet(scratch_packet);

	TC_PRINT("task with TICKS_NONE got available data, as expected\n");

	/*
	 * test nano_task_lifo_get() with TICKS_UNLIMITED and the
	 * data available.
	 */

	TC_PRINT("Trying to take available data with TICKS_UNLIMITED:\n"
			 " will hang the test if it fails.\n");

	scratch_packet = get_scratch_packet();
	nano_task_lifo_put(&lifo_timeout[0], scratch_packet);
	if (!nano_task_lifo_get(&lifo_timeout[0], TICKS_UNLIMITED)) {
		TC_ERROR(" *** This will never be hit!!! .\n");
		return TC_FAIL;
	}
	put_scratch_packet(scratch_packet);

	TC_PRINT("task with TICKS_UNLIMITED got available data, as expected\n");

	/* test fiber with timeout of TICKS_NONE not getting data on empty lifo */

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
	nano_task_lifo_put(&lifo_timeout[0], scratch_packet);
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
	nano_task_lifo_put(&lifo_timeout[0], scratch_packet);
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

	/* test multiple fibers pending on the same lifo with different timeouts */

	test_data_size = ARRAY_SIZE(timeout_order_data);

	TC_PRINT("testing timeouts of %d fibers on same lifo\n", test_data_size);

	rv = test_multiple_fibers_pending(timeout_order_data, test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not time out in the right order\n");
		return TC_FAIL;
	}

	/* test mult. fibers pending on different lifos with different timeouts */

	test_data_size = ARRAY_SIZE(timeout_order_data_mult_lifo);

	TC_PRINT("testing timeouts of %d fibers on different lifos\n",
				test_data_size);

	rv = test_multiple_fibers_pending(timeout_order_data_mult_lifo,
										test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not time out in the right order\n");
		return TC_FAIL;
	}

	/*
	 * test multiple fibers pending on same lifo with different timeouts, but
	 * getting the data in time, except the last one.
	 */

	test_data_size = ARRAY_SIZE(timeout_order_data);

	TC_PRINT("testing %d fibers timing out, but obtaining the data in time\n"
				"(except the last one, which times out)\n",
				test_data_size);

	rv = test_multiple_fibers_get_data(timeout_order_data, test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not get the data in the right order\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Entry point to LIFO tests
 *
 * This is the entry point to the LIFO tests.
 *
 * @return N/A
 */

void main(void)
{
	int     rv;       /* return value from tests */

	TC_START("Test Nanokernel LIFO");

	initNanoObjects();

	/*
	 * Start the fiber.  The fiber will be given a higher priority than the
	 * main task.
	 */

	task_fiber_start(fiberStack, FIBER_STACKSIZE, fiberEntry,
		0, 0, FIBER_PRIORITY, 0);

	rv = taskLifoWaitTest();

	if (rv == TC_PASS) {
		rv = taskLifoNonWaitTest();
	}

	if (rv == TC_PASS) {
		rv = test_multiple_waiters();
	}

	/* test timeouts */
	if (rv == TC_PASS) {
		rv = test_timeout();
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
