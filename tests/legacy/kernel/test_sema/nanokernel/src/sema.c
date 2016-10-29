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
 * @brief Test nanokernel semaphore APIs
 *
 * This module tests four basic scenarios with the usage of the following
 * semaphore routines:
 *
 * nano_sem_init
 * nano_fiber_sem_give, nano_fiber_sem_take
 * nano_task_sem_give, nano_task_sem_take
 * nano_isr_sem_give, nano_isr_sem_take
 *
 * Scenario #1:
 * A task, fiber or ISR does not wait for the semaphore when taking it.
 *
 * Scenario #2:
 * A task or fiber must wait for the semaphore to be given before it gets it.
 *
 * Scenario #3:
 * Multiple fibers pend on the same semaphore.
 *
 * Scenario #4:
 * Timeout scenarios with multiple semaphores and fibers.
 */

#include <tc_util.h>
#include <arch/cpu.h>
#include <misc/util.h>
#include <irq_offload.h>

#include <util_test_common.h>

#define FIBER_STACKSIZE    384
#define FIBER_PRIORITY     4

typedef struct {
	struct nano_sem *sem;    /* ptr to semaphore */
	int              data;   /* data */
} ISR_SEM_INFO;

typedef enum {
	STS_INIT = -1,
	STS_TASK_WOKE_FIBER,
	STS_FIBER_WOKE_TASK,
	STS_ISR_WOKE_TASK
} SEM_TEST_STATE;

static SEM_TEST_STATE  semTestState;
static ISR_SEM_INFO    isrSemInfo;
static struct nano_sem        testSem;
static int             fiberDetectedFailure = 0;

static struct nano_timer     timer;
static void *timerData[1];

static char __stack fiberStack[FIBER_STACKSIZE];

static struct nano_sem multi_waiters;
static struct nano_sem reply_multi_waiters;

/**
 *
 * @brief Take a semaphore
 *
 * This routine is the ISR handler for _trigger_nano_isr_sem_take().  It takes a
 * semaphore within the context of an ISR.
 *
 * @param data    pointer to ISR handler parameter
 *
 * @return N/A
 */

static void my_isr_sem_take(void *data)
{
	ISR_SEM_INFO *pInfo = (ISR_SEM_INFO *) data;

	pInfo->data = nano_isr_sem_take(pInfo->sem, TICKS_NONE);
}

static void _trigger_nano_isr_sem_take(void)
{
	irq_offload(my_isr_sem_take, &isrSemInfo);
}

/**
 *
 * @brief Give a semaphore
 *
 * This routine is the ISR handler for _trigger_nano_isr_sem_take().  It gives a
 * semaphore within the context of an ISR.
 *
 * @param data    pointer to ISR handler parameter
 *
 * @return N/A
 */

static void my_isr_sem_give(void *data)
{
	ISR_SEM_INFO *pInfo = (ISR_SEM_INFO *) data;

	nano_isr_sem_give(pInfo->sem);
	pInfo->data = 1;     /* Indicate semaphore has been given */
}

static void _trigger_nano_isr_sem_give(void)
{
	irq_offload(my_isr_sem_give, &isrSemInfo);
}

/**
 *
 * @brief Give and take the semaphore in a fiber without blocking
 *
 * This test gives and takes the test semaphore in a fiber
 * without blocking on the semaphore.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int testSemFiberNoWait(void)
{
	int  i;

	TC_PRINT("Giving and taking a semaphore in a fiber (non-blocking)\n");

	/*
	 * Give the semaphore many times and then make sure that it can only be
	 * taken that many times.
	 */

	for (i = 0; i < 32; i++) {
		nano_fiber_sem_give(&testSem);
	}

	for (i = 0; i < 32; i++) {
		if (nano_fiber_sem_take(&testSem, TICKS_NONE) != 1) {
			TC_ERROR(" *** Expected nano_fiber_sem_take() to succeed, not fail\n");
			goto errorReturn;
		}
	}

	if (nano_fiber_sem_take(&testSem, TICKS_NONE) != 0) {
		TC_ERROR(" *** Expected  nano_fiber_sem_take() to fail, not succeed\n");
		goto errorReturn;
	}

	return TC_PASS;

errorReturn:
	fiberDetectedFailure = 1;
	return TC_FAIL;
}

/**
 *
 * @brief Entry point for the fiber portion of the semaphore tests
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

	rv = testSemFiberNoWait();
	if (rv != TC_PASS) {
		return;
	}

	/*
	 * At this point <testSem> is not available.  Wait for <testSem> to become
	 * available (the main task will give it).
	 */

	nano_fiber_sem_take(&testSem, TICKS_UNLIMITED);

	semTestState = STS_TASK_WOKE_FIBER;

	/*
	 * Delay for two seconds.  This gives the main task time to print
	 * any messages (very important if I/O link is slow!), and wait
	 * on <testSem>.  Once the delay is done, this fiber will give <testSem>
	 * thus waking the main task.
	 */

	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_test(&timer, TICKS_UNLIMITED);

	/*
	 * The main task is now waiting on <testSem>.  Give the semaphore <testSem>
	 * to wake it.
	 */

	nano_fiber_sem_give(&testSem);

	/*
	 * Some small delay must be done so that the main task can process the
	 * semaphore signal.
	 */

	semTestState = STS_FIBER_WOKE_TASK;

	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_test(&timer, TICKS_UNLIMITED);

	/*
	 * The main task should be waiting on <testSem> again.  This time, instead
	 * of giving the semaphore from the semaphore, give it from an ISR to wake
	 * the main task.
	 */

	isrSemInfo.data = 0;
	isrSemInfo.sem = &testSem;
	_trigger_nano_isr_sem_give();

	if (isrSemInfo.data == 1) {
		semTestState = STS_ISR_WOKE_TASK;
	}
}

/**
 *
 * @brief Initialize nanokernel objects
 *
 * This routine initializes the nanokernel objects used in the semaphore tests.
 *
 * @return N/A
 */

void initNanoObjects(void)
{
	nano_sem_init(&testSem);
	nano_sem_init(&multi_waiters);
	nano_sem_init(&reply_multi_waiters);
	nano_timer_init(&timer, timerData);

	TC_PRINT("Nano objects initialized\n");
}

/**
 *
 * @brief Give and take the semaphore in an ISR without blocking
 *
 * This test gives and takes the test semaphore in the context of an ISR without
 * blocking on the semaphore.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int testSemIsrNoWait(void)
{
	int  i;

	TC_PRINT("Giving and taking a semaphore in an ISR (non-blocking)\n");

	/*
	 * Give the semaphore many times and then make sure that it can only be
	 * taken that many times.
	 */

	isrSemInfo.sem = &testSem;
	for (i = 0; i < 32; i++) {
		_trigger_nano_isr_sem_give();
	}

	for (i = 0; i < 32; i++) {
		isrSemInfo.data = 0;
		_trigger_nano_isr_sem_take();
		if (isrSemInfo.data != 1) {
			TC_ERROR(" *** Expected nano_isr_sem_take() to succeed, not fail\n");
			goto errorReturn;
		}
	}

	_trigger_nano_isr_sem_take();
	if (isrSemInfo.data != 0) {
		TC_ERROR(" *** Expected  nano_isr_sem_take() to fail, not succeed!\n");
		goto errorReturn;
	}

	return TC_PASS;

errorReturn:
	return TC_FAIL;
}

/**
 *
 * @brief Give and take the semaphore in a task without blocking
 *
 * This test gives and takes the test semaphore in a task without
 * blocking on the semaphore.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int testSemTaskNoWait(void)
{
	int  i;     /* loop counter */

	TC_PRINT("Giving and taking a semaphore in a task (non-blocking)\n");

	/*
	 * Give the semaphore many times and then make sure that it can only be
	 * taken that many times.
	 */

	for (i = 0; i < 32; i++) {
		nano_task_sem_give(&testSem);
	}

	for (i = 0; i < 32; i++) {
		if (nano_task_sem_take(&testSem, TICKS_NONE) != 1) {
			TC_ERROR(" *** Expected nano_task_sem_take() to succeed, not fail\n");
			goto errorReturn;
		}
	}

	if (nano_task_sem_take(&testSem, TICKS_NONE) != 0) {
		TC_ERROR(" *** Expected  nano_task_sem_take() to fail, not succeed!\n");
		goto errorReturn;
	}

	return TC_PASS;

errorReturn:
	return TC_FAIL;
}

/**
 *
 * @brief Perform tests that wait on a semaphore
 *
 * This routine works with fiberEntry() to perform the tests that wait on
 * a semaphore.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int testSemWait(void)
{
	if (fiberDetectedFailure != 0) {
		TC_ERROR(" *** Failure detected in the fiber.");
		return TC_FAIL;
	}

	nano_task_sem_give(&testSem);    /* Wake the fiber. */

	if (semTestState != STS_TASK_WOKE_FIBER) {
		TC_ERROR(" *** Expected task to wake fiber.  It did not.\n");
		return TC_FAIL;
	}

	TC_PRINT("Semaphore from the task woke the fiber\n");

	nano_task_sem_take(&testSem, TICKS_UNLIMITED);   /* Wait on <testSem> */

	if (semTestState != STS_FIBER_WOKE_TASK) {
		TC_ERROR(" *** Expected fiber to wake task.  It did not.\n");
		return TC_FAIL;
	}

	TC_PRINT("Semaphore from the fiber woke the task\n");

	nano_task_sem_take(&testSem, TICKS_UNLIMITED);  /* Wait on <testSem> again. */

	if (semTestState != STS_ISR_WOKE_TASK) {
		TC_ERROR(" *** Expected ISR to wake task.  It did not.\n");
		return TC_FAIL;
	}

	TC_PRINT("Semaphore from the ISR woke the task.\n");
	return TC_PASS;
}

/*
 * Multiple-waiters test
 *
 * NUM_WAITERS fibers pend on the multi_waiters semaphore, then the task give
 * the semaphore NUM_WAITERS times. Each time, the first fiber in the queue
 * wakes up, is context-switched to, and gives the reply_multi_waiters
 * semaphore, for a total of NUM_WAITERS times. The task finally must be able
 * to obtain the reply_multi_waiters semaphore NUM_WAITERS times.
 */
#define NUM_WAITERS 3
static char __stack fiber_multi_waiters_stacks[NUM_WAITERS][FIBER_STACKSIZE];

/**
 *
 * @brief Fiber entry point for multiple-waiters test
 *
 * @return N/A
 */

static void fiber_multi_waiters(int arg1, int arg2)
{
	TC_PRINT("multiple-waiter fiber %d trying to get semaphore...\n", arg1);
	nano_fiber_sem_take(&multi_waiters, TICKS_UNLIMITED);
	TC_PRINT("multiple-waiter fiber %d acquired semaphore, sending reply\n",
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

	/* pend all fibers one the same semaphore */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		task_fiber_start(fiber_multi_waiters_stacks[ii], FIBER_STACKSIZE,
							fiber_multi_waiters, ii, 0, FIBER_PRIORITY, 0);
	}

	/* wake up all the fibers: the task is preempted each time */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		nano_task_sem_give(&multi_waiters);
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

	if (nano_task_sem_take(&multi_waiters, TICKS_NONE)) {
		TC_ERROR(" *** multi_waiters should have been empty.\n");
		return TC_FAIL;
	}

	if (nano_task_sem_take(&reply_multi_waiters, TICKS_NONE)) {
		TC_ERROR(" *** reply_multi_waiters should have been empty.\n");
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
	 * Verify a wait q that has been emptied has been reset correctly, so
	 * redo the test.
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
 * Test the nano_xxx_sem_wait_timeout() APIs.
 *
 * First, the task waits with a timeout and times out. Then it wait with a
 * timeout, but gets the semaphore in time.
 *
 * Then, multiple timeout tests are done for the fibers, to test the ordering
 * of queueing/dequeueing when timeout occurs, first on one semaphore, then on
 * multiple semaphores.
 *
 * Finally, multiple fibers pend on one semaphore, and they all get the
 * semaphore in time, except the last one: this tests that the timeout is
 * recomputed correctly when timeouts are aborted.
 */

#include <tc_nano_timeout_common.h>

static struct nano_sem sem_timeout[2];
struct nano_fifo timeout_order_fifo;

struct reply_packet {
	void *link_in_fifo;
	int reply;
};

struct timeout_order_data {
	void *link_in_fifo;
	struct nano_sem *sem;
	int32_t timeout;
	int timeout_order;
	int q_order;
};

struct timeout_order_data timeout_order_data[] = {
	{0, &sem_timeout[0], TIMEOUT(2), 2, 0},
	{0, &sem_timeout[0], TIMEOUT(4), 4, 1},
	{0, &sem_timeout[0], TIMEOUT(0), 0, 2},
	{0, &sem_timeout[0], TIMEOUT(1), 1, 3},
	{0, &sem_timeout[0], TIMEOUT(3), 3, 4},
};

struct timeout_order_data timeout_order_data_mult_sem[] = {
	{0, &sem_timeout[1], TIMEOUT(0), 0, 0},
	{0, &sem_timeout[0], TIMEOUT(3), 3, 1},
	{0, &sem_timeout[0], TIMEOUT(5), 5, 2},
	{0, &sem_timeout[1], TIMEOUT(8), 8, 3},
	{0, &sem_timeout[1], TIMEOUT(7), 7, 4},
	{0, &sem_timeout[0], TIMEOUT(1), 1, 5},
	{0, &sem_timeout[0], TIMEOUT(6), 6, 6},
	{0, &sem_timeout[0], TIMEOUT(2), 2, 7},
	{0, &sem_timeout[1], TIMEOUT(4), 4, 8},
};

#define TIMEOUT_ORDER_NUM_FIBERS ARRAY_SIZE(timeout_order_data_mult_sem)
static char __stack timeout_stacks[TIMEOUT_ORDER_NUM_FIBERS][FIBER_STACKSIZE];

/* a fiber sleeps then gives a semaphore */
static void test_fiber_give_timeout(int sem, int timeout)
{
	fiber_sleep((int32_t)timeout);
	nano_fiber_sem_give((struct nano_sem *)sem);
}

/* a fiber pends on a semaphore then times out */
static void test_fiber_pend_and_timeout(int data, int unused)
{
	struct timeout_order_data *the_data = (void *)data;
	int32_t orig_ticks = sys_tick_get();
	int rv;

	ARG_UNUSED(unused);

	rv = nano_fiber_sem_take(the_data->sem, the_data->timeout);
	if (rv) {
		TC_ERROR(" *** timeout of %d did not time out.\n",
					the_data->timeout);
		return;
	}
	if (!is_timeout_in_range(orig_ticks, the_data->timeout)) {
		return;
	}

	nano_fiber_fifo_put(&timeout_order_fifo, the_data);
}

/* the task spins several fibers that pend and timeout on sempahores */
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
			TC_PRINT(" got fiber (q order: %d, t/o: %d, sem: %p) as expected\n",
						data->q_order, data->timeout, data->sem);
		} else {
			TC_ERROR(" *** fiber %d woke up, expected %d\n",
						data->timeout_order, ii);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/* a fiber pends on a semaphore with a timeout and gets the semaphore in time */
static void test_fiber_pend_and_get_sem(int data, int unused)
{
	struct timeout_order_data *the_data = (void *)data;
	int rv;

	ARG_UNUSED(unused);

	rv = nano_fiber_sem_take(the_data->sem, the_data->timeout);
	if (!rv) {
		TC_PRINT(" *** fiber (q order: %d, t/o: %d, sem: %p) timed out!\n",
						the_data->q_order, the_data->timeout, the_data->sem);
		return;
	}

	nano_fiber_fifo_put(&timeout_order_fifo, the_data);
}

/* the task spins fibers that get the sem in time, except the last one */
static int test_multiple_fibers_get_sem(struct timeout_order_data *test_data,
										int test_data_size)
{
	struct timeout_order_data *data;
	int ii;

	for (ii = 0; ii < test_data_size-1; ii++) {
		task_fiber_start(timeout_stacks[ii], FIBER_STACKSIZE,
							test_fiber_pend_and_get_sem,
							(int)&test_data[ii], 0,
							FIBER_PRIORITY, 0);
	}
	task_fiber_start(timeout_stacks[ii], FIBER_STACKSIZE,
						test_fiber_pend_and_timeout,
						(int)&test_data[ii], 0,
						FIBER_PRIORITY, 0);

	for (ii = 0; ii < test_data_size-1; ii++) {
		nano_task_sem_give(test_data[ii].sem);

		data = nano_task_fifo_get(&timeout_order_fifo, TICKS_UNLIMITED);

		if (data->q_order == ii) {
			TC_PRINT(" got fiber (q order: %d, t/o: %d, sem: %p) as expected\n",
						data->q_order, data->timeout, data->sem);
		} else {
			TC_ERROR(" *** fiber %d woke up, expected %d\n",
						data->q_order, ii);
			return TC_FAIL;
		}
	}

	data = nano_task_fifo_get(&timeout_order_fifo, TICKS_UNLIMITED);
	if (data->q_order == ii) {
		TC_PRINT(" got fiber (q order: %d, t/o: %d, sem: %p) as expected\n",
					data->q_order, data->timeout, data->sem);
	} else {
		TC_ERROR(" *** fiber %d woke up, expected %d\n",
					data->timeout_order, ii);
		return TC_FAIL;
	}

	return TC_PASS;
}

static void test_fiber_ticks_special_values(int packet, int special_value)
{
	struct reply_packet *reply_packet = (void *)packet;

	reply_packet->reply =
		nano_fiber_sem_take(&sem_timeout[0], special_value);
	nano_fiber_fifo_put(&timeout_order_fifo, reply_packet);
}

/* the timeout test entry point */
static int test_timeout(void)
{
	int64_t orig_ticks;
	int32_t timeout;
	int rv;
	int test_data_size;
	struct reply_packet reply_packet;

	nano_sem_init(&sem_timeout[0]);
	nano_sem_init(&sem_timeout[1]);
	nano_fifo_init(&timeout_order_fifo);

	/* test nano_task_sem_take() with timeout */
	timeout = 10;
	orig_ticks = sys_tick_get();
	rv = nano_task_sem_take(&sem_timeout[0], timeout);
	if (rv) {
		TC_ERROR(" *** timeout of %d did not time out.\n", timeout);
		return TC_FAIL;
	}
	if ((sys_tick_get() - orig_ticks) < timeout) {
		TC_ERROR(" *** task did not wait long enough on timeout of %d.\n",
					timeout);
		return TC_FAIL;
	}

	/* test nano_task_sem_take() with timeout of 0 */

	rv = nano_task_sem_take(&sem_timeout[0], 0);
	if (rv) {
		TC_ERROR(" *** timeout of 0 did not time out.\n");
		return TC_FAIL;
	}

	/* test nano_task_sem_take() with timeout > 0 */

	TC_PRINT("test nano_task_sem_take() with timeout > 0\n");

	timeout = 3;
	orig_ticks = sys_tick_get();

	rv = nano_task_sem_take(&sem_timeout[0], timeout);

	if (rv) {
		TC_ERROR(" *** timeout of %d did not time out.\n",
				timeout);
		return TC_FAIL;
	}

	if (!is_timeout_in_range(orig_ticks, timeout)) {
		return TC_FAIL;
	}

	TC_PRINT("nano_task_sem_take() timed out as expected\n");

	/*
	 * test nano_task_sem_take() with a timeout and fiber that gives
	 * the semaphore on time
	 */

	timeout = 5;
	orig_ticks = sys_tick_get();

	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_give_timeout, (int)&sem_timeout[0],
						timeout,
						FIBER_PRIORITY, 0);

	rv = nano_task_sem_take(&sem_timeout[0], (int)(timeout + 5));
	if (!rv) {
		TC_ERROR(" *** timed out even if semaphore was given in time.\n");
		return TC_FAIL;
	}

	if (!is_timeout_in_range(orig_ticks, timeout)) {
		return TC_FAIL;
	}

	TC_PRINT("nano_task_sem_take() got sem in time, as expected\n");

	/*
	 * test nano_task_sem_take() with TICKS_NONE and the
	 * semaphore unavailable.
	 */

	if (nano_task_sem_take(&sem_timeout[0], TICKS_NONE)) {
		TC_ERROR("task with TICKS_NONE got sem, but shouldn't have\n");
		return TC_FAIL;
	}

	TC_PRINT("task with TICKS_NONE did not get sem, as expected\n");

	/*
	 * test nano_task_sem_take() with TICKS_NONE and the
	 * semaphore available.
	 */

	nano_task_sem_give(&sem_timeout[0]);
	if (!nano_task_sem_take(&sem_timeout[0], TICKS_NONE)) {
		TC_ERROR("task with TICKS_NONE did not get available sem\n");
		return TC_FAIL;
	}

	TC_PRINT("task with TICKS_NONE got available sem, as expected\n");

	/*
	 * test nano_task_sem_take() with TICKS_UNLIMITED and the
	 * semaphore available.
	 */

	TC_PRINT("Trying to take available sem with TICKS_UNLIMITED:\n"
			 " will hang the test if it fails.\n");

	nano_task_sem_give(&sem_timeout[0]);
	if (!nano_task_sem_take(&sem_timeout[0], TICKS_UNLIMITED)) {
		TC_ERROR(" *** This will never be hit!!! .\n");
		return TC_FAIL;
	}

	TC_PRINT("task with TICKS_UNLIMITED got available sem, as expected\n");

	/* test fiber with timeout of TICKS_NONE not getting empty semaphore */

	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_ticks_special_values,
						(int)&reply_packet, TICKS_NONE, FIBER_PRIORITY, 0);

	if (!nano_task_fifo_get(&timeout_order_fifo, TICKS_NONE)) {
		TC_ERROR(" *** fiber should have run and filled the fifo.\n");
		return TC_FAIL;
	}

	if (reply_packet.reply != 0) {
		TC_ERROR(" *** fiber should not have obtained the semaphore.\n");
		return TC_FAIL;
	}

	TC_PRINT("fiber with TICKS_NONE did not get sem, as expected\n");

	/* test fiber with timeout of TICKS_NONE getting full semaphore */

	nano_task_sem_give(&sem_timeout[0]);

	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_ticks_special_values,
						(int)&reply_packet, TICKS_NONE, FIBER_PRIORITY, 0);

	if (!nano_task_fifo_get(&timeout_order_fifo, TICKS_NONE)) {
		TC_ERROR(" *** fiber should have run and filled the fifo.\n");
		return TC_FAIL;
	}

	if (reply_packet.reply != 1) {
		TC_ERROR(" *** fiber should have obtained the semaphore.\n");
		return TC_FAIL;
	}

	TC_PRINT("fiber with TICKS_NONE got available sem, as expected\n");

	/* test fiber with timeout of TICKS_UNLIMITED getting full semaphore */

	nano_task_sem_give(&sem_timeout[0]);

	task_fiber_start(timeout_stacks[0], FIBER_STACKSIZE,
						test_fiber_ticks_special_values,
						(int)&reply_packet, TICKS_UNLIMITED, FIBER_PRIORITY, 0);

	if (!nano_task_fifo_get(&timeout_order_fifo, TICKS_NONE)) {
		TC_ERROR(" *** fiber should have run and filled the fifo.\n");
		return TC_FAIL;
	}

	if (reply_packet.reply != 1) {
		TC_ERROR(" *** fiber should have obtained the semaphore.\n");
		return TC_FAIL;
	}

	TC_PRINT("fiber with TICKS_UNLIMITED got available sem, as expected\n");

	/* test multiple fibers pending on the same sem with different timeouts */

	test_data_size = ARRAY_SIZE(timeout_order_data);

	TC_PRINT("testing timeouts of %d fibers on same sem\n", test_data_size);

	rv = test_multiple_fibers_pending(timeout_order_data, test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not time out in the right order\n");
		return TC_FAIL;
	}

	/* test multiple fibers pending on different sems with different timeouts */

	test_data_size = ARRAY_SIZE(timeout_order_data_mult_sem);

	TC_PRINT("testing timeouts of %d fibers on different sems\n",
				test_data_size);

	rv = test_multiple_fibers_pending(timeout_order_data_mult_sem,
										test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not time out in the right order\n");
		return TC_FAIL;
	}

	/*
	 * test multiple fibers pending on same sem with different timeouts, but
	 * getting the semaphore in time, except the last one.
	 */

	test_data_size = ARRAY_SIZE(timeout_order_data);

	TC_PRINT("testing %d fibers timing out, but obtaining the sem in time\n"
				"(except the last one, which times out)\n",
				test_data_size);

	rv = test_multiple_fibers_get_sem(timeout_order_data, test_data_size);
	if (rv != TC_PASS) {
		TC_ERROR(" *** fibers did not get the sem in the right order\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Entry point to semaphore tests
 *
 * This is the entry point to the semaphore tests.
 *
 * @return N/A
 */

void main(void)
{
	int     rv;       /* return value from tests */

	TC_START("Test Nanokernel Semaphores");

	initNanoObjects();

	rv = testSemTaskNoWait();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	rv = testSemIsrNoWait();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	semTestState = STS_INIT;

	/*
	 * Start the fiber.  The fiber will be given a higher priority than the
	 * main task.
	 */

	task_fiber_start(fiberStack, FIBER_STACKSIZE, fiberEntry,
					 0, 0, FIBER_PRIORITY, 0);

	rv = testSemWait();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	rv = test_multiple_waiters();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	rv = test_timeout();
	if (rv != TC_PASS) {
		goto doneTests;
	}

doneTests:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
