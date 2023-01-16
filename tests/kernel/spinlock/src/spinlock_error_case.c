/*
 * Copyright (c) 2018,2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/ztest_test_new.h"
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/spinlock.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

static struct k_spinlock lock;
static struct k_spinlock mylock;
static k_spinlock_key_t key;

/* Like all spin locks in Zephyr (and things that directly hold them), this must
 * be placed globally in code paths that intel_adsp runs to be valid. Even if
 * only used in a local function context as is the case here when SPIN_VALIDATE
 * and KERNEL_COHERENCE are enabled.
 *
 * When both are enabled a check to verify that a spin lock is placed
 * in coherent (uncached) memory is done and asserted on. Spin locks placed
 * on a stack will fail on platforms where KERNEL_COHERENCE is needed such
 * as intel_adsp.
 *
 * See kernel/Kconfig KERNEL_COHERENCE and subsys/debug/Kconfig SPIN_VALIDATE
 * for more details.
 */
#ifdef CONFIG_SPIN_LOCK_TIME_LIMIT
	static struct k_spinlock timeout_lock;
#endif


static ZTEST_DMEM volatile bool valid_assert;
static ZTEST_DMEM volatile bool unlock_after_assert;


static inline void set_assert_valid(bool valid, bool unlock)
{
	valid_assert = valid;
	unlock_after_assert = unlock;
}

static void action_after_assert_fail(void)
{
	if (unlock_after_assert) {
		k_spin_unlock(&lock, key);
	}

	ztest_test_pass();
}

#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
	ARG_UNUSED(file);
	ARG_UNUSED(line);
#endif

	printk("Caught an assert.\n");

	if (valid_assert) {
		valid_assert = false; /* reset back to normal */
		printk("Assert error expected as part of test case.\n");

		/* do some action after fatal error happened */
		action_after_assert_fail();
	} else {
		printk("Assert failed was unexpected, aborting...\n");
#ifdef CONFIG_USERSPACE
	/* User threads aren't allowed to induce kernel panics; generate
	 * an oops instead.
	 */
		if (k_is_user_context()) {
			k_oops();
		}
#endif
		k_panic();
	}
}


/**
 * @brief Test spinlock cannot be recursive
 *
 * @details Validate using spinlock recursive will trigger assertion.
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see k_spin_lock()
 */
ZTEST(spinlock, test_spinlock_no_recursive)
{
	k_spinlock_key_t re;

	key = k_spin_lock(&lock);

	set_assert_valid(true, true);
	re = k_spin_lock(&lock);

	ztest_test_fail();
}

/**
 * @brief Test unlocking incorrect spinlock
 *
 * @details Validate unlocking incorrect spinlock will trigger assertion.
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see k_spin_unlock()
 */
ZTEST(spinlock, test_spinlock_unlock_error)
{
	key = k_spin_lock(&lock);

	set_assert_valid(true, true);
	k_spin_unlock(&mylock, key);

	ztest_test_fail();
}

/**
 * @brief Test unlocking incorrect spinlock
 *
 * @details Validate unlocking incorrect spinlock will trigger assertion.
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see k_spin_release()
 */
ZTEST(spinlock, test_spinlock_release_error)
{
	key = k_spin_lock(&lock);

	set_assert_valid(true, true);
	k_spin_release(&mylock);

	ztest_test_fail();
}


/**
 * @brief Test unlocking spinlock held over the time limit
 *
 * @details Validate unlocking spinlock held past the time limit will trigger
 * assertion.
 *
 * @ingroup kernel_spinlock_tests
 *
 * @see k_spin_unlock()
 */
ZTEST(spinlock, test_spinlock_lock_time_limit)
{
#ifndef CONFIG_SPIN_LOCK_TIME_LIMIT
	ztest_test_skip();
	return;
#else
	if (CONFIG_SPIN_LOCK_TIME_LIMIT == 0) {
		ztest_test_skip();
		return;
	}



	TC_PRINT("testing lock time limit, limit is %d!\n", CONFIG_SPIN_LOCK_TIME_LIMIT);


	key = k_spin_lock(&timeout_lock);

	/* spin here a while, the spin lock limit is in terms of system clock
	 * not core clock. So a multiplier is needed here to ensure things
	 * go well past the time limit.
	 */
	for (volatile int i = 0; i < CONFIG_SPIN_LOCK_TIME_LIMIT*10; i++) {
	}

	set_assert_valid(true, false);
	k_spin_unlock(&timeout_lock, key);

	ztest_test_fail();
#endif
}
