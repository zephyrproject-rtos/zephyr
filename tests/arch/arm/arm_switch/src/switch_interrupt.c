/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include "vector_hijack.h"

static bool expect_stack_align;
static bool saw_stack_align;
static void *saved_vtor;

/* The "switch dest" thread is used as a dummy destination for
 * a context switch.
 */
static K_THREAD_STACK_DEFINE(switch_dest_stack, 1024);
static struct k_thread switch_dest_thread;
static k_tid_t switch_dest_thread_id;

/* ID of thread to switch back to after returning from the dummy thread */
static k_tid_t test_thread_id;

/* Set to true when the custom SVC handler schedules the dummy thread */
static bool switched_to_dummy;

/* Entrypoint of the switch destination thread, when we're not testing FPU state */
static void switch_dest_no_fpu(void *a, void *b, void *c)
{
	/* Immediately trigger an SVC, moving us back to the test thread */
	__asm__ volatile("svc #0");
}

static void align_check_svc(void)
{
	if (!switched_to_dummy) {
		/*
		 * Basic exception frame: r0-r3, r12, lr, pc, xPSR.
		 * Thus xPSR is at offset 7.
		 */
		uint32_t *psp = (uint32_t *)__get_PSP();

		saw_stack_align = (psp[7] & BIT(9)) != 0U;
		zassert_equal(saw_stack_align, expect_stack_align,
			      "unexpected stack-align bit in stacked xPSR");
	}

	if (!switched_to_dummy) {
		/* First time entering the SVC, from the main test thread */
		switched_to_dummy = true;

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

/* Triggers SVC #0 synchronously but offsets the sp by 4 bytes so the hardware
 * has to realign the stack by pushing an additional word.
 */
static __noinline __attribute__((naked)) void svc0_unalign_stack(void)
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
	switch_dest_thread_id =
		k_thread_create(&switch_dest_thread, switch_dest_stack,
				K_THREAD_STACK_SIZEOF(switch_dest_stack), switch_dest_no_fpu,
				NULL, NULL, NULL, K_PRIO_COOP(0), 0, K_FOREVER);

	printk("Misaligning PSP and invoking SVC\n");
	expect_stack_align = true;
	svc0_unalign_stack();

	zassert_true(saw_stack_align, "SVC was not taken with misaligned stack");
}

#ifdef CONFIG_FPU

#define FP_REG_CLOBBERS \
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", \
	"s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", \
	"s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", \
	"s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"

/* Armv8.1-M requires LTPSIZE=4 when tail predication is not active. */
#define FPSCR_DEFAULT ((uint32_t)(IS_ENABLED(CONFIG_ARMV8_1_M_MAINLINE) ? BIT(18) : 0U))
/* NZCV, AHP, DN, FZ, RMode and cumulative exception flags, without RES0 bits. */
#define FPSCR_SENTINEL (0xF7C0009FU | FPSCR_DEFAULT)

static void switch_dest_fpu(void *a, void *b, void *c)
{
	/* Clobber the FP registers with a different set of recognisable values */
	uint32_t sregs[32];

	for (size_t i = 0; i < ARRAY_SIZE(sregs); i++) {
		sregs[i] = 0xbbbbbb00U + i;
	}

	/* Keep the register writes, FPCA clear and SVC together: compiler-generated
	 * FP instructions between them could reactivate the context or change the sentinels.
	 */
	__asm__ volatile("vldm %[regs], {s0-s31}\n"
			 "vmsr fpscr, %[fpscr]\n"
			 "mrs r0, control\n"
			 "bic r0, r0, #4\n"
#if defined(CONFIG_ARMV8_M_SE) && !defined(CONFIG_ARM_NONSECURE_FIRMWARE)
			 "bic r0, r0, #8\n"
#endif
			 "msr control, r0\n"
			 "isb\n"
			 "svc #0\n"
			 :
			 : [regs] "r"(sregs), [fpscr] "r"(FPSCR_DEFAULT)
			 : "r0", "memory", FP_REG_CLOBBERS);
}

/* Carries out a test which verifies that FPU state is correctly restored
 * upon interrupt exit.
 *
 * If `misalign_svc` is true, then the original interrupt will be taken
 * with a stack pointer that is not 8 byte aligned, forcing the hardware
 * to push an alignment word.
 */
static void fpu_state_restore_generic(bool misalign_svc)
{
	switch_dest_thread_id =
		k_thread_create(&switch_dest_thread, switch_dest_stack,
				K_THREAD_STACK_SIZEOF(switch_dest_stack), switch_dest_fpu,
				NULL, NULL, NULL, K_PRIO_COOP(0), 0, K_FOREVER);

	/* Load all FP registers with sentinel values,
	 * and load a set of flags into the FPSCR.
	 */
	uint32_t sregs[32];
	uint32_t restored_sregs[32];
	uint32_t restored_fpscr;
	uint32_t saved_fpscr;
	CONTROL_Type control;

	for (size_t i = 0; i < ARRAY_SIZE(sregs); i++) {
		sregs[i] = 0x3f800000U + i;
	}
	expect_stack_align = misalign_svc;

	/* Capture CONTROL before any FP instruction can set FPCA again. Keep all
	 * register observations inside the asm so neither C nor zassert calls can
	 * clobber s0-s15. The clobbers also make the compiler preserve s16-s31.
	 */
	__asm__ volatile("vmrs %[saved], fpscr\n"
			 "vldm %[in], {s0-s31}\n"
			 "vmsr fpscr, %[fpscr]\n"
			 "cmp %[pad], #0\n"
			 "beq 1f\n"
			 "sub sp, sp, #4\n"
			 "svc #0\n"
			 "add sp, sp, #4\n"
			 "b 2f\n"
			 "1: svc #0\n"
			 "2: mrs %[control], control\n"
			 "vmrs %[restored], fpscr\n"
			 "vstm %[out], {s0-s31}\n"
			 "vmsr fpscr, %[saved]\n"
			 : [saved] "=&r"(saved_fpscr), [control] "=&r"(control.w),
			   [restored] "=&r"(restored_fpscr)
			 : [in] "r"(sregs), [out] "r"(restored_sregs),
			   [fpscr] "r"(FPSCR_SENTINEL), [pad] "r"((uint32_t)misalign_svc)
			 : "memory", "cc", FP_REG_CLOBBERS);

	/* ....switched to dummy thread and then back */
	zassert_true(saw_stack_align || !misalign_svc, "SVC was not taken with misaligned stack");

	/* Verify CONTROL.FPCA is set (and SFPA if running in secure mode):
	 * If this bit is not set back to 1, then a future context switch will assume the FP
	 * registers are not in use, and will not bother pushing them to the stack.
	 */
	zassert_true(control.b.FPCA != 0U, "CONTROL.FPCA not set");

#if defined(CONFIG_ARMV8_M_SE) && !defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	zassert_true(control.b.SFPA != 0U, "CONTROL.SFPA not set");
#endif

	zassert_equal(FPSCR_SENTINEL, restored_fpscr,
		      "FPSCR improperly restored: expected 0x%08x, got 0x%08x", FPSCR_SENTINEL,
		      restored_fpscr);

	/* Verify all the FP registers match */
	bool failure = false;

	for (size_t i = 0; i < ARRAY_SIZE(sregs); i++) {
		if (sregs[i] != restored_sregs[i]) {
			TC_ERROR("s%zu improperly restored: expected 0x%08x, got 0x%08x\n", i,
				 sregs[i], restored_sregs[i]);
			failure = true;
		}
	}

	if (failure) {
		ztest_test_fail();
	}
}
ZTEST(arm_switch_interrupt, test_fpu_restore)
{
	fpu_state_restore_generic(false);
}

ZTEST(arm_switch_interrupt, test_fpu_restore_align)
{
	fpu_state_restore_generic(true);
}

#endif

static void before(void *fixture)
{
	expect_stack_align = false;
	saw_stack_align = false;
	switched_to_dummy = false;
	switch_dest_thread_id = NULL;
	test_thread_id = k_current_get();
	saved_vtor = vector_hijack(align_check_svc);
}

static void after(void *fixture)
{
	SCB->VTOR = (uint32_t)saved_vtor;
	__DSB();
	__ISB();
	if (switch_dest_thread_id != NULL) {
		k_thread_abort(switch_dest_thread_id);
	}
}

ZTEST_SUITE(arm_switch_interrupt, NULL, NULL, before, after, NULL);
