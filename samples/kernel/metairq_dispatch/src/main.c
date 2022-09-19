/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include "msgdev.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define STACK_SIZE 2048

/* How many messages can be queued for a single thread */
#define QUEUE_DEPTH 16

/* Array of worker threads, and their stacks */
static struct thread_rec {
	struct k_thread thread;
	struct k_msgq msgq;
	struct msg msgq_buf[QUEUE_DEPTH];
} threads[NUM_THREADS];

K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);

/* The static metairq thread we'll use for dispatch */
static void metairq_fn(void *p1, void *p2, void *p3);
K_THREAD_DEFINE(metairq_thread, STACK_SIZE, metairq_fn,
		NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0, 0);

/* Accumulated list of latencies, for a naive variance computation at
 * the end.
 */
struct {
	atomic_t num_mirq;
	uint32_t mirq_latencies[MAX_EVENTS];
	struct {
		uint32_t nevt;
		uint32_t latencies[MAX_EVENTS * 2 / NUM_THREADS];
	} threads[NUM_THREADS];
} stats;

/* A semaphore with an initial count, used to allow only one thread to
 * log the final report.
 */
K_SEM_DEFINE(report_cookie, 1, 1);

static void metairq_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		/* Receive a message, immediately check a timestamp
		 * and compute a latency value, then dispatch it to
		 * the queue for its target thread
		 */
		struct msg m;

		message_dev_fetch(&m);
		m.metairq_latency = k_cycle_get_32() - m.timestamp;

		int ret = k_msgq_put(&threads[m.target].msgq, &m, K_NO_WAIT);

		if (ret) {
			LOG_INF("Thread %d queue full, message %d dropped",
			       m.target, m.seq);
		}
	}
}

/* Simple recursive implementation of an integer square root, cribbed
 * from wikipedia
 */
static uint32_t isqrt(uint64_t n)
{
	if (n > 1) {
		uint64_t lo = isqrt(n >> 2) << 1;
		uint64_t hi = lo + 1;

		return (uint32_t)(((hi * hi) > n) ? lo : hi);
	}
	return (uint32_t) n;
}

static void calc_stats(const uint32_t *array, uint32_t n,
		       uint32_t *lo, uint32_t *hi, uint32_t *mean, uint32_t *stdev)
{
	uint64_t tot = 0, totsq = 0;

	*lo = INT_MAX;
	*hi = 0;
	for (int i = 0; i < n; i++) {
		*lo = MIN(*lo, array[i]);
		*hi = MAX(*hi, array[i]);
		tot += array[i];
	}

	*mean = (uint32_t)((tot + (n / 2)) / n);

	for (int i = 0; i < n; i++) {
		int64_t d = (int32_t) (array[i] - *mean);

		totsq += d * d;
	}

	*stdev = isqrt((totsq + (n / 2)) / n);
}

static void record_latencies(struct msg *m, uint32_t latency)
{
	/* Workaround: qemu emulation shows an erroneously high
	 * metairq latency for the very first event of 7-8us.  Maybe
	 * it needs to fault in the our code pages in the host?
	 */
	if (IS_ENABLED(CONFIG_QEMU_TARGET) && m->seq == 0) {
		return;
	}

	/* It might be a potential race condition in this subroutine.
	 * We check if the msg sequence is reaching the MAX EVENT first.
	 * To prevent the coming incorrect changes of the array.
	 */
	if (m->seq >= MAX_EVENTS) {
		return;
	}

	int t = m->target;
	int lidx = stats.threads[t].nevt++;

	if (lidx < ARRAY_SIZE(stats.threads[t].latencies)) {
		stats.threads[t].latencies[lidx] = latency;
	}

	stats.mirq_latencies[atomic_inc(&stats.num_mirq)] = m->metairq_latency;

	/* Once we've logged our final event, print a report.  We use
	 * a semaphore with an initial count of 1 to ensure that only
	 * one thread gets to do this.  Also events can be processed
	 * out of order, so add a small sleep to let the queues
	 * finish.
	 */
	if (m->seq == MAX_EVENTS - 1) {
		uint32_t hi, lo, mean, stdev, ret;

		ret = k_sem_take(&report_cookie, K_FOREVER);
		__ASSERT_NO_MSG(ret == 0);
		k_msleep(100);

		calc_stats(stats.mirq_latencies, stats.num_mirq,
			   &lo, &hi, &mean, &stdev);

		LOG_INF("        ---------- Latency (cyc) ----------");
		LOG_INF("            Best    Worst     Mean    Stdev");
		LOG_INF("MetaIRQ %8d %8d %8d %8d", lo, hi, mean, stdev);


		for (int i = 0; i < NUM_THREADS; i++) {
			if (stats.threads[i].nevt == 0) {
				LOG_WRN("No events for thread %d", i);
				continue;
			}

			calc_stats(stats.threads[i].latencies,
				   stats.threads[i].nevt,
				   &lo, &hi, &mean, &stdev);

			LOG_INF("Thread%d %8d %8d %8d %8d",
				i, lo, hi, mean, stdev);
		}

		LOG_INF("MetaIRQ Test Complete");
	}
}

static void thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	int id = (long)p1;
	struct msg m;

	LOG_INF("Starting Thread%d at priority %d", id,
		k_thread_priority_get(k_current_get()));

	while (true) {
		int ret = k_msgq_get(&threads[id].msgq, &m, K_FOREVER);
		uint32_t start = k_cycle_get_32();

		__ASSERT_NO_MSG(ret == 0);

		/* Spin on the CPU for the requested number of cycles
		 * doing the "work" required to "process" the event.
		 * Note the inner loop: hammering on k_cycle_get_32()
		 * on some platforms requires locking around the timer
		 * driver internals and can affect interrupt latency.
		 * Obviously we may be preempted as new events arrive
		 * and get queued.
		 */
		while (k_cycle_get_32() - start < m.proc_cyc) {
			for (volatile int i = 0; i < 100; i++) {
			}
		}

		uint32_t dur = k_cycle_get_32() - start;

#ifdef LOG_EVERY_EVENT
		/* Log the message, its thread, and the following cycle values:
		 * 1. Receive it from the driver in the MetaIRQ thread
		 * 2. Begin processing it out of the queue in the worker thread
		 * 3. The requested processing time in the message
		 * 4. The actual time taken to process the message
		 *    (may be higher if the thread was preempted)
		 */
		LOG_INF("M%d T%d mirq %d disp %d proc %d real %d",
			m.seq, id, m.metairq_latency,
			start - m.timestamp, m.proc_cyc, dur);
#endif

		/* Collect the latency values in a big statistics array */
		record_latencies(&m, start - m.timestamp);
	}
}

void main(void)
{
	for (long i = 0; i < NUM_THREADS; i++) {
		/* Each thread gets a different priority.  Half should
		 * be at (negative) cooperative priorities.  Lower
		 * thread numbers have higher priority values,
		 * e.g. thread 0 will be preempted only by the
		 * metairq.
		 */
		int prio = (-NUM_THREADS/2) + i;

		k_msgq_init(&threads[i].msgq, (char *)threads[i].msgq_buf,
			    sizeof(struct msg), QUEUE_DEPTH);

		k_thread_create(&threads[i].thread,
				thread_stacks[i], STACK_SIZE,
				thread_fn, (void *)i, NULL, NULL,
				prio, 0, K_NO_WAIT);
	}

	message_dev_init();
}
