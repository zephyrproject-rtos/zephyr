/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/ztest.h>
#include "power_mgmt.h"

#define CYCLES 10U
#define PATTERN 0x5a5aa5a5U
#define WORDS 16U

BUILD_ASSERT(IS_ENABLED(CONFIG_PM_S2RAM), "The scenario requires an enabled S2RAM state");

static const struct pm_state_info states[] =
	PM_STATE_INFO_LIST_FROM_DT_CPU(DT_NODELABEL(cpu0));
static volatile uint32_t data_pattern = PATTERN;
static volatile uint32_t bss_pattern;
static volatile unsigned int suspend_calls;
static volatile int suspend_result;
static unsigned int worker_rounds;
static unsigned int worker_failures;

K_SEM_DEFINE(worker_resume, 0, 1);
K_SEM_DEFINE(worker_checked, 0, 1);
K_THREAD_STACK_DEFINE(worker_stack, 1024);
static struct k_thread worker_thread;

int __real_arch_pm_s2ram_suspend(pm_s2ram_system_off_fn_t system_off);

int __wrap_arch_pm_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	suspend_calls++;
	suspend_result = __real_arch_pm_s2ram_suspend(system_off);
	return suspend_result;
}

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);

	/* Only the forced transition may suspend, including during console recovery. */
	return NULL;
}

static void retained_worker(void *p1, void *p2, void *p3)
{
	volatile uint32_t stack_pattern[WORDS];

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (size_t i = 0; i < WORDS; i++) {
		stack_pattern[i] = PATTERN ^ i;
	}
	k_sem_give(&worker_checked);

	while (true) {
		k_sem_take(&worker_resume, K_FOREVER);
		for (size_t i = 0; i < WORDS; i++) {
			if (stack_pattern[i] != (PATTERN ^ i)) {
				worker_failures++;
			}
		}
		worker_rounds++;
		k_sem_give(&worker_checked);
	}
}

ZTEST(power_mgmt, test_pm_multithread)
{
	const struct pm_state_info *state = NULL;
	volatile uint32_t stack_pattern[WORDS];
	uint32_t *heap = k_malloc(WORDS * sizeof(*heap));
	volatile uint32_t *heap_pattern = heap;

	zassert_not_null(heap);
	zassert_equal(data_pattern, PATTERN);
	zassert_equal(bss_pattern, 0);
	for (size_t i = 0; i < ARRAY_SIZE(states); i++) {
		if (states[i].state == PM_STATE_SUSPEND_TO_RAM) {
			state = &states[i];
			break;
		}
	}
	zassert_not_null(state, "No S2RAM state in cpu-power-states");

	k_thread_create(&worker_thread, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
			retained_worker, NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&worker_checked, K_SECONDS(1)), "Worker did not start");

	for (unsigned int cycle = 1; cycle <= CYCLES; cycle++) {
		uint32_t salt = PATTERN ^ cycle;
		unsigned int old_calls = suspend_calls;
		int64_t before;
		int64_t elapsed;

		data_pattern = salt;
		bss_pattern = ~salt;
		for (size_t i = 0; i < WORDS; i++) {
			stack_pattern[i] = salt ^ i;
			heap_pattern[i] = ~salt ^ i;
		}
		test_pm_s2ram_prepare();
		suspend_result = -EINPROGRESS;
		zassert_true(pm_state_force(0, state), "Could not force S2RAM");
		before = k_uptime_get();
		k_msleep(500);
		elapsed = k_uptime_get() - before;
		if (suspend_result == 0) {
			test_pm_s2ram_resume();
		}

		zassert_equal(suspend_calls, old_calls + 1, "Missing or repeated S2RAM entry");
		zassert_equal(suspend_result, 0, "S2RAM did not resume: %d", suspend_result);
		zassert_true(elapsed >= 500, "Sleep timeout fired early");
		zassert_equal(data_pattern, salt);
		zassert_equal(bss_pattern, ~salt);
		for (size_t i = 0; i < WORDS; i++) {
			zassert_equal(stack_pattern[i], salt ^ i, "Main stack was not retained");
			zassert_equal(heap_pattern[i], ~salt ^ i, "Heap was not retained");
		}
		k_sem_give(&worker_resume);
		zassert_ok(k_sem_take(&worker_checked, K_SECONDS(1)), "Worker did not resume");
		zassert_equal(worker_failures, 0, "Worker stack was not retained");
		zassert_equal(worker_rounds, cycle, "Worker progress was not retained");
		printk("PM S2RAM cycle %u PASS\n", cycle);
	}

	k_thread_abort(&worker_thread);
	k_free(heap);
}

ZTEST_SUITE(power_mgmt, NULL, NULL, NULL, NULL, NULL);
