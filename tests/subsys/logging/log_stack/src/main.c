/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/logging/log.h>

/* The idea of this test is to estimate stack usage of logging messages. Each
 * test checks stack usage before and after logging a message. Increase of stack
 * usage is interpreted as stack used by the logging. Test has been used to
 * characterize some common architectures and it will fail if significantly more
 * stack is used.
 *
 * This will help to detect when changes in logging, toolchain or modules used
 * by logging increase stack usage. Failure of the test should lead to investigation
 * to determine if stack usage change is justified. If it is justified, thresholds
 * may be updated.
 */

LOG_MODULE_REGISTER(test);

static size_t simple_usage;
static size_t hexdump_usage;
static size_t more_args_usage;

/* Stack increase margin. */
#ifdef CONFIG_PICOLIBC
#define STACK_USAGE_MARGIN 128
#else
#define STACK_USAGE_MARGIN 16
#endif

static void after(void *data)
{
	PRINT("#elif %sdefined(CONFIG_LOG_MODE_IMMEDIATE) && %sdefined(CONFIG_NO_OPTIMIZATIONS)\n",
		IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? "" : "!",
		IS_ENABLED(CONFIG_NO_OPTIMIZATIONS) ? "" : "!");
	PRINT("#define SIMPLE_USAGE %zu\n", simple_usage);
	PRINT("#define HEXDUMP_USAGE %zu\n", hexdump_usage);
	PRINT("#define MORE_ARGS_USAGE %zu\n", more_args_usage);
}

ZTEST_SUITE(test_log_stack, NULL, NULL, NULL, NULL, after);

#define TEST_LOG_STACK(usage, log_msg) do { \
	size_t unused, unused2; \
	int err; \
	\
	k_msleep(100); \
	err = k_thread_stack_space_get(k_current_get(), &unused); \
	zassert_equal(err, 0, NULL); \
	__DEBRACKET log_msg; \
	\
	k_msleep(100); \
	\
	err = k_thread_stack_space_get(k_current_get(), &unused2); \
	zassert_equal(err, 0, NULL); \
	\
	usage = unused - unused2; \
	PRINT("Stack increase due to log usage: %zu\n", usage); \
} while (0)

#if defined(CONFIG_CPU_CORTEX_M0)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 120
#define HEXDUMP_USAGE 120
#define MORE_ARGS_USAGE 136
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 436
#define HEXDUMP_USAGE 436
#define MORE_ARGS_USAGE 452
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 168
#define HEXDUMP_USAGE 168
#define MORE_ARGS_USAGE 171
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 823
#define HEXDUMP_USAGE 823
#define MORE_ARGS_USAGE 839
#endif

#elif defined(CONFIG_CPU_CORTEX_M3)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 160
#define HEXDUMP_USAGE 160
#define MORE_ARGS_USAGE 176
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 388
#define HEXDUMP_USAGE 388
#define MORE_ARGS_USAGE 404
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 144
#define HEXDUMP_USAGE 88
#define MORE_ARGS_USAGE 144
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 560
#define HEXDUMP_USAGE 560
#define MORE_ARGS_USAGE 576
#endif

#elif defined(CONFIG_X86) && !defined(CONFIG_X86_64)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 72
#define HEXDUMP_USAGE 48
#define MORE_ARGS_USAGE 88
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 420
#define HEXDUMP_USAGE 416
#define MORE_ARGS_USAGE 456
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 256
#define HEXDUMP_USAGE 240
#define MORE_ARGS_USAGE 272
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 876
#define HEXDUMP_USAGE 860
#define MORE_ARGS_USAGE 908
#endif

#elif defined(CONFIG_X86_64)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 168
#define HEXDUMP_USAGE 136
#define MORE_ARGS_USAGE 200
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1088
#define HEXDUMP_USAGE 1088
#define MORE_ARGS_USAGE 1152
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 576
#define HEXDUMP_USAGE 576
#define MORE_ARGS_USAGE 608
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1552
#define HEXDUMP_USAGE 1536
#define MORE_ARGS_USAGE 1616
#endif

#elif defined(CONFIG_RISCV) && !defined(CONFIG_64BIT) && !defined(CONFIG_SMP)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 136
#define HEXDUMP_USAGE 136
#define MORE_ARGS_USAGE 152
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 456
#define HEXDUMP_USAGE 456
#define MORE_ARGS_USAGE 472
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 240
#define HEXDUMP_USAGE 240
#define MORE_ARGS_USAGE 240
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 880
#define HEXDUMP_USAGE 880
#define MORE_ARGS_USAGE 896
#endif

#elif defined(CONFIG_RISCV) && !defined(CONFIG_64BIT) && defined(CONFIG_SMP)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 96
#define HEXDUMP_USAGE 96
#define MORE_ARGS_USAGE 112
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 456
#define HEXDUMP_USAGE 456
#define MORE_ARGS_USAGE 472
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 304
#define HEXDUMP_USAGE 240
#define MORE_ARGS_USAGE 240
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 896
#define HEXDUMP_USAGE 896
#define MORE_ARGS_USAGE 912
#endif

#elif defined(CONFIG_RISCV) && defined(CONFIG_64BIT) && defined(CONFIG_SMP)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 200
#define HEXDUMP_USAGE 168
#define MORE_ARGS_USAGE 232
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 704
#define HEXDUMP_USAGE 768
#define MORE_ARGS_USAGE 736
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 320
#define HEXDUMP_USAGE 320
#define MORE_ARGS_USAGE 320
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1104
#define HEXDUMP_USAGE 1104
#define MORE_ARGS_USAGE 1136
#endif

#elif defined(CONFIG_RISCV) && defined(CONFIG_64BIT)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 240
#define HEXDUMP_USAGE 240
#define MORE_ARGS_USAGE 272
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 704
#define HEXDUMP_USAGE 704
#define MORE_ARGS_USAGE 736
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 320
#define HEXDUMP_USAGE 320
#define MORE_ARGS_USAGE 320
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1104
#define HEXDUMP_USAGE 1104
#define MORE_ARGS_USAGE 1136
#endif

#elif defined(CONFIG_SPARC)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 0
#define HEXDUMP_USAGE 28
#define MORE_ARGS_USAGE 0
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 849
#define HEXDUMP_USAGE 849
#define MORE_ARGS_USAGE 865
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 416
#define HEXDUMP_USAGE 232
#define MORE_ARGS_USAGE 416
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1416
#define HEXDUMP_USAGE 1416
#define MORE_ARGS_USAGE 1432
#endif

#elif defined(CONFIG_XTENSA)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 72
#define HEXDUMP_USAGE 168
#define MORE_ARGS_USAGE 88
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 584
#define HEXDUMP_USAGE 584
#define MORE_ARGS_USAGE 600
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 336
#define HEXDUMP_USAGE 336
#define MORE_ARGS_USAGE 336
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1040
#define HEXDUMP_USAGE 1040
#define MORE_ARGS_USAGE 1056
#endif

#else
/* For other platforms which were not characterized use something big. */
#define SIMPLE_USAGE 2000
#define HEXDUMP_USAGE 2000
#define MORE_ARGS_USAGE 2000
#endif

#define SIMPLE_USAGE_LIMIT (SIMPLE_USAGE + STACK_USAGE_MARGIN)
#define HEXDUMP_USAGE_LIMIT (HEXDUMP_USAGE + STACK_USAGE_MARGIN)
#define MORE_ARGS_USAGE_LIMIT (MORE_ARGS_USAGE + STACK_USAGE_MARGIN)

ZTEST(test_log_stack, test_log_simple)
{
	uint32_t test_u = 1;
	int test_i = 1;

	TEST_LOG_STACK(simple_usage, (LOG_INF("test %u %d", test_u, test_i)));

	zassert_true(simple_usage <= SIMPLE_USAGE_LIMIT, "Usage:%zu, expected:%zu",
			simple_usage, SIMPLE_USAGE_LIMIT);
}

ZTEST(test_log_stack, test_log_hexdump)
{
	uint8_t data[] = {1, 2, 3, 4};

	TEST_LOG_STACK(hexdump_usage, (LOG_HEXDUMP_INF(data, sizeof(data), "description")));

	zassert_true(hexdump_usage <= HEXDUMP_USAGE_LIMIT, "Usage:%zu, expected:%zu",
			hexdump_usage, HEXDUMP_USAGE_LIMIT);
}

ZTEST(test_log_stack, test_log_more_args)
{
	TEST_LOG_STACK(more_args_usage, (LOG_INF("test %d %d %d %d %d %d",
				k_uptime_get_32(), k_uptime_get_32(),
				k_uptime_get_32(), k_uptime_get_32(),
				k_uptime_get_32(), k_uptime_get_32())));

	zassert_true(more_args_usage <= MORE_ARGS_USAGE_LIMIT, "Usage:%zu, expected:%zu",
			more_args_usage, MORE_ARGS_USAGE_LIMIT);
}
