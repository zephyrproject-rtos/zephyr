/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

#include <tc_util.h>
#include <zephyr.h>
#include <kernel_structs.h>
#include <stdbool.h>

#define  SECONDS(x)                 ((x) * sys_clock_ticks_per_sec)
#define  HALF_SECOND                (sys_clock_ticks_per_sec / 2)
#define  THIRD_SECOND               (sys_clock_ticks_per_sec / 3)
#define  FOURTH_SECOND              (sys_clock_ticks_per_sec / 4)

#define FIBER_STACKSIZE  1024

#define FIFO_TEST_START       10
#define FIFO_TEST_END         20

#define SEM_TEST_START        30
#define SEM_TEST_END          40

#define LIFO_TEST_START       50
#define LIFO_TEST_END         60

#define NON_NULL_PTR          ((void *)0x12345678)

struct fifo_data {
	uint32_t  reserved;
	uint32_t  data;
};

struct lifo_data {
	uint32_t  reserved;
	uint32_t  data;
};

static char __stack fiber_stack[2][FIBER_STACKSIZE];

static struct nano_fifo  fifo;
static struct nano_lifo  lifo;
static struct nano_timer timer;

static struct nano_sem start_test_sem;
static struct nano_sem sync_test_sem;
static struct nano_sem end_test_sem;

struct fifo_data fifo_test_data[4] = {
		{0, FIFO_TEST_END + 1}, {0, FIFO_TEST_END + 2},
		{0, FIFO_TEST_END + 3}, {0, FIFO_TEST_END + 4}
	};

struct lifo_data lifo_test_data[4] = {
		{0, LIFO_TEST_END + 1}, {0, LIFO_TEST_END + 2},
		{0, LIFO_TEST_END + 3}, {0, LIFO_TEST_END + 4}
	};

static uint32_t timer_start_tick;
static uint32_t timer_end_tick;
static void *timer_data;

static int __noinit fiber_high_state;
static int __noinit fiber_low_state;
static int __noinit task_high_state;
static int __noinit task_low_state;

static int __noinit counter;

static int increment_counter(void)
{
	int  tmp;
	int  key = irq_lock();

	tmp = ++counter;
	irq_unlock(key);

	return tmp;
}

static int sync_threads(void *arg)
{
	struct nano_sem *sem = arg;

	nano_fiber_sem_give(sem);
	nano_fiber_sem_give(sem);
	nano_fiber_sem_give(sem);
	nano_fiber_sem_give(sem);

	return 0;
}

static void fifo_tests(int32_t timeout, volatile int *state,
		void *(*get)(struct nano_fifo *, int32_t),
		int (*sem_take)(struct nano_sem *, int32_t))
{
	struct fifo_data  *data;

	sem_take(&start_test_sem, TICKS_UNLIMITED);

	*state = FIFO_TEST_START;
	/* Expect this to time out */
	data = get(&fifo, timeout);
	if (data != NULL) {
		TC_ERROR("**** Unexpected data on FIFO get\n");
		return;
	}
	*state = increment_counter();

	/* Sync up fifo test threads */
	sem_take(&sync_test_sem, TICKS_UNLIMITED);

	/* Expect this to receive data from the fifo */
	*state = FIFO_TEST_END;
	data = get(&fifo, timeout);
	if (data == NULL) {
		TC_ERROR("**** No data on FIFO get\n");
		return;
	}
	*state = increment_counter();

	if (data->data != *state) {
		TC_ERROR("**** Got FIFO data %d, not %d (%d)\n",
			data->data, *state, timeout);
		return;
	}

	sem_take(&end_test_sem, TICKS_UNLIMITED);
}

static void lifo_tests(int32_t timeout, volatile int *state,
		void *(*get)(struct nano_lifo *, int32_t),
		int (*sem_take)(struct nano_sem *, int32_t))
{
	struct lifo_data  *data;

	sem_take(&start_test_sem, TICKS_UNLIMITED);

	*state = LIFO_TEST_START;
	/* Expect this to time out */
	data = get(&lifo, timeout);
	if (data != NULL) {
		TC_ERROR("**** Unexpected data on LIFO get\n");
		return;
	}
	*state = increment_counter();

	/* Sync up all threads */
	sem_take(&sync_test_sem, TICKS_UNLIMITED);

	/* Expect this to receive data from the lifo */
	*state = LIFO_TEST_END;
	data = get(&lifo, timeout);
	if (data == NULL) {
		TC_ERROR("**** No data on LIFO get\n");
		return;
	}
	*state = increment_counter();

	if (data->data != *state) {
		TC_ERROR("**** Got LIFO data %d, not %d (%d)\n",
			data->data, *state, timeout);
		return;
	}

	sem_take(&end_test_sem, TICKS_UNLIMITED);
}

static void timer_tests(void)
{
	nano_task_sem_take(&start_test_sem, TICKS_UNLIMITED);

	timer_start_tick = sys_tick_get_32();

	nano_task_timer_start(&timer, SECONDS(1));

	timer_data = nano_task_timer_test(&timer, TICKS_UNLIMITED);
	timer_end_tick = sys_tick_get_32();

	nano_task_sem_take(&end_test_sem, TICKS_UNLIMITED);
}

static void fiber_high(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	fifo_tests(SECONDS(1), &fiber_high_state,
		nano_fiber_fifo_get, nano_fiber_sem_take);

	lifo_tests(SECONDS(1), &fiber_high_state,
		nano_fiber_lifo_get, nano_fiber_sem_take);
}

static void fiber_low(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	fifo_tests(HALF_SECOND, &fiber_low_state,
		nano_fiber_fifo_get, nano_fiber_sem_take);

	lifo_tests(HALF_SECOND, &fiber_low_state,
		nano_fiber_lifo_get, nano_fiber_sem_take);
}

void task_high(void)
{
	TC_START("Test Microkernel Tasks Pending on Nanokernel Objects");

	nano_fifo_init(&fifo);
	nano_lifo_init(&lifo);
	nano_timer_init(&timer, NON_NULL_PTR);

	nano_sem_init(&start_test_sem);
	nano_sem_init(&sync_test_sem);
	nano_sem_init(&end_test_sem);

	counter = SEM_TEST_START;
	task_fiber_start(fiber_stack[0], FIBER_STACKSIZE,
			fiber_high, 0, 0, 3, 0);

	task_fiber_start(fiber_stack[1], FIBER_STACKSIZE,
			fiber_low, 0, 0, 7, 0);

	counter = FIFO_TEST_START;
	fifo_tests(THIRD_SECOND, &task_high_state,
		nano_task_fifo_get, nano_task_sem_take);

	counter = LIFO_TEST_START;
	lifo_tests(THIRD_SECOND, &task_high_state,
		nano_task_lifo_get, nano_task_sem_take);

	timer_tests();
}

void task_low(void)
{
	fifo_tests(FOURTH_SECOND, &task_low_state,
		nano_task_fifo_get, nano_task_sem_take);

	lifo_tests(FOURTH_SECOND, &task_low_state,
		nano_task_lifo_get, nano_task_sem_take);
}

void task_monitor(void)
{
	int result = TC_FAIL;

	task_offload_to_fiber(sync_threads, &start_test_sem);

	/*
	 * Verify that microkernel tasks 'task_high' and 'task_low' do not
	 * busy-wait. If they are not busy-waiting, then they must be pending.
	 */

	TC_PRINT("Testing microkernel tasks block on nanokernel fifos ...\n");
	if ((fiber_high_state != FIFO_TEST_START) ||
		(fiber_low_state != FIFO_TEST_START) ||
		(task_high_state != FIFO_TEST_START) ||
		(task_low_state != FIFO_TEST_START)) {
		goto error;
	}

	/* Give waiting threads time to time-out */
	task_sleep(SECONDS(2));

	/* Verify that the fibers and tasks timed-out in the correct order. */
	TC_PRINT("Testing nanokernel fifos time-out in correct order ...\n");
	if ((task_low_state != FIFO_TEST_START + 1) ||
		(task_high_state != FIFO_TEST_START + 2) ||
		(fiber_low_state != FIFO_TEST_START + 3) ||
		(fiber_high_state != FIFO_TEST_START + 4)) {
		TC_ERROR("**** Threads timed-out in unexpected order\n");
		goto error;
	}

	counter = FIFO_TEST_END;
	task_offload_to_fiber(sync_threads, &sync_test_sem);

	/* Two fibers and two tasks should be waiting on the FIFO */

	/* Add data to the FIFO */
	TC_PRINT("Testing nanokernel fifos delivered data correctly ...\n");
	nano_task_fifo_put(&fifo, &fifo_test_data[0]);
	nano_task_fifo_put(&fifo, &fifo_test_data[1]);
	nano_task_fifo_put(&fifo, &fifo_test_data[2]);
	nano_task_fifo_put(&fifo, &fifo_test_data[3]);

	if ((fiber_high_state != FIFO_TEST_END + 1) ||
		(fiber_low_state != FIFO_TEST_END + 2) ||
		(task_high_state != FIFO_TEST_END + 3) ||
		(task_low_state != FIFO_TEST_END + 4)) {
		TC_ERROR("**** Unexpected delivery order\n");
		goto error;
	}

	task_offload_to_fiber(sync_threads, &end_test_sem);

	/* ******************************************************** */

	task_offload_to_fiber(sync_threads, &start_test_sem);

	/*
	 * Verify that microkernel tasks 'task_high' and 'task_low' do not
	 * busy-wait. If they are not busy-waiting, then they must be pending.
	 */

	TC_PRINT("Testing microkernel tasks block on nanokernel lifos ...\n");
	if ((fiber_high_state != LIFO_TEST_START) ||
		(fiber_low_state != LIFO_TEST_START) ||
		(task_high_state != LIFO_TEST_START) ||
		(task_low_state != LIFO_TEST_START)) {
		goto error;
	}

	/* Give waiting threads time to time-out */
	task_sleep(SECONDS(2));

	TC_PRINT("Testing nanokernel lifos time-out in correct order ...\n");
	if ((task_low_state != LIFO_TEST_START + 1) ||
		(task_high_state != LIFO_TEST_START + 2) ||
		(fiber_low_state != LIFO_TEST_START + 3) ||
		(fiber_high_state != LIFO_TEST_START + 4)) {
		TC_ERROR("**** Threads timed-out in unexpected order\n");
		goto error;
	}

	counter = LIFO_TEST_END;
	task_offload_to_fiber(sync_threads, &sync_test_sem);

	/* Two fibers and two tasks should be waiting on the LIFO */

	/* Add data to the LIFO */
	nano_task_lifo_put(&lifo, &lifo_test_data[0]);
	nano_task_lifo_put(&lifo, &lifo_test_data[1]);
	nano_task_lifo_put(&lifo, &lifo_test_data[2]);
	nano_task_lifo_put(&lifo, &lifo_test_data[3]);

	TC_PRINT("Testing nanokernel lifos delivered data correctly ...\n");
	if ((fiber_high_state != LIFO_TEST_END + 1) ||
		(fiber_low_state != LIFO_TEST_END + 2) ||
		(task_high_state != LIFO_TEST_END + 3) ||
		(task_low_state != LIFO_TEST_END + 4)) {
		TC_ERROR("**** Unexpected timeout order\n");
		goto error;
	}

	task_offload_to_fiber(sync_threads, &end_test_sem);

	/* ******************************************************** */

	timer_end_tick = 0;
	nano_task_sem_give(&start_test_sem);    /* start timer tests */

	/*
	 * NOTE: The timer test is running in the context of high_task().
	 * Scheduling is expected to yield to high_task().  If high_task()
	 * does not pend as expected, then timer_end_tick will be non-zero.
	 */

	TC_PRINT("Testing microkernel task waiting on nanokernel timer ...\n");
	if (timer_end_tick != 0) {
		TC_ERROR("Task did not pend on nanokernel timer\n");
		goto error;
	}

	/* Let the timer expire */
	task_sleep(SECONDS(2));

	if (timer_end_tick < timer_start_tick + SECONDS(1)) {
		TC_ERROR("Task waiting on a nanokernel timer error\n");
		goto error;
	}

	if (timer_data != NON_NULL_PTR) {
		TC_ERROR("Incorrect data from nanokernel timer\n");
		goto error;
	}

	nano_task_sem_give(&end_test_sem);

	result = TC_PASS;

error:
	TC_END_RESULT(result);
	TC_END_REPORT(result);
}
