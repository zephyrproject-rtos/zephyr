/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/lockdep.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define INSTANCE_LIMIT CONFIG_LOCKDEP_MAX_LOCKS
#define HELD_LIMIT     CONFIG_LOCKDEP_MAX_HELD_LOCKS

static struct k_spinlock instance_locks[INSTANCE_LIMIT + 1];
static struct k_spinlock dependency_locks[INSTANCE_LIMIT];
static struct k_spinlock held_locks[HELD_LIMIT + 1];

static struct z_lockdep_stats get_stats(void)
{
	struct z_lockdep_stats stats;

	z_lockdep_stats_get(&stats);
	return stats;
}

ZTEST(lockdep_overflow, test_instance_table_overflow_is_bounded)
{
	struct z_lockdep_stats stats;
	k_spinlock_key_t key;

	z_lockdep_reset();
	for (size_t i = 0U; i < ARRAY_SIZE(instance_locks); i++) {
		instance_locks[i] = (struct k_spinlock){};
	}

	for (size_t i = 0U; i < ARRAY_SIZE(instance_locks); i++) {
		key = k_spin_lock(&instance_locks[i]);
		k_spin_unlock(&instance_locks[i], key);
	}

	stats = get_stats();
	zassert_equal(stats.lock_instances, INSTANCE_LIMIT);
	zassert_equal(stats.dependencies, 0U);
	zassert_equal(stats.errors, 0U);
	zassert_equal(stats.overflows, 1U);
}

ZTEST(lockdep_overflow, test_dependency_table_overflow_is_bounded)
{
	struct z_lockdep_stats stats;

	z_lockdep_reset();
	for (size_t i = 0U; i < ARRAY_SIZE(dependency_locks); i++) {
		dependency_locks[i] = (struct k_spinlock){};
	}

	/* Every i < j pair creates one unique acyclic dependency. */
	for (size_t i = 0U; i < ARRAY_SIZE(dependency_locks); i++) {
		for (size_t j = i + 1U; j < ARRAY_SIZE(dependency_locks); j++) {
			k_spinlock_key_t first = k_spin_lock(&dependency_locks[i]);
			k_spinlock_key_t second = k_spin_lock(&dependency_locks[j]);

			k_spin_unlock(&dependency_locks[j], second);
			k_spin_unlock(&dependency_locks[i], first);
		}
	}

	stats = get_stats();
	zassert_equal(stats.lock_instances, INSTANCE_LIMIT);
	zassert_equal(stats.dependencies, CONFIG_LOCKDEP_MAX_DEPENDENCIES);
	zassert_equal(stats.errors, 0U);
	zassert_equal(stats.overflows, 1U);
}

ZTEST(lockdep_overflow, test_held_stack_overflow_recovers_on_unlock)
{
	k_spinlock_key_t keys[HELD_LIMIT + 1];
	struct z_lockdep_stats stats;

	z_lockdep_reset();
	for (size_t i = 0U; i < ARRAY_SIZE(held_locks); i++) {
		held_locks[i] = (struct k_spinlock){};
		keys[i] = k_spin_lock(&held_locks[i]);
	}

	for (size_t i = ARRAY_SIZE(held_locks); i > 0U; i--) {
		k_spin_unlock(&held_locks[i - 1U], keys[i - 1U]);
	}

	stats = get_stats();
	zassert_equal(stats.errors, 0U);
	zassert_equal(stats.overflows, 1U);
	zassert_false(z_lockdep_is_held(&held_locks[0]));
}

ZTEST_SUITE(lockdep_overflow, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
