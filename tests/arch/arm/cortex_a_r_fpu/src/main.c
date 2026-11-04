/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */

#include <zephyr/ztest.h>
#include <cmsis_core.h>
#include <zephyr/sys/atomic.h>

#include "fp_context.h"

#define FPSCR_ROUND_UP (1U << 22)
#define FPSCR_ROUND_DOWN (2U << 22)
#define FPSCR_ROUND_MASK (3U << 22)
#define PREEMPTION_ROUNDS 2U
#define WORKER_STACK_SIZE 4096

static struct fp_context main_input;
static struct fp_context main_output;
static struct fp_context peer_input;
static struct fp_context peer_output;
static struct k_thread worker;
static K_THREAD_STACK_DEFINE(worker_stack, WORKER_STACK_SIZE);
static atomic_t preemptions;

static void initial_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	fp_capture_callee(p1);
}

ZTEST(cortex_a_r_fpu, test_new_thread_initializes_callee_registers)
{
	uint64_t registers[8];
	const uint64_t zero_registers[8] = {0};

	/* Seed a reused TCB with state belonging to its previous FP owner. */
	memset(&worker.arch.preempt_float, 0xa5,
	       sizeof(worker.arch.preempt_float));
	memset(registers, 0xff, sizeof(registers));
	k_thread_create(&worker, worker_stack,
			K_THREAD_STACK_SIZEOF(worker_stack), initial_worker,
			registers, NULL, NULL, K_PRIO_PREEMPT(1), K_FP_REGS,
			K_NO_WAIT);
	zassert_ok(k_thread_join(&worker, K_SECONDS(1)));
	zassert_mem_equal(registers, zero_registers, sizeof(registers));
}

static void init_contexts(void)
{
	for (size_t i = 0; i < FP_DOUBLE_REGISTERS; ++i) {
		main_input.registers[i] = UINT64_C(0x1111222233334400) + i;
		peer_input.registers[i] = UINT64_C(0x5555666677778800) + i;
	}
	main_input.fpscr = FPSCR_ROUND_UP;
	peer_input.fpscr = FPSCR_ROUND_DOWN;
	memset(&main_output, 0, sizeof(main_output));
	memset(&peer_output, 0, sizeof(peer_output));
}

static void yield_callback(void)
{
	k_yield();
}

static void cooperative_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	fp_context_call(&peer_input, &peer_output, yield_callback);
}

ZTEST(cortex_a_r_fpu, test_cooperative_callee_and_fpscr)
{
	int priority = k_thread_priority_get(k_current_get());

	init_contexts();
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(1));
	k_thread_create(&worker, worker_stack,
			K_THREAD_STACK_SIZEOF(worker_stack),
			cooperative_worker, NULL, NULL, NULL,
			K_PRIO_COOP(1), K_FP_REGS,
			K_NO_WAIT);
	fp_context_call(&main_input, &main_output, yield_callback);
	zassert_ok(k_thread_join(&worker, K_SECONDS(1)));
	k_thread_priority_set(k_current_get(), priority);
	/* d8-d15 survive ordinary function calls. */
	zassert_mem_equal(&main_input.registers[8], &main_output.registers[8],
			 8U * sizeof(uint64_t));
	zassert_mem_equal(&peer_input.registers[8], &peer_output.registers[8],
			 8U * sizeof(uint64_t));
	zassert_equal(main_output.fpscr & FPSCR_ROUND_MASK, FPSCR_ROUND_UP);
	zassert_equal(peer_output.fpscr & FPSCR_ROUND_MASK, FPSCR_ROUND_DOWN);
}

static void sleeper_callback(void)
{
	atomic_inc(&preemptions);
	k_sleep(K_MSEC(2));
}

static void preempting_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	k_sleep(K_MSEC(2));
	for (uint32_t i = 0; i < PREEMPTION_ROUNDS; ++i) {
		fp_context_call(&peer_input, &peer_output, sleeper_callback);
	}
}

static void wait_callback(void)
{
	/* No FP operations or function calls while registers are live. */
	while (atomic_get(&preemptions) < PREEMPTION_ROUNDS) {
		__asm__ volatile ("nop" ::: "memory");
	}
}

ZTEST(cortex_a_r_fpu, test_timer_preemption_preserves_all_registers)
{
	int priority = k_thread_priority_get(k_current_get());

	init_contexts();
	atomic_set(&preemptions, 0);
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(2));
	k_thread_create(&worker, worker_stack,
			K_THREAD_STACK_SIZEOF(worker_stack),
			preempting_worker, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_FP_REGS, K_NO_WAIT);
	fp_context_call(&main_input, &main_output, wait_callback);
	zassert_ok(k_thread_join(&worker, K_SECONDS(1)));
	k_thread_priority_set(k_current_get(), priority);
	zassert_equal(atomic_get(&preemptions), PREEMPTION_ROUNDS);
	zassert_mem_equal(main_input.registers, main_output.registers,
			 sizeof(main_input.registers));
	zassert_equal(main_output.fpscr, main_input.fpscr);
}

ZTEST(cortex_a_r_fpu, test_disable_and_automatic_reenable)
{
	uint32_t status;
	int priority = k_thread_priority_get(k_current_get());

	init_contexts();
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(1));
	k_thread_create(&worker, worker_stack,
			K_THREAD_STACK_SIZEOF(worker_stack),
			cooperative_worker, NULL, NULL, NULL,
			K_PRIO_COOP(1), K_FP_REGS,
			K_NO_WAIT);
	zassert_equal(k_float_enable(k_current_get(), K_FP_REGS), -ENOTSUP);
	zassert_ok(k_float_disable(k_current_get()));
	/* Read FPEXC before an FP instruction can trap and enable it. */
	__asm__ volatile ("vmrs %0, fpexc" : "=r" (status));
	zassert_equal(status & FPEXC_EN, 0U);
	k_yield();
	__asm__ volatile ("vmrs %0, fpexc" : "=r" (status));
	zassert_equal(status & FPEXC_EN, 0U);
	/* The undefined-instruction handler retries this first FP operation. */
	__asm__ volatile ("vmov.f32 s0, #1.0" ::: "s0");
	__asm__ volatile ("vmrs %0, fpexc" : "=r" (status));
	zassert_not_equal(status & FPEXC_EN, 0U);
	zassert_not_equal(k_current_get()->base.user_options & K_FP_REGS, 0U);
	zassert_ok(k_thread_join(&worker, K_SECONDS(1)));
	k_thread_priority_set(k_current_get(), priority);
}

ZTEST_SUITE(cortex_a_r_fpu, NULL, NULL, NULL, NULL, NULL);
