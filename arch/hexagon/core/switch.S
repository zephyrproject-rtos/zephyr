/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Hexagon context switching implementation
 *
 * Uses a flat stack frame (no allocframe/deallocframe) to save and
 * restore callee-saved registers.  New threads get a fake switch
 * frame on the stack with r31 set to z_hexagon_thread_start, so
 * the restore path's jumpr r31 naturally starts the thread.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <hexagon_vm.h>
#include <switch_frame.h>

GTEXT(z_hexagon_arch_switch)
GTEXT(z_hexagon_thread_start)

/*
 * z_hexagon_arch_switch(void *switch_to, void **switched_from)
 *
 * switch_to (r0): New thread's stack pointer (switch_handle)
 * switched_from (r1): Pointer to old thread's switch_handle field
 *
 * Save callee-saved registers into a flat frame, store SP into
 * *switched_from, load SP from switch_to, restore callee-saved
 * registers, pop the frame, and jumpr r31.
 *
 * For new threads, r31 in the fake frame is z_hexagon_thread_start.
 * For existing threads, r31 is the return address of the caller.
 */
SECTION_FUNC(TEXT, z_hexagon_arch_switch)
	/* Allocate stack frame manually (no allocframe) */
	{
		r29 = add(r29, #-SWITCH_FRAME_SIZE)
	}

	/* Save callee-saved registers to stack */
	{
		memd(r29+#SWITCH_R1716) = r17:16
		memd(r29+#SWITCH_R1918) = r19:18
	}
	{
		memd(r29+#SWITCH_R2120) = r21:20
		memd(r29+#SWITCH_R2322) = r23:22
	}
	{
		memd(r29+#SWITCH_R2524) = r25:24
		memd(r29+#SWITCH_R2726) = r27:26
	}
	/* Save frame pointer (r30) and link register (r31) */
	{
		memd(r29+#SWITCH_FP_LR) = r31:30
	}

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Save current UGP (TLS pointer) */
	{
		r2 = ugp
	}
	{
		memw(r29+#SWITCH_UGP) = r2
	}
#endif

	/* Store current stack pointer as switch handle for old thread */
	{
		memw(r1) = r29
	}

	/* Switch to new thread's stack */
	{
		r29 = r0
	}

	/* Restore callee-saved registers from new thread's stack */
	{
		r17:16 = memd(r29+#SWITCH_R1716)
		r19:18 = memd(r29+#SWITCH_R1918)
	}
	{
		r21:20 = memd(r29+#SWITCH_R2120)
		r23:22 = memd(r29+#SWITCH_R2322)
	}
	{
		r25:24 = memd(r29+#SWITCH_R2524)
		r27:26 = memd(r29+#SWITCH_R2726)
	}
	/* Restore frame pointer and link register */
	{
		r31:30 = memd(r29+#SWITCH_FP_LR)
	}

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Restore UGP (TLS pointer) */
	{
		r2 = memw(r29+#SWITCH_UGP)
	}
	{
		ugp = r2
	}
#endif

	/* Pop frame and return to saved LR */
	{
		r29 = add(r29, #SWITCH_FRAME_SIZE)
		jumpr r31
	}

/*
 * z_hexagon_thread_start
 *
 * Trampoline for new threads.  The fake switch frame sets up:
 *   r16 = entry, r17 = p1, r18 = p2, r19 = p3
 * Move them into the argument registers and tail-call z_thread_entry.
 *
 * New threads arrive here via arch_switch with interrupts disabled
 * (the do_swap path that would normally call arch_irq_unlock is
 * bypassed for new threads).  Enable interrupts before entering
 * the thread entry function, following the same pattern as RISC-V
 * and ARM thread start trampolines.
 */
SECTION_FUNC(TEXT, z_hexagon_thread_start)
	/* Enable interrupts for the new thread (vmsetie(1)) */
	{
		r0 = #1
	}
	trap1(#HEXAGON_VM_vmsetie)
	{
		r0 = r16
		r1 = r17
	}
	{
		r2 = r18
		r3 = r19
	}
	{
		jump ##z_thread_entry
	}
