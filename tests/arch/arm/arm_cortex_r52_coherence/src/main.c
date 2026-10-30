/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Andrei-Edward Popa
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <cmsis_core.h>

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define SCTLR_ENABLED (SCTLR_M_Msk | SCTLR_C_Msk | SCTLR_I_Msk)
#define PATTERN 0x12345678U
#define RESUME_COUNT 64U
#define STACK_WORDS 64U

#ifdef CONFIG_NOCACHE_MEMORY
static uint32_t noncache_data __nocache;
#endif

static volatile uint32_t cached_data __incoherent = PATTERN;
static volatile uint32_t shared_data = PATTERN;
static uint32_t shared_noinit __noinit;
static K_SEM_DEFINE(shared_sem, 0, 1);
static uint32_t shared_bss;

static K_THREAD_STACK_DEFINE(worker_stack, 2048);
static K_SEM_DEFINE(worker_continue, 0, 1);
static K_SEM_DEFINE(worker_ready, 0, 1);
static struct k_thread worker_thread;
static uint32_t expected_cpu;

ZTEST(cortex_r52_coherence, test_memory_placement)
{
	void *allocation = k_malloc(128);

	zassert_not_null(allocation);

	zassert_equal(shared_data, PATTERN);
	zassert_equal(cached_data, PATTERN);

	zassert_true(sys_cache_is_mem_coherent((void *)&shared_data));
	zassert_true(sys_cache_is_mem_coherent(&shared_bss));
	zassert_true(sys_cache_is_mem_coherent(&shared_noinit));
	zassert_true(sys_cache_is_mem_coherent(&shared_sem));
	zassert_true(sys_cache_is_mem_coherent(allocation));
	zassert_false(sys_cache_is_mem_coherent((void *)&cached_data));
#ifdef CONFIG_NOCACHE_MEMORY
	zassert_true(sys_cache_is_mem_coherent(&noncache_data));
	zassert_equal(noncache_data, 0);
#endif

	k_free(allocation);
}

static void stack_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static volatile uint32_t Z_THREAD_LOCAL tls_word = PATTERN;
	volatile uint32_t words[STACK_WORDS];

	for (uint32_t i = 0; i < STACK_WORDS; i++) {
		words[i] = PATTERN ^ i;
	}

	for (uint32_t step = 0; step <= RESUME_COUNT; step++) {
		zassert_equal(arch_curr_cpu()->id, expected_cpu, "Wrong CPU at resume %u", step);
		zassert_equal(__get_SCTLR() & SCTLR_ENABLED, SCTLR_ENABLED,
			      "MPU or caches disabled");
		zassert_equal(tls_word, PATTERN ^ step, "TLS lost at resume %u", step);
		tls_word = PATTERN ^ (step + 1U);

		for (uint32_t i = 0; i < STACK_WORDS; i++) {
			zassert_equal(words[i], PATTERN ^ i ^ step,
				      "Stack word %u lost at resume %u", i, step);
			words[i] = PATTERN ^ i ^ (step + 1U);
		}

		if (step < RESUME_COUNT) {
			k_sem_give(&worker_ready);
			zassert_ok(k_sem_take(&worker_continue, K_SECONDS(2)));
		}
	}
}

static void run_stack_test(bool alternate_cpus)
{
	k_sem_reset(&worker_ready);
	k_sem_reset(&worker_continue);
	expected_cpu = 0U;

	zassert_false(sys_cache_is_mem_coherent(worker_stack));
	zassert_true(sys_cache_is_mem_coherent(&worker_thread));

	k_thread_create(&worker_thread, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
			stack_worker, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(&worker_thread, expected_cpu));
	k_thread_start(&worker_thread);

	for (uint32_t step = 0; step < RESUME_COUNT; step++) {
		zassert_ok(k_sem_take(&worker_ready, K_SECONDS(2)));
		k_thread_suspend(&worker_thread);

		expected_cpu = alternate_cpus ? ((step + 1U) % 2U) : 0U;
		zassert_ok(k_thread_cpu_pin(&worker_thread, expected_cpu));

		k_thread_resume(&worker_thread);
		k_sem_give(&worker_continue);
	}

	zassert_ok(k_thread_join(&worker_thread, K_SECONDS(2)));
}

ZTEST(cortex_r52_coherence, test_stack_migration)
{
	for (uint32_t round = 0; round < 2U; round++) {
		run_stack_test(true);
	}
}

ZTEST(cortex_r52_coherence, test_same_cpu_resume)
{
	run_stack_test(false);
}

ZTEST_SUITE(cortex_r52_coherence, NULL, NULL, NULL, NULL, NULL);
