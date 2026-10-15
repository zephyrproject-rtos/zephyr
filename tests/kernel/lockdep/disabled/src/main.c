/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/lockdep.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * With CONFIG_LOCKDEP=n the validator contributes nothing: no per-thread
 * state, no symbols, and the locking primitives behave exactly as before.
 */
ZTEST(lockdep_disabled, test_disabled_locking_is_unaffected)
{
	struct k_mutex mutex;
	struct k_spinlock spin = {};
	k_spinlock_key_t spin_key;

	zassert_false(IS_ENABLED(CONFIG_LOCKDEP));
	BUILD_ASSERT(!IS_ENABLED(CONFIG_LOCKDEP));

	zassert_ok(k_mutex_init(&mutex));
	zassert_ok(k_mutex_lock(&mutex, K_FOREVER));
	zassert_ok(k_mutex_lock(&mutex, K_NO_WAIT));
	zassert_ok(k_mutex_unlock(&mutex));
	zassert_ok(k_mutex_unlock(&mutex));

	spin_key = k_spin_lock(&spin);
	k_spin_unlock(&spin, spin_key);
	zassert_true(k_spin_trylock(&spin, &spin_key) == 0);
	k_spin_unlock(&spin, spin_key);
}

ZTEST_SUITE(lockdep_disabled, NULL, NULL, NULL, NULL, NULL);
