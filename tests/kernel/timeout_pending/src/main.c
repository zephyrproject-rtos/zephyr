/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

/*
 * Many-pending coverage for any timeout-queue backend. tests/kernel/timeout
 * only exercises K_TIMEOUT_SUM, so a backend can pass that suite without
 * ever linking more than a handful of nodes.
 *
 * Insert order is a coprime stride over the id space, so expiry order is
 * scrambled. Ids are grouped into same-tick clusters. After the earliest
 * cluster has fired, a scattered subset of still-pending timers is aborted.
 *
 * Backends that guarantee same-tick FIFO (delta list, bucket, skip list)
 * must fire remaining timers in (deadline, insert-seq) order. Min-heap and
 * wheel do not promise that, so those variants only check expiry order.
 */
#define NUM_TIMEOUTS 256
#define CLUSTER 8
#define FIRST_DELAY_MIN_TICKS 32
#define FIRST_DELAY_MS 50
#define NUM_CLUSTERS (NUM_TIMEOUTS / CLUSTER)
/* had to keep it as a coprime with NUM_TIMEOUTS so start order is a permutation of ids. */
#define SCRAMBLE_STRIDE 7

BUILD_ASSERT((NUM_TIMEOUTS % CLUSTER) == 0, "N must be a multiple of CLUSTER");
BUILD_ASSERT((NUM_TIMEOUTS % SCRAMBLE_STRIDE) != 0,
	     "SCRAMBLE_STRIDE must be coprime with NUM_TIMEOUTS");
#ifdef CONFIG_TIMEOUT_BACKEND_MINHEAP
BUILD_ASSERT(NUM_TIMEOUTS <= CONFIG_TIMEOUT_HEAP_MAX_ENTRIES,
	     "raise CONFIG_TIMEOUT_HEAP_MAX_ENTRIES for this many pending timeouts");
#endif

static struct k_timer timers[NUM_TIMEOUTS];
static int delay_ticks[NUM_TIMEOUTS];
static int insert_seq[NUM_TIMEOUTS];
static uint8_t saw_fire[NUM_TIMEOUTS];
static bool aborted[NUM_TIMEOUTS];
static int fire_order[NUM_TIMEOUTS];
static int expected[NUM_TIMEOUTS];
static atomic_t fire_count;

static int first_delay_ticks(void)
{
	int t = (int)k_ms_to_ticks_ceil32(FIRST_DELAY_MS);

	/* Keep a tick floor so a slow clock still outlasts the start loop. */
	return (t > FIRST_DELAY_MIN_TICKS) ? t : FIRST_DELAY_MIN_TICKS;
}

static int scrambled_id(int seq)
{
	return (int)(((unsigned int)seq * SCRAMBLE_STRIDE) % NUM_TIMEOUTS);
}

static int cluster_of(int id)
{
	return id / CLUSTER;
}

static bool same_tick_fifo(void)
{
	return !IS_ENABLED(CONFIG_TIMEOUT_BACKEND_MINHEAP) &&
	       !IS_ENABLED(CONFIG_TIMEOUT_BACKEND_WHEEL);
}

static k_timeout_t cluster_timeout(int64_t abs_base, int cluster)
{
#ifdef CONFIG_TIMEOUT_64BIT
	return K_TIMEOUT_ABS_TICKS(abs_base + cluster);
#else
	ARG_UNUSED(abs_base);
	return K_TICKS(first_delay_ticks() + cluster);
#endif
}

static void expiry_fn(struct k_timer *timer)
{
	int id = POINTER_TO_INT(k_timer_user_data_get(timer));
	int idx = (int)atomic_inc(&fire_count);

	saw_fire[id] = 1U;
	if (idx < NUM_TIMEOUTS) {
		fire_order[idx] = id;
	}
}

static void sync_tick(void)
{
	uint32_t uptime = k_uptime_get_32();

	while (uptime == k_uptime_get_32()) {
		Z_SPIN_DELAY(50);
	}
}

static void wait_fired_at_least(int expected)
{
	int64_t extra_ms = k_ticks_to_ms_ceil64(first_delay_ticks() + NUM_CLUSTERS + 20);
	int64_t deadline = k_uptime_get() + extra_ms + 5000;

	while (atomic_get(&fire_count) < expected && k_uptime_get() < deadline) {
		k_sleep(K_TICKS(1));
	}

	zassert_true(atomic_get(&fire_count) >= expected,
		     "only %ld of %d timeouts fired",
		     (long)atomic_get(&fire_count), expected);
}

static void sort_by_expiry_then_insert(int *ids, int n)
{
	int i;

	for (i = 1; i < n; i++) {
		int key = ids[i];
		int j = i - 1;

		while (j >= 0) {
			int a = ids[j];
			bool after = (delay_ticks[a] < delay_ticks[key]) ||
				     (delay_ticks[a] == delay_ticks[key] &&
				      insert_seq[a] <= insert_seq[key]);

			if (after) {
				break;
			}
			ids[j + 1] = a;
			j--;
		}
		ids[j + 1] = key;
	}
}

/**
 * @brief Many pending timeouts: scrambled keys, same-tick clusters, mid-flight abort.
 *
 * @ingroup kernel_timeout_tests
 *
 * @details
 * Starts NUM_TIMEOUTS one-shots in scrambled order, grouped into same-tick
 * clusters. After the earliest cluster fires, aborts a scattered subset of
 * still-pending timers so removal is not only at the head. Remaining timers
 * must expire in deadline order; backends that keep same-tick FIFO must also
 * preserve insertion order inside a cluster.
 *
 * @see k_timer_start()
 * @see k_timer_stop()
 */
ZTEST(timeout_pending, test_many_pending_scrambled)
{
	int i;
	int nexp = 0;
	int64_t abs_base = 0;
	int first_delay;

	if (!IS_ENABLED(CONFIG_TIMEOUT_64BIT)) {
		/*
		 * Relative starts re-anchor per call, so the loop cannot land a
		 * cluster on one tick and delay_ticks[] stops modelling the real
		 * deadlines. Neither assertion below is checkable.
		 */
		ztest_test_skip();
	}

	first_delay = first_delay_ticks();

	for (i = 0; i < NUM_TIMEOUTS; i++) {
		int id = scrambled_id(i);

		delay_ticks[id] = first_delay + cluster_of(id);
		insert_seq[id] = i;
		saw_fire[id] = 0U;
		aborted[id] = false;
		fire_order[i] = -1;
		k_timer_init(&timers[id], expiry_fn, NULL);
		k_timer_user_data_set(&timers[id], INT_TO_POINTER(id));
	}
	atomic_set(&fire_count, 0);

	sync_tick();
	abs_base = k_uptime_ticks() + first_delay;
	k_sched_lock();
	for (i = 0; i < NUM_TIMEOUTS; i++) {
		int id = scrambled_id(i);

		k_timer_start(&timers[id],
			      cluster_timeout(abs_base, cluster_of(id)),
			      K_NO_WAIT);
	}
	k_sched_unlock();
	zassert_equal(atomic_get(&fire_count), 0,
		      "first cluster fired during start loop; raise FIRST_DELAY_MS");

	/* First cluster must fire before aborts so remaining unlinks are interior. */
	wait_fired_at_least(CLUSTER);

	for (i = 0; i < NUM_TIMEOUTS; i++) {
		int id = scrambled_id(i);

		if ((insert_seq[id] % 5) != 1) {
			continue;
		}
		if (cluster_of(id) < 3) {
			continue;
		}
		if (saw_fire[id] != 0U) {
			continue;
		}
		if (k_timer_remaining_ticks(&timers[id]) == 0) {
			continue;
		}
		k_timer_stop(&timers[id]);
		if (saw_fire[id] == 0U) {
			aborted[id] = true;
		}
	}

	for (i = 0; i < NUM_TIMEOUTS; i++) {
		if (!aborted[i]) {
			expected[nexp++] = i;
		}
	}

	wait_fired_at_least(nexp);
	zassert_equal(atomic_get(&fire_count), nexp,
		      "fire count %ld != expected %d",
		      (long)atomic_get(&fire_count), nexp);

	for (i = 0; i < NUM_TIMEOUTS; i++) {
		if (aborted[i]) {
			zassert_equal(saw_fire[i], 0U, "aborted timer %d still fired", i);
		} else {
			zassert_equal(saw_fire[i], 1U, "timer %d neither aborted nor fired", i);
		}
	}

	if (same_tick_fifo()) {
		sort_by_expiry_then_insert(expected, nexp);
		for (i = 0; i < nexp; i++) {
			zassert_equal(fire_order[i], expected[i],
				      "expiry[%d] was timer %d, expected %d (delay %d seq %d)",
				      i, fire_order[i], expected[i],
				      delay_ticks[expected[i]], insert_seq[expected[i]]);
		}
	} else {
		for (i = 1; i < nexp; i++) {
			zassert_true(delay_ticks[fire_order[i]] >= delay_ticks[fire_order[i - 1]],
				     "expiry order broken at %d: delay %d then %d",
				     i, delay_ticks[fire_order[i - 1]],
				     delay_ticks[fire_order[i]]);
		}
	}

	for (i = 0; i < NUM_TIMEOUTS; i++) {
		k_timer_stop(&timers[i]);
	}
}

ZTEST_SUITE(timeout_pending, NULL, NULL, NULL, NULL, NULL);
