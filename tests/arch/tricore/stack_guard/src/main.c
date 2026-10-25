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

#define GUARD_STACK_SIZE 1024
#define FRAME_BYTES      64

static K_THREAD_STACK_DEFINE(guard_stack, GUARD_STACK_SIZE);
static struct k_thread guard_thread;
static volatile uint32_t sink;
static volatile unsigned int hook_reason = UINT32_MAX;
static volatile uintptr_t lowest_sp = UINTPTR_MAX;

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	hook_reason = reason;
}

/* Each level puts FRAME_BYTES on the stack */
static uint32_t consume(uint32_t depth)
{
	volatile uint8_t buf[FRAME_BYTES];

	buf[0] = (uint8_t)depth;
	buf[FRAME_BYTES - 1] = (uint8_t)depth;
	lowest_sp = (uintptr_t)&buf[0];

	/* Bound at four stack sizes; the guard stops the thread long before */
	if (depth > (4 * GUARD_STACK_SIZE) / FRAME_BYTES) {
		return 0;
	}

	return buf[0] + buf[FRAME_BYTES - 1] + consume(depth + 1);
}

static void guard_entry(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	ztest_set_fault_valid(true);
	sink = consume(0);

	zassert_unreachable("stack overflow not detected");
}

ZTEST(tricore_stack_guard, test_thread_stack_overflow)
{
	k_tid_t t = k_thread_create(&guard_thread, guard_stack, GUARD_STACK_SIZE, guard_entry,
				    NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	k_thread_join(t, K_FOREVER);

	zassert_equal(hook_reason, K_ERR_STACK_CHK_FAIL, "reason %u instead of a stack overflow",
		      hook_reason);
	zassert_true(lowest_sp >= guard_thread.stack_info.start - CONFIG_MPU_STACK_GUARD_MIN_SIZE,
		     "stack store at 0x%lx below the guard", (unsigned long)lowest_sp);
}

ZTEST_SUITE(tricore_stack_guard, NULL, NULL, NULL, NULL, NULL);
