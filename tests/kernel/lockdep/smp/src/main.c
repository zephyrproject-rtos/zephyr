/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/lockdep.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

#define CPU_COUNT  2
#define STACK_SIZE (1536 + CONFIG_TEST_EXTRA_STACK_SIZE)

static struct k_thread workers[CPU_COUNT];
static K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, CPU_COUNT, STACK_SIZE);

static struct z_lockdep_stats get_stats(void)
{
	struct z_lockdep_stats stats;

	z_lockdep_stats_get(&stats);
	return stats;
}

struct concurrent_args {
	int cpu;
	struct k_spinlock *own_outer;
	struct k_spinlock *own_inner;
	struct k_spinlock *other_outer;
	bool held_own;
	bool held_other;
	int actual_cpu;
};

static struct k_spinlock concurrent_locks[CPU_COUNT][2];
static struct concurrent_args concurrent_args[CPU_COUNT];
static atomic_t concurrent_ready;

static void concurrent_lock_worker(void *arg1, void *unused2, void *unused3)
{
	struct concurrent_args *args = arg1;
	k_spinlock_key_t outer_key;
	k_spinlock_key_t inner_key;

	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	args->actual_cpu = arch_curr_cpu()->id;
	outer_key = k_spin_lock(args->own_outer);
	inner_key = k_spin_lock(args->own_inner);
	atomic_or(&concurrent_ready, BIT(args->cpu));
	while ((atomic_get(&concurrent_ready) & BIT_MASK(CPU_COUNT)) != BIT_MASK(CPU_COUNT)) {
		arch_spin_relax();
	}

	args->held_own = z_lockdep_is_held(args->own_outer) && z_lockdep_is_held(args->own_inner);
	args->held_other = z_lockdep_is_held(args->other_outer);
	k_spin_unlock(args->own_inner, inner_key);
	k_spin_unlock(args->own_outer, outer_key);
}

ZTEST(lockdep_smp, test_concurrent_graph_updates_and_per_cpu_held_state)
{
	uint32_t errors_before = get_stats().errors;
	struct z_lockdep_stats stats;

	atomic_set(&concurrent_ready, 0);
	for (int cpu = 0; cpu < CPU_COUNT; cpu++) {
		concurrent_locks[cpu][0] = (struct k_spinlock){};
		concurrent_locks[cpu][1] = (struct k_spinlock){};
		concurrent_args[cpu] = (struct concurrent_args){
			.cpu = cpu,
			.own_outer = &concurrent_locks[cpu][0],
			.own_inner = &concurrent_locks[cpu][1],
			.other_outer = &concurrent_locks[1 - cpu][0],
			.actual_cpu = -1,
		};
		k_thread_create(&workers[cpu], worker_stacks[cpu],
				K_THREAD_STACK_SIZEOF(worker_stacks[cpu]), concurrent_lock_worker,
				&concurrent_args[cpu], NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
		zassert_ok(k_thread_cpu_pin(&workers[cpu], cpu));
	}

	/* Start CPU 1 first so it can run while the CPU 0 worker is scheduled. */
	k_thread_start(&workers[1]);
	k_thread_start(&workers[0]);
	for (int cpu = 0; cpu < CPU_COUNT; cpu++) {
		zassert_ok(k_thread_join(&workers[cpu], K_FOREVER));
		zassert_equal(concurrent_args[cpu].actual_cpu, cpu);
		zassert_true(concurrent_args[cpu].held_own);
		zassert_false(concurrent_args[cpu].held_other);
	}

	stats = get_stats();
	zassert_equal(stats.errors, errors_before);
	zassert_true(stats.dependencies >= CPU_COUNT);
}

static struct k_spinlock shared_spin;
static struct k_spinlock outer_spin;
static atomic_t holder_ready;
static atomic_t holder_release;
static int holder_cpu;
static int contender_cpu;
static int trylock_result;

static void spin_holder_worker(void *unused1, void *unused2, void *unused3)
{
	k_spinlock_key_t key;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	holder_cpu = arch_curr_cpu()->id;
	key = k_spin_lock(&shared_spin);
	atomic_set(&holder_ready, 1);
	while (atomic_get(&holder_release) == 0) {
		arch_spin_relax();
	}
	k_spin_unlock(&shared_spin, key);
}

static void spin_contender_worker(void *unused1, void *unused2, void *unused3)
{
	k_spinlock_key_t outer_key;
	k_spinlock_key_t shared_key;
	k_spinlock_key_t unused_key;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	contender_cpu = arch_curr_cpu()->id;
	while (atomic_get(&holder_ready) == 0) {
		k_busy_wait(1);
	}

	outer_key = k_spin_lock(&outer_spin);
	trylock_result = k_spin_trylock(&shared_spin, &unused_key);
	k_spin_unlock(&outer_spin, outer_key);
	atomic_set(&holder_release, 1);

	/* Learn shared -> outer after the holder has released shared. */
	shared_key = k_spin_lock(&shared_spin);
	outer_key = k_spin_lock(&outer_spin);
	k_spin_unlock(&outer_spin, outer_key);
	k_spin_unlock(&shared_spin, shared_key);
}

ZTEST(lockdep_smp, test_failed_cross_cpu_spin_trylock_adds_no_dependency)
{
	uint32_t errors_before = get_stats().errors;

	shared_spin = (struct k_spinlock){};
	outer_spin = (struct k_spinlock){};
	atomic_set(&holder_ready, 0);
	atomic_set(&holder_release, 0);
	holder_cpu = -1;
	contender_cpu = -1;
	trylock_result = -EINVAL;

	k_thread_create(&workers[1], worker_stacks[1], K_THREAD_STACK_SIZEOF(worker_stacks[1]),
			spin_holder_worker, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&workers[1], 1));
	k_thread_create(&workers[0], worker_stacks[0], K_THREAD_STACK_SIZEOF(worker_stacks[0]),
			spin_contender_worker, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&workers[0], 0));

	k_thread_start(&workers[1]);
	k_thread_start(&workers[0]);
	zassert_ok(k_thread_join(&workers[0], K_FOREVER));
	zassert_ok(k_thread_join(&workers[1], K_FOREVER));

	zassert_equal(contender_cpu, 0);
	zassert_equal(holder_cpu, 1);
	zassert_equal(trylock_result, -EBUSY);
	zassert_equal(get_stats().errors, errors_before);
}

struct spin_order_args {
	struct k_spinlock *first;
	struct k_spinlock *second;
	int actual_cpu;
};

static struct k_spinlock graph_a;
static struct k_spinlock graph_b;
static struct spin_order_args graph_args;

static void spin_order_worker(void *arg1, void *unused2, void *unused3)
{
	struct spin_order_args *args = arg1;
	k_spinlock_key_t first;
	k_spinlock_key_t second;

	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	args->actual_cpu = arch_curr_cpu()->id;
	first = k_spin_lock(args->first);
	second = k_spin_lock(args->second);
	k_spin_unlock(args->second, second);
	k_spin_unlock(args->first, first);
}

static void run_spin_order_on_cpu(int cpu, struct k_spinlock *first, struct k_spinlock *second)
{
	graph_args = (struct spin_order_args){
		.first = first,
		.second = second,
		.actual_cpu = -1,
	};
	k_thread_create(&workers[cpu], worker_stacks[cpu],
			K_THREAD_STACK_SIZEOF(worker_stacks[cpu]), spin_order_worker, &graph_args,
			NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&workers[cpu], cpu));
	k_thread_start(&workers[cpu]);
	zassert_ok(k_thread_join(&workers[cpu], K_FOREVER));
	zassert_equal(graph_args.actual_cpu, cpu);
}

ZTEST(lockdep_smp, test_dependency_graph_is_shared_across_cpus)
{
	uint32_t errors_before = get_stats().errors;

	graph_a = (struct k_spinlock){};
	graph_b = (struct k_spinlock){};

	run_spin_order_on_cpu(0, &graph_a, &graph_b);
	zassert_equal(get_stats().errors, errors_before);
	run_spin_order_on_cpu(1, &graph_b, &graph_a);
	zassert_true(get_stats().errors > errors_before);
}

static struct k_mutex cross_cpu_mutex;
static struct k_spinlock cross_cpu_spin;
static struct k_sem cross_cpu_holder_ready;
static struct k_sem cross_cpu_release;
static struct k_sem cross_cpu_waiter_ready;
static struct k_sem cross_cpu_isr_done;
static int cross_cpu_holder_id;
static int cross_cpu_waiter_id;
static int cross_cpu_waiter_result;
static bool cross_cpu_waiter_saw_held;
static bool cross_cpu_isr_saw_mutex;
static int cross_cpu_isr_id;

static void cross_cpu_mutex_holder(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	cross_cpu_holder_id = arch_curr_cpu()->id;
	zassert_ok(k_mutex_lock(&cross_cpu_mutex, K_FOREVER));
	k_sem_give(&cross_cpu_holder_ready);
	zassert_ok(k_sem_take(&cross_cpu_release, K_FOREVER));
	zassert_ok(k_mutex_unlock(&cross_cpu_mutex));
}

static void start_cross_cpu_mutex_holder(void)
{
	zassert_ok(k_mutex_init(&cross_cpu_mutex));
	k_sem_init(&cross_cpu_holder_ready, 0, 1);
	k_sem_init(&cross_cpu_release, 0, 1);
	cross_cpu_holder_id = -1;
	k_thread_create(&workers[0], worker_stacks[0], K_THREAD_STACK_SIZEOF(worker_stacks[0]),
			cross_cpu_mutex_holder, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&workers[0], 0));
	k_thread_start(&workers[0]);
	zassert_ok(k_sem_take(&cross_cpu_holder_ready, K_FOREVER));
	zassert_equal(cross_cpu_holder_id, 0);
}

static void stop_cross_cpu_mutex_holder(void)
{
	k_sem_give(&cross_cpu_release);
	zassert_ok(k_thread_join(&workers[0], K_FOREVER));
}

static void cross_cpu_mutex_waiter(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	cross_cpu_waiter_id = arch_curr_cpu()->id;
	k_sem_give(&cross_cpu_waiter_ready);
	cross_cpu_waiter_result = k_mutex_lock(&cross_cpu_mutex, K_FOREVER);
	if (cross_cpu_waiter_result == 0) {
		cross_cpu_waiter_saw_held = z_lockdep_is_held(&cross_cpu_mutex);
		cross_cpu_waiter_result = k_mutex_unlock(&cross_cpu_mutex);
	}
}

ZTEST(lockdep_smp, test_cross_cpu_mutex_handoff_has_no_false_release_error)
{
	uint32_t errors_before;

	start_cross_cpu_mutex_holder();
	errors_before = get_stats().errors;
	k_sem_init(&cross_cpu_waiter_ready, 0, 1);
	cross_cpu_waiter_id = -1;
	cross_cpu_waiter_result = -EINVAL;
	cross_cpu_waiter_saw_held = false;
	k_thread_create(&workers[1], worker_stacks[1], K_THREAD_STACK_SIZEOF(worker_stacks[1]),
			cross_cpu_mutex_waiter, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&workers[1], 1));
	k_thread_start(&workers[1]);
	zassert_ok(k_sem_take(&cross_cpu_waiter_ready, K_FOREVER));
	zassert_true(WAIT_FOR((workers[1].base.thread_state & _THREAD_PENDING) != 0U, 1000000,
			      k_msleep(1)));
	stop_cross_cpu_mutex_holder();
	zassert_ok(k_thread_join(&workers[1], K_FOREVER));
	zassert_equal(cross_cpu_waiter_id, 1);
	zassert_ok(cross_cpu_waiter_result);
	zassert_true(cross_cpu_waiter_saw_held);
	zassert_equal(get_stats().errors, errors_before);
}

static void cross_cpu_isr_probe(const void *unused)
{
	k_spinlock_key_t key;

	ARG_UNUSED(unused);
	cross_cpu_isr_id = arch_curr_cpu()->id;
	cross_cpu_isr_saw_mutex = z_lockdep_is_held(&cross_cpu_mutex);
	key = k_spin_lock(&cross_cpu_spin);
	k_spin_unlock(&cross_cpu_spin, key);
}

static void cross_cpu_isr_worker(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	irq_offload(cross_cpu_isr_probe, NULL);
	k_sem_give(&cross_cpu_isr_done);
}

ZTEST(lockdep_smp, test_cross_cpu_isr_does_not_inherit_mutex)
{
	uint32_t errors_before;
	k_spinlock_key_t key;

	start_cross_cpu_mutex_holder();
	errors_before = get_stats().errors;
	cross_cpu_spin = (struct k_spinlock){};
	cross_cpu_isr_saw_mutex = true;
	cross_cpu_isr_id = -1;
	k_sem_init(&cross_cpu_isr_done, 0, 1);
	k_thread_create(&workers[1], worker_stacks[1], K_THREAD_STACK_SIZEOF(worker_stacks[1]),
			cross_cpu_isr_worker, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&workers[1], 1));
	k_thread_start(&workers[1]);
	zassert_ok(k_sem_take(&cross_cpu_isr_done, K_FOREVER));
	zassert_ok(k_thread_join(&workers[1], K_FOREVER));
	zassert_equal(cross_cpu_isr_id, 1);
	zassert_false(cross_cpu_isr_saw_mutex);
	stop_cross_cpu_mutex_holder();

	/* A spurious CPU 0 mutex -> CPU 1 spinlock edge would close this cycle. */
	key = k_spin_lock(&cross_cpu_spin);
	zassert_ok(k_mutex_lock(&cross_cpu_mutex, K_FOREVER));
	zassert_ok(k_mutex_unlock(&cross_cpu_mutex));
	k_spin_unlock(&cross_cpu_spin, key);
	zassert_equal(get_stats().errors, errors_before);
}

ZTEST_SUITE(lockdep_smp, NULL, NULL, NULL, NULL, NULL);
