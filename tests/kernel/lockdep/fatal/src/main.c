/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/lockdep.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

static struct k_mutex mutex_a;
static struct k_mutex mutex_b;
static struct k_spinlock recursive_spin;

/*
 * The expected fatal aborts the test thread while it still owns a lock. The
 * hook must not touch kernel locking primitives: on several architectures it
 * runs in exception context where k_mutex_unlock() is not permitted. Leftover
 * lock state is reclaimed by lockdep_fatal_before() instead.
 */
void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);
}

static void lockdep_fatal_before(void *fixture)
{
	ARG_UNUSED(fixture);
	z_lockdep_reset();
	zassert_ok(k_mutex_init(&mutex_a));
	zassert_ok(k_mutex_init(&mutex_b));
	recursive_spin = (struct k_spinlock){};
}

ZTEST(lockdep_fatal, test_cycle_panics_when_configured)
{
	zassert_ok(k_mutex_lock(&mutex_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&mutex_b, K_FOREVER));
	zassert_ok(k_mutex_unlock(&mutex_b));
	zassert_ok(k_mutex_unlock(&mutex_a));

	zassert_ok(k_mutex_lock(&mutex_b, K_FOREVER));
	ztest_set_fault_valid(true);
	(void)k_mutex_lock(&mutex_a, K_FOREVER);
	zassert_unreachable("lockdep cycle should have panicked");
}

ZTEST(lockdep_fatal, test_recursive_spinlock_always_panics)
{
	(void)k_spin_lock(&recursive_spin);
	ztest_set_fault_valid(true);
	(void)k_spin_lock(&recursive_spin);
	zassert_unreachable("recursive spinlock should have panicked");
}

ZTEST_SUITE(lockdep_fatal, NULL, NULL, lockdep_fatal_before, NULL, NULL);
