/*
 * Copyright (c) 2020 Nordic Semiconductor ASA.
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

#if defined(CONFIG_USERSPACE)

#define PRIORITY 0
#define DB_VAL 0xDEADBEEF

static struct k_thread user_thread;
static K_THREAD_STACK_DEFINE(user_thread_stack, 1024);

#include <syscall_handler.h>
#include "test_syscalls.h"

void z_impl_test_arm_user_syscall(void)
{
	/* User thread system call
	 *
	 * Verify the following
	 * - mode variable indicates PRIV mode
	 * - the PSP is inside the thread's privileged stack
	 * - PSPLIM register guards the privileged stack
	 * - MSPLIM register still guards the interrupt stack
	 */
	zassert_true((_current->arch.mode & CONTROL_nPRIV_Msk) == 0,
	"mode variable not set to PRIV mode in system call\n");

	zassert_false(arch_is_user_context(),
	"arch_is_user_context() indicates nPRIV\n");

	zassert_true(
		((__get_PSP() >= _current->arch.priv_stack_start) &&
		(__get_PSP() < (_current->arch.priv_stack_start +
			CONFIG_PRIVILEGED_STACK_SIZE))),
	"Process SP outside thread privileged stack limits\n");

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	zassert_true(__get_PSPLIM() == _current->arch.priv_stack_start,
	"PSPLIM not guarding the thread's privileged stack\n");
	zassert_true(__get_MSPLIM() == (uint32_t)z_interrupt_stacks,
	"MSPLIM not guarding the interrupt stack\n");
#endif
}

static inline void z_vrfy_test_arm_user_syscall(void)
{
	z_impl_test_arm_user_syscall();
}
#include <syscalls/test_arm_user_syscall_mrsh.c>


void arm_isr_handler(const void *args)
{
	ARG_UNUSED(args);

	/* Interrupt triggered while running a user thread
	 *
	 * Verify the following
	 * - mode variable indicates nPRIV mode
	 * - the PSP is inside the thread's default (user) stack
	 * - PSPLIM register is not set (applies on the second ISR call)
	 * - MSPLIM register still guards the interrupt stack
	 */

	zassert_true((_current->arch.mode & CONTROL_nPRIV_Msk) != 0,
	"mode variable not set to nPRIV mode for user thread\n");

	zassert_false(arch_is_user_context(),
	"arch_is_user_context() indicates nPRIV in ISR\n");

	zassert_true(
		((__get_PSP() >= _current->stack_info.start) &&
		(__get_PSP() < (_current->stack_info.start +
			_current->stack_info.size))),
	"Process SP outside thread stack limits\n");

	static int first_call = 1;

	if (first_call == 1) {
		first_call = 0;

		/* Trigger thread yield() manually */
		(void)irq_lock();
		z_move_thread_to_end_of_prio_q(_current);
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		irq_unlock(0);

	} else if (first_call == 0) {
#if defined(CONFIG_BUILTIN_STACK_GUARD)
		/* Second ISR run occurs after thread context-switch.
		 * We expect PSPLIM to be clear at this point.
		 */
		zassert_true(__get_PSPLIM() == 0,
		"PSPLIM not clear\n");
		zassert_true(__get_MSPLIM() == (uint32_t)z_interrupt_stacks,
		"MSPLIM not guarding the interrupt stack\n");
#endif
	}
}

static void user_thread_entry(uint32_t irq_line)
{
	/* User Thread */
#if !defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	ARG_UNUSED(irq_line);
#endif
	/* Trigger a system call to switch to supervisor thread
	 * mode and verify the thread state during system calls.
	 */
	test_arm_user_syscall();

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

	/*
	 * Trigger an ISR to switch to handler mode, to inspect
	 * the kernel structs and verify the thread state.
	 */
	TC_PRINT("USR Thread: IRQ Line: %u\n", (uint32_t)irq_line);

	NVIC->STIR = irq_line;
	__DSB();
	__ISB();

	/* ISR is set to cause thread to context-switch -out and -in again.
	 * We inspect for a second time, to verlfy the status, after
	 * the user thread is switch back in.
	 */
	NVIC->STIR = irq_line;
	__DSB();
	__ISB();
#endif
}

void test_arm_syscalls(void)
{
	int i = 0;

	/* Supervisor Thread (ztest thread)
	 *
	 * Verify the following:
	 * - the "mode" variable indicates PRIV mode
	 * - arch_is_user_context() is negative
	 * - the PSP is inside the default thread stack
	 * - PSPLIM register guards the default stack
	 * - MSPLIM register guards the interrupt stack
	 */

	zassert_true((_current->arch.mode & CONTROL_nPRIV_Msk) == 0,
	"mode variable not set to PRIV mode for supervisor thread\n");

	zassert_false(arch_is_user_context(),
	"arch_is_user_context() indicates nPRIV\n");

	zassert_true(
		((__get_PSP() >= _current->stack_info.start) &&
		(__get_PSP() < (_current->stack_info.start +
			_current->stack_info.size))),
	"Process SP outside thread stack limits\n");

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	zassert_true(__get_PSPLIM() == _current->stack_info.start,
	"PSPLIM not guarding the default stack\n");
	zassert_true(__get_MSPLIM() == (uint32_t)z_interrupt_stacks,
	"MSPLIM not guarding the interrupt stack\n");
#endif

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
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
				 * guaranteed that it is implemented.
				 */
				break;
			}
		}
	}

	zassert_true(i >= 0,
		 "No available IRQ line to use in the test\n");

	TC_PRINT("Available IRQ line: %u\n", i);

	arch_irq_connect_dynamic(i, 0 /* highest priority */,
		arm_isr_handler,
		NULL,
		0);

	NVIC_ClearPendingIRQ(i);
	NVIC_EnableIRQ(i);

	/* Allow the user thread to trigger an interrupt;
	 * this is *ONLY* done for testing purposes, here,
	 * i.e. to allow the inspection of the thread state
	 * while running in user mode.
	 */
	 SCB->CCR |= SCB_CCR_USERSETMPEND_Msk;
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE*/

	/* Create and switch to a user thread, passing
	 * as argument the IRQ line to used in the test.
	 */
	k_thread_create(&user_thread,
		user_thread_stack,
		K_THREAD_STACK_SIZEOF(user_thread_stack),
		(k_thread_entry_t)user_thread_entry,
		(uint32_t *)i, NULL, NULL,
		K_PRIO_COOP(PRIORITY), K_USER,
		K_NO_WAIT);
}

void z_impl_test_arm_cpu_write_reg(void)
{
	/* User thread CPU write registers system call for testing
	 *
	 * Verify the following
	 * - Write 0xDEADBEEF values during system call into registers
	 * - In main test we will read that registers to verify
	 * that all of them were scrubbed and do not contain any sensitive data
	 */

	/* Part below is made to test that kernel scrubs CPU registers
	 * after returning from the system call
	 */
	TC_PRINT("Writing 0xDEADBEEF values into registers\n");
	__asm__ volatile (
		"ldr r0, =0xDEADBEEF;\n\t"
		"ldr r1, =0xDEADBEEF;\n\t"
		"ldr r2, =0xDEADBEEF;\n\t"
		"ldr r3, =0xDEADBEEF;\n\t"
		);
	TC_PRINT("Exit from system call\n");
}

static inline void z_vrfy_test_arm_cpu_write_reg(void)
{
	z_impl_test_arm_cpu_write_reg();
}
#include <syscalls/test_arm_cpu_write_reg_mrsh.c>

/**
 * @brief Test CPU scrubs registers after system call
 *
 * @details - Call from user mode a syscall test_arm_cpu_write_reg(),
 * the system call function writes into registers 0xDEADBEEF value
 * - Then in main test function below check registers values,
 * if no 0xDEADBEEF value detected, that means CPU scrubbed registers
 * before exit from the system call.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_syscall_cpu_scrubs_regs(void)
{
	uint32_t arm_reg_val[4];

	test_arm_cpu_write_reg();

	__asm__ volatile ("mov %0, r0" : "=r"(arm_reg_val[0]));
	__asm__ volatile ("mov %0, r1" : "=r"(arm_reg_val[1]));
	__asm__ volatile ("mov %0, r2" : "=r"(arm_reg_val[2]));
	__asm__ volatile ("mov %0, r3" : "=r"(arm_reg_val[3]));

	for (int i = 0; i < 4; i++) {
		zassert_not_equal(arm_reg_val[i], DB_VAL,
				"register value is 0xDEADBEEF, "
				"not scrubbed after system call.");
	}
}
#endif /* CONFIG_USERSPACE */
/**
 * @}
 */
