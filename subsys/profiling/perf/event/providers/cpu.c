/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/arch/perf_counter.h>
#include <zephyr/kernel.h>
#include <zephyr/profiling/perf_provider.h>
#include <zephyr/sys/barrier.h>

#define COUNTER_COUNT MIN(CONFIG_PMU_MAX_COUNTERS, CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS)
#define EVENT_LIST_BATCH_SIZE 8U

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
	struct counter_state counters[COUNTER_COUNT]; /* State for each hardware counter. */
	enum cpu_operation requested_operation;       /* Operation requested by the caller. */
	uint32_t event_index;                         /* Index requested or returned by lookup. */
	const char *requested_event_name;             /* Name borrowed until lookup completes. */
	struct event_list_batch *requested_event_list; /* Buffer borrowed for listing. */
	uint64_t returned_count;                      /* Returned baseline or final count. */
	uint32_t usable_counter_count;                /* Counter limit from hardware and storage. */
	size_t allocated_counter_count;               /* Counters allocated to the session. */
	int request_result;                           /* Request's success or error status. */
	bool counting_started;                        /* Counting began; cleared after cleanup. */
	bool final_counts_ready;                      /* Final values captured after stopping. */
};

static struct cpu_provider cpus[CONFIG_MP_MAX_NUM_CPUS];

/* Fetch a bounded group of names; the caller invokes callbacks after completion. */
static int cpu_event_list(struct cpu_provider *cpu)
{
	struct event_list_batch *batch = cpu->requested_event_list;
	uint32_t index = cpu->event_index;

	if (index == 0U) {
		batch->total_event_count = perf_counter_num_events();
		if (batch->total_event_count == 0U) {
			return -ENOTSUP;
		}
	}
	while (batch->event_count < ARRAY_SIZE(batch->event_names) &&
	       index < batch->total_event_count) {
		struct perf_counter_event_info event;
		int ret = perf_counter_event_get(index, &event);

		if (ret != 0) {
			return ret;
		}
		batch->event_names[batch->event_count] = event.name;
		batch->event_count++;
		index++;
	}
	return 0;
}

/* Search the owning CPU's supported events in one request. */
static int cpu_event_lookup(struct cpu_provider *cpu)
{
	uint32_t event_count = perf_counter_num_events();

	if (event_count == 0U) {
		return -ENOTSUP;
	}
	for (uint32_t i = 0U; i < event_count; i++) {
		struct perf_counter_event_info event;
		int ret = perf_counter_event_get(i, &event);

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
	struct perf_counter_event_info event;
	int ret = perf_counter_event_get(cpu->event_index, &event);
	uint32_t counter_index;

	if (ret != 0) {
		return ret;
	}
	if (cpu->counting_started) {
		return -EBUSY;
	}
	if (cpu->allocated_counter_count == 0U) {
		ret = perf_counter_init();
		if (ret != 0) {
			return ret;
		}
		cpu->usable_counter_count =
			MIN(perf_counter_num_counters(), (uint32_t)ARRAY_SIZE(cpu->counters));
		if (cpu->usable_counter_count == 0U) {
			return -ENOTSUP;
		}
		perf_counter_stop();
		perf_counter_disable_all();
	}
	for (counter_index = 0U; counter_index < cpu->usable_counter_count; counter_index++) {
		if (!cpu->counters[counter_index].allocated) {
			break;
		}
	}
	if (counter_index == cpu->usable_counter_count) {
		return -ENOSPC;
	}
	ret = perf_counter_config(counter_index, event.event);
	if (ret != 0) {
		perf_counter_disable(counter_index);
		return ret;
	}
	perf_counter_reset(counter_index);
	perf_counter_clear_overflow(counter_index);
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
		unsigned int key = arch_irq_lock();

		for (uint32_t i = 0U; i < cpu->usable_counter_count; i++) {
			if (cpu->counters[i].allocated) {
				perf_counter_enable(i);
			}
		}
		perf_counter_start();
		barrier_isync_fence_full();
		cpu->counting_started = true;
		arch_irq_unlock(key);
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
		perf_counter_stop();
		barrier_isync_fence_full();
		for (uint32_t i = 0U; i < cpu->usable_counter_count; i++) {
			if (cpu->counters[i].allocated) {
				cpu->counters[i].final_count = perf_counter_read(i);
				cpu->counters[i].count_error =
					perf_counter_overflow(i) ? -EOVERFLOW : 0;
			}
		}
		cpu->final_counts_ready = true;
	}
	cpu->returned_count = counter->final_count;
	ret = counter->count_error;
	perf_counter_disable((uint32_t)(counter - cpu->counters));
	perf_counter_clear_overflow((uint32_t)(counter - cpu->counters));
	*counter = (struct counter_state){0};
	cpu->allocated_counter_count--;
	/* Keep the session flags until all prepared counters have been released. */
	if (cpu->allocated_counter_count == 0U) {
		perf_counter_stop();
		perf_counter_disable_all();
		cpu->counting_started = false;
		cpu->final_counts_ready = false;
	}
	return ret;
}

static int cpu_dispatch(struct cpu_provider *cpu)
{
	switch (cpu->requested_operation) {
	case CPU_EVENT_LIST:
		return cpu_event_list(cpu);
	case CPU_EVENT_LOOKUP:
		return cpu_event_lookup(cpu);
	case CPU_PREPARE:
		return cpu_prepare(cpu);
	case CPU_START:
		return cpu_start(cpu);
	case CPU_STOP:
		return cpu_stop(cpu);
	default:
		return -EINVAL;
	}
}

#ifdef CONFIG_SMP
struct cpu_request {
	struct k_ipi_work work;
	struct cpu_provider *cpu;
};

static void cpu_ipi_handler(struct k_ipi_work *work)
{
	struct cpu_request *request = CONTAINER_OF(work, struct cpu_request, work);

	request->cpu->request_result = cpu_dispatch(request->cpu);
}
#endif

/*
 * PMU registers are CPU-local, including those used for event discovery.
 * The perf core serializes callbacks. Remote requests complete before returning,
 * so borrowed arguments and results remain valid for the entire operation.
 */
static int cpu_call(struct cpu_provider *cpu, enum cpu_operation operation, uint64_t id)
{
	uint32_t index = (uint32_t)(cpu - cpus);
	unsigned int key;
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
	cpu->requested_operation = operation;
	cpu->event_index = (uint32_t)id;

	/*
	 * Keep CPU selection and PMU access on the same CPU. irq_lock()
	 * also takes the SMP global lock, which can deadlock with PMU
	 * logging. The perf core mutex already serializes requests.
	 */
	key = arch_irq_lock();
	if (arch_curr_cpu()->id == index) {
		ret = cpu_dispatch(cpu);
		arch_irq_unlock(key);
		return ret;
	}
#ifdef CONFIG_SMP
	/* One reusable request is sufficient under the perf core's mutex. */
	static struct cpu_request request;
	static bool request_initialized;

	if (!request_initialized) {
		k_ipi_work_init(&request.work);
		request_initialized = true;
	}
	request.cpu = cpu;
	ret = k_ipi_work_add(&request.work, BIT(index), cpu_ipi_handler);
	if (ret == 0) {
		k_ipi_work_signal();
	}
	arch_irq_unlock(key);
	if (ret != 0) {
		return ret;
	}
	/* Waiting with interrupts locked can deadlock cross-CPU IPI work. */
	ret = k_ipi_work_wait(&request.work, K_FOREVER);
	return ret == 0 ? cpu->request_result : ret;
#else
	arch_irq_unlock(key);
	return -ENODEV;
#endif
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
