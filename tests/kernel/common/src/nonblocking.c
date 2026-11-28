/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/**
 * @brief Test calls from a __nonblocking function
 *
 * @defgroup test_nonblocking __nonblocking tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */

static struct k_spinlock nb_lock;
static atomic_t nb_count;
static K_SEM_DEFINE(nb_sem, 0, 1);

/*
 * With CONFIG_FUNCTION_EFFECT_ANALYSIS, Clang 20 and later reject any call in
 * here that may block, such as k_sem_take().
 */
static bool nb_work(void) __nonblocking
{
	k_spinlock_key_t key;
	unsigned int irq_key;

	key = k_spin_lock(&nb_lock);
	(void)atomic_inc(&nb_count);
	k_spin_unlock(&nb_lock, key);

	irq_key = irq_lock();
	(void)atomic_inc(&nb_count);
	irq_unlock(irq_key);

	k_sem_give(&nb_sem);

	return k_is_in_isr();
}

/**
 * @brief Call the nonblocking kernel API from a __nonblocking function
 *
 * The build checks that nb_work() only calls functions marked __nonblocking;
 * at run time, check that the calls took effect.
 */
ZTEST(nonblocking, test_nonblocking_calls)
{
	zassert_false(nb_work());
	zassert_equal(atomic_get(&nb_count), 2);
	zassert_ok(k_sem_take(&nb_sem, K_NO_WAIT));
}

extern void *common_setup(void);
ZTEST_SUITE(nonblocking, NULL, common_setup, NULL, NULL, NULL);
