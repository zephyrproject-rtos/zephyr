/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/arch/pmu.h>
#include <zephyr/kernel.h>
#include <zephyr/profiling/perf_provider.h>
#include <zephyr/sys/barrier.h>

#define COUNTER_COUNT MIN(CONFIG_PMU_MAX_COUNTERS, CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS)
#define EVENT_LIST_BATCH_SIZE 8U

BUILD_ASSERT(CONFIG_NUM_METAIRQ_PRIORITIES > 0, "CPU perf providers require a meta-IRQ priority");

struct counter_state {
	uint32_t event_index; /* Catalog index assigned to this counter. */
	uint64_t final_count; /* Count captured when counting stops. */
	int count_error;      /* Saved status: 0 or -EOVERFLOW. */
	bool allocated;       /* Counter is assigned to an event. */
};

struct event_list_batch {
	const char *event_names[EVENT_LIST_BATCH_SIZE]; /* Names with static lifetime. */
	uint32_t event_count;                           /* Names returned by this request. */
	uint32_t total_event_count;                     /* Supported events for this listing. */
};

enum cpu_operation {
	CPU_EVENT_LIST,
	CPU_EVENT_LOOKUP,
	CPU_PREPARE,
	CPU_START,
	CPU_STOP,
};

struct cpu_provider {
	struct k_thread worker_thread;                /* Worker pinned to this CPU. */
	struct k_sem request_ready;                   /* Caller signals a pending request. */
	struct k_sem request_done;                    /* Worker signals request completion. */
	struct counter_state counters[COUNTER_COUNT]; /* State for each hardware counter. */
	enum cpu_operation requested_operation;       /* Operation requested by the caller. */
	uint32_t event_index;                         /* Index requested or returned by lookup. */
	const char *requested_event_name;             /* Name borrowed until lookup completes. */
	struct event_list_batch *requested_event_list; /* Buffer borrowed for listing. */
	uint64_t returned_count;                      /* Returned baseline or final count. */
	uint32_t usable_counter_count;                /* Counter limit from hardware and storage. */
	size_t allocated_counter_count;               /* Counters allocated to the session. */
	int request_result;                           /* Request's success or error status. */
	bool worker_initialized;                      /* Worker and semaphores initialized. */
	bool counting_started;                        /* Counting began; cleared after cleanup. */
	bool final_counts_ready;                      /* Final values captured after stopping. */
};

static struct cpu_provider cpus[CONFIG_MP_MAX_NUM_CPUS];
K_KERNEL_STACK_ARRAY_DEFINE(cpu_stacks, CONFIG_MP_MAX_NUM_CPUS,
			    CONFIG_PROFILING_PERF_EVENT_PROVIDER_CPU_STACK_SIZE);

/* Fetch a bounded group of names; the caller invokes callbacks after completion. */
static int cpu_event_list(struct cpu_provider *cpu)
{
	struct event_list_batch *batch = cpu->requested_event_list;
	uint32_t index = cpu->event_index;

	if (index == 0U) {
		batch->total_event_count = pmu_num_events();
		if (batch->total_event_count == 0U) {
			return -ENOTSUP;
		}
	}
	while (batch->event_count < ARRAY_SIZE(batch->event_names) &&
	       index < batch->total_event_count) {
		struct pmu_event_info event;
		int ret = pmu_event_get(index, &event);

		if (ret != 0) {
			return ret;
		}
		batch->event_names[batch->event_count] = event.name;
		batch->event_count++;
		index++;
	}
	return 0;
}

/* Search the owning CPU's supported events in one worker request. */
static int cpu_event_lookup(struct cpu_provider *cpu)
{
	uint32_t event_count = pmu_num_events();

	if (event_count == 0U) {
		return -ENOTSUP;
	}
	for (uint32_t i = 0U; i < event_count; i++) {
		struct pmu_event_info event;
		int ret = pmu_event_get(i, &event);

		if (ret != 0) {
			return ret;
		}
		if (strcmp(cpu->requested_event_name, event.name) == 0) {
			cpu->event_index = i;
			return 0;
		}
	}
	return -ENOENT;
}

/*
 * Initialize the PMU when preparing the first event on this CPU.
 * Reserve, configure and reset one counter per call, leaving it disabled.
 * The core prepares all session events before calling start() for any event.
 */
static int cpu_prepare(struct cpu_provider *cpu)
{
	struct pmu_event_info event;
	int ret = pmu_event_get(cpu->event_index, &event);
	uint32_t counter_index;

	if (ret != 0) {
		return ret;
	}
	if (cpu->counting_started) {
		return -EBUSY;
	}
	if (cpu->allocated_counter_count == 0U) {
		ret = pmu_init();
		if (ret != 0) {
			return ret;
		}
		cpu->usable_counter_count =
			MIN(pmu_num_counters(), (uint32_t)ARRAY_SIZE(cpu->counters));
		if (cpu->usable_counter_count == 0U) {
			return -ENOTSUP;
		}
		pmu_stop();
		pmu_counter_disable_all();
	}
	for (counter_index = 0U; counter_index < cpu->usable_counter_count; counter_index++) {
		if (!cpu->counters[counter_index].allocated) {
			break;
		}
	}
	if (counter_index == cpu->usable_counter_count) {
		return -ENOSPC;
	}
	ret = pmu_counter_config(counter_index, event.event);
	if (ret != 0) {
		pmu_counter_disable(counter_index);
		return ret;
	}
	pmu_counter_reset(counter_index);
	pmu_counter_clear_overflow(counter_index);
	cpu->counters[counter_index] =
		(struct counter_state){.event_index = cpu->event_index, .allocated = true};
	cpu->allocated_counter_count++;
	return 0;
}

static struct counter_state *cpu_find_counter(struct cpu_provider *cpu)
{
	for (size_t i = 0U; i < cpu->usable_counter_count; i++) {
		if (cpu->counters[i].allocated &&
		    cpu->counters[i].event_index == cpu->event_index) {
			return &cpu->counters[i];
		}
	}
	return NULL;
}

/*
 * The first start enables all prepared counters before starting this CPU's PMU.
 * Later per-event starts leave it running. Each CPU starts independently.
 */
static int cpu_start(struct cpu_provider *cpu)
{
	if (cpu_find_counter(cpu) == NULL || cpu->final_counts_ready) {
		return -EINVAL;
	}
	if (!cpu->counting_started) {
		unsigned int key = irq_lock();

		for (uint32_t i = 0U; i < cpu->usable_counter_count; i++) {
			if (cpu->counters[i].allocated) {
				pmu_counter_enable(i);
			}
		}
		pmu_start();
		barrier_isync_fence_full();
		cpu->counting_started = true;
		irq_unlock(key);
	}
	/* Use the reset value even for events already counting after the first start. */
	cpu->returned_count = 0U;
	return 0;
}

/*
 * The first stop stops the PMU and saves all allocated counters on this CPU.
 * Each call returns one saved result and releases that event's counter.
 * If counting never started, the prepared counter's saved result is still zero.
 */
static int cpu_stop(struct cpu_provider *cpu)
{
	struct counter_state *counter = cpu_find_counter(cpu);
	int ret;

	if (counter == NULL) {
		return -EINVAL;
	}
	if (cpu->counting_started && !cpu->final_counts_ready) {
		pmu_stop();
		barrier_isync_fence_full();
		for (uint32_t i = 0U; i < cpu->usable_counter_count; i++) {
			if (cpu->counters[i].allocated) {
				cpu->counters[i].final_count = pmu_counter_read(i);
				cpu->counters[i].count_error =
					pmu_counter_overflow(i) ? -EOVERFLOW : 0;
			}
		}
		cpu->final_counts_ready = true;
	}
	cpu->returned_count = counter->final_count;
	ret = counter->count_error;
	pmu_counter_disable((uint32_t)(counter - cpu->counters));
	pmu_counter_clear_overflow((uint32_t)(counter - cpu->counters));
	*counter = (struct counter_state){0};
	cpu->allocated_counter_count--;
	/* Keep the session flags until all prepared counters have been released. */
	if (cpu->allocated_counter_count == 0U) {
		pmu_stop();
		pmu_counter_disable_all();
		cpu->counting_started = false;
		cpu->final_counts_ready = false;
	}
	return ret;
}

static void cpu_worker(void *arg, void *unused1, void *unused2)
{
	struct cpu_provider *cpu = arg;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	for (;;) {
		int ret = k_sem_take(&cpu->request_ready, K_FOREVER);

		if (ret != 0) {
			continue;
		}
		switch (cpu->requested_operation) {
		case CPU_EVENT_LIST:
			cpu->request_result = cpu_event_list(cpu);
			break;
		case CPU_EVENT_LOOKUP:
			cpu->request_result = cpu_event_lookup(cpu);
			break;
		case CPU_PREPARE:
			cpu->request_result = cpu_prepare(cpu);
			break;
		case CPU_START:
			cpu->request_result = cpu_start(cpu);
			break;
		case CPU_STOP:
			cpu->request_result = cpu_stop(cpu);
			break;
		default:
			cpu->request_result = -EINVAL;
			break;
		}
		k_sem_give(&cpu->request_done);
	}
}

/*
 * PMU capability and counter registers are CPU-local, so even event discovery
 * runs on the pinned worker. The perf core serializes callbacks; waiting here
 * keeps each request and its result within one callback.
 */
static int cpu_call(struct cpu_provider *cpu, enum cpu_operation operation, uint64_t id)
{
	uint32_t index = (uint32_t)(cpu - cpus);
	int ret;

	if (index >= arch_num_cpus()) {
		return -ENODEV;
	}
	/* Deferred CPUs have no current thread until they enter kernel startup. */
	if (IS_ENABLED(CONFIG_SMP) && _kernel.cpus[index].current == NULL) {
		return -ENODEV;
	}
	if (id > UINT32_MAX) {
		return -EINVAL;
	}
	if (!cpu->worker_initialized) {
		ret = k_sem_init(&cpu->request_ready, 0U, 1U);
		if (ret != 0) {
			return ret;
		}
		ret = k_sem_init(&cpu->request_done, 0U, 1U);
		if (ret != 0) {
			return ret;
		}
		/* Meta-IRQ requests can preempt cooperative and scheduler-locked workloads. */
		k_thread_create(&cpu->worker_thread, cpu_stacks[index],
				K_KERNEL_STACK_SIZEOF(cpu_stacks[index]), cpu_worker, cpu, NULL,
				NULL, K_HIGHEST_THREAD_PRIO, 0U, K_FOREVER);
#ifdef CONFIG_SMP
		ret = k_thread_cpu_pin(&cpu->worker_thread, (int)index);
		if (ret != 0) {
			k_thread_abort(&cpu->worker_thread);
			return ret;
		}
#endif
		cpu->worker_initialized = true;
		k_thread_start(&cpu->worker_thread);
	}
	cpu->requested_operation = operation;
	cpu->event_index = (uint32_t)id;
	k_sem_give(&cpu->request_ready);
	ret = k_sem_take(&cpu->request_done, K_FOREVER);
	return ret == 0 ? cpu->request_result : ret;
}

static int provider_list(void *context, perf_event_list_cb_t callback, void *user_data)
{
	struct cpu_provider *cpu = context;
	struct event_list_batch batch = {0};
	uint32_t next_event_index = 0U;
	int ret;

	do {
		/* A failed dispatch must not reuse names from the previous request. */
		batch.event_count = 0U;
		cpu->requested_event_list = &batch;
		ret = cpu_call(cpu, CPU_EVENT_LIST, next_event_index);
		cpu->requested_event_list = NULL;
		/* Keep names local so callbacks can safely start another listing or lookup. */
		for (uint32_t i = 0U; i < batch.event_count; i++) {
			int callback_ret =
				callback(batch.event_names[i], batch.event_names[i], user_data);

			if (callback_ret != 0) {
				return callback_ret;
			}
		}
		/* Deliver successfully fetched names before reporting a later read error. */
		if (ret != 0) {
			return ret;
		}
		next_event_index += batch.event_count;
	} while (next_event_index < batch.total_event_count);
	return 0;
}

static int provider_lookup(void *context, const char *name, uint64_t *id)
{
	struct cpu_provider *cpu = context;
	int ret;

	/* The core serializes requests, and the name stays valid until completion. */
	cpu->requested_event_name = name;
	ret = cpu_call(cpu, CPU_EVENT_LOOKUP, 0U);
	cpu->requested_event_name = NULL;
	if (ret == 0) {
		*id = cpu->event_index;
	}
	return ret;
}

static int provider_prepare(void *context, uint64_t id)
{
	return cpu_call(context, CPU_PREPARE, id);
}

static int provider_start(void *context, uint64_t id, uint64_t *baseline)
{
	struct cpu_provider *cpu = context;
	int ret = cpu_call(cpu, CPU_START, id);

	if (ret == 0) {
		*baseline = cpu->returned_count;
	}
	return ret;
}

static int provider_stop(void *context, uint64_t id, uint64_t *final_count)
{
	struct cpu_provider *cpu = context;
	int ret = cpu_call(cpu, CPU_STOP, id);

	*final_count = cpu->returned_count;
	return ret;
}

static const struct perf_event_provider_api cpu_api = {
	.list = provider_list,
	.lookup = provider_lookup,
	.prepare = provider_prepare,
	.start = provider_start,
	.stop = provider_stop,
};

#define CPU_PROVIDER(n, unused)                                                                    \
	PERF_EVENT_PROVIDER_DEFINE(cpu##n, "CPU performance counters", &cpu_api, &cpus[n]);

LISTIFY(CONFIG_MP_MAX_NUM_CPUS, CPU_PROVIDER, ())
