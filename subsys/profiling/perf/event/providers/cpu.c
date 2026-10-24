/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
 * SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CPU performance-counter provider backed by the portable PMU API.
 *
 * This provider replaces the initial dummy cpu0 provider with a real,
 * hardware-backed implementation built on the architecture-neutral
 * <zephyr/arch/pmu.h> primitives (ARMv8-A PMUv3 backend today; any future
 * backend that implements pmu_*() works unchanged).
 *
 * One provider is registered per logical CPU (cpu0, cpu1, ...). Because PMUv3
 * event counters are strictly per-CPU, every start()/stop() is executed on the
 * target CPU: inline when the caller already runs there (or on a UP build),
 * otherwise on a short-lived worker thread pinned with k_thread_cpu_pin()
 * (requires CONFIG_SMP && CONFIG_SCHED_CPU_MASK).
 *
 * Event model
 * -----------
 * The core passes provider-local event IDs unchanged from lookup() to
 * prepare()/start()/stop(). Here an event ID is an index into cpu_events[].
 * The "cycles" event uses the dedicated cycle counter; every other event is
 * mapped onto a general-purpose event counter allocated on first start().
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/pmu.h>
#include <zephyr/sys/util.h>

#include "../provider.h"

/** Sentinel event ID for the dedicated cycle counter. */
#define CPU_EVENT_CYCLES 0U

/** Human-visible event mapped onto an architectural PMU event code. */
struct cpu_event {
	const char *name;
	const char *description;
	bool is_cycle;
	pmu_evt_t event;
};

/*
 * Curated, portable subset of the common ARMv8-A PMUv3 architectural events.
 * Index into this table is the provider-local event ID. "cycles" must stay at
 * index 0 (CPU_EVENT_CYCLES) so it maps to the dedicated cycle counter.
 */
static const struct cpu_event cpu_events[] = {
	{"cycles", "CPU cycle counter", true, PMU_EVT_CPU_CYCLES},
	{"instructions", "Instructions architecturally retired", false, PMU_EVT_INST_RETIRED},
	{"branches", "Predictable branches", false, PMU_EVT_BR_PRED},
	{"branch-misses", "Mispredicted or not-predicted branches", false, PMU_EVT_BR_MIS_PRED},
	{"l1d-cache", "L1 data cache accesses", false, PMU_EVT_L1D_CACHE},
	{"l1d-cache-refill", "L1 data cache refills", false, PMU_EVT_L1D_CACHE_REFILL},
	{"l1i-cache-refill", "L1 instruction cache refills", false, PMU_EVT_L1I_CACHE_REFILL},
	{"l2d-cache-refill", "L2 data cache refills", false, PMU_EVT_L2D_CACHE_REFILL},
	{"mem-access", "Data memory accesses", false, PMU_EVT_MEM_ACCESS},
	{"bus-access", "Bus accesses", false, PMU_EVT_BUS_ACCESS},
};

#define CPU_NUM_EVENTS ARRAY_SIZE(cpu_events)

BUILD_ASSERT(CPU_NUM_EVENTS <= UINT16_MAX, "event ID must fit the provider-local range");

/** Per-CPU provider state. */
struct cpu_provider_ctx {
	uint8_t cpu_id;
	bool inited;
	/* Counter index assigned to each active event, or -1 when inactive. */
	int8_t event_counter[CPU_NUM_EVENTS];
	/* Bitmap of general-purpose event counters currently in use. */
	uint32_t used_counters;
	struct k_spinlock lock;
};

static struct cpu_provider_ctx cpu_ctx[CONFIG_MP_MAX_NUM_CPUS];

/* ------------------------------------------------------------------------- */
/* Per-CPU execution helper                                                  */
/* ------------------------------------------------------------------------- */

/* Operation dispatched onto the target CPU. */
struct pmu_op {
	struct cpu_provider_ctx *ctx;
	uint64_t event_id;
	uint64_t value; /* out: baseline or final count */
	int ret;        /* out */
	int (*fn)(struct cpu_provider_ctx *ctx, uint64_t event_id, uint64_t *value);
};

static inline void pmu_op_invoke(struct pmu_op *op)
{
	op->ret = op->fn(op->ctx, op->event_id, &op->value);
}

#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_CPU_MASK)

#define CPU_PROVIDER_STEERING 1

/* Cooperative priority so the pinned worker runs to completion without preemption. */
#define CPU_WORKER_PRIO      K_PRIO_COOP(0)
#define CPU_WORKER_STACKSIZE 1024

static K_THREAD_STACK_ARRAY_DEFINE(cpu_worker_stacks, CONFIG_MP_MAX_NUM_CPUS,
				   CPU_WORKER_STACKSIZE);
static struct k_thread cpu_worker[CONFIG_MP_MAX_NUM_CPUS];

static void pmu_op_worker(void *a, void *b, void *c)
{
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	pmu_op_invoke((struct pmu_op *)a);
}

static int run_on_cpu(uint8_t cpu_id, struct pmu_op *op)
{
	/*
	 * Run the PMU access on a worker pinned to the target CPU. This is
	 * correct even when the caller already runs on that CPU (the worker is
	 * simply scheduled there). PMUv3 counters are per-CPU, so the access
	 * must not migrate mid-measurement.
	 */
	k_tid_t tid = k_thread_create(&cpu_worker[cpu_id], cpu_worker_stacks[cpu_id],
				      CPU_WORKER_STACKSIZE, pmu_op_worker, op, NULL, NULL,
				      CPU_WORKER_PRIO, 0, K_FOREVER);

	int err = k_thread_cpu_pin(tid, cpu_id);

	if (err != 0) {
		k_thread_abort(tid);
		return err;
	}

	k_thread_start(tid);
	k_thread_join(tid, K_FOREVER);

	return op->ret;
}

#else /* UP, or SMP without CPU-mask steering */

static int run_on_cpu(uint8_t cpu_id, struct pmu_op *op)
{
	ARG_UNUSED(cpu_id);
	pmu_op_invoke(op);
	return op->ret;
}

#endif

/* ------------------------------------------------------------------------- */
/* PMU operations (always executed on the target CPU)                        */
/* ------------------------------------------------------------------------- */

static int ctx_ensure_init(struct cpu_provider_ctx *ctx)
{
	if (ctx->inited) {
		return 0;
	}

	int err = pmu_init();

	if (err != 0) {
		return err;
	}

	ctx->inited = true;
	return 0;
}

static int op_start(struct cpu_provider_ctx *ctx, uint64_t event_id, uint64_t *baseline)
{
	const struct cpu_event *ev = &cpu_events[event_id];
	int err = ctx_ensure_init(ctx);

	if (err != 0) {
		return err;
	}

	if (ev->is_cycle) {
		pmu_cycle_reset();
		/* Global enable (PMCR.E); also enables the cycle counter (bit 31). */
		pmu_start();
		K_SPINLOCK(&ctx->lock) {
			ctx->event_counter[event_id] = INT8_MAX; /* mark active (cycle) */
		}
		*baseline = pmu_cycle_count();
		return 0;
	}

	/* Allocate a free general-purpose counter for this event. */
	uint32_t ncounters = pmu_num_counters();
	int counter = -1;

	K_SPINLOCK(&ctx->lock) {
		for (uint32_t i = 0; i < ncounters && i < 32U; i++) {
			if ((ctx->used_counters & BIT(i)) == 0U) {
				ctx->used_counters |= BIT(i);
				ctx->event_counter[event_id] = (int8_t)i;
				counter = (int)i;
				break;
			}
		}
	}

	if (counter < 0) {
		return -ENOSPC;
	}

	err = pmu_counter_config((uint32_t)counter, ev->event);
	if (err != 0) {
		K_SPINLOCK(&ctx->lock) {
			ctx->used_counters &= ~BIT(counter);
			ctx->event_counter[event_id] = -1;
		}
		return err;
	}

	pmu_counter_reset((uint32_t)counter);
	pmu_counter_enable((uint32_t)counter);
	/* Global enable (PMCR.E) so the configured counter starts counting. */
	pmu_start();
	*baseline = pmu_counter_read((uint32_t)counter);

	return 0;
}

static int op_stop(struct cpu_provider_ctx *ctx, uint64_t event_id, uint64_t *final_count)
{
	const struct cpu_event *ev = &cpu_events[event_id];
	int8_t counter;

	K_SPINLOCK(&ctx->lock) {
		counter = ctx->event_counter[event_id];
	}

	if (counter < 0) {
		return -EINVAL; /* stop without a matching start */
	}

	if (ev->is_cycle) {
		*final_count = pmu_cycle_count();
		K_SPINLOCK(&ctx->lock) {
			ctx->event_counter[event_id] = -1;
		}
		return 0;
	}

	*final_count = pmu_counter_read((uint32_t)counter);
	pmu_counter_disable((uint32_t)counter);

	K_SPINLOCK(&ctx->lock) {
		ctx->used_counters &= ~BIT(counter);
		ctx->event_counter[event_id] = -1;
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
/* provider_api implementation                                               */
/* ------------------------------------------------------------------------- */

static int cpu_provider_list(void *context, perf_event_list_cb_t callback, void *user_data)
{
	ARG_UNUSED(context);

	if (callback == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < CPU_NUM_EVENTS; i++) {
		int err = callback(cpu_events[i].name, cpu_events[i].description, user_data);

		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static int cpu_provider_lookup(void *context, const char *event_name, uint64_t *event_id)
{
	ARG_UNUSED(context);

	if (event_name == NULL || event_id == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < CPU_NUM_EVENTS; i++) {
		if (strcmp(event_name, cpu_events[i].name) == 0) {
			*event_id = (uint64_t)i;
			return 0;
		}
	}

	return -ENOENT;
}

static int cpu_provider_prepare(void *context, uint64_t event_id)
{
	ARG_UNUSED(context);

	return event_id < CPU_NUM_EVENTS ? 0 : -EINVAL;
}

static int cpu_provider_start(void *context, uint64_t event_id, uint64_t *baseline)
{
	struct cpu_provider_ctx *ctx = context;

	if (ctx == NULL || baseline == NULL || event_id >= CPU_NUM_EVENTS) {
		return -EINVAL;
	}

	struct pmu_op op = {
		.ctx = ctx,
		.event_id = event_id,
		.fn = op_start,
	};

	int ret = run_on_cpu(ctx->cpu_id, &op);

	if (ret == 0) {
		*baseline = op.value;
	}

	return ret;
}

static int cpu_provider_stop(void *context, uint64_t event_id, uint64_t *final_count)
{
	struct cpu_provider_ctx *ctx = context;

	if (ctx == NULL || final_count == NULL || event_id >= CPU_NUM_EVENTS) {
		return -EINVAL;
	}

	struct pmu_op op = {
		.ctx = ctx,
		.event_id = event_id,
		.fn = op_stop,
	};

	int ret = run_on_cpu(ctx->cpu_id, &op);

	if (ret == 0) {
		*final_count = op.value;
	}

	return ret;
}

static const struct perf_event_provider_api cpu_provider_api = {
	.list = cpu_provider_list,
	.lookup = cpu_provider_lookup,
	.prepare = cpu_provider_prepare,
	.start = cpu_provider_start,
	.stop = cpu_provider_stop,
};

/* ------------------------------------------------------------------------- */
/* Per-CPU provider registration: cpu0, cpu1, ...                            */
/* ------------------------------------------------------------------------- */

#define CPU_PROVIDER_INIT(n, _)                                                                    \
	static const STRUCT_SECTION_ITERABLE(perf_event_provider, perf_event_provider_cpu##n) = {  \
		.name = "cpu" #n,                                                                  \
		.description = "CPU " #n " hardware performance counters",                         \
		.api = &cpu_provider_api,                                                          \
		.context = &cpu_ctx[n],                                                            \
	}

static int cpu_provider_ctx_init(void)
{
	for (size_t c = 0; c < ARRAY_SIZE(cpu_ctx); c++) {
		cpu_ctx[c].cpu_id = (uint8_t)c;
		for (size_t e = 0; e < CPU_NUM_EVENTS; e++) {
			cpu_ctx[c].event_counter[e] = -1;
		}
	}

	return 0;
}

SYS_INIT(cpu_provider_ctx_init, PRE_KERNEL_1, 0);

LISTIFY(CONFIG_MP_MAX_NUM_CPUS, CPU_PROVIDER_INIT, (;));
