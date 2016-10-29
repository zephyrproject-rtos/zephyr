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

/*
 * @file
 * @brief Test early sleeping microkernel mechanism
 *
 * This test verifies that both fiber_sleep() and task_sleep()
 * can each be used to put the calling thread to sleep for a specified
 * number of ticks during system initialization (before k_server() starts)
 * as well as after the microkernel initializes (after k_server() starts).
 *
 * To ensure that the nanokernel timeout both operates correctly during
 * system initialization and that it allows fibers to sleep for a specified
 * number of ticks the test has a fiber invoke fiber_sleep() before the init
 * task invokes task_sleep(). The fiber sleep time is less than that of the
 * task sleep time so that the fiber will wake before the init task wakes.
 */

#include <zephyr.h>
#include <tc_util.h>
#include <init.h>


#define FIBER_TICKS_TO_SLEEP 40
#define TASK_TICKS_TO_SLEEP 50

/* time that the task was actually sleeping */
static int task_actual_sleep_ticks;
static int task_actual_sleep_nano_ticks;
static int task_actual_sleep_micro_ticks;
static int task_actual_sleep_app_ticks;

/* time that the fiber was actually sleeping */
static volatile int fiber_actual_sleep_ticks;

/*
 * Flag is changed by lower priority task to make sure
 * that sleeping did not go in a tight toop
 */
static bool alternate_task_run;

/* test fiber synchronization semaphore */
static struct nano_sem test_fiber_sem;

/**
 *
 * @brief Put task to sleep and measure time it really slept
 *
 * @param ticks_to_sleep number of ticks for a task to sleep
 *
 * @return number of ticks the task actually slept
 */
int test_task_sleep(int ticks_to_sleep)
{
	uint32_t start_time;
	uint32_t stop_time;

	start_time = sys_cycle_get_32();
	task_sleep(ticks_to_sleep);
	stop_time = sys_cycle_get_32();

	return (stop_time - start_time) / sys_clock_hw_cycles_per_tick;
}


/**
 *
 * @brief Put fiber to sleep and measure time it really slept
 *
 * @param ticks_to_sleep number of ticks for a fiber to sleep
 *
 * @return number of ticks the fiber actually slept
 */
int test_fiber_sleep(int ticks_to_sleep)
{
	uint32_t start_time;
	uint32_t stop_time;

	start_time = sys_cycle_get_32();
	fiber_sleep(ticks_to_sleep);
	stop_time = sys_cycle_get_32();

	return (stop_time - start_time) / sys_clock_hw_cycles_per_tick;
}


/**
 *
 * @brief Early task sleep test
 *
 * Note: it will be used to test the early sleep at SECONDARY level too
 *
 * Call task_sleep() and checks the time sleep actually
 * took to make sure that task actually slept
 *
 * @return 0
 */
static int test_early_task_sleep(struct device *unused)
{
	ARG_UNUSED(unused);
	task_actual_sleep_ticks = test_task_sleep(TASK_TICKS_TO_SLEEP);
	return 0;
}

SYS_INIT(test_early_task_sleep, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/**
 *
 * @brief Early task sleep test in NANOKERNEL level only
 *
 * Call task_sleep() and checks the time sleep actually
 * took to make sure that task actually slept
 *
 * @return 0
 */
static int test_early_task_sleep_in_nanokernel_level(struct device *unused)
{
	ARG_UNUSED(unused);
	task_actual_sleep_nano_ticks = test_task_sleep(TASK_TICKS_TO_SLEEP);
	return 0;
}

SYS_INIT(test_early_task_sleep_in_nanokernel_level,
	 NANOKERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/**
 *
 * @brief Early task sleep test in MICROKERNEL level only
 *
 * Call task_sleep() and checks the time sleep actually
 * took to make sure that task actually slept
 *
 * @return 0
 */
static int test_early_task_sleep_in_microkernel_level(struct device *unused)
{
	ARG_UNUSED(unused);
	task_actual_sleep_micro_ticks = test_task_sleep(TASK_TICKS_TO_SLEEP);
	return 0;
}

SYS_INIT(test_early_task_sleep_in_microkernel_level,
	 MICROKERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/**
 *
 * @brief Early task sleep test in APPLICATION level only
 *
 * Call task_sleep() and checks the time sleep actually
 * took to make sure that task actually slept
 *
 * @return 0
 */
static int test_early_task_sleep_in_application_level(struct device *unused)
{
	ARG_UNUSED(unused);
	task_actual_sleep_app_ticks = test_task_sleep(TASK_TICKS_TO_SLEEP);
	return 0;
}

SYS_INIT(test_early_task_sleep_in_application_level,
	 APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/**
 *
 * @brief Fiber function that measures fiber sleep time
 *
 * @return N/A
 */
static void test_fiber(int ticks_to_sleep, int unused)
{
	ARG_UNUSED(unused);
	while (1) {
		fiber_actual_sleep_ticks = test_fiber_sleep(ticks_to_sleep);
		fiber_sem_give(TEST_FIBER_SEM);
		nano_sem_take(&test_fiber_sem, TICKS_UNLIMITED);
	}
}

#define STACKSIZE 512
char __stack test_fiber_stack[STACKSIZE];

/**
 *
 * @brief Initialize test fiber data
 *
 * @return 0
 */
static int test_fiber_start(struct device *unused)
{
	ARG_UNUSED(unused);
	fiber_actual_sleep_ticks = 0;
	nano_sem_init(&test_fiber_sem);
	task_fiber_start(&test_fiber_stack[0], STACKSIZE,
					 (nano_fiber_entry_t) test_fiber,
					 FIBER_TICKS_TO_SLEEP, 0, 7, 0);
	return 0;
}

SYS_INIT(test_fiber_start, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);


/**
 *
 * @brief Lower priority task to make sure that main task really sleeps
 *
 *
 *
 * @return N/A
 */
void AlternateTask(void)
{
	alternate_task_run = true;
}

/**
 *
 * @brief Regression task
 *
 * Checks the results of the early sleep
 *
 * @return N/A
 */
void RegressionTask(void)
{
	TC_START("Test early and regular task and fiber sleep functionality\n");
	alternate_task_run = false;

	TC_PRINT("Test fiber_sleep() call during the system initialization\n");
	/*
	 * Make sure that the fiber_sleep() called during the
	 * initialization has returned.
	 * fiber_sleep() invoked during the initialization for the
	 * shorter period that task_sleep() should return by now.
	 */
	if (task_sem_take(TEST_FIBER_SEM, TICKS_NONE) != RC_OK) {
		TC_ERROR("fiber_sleep() has not returned while expected\n");
	}

	/*
	 * Check that the fiber_sleep() called during the system
	 * initialization put the fiber to sleep for the specified
	 * amount of time
	 *
	 * On heavily loaded systems QEMU may demonstrate a drift
	 * of hardware clock ticks to system clock. Test verifies
	 * that sleep took at least not less amount of time.
	 * Allow up to 1 tick variance as the test may not have put
	 * the task to sleep on a tick boundary.
	 */
	if ((fiber_actual_sleep_ticks + 1) < FIBER_TICKS_TO_SLEEP) {
		TC_ERROR("fiber_sleep() time is too small: %d\n",
			fiber_actual_sleep_ticks);
		goto error_out;
	}

	/*
	 * Check that the task_sleep() called during the system
	 * initialization puts the task to sleep for the specified
	 * amount of time
	 */
	TC_PRINT("Test task_sleep() call during the system initialization\n");
	TC_PRINT("- At SECONDARY level\n");

	if ((task_actual_sleep_ticks + 1) < TASK_TICKS_TO_SLEEP) {
		TC_ERROR("task_sleep() time is is too small: %d\n",
			task_actual_sleep_ticks);
		goto error_out;
	}

	/*
	 * Check that the task_sleep() called during the system
	 * initialization at NANOKERNEL level puts the task to sleep for
	 * the specified amount of time
	 */
	TC_PRINT("- At NANOKERNEL level\n");

	if ((task_actual_sleep_nano_ticks + 1) < TASK_TICKS_TO_SLEEP) {
		TC_ERROR("task_sleep() time is is too small: %d\n",
			task_actual_sleep_nano_ticks);
		goto error_out;
	}

	/*
	 * Check that the task_sleep() called during the system
	 * initialization at MICROKERNEL level puts the task to sleep for
	 * the specified amount of time
	 */
	TC_PRINT("- At MICROKERNEL level\n");

	if ((task_actual_sleep_micro_ticks + 1) < TASK_TICKS_TO_SLEEP) {
		TC_ERROR("task_sleep() time is is too small: %d\n",
			task_actual_sleep_micro_ticks);
		goto error_out;
	}

	/*
	 * Check that the task_sleep() called during the system
	 * initialization at APPLICATION level puts the task to sleep for
	 * the specified amount of time
	 */
	TC_PRINT("- At APPLICATION level\n");

	if ((task_actual_sleep_app_ticks + 1) < TASK_TICKS_TO_SLEEP) {
		TC_ERROR("task_sleep() time is is too small: %d\n",
			task_actual_sleep_app_ticks);
		goto error_out;
	}

	/*
	 * Check that the task_sleep() called during the normal
	 * microkernel work put the task to sleep for the specified
	 * amount of time
	 */
	TC_PRINT("Test task_sleep() call on a running system\n");
	task_actual_sleep_ticks = test_task_sleep(TASK_TICKS_TO_SLEEP);

	if ((task_actual_sleep_ticks + 1) < TASK_TICKS_TO_SLEEP) {
		TC_ERROR("task_sleep() time is too small: %d\n",
			task_actual_sleep_ticks);
		goto error_out;
	}

	/* check that calling task_sleep() allowed the lower priority task run */
	if (!alternate_task_run) {
		TC_ERROR("Lower priority task did not run during task_sleep()\n");
		goto error_out;
	}

	/*
	 * Check that the fiber_sleep() called during the normal
	 * microkernel work put the fiber to sleep for the specified
	 * amount of time
	 */
	TC_PRINT("Test fiber_sleep() call on a running system\n");

	fiber_actual_sleep_ticks = 0;
	nano_sem_give(&test_fiber_sem);
	/* wait for the test fiber return from the sleep */
	task_sem_take(TEST_FIBER_SEM, TICKS_UNLIMITED);

	if ((fiber_actual_sleep_ticks + 1) < FIBER_TICKS_TO_SLEEP) {
		TC_ERROR("fiber_sleep() time is too small: %d\n",
			fiber_actual_sleep_ticks);
		goto error_out;
	}

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
	return;

error_out:
	TC_END_RESULT(TC_FAIL);
	TC_END_REPORT(TC_FAIL);
}
