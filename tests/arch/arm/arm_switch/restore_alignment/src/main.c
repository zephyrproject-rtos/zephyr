/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/ztest.h>

#define REGISTER_SENTINEL 0x12345678U
#define STACK_SENTINEL    0x89abcdefU
#define FP_SENTINEL       0x3f800000U

static K_KERNEL_STACK_DEFINE(interrupt_stack, 2048);
static uint32_t __aligned(1024) vectors[256];
static uint32_t saved_vtor;
static uint32_t saved_irq_priority;
static IRQn_Type probe_irq;
static bool probe_installed;
static void *saved_handle;
static uint32_t seen_alignments;
static struct k_thread dummy_thread;
static struct k_thread *saved_ready_cache;
static volatile bool frame_observed;
static volatile uint32_t frame_sp;
static volatile uint32_t frame_xpsr;

/* Observations made by the assembly helper, before returning to C. */
uint32_t test_save_sp;
uint32_t test_call_sp;
uint32_t test_restored_sp;
uint32_t test_restored_reg;
uint32_t test_restored_canary;
uint32_t test_restored_fp;
uint32_t test_restored_control;
static bool restore_pending;

void test_switch(void *next, void **last, uint32_t offset, uint32_t fp_active);
void test_svc(void);
void *__real_z_get_next_switch_handle(void *interrupted);

/* This leaf helper has no C callees or doubleword-aligned locals. The assembly
 * caller invokes it at both word alignments; only this low-level helper runs
 * with the deliberately offset stack, never the scheduler or test framework.
 */
__noinline void test_save_context(void *next, void **last)
{
	test_save_sp = __get_PSP();
#ifdef CONFIG_ARM_FP_CLOBBER_WORKAROUND
	/* arm_m_switch() saves r7 before constructing the switch frame. */
	test_save_sp -= sizeof(uint32_t);
#endif
	compiler_barrier();
	arm_m_switch(next, last);
	compiler_barrier();
}

/* Make the SVC enter the switching path. The dummy thread is never run:
 * the wrapped scheduler supplies our cooperatively saved handle instead.
 */
void test_prepare_restore(void)
{
	__set_BASEPRI(_EXC_IRQ_DEFAULT_PRIO);
	saved_ready_cache = _kernel.ready_q.cache;
	_kernel.ready_q.cache = &dummy_thread;
	restore_pending = true;
}

/* Replace only the scheduling decision for our SVC. Both the conversion and
 * exception return execute the production context-switch implementation.
 */
void *__wrap_z_get_next_switch_handle(void *interrupted)
{
	if (!restore_pending) {
		return __real_z_get_next_switch_handle(interrupted);
	}
	restore_pending = false;
	_kernel.ready_q.cache = saved_ready_cache;
	/* Lower priority than SVC: this interrupt tail-chains after the frame
	 * conversion, observing the synthesized frame before it is unstacked.
	 */
	NVIC_SetPendingIRQ(probe_irq);
	return saved_handle;
}

static void probe_exception_frame(void)
{
	uint32_t *frame = (uint32_t *)__get_PSP();

	frame_sp = (uint32_t)frame;
	frame_xpsr = frame[7];
	frame_observed = true;
}

static void interrupt_context(void *a, void *b, void *c, void *d)
{
	__asm__ volatile("svc #0" ::: "memory");
	zassert_unreachable("SVC did not restore the saved context");
}

static void check_restore(bool fp_active)
{
	seen_alignments = 0U;

	for (uint32_t offset = 0U; offset <= 4U; offset += 4U) {
		void *next = arm_m_new_stack(K_KERNEL_STACK_BUFFER(interrupt_stack),
					     K_KERNEL_STACK_SIZEOF(interrupt_stack),
					     interrupt_context, NULL, NULL, NULL, NULL);
		void *old_handle = _current->switch_handle;

		zassert_not_null(next);
		frame_observed = false;
		k_sched_lock();
		test_switch(next, &saved_handle, offset, fp_active);
		_current->switch_handle = old_handle;
		k_sched_unlock();

		bool padded = (test_save_sp & 4U) != 0U;
		/* Basic frame: 8 words; extended frame: another 18 words. */
		uint32_t frame_size = (8U + (fp_active ? 18U : 0U)) * sizeof(uint32_t);

		seen_alignments |= BIT(padded ? 1 : 0);
		zassert_true(frame_observed, "probe interrupt did not run");
		zassert_equal(frame_sp & 7U, 0U, "exception frame is not 8-byte aligned");
		zassert_equal((frame_xpsr & BIT(9)) != 0U, padded, "incorrect SPREALIGN");
		zassert_equal(frame_sp + frame_size + (padded ? sizeof(uint32_t) : 0U),
			      test_save_sp, "exception frame has the wrong size");
		zassert_equal(test_restored_sp, test_call_sp, "SP changed across context switch");
		zassert_equal(test_restored_reg, REGISTER_SENTINEL, "register corrupted");
		zassert_equal(test_restored_canary, STACK_SENTINEL, "stack corrupted");
#ifdef CONFIG_FPU
		zassert_equal((test_restored_control & CONTROL_FPCA_Msk) != 0U, fp_active,
			      "incorrect restored FPCA");
		if (fp_active) {
			zassert_equal(test_restored_fp, FP_SENTINEL, "FP register corrupted");
		}
#endif
	}
	zassert_equal(seen_alignments, BIT(0) | BIT(1), "both alignments must be exercised");
}

/** @brief Restore aligned and misaligned cooperative contexts without FP state. */
ZTEST(arm_switch_restore_alignment, test_integer_context)
{
	check_restore(false);
}

/** @brief Restore aligned and misaligned cooperative contexts with FP state. */
ZTEST(arm_switch_restore_alignment, test_fpu_context)
{
	if (!IS_ENABLED(CONFIG_FPU)) {
		ztest_test_skip();
		return;
	}
	check_restore(true);
}

static void before(void *fixture)
{
	size_t vector_size = _vector_end - _vector_start;

	BUILD_ASSERT(offsetof(struct arm_m_cs_ptrs, lr_save) == 8U);
	probe_installed = false;
	zassert_true(vector_size <= sizeof(vectors));
	probe_irq = (IRQn_Type)-1;
	for (uint32_t irq = 0U; irq + 16U < vector_size / sizeof(vectors[0]); irq++) {
		if (NVIC_GetEnableIRQ((IRQn_Type)irq) == 0U &&
		    NVIC_GetPendingIRQ((IRQn_Type)irq) == 0U) {
			probe_irq = (IRQn_Type)irq;
			break;
		}
	}
	zassert_not_equal(probe_irq, (IRQn_Type)-1, "no unused IRQ for the frame probe");
	saved_vtor = SCB->VTOR;
	saved_irq_priority = NVIC_GetPriority(probe_irq);
	probe_installed = true;
	NVIC_SetPriority(probe_irq, BIT(__NVIC_PRIO_BITS) - 1U);
	zassert_true(NVIC_GetPriority(probe_irq) > NVIC_GetPriority(SVCall_IRQn),
		     "probe must tail-chain after SVC");
	memcpy(vectors, (void *)saved_vtor, vector_size);
	vectors[11] = (uint32_t)test_svc;
	vectors[16 + probe_irq] = (uint32_t)probe_exception_frame;
	SCB->VTOR = (uint32_t)vectors;
	__DSB();
	__ISB();
	NVIC_EnableIRQ(probe_irq);
}

static void after(void *fixture)
{
	if (!probe_installed) {
		return;
	}
	NVIC_DisableIRQ(probe_irq);
	NVIC_ClearPendingIRQ(probe_irq);
	NVIC_SetPriority(probe_irq, saved_irq_priority);
	SCB->VTOR = saved_vtor;
	__DSB();
	__ISB();
}

ZTEST_SUITE(arm_switch_restore_alignment, NULL, NULL, before, after, NULL);
