/* Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include <xtensa/corebits.h>
#include <xtensa/config/core-isa.h>
#include <syscall_list.h>
#include <zsr.h>
#include <offsets.h>
#include <syscall_list.h>

/* Saves the "odd" thread context registers (thread state not captured
 * by GPRs) to a context struct
 */
.macro ODD_REG_SAVE val_reg ctx_reg
	rsr \val_reg, SAR
	s32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_sar_OFFSET
#if XCHAL_HAVE_LOOPS
	rsr \val_reg, LBEG
	s32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_lbeg_OFFSET
	rsr \val_reg, LEND
	s32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_lend_OFFSET
	rsr \val_reg, LCOUNT
	s32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_lcount_OFFSET
#endif
#if XCHAL_HAVE_S32C1I
	rsr \val_reg, SCOMPARE1
	s32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_scompare1_OFFSET
#endif
#if XCHAL_HAVE_THREADPTR
	rur \val_reg, THREADPTR
	s32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_threadptr_OFFSET
#endif
.endm

/* Similarly, restores the "odd" registers from a thread context */
.macro ODD_REG_RESTORE val_reg ctx_reg
	l32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_sar_OFFSET
	wsr \val_reg, SAR
#if XCHAL_HAVE_LOOPS
	l32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_lbeg_OFFSET
	wsr \val_reg, LBEG
	l32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_lend_OFFSET
	wsr \val_reg, LEND
	l32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_lcount_OFFSET
	wsr \val_reg, LCOUNT
#endif
#if XCHAL_HAVE_S32C1I
	l32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_scompare1_OFFSET
	wsr \val_reg, SCOMPARE1
#endif
#if XCHAL_HAVE_THREADPTR
	l32i \val_reg, \ctx_reg, __xtensa_win0_ctx_t_threadptr_OFFSET
	wur \val_reg, THREADPTR
#endif
.endm

/* Inc/decrement nested interrupt count on the CPU struct */
.macro MOD_NEST_COUNT val cpu_reg scratch_reg
	l32i \scratch_reg, \cpu_reg, ___cpu_t_nested_OFFSET
	addi \scratch_reg, \scratch_reg, \val
	s32i \scratch_reg, \cpu_reg, ___cpu_t_nested_OFFSET
.endm

/* On kernel entry, sets RING to zero (enabling kernel memory access)
 * and clears EXCM (enabling further nested exceptions and interrupts)
 */
.macro PS_ENTRY_UNMASK ps_reg val_reg
	rsr \ps_reg, PS
	movi \val_reg, ~(PS_EXCM_MASK | PS_RING_MASK)
	and \ps_reg, \ps_reg, \val_reg
	wsr \ps_reg, PS
.endm

/* Vector handlers: the vector sections themselves are very small (~10
 * instructions on some devices!), so they do minimal work.  Their job:
 *
 * + Place the interrupted a0 register into ZSR_A0SAVE
 * + Place a C handler address corresponding to the interrupt in a0
 * + Store the interrupted PC in ZSR_EPC
 * + Store the interrupted PS register value in ZSR_EPS
 * + Jump to the shared entry code
 *
 * Note that the User and Kernel Level 1 handlers must also check for
 * two special entry paths (so they do the bulk of their work in a
 * "l1_handler" section outside the vector region):
 *
 * + System calls have their own optimized handler
 * + MMU TLB exceptions are handled directly, as they must not
 *   re-enable interrupts at any point before returning to the trapping
 *   code.  (The handling is very simple: Zephyr does not use "lazy"
 *   VM regions, and it pins the VM area of the page table itself, so
 *   the only TLB trap that can happen is a missing entry for the page
 *   table data, which can be primed into the data TLB by simply
 *   loading it).
 */
.section .UserExceptionVector.text, "ax"
UserExceptionVector:
	wsr a0, ZSR_A0SAVE
	rsr a0, EXCCAUSE
	beqi a0, EXCCAUSE_SYSCALL, syscall_jump
	addi a0, a0, -(EXCCAUSE_ITLB_MISS) /* cursed immediate encoding for beqi... */
	beqi a0, (EXCCAUSE_DTLB_MISS - EXCCAUSE_ITLB_MISS), tlb_jump
	beqz a0, tlb_jump
	j l1_handler
tlb_jump:
	j tlb_miss
syscall_jump:
	j syscall_handler

.section .KernelExceptionVector.text, "ax"
KernelExceptionVector:
	wsr a0, ZSR_A0SAVE
	rsr a0, EXCCAUSE
	addi a0, a0, -(EXCCAUSE_ITLB_MISS) /* see above */
	beqi a0, (EXCCAUSE_DTLB_MISS - EXCCAUSE_ITLB_MISS), tlb_miss
	beqz a0, tlb_miss
	j l1_handler
tlb_miss:
#if XCHAL_HAVE_PTP_MMU
	rsr a0, PTEVADDR
	l32i a0, a0, 0 /* can double-fault, see below */
#endif
	rsr a0, ZSR_A0SAVE
	rfe

/* A double fault happens in Zephyr when a TLB miss is trapped loading
 * a PTE inside another TLB miss exception.  It simply means that the
 * CPU tried to address memory not in the page table, and we need to
 * treat it the same way we would if it trapped a "load/store or
 * execute prohibited" exception trying to read through a
 * mapped-but-invalid/unusable PTE.  The EPC register is already set
 * for us by the original exception, EXCCAUSE shows the double fault
 * (that can be interpreted in C code if needed, e.g. if we want to
 * implement fault-based lazy VM mapping), and like level 1 exceptions
 * there is no EPSx register (PS is unchanged from the intermediate
 * exception, and can be reset identically).
 */
.section .DoubleExceptionVector.text, "ax"
_DoubleExceptionVector:
	j KernelExceptionVector

/* Note: the specific level matching ZSR_RFI_LEVEL will end
 * up issuing four needless instructions to copy its EPS/EPC
 * registers back to themselves.  Should fix.
 */
.macro EMIT_VECTOR name, num
.section .\name\()InterruptVector.text, "ax"
\name\()InterruptVector:
	wsr a0, ZSR_A0SAVE
	rsr a0, EPC\num
	wsr a0, ZSR_EPC
	rsr a0, EPS\num
	wsr a0, ZSR_EPS
	movi a0, xtensa_int\num\()_c
	j int_handler
.endm

#if XCHAL_NUM_INTLEVELS >= 2
 EMIT_VECTOR Level2 2
#endif
#if XCHAL_NUM_INTLEVELS >= 3
 EMIT_VECTOR Level3 3
#endif
#if XCHAL_NUM_INTLEVELS >= 4
 EMIT_VECTOR Level4 4
#endif
#if XCHAL_NUM_INTLEVELS >= 5
 EMIT_VECTOR Level5 5
#endif
#if XCHAL_NUM_INTLEVELS >= 6
 EMIT_VECTOR Level6 6
#endif
#if XCHAL_NUM_INTLEVELS >= 7
 EMIT_VECTOR Level7 7
#endif
#if XCHAL_HAVE_DEBUG
 EMIT_VECTOR Debug XCHAL_DEBUGLEVEL
#endif
#if XCHAL_HAVE_NMI
 EMIT_VECTOR NMI XCHAL_NMILEVEL
#endif

/* The handler code below all lives in the "iram" section so it can be
 * contiguous with the vectors (important for MMU configurations so
 * the vector mapping can be pinned in the TLB)
 */
.section .iram.text, "ax"

/* Slightly special entry to level 1 interrupts (from the
 * User/KernelExceptionVectors): There is no "EPS1" register, yet we
 * restore interrupted threads with the RFI instruction which needs an
 * interrupted PS.  So we consruct one from the current PS by clearing
 * EXCM.  Also some platforms (IIRC esp32 and qemu) fail to set
 * INTLEVEL correctly in all cases (leaving it at zero), which causes
 * a recursive interrupt when EXCM gets unmasked, so fix that.
 *
 * Note: the rsil will break masking (it REDUCES the interrupt level)
 * if we're handling an exception raised inside a level 2+ interrupt!
 * Maybe that's benign enough?  It does mean we'll probably never get
 * e.g. telemetry from panics inside medium priority interrupts.
 */
l1_handler:
	rsr a0, EPC1
	wsr a0, ZSR_EPC
	rsil a0, 1                   /* Forces INTLEVEL to 1 if hardware didn't */
	addi a0, a0, -(PS_EXCM_MASK) /* one bit field means we can subtract */
	wsr a0, ZSR_EPS
	movi a0, xtensa_excint1_c
	/* fall through to int_handler below */

/* General interrupt handler.  Arrive here via a jump from the vector
 * with the interrupted a0 register in ZSR_A0SAVE, the interrupted
 * PS/PC in ZSR_EPS/EPC, and a handler address in a0
 */
int_handler:
	wsr a0, EPC1 /* stash handler (we know EPC1 is free for scratch) */
	rsr a0, ZSR_CPU
	l32i a0, a0, ___cpu_t_nested_OFFSET
	bnez a0, nested_handler

	/* Not nested, so we can just rotate the register set.  Note
	 * that this happens in two hops so we can sneak the saved a0
	 * register back into its spot before it disappears.
	 */
	rotw -3
	rsr a12, ZSR_A0SAVE /* a12 = register formerly known as a0 */
	rotw -1

	/* Bump the nest count and clear the remaining scratch SR's.
	 * Now we're safe to interrupt, so we can clear EXCM.  Note
	 * that a12-15 are preserved across the call, so that's where
	 * we store the cpu/thread/context pointers.
	 */
	rsr a12, ZSR_CPU
	MOD_NEST_COUNT 1 a12, a0
	l32i a13, a12, ___cpu_t_current_OFFSET
	addi a14, a13, ___thread_t_arch_OFFSET
	rsr a0, ZSR_EPS
	s32i a0, a14, __xtensa_win0_ctx_t_ps_OFFSET
	rsr a0, ZSR_EPC
	s32i a0, a14, __xtensa_win0_ctx_t_pc_OFFSET
	rsr a0, EPC1
	l32i a1, a12, ___cpu_t_irq_stack_OFFSET
	PS_ENTRY_UNMASK a2 a3
	ODD_REG_SAVE a2, a14 /* These are interrupt-safe to save outside lock */

	/* Call handler, check return value for needed context switch */
	mov a2, a14
	callx0 a0
	MOD_NEST_COUNT -1 a12 a0
	bnez a2, must_switch

	/* fast path resume to same context */
	ODD_REG_RESTORE a0, a14
	l32i a4, a14, __xtensa_win0_ctx_t_ps_OFFSET
	l32i a5, a14, __xtensa_win0_ctx_t_pc_OFFSET
	rsil a0, 0xf
	wsr a4, ZSR_EPS
	wsr a5, ZSR_EPC
	rotw 4
	rfi ZSR_RFI_LEVEL

must_switch:
	/* Rotate in two stages: first step has interrupted registers
	 * a0-a11 in a4-a15, and we use a1-a3 (a13-a15 before the
	 * first rotw) to hold: old thread, interrupted context, new
	 * context.  Final step frog-hops our three working registers
	 * and finishes saving a12-15.
	 */
	mov a15, a2 /* Restore context goes to a3 after rotate */
	rotw 3
	s32i a4, a2, __xtensa_win0_ctx_t_a0_OFFSET
	s32i a5, a2, __xtensa_win0_ctx_t_a1_OFFSET
	s32i a6, a2, __xtensa_win0_ctx_t_a2_OFFSET
	s32i a7, a2, __xtensa_win0_ctx_t_a3_OFFSET
	s32i a8, a2, __xtensa_win0_ctx_t_a4_OFFSET
	s32i a9, a2, __xtensa_win0_ctx_t_a5_OFFSET
	s32i a10, a2, __xtensa_win0_ctx_t_a6_OFFSET
	s32i a11, a2, __xtensa_win0_ctx_t_a7_OFFSET
	s32i a12, a2, __xtensa_win0_ctx_t_a8_OFFSET
	s32i a13, a2, __xtensa_win0_ctx_t_a9_OFFSET
	s32i a14, a2, __xtensa_win0_ctx_t_a10_OFFSET
	s32i a15, a2, __xtensa_win0_ctx_t_a11_OFFSET
	mov a5, a1  /* outgoing thread stays in a1 */
	mov a6, a2  /* old context stays in a2 */
	mov a7, a3  /* new context stays in a3 for restore */
	rotw 1
	s32i a12, a2, __xtensa_win0_ctx_t_a12_OFFSET
	s32i a13, a2, __xtensa_win0_ctx_t_a13_OFFSET
	s32i a14, a2, __xtensa_win0_ctx_t_a14_OFFSET
	s32i a15, a2, __xtensa_win0_ctx_t_a15_OFFSET

	/* Set the switch_handle field LAST */
	s32i a2, a1, ___thread_t_switch_handle_OFFSET
	/* fall through to restore: */

/* Full context restore.  Pointer to context is in a3.  Interrupts
 * must be masked
 */
restore:
	l32i a0, a3, __xtensa_win0_ctx_t_ps_OFFSET
	wsr a0, ZSR_EPS
	l32i a0, a3, __xtensa_win0_ctx_t_pc_OFFSET
	wsr a0, ZSR_EPC
	ODD_REG_RESTORE a0, a3
	l32i a0, a3, __xtensa_win0_ctx_t_a0_OFFSET
	l32i a1, a3, __xtensa_win0_ctx_t_a1_OFFSET
	l32i a2, a3, __xtensa_win0_ctx_t_a2_OFFSET
	l32i a4, a3, __xtensa_win0_ctx_t_a4_OFFSET
	l32i a5, a3, __xtensa_win0_ctx_t_a5_OFFSET
	l32i a6, a3, __xtensa_win0_ctx_t_a6_OFFSET
	l32i a7, a3, __xtensa_win0_ctx_t_a7_OFFSET
	l32i a8, a3, __xtensa_win0_ctx_t_a8_OFFSET
	l32i a9, a3, __xtensa_win0_ctx_t_a9_OFFSET
	l32i a10, a3, __xtensa_win0_ctx_t_a10_OFFSET
	l32i a11, a3, __xtensa_win0_ctx_t_a11_OFFSET
	l32i a12, a3, __xtensa_win0_ctx_t_a12_OFFSET
	l32i a13, a3, __xtensa_win0_ctx_t_a13_OFFSET
	l32i a14, a3, __xtensa_win0_ctx_t_a14_OFFSET
	l32i a15, a3, __xtensa_win0_ctx_t_a15_OFFSET
	l32i a3, a3, __xtensa_win0_ctx_t_a3_OFFSET
	rfi ZSR_RFI_LEVEL

nested_handler:
	/* Push a context on the stack, spill a0-a3 */
	addi a0, a1, -(__xtensa_win0_ctx_t_SIZEOF)
	s32i a1, a0, __xtensa_win0_ctx_t_a1_OFFSET
	rsr a1, ZSR_A0SAVE
	s32i a1, a0, __xtensa_win0_ctx_t_a0_OFFSET
	s32i a2, a0, __xtensa_win0_ctx_t_a2_OFFSET
	s32i a3, a0, __xtensa_win0_ctx_t_a3_OFFSET
	mov a3, a0

	/* Save interrupt-sensitive state and unmask */
	rsr a1, ZSR_CPU
	MOD_NEST_COUNT 1 a1 a2
	rsr a2, ZSR_EPS
	s32i a2, a3, __xtensa_win0_ctx_t_ps_OFFSET
	rsr a2, ZSR_EPC
	s32i a2, a3, __xtensa_win0_ctx_t_pc_OFFSET
	rsr a0, EPC1 /* C handler address, stashed in int_handler */
	mov a1, a3 /* new sp */
	PS_ENTRY_UNMASK a2, a3
	ODD_REG_SAVE a2, a1

	/* Finish spilling. Note: the ABI call will preserve a12-15,
	 * so we could elide spilling those if we were willing to
	 * write custom restore code after the handler.
	 */
	s32i a4, a1, __xtensa_win0_ctx_t_a4_OFFSET
	s32i a5, a1, __xtensa_win0_ctx_t_a5_OFFSET
	s32i a6, a1, __xtensa_win0_ctx_t_a6_OFFSET
	s32i a7, a1, __xtensa_win0_ctx_t_a7_OFFSET
	s32i a8, a1, __xtensa_win0_ctx_t_a8_OFFSET
	s32i a9, a1, __xtensa_win0_ctx_t_a9_OFFSET
	s32i a10, a1, __xtensa_win0_ctx_t_a10_OFFSET
	s32i a11, a1, __xtensa_win0_ctx_t_a11_OFFSET
	s32i a12, a1, __xtensa_win0_ctx_t_a12_OFFSET
	s32i a13, a1, __xtensa_win0_ctx_t_a13_OFFSET
	s32i a14, a1, __xtensa_win0_ctx_t_a14_OFFSET
	s32i a15, a1, __xtensa_win0_ctx_t_a15_OFFSET

	mov a2, a1 /* context is argument */
	callx0 a0
	rsil a0, 0xf
	rsr a2, ZSR_CPU
	MOD_NEST_COUNT -1 a2 a0
	mov a3, a1
	j restore

/* System call exceptions are an extremely pruned case of general
 * exception handling:
 *
 * + We know we were a trap from a userspace thread context, and don't
 *   need to check for nested interrupts or entry from kernel threads
 * + We don't need to do a full context switch: userspace knew it was
 *   trapping and will have pre-spilled state we don't need to save.
 *   (In practice we preserve a0-1 and a12-15, and leave a return value
 *   in a2)
 * + Likewise we know userspace isn't in the middle of code using
 *   compiler-generated state registers like
 *   SAR/SCOMPARE/loops/etc... (The compiler won't emit a LOOP
 *   construction if it can't prove there is no nesting, so a simple
 *   __asm__ block defeats it)
 * + The parameter registers are the same as the C ABI, so we can
 *   call the C handlers directly out of the table.  This will
 *   preserve a12-15 for us, so we don't have to spill them.
 *
 * With a few special conventions and rules:
 *
 * + The syscall number itself is passed in a11 and we use a10 as
 *   scratch (allows space for 8 parameters in a2-a9)
 * + We stash the userspace a0/a1/EPC in a special location in the
 *   thread struct, as the thread's own context now belongs to this
 *   syscall (which can itself be switched out!)
 * + We must switch to the thread's priviledged stack
 * + The nine unused-at-exit registers (a3-a11) must be wiped on exit
 *   such that they don't contain stale kernel data.
 * + We still have to unmask EXCM, set RING to zero, and fix INTLEVEL
 *   due to a bug on some platforms that leaves it at zero incorrectly
 * + And we return with RFI, not RFE, because we have to set a PS with
 *   RING re-elevated
 */
syscall_handler:
	rsr a10, ZSR_CPU
	l32i a10, a10, ___cpu_t_current_OFFSET
	rsr a0, ZSR_A0SAVE
	s32i a0, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_a0_OFFSET
	s32i a1, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_a1_OFFSET
	rsr a0, EPC1
	addi a0, a0, 3 /* skip over syscall instruction */
	s32i a0, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_pc_OFFSET
	rsr a0, PS
	addi a0, a0, -(PS_EXCM_MASK)
	s32i a0, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_ps_OFFSET

	/* Further cook PS to set RING=0 and UM=0, then set our sp and unmask */
	movi a1, ~(PS_RING_MASK | PS_UM_MASK)
	and a0, a0, a1
#ifdef CONFIG_USERSPACE
	l32i a1, a10, ___thread_t_arch_OFFSET + ___thread_arch_t_priv_sp_OFFSET
#else
	movi a1, _mock_priv_stack
	l32i a1, a1, 0
#endif
	wsr a0, PS

	/* Extract handler pointer and call it */
	movi a0, K_SYSCALL_LIMIT
	bltu a11, a0, 1f
	mov a2, a11 /* first parameter to bad_syscall() is syscall id */
	movi a11, K_SYSCALL_BAD
1:
	movi a10, _k_syscall_table
	add a11, a11, a11 /* Turn syscall index... */
	add a11, a11, a11 /* ...into a word offset */
	add a10, a10, a11
	l32i a10, a10, 0
	callx0 a10

	movi a4, 0 /* Clear caller-save registers that might still hold kernel */
	movi a5, 0 /*  data.  Might consider eliding this based on a kconfig:  */
	movi a6, 0 /*  not all userspace threat models involve data security   */
	movi a7, 0
	movi a8, 0
	movi a9, 0

	rsil a11, 0xf /* lock before getting CPU pointer, we can migrate! */
	rsr a10, ZSR_CPU
	l32i a10, a10, ___cpu_t_current_OFFSET
	l32i a0, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_a0_OFFSET
	l32i a1, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_a1_OFFSET
	l32i a11, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_ps_OFFSET
	wsr a11, ZSR_EPS
	l32i a11, a10, ___thread_t_arch_OFFSET + __xtensa_win0_ctx_t_user_pc_OFFSET
	wsr a11, ZSR_EPC
	rsync
	rfi ZSR_RFI_LEVEL

/* This is a standard CALL0 ABI function: new switch handle is in a2,
 * pointer to store the outgoing one is in a3 (which is the old
 * thread's switch_handle field from which we recover the context
 * pointer), PC to which to return is in a0.  Only need to save state
 * in use (not much: return address, sp, callee-save registers)
 */
.global xtensa_switch
xtensa_switch:
	addi a4, a3, ___thread_t_arch_OFFSET - ___thread_t_switch_handle_OFFSET
	s32i a0, a4, __xtensa_win0_ctx_t_pc_OFFSET /* return addr = restore PC */
	s32i a1, a4, __xtensa_win0_ctx_t_a1_OFFSET
	s32i a12, a4, __xtensa_win0_ctx_t_a12_OFFSET
	s32i a13, a4, __xtensa_win0_ctx_t_a13_OFFSET
	s32i a14, a4, __xtensa_win0_ctx_t_a14_OFFSET
	s32i a15, a4, __xtensa_win0_ctx_t_a15_OFFSET
	rsr a5, PS
	s32i a5, a4, __xtensa_win0_ctx_t_ps_OFFSET
	s32i a4, a3, 0 /* store switch handle */
	mov a3, a2
	j restore

/* This file also contains the system call stubs, though those are
 * used only by ring 3 code and live in the regular .text section.
 * The system calls work just like CALL0 ABI functions, with arguments
 * marshalled for us by the compiler.  All we need to do is put the ID
 * number in A11.
 */
.section .text.asm_syscall_stubs, "ax"

.global xtensa_syscall6
xtensa_syscall6:
	mov a11, a8
	syscall
	ret

.global xtensa_syscall5
xtensa_syscall5:
	mov a11, a7
	syscall
	ret

.global xtensa_syscall4
xtensa_syscall4:
	mov a11, a6
	syscall
	ret

.global xtensa_syscall3
xtensa_syscall3:
	mov a11, a5
	syscall
	ret

.global xtensa_syscall2
xtensa_syscall2:
	mov a11, a4
	syscall
	ret

.global xtensa_syscall1
xtensa_syscall1:
	mov a11, a3
	syscall
	ret
