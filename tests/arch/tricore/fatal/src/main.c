/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/kernel.h>

#define STACK_SIZE 2048
#define PRIO       5

enum fault_case {
	CASE_BAD_STORE,
	CASE_PSE,
	CASE_OOPS,
};

static K_THREAD_STACK_DEFINE(fault_stack, STACK_SIZE);
static struct k_thread fault_thread;

static void fault_entry(void *p1, void *p2, void *p3)
{
	enum fault_case c = (enum fault_case)(uintptr_t)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);

	switch (c) {
	case CASE_BAD_STORE:
		/* misaligned word store, traps as a synchronous data access
		 * error (Class 4 TIN 2) on AURIX
		 */
		*(volatile int *)1 = 0;
		break;
	case CASE_PSE:
		/* call into peripheral space, program fetch synchronous error
		 * (Class 4 TIN 1)
		 */
		((void (*)(void))0xF0000000)();
		break;
	case CASE_OOPS:
		k_oops();
		break;
	}

	zassert_unreachable("fault case %d did not trap", c);
}

static void run_fault(enum fault_case c)
{
	k_tid_t t = k_thread_create(&fault_thread, fault_stack, STACK_SIZE, fault_entry,
				    (void *)(uintptr_t)c, NULL, NULL, K_PRIO_PREEMPT(PRIO), 0,
				    K_NO_WAIT);

	k_thread_join(t, K_FOREVER);
}

ZTEST(tricore_fatal, test_bad_store_trap)
{
	run_fault(CASE_BAD_STORE);
}

ZTEST(tricore_fatal, test_peripheral_call_trap)
{
	run_fault(CASE_PSE);
}

ZTEST(tricore_fatal, test_kernel_oops)
{
	run_fault(CASE_OOPS);
}

ZTEST_SUITE(tricore_fatal, NULL, NULL, NULL, NULL, NULL);
