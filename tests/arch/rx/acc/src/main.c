/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/kernel.h"
#include <stdint.h>
#include <zephyr/ztest.h>
#include <soc.h>

#define STACK_SIZE (512)

ZTEST_SUITE(rx_acc_tests, NULL, NULL, NULL, NULL, NULL);

/*local variables*/
static K_THREAD_STACK_DEFINE(tstack_thread_1, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack_thread_2, STACK_SIZE);
static struct k_thread thread_1;
static struct k_thread thread_2;
struct k_event my_event;
static volatile int32_t thread_1_m, thread_1_h, thread_2_m, thread_2_h;

static void thread_1_entry(void *p1, void *p2, void *p3)
{

	/* Set the initial accumulator registers */
	__asm volatile("MOV #0x90ABCDEF,R15\n"
			"MVTACLO	R15\n"
			"MOV #0x12345678,R15\n"
			"MVTACHI	R15\n");

	/* yield the current thread. */
	k_yield();

	while (1) {
		/* Get Accumulator registers */
		__asm volatile("MVFACMI	R15\n"
				"MOV.L R15, %0"
				: "=r"(thread_1_m));

		__asm volatile("MVFACHI	R15\n"
				"MOV.L R15, %0"
				: "=r"(thread_1_h));

		/* Set Event flag */
		k_event_post(&my_event, 0x001);
		k_sleep(K_SECONDS(10U));
	}
}

static void thread_2_entry(void *p1, void *p2, void *p3)
{

	/* Set the initial accumulator registers */
	__asm volatile("MOV #0x23456789,R15\n"
			"MVTACLO	R15\n"
			"MOV #0xABCDEF01,R15\n"
			"MVTACHI	R15\n");

	/* yield the current thread. */
	k_yield();

	while (1) {
		/* Get Accumulator registers */
		__asm volatile("MVFACMI	R15\n"
				"MOV.L R15, %0"
				: "=r"(thread_2_m));
		__asm volatile("MVFACHI	R15\n"
				"MOV.L R15, %0"
				: "=r"(thread_2_h));

		/* Set Event flag */
		k_event_post(&my_event, 0x010);
		k_sleep(K_SECONDS(10U));
	}
}

/**
 * @brief Test switching accumulator
 *
 * This test verifies various assert macros provided by ztest.
 *
 */
ZTEST(rx_acc_tests, test_counting_value)
{

	uint32_t events;

	k_event_init(&my_event);

	k_tid_t tid_1 = k_thread_create(&thread_1, tstack_thread_1, STACK_SIZE, thread_1_entry,
					NULL, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);

	k_tid_t tid_2 = k_thread_create(&thread_2, tstack_thread_2, STACK_SIZE, thread_2_entry,
					NULL, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);

	k_event_wait_all(&my_event, 0x11, false, K_FOREVER);

	/* cleanup environment */
	k_thread_abort(tid_1);
	k_thread_abort(tid_2);

	/* Asset the value */
	zassert_equal(thread_1_m, 0x567890AB, "Failed thread_1_m");
	zassert_equal(thread_1_h, 0x12345678, "Failed thread_1_h");
	zassert_equal(thread_2_m, 0xEF012345, "Failed thread_2_m");
	zassert_equal(thread_2_h, 0xABCDEF01, "Failed thread_2_h");
}
