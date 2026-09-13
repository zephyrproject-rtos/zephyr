/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* k_stack push and pop, uncontended. */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(stack, NULL, NULL);

#define STACK_DEPTH 4

K_STACK_DEFINE(stack, STACK_DEPTH);

static void stack_fill(void)
{
	(void)k_stack_push(&stack, (stack_data_t)0xbeef);
}

static void stack_drain(void)
{
	stack_data_t value;

	(void)k_stack_pop(&stack, &value, K_NO_WAIT);
}

ZTEST_BENCHMARK(stack, push, BENCH_SAMPLES, NULL, stack_drain)
{
	(void)k_stack_push(&stack, (stack_data_t)0xbeef);
}

ZTEST_BENCHMARK(stack, pop, BENCH_SAMPLES, stack_fill, NULL)
{
	stack_data_t value;

	(void)k_stack_pop(&stack, &value, K_NO_WAIT);
}
