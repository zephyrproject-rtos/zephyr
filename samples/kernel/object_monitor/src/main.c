/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/obj_core.h>
#include <zephyr/sys/printk.h>

/*
 * The application: a sensor thread feeds samples into a message queue and a
 * dispatcher starts short-lived job threads to process them. Every job owns
 * a heap-allocated request holding a semaphore, a mutex and a timer, and
 * waits on a stack-local semaphore for each processing step. Threads,
 * requests and their embedded objects come and go all the time.
 *
 * The monitor: a low priority thread that periodically walks the object
 * types and prints how many objects of each type exist, how many were not
 * registered and why, and the state and CPU share of every thread.
 */

#define NUM_JOB_SLOTS 4
#define JOB_STACK_SIZE 1024
#define MAX_THREADS 16

struct sensor_sample {
	uint32_t seq;
	uint32_t value;
};

K_MSGQ_DEFINE(samples, sizeof(struct sensor_sample), 8, 4);

struct request {
	uint32_t id;
	uint32_t steps;
	uint32_t processed;
	struct k_sem done;
	struct k_mutex lock;
	struct k_timer deadline;
};

static struct k_thread job_threads[NUM_JOB_SLOTS];
static K_THREAD_STACK_ARRAY_DEFINE(job_stacks, NUM_JOB_SLOTS, JOB_STACK_SIZE);
static bool slot_busy[NUM_JOB_SLOTS];

static uint32_t jobs_started;
static uint32_t jobs_finished;
static uint32_t samples_dropped;

static uint32_t prng(uint32_t range)
{
	static uint32_t state = 0x2545F491U;

	state = state * 1103515245U + 12345U;

	return (state >> 16) % range;
}

static void deadline_expired(struct k_timer *timer)
{
	struct request *req = k_timer_user_data_get(timer);

	/* Wake the job whatever it is waiting for */
	k_sem_give(&req->done);
}

static void job_fn(void *p1, void *p2, void *p3)
{
	struct request *req = p1;
	struct sensor_sample sample;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_timer_user_data_set(&req->deadline, req);
	k_timer_start(&req->deadline, K_MSEC(2000), K_NO_WAIT);

	for (uint32_t step = 0; step < req->steps; step++) {
		struct k_sem step_done;

		/* A stack-local semaphore per step: never registered, counted
		 * as skipped by the semaphore type.
		 */
		k_sem_init(&step_done, 0, 1);

		if (k_msgq_get(&samples, &sample, K_MSEC(300)) == 0) {
			k_mutex_lock(&req->lock, K_FOREVER);
			req->processed++;
			k_mutex_unlock(&req->lock);
		}

		/* Simulate work, then wait for the step to be signalled */
		k_busy_wait(1000U * (1U + prng(5)));
		k_sem_take(&step_done, K_MSEC(50 + prng(200)));

		if (k_sem_take(&req->done, K_NO_WAIT) == 0) {
			break; /* deadline hit */
		}
	}

	k_timer_stop(&req->deadline);
	k_free(req); /* the embedded objects stop being reported here */
	jobs_finished++;
}

static void dispatcher_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		for (size_t i = 0; i < NUM_JOB_SLOTS; i++) {
			struct request *req;
			char name[CONFIG_THREAD_MAX_NAME_LEN];

			if (slot_busy[i]) {
				if (k_thread_join(&job_threads[i], K_NO_WAIT) == 0) {
					slot_busy[i] = false;
				}
				continue;
			}

			if (prng(3) != 0) {
				continue; /* leave the slot idle for a while */
			}

			req = k_malloc(sizeof(*req));
			if (req == NULL) {
				continue;
			}

			req->id = ++jobs_started;
			req->steps = 3 + prng(8);
			req->processed = 0;
			k_sem_init(&req->done, 0, 1);
			k_mutex_init(&req->lock);
			k_timer_init(&req->deadline, deadline_expired, NULL);

			k_thread_create(&job_threads[i], job_stacks[i], JOB_STACK_SIZE, job_fn,
					req, NULL, NULL, 5, 0, K_NO_WAIT);
			snprintk(name, sizeof(name), "job-%u", req->id);
			k_thread_name_set(&job_threads[i], name);
			slot_busy[i] = true;
		}

		k_sleep(K_MSEC(200 + prng(400)));
	}
}

static void sensor_fn(void *p1, void *p2, void *p3)
{
	struct sensor_sample sample = { .seq = 0, .value = 0 };

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		sample.seq++;
		sample.value = 1000 + prng(500);
		if (k_msgq_put(&samples, &sample, K_NO_WAIT) != 0) {
			samples_dropped++;
		}
		k_sleep(K_MSEC(50 + prng(100)));
	}
}

K_THREAD_DEFINE(dispatcher, 1024, dispatcher_fn, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(sensor, 1024, sensor_fn, NULL, NULL, NULL, 4, 0, 0);

/* Monitor */

static const struct {
	uint32_t id;
	const char *name;
} types[] = {
	{ K_OBJ_TYPE_THREAD_ID, "thread" },
	{ K_OBJ_TYPE_SEM_ID, "semaphore" },
	{ K_OBJ_TYPE_MUTEX_ID, "mutex" },
	{ K_OBJ_TYPE_TIMER_ID, "timer" },
	{ K_OBJ_TYPE_MSGQ_ID, "message queue" },
};

/* Cycles of each thread at the previous refresh, to compute its CPU share
 * over the last period. Thread structures are reused by new jobs, so a
 * count that went backwards belongs to a new thread.
 */
struct thread_cycles {
	struct k_thread *thread;
	uint64_t cycles;
};

static struct thread_cycles prev_cycles[MAX_THREADS];
static uint64_t prev_cpu_cycles;
static uint64_t cpu_cycles_delta;

static uint64_t thread_cycles_delta(struct k_thread *thread, uint64_t cycles)
{
	struct thread_cycles *free_entry = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(prev_cycles); i++) {
		if (prev_cycles[i].thread == thread) {
			uint64_t prev = prev_cycles[i].cycles;

			prev_cycles[i].cycles = cycles;
			return (cycles >= prev) ? cycles - prev : cycles;
		}
		if ((prev_cycles[i].thread == NULL) && (free_entry == NULL)) {
			free_entry = &prev_cycles[i];
		}
	}

	if (free_entry != NULL) {
		free_entry->thread = thread;
		free_entry->cycles = cycles;
	}

	return cycles;
}

static int count_op(struct k_obj_core *obj_core, void *data)
{
	ARG_UNUSED(obj_core);
	(*(uint32_t *)data)++;

	return 0;
}

static int print_thread_op(struct k_obj_core *obj_core, void *data)
{
	struct k_thread *thread =
		(struct k_thread *)((uint8_t *)obj_core - obj_core->type->obj_core_offset);
	struct k_thread_runtime_stats stats;
	char state[24];
	const char *name = k_thread_name_get(thread);
	uint64_t delta;
	unsigned int percent = 0;

	ARG_UNUSED(data);

	if (k_obj_core_stats_query(obj_core, &stats, sizeof(stats)) == 0) {
		delta = thread_cycles_delta(thread, stats.total_cycles);
		if (cpu_cycles_delta != 0U) {
			percent = (unsigned int)((delta * 1000U) / cpu_cycles_delta);
		}
	}

	if (thread == k_current_get()) {
		strcpy(state, "running");
	} else if (k_thread_state_str(thread, state, sizeof(state))[0] == '\0') {
		strcpy(state, "ready");
	}

	printk("  %-12s %p  %-9s %3u.%u%%\n", (name != NULL) ? name : "?", thread, state,
	       percent / 10, percent % 10);

	return 0;
}

static void monitor_refresh(uint32_t refresh)
{
	struct k_thread_runtime_stats cpu_stats;
	int64_t uptime = k_uptime_get();

	if (k_obj_core_stats_query(K_OBJ_CORE(&_kernel), &cpu_stats, sizeof(cpu_stats)) == 0) {
		cpu_cycles_delta = cpu_stats.execution_cycles - prev_cpu_cycles;
		prev_cpu_cycles = cpu_stats.execution_cycles;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_MONITOR_CLEAR_SCREEN)) {
		printk("\x1b[2J\x1b[H");
	}

	printk("Object monitor%s   uptime %lld.%llds   refresh %u\n\n",
	       IS_ENABLED(CONFIG_TRACING_OBJECT_TRACKING) ? " (object tracking)" : "",
	       (long long)(uptime / 1000), (long long)((uptime / 100) % 10), refresh);

	printk("  %-14s %7s %8s %8s\n", "type", "objects", "skipped", "dropped");
	for (size_t i = 0; i < ARRAY_SIZE(types); i++) {
		struct k_obj_type *type = k_obj_type_find(types[i].id);
		uint32_t count = 0;

		if (type == NULL) {
			continue;
		}
		k_obj_type_walk_locked(type, count_op, &count);
		printk("  %-14s %7u %8u %8u\n", types[i].name, count, type->skipped,
		       type->dropped);
	}

	printk("\n  %-12s %-10s %-9s %6s\n", "thread", "address", "state", "cpu");
	k_obj_type_walk_locked(k_obj_type_find(K_OBJ_TYPE_THREAD_ID), print_thread_op, NULL);

	printk("\n  jobs: %u started, %u finished, %u running; samples dropped: %u\n",
	       jobs_started, jobs_finished, jobs_started - jobs_finished, samples_dropped);
}

int main(void)
{
	uint32_t refresh = 0;

	k_thread_name_set(k_current_get(), "monitor");

	while (true) {
		k_sleep(K_MSEC(CONFIG_SAMPLE_MONITOR_PERIOD_MS));
		monitor_refresh(++refresh);
	}

	return 0;
}
