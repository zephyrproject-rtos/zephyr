/*
 * Copyright (c) 2018,2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <ztest.h>
#include <zephyr/spinlock.h>

BUILD_ASSERT(CONFIG_MP_NUM_CPUS > 1);

static struct k_spinlock lock;
static struct k_spinlock mylock;
static k_spinlock_key_t key;

static ZTEST_DMEM volatile bool valid_assert;

static inline void set_assert_valid(bool valid)
{
	valid_assert = valid;
}

static void action_after_assert_fail(void)
{
	k_spin_unlock(&lock, key);

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

	set_assert_valid(true);
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

	set_assert_valid(true);
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

	set_assert_valid(true);
	k_spin_release(&mylock);

	ztest_test_fail();
}
