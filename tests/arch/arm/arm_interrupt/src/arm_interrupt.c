/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

static volatile int test_flag;
static volatile int expected_reason = -1;

/* Used to validate ESF collection during a fault */
static volatile int run_esf_validation;
static volatile int esf_validation_rv;
static volatile uint32_t expected_msp;
static K_THREAD_STACK_DEFINE(esf_collection_stack, 1024);
static struct k_thread esf_collection_thread;
#define MAIN_PRIORITY 7
#define PRIORITY 5

/**
 * Validates that pEsf matches state from set_regs_with_known_pattern()
 */
static int check_esf_matches_expectations(const z_arch_esf_t *pEsf)
{
	const uint16_t expected_fault_instruction = 0xde5a; /* udf #90 */
	const bool caller_regs_match_expected =
		(pEsf->basic.r0 == 0) &&
		(pEsf->basic.r1 == 1) &&
		(pEsf->basic.r2 == 2) &&
		(pEsf->basic.r3 == 3) &&
		(pEsf->basic.lr == 15) &&
		(*(uint16_t *)pEsf->basic.pc == expected_fault_instruction);
	if (!caller_regs_match_expected) {
		printk("__basic_sf member of ESF is incorrect\n");
		return -1;
	}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	const struct _callee_saved *callee_regs = pEsf->extra_info.callee;
	const bool callee_regs_match_expected =
		(callee_regs->v1 /* r4 */ == 4) &&
		(callee_regs->v2 /* r5 */ == 5) &&
		(callee_regs->v3 /* r6 */ == 6) &&
		(callee_regs->v4 /* r7 */ == 7) &&
		(callee_regs->v5 /* r8 */ == 8) &&
		(callee_regs->v6 /* r9 */ == 9) &&
		(callee_regs->v7 /* r10 */ == 10) &&
		(callee_regs->v8 /* r11 */ == 11);
	if (!callee_regs_match_expected) {
		printk("_callee_saved_t member of ESF is incorrect\n");
		return -1;
	}

	/* we expect the EXC_RETURN value to have:
	 *  - PREFIX: bits [31:24] = 0xFF
	 *  - Mode, bit [3] = 1 since exception occurred from thread mode
	 *  - SPSEL, bit [2] = 1 since frame should reside on PSP
	 */
	const uint32_t exc_bits_set_mask = 0xff00000C;

	if ((pEsf->extra_info.exc_return & exc_bits_set_mask) !=
		exc_bits_set_mask) {
		printk("Incorrect EXC_RETURN of 0x%08x",
			pEsf->extra_info.exc_return);
		return -1;
	}

	/* the psp should match the contents of the esf copy up
	 * to the xpsr. (the xpsr value in the copy used for pEsf
	 * is overwritten in fault.c)
	 */
	if (memcmp((void *)callee_regs->psp, pEsf,
		offsetof(struct __esf, basic.xpsr)) != 0) {
		printk("psp does not match __basic_sf provided\n");
		return -1;
	}

	if (pEsf->extra_info.msp != expected_msp) {
		printk("MSP is 0x%08x but should be 0x%08x",
			pEsf->extra_info.msp, expected_msp);
		return -1;
	}
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
	return 0;
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	TC_PRINT("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		k_fatal_halt(reason);
	}

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason,
			expected_reason);
		k_fatal_halt(reason);
	}

	if (run_esf_validation) {
		if (check_esf_matches_expectations(pEsf) == 0) {
			esf_validation_rv = TC_PASS;
		}
		run_esf_validation = 0;
	}

	expected_reason = -1;
}

/**
 * Set ARM registers with a known pattern:
 *  r0-r12 are set to 0...12, respectively
 *  r13 (sp) is left untouched
 *  r14 (pc) will point to the faulting instruction (udf #90)
 *  r15 (lr) is set to 15 (since a fault takes place, we never use the value)
 *
 * Note: Routine was written to be ARMV6M compatible
 *
 * In k_sys_fatal_error_handler above we will check that the ESF provided
 * as a parameter matches these expectations.
 */
void set_regs_with_known_pattern(void)
{
	__asm__ volatile(
		"mov r1, #1\n"
		"mov r2, #2\n"
		"mov r3, #3\n"
		"mov r4, #4\n"
		"mov r5, #5\n"
		"mov r6, #6\n"
		"mov r7, #7\n"
		"mov r0, #8\n"
		"mov r8, r0\n"
		"add r0, r0, #1\n"
		"mov r9, r0\n"
		"add r0, r0, #1\n"
		"mov r10, r0\n"
		"add r0, r0, #1\n"
		"mov r11, r0\n"
		"add r0, r0, #1\n"
		"mov r12, r0\n"
		"add r0, r0, #3\n"
		"mov lr, r0\n"
		"mov r0, #0\n"
		"udf #90\n"
	);
}

void test_arm_esf_collection(void)
{
	int test_validation_rv;

	/* if the check in the fault handler succeeds,
	 * this will be set to TC_PASS
	 */
	esf_validation_rv = TC_FAIL;

	/* since the fault is from a task, the interrupt stack (msp)
	 * should match whatever the current value is
	 */
	expected_msp = __get_MSP();

	run_esf_validation = 1;
	expected_reason = K_ERR_CPU_EXCEPTION;

	/* Run test thread and main thread at same priority to guarantee the
	 * crashy thread we create below runs to completion before we get
	 * to the end of this function
	 */
	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

	TC_PRINT("Testing ESF Reporting\n");
	k_thread_create(&esf_collection_thread, esf_collection_stack,
			K_THREAD_STACK_SIZEOF(esf_collection_stack),
			(k_thread_entry_t)set_regs_with_known_pattern,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);

	test_validation_rv = esf_validation_rv;

	zassert_not_equal(test_validation_rv, TC_FAIL,
		"ESF fault collection failed");
}

void arm_isr_handler(const void *args)
{
	ARG_UNUSED(args);

#if defined(CONFIG_CPU_CORTEX_M) && defined(CONFIG_FPU) && \
	defined(CONFIG_FPU_SHARING)
	/* Clear Floating Point Status and Control Register (FPSCR),
	 * to prevent from having the interrupt line set to pending again,
	 * in case FPU IRQ is selected by the test as "Available IRQ line"
	 */
	__set_FPSCR(0);
#endif

	test_flag++;

	if (test_flag == 1) {
		/* Intentional Kernel oops */
		expected_reason = K_ERR_KERNEL_OOPS;
		k_oops();
	} else if (test_flag == 2) {
		/* Intentional Kernel panic */
		expected_reason = K_ERR_KERNEL_PANIC;
		k_panic();
	} else if (test_flag == 3) {
		/* Intentional ASSERT */
		expected_reason = K_ERR_KERNEL_PANIC;
		__ASSERT(0, "Intentional assert\n");
	} else if (test_flag == 4) {
#if defined(CONFIG_CPU_CORTEX_M_HAS_SYSTICK)
#if !defined(CONFIG_SYS_CLOCK_EXISTS) || !defined(CONFIG_CORTEX_M_SYSTICK)
		expected_reason = K_ERR_CPU_EXCEPTION;
		SCB->ICSR |= SCB_ICSR_PENDSTSET_Msk;
		__DSB();
		__ISB();
#endif
#endif
	} else if (test_flag == 5) {
#if defined(CONFIG_HW_STACK_PROTECTION)
		/*
		 * Verify that the Stack Overflow has been reported by the core
		 * and the expected reason variable is reset.
		 */
		int reason = expected_reason;

		zassert_equal(reason, -1,
			"expected_reason has not been reset (%d)\n", reason);
#endif
	}
}

void test_arm_interrupt(void)
{
	/* Determine an NVIC IRQ line that is not currently in use. */
	int i;
	int init_flag, post_flag, reason;

	init_flag = test_flag;

	zassert_false(init_flag, "Test flag not initialized to zero\n");

	for (i = CONFIG_NUM_IRQS - 1; i >= 0; i--) {
		if (NVIC_GetEnableIRQ(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. NVIC_GetEnableIRQ()
			 * returning false, here, implies that the IRQ line is
			 * either not implemented or it is not enabled, thus,
			 * currently not in use by Zephyr.
			 */

			/* Set the NVIC line to pending. */
			NVIC_SetPendingIRQ(i);

			if (NVIC_GetPendingIRQ(i)) {
				/* If the NVIC line is pending, it is
				 * guaranteed that it is implemented; clear the
				 * line.
				 */
				NVIC_ClearPendingIRQ(i);

				if (!NVIC_GetPendingIRQ(i)) {
					/*
					 * If the NVIC line can be successfully
					 * un-pended, it is guaranteed that it
					 * can be used for software interrupt
					 * triggering.
					 */
					break;
				}
			}
		}
	}

	zassert_true(i >= 0,
		"No available IRQ line to use in the test\n");

	TC_PRINT("Available IRQ line: %u\n", i);

	/* Verify that triggering an interrupt in an IRQ line,
	 * on which an ISR has not yet been installed, leads
	 * to a fault of type K_ERR_SPURIOUS_IRQ.
	 */
	expected_reason = K_ERR_SPURIOUS_IRQ;
	NVIC_ClearPendingIRQ(i);
	NVIC_EnableIRQ(i);
	NVIC_SetPendingIRQ(i);
	__DSB();
	__ISB();

	/* Verify that the spurious ISR has led to the fault and the
	 * expected reason variable is reset.
	 */
	reason = expected_reason;
	zassert_equal(reason, -1,
		"expected_reason has not been reset (%d)\n", reason);
	NVIC_DisableIRQ(i);

	arch_irq_connect_dynamic(i, 0 /* highest priority */,
		arm_isr_handler,
		NULL,
		0);

	NVIC_ClearPendingIRQ(i);
	NVIC_EnableIRQ(i);

	for (int j = 1; j <= 3; j++) {

		/* Set the dynamic IRQ to pending state. */
		NVIC_SetPendingIRQ(i);

		/*
		 * Instruction barriers to make sure the NVIC IRQ is
		 * set to pending state before 'test_flag' is checked.
		 */
		__DSB();
		__ISB();

		/* Returning here implies the thread was not aborted. */

		/* Confirm test flag is set by the ISR handler. */
		post_flag = test_flag;
		zassert_true(post_flag == j, "Test flag not set by ISR\n");
	}

#if defined(CONFIG_CPU_CORTEX_M_HAS_SYSTICK)
#if !defined(CONFIG_SYS_CLOCK_EXISTS) || !defined(CONFIG_CORTEX_M_SYSTICK)
	/* Verify that triggering a Cortex-M exception (accidentally) that has
	 * not been installed in the vector table, leads to the reserved
	 * exception been called and a resulting CPU fault. We test this using
	 * the SysTick exception in platforms that are not expecting to use the
	 * SysTick timer for system timing.
	 */

	/* The ISR will manually set the SysTick exception to pending state. */
	NVIC_SetPendingIRQ(i);
	__DSB();
	__ISB();

	/* Verify that the spurious exception has led to the fault and the
	 * expected reason variable is reset.
	 */
	reason = expected_reason;
	zassert_equal(reason, -1,
		"expected_reason has not been reset (%d)\n", reason);
#endif
#endif

#if defined(CONFIG_HW_STACK_PROTECTION)
	/*
	 * Simulate a stacking error that is caused explicitly by the
	 * exception entry context stacking, to verify that the CPU can
	 * correctly report stacking errors that are not also Data
	 * access violation errors.
	 */
	expected_reason = K_ERR_STACK_CHK_FAIL;

	__disable_irq();

	/* Trigger an interrupt to cause the stacking error */
	NVIC_ClearPendingIRQ(i);
	NVIC_EnableIRQ(i);
	NVIC_SetPendingIRQ(i);

	/* Set test flag so the IRQ handler executes the appropriate case. */
	test_flag = 4;

	/* Manually set PSP almost at the bottom of the stack. An exception
	 * entry will make PSP descend below the limit and into the MPU guard
	 * section (or beyond the address pointed by PSPLIM in ARMv8-M MCUs).
	 */
	__set_PSP(_current->stack_info.start + 0x10);

	__enable_irq();
	__DSB();
	__ISB();

	/* No stack variable access below this point.
	 * The IRQ will handle the verification.
	 */
#endif /* CONFIG_HW_STACK_PROTECTION */
}

#if defined(CONFIG_USERSPACE)
#include <syscall_handler.h>
#include "test_syscalls.h"

void z_impl_test_arm_user_interrupt_syscall(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* Confirm IRQs are not locked */
	zassert_false(__get_PRIMASK(), "PRIMASK is set\n");
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

	static bool first_call = 1;

	if (first_call == 1) {

		/* First time the syscall is invoked */
		first_call = 0;

		/* Lock IRQs in supervisor mode */
		int key = irq_lock();

		/* Verify that IRQs were not already locked */
		zassert_false(key, "IRQs locked in system call\n");
	}

	/* Confirm IRQs are still locked */
	zassert_true(__get_BASEPRI(), "BASEPRI not set\n");
#endif
}

static inline void z_vrfy_test_arm_user_interrupt_syscall(void)
{
	z_impl_test_arm_user_interrupt_syscall();
}
#include <syscalls/test_arm_user_interrupt_syscall_mrsh.c>

void test_arm_user_interrupt(void)
{
	/* Test thread executing in user mode */
	zassert_true(arch_is_user_context(),
		"Test thread not running in user mode\n");

	/* Attempt to lock IRQs in user mode */
	irq_lock();
	/* Attempt to lock again should return non-zero value of previous
	 * locking attempt, if that were to be successful.
	 */
	int lock = irq_lock();

	zassert_false(lock, "IRQs shown locked in user mode\n");

	/* Generate a system call to manage the IRQ locking */
	test_arm_user_interrupt_syscall();

	/* Attempt to unlock IRQs in user mode */
	irq_unlock(0);

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* The first system call has left the IRQs locked.
	 * Generate a second system call to inspect the IRQ locking.
	 *
	 * In Cortex-M Baseline system calls cannot be invoked
	 * with interrupts locked, so we skip this part of the
	 * test.
	 */
	test_arm_user_interrupt_syscall();

	/* Verify that thread is not able to infer that IRQs are locked. */
	zassert_false(irq_lock(), "IRQs are shown to be locked\n");
#endif
}
#else
void test_arm_user_interrupt(void)
{
	TC_PRINT("Skipped\n");
}
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_CORTEX_M_DEBUG_NULL_POINTER_EXCEPTION)
#pragma GCC push_options
#pragma GCC optimize("O0")
/* Avoid compiler optimizing null pointer de-referencing. */
void test_arm_null_pointer_exception(void)
{
	int reason;

	struct test_struct {
		uint32_t val[2];
	};

	struct test_struct *test_struct_null_pointer = 0x0;

	expected_reason = K_ERR_CPU_EXCEPTION;

	printk("Reading a null pointer value: 0x%0x\n",
		test_struct_null_pointer->val[1]);

	reason = expected_reason;
	zassert_equal(reason, -1,
		"expected_reason has not been reset (%d)\n", reason);
}
#pragma GCC pop_options
#else
void test_arm_null_pointer_exception(void)
{
	TC_PRINT("Skipped\n");
}

#endif /* CONFIG_CORTEX_M_DEBUG_NULL_POINTER_EXCEPTION */

/**
 * @}
 */
