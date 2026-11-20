/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/lockdep.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static struct k_mutex lock_a;
static struct k_mutex lock_b;
static struct k_mutex lock_c;
static struct k_spinlock spin_a;
static struct k_spinlock spin_b;
static struct k_thread holder_thread;
static struct k_sem holder_ready;
static struct k_sem holder_release;
K_MUTEX_DEFINE(static_mutex_a);
K_MUTEX_DEFINE(static_mutex_b);
K_THREAD_STACK_DEFINE(holder_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);

static void get_stats(struct z_lockdep_stats *stats)
{
	z_lockdep_stats_get(stats);
}

static void reset_mutexes(void)
{
	z_lockdep_reset();
	zassert_ok(k_mutex_init(&lock_a));
	zassert_ok(k_mutex_init(&lock_b));
	zassert_ok(k_mutex_init(&lock_c));
}

static uint32_t error_count(void)
{
	struct z_lockdep_stats stats;

	get_stats(&stats);
	return stats.errors;
}

static void lock_mutex_pair(struct k_mutex *first, struct k_mutex *second)
{
	zassert_ok(k_mutex_lock(first, K_FOREVER));
	zassert_ok(k_mutex_lock(second, K_FOREVER));
	zassert_ok(k_mutex_unlock(second));
	zassert_ok(k_mutex_unlock(first));
}

ZTEST(lockdep, test_consistent_mutex_order)
{
	struct z_lockdep_stats stats;

	reset_mutexes();
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_true(z_lockdep_is_held(&lock_a));
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_ok(k_mutex_unlock(&lock_b));
	zassert_ok(k_mutex_unlock(&lock_a));

	get_stats(&stats);
	zassert_equal(stats.errors, 0U);
	zassert_true(stats.dependencies > 0U);
}

ZTEST(lockdep, test_duplicate_dependency_is_not_recorded_twice)
{
	struct z_lockdep_stats first;
	struct z_lockdep_stats second;

	reset_mutexes();
	lock_mutex_pair(&lock_a, &lock_b);
	get_stats(&first);
	lock_mutex_pair(&lock_a, &lock_b);
	get_stats(&second);

	zassert_equal(second.errors, 0U);
	zassert_equal(second.dependencies, first.dependencies);
}

ZTEST(lockdep, test_mutex_lock_inversion)
{
	reset_mutexes();

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_ok(k_mutex_unlock(&lock_b));
	zassert_ok(k_mutex_unlock(&lock_a));

	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_true(error_count() > 0U);
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_ok(k_mutex_unlock(&lock_b));
}

ZTEST(lockdep, test_transitive_mutex_cycle)
{
	reset_mutexes();

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_ok(k_mutex_unlock(&lock_b));
	zassert_ok(k_mutex_unlock(&lock_a));

	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_c, K_FOREVER));
	zassert_ok(k_mutex_unlock(&lock_c));
	zassert_ok(k_mutex_unlock(&lock_b));

	zassert_ok(k_mutex_lock(&lock_c, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_true(error_count() > 0U);
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_ok(k_mutex_unlock(&lock_c));
}

ZTEST(lockdep, test_recursive_mutex_is_allowed)
{
	reset_mutexes();

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_equal(error_count(), 0U);
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_true(z_lockdep_is_held(&lock_a));
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_false(z_lockdep_is_held(&lock_a));
}

static void check_nested_mutex_recursion(k_timeout_t timeout)
{
	reset_mutexes();
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));

	zassert_ok(k_mutex_lock(&lock_a, timeout));
	zassert_equal(error_count(), 0U);
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_true(z_lockdep_is_held(&lock_a));
	zassert_ok(k_mutex_unlock(&lock_b));
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_false(z_lockdep_is_held(&lock_a));
	zassert_equal(error_count(), 0U);

	/* Recursion must not discard the original A -> B dependency. */
	lock_mutex_pair(&lock_b, &lock_a);
	zassert_true(error_count() > 0U);
}

ZTEST(lockdep, test_nested_recursive_mutex_is_allowed)
{
	check_nested_mutex_recursion(K_FOREVER);
}

ZTEST(lockdep, test_nested_recursive_mutex_trylock_is_allowed)
{
	check_nested_mutex_recursion(K_NO_WAIT);
}

ZTEST(lockdep, test_successful_mutex_trylock_adds_dependency)
{
	reset_mutexes();

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_b, K_NO_WAIT));
	zassert_ok(k_mutex_unlock(&lock_b));
	zassert_ok(k_mutex_unlock(&lock_a));

	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_true(error_count() > 0U);
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_ok(k_mutex_unlock(&lock_b));
}

static void mutex_holder(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	k_sem_give(&holder_ready);
	zassert_ok(k_sem_take(&holder_release, K_FOREVER));
	zassert_ok(k_mutex_unlock(&lock_a));
}

ZTEST(lockdep, test_failed_trylock_does_not_add_dependency)
{
	reset_mutexes();
	k_sem_init(&holder_ready, 0, 1);
	k_sem_init(&holder_release, 0, 1);
	k_thread_create(&holder_thread, holder_stack, K_THREAD_STACK_SIZEOF(holder_stack),
			mutex_holder, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&holder_ready, K_FOREVER));

	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_equal(k_mutex_lock(&lock_a, K_NO_WAIT), -EBUSY);
	zassert_ok(k_mutex_unlock(&lock_b));
	k_sem_give(&holder_release);
	zassert_ok(k_thread_join(&holder_thread, K_FOREVER));

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_equal(error_count(), 0U);
	zassert_ok(k_mutex_unlock(&lock_b));
	zassert_ok(k_mutex_unlock(&lock_a));
}

ZTEST(lockdep, test_failed_timed_lock_adds_potential_dependency)
{
	reset_mutexes();
	k_sem_init(&holder_ready, 0, 1);
	k_sem_init(&holder_release, 0, 1);
	k_thread_create(&holder_thread, holder_stack, K_THREAD_STACK_SIZEOF(holder_stack),
			mutex_holder, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&holder_ready, K_FOREVER));

	/* A finite wait can block, so lockdep conservatively learns B -> A. */
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_equal(k_mutex_lock(&lock_a, K_MSEC(10)), -EAGAIN);
	zassert_false(z_lockdep_is_held(&lock_a));
	zassert_ok(k_mutex_unlock(&lock_b));
	k_sem_give(&holder_release);
	zassert_ok(k_thread_join(&holder_thread, K_FOREVER));

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	zassert_true(error_count() > 0U);
	zassert_ok(k_mutex_unlock(&lock_b));
	zassert_ok(k_mutex_unlock(&lock_a));
}

static struct k_sem waiter_started;
static struct k_sem waiter_done;
static int waiter_result;
static bool waiter_saw_lock_held;

static void mutex_waiter(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	k_sem_give(&waiter_started);
	waiter_result = k_mutex_lock(&lock_a, K_FOREVER);
	if (waiter_result == 0) {
		waiter_saw_lock_held = z_lockdep_is_held(&lock_a);
		waiter_result = k_mutex_unlock(&lock_a);
	}
	k_sem_give(&waiter_done);
}

ZTEST(lockdep, test_contended_mutex_handoff_records_new_owner)
{
	reset_mutexes();
	k_sem_init(&waiter_started, 0, 1);
	k_sem_init(&waiter_done, 0, 1);
	waiter_result = -EINVAL;
	waiter_saw_lock_held = false;

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	k_thread_create(&holder_thread, holder_stack, K_THREAD_STACK_SIZEOF(holder_stack),
			mutex_waiter, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&waiter_started, K_FOREVER));
	k_sleep(K_MSEC(1));
	zassert_equal(k_sem_take(&waiter_done, K_NO_WAIT), -EBUSY);
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_ok(k_sem_take(&waiter_done, K_FOREVER));
	zassert_ok(k_thread_join(&holder_thread, K_FOREVER));

	zassert_ok(waiter_result);
	zassert_true(waiter_saw_lock_held);
	/* A blocking wait releases the scheduler spinlock without a LIFO warning. */
	zassert_equal(error_count(), 0U);
}

struct mutex_order_args {
	struct k_mutex *first;
	struct k_mutex *second;
	int result;
};

static void mutex_order_worker(void *arg1, void *unused2, void *unused3)
{
	struct mutex_order_args *args = arg1;

	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	args->result = k_mutex_lock(args->first, K_FOREVER);
	if (args->result != 0) {
		return;
	}
	args->result = k_mutex_lock(args->second, K_FOREVER);
	if (args->result == 0) {
		args->result = k_mutex_unlock(args->second);
	}
	if (k_mutex_unlock(args->first) != 0) {
		args->result = -EINVAL;
	}
}

static void run_mutex_order_thread(struct k_mutex *first, struct k_mutex *second)
{
	struct mutex_order_args args = {
		.first = first,
		.second = second,
		.result = -EINVAL,
	};

	k_thread_create(&holder_thread, holder_stack, K_THREAD_STACK_SIZEOF(holder_stack),
			mutex_order_worker, &args, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&holder_thread, K_FOREVER));
	zassert_ok(args.result);
}

ZTEST(lockdep, test_dependency_graph_is_shared_across_threads)
{
	reset_mutexes();

	run_mutex_order_thread(&lock_a, &lock_b);
	zassert_equal(error_count(), 0U);
	run_mutex_order_thread(&lock_b, &lock_a);
	zassert_true(error_count() > 0U);
}

static bool reused_thread_inherited_lock;

static void abortable_mutex_holder(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&lock_b, K_FOREVER));
	k_sem_give(&holder_ready);
	zassert_ok(k_sem_take(&holder_release, K_FOREVER));
}

static void reused_thread_probe(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	reused_thread_inherited_lock = z_lockdep_is_held(&lock_a) || z_lockdep_is_held(&lock_b);
}

ZTEST(lockdep, test_aborted_thread_reuse_and_mutex_reinitialization)
{
	struct z_lockdep_stats before;
	struct z_lockdep_stats after;

	reset_mutexes();
	k_sem_init(&holder_ready, 0, 1);
	k_sem_init(&holder_release, 0, 1);
	reused_thread_inherited_lock = true;
	k_thread_create(&holder_thread, holder_stack, K_THREAD_STACK_SIZEOF(holder_stack),
			abortable_mutex_holder, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&holder_ready, K_FOREVER));
	k_thread_abort(&holder_thread);
	get_stats(&before);
	zassert_true(before.dependencies > 0U);

	/* Aborting a thread does not release its mutexes; reclaim both explicitly. */
	zassert_ok(k_mutex_init(&lock_a));
	zassert_ok(k_mutex_init(&lock_b));
	get_stats(&after);
	zassert_equal(after.lock_instances + 2U, before.lock_instances);
	zassert_true(after.dependencies < before.dependencies);
	zassert_equal(after.errors, before.errors);

	k_thread_create(&holder_thread, holder_stack, K_THREAD_STACK_SIZEOF(holder_stack),
			reused_thread_probe, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&holder_thread, K_FOREVER));
	zassert_false(reused_thread_inherited_lock);
	lock_mutex_pair(&lock_b, &lock_a);
	zassert_equal(error_count(), 0U);
}

static int init_mutex_at_one_site(struct k_mutex *mutex)
{
	return k_mutex_init(mutex);
}

#ifdef CONFIG_USERSPACE
static struct k_mutex user_mutex_a;
static struct k_mutex user_mutex_b;
static struct k_thread user_thread;
static ZTEST_DMEM int user_init_result_a;
static ZTEST_DMEM int user_init_result_b;
static ZTEST_DMEM int user_inversion_result;
K_THREAD_STACK_DEFINE(user_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);

static void user_mutex_init_worker(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	user_init_result_a = init_mutex_at_one_site(&user_mutex_a);
	user_init_result_b = init_mutex_at_one_site(&user_mutex_b);
	if ((user_init_result_a == 0) && (user_init_result_b == 0)) {
		user_init_result_a = k_mutex_lock(&user_mutex_a, K_FOREVER);
		if (user_init_result_a == 0) {
			user_init_result_b = k_mutex_lock(&user_mutex_b, K_FOREVER);
			if (user_init_result_b == 0) {
				user_init_result_b = k_mutex_unlock(&user_mutex_b);
			}
			user_init_result_a = k_mutex_unlock(&user_mutex_a);
		}
	}
}

ZTEST(lockdep, test_mutex_init_callsite_keeps_instances_distinct_from_user_mode)
{
	struct z_lockdep_stats stats;

	z_lockdep_reset();
	user_init_result_a = -EINVAL;
	user_init_result_b = -EINVAL;
	k_thread_access_grant(k_current_get(), &user_mutex_a, &user_mutex_b);
	k_thread_create(&user_thread, user_stack, K_THREAD_STACK_SIZEOF(user_stack),
			user_mutex_init_worker, NULL, NULL, NULL, K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	zassert_ok(k_thread_join(&user_thread, K_FOREVER));
	zassert_ok(user_init_result_a);
	zassert_ok(user_init_result_b);

	get_stats(&stats);
	zassert_true(stats.lock_instances >= 2U);
	zassert_equal(stats.errors, 0U);
}

static int user_lock_pair(struct k_mutex *first, struct k_mutex *second)
{
	int ret = k_mutex_lock(first, K_FOREVER);

	if (ret != 0) {
		return ret;
	}
	ret = k_mutex_lock(second, K_FOREVER);
	if (ret == 0) {
		ret = k_mutex_unlock(second);
	}
	if (k_mutex_unlock(first) != 0) {
		ret = -EINVAL;
	}
	return ret;
}

static void user_mutex_inversion_worker(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	user_inversion_result = init_mutex_at_one_site(&user_mutex_a);
	if (user_inversion_result != 0) {
		return;
	}
	user_inversion_result = init_mutex_at_one_site(&user_mutex_b);
	if (user_inversion_result != 0) {
		return;
	}
	user_inversion_result = user_lock_pair(&user_mutex_a, &user_mutex_b);
	if (user_inversion_result == 0) {
		user_inversion_result = user_lock_pair(&user_mutex_b, &user_mutex_a);
	}
}

ZTEST(lockdep, test_user_mode_mutex_inversion_is_reported)
{
	z_lockdep_reset();
	user_inversion_result = -EINVAL;
	k_thread_access_grant(k_current_get(), &user_mutex_a, &user_mutex_b);
	k_thread_create(&user_thread, user_stack, K_THREAD_STACK_SIZEOF(user_stack),
			user_mutex_inversion_worker, NULL, NULL, NULL, K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	zassert_ok(k_thread_join(&user_thread, K_FOREVER));
	zassert_ok(user_inversion_result);
	zassert_true(error_count() > 0U);
}
#endif /* CONFIG_USERSPACE */

ZTEST(lockdep, test_mutex_init_callsite_keeps_instances_distinct)
{
	struct z_lockdep_stats stats;
	struct k_mutex a1;
	struct k_mutex a2;
	struct k_mutex b1;
	struct k_mutex b2;

	z_lockdep_reset();
	zassert_ok(init_mutex_at_one_site(&a1));

	/*
	 * With CONFIG_USERSPACE, k_object_init() takes kernel spinlocks that
	 * are registered lazily on first use, so take the baseline after the
	 * first init. The remaining inits must not register anything.
	 */
	get_stats(&stats);
	uint32_t instances_after_first_init = stats.lock_instances;

	zassert_ok(init_mutex_at_one_site(&a2));
	zassert_ok(init_mutex_at_one_site(&b1));
	zassert_ok(init_mutex_at_one_site(&b2));

	/* Initialization alone must not register the mutexes. */
	get_stats(&stats);
	zassert_equal(stats.lock_instances, instances_after_first_init);
	zassert_equal(stats.dependencies, 0U);

	lock_mutex_pair(&a1, &b1);
	lock_mutex_pair(&b2, &a2);
	lock_mutex_pair(&a1, &a2);
	zassert_equal(error_count(), 0U);

	/* Only an inversion involving the same addresses closes a cycle. */
	zassert_ok(k_mutex_lock(&b1, K_FOREVER));
	zassert_ok(k_mutex_lock(&a1, K_FOREVER));
	zassert_true(error_count() > 0U);
	zassert_ok(k_mutex_unlock(&a1));
	zassert_ok(k_mutex_unlock(&b1));
	get_stats(&stats);
	zassert_true(stats.lock_instances >= 4U);
}

ZTEST(lockdep, test_static_mutexes_are_distinct_instances)
{
	struct z_lockdep_stats stats;

	z_lockdep_reset();
	zassert_ok(k_mutex_lock(&static_mutex_a, K_FOREVER));
	zassert_ok(k_mutex_lock(&static_mutex_b, K_FOREVER));
	zassert_ok(k_mutex_unlock(&static_mutex_b));
	zassert_ok(k_mutex_unlock(&static_mutex_a));

	get_stats(&stats);
	zassert_true(stats.lock_instances >= 2U);
	zassert_equal(stats.errors, 0U);
}

ZTEST(lockdep, test_mutex_reinitialization_reclaims_edges_and_remaps_instances)
{
	reset_mutexes();
	lock_mutex_pair(&lock_a, &lock_b);
	lock_mutex_pair(&lock_a, &lock_c);
	zassert_ok(k_mutex_init(&lock_b));

	/* The old A -> B edge is gone, but A -> C survives C's ID move. */
	lock_mutex_pair(&lock_b, &lock_a);
	zassert_equal(error_count(), 0U);
	lock_mutex_pair(&lock_c, &lock_a);
	zassert_true(error_count() > 0U);
}

ZTEST(lockdep, test_spinlock_inversion)
{
	k_spinlock_key_t key_a;
	k_spinlock_key_t key_b;

	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	spin_b = (struct k_spinlock){};

	key_a = k_spin_lock(&spin_a);
	key_b = k_spin_lock(&spin_b);
	k_spin_unlock(&spin_b, key_b);
	k_spin_unlock(&spin_a, key_a);

	key_b = k_spin_lock(&spin_b);
	key_a = k_spin_lock(&spin_a);
	zassert_true(error_count() > 0U);
	k_spin_unlock(&spin_a, key_a);
	k_spin_unlock(&spin_b, key_b);
}

ZTEST(lockdep, test_consistent_spinlock_order_and_held_state)
{
	struct z_lockdep_stats stats;
	k_spinlock_key_t key_a;
	k_spinlock_key_t key_b;

	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	spin_b = (struct k_spinlock){};

	key_a = k_spin_lock(&spin_a);
	zassert_true(z_lockdep_is_held(&spin_a));
	zassert_false(z_lockdep_is_held(&spin_b));
	key_b = k_spin_lock(&spin_b);
	k_spin_unlock(&spin_b, key_b);
	k_spin_unlock(&spin_a, key_a);
	zassert_false(z_lockdep_is_held(&spin_a));

	get_stats(&stats);
	zassert_equal(stats.errors, 0U);
	zassert_true(stats.dependencies > 0U);
}

/* Exercise invalid release sequences without unlocking kernel spinlocks out of order. */
static void simulate_spin_acquire(struct k_spinlock *lock)
{
	z_lockdep_before_acquire(lock, Z_LOCKDEP_SPINLOCK, false, 0U);
	z_lockdep_after_acquire(lock, Z_LOCKDEP_SPINLOCK, false, 0U);
}

ZTEST(lockdep, test_out_of_order_spinlock_release_is_reported)
{
	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	spin_b = (struct k_spinlock){};
	simulate_spin_acquire(&spin_a);
	simulate_spin_acquire(&spin_b);

	z_lockdep_release(&spin_a, Z_LOCKDEP_SPINLOCK);
	zassert_equal(error_count(), 1U);
	zassert_false(z_lockdep_is_held(&spin_a));
	zassert_true(z_lockdep_is_held(&spin_b));
	z_lockdep_release(&spin_b, Z_LOCKDEP_SPINLOCK);
	zassert_equal(error_count(), 1U);
}

ZTEST(lockdep, test_unordered_spinlock_release_is_allowed)
{
	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	spin_b = (struct k_spinlock){};
	simulate_spin_acquire(&spin_a);
	simulate_spin_acquire(&spin_b);

	z_lockdep_release_unordered(&spin_a, Z_LOCKDEP_SPINLOCK);
	z_lockdep_release(&spin_b, Z_LOCKDEP_SPINLOCK);
	zassert_equal(error_count(), 0U);
}

ZTEST(lockdep, test_release_without_matching_acquisition_is_reported)
{
	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	z_lockdep_before_acquire(&spin_a, Z_LOCKDEP_SPINLOCK, true, 0U);

	z_lockdep_release(&spin_a, Z_LOCKDEP_SPINLOCK);
	zassert_equal(error_count(), 1U);
}

ZTEST(lockdep, test_release_as_wrong_lock_type_preserves_held_state)
{
	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	simulate_spin_acquire(&spin_a);

	z_lockdep_release(&spin_a, Z_LOCKDEP_MUTEX);
	zassert_equal(error_count(), 1U);
	zassert_true(z_lockdep_is_held(&spin_a));
	z_lockdep_release(&spin_a, Z_LOCKDEP_SPINLOCK);
	zassert_false(z_lockdep_is_held(&spin_a));
	zassert_equal(error_count(), 1U);
}

ZTEST(lockdep, test_same_address_cannot_be_used_as_different_lock_types)
{
	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	z_lockdep_before_acquire(&spin_a, Z_LOCKDEP_SPINLOCK, true, 0U);
	z_lockdep_before_acquire(&spin_a, Z_LOCKDEP_MUTEX, true, 0U);
	zassert_equal(error_count(), 1U);
}

static bool isr_saw_locks_held;

static void learn_spin_order_from_isr(const void *unused)
{
	k_spinlock_key_t key_a;
	k_spinlock_key_t key_b;

	ARG_UNUSED(unused);
	key_a = k_spin_lock(&spin_a);
	key_b = k_spin_lock(&spin_b);
	isr_saw_locks_held = z_lockdep_is_held(&spin_a) && z_lockdep_is_held(&spin_b);
	k_spin_unlock(&spin_b, key_b);
	k_spin_unlock(&spin_a, key_a);
}

ZTEST(lockdep, test_spinlock_dependency_learned_in_isr)
{
	k_spinlock_key_t key_a;
	k_spinlock_key_t key_b;

	z_lockdep_reset();
	spin_a = (struct k_spinlock){};
	spin_b = (struct k_spinlock){};
	isr_saw_locks_held = false;

	irq_offload(learn_spin_order_from_isr, NULL);
	zassert_true(isr_saw_locks_held);
	zassert_equal(error_count(), 0U);

	key_b = k_spin_lock(&spin_b);
	key_a = k_spin_lock(&spin_a);
	zassert_true(error_count() > 0U);
	k_spin_unlock(&spin_a, key_a);
	k_spin_unlock(&spin_b, key_b);
}

static bool isr_saw_interrupted_mutex;

static void acquire_spin_while_mutex_held_in_isr(const void *unused)
{
	k_spinlock_key_t key;

	ARG_UNUSED(unused);
	isr_saw_interrupted_mutex = z_lockdep_is_held(&lock_a);
	key = k_spin_lock(&spin_a);
	k_spin_unlock(&spin_a, key);
}

ZTEST(lockdep, test_isr_spinlock_depends_on_interrupted_mutex)
{
	k_spinlock_key_t key;

	reset_mutexes();
	spin_a = (struct k_spinlock){};
	isr_saw_interrupted_mutex = false;
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	irq_offload(acquire_spin_while_mutex_held_in_isr, NULL);
	zassert_true(isr_saw_interrupted_mutex);
	zassert_ok(k_mutex_unlock(&lock_a));
	zassert_equal(error_count(), 0U);

	key = k_spin_lock(&spin_a);
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_true(error_count() > 0U);
	zassert_ok(k_mutex_unlock(&lock_a));
	k_spin_unlock(&spin_a, key);
}

ZTEST(lockdep, test_mixed_mutex_spinlock_cycle)
{
	k_spinlock_key_t key;

	reset_mutexes();
	spin_a = (struct k_spinlock){};

	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	key = k_spin_lock(&spin_a);
	k_spin_unlock(&spin_a, key);
	zassert_ok(k_mutex_unlock(&lock_a));

	key = k_spin_lock(&spin_a);
	zassert_ok(k_mutex_lock(&lock_a, K_FOREVER));
	zassert_true(error_count() > 0U);
	zassert_ok(k_mutex_unlock(&lock_a));
	k_spin_unlock(&spin_a, key);
}

ZTEST(lockdep, test_reset_clears_graph_and_statistics)
{
	struct z_lockdep_stats stats;

	reset_mutexes();
	lock_mutex_pair(&lock_a, &lock_b);
	get_stats(&stats);
	zassert_true(stats.lock_instances > 0U);
	zassert_true(stats.dependencies > 0U);

	z_lockdep_reset();
	get_stats(&stats);
	zassert_equal(stats.lock_instances, 0U);
	zassert_equal(stats.dependencies, 0U);
	zassert_equal(stats.errors, 0U);
	zassert_equal(stats.overflows, 0U);
	zassert_false(z_lockdep_is_held(&lock_a));
}

/*
 * These tests assert exact graph counters after z_lockdep_reset(). Another
 * CPU running scheduler or timer code would register its own locks into the
 * shared graph, so pin the suite to one CPU. Multi-CPU behaviour is covered
 * by tests/kernel/lockdep/smp.
 */
ZTEST_SUITE(lockdep, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
