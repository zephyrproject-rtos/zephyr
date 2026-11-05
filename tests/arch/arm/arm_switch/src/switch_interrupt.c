/*
 * SPDX-FileCopyrightText: 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include "vector_hijack.h"

bool expect_stack_align;
bool saw_stack_align;

/* The "switch dest" thread is used as a dummy destination for
 * a context switch.
 */
K_THREAD_STACK_DEFINE(switch_dest_stack, 1024);
struct k_thread switch_dest_thread;
k_tid_t switch_dest_thread_id;

/* ID of thread that invoked `test_restore_alignment_word` */
k_tid_t test_thread_id;

/* Entrypoint of the switch destination thread. */
void switch_dest_entry(void *a, void *b, void *c)
{
	/* Immediately trigger an SVC, moving us back to the test thread */
	__asm__ volatile("svc #0");
}

void my_svc(void)
{
	if (expect_stack_align) {
		/* First time entering the SVC, from the main test thread */
		expect_stack_align = false;

		/*
		 * Basic exception frame: r0-r3, r12, lr, pc, xPSR.
		 * Thus xPSR is at offset 7.
		 */
		uint32_t *psp = (uint32_t *)__get_PSP();

		zassert_true(psp[7] & BIT(9), "stack-align bit not set in stacked xPSR");
		saw_stack_align = true;

		/* Suspend the main thread and wake up a new thread to context switch to */
		k_wakeup(switch_dest_thread_id);
		k_thread_suspend(test_thread_id);
	} else {
		/* Resume the main test thread */
		k_thread_resume(test_thread_id);
		k_thread_suspend(switch_dest_thread_id);

		/*
		 * Return from interrupt triggers restoration of the hardware frame
		 * created in the first SVC.
		 */
	}

	arm_m_exc_tail();
}

/* Triggers SVC 0 with misaligned PSP */
static __noinline __attribute__((naked)) void misalign_then_svc(void)
{
	__asm__ volatile("sub sp, sp, #4\n"
			 "svc #0\n"
			 "add sp, sp, #4\n"
			 "bx lr");
}

/*
 * When an interrupt is taken with a stack that is not 8-byte aligned,
 * the hardware adds an alignment word. Upon interrupt exit, context
 * switching may occur.
 *
 * When we switch back to a context that previously had an alignment word,
 * we need to restore that alignment word before returning from interrupt.
 * This test verifies the alignment word is restored.
 */
ZTEST(arm_switch_interrupt, test_restore_alignment_word)
{
	vector_hijack(my_svc);

	test_thread_id = _current;

	/* Create dummy thread to switch to */
	switch_dest_thread_id =
		k_thread_create(&switch_dest_thread, switch_dest_stack, 1024, switch_dest_entry,
				NULL, NULL, NULL, -1, 0, K_NO_WAIT);

	printk("Misaligning PSP and invoking SVC\n");
	expect_stack_align = true;
	misalign_then_svc();

	/* Verify that the SVC was entered with a misaligned stack */
	zassert_true(saw_stack_align, "SVC was not taken with misaligned stack");

	/* Successful switch back to this context means the alignment word was restored properly */
}

ZTEST_SUITE(arm_switch_interrupt, NULL, NULL, NULL, NULL, NULL);
