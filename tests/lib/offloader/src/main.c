/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/offloader/engine.h>

#define PRIO_HIGH K_PRIO_COOP(2)
#define PRIO_MID  K_PRIO_PREEMPT(0)
#define PRIO_LOW  K_PRIO_PREEMPT(10)

#define NUM_WORKERS 3
#define CALLER_STACK_SZ 512
#define WORKER_STACK_SZ 3096

OFFLOADER_ENGINE_DEFINE(eng, NUM_WORKERS, WORKER_STACK_SZ);

static K_THREAD_STACK_ARRAY_DEFINE(caller_stacks, NUM_WORKERS, CALLER_STACK_SZ);
static struct k_thread caller_threads[NUM_WORKERS];

static struct k_sem started;
static struct k_sem release;
static struct k_sem observer_started;
static struct k_sem observer_release;

static bool record_ran;
static int record_prio;
static k_tid_t record_tid;
static atomic_t record_count;

struct block_args {
	struct k_sem *started;
	struct k_sem *release;
};

struct record_args {
	bool *ran;
	int *prio;
	k_tid_t *tid;
	atomic_t *count;
};

static void record_fn(void *args)
{
	struct record_args *a = args;

	*a->tid = k_current_get();
	*a->prio = k_thread_priority_get(*a->tid);
	*a->ran = true;
	atomic_inc(a->count);
}

static void block_fn(void *args)
{
	struct block_args *a = args;

	k_sem_give(a->started);
	k_sem_take(a->release, K_FOREVER);
}

static void occupant_entry(void *p1, void *p2, void *p3)
{
	struct block_args a = { &started, &release };

	ARGS_UNUSED(p1, p2, p3);
	offloader_dispatch(&eng, block_fn, &a);
}

static void record_entry(void *p1, void *p2, void *p3)
{
	struct record_args a = { &record_ran, &record_prio, &record_tid, &record_count };

	ARGS_UNUSED(p1, p2, p3);
	k_sem_give(&observer_started);
	offloader_dispatch(&eng, record_fn, &a);
	k_sem_take(&observer_release, K_FOREVER);
}

static size_t workers_at_prio(int prio)
{
	size_t count = 0;

	for (size_t i = 0; i < eng.num_workers; i++) {
		if (k_thread_priority_get(&eng.workers[i]) == prio) {
			count++;
		}
	}

	return count;
}

static int prio(int *high, int *mid, int *low)
{
	int prio;

	zassert_true(*high || *mid || *low, "no priorities left to assign");
	if (*high) {
		prio = PRIO_HIGH;
		*high -= 1;
	} else if (*mid) {
		prio = PRIO_MID;
		*mid -= 1;
	} else {
		prio = PRIO_LOW;
		*low -= 1;
	}

	return prio;
}

static void occupy_workers(int high, int mid, int low)
{
	for (size_t i = 0; i < NUM_WORKERS; i++) {
		int p = prio(&high, &mid, &low);

		k_thread_create(&caller_threads[i], caller_stacks[i], CALLER_STACK_SZ,
				occupant_entry, NULL, NULL, NULL, p, 0, K_NO_WAIT);
	}

	for (size_t i = 0; i < NUM_WORKERS; i++) {
		k_sem_take(&started, K_FOREVER);
	}
}

static void reclaim_workers(void)
{
	for (size_t i = 0; i < NUM_WORKERS; i++) {
		k_sem_give(&release);
	}

	for (size_t i = 0; i < NUM_WORKERS; i++) {
		k_thread_join(&caller_threads[i], K_FOREVER);
	}
}

static K_THREAD_STACK_ARRAY_DEFINE(recorder_stacks, NUM_WORKERS, CALLER_STACK_SZ);
static struct k_thread record_threads[NUM_WORKERS];

static void start_observers(size_t n, int prio)
{
	for (size_t i = 0; i < n; i++) {
		k_thread_create(&record_threads[i], recorder_stacks[i], CALLER_STACK_SZ,
				record_entry, NULL, NULL, NULL,
				prio, 0, K_NO_WAIT);
	}

	for (size_t i = 0; i < n; i++) {
		k_sem_take(&observer_started, K_FOREVER);
	}
}

static void reclaim_observers(size_t n)
{
	for (size_t i = 0; i < n; i++) {
		k_sem_give(&observer_release);
	}

	for (size_t i = 0; i < n; i++) {
		k_thread_join(&record_threads[i], K_FOREVER);
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_init(&started, 0, NUM_WORKERS);
	k_sem_init(&release, 0, NUM_WORKERS);
	k_sem_init(&observer_started, 0, NUM_WORKERS);
	k_sem_init(&observer_release, 0, NUM_WORKERS);
	record_ran = false;
	record_prio = -200;
	atomic_clear(&record_count);
	k_thread_priority_set(k_current_get(), PRIO_LOW + 1);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&started);
	k_sem_reset(&release);
	k_sem_reset(&observer_started);
	k_sem_reset(&observer_release);

	for (size_t i = 0; i < NUM_WORKERS; i++) {
		k_thread_abort(&caller_threads[i]);
		k_thread_abort(&record_threads[i]);
	}
}

ZTEST(offloader_engine, test_dispatch_simple)
{
	start_observers(1, PRIO_MID);
	reclaim_observers(1);
	zassert_true(record_ran, "work did not run");
	zassert_equal(record_prio, PRIO_MID, "work ran at %d, expected %d", record_prio, PRIO_MID);
	zassert_true(record_tid != &record_threads[0], "work ran in the caller thread");
	zassert_true(record_tid == &eng.workers[0] || record_tid == &eng.workers[1] ||
		     record_tid == &eng.workers[2], "work ran in a non-worker thread");
}

ZTEST(offloader_engine, test_dispatch_fully_active)
{
	occupy_workers(0, 1, 2);

	start_observers(1, PRIO_MID);

	zassert_false(record_ran, "work ran while all workers were busy");
	zassert_equal(workers_at_prio(PRIO_MID), 1,
		      "a worker already at the request priority must not trigger a boost");
	zassert_equal(workers_at_prio(PRIO_LOW), 2,
		      "lower priority workers must keep their priority when no inversion exists");
	reclaim_workers();
	reclaim_observers(1);

	zassert_true(record_ran, "queued work did not run after a worker freed");
	zassert_equal(record_prio, PRIO_MID, "Observer dispatch ran at wrong priority");
}

ZTEST(offloader_engine, test_dispatch_priority_inversion)
{
	occupy_workers(0, 1, 2);

	zassert_equal(workers_at_prio(PRIO_MID), 1, "not all workers ran at the caller priority");
	zassert_equal(workers_at_prio(PRIO_LOW), 2, "not all workers ran at the caller priority");

	start_observers(1, PRIO_HIGH);

	zassert_equal(workers_at_prio(PRIO_HIGH), 1, "exactly one worker should be boosted");
	zassert_equal(workers_at_prio(PRIO_MID), 1,
		      "non-boosted workers should keep the caller priority");
	zassert_equal(workers_at_prio(PRIO_LOW), 1,
		      "Only 1 lower priority worker should be boosted");

	zassert_false(record_ran, "boosted work ran before a worker freed");
	reclaim_workers();
	reclaim_observers(1);

	zassert_true(record_ran, "boosted work did not run");
	zassert_equal(record_prio, PRIO_HIGH,
		      "Observer dispatch ran at wrong priority %d, expected %d", record_prio,
		      PRIO_HIGH);
}

ZTEST(offloader_engine, test_dispatch_full_inheritance)
{
	occupy_workers(0, 1, 2);

	start_observers(NUM_WORKERS, PRIO_HIGH);

	zassert_false(record_ran, "boosted work ran before a worker freed");
	zassert_equal(workers_at_prio(PRIO_HIGH), NUM_WORKERS,
		      "every worker must inherit the priority of a waiting request");
	zassert_equal(workers_at_prio(PRIO_MID), 0, "no worker should keep a lower priority");
	zassert_equal(workers_at_prio(PRIO_LOW), 0, "no worker should keep a lower priority");

	reclaim_workers();
	reclaim_observers(NUM_WORKERS);

	zassert_equal(atomic_get(&record_count), NUM_WORKERS, "not every queued request ran");
}

ZTEST_SUITE(offloader_engine, NULL, NULL, before, after, NULL);
