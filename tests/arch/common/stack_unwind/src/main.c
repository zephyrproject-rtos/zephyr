/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>

#include <zephyr/kernel.h>

#if defined(STACK_UNWIND_LEAF_TEST)
extern void leaf_fault(void);

static void (*volatile leaf_fault_call)(void) = leaf_fault;

static void __noinline leaf_caller(void)
{
	leaf_fault_call();
	printf("unexpected return\n");
}
#elif defined(STACK_UNWIND_THREAD_TEST)

#include <zephyr/ztest.h>
#include <zephyr/linker/linker-defs.h>

#define MAX_THREAD_FRAMES CONFIG_ARCH_STACKWALK_MAX_FRAMES

static uintptr_t trace[MAX_THREAD_FRAMES];
static int trace_cnt;

/*
 * Return address of the caller of level1/level2/level3, recorded by each
 * level: [0] is the call site in main, [1] in level1, [2] in level2.
 */
static uintptr_t callers[3];

static bool collect_frame(void *cookie, unsigned long addr)
{
	ARG_UNUSED(cookie);

	if (trace_cnt < MAX_THREAD_FRAMES) {
		trace[trace_cnt++] = (uintptr_t)addr;
	}

	return true;
}

static bool trace_has_chain(void)
{
	int level = 0;

	/*
	 * The walk reports the innermost caller first, so the recorded call
	 * sites must show up in reverse order.
	 */
	for (int i = 0; (i < trace_cnt) && (level < ARRAY_SIZE(callers)); i++) {
		if (trace[i] == callers[ARRAY_SIZE(callers) - 1 - level]) {
			level++;
		}
	}

	return level == ARRAY_SIZE(callers);
}

/* Unwind from a plain thread context, not from an exception frame. */
static void __noinline level3(void)
{
	callers[2] = (uintptr_t)__builtin_return_address(0);

	trace_cnt = 0;
	arch_stack_walk(collect_frame, NULL, k_current_get(), NULL);

	for (int i = 0; i < trace_cnt; i++) {
		printf("  %d: 0x%lx\n", i, (unsigned long)trace[i]);
	}
}

static void __noinline level2(void)
{
	callers[1] = (uintptr_t)__builtin_return_address(0);
	level3();
}

static void __noinline level1(void)
{
	callers[0] = (uintptr_t)__builtin_return_address(0);
	level2();
}

ZTEST(stack_unwind_thread, test_call_chain)
{
	level1();

	zassert_true(trace_cnt > 0, "arch_stack_walk() captured no frame");

	for (int i = 0; i < trace_cnt; i++) {
		zassert_true(trace[i] >= (uintptr_t)__text_region_start &&
				     trace[i] < (uintptr_t)__text_region_end,
			     "frame %d (0x%lx) is outside the text region", i,
			     (unsigned long)trace[i]);
	}

	zassert_true(trace_has_chain(),
		     "captured frames do not contain the caller chain in order");
}

ZTEST_SUITE(stack_unwind_thread, NULL, NULL, NULL, NULL, NULL);
#else
static void func1(int a);
static void func2(int a);

static void __noinline func2(int a)
{
	printf("%d: %s\n", a, __func__);

	if (a >= 5) {
#if defined(STACK_UNWIND_NONLEAF_TEST)
		__asm__ volatile(".word 0");
#else
		k_oops();
#endif
	}

	func1(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}

static void __noinline func1(int a)
{
	printf("%d: %s\n", a, __func__);
	func2(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}
#endif

/* ztest provides its own main() which runs the registered suites. */
#if !defined(STACK_UNWIND_THREAD_TEST)
int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

#if defined(STACK_UNWIND_LEAF_TEST)
	leaf_caller();
#else
	func1(1);
#endif

	return 0;
}
#endif
