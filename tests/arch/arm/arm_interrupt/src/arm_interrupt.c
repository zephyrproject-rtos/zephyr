/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
#include <cmsis_core.h>
#endif
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>

/* Abstraction layer for interrupt controller operations */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* Cortex-M: Use NVIC */
#define irq_controller_get_enable(irq)     NVIC_GetEnableIRQ(irq)
#define irq_controller_set_pending(irq)    NVIC_SetPendingIRQ(irq)
#define irq_controller_get_pending(irq)    NVIC_GetPendingIRQ(irq)
#define irq_controller_clear_pending(irq)  NVIC_ClearPendingIRQ(irq)
#elif defined(CONFIG_ARMV7_R)
/* Cortex-R: Use GIC Software Generated Interrupts (SGIs 0-15) */
#include <zephyr/drivers/interrupt_controller/gic.h>

static volatile bool actually_trigger_sgi;

static inline int irq_controller_get_enable(unsigned int irq)
{
	/* SGIs 0-15 are always available */
	return (irq < 16) ? 0 : 1;
}

static inline void irq_controller_set_pending(unsigned int irq)
{
	/* Only actually send SGI when told to (not during IRQ selection check) */
	if (irq < 16 && actually_trigger_sgi) {
		/* Send SGI to this CPU (target_aff=0, target_list=0x01 for CPU 0) */
		gic_raise_sgi(irq, 0, 0x01);
	}
}

static inline int irq_controller_get_pending(unsigned int irq)
{
	/* During IRQ selection (actually_trigger_sgi==false): return 1 for SGIs so they pass
	 * the test
	 * During actual testing (actually_trigger_sgi==true): return 0 since SGIs dont have
	 * queryable pending state
	 */
	if (irq < 16) {
		return actually_trigger_sgi ? 0 : 1;
	}
	return 0;
}

static inline void irq_controller_clear_pending(unsigned int irq)
{
	/* SGIs auto-clear on IAR read */
}

#endif


static volatile int test_flag;
static volatile int expected_reason = -1;

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/* Used to validate ESF collection during a fault */
static volatile int run_esf_validation;
static volatile int esf_validation_rv;
static volatile uint32_t expected_msp;
static K_THREAD_STACK_DEFINE(esf_collection_stack, 2048 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread esf_collection_thread;
#define MAIN_PRIORITY 7
#define PRIORITY      5

/**
 * Validates that pEsf matches state from set_regs_with_known_pattern()
 */
static int check_esf_matches_expectations(const struct arch_esf *pEsf)
{
	const uint16_t expected_fault_instruction = 0xde5a; /* udf #90 */
	const bool caller_regs_match_expected =
		(pEsf->basic.r0 == 0) && (pEsf->basic.r1 == 1) && (pEsf->basic.r2 == 2) &&
		(pEsf->basic.r3 == 3) && (pEsf->basic.lr == 15) &&
		(*(uint16_t *)pEsf->basic.pc == expected_fault_instruction);
	if (!caller_regs_match_expected) {
		printk("__basic_sf member of ESF is incorrect\n");
		return -1;
	}

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	const struct _callee_saved *callee_regs = pEsf->extra_info.callee;
	const bool callee_regs_match_expected =
		(callee_regs->v1 /* r4 */ == 4) && (callee_regs->v2 /* r5 */ == 5) &&
		(callee_regs->v3 /* r6 */ == 6) && (callee_regs->v4 /* r7 */ == 7) &&
		(callee_regs->v5 /* r8 */ == 8) && (callee_regs->v6 /* r9 */ == 9) &&
		(callee_regs->v7 /* r10 */ == 10) && (callee_regs->v8 /* r11 */ == 11);
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

	if ((pEsf->extra_info.exc_return & exc_bits_set_mask) != exc_bits_set_mask) {
		printk("Incorrect EXC_RETURN of 0x%08x", pEsf->extra_info.exc_return);
		return -1;
	}

	/* the psp should match the contents of the esf copy up
	 * to the xpsr. (the xpsr value in the copy used for pEsf
	 * is overwritten in fault.c)
	 */
	if (memcmp((void *)callee_regs->psp, pEsf, offsetof(struct arch_esf, basic.xpsr)) != 0) {
		printk("psp does not match __basic_sf provided\n");
		return -1;
	}

	if (pEsf->extra_info.msp != expected_msp) {
		printk("MSP is 0x%08x but should be 0x%08x", pEsf->extra_info.msp, expected_msp);
		return -1;
	}
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
	return 0;
}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE || CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	TC_PRINT("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		k_fatal_halt(reason);
	}

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason, expected_reason);
		k_fatal_halt(reason);
	}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	if (run_esf_validation) {
		if (check_esf_matches_expectations(pEsf) == 0) {
			esf_validation_rv = TC_PASS;
		}
		run_esf_validation = 0;
	}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE || CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

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
void set_regs_with_known_pattern(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	__asm__ volatile("mov r1, #1\n"
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
			 "udf #90\n");
}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/**
 * @brief Test to verify code fault handling in ISR execution context
 * @ingroup kernel_fatal_tests
 * @note Cortex-M only - uses MSP/PSP
 */
ZTEST(arm_interrupt, test_arm_esf_collection)
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
			K_THREAD_STACK_SIZEOF(esf_collection_stack), set_regs_with_known_pattern,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0, K_NO_WAIT);

	test_validation_rv = esf_validation_rv;

	zassert_not_equal(test_validation_rv, TC_FAIL, "ESF fault collection failed");
}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE || CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

void arm_isr_handler(const void *args)
{
	ARG_UNUSED(args);

#if defined(CONFIG_CPU_CORTEX_M) && defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Clear Floating Point Status and Control Register (FPSCR),
	 * to prevent from having the interrupt line set to pending again,
	 * in case FPU IRQ is selected by the test as "Available IRQ line"
	 */
#if defined(CONFIG_ARMV8_1_M_MAINLINE)
	/*
	 * For ARMv8.1-M with FPU, the FPSCR[18:16] LTPSIZE field must be set
	 * to 0b100 for "Tail predication not applied" as it's reset value
	 */
	__set_FPSCR(4 << FPU_FPDSCR_LTPSIZE_Pos);
#else
	__set_FPSCR(0);
#endif
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
#if defined(CONFIG_HW_STACK_PROTECTION)
		/*
		 * Verify that the Stack Overflow has been reported by the core
		 * and the expected reason variable is reset.
		 */
		int reason = expected_reason;

		zassert_equal(reason, -1, "expected_reason has not been reset (%d)\n", reason);
#endif
	}
}

/**
 * @brief Test ARM Interrupt handling
 * @ingroup kernel_arch_interrupt_tests
 */
ZTEST(arm_interrupt, test_arm_interrupt)
{
#if defined(CONFIG_EXTRA_EXCEPTION_INFO) && defined(CONFIG_ARMV7_R)
	/* EXTRA_EXCEPTION_INFO is not fully implemented for Cortex-R */
	ztest_test_skip();
#endif

	/* Determine an NVIC IRQ line that is not currently in use. */
	int i;

#if defined(CONFIG_ARMV7_R)
	/* Control when SGIs actually fire */
	actually_trigger_sgi = false;
#endif
	int init_flag, post_flag;
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	int reason;
#endif

	init_flag = test_flag;

	zassert_false(init_flag, "Test flag not initialized to zero\n");

#if defined(CONFIG_ARMV7_R)
	/* For Cortex-R, just use SGI 1 (0 is sometimes reserved) */
	i = 1;
	TC_PRINT("Using SGI %u for Cortex-R\n", i);
#else

	for (i = CONFIG_NUM_IRQS - 1; i >= 0; i--) {
		if (irq_controller_get_enable(i) == 0) {
			/*
			 * Interrupts configured statically with IRQ_CONNECT(.)
			 * are automatically enabled. irq_controller_get_enable()
			 * returning false, here, implies that the IRQ line is
			 * either not implemented or it is not enabled, thus,
			 * currently not in use by Zephyr.
			 */

			/* Set the NVIC line to pending. */
			irq_controller_set_pending(i);

			if (irq_controller_get_pending(i)) {
				/* If the NVIC line is pending, it is
				 * guaranteed that it is implemented; clear the
				 * line.
				 */
				irq_controller_clear_pending(i);

				if (!irq_controller_get_pending(i)) {
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

	zassert_true(i >= 0, "No available IRQ line to use in the test\n");

	TC_PRINT("Available IRQ line: %u\n", i);
#endif

#if defined(CONFIG_ARMV7_R)
	/* Now enable actual SGI triggering for subsequent tests */
	actually_trigger_sgi = true;
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	/* Verify that triggering an interrupt in an IRQ line,
	 * on which an ISR has not yet been installed, leads
	 * to a fault of type K_ERR_SPURIOUS_IRQ.
	 */
	expected_reason = K_ERR_SPURIOUS_IRQ;
	irq_controller_clear_pending(i);
	irq_enable(i);
	irq_controller_set_pending(i);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Verify that the spurious ISR has led to the fault and the
	 * expected reason variable is reset.
	 */
	reason = expected_reason;
	zassert_equal(reason, -1, "expected_reason has not been reset (%d)\n", reason);
	irq_disable(i);

#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE || CONFIG_ARMV7_M_ARMV8_M_MAINLINE */

	arch_irq_connect_dynamic(i, 0 /* highest priority */, arm_isr_handler, NULL, 0);

	irq_controller_clear_pending(i);
	irq_enable(i);

	for (int j = 1; j <= 3; j++) {

		/* Set the dynamic IRQ to pending state. */
		irq_controller_set_pending(i);

		/*
		 * Instruction barriers to make sure the NVIC IRQ is
		 * set to pending state before 'test_flag' is checked.
		 */
		barrier_dsync_fence_full();
		barrier_isync_fence_full();

#if defined(CONFIG_ARMV7_R)
		/* On Cortex-R, SGI is sent immediately - allow time for processing */
		k_busy_wait(100);
#endif

		/* Returning here implies the thread was not aborted. */

		/* Confirm test flag is set by the ISR handler. */
		post_flag = test_flag;
		zassert_true(post_flag == j, "Test flag not set by ISR\n");
	}

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
	irq_controller_clear_pending(i);
	irq_enable(i);
	irq_controller_set_pending(i);

	/* Manually set PSP almost at the bottom of the stack. An exception
	 * entry will make PSP descend below the limit and into the MPU guard
	 * section (or beyond the address pointed by PSPLIM in ARMv8-M MCUs).
	 */
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING) && defined(CONFIG_MPU_STACK_GUARD)
#define FPU_STACK_EXTRA_SIZE 0x48
	/* If an FP context is present, we should not set the PSP
	 * too close to the end of the stack, because stacking of
	 * the ESF might corrupt kernel memory, making it not
	 * possible to continue the test execution.
	 */
	uint32_t fp_extra_size = (__get_CONTROL() & CONTROL_FPCA_Msk) ? FPU_STACK_EXTRA_SIZE : 0;
	__set_PSP(_current->stack_info.start + 0x10 + fp_extra_size);
#else
	__set_PSP(_current->stack_info.start + 0x10);
#endif

	__enable_irq();
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* No stack variable access below this point.
	 * The IRQ will handle the verification.
	 */
#endif /* CONFIG_HW_STACK_PROTECTION */
}

#if defined(CONFIG_USERSPACE)
#include <zephyr/internal/syscall_handler.h>
#include "test_syscalls.h"

void z_impl_test_arm_user_interrupt_syscall(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* Confirm IRQs are not locked */
	zassert_false(__get_PRIMASK(), "PRIMASK is set\n");
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) && defined(CONFIG_USE_SWITCH)
	/* Confirm IRQs are not locked */
	zassert_false(__get_BASEPRI(), "BASEPRI is set\n");
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) && !defined(CONFIG_USE_SWITCH)
	/* The original ARMv7M userspace implementation had the
	 * somewhat odd (and dangerous!) behavior that an interrupt
	 * mask done in a system call would persist (!!) on return
	 * into userspace, and be seen again once we were back in
	 * kernel mode, and this is a test for it.  It's legacy
	 * though, not something we ever documented, and not the way
	 * userspace works (or even can work!) on other platforms, nor
	 * with the newer arch_switch() code.  Consider removing.
	 */
	static bool first_call = true;

	if (first_call == 1) {

		/* First time the syscall is invoked */
		first_call = false;

		/* Lock IRQs in supervisor mode */
		unsigned int key = irq_lock();

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
#include <zephyr/syscalls/test_arm_user_interrupt_syscall_mrsh.c>

/**
 * @brief Test ARM Interrupt handling in user mode
 * @ingroup kernel_arch_interrupt_tests
 */
ZTEST_USER(arm_interrupt, test_arm_user_interrupt)
{
	/* Test thread executing in user mode */
	zassert_true(arch_is_user_context(), "Test thread not running in user mode\n");

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
ZTEST_USER(arm_interrupt, test_arm_user_interrupt)
{
	ztest_test_skip();
}
#endif /* CONFIG_USERSPACE */

#pragma GCC push_options
#pragma GCC optimize("O0")
/* Avoid compiler optimizing null pointer de-referencing. */

/**
 * @brief Test ARM Null Pointer Exception handling
 * @ingroup kernel_arch_interrupt_tests
 */
ZTEST(arm_interrupt, test_arm_null_pointer_exception)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_CORTEX_M_NULL_POINTER_EXCEPTION);

	int reason;

	struct test_struct {
		uint32_t val[2];
	};

	struct test_struct *test_struct_null_pointer = 0x0;

	expected_reason = K_ERR_CPU_EXCEPTION;

	printk("Reading a null pointer value: 0x%0x\n", test_struct_null_pointer->val[1]);

	reason = expected_reason;
	zassert_equal(reason, -1, "expected_reason has not been reset (%d)\n", reason);
}
#pragma GCC pop_options
