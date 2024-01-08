/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/p4wq.h>

#define MAX_NUM_THREADS (CONFIG_MP_MAX_NUM_CPUS * 2)
#define NUM_THREADS (arch_num_cpus() * 2)
#define MAX_ITEMS (MAX_NUM_THREADS * 8)
#define MAX_EVENTS 1024

K_P4WQ_DEFINE(wq, MAX_NUM_THREADS, 2048);

static struct k_p4wq_work simple_item;
static volatile int has_run;
static volatile int run_count;
static volatile int spin_release;

struct test_item {
	struct k_p4wq_work item;
	bool active;
	bool running;
};

static struct k_spinlock lock;
static struct test_item items[MAX_ITEMS];
static int active_items;
static int event_count;
static bool stress_complete;

static void stress_handler(struct k_p4wq_work *item);

static void stress_sub(struct test_item *item)
{
	/* Choose a random preemptible priority higher than the idle
	 * priority, and a random deadline sometime within the next
	 * 2ms
	 */
	item->item.priority = sys_rand32_get() % (K_LOWEST_THREAD_PRIO - 1);
	item->item.deadline = sys_rand32_get() % k_ms_to_cyc_ceil32(2);
	item->item.handler = stress_handler;
	item->running = false;
	item->active = true;
	active_items++;
	k_p4wq_submit(&wq, &item->item);
}

static void stress_handler(struct k_p4wq_work *item)
{
	k_spinlock_key_t k = k_spin_lock(&lock);
	struct test_item *titem = CONTAINER_OF(item, struct test_item, item);

	titem->running = true;

	int curr_pri = k_thread_priority_get(k_current_get());

	zassert_true(curr_pri == item->priority,
		     "item ran with wrong priority: want %d have %d",
		     item->priority, curr_pri);

	if (stress_complete) {
		k_spin_unlock(&lock, k);
		return;
	}

	active_items--;

	/* Pick 0-3 random item slots and submit them if they aren't
	 * already.  Make sure we always have at least one active.
	 */
	int num_tries = sys_rand32_get() % 4;

	for (int i = 0; (active_items == 0) || (i < num_tries); i++) {
		int ii = sys_rand32_get() % MAX_ITEMS;

		if (items[ii].item.thread == NULL &&
		    &items[ii] != titem && !items[ii].active) {
			stress_sub(&items[ii]);
		}
	}

	if (event_count++ >= MAX_EVENTS) {
		stress_complete = true;
	}

	titem->active = false;
	k_spin_unlock(&lock, k);
}

/* Simple stress test designed to flood the queue and retires as many
 * items of random priority as possible.  Note that because of the
 * random priorities, this tends to produce a lot of "out of worker
 * threads" warnings from the queue as we randomly try to submit more
 * schedulable (i.e. high priority) items than there are threads to
 * run them.
 */
ZTEST(lib_p4wq, test_stress)
{
	k_thread_priority_set(k_current_get(), -1);
	memset(items, 0, sizeof(items));

	stress_complete = false;
	active_items = 1;
	items[0].item.priority = -1;
	stress_handler(&items[0].item);

	while (!stress_complete) {
		k_msleep(100);
	}
	k_msleep(10);

	zassert_true(event_count > 1, "stress tests didn't run");
}

static int active_count(void)
{
	/* Whitebox: count the number of BLOCKED threads, because the
	 * queue will unpend them synchronously in submit but the
	 * "active" list is maintained from the thread itself against
	 * which we can't synchronize easily.
	 */
	int count = 0;
	sys_dnode_t *dummy;

	SYS_DLIST_FOR_EACH_NODE(&wq.waitq.waitq, dummy) {
		count++;
	}

	count = MAX_NUM_THREADS - count;
	return count;
}

static void spin_handler(struct k_p4wq_work *item)
{
	while (!spin_release) {
		k_busy_wait(10);
	}
}

/* Selects and adds a new item to the queue, returns an indication of
 * whether the item changed the number of active threads.  Does not
 * return the item itself, not needed.
 */
static bool add_new_item(int pri)
{
	static int num_items;
	int n0 = active_count();
	struct k_p4wq_work *item = &items[num_items++].item;

	__ASSERT_NO_MSG(num_items < MAX_ITEMS);
	item->priority = pri;
	item->deadline = k_us_to_cyc_ceil32(100);
	item->handler = spin_handler;
	k_p4wq_submit(&wq, item);
	k_usleep(1);

	return (active_count() != n0);
}

/* Whitebox test of thread state: make sure that as we add threads
 * they get scheduled as needed, up to NUM_CPUS (at which point the
 * queue should STOP scheduling new threads).  Then add more at higher
 * priorities and verify that they get scheduled too (to allow
 * preemption), up to the maximum number of threads that we created.
 */
ZTEST(lib_p4wq, test_fill_queue)
{
	int p0 = 4;

	/* The work item priorities are 0-4, this thread should be -1
	 * so it's guaranteed not to be preempted
	 */
	k_thread_priority_set(k_current_get(), -1);

	/* Spawn enough threads so the queue saturates the CPU count
	 * (note they have lower priority than the current thread so
	 * we can be sure to run).  They should all be made active
	 * when added.
	 */
	unsigned int num_cpus = arch_num_cpus();
	unsigned int num_threads = NUM_THREADS;

	for (int i = 0; i < num_cpus; i++) {
		zassert_true(add_new_item(p0), "thread should be active");
	}

	/* Add one more, it should NOT be scheduled */
	zassert_false(add_new_item(p0), "thread should not be active");

	/* Now add more at higher priorities, they should get
	 * scheduled (so that they can preempt the running ones) until
	 * we run out of threads.
	 */
	for (int pri = p0 - 1; pri >= p0 - 4; pri++) {
		for (int i = 0; i < num_cpus; i++) {
			bool active = add_new_item(pri);

			if (!active) {
				zassert_equal(active_count(), num_threads,
					      "thread max not reached");
				goto done;
			}
		}
	}

 done:
	/* Clean up and wait for the threads to be idle */
	spin_release = 1;
	do {
		k_msleep(1);
	} while (active_count() != 0);
	k_msleep(1);
}

static void resubmit_handler(struct k_p4wq_work *item)
{
	if (run_count++ == 0) {
		k_p4wq_submit(&wq, item);
	} else {
		/* While we're here: validate that it doesn't show
		 * itself as "live" while executing
		 */
		zassert_false(k_p4wq_cancel(&wq, item),
			      "item should not be cancelable while running");
	}
}

/* Validate item can be resubmitted from its own handler */
ZTEST(lib_p4wq, test_resubmit)
{
	run_count = 0;
	simple_item = (struct k_p4wq_work){};
	simple_item.handler = resubmit_handler;
	k_p4wq_submit(&wq, &simple_item);

	k_msleep(100);
	zassert_equal(run_count, 2, "Wrong run count: %d\n", run_count);
}

void simple_handler(struct k_p4wq_work *work)
{
	zassert_equal(work, &simple_item, "bad work item pointer");
	zassert_false(has_run, "ran twice");
	has_run = true;
}

/* Simple test that submitted items run, and at the correct priority */
ZTEST(lib_p4wq_1cpu, test_p4wq_simple)
{
	int prio = 2;

	k_thread_priority_set(k_current_get(), prio);

	/* Lower priority item, should not run until we yield */
	simple_item.priority = prio + 1;
	simple_item.deadline = 0;
	simple_item.handler = simple_handler;

	has_run = false;
	k_p4wq_submit(&wq, &simple_item);
	zassert_false(has_run, "ran too early");

	k_msleep(10);
	zassert_true(has_run, "low-priority item didn't run");

	/* Higher priority, should preempt us */
	has_run = false;
	simple_item.priority = prio - 1;
	k_p4wq_submit(&wq, &simple_item);
	zassert_true(has_run, "high-priority item didn't run");
}

ZTEST_SUITE(lib_p4wq, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(lib_p4wq_1cpu, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
