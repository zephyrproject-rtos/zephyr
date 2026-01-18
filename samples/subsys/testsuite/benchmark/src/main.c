/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>

#include <zephyr/timing/timing.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_benchmarks, LOG_LEVEL_DBG);

static void suit_setup(void)
{
	LOG_DBG("Setting up math benchmarks suite");
}

static void suit_teardown(void)
{
	LOG_DBG("Tearing down math benchmarks suite");
}

ZTEST_BENCHMARK_SUITE(math_benchmarks, suit_setup, suit_teardown);

ZTEST_BENCHMARK(math_benchmarks, void_test, 100)
{
	/* Intentionally left blank */
}

ZTEST_BENCHMARK(math_benchmarks, benchmark_stack_add, 100)
{
	int a = 5;
	int b = 10;
	int c = 0;

	c += a + b;
}

ZTEST_BENCHMARK(math_benchmarks, benchmark_stack_sub, 100)
{
	int a = 10;
	int b = 5;
	int c = 0;

	c += a - b;
}

struct container {
	int a;
	int b;
	int res;
};

static struct container data;

static void setup_fn(void)
{
	data.a = 20;
	data.b = 15;
	data.res = 0;
}

static void teardown_fn(void)
{
	memset(&data, 0, sizeof(data));
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(math_benchmarks, global_subtraction, 100, setup_fn,
		teardown_fn, NULL)
{
	data.res = data.a - data.b;
}

ZTEST_BENCHMARK_SETUP_TEARDOWN(math_benchmarks, global_addition, 100, setup_fn, teardown_fn, NULL)
{
	data.res = data.a + data.b;
}

ZTEST_BENCHMARK(math_benchmarks, benchmark_pure_asm_add, 100)
{
	__asm__ volatile (
#if defined(CONFIG_X86)
		"add %eax, %eax\n\t"
#elif defined(CONFIG_ARM)
		"add r0, r0, r0\n\t"
#elif defined(CONFIG_RISCV)
		"add a0, a0, a0\n\t"
#else
	#error "Unsupported architecture for pure assembly benchmark"
#endif
	);
}

static uint8_t buffer[32];
#include <zephyr/random/random.h>
ZTEST_BENCHMARK(math_benchmarks, csrand, 100)
{
	for (size_t i = 0; i < 1000; i++) {
		sys_csrand_get(buffer, sizeof(buffer));
	}
}

ZTEST_BENCHMARK_TIMED(math_benchmarks, csrand_timed, 1000)
{
	sys_csrand_get(buffer, sizeof(buffer));
}

ZTEST_BENCHMARK_TIMED(math_benchmarks, add, 1000)
{
	int a = 1;
	int b = 2;
	int c = a + b;
}

ZTEST_BENCHMARK_TIMED(math_benchmarks, benchmark_pure_asm_add_timed, 1000)
{
	__asm__ volatile (
#if defined(CONFIG_X86)
		"add %eax, %eax\n\t"
#elif defined(CONFIG_ARM)
		"add r0, r0, r0\n\t"
#elif defined(CONFIG_RISCV)
		"add a0, a0, a0\n\t"
#else
	#error "Unsupported architecture for pure assembly benchmark"
#endif
	);
}

struct stupid_counter {
	uint64_t iterations;
	struct ztest_benchmark_counter c;
};

static void count_fn(struct ztest_benchmark_counter *counter)
{
	struct stupid_counter *sc = CONTAINER_OF(counter, struct stupid_counter, c);

	sc->iterations++;
}
static void print_fn(struct ztest_benchmark_counter *counter)
{
	struct stupid_counter *sc = CONTAINER_OF(counter, struct stupid_counter, c);

	printk("Total iterations counted: %llu\n", sc->iterations);
}

struct stupid_counter sc = {
	.iterations = 0,
	.c = ZTEST_BENCHMARK_COUNTER_INITIALIZER(count_fn, print_fn),
};

ZTEST_BENCHMARK_SETUP_TEARDOWN(math_benchmarks, addition_counter, 100, setup_fn, teardown_fn, &sc.c)
{
	data.res = data.a + data.b;
}
