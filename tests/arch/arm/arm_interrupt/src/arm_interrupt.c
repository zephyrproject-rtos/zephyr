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

	expected_reason = -1;
}

void arm_isr_handler(void *args)
{
	ARG_UNUSED(args);

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


/**
 * @}
 */
