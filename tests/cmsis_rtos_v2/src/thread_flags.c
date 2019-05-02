/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os2.h>

#include <irq_offload.h>
#include <kernel_structs.h>

#define TIMEOUT_TICKS   (10)
#define FLAG1           (0x00000020)
#define FLAG2           (0x00000004)
#define FLAG            (FLAG1 | FLAG2)
#define ISR_FLAG        (0x50)
#define STACKSZ         CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE

static void thread1(void *arg)
{
	u32_t flags;

	/* wait for FLAG1. It should return immediately as it is
	 * already triggered.
	 */
	flags = osThreadFlagsWait(FLAG1, osFlagsWaitAny | osFlagsNoClear, 0);
	zassert_equal(flags & FLAG1, FLAG1, "");

	/* Since the flags are not cleared automatically in the previous step,
	 * we should be able to get the same flags upon query below.
	 */
	flags = osThreadFlagsGet();
	zassert_equal(flags & FLAG1, FLAG1, "");

	/* Clear the Flag explicitly */
	flags = osThreadFlagsClear(FLAG1);
	zassert_not_equal(flags, osFlagsErrorParameter,
			  "ThreadFlagsClear failed");

	/* wait for FLAG1. It should timeout here as the flag
	 * though triggered, gets cleared in the previous step.
	 */
	flags = osThreadFlagsWait(FLAG1, osFlagsWaitAny, TIMEOUT_TICKS);
	zassert_equal(flags, osFlagsErrorTimeout, "ThreadFlagsWait failed");
}

static void thread2(void *arg)
{
	u32_t flags;

	flags = osThreadFlagsWait(FLAG, osFlagsWaitAll, TIMEOUT_TICKS);
	zassert_equal(flags & FLAG, FLAG,
		      "osThreadFlagsWait failed unexpectedly");

	/* validate by passing invalid parameters */
	zassert_equal(osThreadFlagsSet(NULL, 0), osFlagsErrorParameter,
		      "Invalid Thread Flags ID is unexpectedly working!");
	zassert_equal(osThreadFlagsSet(osThreadGetId(), 0x80010000),
		      osFlagsErrorParameter,
		      "Thread with MSB set is set unexpectedly");

	zassert_equal(osThreadFlagsClear(0x80010000), osFlagsErrorParameter,
		      "Thread with MSB set is cleared unexpectedly");

	/* cannot wait for Flag mask with MSB set */
	zassert_equal(osThreadFlagsWait(0x80010000, osFlagsWaitAny, 0),
		      osFlagsErrorParameter,
		      "ThreadFlagsWait passed unexpectedly");
}

static K_THREAD_STACK_DEFINE(test_stack1, STACKSZ);
static osThreadAttr_t thread1_attr = {
	.name = "Thread1",
	.stack_mem = &test_stack1,
	.stack_size = STACKSZ,
	.priority = osPriorityHigh,
};

static K_THREAD_STACK_DEFINE(test_stack2, STACKSZ);
static osThreadAttr_t thread2_attr = {
	.name = "Thread2",
	.stack_mem = &test_stack2,
	.stack_size = STACKSZ,
	.priority = osPriorityHigh,
};

void test_thread_flags_no_wait_timeout(void)
{
	osThreadId_t id1;
	u32_t flags;

	id1 = osThreadNew(thread1, NULL, &thread1_attr);
	zassert_true(id1 != NULL, "Failed creating thread1");

	flags = osThreadFlagsSet(id1, FLAG1);
	zassert_equal(flags & FLAG1, FLAG1, "");

	/* Let id1 run to do the tests for Thread Flags */
	osDelay(TIMEOUT_TICKS);
}

void test_thread_flags_signalled(void)
{
	osThreadId_t id;
	u32_t flags;

	id = osThreadNew(thread2, osThreadGetId(), &thread2_attr);
	zassert_true(id != NULL, "Failed creating thread2");

	flags = osThreadFlagsSet(id, FLAG1);
	zassert_equal(flags & FLAG1, FLAG1, "");

	/* Let id run to do the tests for Thread Flags */
	osDelay(TIMEOUT_TICKS / 2);

	flags = osThreadFlagsSet(id, FLAG2);
	zassert_equal(flags & FLAG2, FLAG2, "");

	/* z_thread has a higher priority over the other threads.
	 * Hence, this thread needs to be put to sleep for thread2
	 * to become the active thread.
	 */
	osDelay(TIMEOUT_TICKS / 2);
}

/* IRQ offload function handler to set Thread flag */
static void offload_function(void *param)
{
	int flags;

	/* Make sure we're in IRQ context */
	zassert_true(k_is_in_isr(), "Not in IRQ context!");

	flags = osThreadFlagsSet((osThreadId_t)param, ISR_FLAG);
	zassert_equal(flags & ISR_FLAG, ISR_FLAG,
		      "ThreadFlagsSet failed in ISR");
}

void test_thread_flags_from_isr(void *thread_id)
{
	u32_t flags;

	/**TESTPOINT: Offload to IRQ context*/
	irq_offload(offload_function, (void *)osThreadGetId());

	flags = osThreadFlagsWait(ISR_FLAG, osFlagsWaitAll, TIMEOUT_TICKS);
	zassert_equal((flags & ISR_FLAG),
		      ISR_FLAG, "unexpected Thread flags value");
}

static K_THREAD_STACK_DEFINE(test_stack3, STACKSZ);
static osThreadAttr_t thread3_attr = {
	.name = "Thread3",
	.stack_mem = &test_stack3,
	.stack_size = STACKSZ,
	.priority = osPriorityHigh,
};

void test_thread_flags_isr(void)
{
	osThreadId_t id;

	id = osThreadNew(test_thread_flags_from_isr, osThreadGetId(),
			 &thread3_attr);
	zassert_true(id != NULL, "Failed creating thread");

	osDelay(TIMEOUT_TICKS);
}
