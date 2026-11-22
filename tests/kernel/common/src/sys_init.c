/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static int init_seq;
static int early_seq;
static int pre_kernel_1_seq;
static int pre_kernel_2_seq;
static int post_kernel_seq;
static int application_seq;
#ifdef CONFIG_SMP
static int smp_seq;
#endif

#define RECORD_INIT_LEVEL(name)                                                \
	static int record_##name(void)                                         \
	{                                                                      \
		name##_seq = ++init_seq;                                       \
		return 0;                                                      \
	}

RECORD_INIT_LEVEL(early)
RECORD_INIT_LEVEL(pre_kernel_1)
RECORD_INIT_LEVEL(pre_kernel_2)
RECORD_INIT_LEVEL(post_kernel)
RECORD_INIT_LEVEL(application)
#ifdef CONFIG_SMP
RECORD_INIT_LEVEL(smp)
#endif

SYS_INIT(record_early, EARLY, 0);
SYS_INIT(record_pre_kernel_1, PRE_KERNEL_1, 0);
SYS_INIT(record_pre_kernel_2, PRE_KERNEL_2, 0);
SYS_INIT(record_post_kernel, POST_KERNEL, 0);
SYS_INIT(record_application, APPLICATION, 0);
#ifdef CONFIG_SMP
SYS_INIT(record_smp, SMP, 0);
#endif

/**
 * @brief Verify the initialization levels run in order before the test.
 *
 * @ingroup kernel_init_tests
 *
 * @details
 * An init hook at every initialization level records the order it was
 * invoked in. By the time the test runs from the main thread, every
 * level must have run, in the defined order: early, the two pre-kernel
 * levels, post-kernel, application and, with SMP enabled, the
 * multiprocessor level. The level ordinals must follow the same order.
 *
 * Test steps:
 * - Register a SYS_INIT() hook at each initialization level that
 *   records a running sequence number.
 * - Check every hook ran and their sequence is strictly ascending.
 * - Check INIT_LEVEL_ORD() orders the levels the same way.
 *
 * Expected result:
 * - All initialization levels completed, in order, before the
 *   application entry point ran.
 *
 * @see SYS_INIT()
 * @see INIT_LEVEL_ORD()
 * @verifies ZEP-SRS-12-8
 * @verifies ZEP-SRS-12-13
 * @verifies ZEP-SRS-12-14
 */
ZTEST(common, test_sys_init_levels)
{
	zassert_true(early_seq > 0, "EARLY hook did not run");
	zassert_true(pre_kernel_1_seq > early_seq,
		     "PRE_KERNEL_1 did not follow EARLY");
	zassert_true(pre_kernel_2_seq > pre_kernel_1_seq,
		     "PRE_KERNEL_2 did not follow PRE_KERNEL_1");
	zassert_true(post_kernel_seq > pre_kernel_2_seq,
		     "POST_KERNEL did not follow PRE_KERNEL_2");
	zassert_true(application_seq > post_kernel_seq,
		     "APPLICATION did not follow POST_KERNEL");
#ifdef CONFIG_SMP
	zassert_true(smp_seq > application_seq,
		     "SMP did not follow APPLICATION");
#endif

	zassert_true(INIT_LEVEL_ORD(EARLY) < INIT_LEVEL_ORD(PRE_KERNEL_1),
		     "unexpected EARLY ordinal");
	zassert_true(INIT_LEVEL_ORD(PRE_KERNEL_1) < INIT_LEVEL_ORD(PRE_KERNEL_2),
		     "unexpected PRE_KERNEL_1 ordinal");
	zassert_true(INIT_LEVEL_ORD(PRE_KERNEL_2) < INIT_LEVEL_ORD(POST_KERNEL),
		     "unexpected PRE_KERNEL_2 ordinal");
	zassert_true(INIT_LEVEL_ORD(POST_KERNEL) < INIT_LEVEL_ORD(APPLICATION),
		     "unexpected POST_KERNEL ordinal");
	zassert_true(INIT_LEVEL_ORD(APPLICATION) < INIT_LEVEL_ORD(SMP),
		     "unexpected APPLICATION ordinal");
}
