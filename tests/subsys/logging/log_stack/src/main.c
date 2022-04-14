/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <logging/log.h>

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
#define STACK_USAGE_MARGIN 16

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
#define SIMPLE_USAGE 96
#define HEXDUMP_USAGE 96
#define MORE_ARGS_USAGE 112
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 412
#define HEXDUMP_USAGE 412
#define MORE_ARGS_USAGE 428
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 152
#define HEXDUMP_USAGE 152
#define MORE_ARGS_USAGE 152
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 783
#define HEXDUMP_USAGE 783
#define MORE_ARGS_USAGE 799
#endif

#elif defined(CONFIG_CPU_CORTEX_M3)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 88
#define HEXDUMP_USAGE 88
#define MORE_ARGS_USAGE 104
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 388
#define HEXDUMP_USAGE 388
#define MORE_ARGS_USAGE 404
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 120
#define HEXDUMP_USAGE 120
#define MORE_ARGS_USAGE 120
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 528
#define HEXDUMP_USAGE 528
#define MORE_ARGS_USAGE 544
#endif

#elif defined(CONFIG_X86) && !defined(CONFIG_X86_64)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 12
#define HEXDUMP_USAGE 24
#define MORE_ARGS_USAGE 28
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 388
#define HEXDUMP_USAGE 384
#define MORE_ARGS_USAGE 416
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 224
#define HEXDUMP_USAGE 224
#define MORE_ARGS_USAGE 240
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 796
#define HEXDUMP_USAGE 796
#define MORE_ARGS_USAGE 828
#endif

#elif defined(CONFIG_X86_64)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 80
#define HEXDUMP_USAGE 80
#define MORE_ARGS_USAGE 112
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1088
#define HEXDUMP_USAGE 1088
#define MORE_ARGS_USAGE 1152
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 528
#define HEXDUMP_USAGE 528
#define MORE_ARGS_USAGE 560
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1440
#define HEXDUMP_USAGE 1440
#define MORE_ARGS_USAGE 1504
#endif

#elif defined(CONFIG_RISCV) && !defined(CONFIG_64BIT) && !defined(CONFIG_SMP)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 56
#define HEXDUMP_USAGE 84
#define MORE_ARGS_USAGE 72
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 456
#define HEXDUMP_USAGE 456
#define MORE_ARGS_USAGE 472
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 208
#define HEXDUMP_USAGE 208
#define MORE_ARGS_USAGE 208
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 844
#define HEXDUMP_USAGE 844
#define MORE_ARGS_USAGE 860
#endif

#elif defined(CONFIG_RISCV) && !defined(CONFIG_64BIT) && defined(CONFIG_SMP)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 44
#define HEXDUMP_USAGE 60
#define MORE_ARGS_USAGE 60
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 456
#define HEXDUMP_USAGE 456
#define MORE_ARGS_USAGE 472
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 208
#define HEXDUMP_USAGE 208
#define MORE_ARGS_USAGE 208
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 876
#define HEXDUMP_USAGE 848
#define MORE_ARGS_USAGE 864
#endif

#elif defined(CONFIG_RISCV) && defined(CONFIG_64BIT) && defined(CONFIG_SMP)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 112
#define HEXDUMP_USAGE 112
#define MORE_ARGS_USAGE 144
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 704
#define HEXDUMP_USAGE 768
#define MORE_ARGS_USAGE 736
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 272
#define HEXDUMP_USAGE 272
#define MORE_ARGS_USAGE 272
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1040
#define HEXDUMP_USAGE 1040
#define MORE_ARGS_USAGE 1072
#endif

#elif defined(CONFIG_RISCV) && defined(CONFIG_64BIT)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 152
#define HEXDUMP_USAGE 152
#define MORE_ARGS_USAGE 184
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 704
#define HEXDUMP_USAGE 704
#define MORE_ARGS_USAGE 736
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 272
#define HEXDUMP_USAGE 272
#define MORE_ARGS_USAGE 272
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1040
#define HEXDUMP_USAGE 1040
#define MORE_ARGS_USAGE 1072
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
#define SIMPLE_USAGE 344
#define HEXDUMP_USAGE 344
#define MORE_ARGS_USAGE 344
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 1312
#define HEXDUMP_USAGE 1312
#define MORE_ARGS_USAGE 1328
#endif

#elif defined(CONFIG_XTENSA)

#if !defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 72
#define HEXDUMP_USAGE 168
#define MORE_ARGS_USAGE 88
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && !defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 504
#define HEXDUMP_USAGE 504
#define MORE_ARGS_USAGE 520
#elif !defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 336
#define HEXDUMP_USAGE 336
#define MORE_ARGS_USAGE 336
#elif defined(CONFIG_LOG_MODE_IMMEDIATE) && defined(CONFIG_NO_OPTIMIZATIONS)
#define SIMPLE_USAGE 944
#define HEXDUMP_USAGE 944
#define MORE_ARGS_USAGE 960
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
