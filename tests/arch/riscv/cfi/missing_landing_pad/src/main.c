/*
 * Copyright (c) 2026 Zephyr Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/riscv/csr.h>

#define STACK_SIZE  1024
#define THREAD_PRIO 5

K_THREAD_STACK_DEFINE(worker_stack, STACK_SIZE);
struct k_thread worker_thread;
K_SEM_DEFINE(fault_sem, 0, 1);

static volatile bool valid_fault;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
#ifdef CONFIG_RISCV_S_MODE
	unsigned long cause = csr_read(scause);
#else
	unsigned long cause = csr_read(mcause);
#endif
	cause &= CONFIG_RISCV_MCAUSE_EXCEPTION_MASK;

	if (valid_fault && cause == 18) {
		valid_fault = false;
		k_sem_give(&fault_sem);
		/* Abort only the faulting thread so ZTest thread can continue */
		k_thread_abort(k_current_get());
	} else {
		/* Unexpected fault */
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

/*
 * Top-level assembly defining bad_target_function.
 * Written directly in global assembly so the compiler cannot insert 'lpad'.
 */
__asm__(".section .text\n"
	".globl bad_target_function\n"
	".type bad_target_function, @function\n"
	"bad_target_function:\n"
	"    nop\n"
	"    ret\n"
	".size bad_target_function, .-bad_target_function\n");

/* C prototype for the assembly target function above */
extern void bad_target_function(void);

static void worker_entry(void *p1, void *p2, void *p3)
{
	/*
	 * Load address of bad_target_function into x6 (t1) — a register other than x1, x5, x7.
	 * Execute indirect call (jalr x1, x6, 0).
	 * If CONFIG_RISCV_LANDING_PADS (Zicfilp) is enabled, jumping to a function lacking an
	 * 'lpad' instruction triggers a landing pad fault (cause 18).
	 */
	__asm__ volatile("la x6, bad_target_function\n\t"
			 "jalr x1, x6, 0\n\t"
			 :
			 :
			 : "x6", "x1", "memory");
}

ZTEST(riscv_cfi_missing_landing_pad, test_missing_landing_pad_trap)
{
	valid_fault = true;

	/* Create worker thread to execute the faulting call */
	k_thread_create(&worker_thread, worker_stack, STACK_SIZE, worker_entry, NULL, NULL, NULL,
			THREAD_PRIO, 0, K_NO_WAIT);

	/* Wait for the fatal error handler to catch the trap */
	zassert_equal(k_sem_take(&fault_sem, K_MSEC(1000)), 0,
		      "Landing pad trap was not triggered within timeout");
}

ZTEST_SUITE(riscv_cfi_missing_landing_pad, NULL, NULL, NULL, NULL, NULL);
