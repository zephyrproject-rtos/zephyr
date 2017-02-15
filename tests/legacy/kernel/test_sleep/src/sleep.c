/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief Test nanokernel sleep and wakeup APIs
 *
 * This module tests the following sleep and wakeup scenarios:
 * 1. fiber_sleep() without cancellation
 * 2. fiber_sleep() cancelled via fiber_fiber_wakeup()
 * 3. fiber_sleep() cancelled via isr_fiber_wakeup()
 * 4. fiber_sleep() cancelled via task_fiber_wakeup()
 * 5. task_sleep() - no cancellation exists
 */

#include <tc_util.h>
#include <arch/cpu.h>
#include <misc/util.h>
#include <irq_offload.h>
#include <stdbool.h>

#include <util_test_common.h>

#if defined(CONFIG_ASSERT) && defined(CONFIG_DEBUG)
#define FIBER_STACKSIZE    (384 + CONFIG_TEST_EXTRA_STACKSIZE)
#else
#define FIBER_STACKSIZE    (256 + CONFIG_TEST_EXTRA_STACKSIZE)
#endif

#define TEST_FIBER_PRIORITY       4
#define HELPER_FIBER_PRIORITY    10

#define ONE_SECOND    (sys_clock_ticks_per_sec)

static struct nano_sem test_fiber_sem;
static struct nano_sem helper_fiber_sem;
static struct nano_sem task_sem;

static char __stack test_fiber_stack[FIBER_STACKSIZE];
static char __stack helper_fiber_stack[FIBER_STACKSIZE];

static nano_thread_id_t  test_fiber_id;
static nano_thread_id_t  helper_fiber_id;

static bool test_failure = true;     /* Assume the test will fail */

static void test_objects_init(void)
{
	nano_sem_init(&test_fiber_sem);
	nano_sem_init(&helper_fiber_sem);
	nano_sem_init(&task_sem);

	TC_PRINT("Nanokernel objects initialized\n");
}

static void align_to_tick_boundary(void)
{
	uint32_t tick;

	tick = sys_tick_get_32();
	while (sys_tick_get_32() == tick) {
		/* Busy wait to align to tick boundary */
	}
}

/* Shouldn't ever sleep for less than requested time, but allow for 1
 * tick of "too long" slop for aliasing between wakeup and
 * measurement. Qemu at least will leak the external world's clock
 * rate into the simulator when the host is under load.
 */
static int sleep_time_valid(uint32_t start, uint32_t end, uint32_t dur)
{
	uint32_t dt = end - start;

	return dt >= dur && dt <= (dur + 1);
}

static void test_fiber(int arg1, int arg2)
{
	uint32_t start_tick;
	uint32_t end_tick;

	nano_fiber_sem_take(&test_fiber_sem, TICKS_UNLIMITED);

	TC_PRINT("Testing normal expiration of fiber_sleep()\n");
	align_to_tick_boundary();

	start_tick = sys_tick_get_32();
	fiber_sleep(ONE_SECOND);
	end_tick = sys_tick_get_32();

	if (!sleep_time_valid(start_tick, end_tick, ONE_SECOND)) {
		TC_ERROR(" *** fiber_sleep() slept for %d ticks not %d.",
				 end_tick - start_tick, ONE_SECOND);

		return;
	}

	TC_PRINT("Testing fiber_sleep() + fiber_fiber_wakeup()\n");
	nano_fiber_sem_give(&helper_fiber_sem);   /* Activate helper fiber */
	align_to_tick_boundary();

	start_tick = sys_tick_get_32();
	fiber_sleep(ONE_SECOND);
	end_tick = sys_tick_get_32();

	if (end_tick - start_tick > 1) {
		TC_ERROR(" *** fiber_fiber_wakeup() took too long (%d ticks)\n",
				 end_tick - start_tick);
		return;
	}

	TC_PRINT("Testing fiber_sleep() + isr_fiber_wakeup()\n");
	nano_fiber_sem_give(&helper_fiber_sem);   /* Activate helper fiber */
	align_to_tick_boundary();

	start_tick = sys_tick_get_32();
	fiber_sleep(ONE_SECOND);
	end_tick = sys_tick_get_32();

	if (end_tick - start_tick > 1) {
		TC_ERROR(" *** isr_fiber_wakeup() took too long (%d ticks)\n",
				 end_tick - start_tick);
		return;
	}

	TC_PRINT("Testing fiber_sleep() + task_fiber_wakeup()\n");
	nano_task_sem_give(&task_sem);    /* Activate task */
	align_to_tick_boundary();

	start_tick = sys_tick_get_32();
	fiber_sleep(ONE_SECOND);           /* Task will execute */
	end_tick = sys_tick_get_32();

	if (end_tick - start_tick > 1) {
		TC_ERROR(" *** task_fiber_wakeup() took too long (%d ticks)\n",
				 end_tick - start_tick);
		return;
	}

	test_failure = false;
}

static void irq_offload_isr(void *arg)
{
	isr_fiber_wakeup((nano_thread_id_t) arg);
}

static void helper_fiber(int arg1, int arg2)
{
	nano_fiber_sem_take(&helper_fiber_sem, TICKS_UNLIMITED);

	/* Wake the test fiber */
	fiber_fiber_wakeup(test_fiber_id);
	nano_fiber_sem_take(&helper_fiber_sem, TICKS_UNLIMITED);

	/* Wake the test fiber from an ISR */
	irq_offload(irq_offload_isr, (void *)test_fiber_id);
}

void main(void)
{
	int       status = TC_FAIL;
	uint32_t  start_tick;
	uint32_t  end_tick;

	TC_START("Test Nanokernel Sleep and Wakeup APIs\n");

	test_objects_init();

	test_fiber_id = task_fiber_start(test_fiber_stack, FIBER_STACKSIZE,
					 test_fiber, 0, 0, TEST_FIBER_PRIORITY, 0);
	TC_PRINT("Test fiber started: id = %p\n", test_fiber_id);

	helper_fiber_id = task_fiber_start(helper_fiber_stack, FIBER_STACKSIZE,
						helper_fiber, 0, 0, HELPER_FIBER_PRIORITY, 0);
	TC_PRINT("Helper fiber started: id = %p\n", helper_fiber_id);

	/* Activate test_fiber */
	nano_task_sem_give(&test_fiber_sem);

	/* Wait for test_fiber to activate us */
	nano_task_sem_take(&task_sem, TICKS_UNLIMITED);

	/* Wake the test fiber */
	task_fiber_wakeup(test_fiber_id);

	if (test_failure) {
		goto done_tests;
	}

	TC_PRINT("Testing nanokernel task_sleep()\n");
	align_to_tick_boundary();
	start_tick = sys_tick_get_32();
	task_sleep(ONE_SECOND);
	end_tick = sys_tick_get_32();

	if (!sleep_time_valid(start_tick, end_tick, ONE_SECOND)) {
		TC_ERROR("task_sleep() slept for %d ticks, not %d\n",
				 end_tick - start_tick, ONE_SECOND);
		goto done_tests;
	}

	status = TC_PASS;

done_tests:
	TC_END_REPORT(status);
}
