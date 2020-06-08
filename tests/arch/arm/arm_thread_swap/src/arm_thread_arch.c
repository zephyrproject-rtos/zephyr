/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <kernel_structs.h>
#include <offsets_short_arch.h>
#include <ksched.h>

#if !defined(__GNUC__)
#error __FILE__ goes only with Cortex-M GCC
#endif

#if !defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && \
	!defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
#error "Unsupported architecture"
#endif

#define PRIORITY 0
#define BASEPRI_MODIFIED_1 0x20
#define BASEPRI_MODIFIED_2 0x40
#define SWAP_RETVAL        0x1234

extern void z_move_thread_to_end_of_prio_q(struct k_thread *thread);

static struct k_thread alt_thread;
static K_THREAD_STACK_DEFINE(alt_thread_stack, 1024);

/* Status variable to indicate that context-switch has occurred. */
bool volatile switch_flag;

struct k_thread *p_ztest_thread;

_callee_saved_t ztest_thread_callee_saved_regs_container;
int ztest_swap_return_val;

/* Arbitrary values for the callee-saved registers,
 * enforced in the beginning of the test.
 */
const _callee_saved_t ztest_thread_callee_saved_regs_init = {
	.v1 = 0x12345678, .v2 = 0x23456789, .v3 = 0x3456789a, .v4 = 0x456789ab,
	.v5 = 0x56789abc, .v6 = 0x6789abcd, .v7 = 0x789abcde, .v8 = 0x89abcdef
};

static void load_callee_saved_regs(const _callee_saved_t *regs)
{
	/* Load the callee-saved registers with given values */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	__asm__ volatile (
		"mov r1, r7;\n\t"
		"mov r0, %0;\n\t"
		"ldmia r0!, {r4-r7};\n\t"
		"ldmia r0!, {r4-r7};\n\t"
		"mov r8, r4;\n\t"
		"mov r9, r5;\n\t"
		"mov r10, r6;\n\t"
		"mov r11, r7;\n\t"
		"mov r7, r1;\n\t"
		:
		: "r" (regs)
		: "memory"
	);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile (
		"mov r1, r7;\n\t"
		"ldmia %0, {v1-v8};\n\t"
		"mov r7, r1;\n\t"
		:
		: "r" (regs)
		: "memory"
	);
#endif
	__DSB();
}

static void verify_callee_saved(const _callee_saved_t *src,
		const _callee_saved_t *dst)
{
	/* Verify callee-saved registers are as expected */
	zassert_true((src->v1 == dst->v1)
			&& (src->v2 == dst->v2)
			&& (src->v3 == dst->v3)
			&& (src->v4 == dst->v4)
			&& (src->v5 == dst->v5)
			&& (src->v6 == dst->v6)
			&& (src->v7 == dst->v7)
			&& (src->v8 == dst->v8),
		" got: 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x\n"
		" expected:  0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x\n",
		src->v1,
		src->v2,
		src->v3,
		src->v4,
		src->v5,
		src->v6,
		src->v7,
		src->v8,
		dst->v1,
		dst->v2,
		dst->v3,
		dst->v4,
		dst->v5,
		dst->v6,
		dst->v7,
		dst->v8
	);
}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
/* Arbitrary values for the floating-point callee-saved registers */
struct _preempt_float ztest_thread_fp_callee_saved_regs = {
	.s16 = 0x11111111, .s17 = 0x22222222,
	.s18 = 0x33333333, .s19 = 0x44444444,
	.s20 = 0x55555555, .s21 = 0x66666666,
	.s22 = 0x77777777, .s23 = 0x88888888,
	.s24 = 0x99999999, .s25 = 0xaaaaaaaa,
	.s26 = 0xbbbbbbbb, .s27 = 0xcccccccc,
	.s28 = 0xdddddddd, .s29 = 0xeeeeeeee,
	.s30 = 0xffffffff, .s31 = 0x00000000,
};

static void load_fp_callee_saved_regs(
	const volatile struct _preempt_float *regs)
{
	__asm__ volatile (
		"vldmia %0, {s16-s31};\n\t"
		:
		: "r" (regs)
		: "memory"
		);
	__DSB();
}

static void verify_fp_callee_saved(const struct _preempt_float *src,
		const struct _preempt_float *dst)
{
	/* Verify FP callee-saved registers are as expected */
	zassert_true((src->s16 == dst->s16)
			&& (src->s17 == dst->s17)
			&& (src->s18 == dst->s18)
			&& (src->s19 == dst->s19)
			&& (src->s20 == dst->s20)
			&& (src->s21 == dst->s21)
			&& (src->s22 == dst->s22)
			&& (src->s23 == dst->s23)
			&& (src->s24 == dst->s24)
			&& (src->s25 == dst->s25)
			&& (src->s26 == dst->s26)
			&& (src->s27 == dst->s27)
			&& (src->s28 == dst->s28)
			&& (src->s29 == dst->s29)
			&& (src->s30 == dst->s30)
			&& (src->s31 == dst->s31),
		" got: 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x"
		" 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x\n"
		" expected:  0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x"
		" 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x 0x%0x\n",
		src->s16,
		src->s17,
		src->s18,
		src->s19,
		src->s20,
		src->s21,
		src->s22,
		src->s23,
		src->s24,
		src->s25,
		src->s26,
		src->s27,
		src->s28,
		src->s29,
		src->s30,
		src->s31,
		dst->s16,
		dst->s17,
		dst->s18,
		dst->s19,
		dst->s20,
		dst->s21,
		dst->s22,
		dst->s23,
		dst->s24,
		dst->s25,
		dst->s26,
		dst->s27,
		dst->s28,
		dst->s29,
		dst->s30,
		dst->s31
	);
}

#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

static void alt_thread_entry(void)
{
	int init_flag, post_flag;

	/* Lock interrupts to make sure we get preempted only when
	 * it is required by the test */
	(void)irq_lock();

	init_flag = switch_flag;
	zassert_true(init_flag == false,
		"Alternative thread: switch flag not false on thread entry\n");

	/* Set switch flag */
	switch_flag = true;

#if defined(CONFIG_NO_OPTIMIZATIONS)
	zassert_true(p_ztest_thread->arch.basepri == 0,
		"ztest thread basepri not preserved in swap-out\n");
#else
	/* Verify that the main test thread has the correct value
	 * for state variable thread.arch.basepri (set before swap).
	 */
	zassert_true(p_ztest_thread->arch.basepri == BASEPRI_MODIFIED_1,
		"ztest thread basepri not preserved in swap-out\n");

	/* Verify original swap return value (set by arch_swap() */
	zassert_true(p_ztest_thread->arch.swap_return_value == -EAGAIN,
		"ztest thread swap-return-value not preserved in swap-out\n");
#endif

	/* Verify that the main test thread (ztest) has stored the callee-saved
	 * registers properly in its corresponding callee-saved container.
	 */
	verify_callee_saved(
		(const _callee_saved_t *)&p_ztest_thread->callee_saved,
		&ztest_thread_callee_saved_regs_container);

	/* Zero the container of the callee-saved registers, to validate,
	 * later, that it is populated properly.
	 */
	memset(&ztest_thread_callee_saved_regs_container,
		0, sizeof(_callee_saved_t));

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)

	/*  Verify that the _current_ (alt) thread is initialized with FPCA cleared. */
	zassert_true((__get_CONTROL() & CONTROL_FPCA_Msk) == 0,
		"CONTROL.FPCA is not cleared at initialization: 0x%x\n",
		__get_CONTROL());

	/* Verify that the _current_ (alt) thread is initialized with FPSCR cleared. */
	zassert_true(__get_FPSCR() == 0,
		"(Alt thread) FPSCR is not cleared at initialization: 0x%x\n", __get_FPSCR());

	zassert_true((p_ztest_thread->arch.mode & CONTROL_FPCA_Msk) != 0,
		"ztest thread mode FPCA flag not updated at swap-out: 0x%0x\n",
		p_ztest_thread->arch.mode);

	/* Verify that the main test thread (ztest) has stored the FP
	 *  callee-saved registers properly in its corresponding FP
	 *  callee-saved container.
	 */
	verify_fp_callee_saved((const struct _preempt_float *)
		&p_ztest_thread->arch.preempt_float,
		&ztest_thread_fp_callee_saved_regs);

	/* Zero the container of the FP callee-saved registers, to validate,
	 * later, that it is populated properly.
	 */
	memset(&ztest_thread_fp_callee_saved_regs,
		0, sizeof(ztest_thread_fp_callee_saved_regs));

#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Modify the arch.basepri flag of the main test thread, to verify,
	 * later, that this is passed properly to the BASEPRI.
	 */
	p_ztest_thread->arch.basepri = BASEPRI_MODIFIED_2;

#if !defined(CONFIG_NO_OPTIMIZATIONS)
	/* Modify the arch.swap_return_value flag of the main test thread,
	 * to verify later, that this value is properly returned by swap.
	 */
	p_ztest_thread->arch.swap_return_value = SWAP_RETVAL;
#endif

	z_move_thread_to_end_of_prio_q(_current);

	/* Modify the callee-saved registers by zero-ing them.
	 * The main test thread will, later, assert that they
	 * are restored to their original values upon context
	 * switch.
	 *
	 * Note: preserve r7 register (frame pointer).
	 */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	__asm__ volatile (
		"mov r1, r7;\n\t"
		"mov r0, %0;\n\t"
		"ldmia r0!, {r4-r7};\n\t"
		"ldmia r0!, {r4-r7};\n\t"
		"mov r8, r4;\n\t"
		"mov r9, r5;\n\t"
		"mov r10, r6;\n\t"
		"mov r11, r7;\n\t"
		"mov r7, r1;\n\t"
		:	: "r" (&ztest_thread_callee_saved_regs_container)
		: "memory"
	);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile (
		"mov r0, r7;\n\t"
		"ldmia %0, {v1-v8};\n\t"
		"mov r7, r0;\n\t"
		: : "r" (&ztest_thread_callee_saved_regs_container)
		: "memory"
	);
#endif

	/* Manually trigger a context-switch, to swap-out
	 * the alternative test thread.
	 */
	__DMB();
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	irq_unlock(0);

	/* Verify that the main test thread has managed to resume, before
	 * we return to the alternative thread (we verify this by checking
	 * the status of the switch flag; the main test thread will clear
	 * it when it is swapped-back in.
	 */
	post_flag = switch_flag;
	zassert_true(post_flag == false,
		"Alternative thread: switch flag not false on thread exit\n");
}

void test_arm_thread_swap(void)
{
	int test_flag;

	/* Main test thread (ztest)
	 *
	 * Simulating initial conditions:
	 * - set arbitrary values at the callee-saved registers
	 * - set arbitrary values at the FP callee-saved registers,
	 *   if building with CONFIG_FPU/CONFIG_FPU_SHARING
	 * - zero the thread's callee-saved data structure
	 * - set thread's priority same as the alternative test thread
	 */

	/* Load the callee-saved registers with initial arbitrary values
	 * (from ztest_thread_callee_saved_regs_init)
	 */
	load_callee_saved_regs(&ztest_thread_callee_saved_regs_init);

	k_thread_priority_set(_current, K_PRIO_COOP(PRIORITY));

	/* Export current thread's callee-saved registers pointer
	 * and arch.basepri variable pointer, into global pointer
	 * variables, so they can be easily accessible by other
	 * (alternative) test thread.
	 */
	p_ztest_thread = _current;

	/* Confirm initial conditions before starting the test. */
	test_flag = switch_flag;
	zassert_true(test_flag == false,
		"Switch flag not initialized properly\n");
	zassert_true(_current->arch.basepri == 0,
		"Thread BASEPRI flag not clear at thread start\n");
	/* Verify, also, that the interrupts are unlocked. */
#if defined(CONFIG_CPU_CORTEX_M_HAS_BASEPRI)
	zassert_true(__get_BASEPRI() == 0,
		"initial BASEPRI not zero\n");
#else
	/* For Cortex-M Baseline architecture, we verify that
	 * the interrupt lock is disabled.
	 */
	 zassert_true(__get_PRIMASK() == 0,
	 "initial PRIMASK not zero\n");
#endif /* CONFIG_CPU_CORTEX_M_HAS_BASEPRI */

#if defined(CONFIG_USERSPACE)
	/* The main test thread is set to run in privilege mode */
	zassert_false((arch_is_user_context()),
		"Main test thread does not start in privilege mode\n");

	/* Assert that the mode status variable indicates privilege mode */
	zassert_true((_current->arch.mode & CONTROL_nPRIV_Msk) == 0,
		"Thread nPRIV flag not clear for supervisor thread: 0x%0x\n",
		_current->arch.mode);
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* The main test thread is not (yet) actively using the FP registers */
	zassert_true((_current->arch.mode & CONTROL_FPCA_Msk) == 0,
		"Thread FPCA flag not clear at initialization 0x%0x\n",
		_current->arch.mode);

	/* Verify that the main test thread is initialized with FPCA cleared. */
	zassert_true((__get_CONTROL() & CONTROL_FPCA_Msk) == 0,
		"CONTROL.FPCA is not cleared at initialization: 0x%x\n",
		__get_CONTROL());
	/* Verify that the main test thread is initialized with FPSCR cleared. */
	zassert_true(__get_FPSCR() == 0,
		"FPSCR is not cleared at initialization: 0x%x\n", __get_FPSCR());

	/* Clear the thread's floating-point callee-saved registers' container.
	 * The container will, later, be populated by the swap mechanism.
	 */
	memcpy(&_current->arch.preempt_float, 0,
		sizeof(struct _preempt_float));

	/* Randomize the FP callee-saved registers at test initialization */
	load_fp_callee_saved_regs(&ztest_thread_fp_callee_saved_regs);

	/* Modify bit-0 of the FPSCR - will be checked again upon swap-in. */
	zassert_true((__get_FPSCR() & 0x1) == 0,
		"FPSCR bit-0 has been set before testing it\n");
	__set_FPSCR(__get_FPSCR() | 0x1);

	/* The main test thread is using the FP registers, but the .mode
	 * flag is not updated until the next context switch.
	 */
	zassert_true((_current->arch.mode & CONTROL_FPCA_Msk) == 0,
		"Thread FPCA flag not clear at initialization\n");
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Create an alternative (supervisor) testing thread */
	k_thread_create(&alt_thread,
		alt_thread_stack,
		K_THREAD_STACK_SIZEOF(alt_thread_stack),
		(k_thread_entry_t)alt_thread_entry,
		NULL, NULL, NULL,
		K_PRIO_COOP(PRIORITY), 0,
		K_NO_WAIT);

	/* Verify context-switch has not occurred. */
	test_flag = switch_flag;
	zassert_true(test_flag == false,
		"Switch flag incremented when it should not have\n");

	/* Prepare to force a context switch to the alternative thread,
	 * by manually adding the current thread to the end of the queue,
	 * so it will be context switched-out.
	 *
	 * Lock interrupts to make sure we get preempted only when it is
	 * explicitly required by the test.
	 */
	(void)irq_lock();
	z_move_thread_to_end_of_prio_q(_current);

	/* Clear the thread's callee-saved registers' container.
	 * The container will, later, be populated by the swap
	 * mechanism.
	 */
	memset(&_current->callee_saved, 0, sizeof(_callee_saved_t));

	/* Verify context-switch has not occurred yet. */
	test_flag = switch_flag;
	zassert_true(test_flag == false,
		"Switch flag incremented by unexpected context-switch.\n");

	/* Store the callee-saved registers to some global memory
	 * accessible to the alternative testing thread. That
	 * thread is going to verify that the callee-saved regs
	 * are successfully loaded into the thread's callee-saved
	 * registers' container.
	 */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	__asm__ volatile (
		"push {r0, r1, r2, r3};\n\t"
		"mov r1, %0;\n\t"
		"stmia r1!, {r4-r7};\n\t"
		"mov r2, r8;\n\t"
		"mov r3, r9;\n\t"
		"stmia r1!, {r2-r3};\n\t"
		"mov r2, r10;\n\t"
		"mov r3, r11;\n\t"
		"stmia r1!, {r2-r3};\n\t"
		"pop {r0, r1, r2, r3};\n\t"
		:	: "r" (&ztest_thread_callee_saved_regs_container)
		: "memory"
	);
 #elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile (
		"stmia %0, {v1-v8};\n\t"
		:
		: "r" (&ztest_thread_callee_saved_regs_container)
		: "memory"
	);
#endif

	/* Manually trigger a context-switch to swap-out the current thread.
	 * Request a return to a different interrupt lock state.
	 */
	__DMB();

#if defined(CONFIG_NO_OPTIMIZATIONS)
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	irq_unlock(0);
	/* The thread is now swapped-back in. */

#else/* CONFIG_NO_OPTIMIZATIONS */

	/* Fake a different irq_unlock key when performing swap.
	 * This will be verified by the alternative test thread.
	 */
	register int swap_return_val __asm__("r0") =
		arch_swap(BASEPRI_MODIFIED_1);

#endif /* CONFIG_NO_OPTIMIZATIONS */

	/* Dump callee-saved registers to memory. */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	__asm__ volatile (
		"push {r0, r1, r2, r3};\n\t"
		"mov r1, %0;\n\t"
		"stmia r1!, {r4-r7};\n\t"
		"mov r2, r8;\n\t"
		"mov r3, r9;\n\t"
		"stmia r1!, {r2-r3};\n\t"
		"mov r2, r10;\n\t"
		"mov r3, r11;\n\t"
		"stmia r1!, {r2-r3};\n\t"
		"pop {r0, r1, r2, r3};\n\t"
		:	: "r" (&ztest_thread_callee_saved_regs_container)
		: "memory"
	);
 #elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile (
		"stmia %0, {v1-v8};\n\t"
		:
		: "r" (&ztest_thread_callee_saved_regs_container)
		: "memory"
	);
#endif

#if !defined(CONFIG_NO_OPTIMIZATIONS)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* Note: ARMv6-M will always write back the base register,
	 * so we make sure we preserve the state of the register
	 * used, as base register in the Store Multiple instruction.
	 * We also enforce write-back to suppress assembler warning.
	 */
	__asm__ volatile (
		"push {r0, r1, r2, r3, r4, r5, r6, r7};\n\t"
		"stm %0!, {%1};\n\t"
		"pop {r0, r1, r2, r3, r4, r5, r6, r7};\n\t"
		:
		: "r" (&ztest_swap_return_val), "r" (swap_return_val)
		: "memory"
	);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__asm__ volatile (
		"stm %0, {%1};\n\t"
		:
		: "r" (&ztest_swap_return_val), "r" (swap_return_val)
		: "memory"
	);
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif


	/* After swap-back, verify that the callee-saved registers loaded,
	 * look exactly as what is located in the respective callee-saved
	 * container of the thread.
	 */
	verify_callee_saved(
		&ztest_thread_callee_saved_regs_container,
		&_current->callee_saved);

	/* Verify context-switch did occur. */
	test_flag = switch_flag;
	zassert_true(test_flag == true,
		"Switch flag not incremented as expected %u\n",
		switch_flag);
	/* Clear the switch flag to signal that the main test thread
	 * has been successfully swapped-in, as expected by the test.
	 */
	switch_flag = false;

	/* Verify that the arch.basepri flag is cleared, after
	 * the alternative thread modified it, since the thread
	 * is now switched back in.
	 */
	zassert_true(_current->arch.basepri == 0,
		"arch.basepri value not in accordance with the update\n");

#if defined(CONFIG_CPU_CORTEX_M_HAS_BASEPRI)
	/* Verify that the BASEPRI register is updated during the last
	 * swap-in of the thread.
	 */
	zassert_true(__get_BASEPRI() == BASEPRI_MODIFIED_2,
		"BASEPRI not in accordance with the update: 0x%0x\n",
		__get_BASEPRI());
#else
	/* For Cortex-M Baseline architecture, we verify that
	 * the interrupt lock is enabled.
	 */
	 zassert_true(__get_PRIMASK() != 0,
	 "PRIMASK not in accordance with the update: 0x%0x\n",
	 __get_PRIMASK());
#endif /* CONFIG_CPU_CORTEX_M_HAS_BASEPRI */

#if !defined(CONFIG_NO_OPTIMIZATIONS)
	/* The thread is now swapped-back in. */
	zassert_equal(_current->arch.swap_return_value, SWAP_RETVAL,
		"Swap value not set as expected: 0x%x (0x%x)\n",
		_current->arch.swap_return_value, SWAP_RETVAL);
	zassert_equal(_current->arch.swap_return_value, ztest_swap_return_val,
		"Swap value not returned as expected 0x%x (0x%x)\n",
		_current->arch.swap_return_value, ztest_swap_return_val);
#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Dump callee-saved registers to memory. */
	__asm__ volatile (
		"vstmia %0, {s16-s31};\n\t"
		:
		: "r" (&ztest_thread_fp_callee_saved_regs)
		: "memory"
	);

	/* After swap-back, verify that the FP callee-saved registers loaded,
	 * look exactly as what is located in the respective FP callee-saved
	 * container of the thread.
	 */
	verify_fp_callee_saved(
		&ztest_thread_fp_callee_saved_regs,
		&_current->arch.preempt_float);

	/* Verify that the main test thread restored the FPSCR bit-0. */
	zassert_true((__get_FPSCR() & 0x1) == 0x1,
		"FPSCR bit-0 not restored at swap: 0x%x\n", __get_FPSCR());

#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

}
/**
 * @}
 */
