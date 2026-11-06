/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Validation for the hardware-backed CPU performance-counter provider.
 *
 * These tests prove that the provider reports genuine hardware counts (the
 * counters strictly advance across a workload) rather than the placeholder
 * zeros returned by the initial dummy provider.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/profiling/perf.h>

/* A workload heavy enough that cycles and retired instructions clearly move. */
static volatile uint64_t sink;

static void busy_workload(void)
{
	uint64_t acc = 0;

	for (volatile uint32_t i = 0; i < 200000U; i++) {
		acc += i ^ (acc << 1);
	}

	sink = acc;
}

static uint64_t measure_single(const char *event_name)
{
	struct perf_event_handle handle;
	int err = perf_event_lookup(event_name, &handle);

	zassert_ok(err, "lookup of %s failed (%d)", event_name, err);

	struct perf_stat_config cfg = {
		.events = &handle,
		.num_events = 1,
	};

	err = perf_stat_start(&cfg);
	zassert_ok(err, "start of %s failed (%d)", event_name, err);

	busy_workload();

	err = perf_stat_stop(&cfg);
	zassert_ok(err, "stop of %s failed (%d)", event_name, err);
	zassert_ok(handle.status, "%s reported status %d", event_name, handle.status);

	return handle.final_count - handle.baseline;
}

ZTEST(perf_provider_cpu, test_cycles_advance)
{
	uint64_t delta = measure_single("cpu0.cycles");

	zassert_true(delta > 0, "cycle counter did not advance (delta=%llu)", delta);
}

ZTEST(perf_provider_cpu, test_instructions_advance)
{
	uint64_t delta = measure_single("cpu0.instructions");

	zassert_true(delta > 0, "instruction counter did not advance (delta=%llu)", delta);
}

ZTEST(perf_provider_cpu, test_lookup_unknown_event)
{
	struct perf_event_handle handle;

	zassert_equal(perf_event_lookup("cpu0.does-not-exist", &handle), -ENOENT,
		      "unknown event should not resolve");
}

ZTEST(perf_provider_cpu, test_multi_event_session)
{
	struct perf_event_handle handles[2];

	zassert_ok(perf_event_lookup("cpu0.cycles", &handles[0]), "cycles lookup failed");
	zassert_ok(perf_event_lookup("cpu0.instructions", &handles[1]),
		   "instructions lookup failed");

	struct perf_stat_config cfg = {
		.events = handles,
		.num_events = ARRAY_SIZE(handles),
	};

	zassert_ok(perf_stat_start(&cfg), "multi-event start failed");
	busy_workload();
	zassert_ok(perf_stat_stop(&cfg), "multi-event stop failed");

	zassert_true(handles[0].final_count - handles[0].baseline > 0, "cycles did not advance");
	zassert_true(handles[1].final_count - handles[1].baseline > 0,
		     "instructions did not advance");
}

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
ZTEST(perf_provider_cpu, test_secondary_cpu_counts)
{
	/* Steered onto CPU 1 by the provider; proves per-CPU counting. */
	uint64_t delta = measure_single("cpu1.cycles");

	zassert_true(delta > 0, "cpu1 cycle counter did not advance (delta=%llu)", delta);
}
#endif

ZTEST_SUITE(perf_provider_cpu, NULL, NULL, NULL, NULL, NULL);
