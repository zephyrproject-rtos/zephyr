/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Interrupt and exception handling for Hexagon under HVM (H2).
 *
 * When H2 delivers an event to the guest:
 *   - GELR (g0) = return PC (where the guest was interrupted)
 *   - GSR  (g1) = guest status register (IE bit 30, etc.)
 *   - Guest virtual interrupts are disabled (GSR.IE=0)
 *   - GPRs are unchanged (still hold the interrupted code's values)
 *
 * On exit the guest sets g0 = return PC, g1 = saved status, and
 * calls trap1(#vmrte) to resume the interrupted context.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <hexagon_vm.h>

/* Event context structure offsets */
#define EVENT_CTX_R0_R1     0x00
#define EVENT_CTX_R2_R3     0x08
#define EVENT_CTX_R4_R5     0x10
#define EVENT_CTX_R6_R7     0x18
#define EVENT_CTX_R8_R9     0x20
#define EVENT_CTX_R10_R11   0x28
#define EVENT_CTX_R12_R13   0x30
#define EVENT_CTX_R14_R15   0x38
#define EVENT_CTX_PRED      0x40
#define EVENT_CTX_LR        0x44
#define EVENT_CTX_GELR      0x48
#define EVENT_CTX_GSR       0x4C
#define EVENT_CTX_SA0       0x50
#define EVENT_CTX_LC0       0x54
#define EVENT_CTX_SA1       0x58
#define EVENT_CTX_LC1       0x5C
#define EVENT_CTX_M0        0x60
#define EVENT_CTX_M1        0x64
#define EVENT_CTX_USR       0x68
#define EVENT_CTX_R28       0x6C
#define EVENT_CTX_SCRATCH   0x70
#if defined(CONFIG_GDBSTUB) || defined(CONFIG_DEBUG_COREDUMP)
#define EVENT_CTX_R16_R17   0x78
#define EVENT_CTX_R18_R19   0x80
#define EVENT_CTX_R20_R21   0x88
#define EVENT_CTX_R22_R23   0x90
#define EVENT_CTX_R24_R25   0x98
#define EVENT_CTX_R26_R27   0xA0
#define EVENT_CTX_GP        0xA8
#define EVENT_CTX_UGP       0xAC
#define EVENT_CTX_SIZE      0xB0  /* 8-byte aligned */
#else
#define EVENT_CTX_SIZE      0x78  /* 8-byte aligned */
#endif

GTEXT(z_hexagon_enhanced_machine_check)
GTEXT(z_hexagon_enhanced_exception)
GTEXT(z_hexagon_enhanced_trap0)
GTEXT(z_hexagon_enhanced_interrupt)
GTEXT(z_hexagon_event_handler)
GTEXT(z_get_next_switch_handle)
GTEXT(z_hexagon_arch_switch)
#ifdef CONFIG_HW_STACK_PROTECTION
GTEXT(z_hexagon_event_exit_stack_protect)
#endif
#ifdef CONFIG_USERSPACE
GTEXT(z_hexagon_event_exit_user_sync)
#endif
GDATA(_kernel)

/* offsets.h will be generated and included during build */
#include <offsets_short.h>

/* Common event entry macro */
.macro EVENT_ENTRY event_num
	/* Allocate space for event context on the interrupted code's stack */
	allocframe(#EVENT_CTX_SIZE)

	/* Save r0:r1 first -- we need r0 as scratch */
	memd(r29+#EVENT_CTX_R0_R1) = r1:0

	/* Save GELR (g0 = return PC) */
	r0 = gelr
	memw(r29+#EVENT_CTX_GELR) = r0

	/* Save GSR (g1 = guest status at time of event).
	 * H2 has already disabled guest interrupts (GSR.IE=0), but does
	 * not touch UM (bit 31): GSR.UM is a software-defined semantic H2
	 * just leaves as it was in the interrupted context. A trap0 taken
	 * from a user thread would otherwise run this whole handler with
	 * UM still set to 1, so kernel data reachable only through the
	 * U=0 fixed-tail RAM entries (see hexagon_fixed_tail_init()) would
	 * fault. Clear UM here so the handler body runs at guest
	 * privilege like every other RTOS's trap handler; EVENT_EXIT
	 * restores the original saved value below before vmrte, which
	 * re-applies the interrupted thread's real UM on return.
	 */
	r0 = gsr
	memw(r29+#EVENT_CTX_GSR) = r0
	r0 = clrbit(r0, #31)
	gsr = r0

	/* Save remaining volatile registers */
	{
		memd(r29+#EVENT_CTX_R2_R3) = r3:2
		memd(r29+#EVENT_CTX_R4_R5) = r5:4
	}
	{
		memd(r29+#EVENT_CTX_R6_R7) = r7:6
		memd(r29+#EVENT_CTX_R8_R9) = r9:8
	}
	{
		memd(r29+#EVENT_CTX_R10_R11) = r11:10
		memd(r29+#EVENT_CTX_R12_R13) = r13:12
	}
	{
		memd(r29+#EVENT_CTX_R14_R15) = r15:14
	}

	/* Save predicate registers */
	r0 = p3:0
	memw(r29+#EVENT_CTX_PRED) = r0

	/* Save link register */
	memw(r29+#EVENT_CTX_LR) = r31

	/* Save r28 (caller-saved, not part of r0-r15 register pairs) */
	memw(r29+#EVENT_CTX_R28) = r28

	/* Save hardware loop, modifier, and USR registers.
	 * These are caller-saved and will be clobbered by the C handler.
	 */
	r0 = sa0
	memw(r29+#EVENT_CTX_SA0) = r0
	r0 = lc0
	memw(r29+#EVENT_CTX_LC0) = r0
	r0 = sa1
	memw(r29+#EVENT_CTX_SA1) = r0
	r0 = lc1
	memw(r29+#EVENT_CTX_LC1) = r0
	r0 = m0
	memw(r29+#EVENT_CTX_M0) = r0
	r0 = m1
	memw(r29+#EVENT_CTX_M1) = r0
	r0 = usr
	memw(r29+#EVENT_CTX_USR) = r0

#if defined(CONFIG_GDBSTUB) || defined(CONFIG_DEBUG_COREDUMP)
	/* Save callee-saved GPRs and GP/UGP before C code can clobber them. */
	{
		memd(r29+#EVENT_CTX_R16_R17) = r17:16
		memd(r29+#EVENT_CTX_R18_R19) = r19:18
	}
	{
		memd(r29+#EVENT_CTX_R20_R21) = r21:20
		memd(r29+#EVENT_CTX_R22_R23) = r23:22
	}
	{
		memd(r29+#EVENT_CTX_R24_R25) = r25:24
		memd(r29+#EVENT_CTX_R26_R27) = r27:26
	}
	r0 = gp
	memw(r29+#EVENT_CTX_GP) = r0
	r0 = ugp
	memw(r29+#EVENT_CTX_UGP) = r0
#endif

	/* Call C handler with event number and saved context */
	{
		r0 = #\event_num
		r1 = r29
	}
	{
		call ##z_hexagon_event_handler
	}
.endm

/* Common event exit macro */
.macro EVENT_EXIT event_id
#ifdef CONFIG_MULTITHREADING
	/*
	 * Check whether the scheduler wants to preempt.
	 *
	 * Only volatile (caller-saved) registers r0-r15 are used here.
	 * EVENT_ENTRY already saved r0-r15, so clobbering them is safe.
	 * We must NOT touch callee-saved r16-r27 because EVENT_ENTRY
	 * does not save them, and the interrupted code expects them
	 * to be preserved.
	 */

	/* r4 = &_kernel.cpus[0] */
	r4 = ##_kernel
	{
		r4 = add(r4, #___kernel_t_cpus_OFFSET)
	}

	/* r5 = _current_cpu->current (old thread pointer) */
	{
		r5 = memw(r4+#___cpu_t_current_OFFSET)
	}

	/* Save old thread pointer to scratch slot (r5 is caller-saved
	 * and will be clobbered by the function call below).
	 */
	{
		memw(r29+#EVENT_CTX_SCRATCH) = r5
	}

	/* Call z_get_next_switch_handle(NULL) - following RISC-V/ARM64 pattern */
	{
		r0 = #0   /* interrupted = NULL */
		call ##z_get_next_switch_handle
	}

	/* Check if we need to switch: if (r0 == 0) no switch */
	{
		p0 = cmp.eq(r0, #0)
		if (p0.new) jump:t ##_event_no_switch_\event_id
	}

	/* Reload old thread pointer from scratch slot */
	{
		r5 = memw(r29+#EVENT_CTX_SCRATCH)
	}

	/* r0 = new thread's switch_handle, r5 = old thread pointer */
	/* arch_switch needs &thread->switch_handle as second arg */
	{
		r1 = add(r5, #___thread_t_switch_handle_OFFSET)
		call ##z_hexagon_arch_switch
	}

#ifdef CONFIG_HW_STACK_PROTECTION
	/* Update FRAMELIMIT for the newly scheduled thread */
	{
		call ##z_hexagon_event_exit_stack_protect
	}
#endif

_event_no_switch_\event_id:
#endif /* CONFIG_MULTITHREADING */

#ifdef CONFIG_USERSPACE
	/*
	 * _current now points to the thread that will actually resume; sync
	 * _hexagon_user_mode_active for it before the restore below returns.
	 */
	{
		call ##z_hexagon_event_exit_user_sync
	}

	/* Reinstall this thread's hardware memory-domain map, if needed. */
	{
		call ##z_hexagon_mem_domain_switch
	}
#endif

_event_restore_\event_id:
	/* Restore r28 */
	r28 = memw(r29+#EVENT_CTX_R28)

	/* Restore hardware loop, modifier, and USR registers */
	r0 = memw(r29+#EVENT_CTX_SA0)
	sa0 = r0
	r0 = memw(r29+#EVENT_CTX_LC0)
	lc0 = r0
	r0 = memw(r29+#EVENT_CTX_SA1)
	sa1 = r0
	r0 = memw(r29+#EVENT_CTX_LC1)
	lc1 = r0
	r0 = memw(r29+#EVENT_CTX_M0)
	m0 = r0
	r0 = memw(r29+#EVENT_CTX_M1)
	m1 = r0
	r0 = memw(r29+#EVENT_CTX_USR)
	usr = r0

#if defined(CONFIG_GDBSTUB) || defined(CONFIG_DEBUG_COREDUMP)
	/* Restore callee-saved GPRs and GP/UGP. */
	{
		r17:16 = memd(r29+#EVENT_CTX_R16_R17)
		r19:18 = memd(r29+#EVENT_CTX_R18_R19)
	}
	{
		r21:20 = memd(r29+#EVENT_CTX_R20_R21)
		r23:22 = memd(r29+#EVENT_CTX_R22_R23)
	}
	{
		r25:24 = memd(r29+#EVENT_CTX_R24_R25)
		r27:26 = memd(r29+#EVENT_CTX_R26_R27)
	}
	r0 = memw(r29+#EVENT_CTX_GP)
	gp = r0
	r0 = memw(r29+#EVENT_CTX_UGP)
	ugp = r0
#endif

	/* Restore link register */
	{
		r31 = memw(r29+#EVENT_CTX_LR)
	}

	/* Restore predicate registers */
	r0 = memw(r29+#EVENT_CTX_PRED)
	p3:0 = r0

	/* Restore volatile registers (except r0:r1 -- still needed as scratch) */
	{
		r15:14 = memd(r29+#EVENT_CTX_R14_R15)
		r13:12 = memd(r29+#EVENT_CTX_R12_R13)
	}
	{
		r11:10 = memd(r29+#EVENT_CTX_R10_R11)
		r9:8 = memd(r29+#EVENT_CTX_R8_R9)
	}
	{
		r7:6 = memd(r29+#EVENT_CTX_R6_R7)
		r5:4 = memd(r29+#EVENT_CTX_R4_R5)
	}
	{
		r3:2 = memd(r29+#EVENT_CTX_R2_R3)
	}

	/* Set GELR to saved return PC */
	r0 = memw(r29+#EVENT_CTX_GELR)
	gelr = r0

	/* Set GSR to saved guest status (restores IE state) */
	r0 = memw(r29+#EVENT_CTX_GSR)
	gsr = r0

	/* Restore r0:r1 last */
	{
		r1:0 = memd(r29+#EVENT_CTX_R0_R1)
	}

	/* Pop the event frame */
	{
		deallocframe
	}

	/* Return to interrupted context via HVM vmrte */
	{
		trap1(#HEXAGON_VM_vmrte)
	}
.endm

/* Enhanced machine check handler */
SECTION_FUNC(TEXT, z_hexagon_enhanced_machine_check)
	EVENT_ENTRY 1
	EVENT_EXIT 1

/* Enhanced general exception handler */
SECTION_FUNC(TEXT, z_hexagon_enhanced_exception)
	EVENT_ENTRY 2
	EVENT_EXIT 2

/* Enhanced trap0 (syscall) handler */
SECTION_FUNC(TEXT, z_hexagon_enhanced_trap0)
	EVENT_ENTRY 5
	EVENT_EXIT 5

/* Enhanced interrupt handler */
SECTION_FUNC(TEXT, z_hexagon_enhanced_interrupt)
	EVENT_ENTRY 7
	EVENT_EXIT 7
