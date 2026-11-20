/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/arch/pmu.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/profiling/perf.h>
#include <zephyr/profiling/perf_provider.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/ztest.h>

#include "internal.h"

static struct perf_event_handle handles[CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS];
static struct perf_stat_config config = {.events = handles};
static struct k_thread helper_thread;
K_THREAD_STACK_DEFINE(helper_stack, 2048);

struct helper_job {
	void (*function)(void *argument);
	void *argument;
};

static void helper_entry(void *arg, void *unused1, void *unused2)
{
	struct helper_job *job = arg;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	job->function(job->argument);
}

static void on_cpu(uint32_t cpu, void (*function)(void *argument), void *argument)
{
	struct helper_job job = {.function = function, .argument = argument};

	k_thread_create(&helper_thread, helper_stack, K_THREAD_STACK_SIZEOF(helper_stack),
			helper_entry, &job, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0U, K_FOREVER);
#ifdef CONFIG_SMP
	zassert_ok(k_thread_cpu_pin(&helper_thread, (int)cpu));
#else
	ARG_UNUSED(cpu);
#endif
	k_thread_start(&helper_thread);
	zassert_ok(k_thread_join(&helper_thread, K_FOREVER));
}

static void increment(void *arg)
{
	uint32_t count = *(uint32_t *)arg;

	for (uint32_t i = 0U; i < count; i++) {
		__asm__ volatile("msr pmswinc_el0, %0" : : "r"(1UL) : "memory");
	}
	barrier_isync_fence_full();
}

static void overflow(void *arg)
{
	ARG_UNUSED(arg);
	unsigned int key = irq_lock();
	uint32_t count = 3U;

	write_pmselr_el0(0U);
	barrier_isync_fence_full();
	write_pmxevcntr_el0(UINT32_MAX - 1U);
	barrier_isync_fence_full();
	increment(&count);
	irq_unlock(key);
}

static void check_disabled(void *arg)
{
	ARG_UNUSED(arg);
	zassert_equal(read_pmcr_el0() & PMCR_E, 0U);
	zassert_equal(read_pmcntenset_el0(), 0U);
}

static void check_group(void *arg)
{
	ARG_UNUSED(arg);
	zassert_equal(read_pmcntenset_el0() & 3U, 3U, "Both prepared counters must be enabled");
	zassert_not_equal(read_pmcr_el0() & PMCR_E, 0U);
}

static void select_event(size_t index, const char *name)
{
	zassert_ok(perf_event_lookup(name, &handles[index]), "Unavailable event: %s", name);
}

static void recovery(uint32_t cpu)
{
	char name[32];
	uint32_t count = 13U + cpu;
	int length = snprintk(name, sizeof(name), "cpu%u.SW_INCR", cpu);

	zassert_true(length > 0 && (size_t)length < sizeof(name));
	config.num_events = 1U;
	select_event(0U, name);

	zassert_ok(perf_stat_start(&config));
	on_cpu(cpu, increment, &count);
	zassert_ok(perf_stat_stop(&config));
	zassert_ok(handles[0].status);
	zassert_equal(handles[0].final_count - handles[0].baseline, count);
}

ZTEST(perf_arm64, test_cycles)
{
	config.num_events = 1U;
	select_event(0U, "cpu0.CPU_CYCLES");
	zassert_ok(perf_stat_start(&config));
	k_busy_wait(100U);
	zassert_ok(perf_stat_stop(&config));
	zassert_ok(handles[0].status);
	zassert_true(handles[0].final_count > handles[0].baseline);
	recovery(0U);
}

ZTEST(perf_arm64, test_group_enable)
{
	uint32_t count = 23U;

	config.num_events = 2U;
	select_event(0U, "cpu0.SW_INCR");
	select_event(1U, "cpu0.CPU_CYCLES");
	zassert_ok(perf_stat_start(&config));
	on_cpu(0U, check_group, NULL);
	on_cpu(0U, increment, &count);
	zassert_ok(perf_stat_stop(&config));
	for (size_t i = 0U; i < config.num_events; i++) {
		zassert_ok(handles[i].status);
	}
	zassert_equal(handles[0].final_count - handles[0].baseline, count);
	zassert_true(handles[1].final_count > handles[1].baseline);
}

ZTEST(perf_arm64, test_lookup_validation)
{
	struct perf_event_handle event = {
		.token = UINT64_MAX,
		.baseline = UINT64_MAX,
		.final_count = UINT64_MAX,
		.status = -1,
	};

	zassert_ok(perf_event_lookup("cpu0.CPU_CYCLES", &event));
	zassert_not_equal(event.token, 0U);
	zassert_equal(event.baseline, 0U);
	zassert_equal(event.final_count, 0U);
	zassert_ok(event.status);
	zassert_equal(perf_event_lookup("cpu0.unknown", &event), -ENOENT);
	zassert_equal(perf_event_lookup("unknown.CPU_CYCLES", &event), -ENOENT);
	zassert_equal(perf_event_lookup("CPU_CYCLES", &event), -EINVAL);
	zassert_equal(perf_event_lookup(NULL, &event), -EINVAL);
	zassert_equal(perf_event_lookup("cpu0.CPU_CYCLES", NULL), -EINVAL);
}

ZTEST(perf_arm64, test_duplicate_event)
{
	config.num_events = 2U;
	select_event(0U, "cpu0.CPU_CYCLES");
	handles[1] = handles[0];
	zassert_equal(perf_stat_start(&config), -EINVAL);
	recovery(0U);
}

ZTEST(perf_arm64, test_config_and_session_identity)
{
	struct perf_stat_config other_config;
	struct perf_event_handle invalid_event = {0};
	struct perf_stat_config invalid_config = {.events = &invalid_event, .num_events = 1U};

	zassert_equal(perf_stat_start(NULL), -EINVAL);
	zassert_equal(perf_stat_start(&invalid_config), -EINVAL);
	config.num_events = 1U;
	select_event(0U, "cpu0.CPU_CYCLES");
	other_config = config;
	zassert_ok(perf_stat_start(&config));
	zassert_equal(perf_stat_start(&config), -EBUSY);
	zassert_equal(perf_stat_stop(&other_config), -EINVAL);
	zassert_ok(perf_stat_stop(&config));
	zassert_equal(perf_stat_stop(&config), -EINVAL);
	recovery(0U);
}

ZTEST(perf_arm64, test_record_excludes_stat)
{
	int ret;

	config.num_events = 1U;
	select_event(0U, "cpu0.CPU_CYCLES");
	zassert_ok(z_perf_session_claim(PERF_SESSION_RECORD));
	ret = perf_stat_start(&config);
	z_perf_session_release(PERF_SESSION_RECORD);
	zassert_equal(ret, -EBUSY);
	recovery(0U);
}

ZTEST(perf_arm64, test_overflow)
{
	config.num_events = 1U;
	select_event(0U, "cpu0.SW_INCR");
	zassert_ok(perf_stat_start(&config));
	on_cpu(0U, overflow, NULL);
	zassert_ok(perf_stat_stop(&config));
	zassert_equal(handles[0].status, -EOVERFLOW);
	recovery(0U);
}

static void start_session(void *arg)
{
	*(int *)arg = perf_stat_start(&config);
}

static void stop_session(void *arg)
{
	*(int *)arg = perf_stat_stop(&config);
}

ZTEST(perf_arm64, test_cpu_isolation_and_caller_migration)
{
	uint32_t num_cpus = arch_num_cpus();
	int ret;

	zassert_true(num_cpus <= ARRAY_SIZE(handles));
	config.num_events = num_cpus;
	for (uint32_t cpu = 0U; cpu < num_cpus; cpu++) {
		char name[32];
		int length = snprintk(name, sizeof(name), "cpu%u.SW_INCR", cpu);

		zassert_true(length > 0 && (size_t)length < sizeof(name));
		select_event(cpu, name);
	}
	on_cpu(0U, start_session, &ret);
	zassert_ok(ret);
	for (uint32_t cpu = 0U; cpu < num_cpus; cpu++) {
		uint32_t count = 17U * (cpu + 1U);

		on_cpu(cpu, increment, &count);
	}
	on_cpu(num_cpus - 1U, stop_session, &ret);
	zassert_ok(ret);
	for (uint32_t cpu = 0U; cpu < num_cpus; cpu++) {
		zassert_ok(handles[cpu].status);
		zassert_equal(handles[cpu].final_count - handles[cpu].baseline, 17U * (cpu + 1U));
	}
}

static char event_names[32][48];
static size_t event_name_count;

static int save_event(const char *name, const char *description, void *user_data)
{
	ARG_UNUSED(description);
	ARG_UNUSED(user_data);
	zassert_true(event_name_count < ARRAY_SIZE(event_names));
	int ret = snprintk(event_names[event_name_count], sizeof(event_names[0]), "cpu0.%s", name);

	zassert_true(ret > 0 && (size_t)ret < sizeof(event_names[0]));
	event_name_count++;
	return 0;
}

static void get_capacity(void *arg)
{
	zassert_ok(pmu_init());
	*(uint32_t *)arg = MIN(pmu_num_counters(), CONFIG_PMU_MAX_COUNTERS);
}

struct event_catalog {
	struct pmu_event_info events[32];
	uint32_t count;
	uint32_t position;
	k_tid_t caller;
	bool stop_after_one;
};

static void read_catalog(void *arg)
{
	struct event_catalog *catalog = arg;

	catalog->count = pmu_num_events();
	zassert_true(catalog->count <= ARRAY_SIZE(catalog->events));
	for (uint32_t i = 0U; i < catalog->count; i++) {
		zassert_ok(pmu_event_get(i, &catalog->events[i]));
	}
}

static int compare_event(const char *name, const char *description, void *user_data)
{
	struct event_catalog *catalog = user_data;
	const char *expected;

	zassert_equal(k_current_get(), catalog->caller, "Callback changed execution context");
	zassert_true(catalog->position < catalog->count);
	expected = catalog->events[catalog->position++].name;
	zassert_str_equal(description, expected);
	zassert_str_equal(name, expected);
	return catalog->stop_after_one ? -EIO : 0;
}

ZTEST(perf_arm64, test_backend_catalog)
{
	struct event_catalog catalog = {.caller = k_current_get()};

	for (uint32_t cpu = 0U; cpu < arch_num_cpus(); cpu++) {
		char provider[16];
		int length = snprintk(provider, sizeof(provider), "cpu%u", cpu);

		zassert_true(length > 0 && (size_t)length < sizeof(provider));
		on_cpu(cpu, read_catalog, &catalog);
		catalog.position = 0U;
		zassert_ok(z_perf_provider_event_list(provider, compare_event, &catalog));
		zassert_equal(catalog.position, catalog.count);
		for (uint32_t i = 0U; i < catalog.count; i++) {
			const char *name = catalog.events[i].name;
			struct perf_event_handle event;
			char canonical[64];

			length = snprintk(canonical, sizeof(canonical), "%s.%s", provider, name);
			zassert_true(length > 0 && (size_t)length < sizeof(canonical));
			zassert_ok(perf_event_lookup(canonical, &event));
		}
	}
}

ZTEST(perf_arm64, test_list_callback_error)
{
	struct event_catalog catalog = {
		.caller = k_current_get(),
		.stop_after_one = true,
	};

	on_cpu(0U, read_catalog, &catalog);
	zassert_equal(z_perf_provider_event_list("cpu0", compare_event, &catalog), -EIO);
	zassert_equal(catalog.position, 1U);
	recovery(0U);
}

ZTEST(perf_arm64, test_discovery_during_session)
{
	struct event_catalog catalog = {.caller = k_current_get()};
	struct perf_event_handle event;
	uint32_t count = 19U;

	config.num_events = 1U;
	select_event(0U, "cpu0.SW_INCR");
	zassert_ok(perf_stat_start(&config));
	on_cpu(0U, increment, &count);
	on_cpu(0U, read_catalog, &catalog);
	zassert_ok(z_perf_provider_event_list("cpu0", compare_event, &catalog));
	zassert_equal(catalog.position, catalog.count);
	zassert_ok(perf_event_lookup("cpu0.CPU_CYCLES", &event));
	on_cpu(0U, increment, &count);
	zassert_ok(perf_stat_stop(&config));
	zassert_ok(handles[0].status);
	zassert_equal(handles[0].final_count - handles[0].baseline, 2U * count);
}

ZTEST(perf_arm64, test_capacity_cleanup)
{
	uint32_t capacity;
	size_t offset = IS_ENABLED(CONFIG_SMP) ? 1U : 0U;

	event_name_count = 0U;
	zassert_ok(z_perf_provider_event_list("cpu0", save_event, NULL));
	on_cpu(0U, get_capacity, &capacity);
	zassert_true(event_name_count > capacity);
	config.num_events = offset + capacity + 1U;
	zassert_true(config.num_events <= ARRAY_SIZE(handles));
	if (IS_ENABLED(CONFIG_SMP)) {
		/* Prepare another CPU before exhausting CPU0's counter slots. */
		select_event(0U, "cpu1.SW_INCR");
	}
	for (size_t i = 0U; i <= capacity; i++) {
		select_event(offset + i, event_names[i]);
	}
	zassert_equal(perf_stat_start(&config), -ENOSPC);
	on_cpu(0U, check_disabled, NULL);
	if (IS_ENABLED(CONFIG_SMP)) {
		on_cpu(1U, check_disabled, NULL);
	}
	recovery(0U);
	if (IS_ENABLED(CONFIG_SMP)) {
		recovery(1U);
	}
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	/* Release an active session if an assertion interrupted the test. */
	z_perf_session_release(PERF_SESSION_RECORD);
	(void)perf_stat_stop(&config);
}

ZTEST_SUITE(perf_arm64, NULL, NULL, NULL, after, NULL);
