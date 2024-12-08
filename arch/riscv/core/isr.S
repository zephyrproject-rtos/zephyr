/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/syscall.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/arch/riscv/syscall.h>
#include "asm_macros.inc"

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
#include <soc_isr_stacking.h>
#endif

/* Convenience macro for loading/storing register states. */
#define DO_CALLER_SAVED(op) \
	RV_E(	op t0, __struct_arch_esf_t0_OFFSET(sp)	);\
	RV_E(	op t1, __struct_arch_esf_t1_OFFSET(sp)	);\
	RV_E(	op t2, __struct_arch_esf_t2_OFFSET(sp)	);\
	RV_I(	op t3, __struct_arch_esf_t3_OFFSET(sp)	);\
	RV_I(	op t4, __struct_arch_esf_t4_OFFSET(sp)	);\
	RV_I(	op t5, __struct_arch_esf_t5_OFFSET(sp)	);\
	RV_I(	op t6, __struct_arch_esf_t6_OFFSET(sp)	);\
	RV_E(	op a0, __struct_arch_esf_a0_OFFSET(sp)	);\
	RV_E(	op a1, __struct_arch_esf_a1_OFFSET(sp)	);\
	RV_E(	op a2, __struct_arch_esf_a2_OFFSET(sp)	);\
	RV_E(	op a3, __struct_arch_esf_a3_OFFSET(sp)	);\
	RV_E(	op a4, __struct_arch_esf_a4_OFFSET(sp)	);\
	RV_E(	op a5, __struct_arch_esf_a5_OFFSET(sp)	);\
	RV_I(	op a6, __struct_arch_esf_a6_OFFSET(sp)	);\
	RV_I(	op a7, __struct_arch_esf_a7_OFFSET(sp)	);\
	RV_E(	op ra, __struct_arch_esf_ra_OFFSET(sp)	)

#ifdef CONFIG_EXCEPTION_DEBUG
/* Convenience macro for storing callee saved register [s0 - s11] states. */
#define STORE_CALLEE_SAVED() \
	RV_E(	sr s0, ___callee_saved_t_s0_OFFSET(sp)		);\
	RV_E(	sr s1, ___callee_saved_t_s1_OFFSET(sp)		);\
	RV_I(	sr s2, ___callee_saved_t_s2_OFFSET(sp)		);\
	RV_I(	sr s3, ___callee_saved_t_s3_OFFSET(sp)		);\
	RV_I(	sr s4, ___callee_saved_t_s4_OFFSET(sp)		);\
	RV_I(	sr s5, ___callee_saved_t_s5_OFFSET(sp)		);\
	RV_I(	sr s6, ___callee_saved_t_s6_OFFSET(sp)		);\
	RV_I(	sr s7, ___callee_saved_t_s7_OFFSET(sp)		);\
	RV_I(	sr s8, ___callee_saved_t_s8_OFFSET(sp)		);\
	RV_I(	sr s9, ___callee_saved_t_s9_OFFSET(sp)		);\
	RV_I(	sr s10, ___callee_saved_t_s10_OFFSET(sp)	);\
	RV_I(	sr s11, ___callee_saved_t_s11_OFFSET(sp)	)
#endif /* CONFIG_EXCEPTION_DEBUG */

	.macro get_current_cpu dst
#if defined(CONFIG_SMP) || defined(CONFIG_USERSPACE)
	csrr \dst, mscratch
#else
	la \dst, _kernel + ___kernel_t_cpus_OFFSET
#endif
	.endm

/* imports */
GDATA(_sw_isr_table)
#ifdef CONFIG_RISCV_SOC_EXCEPTION_FROM_IRQ
GTEXT(__soc_is_irq)
#endif
GTEXT(__soc_handle_irq)
GTEXT(_Fault)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#ifdef CONFIG_EXCEPTION_DEBUG
GTEXT(z_riscv_fatal_error_csf)
#else
GTEXT(z_riscv_fatal_error)
#endif /* CONFIG_EXCEPTION_DEBUG */

GTEXT(z_get_next_switch_handle)
GTEXT(z_riscv_switch)
GTEXT(z_riscv_thread_start)

#ifdef CONFIG_TRACING
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif

#ifdef CONFIG_USERSPACE
GDATA(_k_syscall_table)
#endif

#ifdef CONFIG_RISCV_SOC_HAS_CUSTOM_IRQ_HANDLING
GTEXT(__soc_handle_all_irqs)
#endif

/* exports */
GTEXT(_isr_wrapper)

/* use ABI name of registers for the sake of simplicity */

/*
 * Generic architecture-level IRQ handling, along with callouts to
 * SoC-specific routines.
 *
 * Architecture level IRQ handling includes basic context save/restore
 * of standard registers and calling ISRs registered at Zephyr's driver
 * level.
 *
 * Since RISC-V does not completely prescribe IRQ handling behavior,
 * implementations vary (some implementations also deviate from
 * what standard behavior is defined). Hence, the arch level code expects
 * the following functions to be provided at the SOC level:
 *
 *     - __soc_is_irq (optional): decide if we're handling an interrupt or an
         exception
 *     - __soc_handle_irq: handle SoC-specific details for a pending IRQ
 *       (e.g. clear a pending bit in a SoC-specific register)
 *
 * If CONFIG_RISCV_SOC_CONTEXT_SAVE=y, calls to SoC-level context save/restore
 * routines are also made here. For details, see the Kconfig help text.
 */

/*
 * Handler called upon each exception/interrupt/fault
 */
SECTION_FUNC(exception.entry, _isr_wrapper)

/* Provide requested alignment, which depends e.g. on MTVEC format */
.balign CONFIG_RISCV_TRAP_HANDLER_ALIGNMENT

#ifdef CONFIG_USERSPACE
	/* retrieve address of _current_cpu preserving s0 */
	csrrw s0, mscratch, s0

	/* preserve t0 and t1 temporarily */
	sr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	sr t1, _curr_cpu_arch_user_exc_tmp1(s0)

	/* determine if we come from user space */
	csrr t0, mstatus
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f

	/* in user space we were: switch to our privileged stack */
	mv t0, sp
	lr sp, _curr_cpu_arch_user_exc_sp(s0)

	/* Save user stack value. Coming from user space, we know this
	 * can't overflow the privileged stack. The esf will be allocated
	 * later but it is safe to store our saved user sp here. */
	sr t0, (-__struct_arch_esf_SIZEOF + __struct_arch_esf_sp_OFFSET)(sp)

	/* Make sure tls pointer is sane */
	lr t0, ___cpu_t_current_OFFSET(s0)
	lr tp, _thread_offset_to_tls(t0)

	/* Make sure global pointer is sane */
#ifdef CONFIG_RISCV_GP
	.option push
	.option norelax
	la gp, __global_pointer$
	.option pop
#elif defined(CONFIG_RISCV_CURRENT_VIA_GP)
	lr gp, ___cpu_t_current_OFFSET(s0)
#endif /* CONFIG_RISCV_GP / CONFIG_RISCV_CURRENT_VIA_GP */

	/* Clear our per-thread usermode flag */
	lui t0, %tprel_hi(is_user_mode)
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb zero, %tprel_lo(is_user_mode)(t0)
1:
	/* retrieve original t0/t1 values */
	lr t0, _curr_cpu_arch_user_exc_tmp0(s0)
	lr t1, _curr_cpu_arch_user_exc_tmp1(s0)

	/* retrieve original s0 and restore _current_cpu in mscratch */
	csrrw s0, mscratch, s0
#endif

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
	SOC_ISR_SW_STACKING
#else
	/* Save caller-saved registers on current thread stack. */
	addi sp, sp, -__struct_arch_esf_SIZEOF
	DO_CALLER_SAVED(sr)		;
#endif /* CONFIG_RISCV_SOC_HAS_ISR_STACKING */

	/* Save s0 in the esf and load it with &_current_cpu. */
	sr s0, __struct_arch_esf_s0_OFFSET(sp)
	get_current_cpu s0

	/* Save MEPC register */
	csrr t0, mepc
	sr t0, __struct_arch_esf_mepc_OFFSET(sp)

	/* Save MSTATUS register */
	csrr t2, mstatus
	sr t2, __struct_arch_esf_mstatus_OFFSET(sp)

#if defined(CONFIG_FPU_SHARING)
	/* determine if FPU access was disabled */
	li t1, MSTATUS_FS
	and t1, t1, t2
	bnez t1, no_fp
	/* determine if this is an Illegal Instruction exception */
	csrr t2, mcause
	li t1, CONFIG_RISCV_MCAUSE_EXCEPTION_MASK
	and t2, t2, t1
	li t1, 2		/* 2 = illegal instruction */
	bne t1, t2, no_fp
	/* determine if we trapped on an FP instruction. */
	csrr t2, mtval		/* get faulting instruction */
#ifdef CONFIG_QEMU_TARGET
	/*
	 * Some implementations may not support MTVAL in this capacity.
	 * Notably QEMU when a CSR instruction is involved.
	 */
	bnez t2, 1f
	lw t2, 0(t0)		/* t0 = mepc */
1:
#endif
	andi t0, t2, 0x7f	/* keep only the opcode bits */
	/*
	 * Major FP opcodes:
	 * 0000111 = LOAD-FP
	 * 0100111 = STORE-FP
	 * 1000011 = MADD
	 * 1000111 = MSUB
	 * 1001011 = NMSUB
	 * 1001111 = NMADD
	 * 1010011 = OP-FP
	 */
	xori t1, t0, 0b1010011	/* OP-FP */
	beqz t1, is_fp
	ori  t1, t0, 0b0100000
	xori t1, t1, 0b0100111	/* LOAD-FP / STORE-FP */
	beqz t1, is_fp
	ori  t1, t0, 0b0001100
	xori t1, t1, 0b1001111	/* MADD / MSUB / NMSUB / NMADD */
	beqz t1, is_fp
	/*
	 * The FRCSR, FSCSR, FRRM, FSRM, FSRMI, FRFLAGS, FSFLAGS and FSFLAGSI
	 * are in fact CSR instructions targeting the fcsr, frm and fflags
	 * registers. They should be caught as FPU instructions as well.
	 *
	 * CSR format: csr#[31-20] src[19-15] op[14-12] dst[11-7] SYSTEM[6-0]
	 * SYSTEM = 0b1110011, op = 0b.xx where xx is never 0
	 * The csr# of interest are: 1=fflags, 2=frm, 3=fcsr
	 */
	xori t1, t0, 0b1110011	/* SYSTEM opcode */
	bnez t1, 2f		/* not a CSR insn */
	srli t0, t2, 12
	andi t0, t0, 0x3
	beqz t0, 2f		/* not a CSR insn */
	srli t0, t2, 20		/* isolate the csr register number */
	beqz t0, 2f		/* 0=ustatus */
	andi t0, t0, ~0x3	/* 1=fflags, 2=frm, 3=fcsr */
#if !defined(CONFIG_RISCV_ISA_EXT_C)
	bnez t0, no_fp
#else
	beqz t0, is_fp
2:	/* remaining non RVC (0b11) and RVC with 0b01 are not FP instructions */
	andi t1, t2, 1
	bnez t1, no_fp
	/*
	 * 001...........00 = C.FLD    RV32/64  (RV128 = C.LQ)
	 * 001...........10 = C.FLDSP  RV32/64  (RV128 = C.LQSP)
	 * 011...........00 = C.FLW    RV32     (RV64/128 = C.LD)
	 * 011...........10 = C.FLWSPP RV32     (RV64/128 = C.LDSP)
	 * 101...........00 = C.FSD    RV32/64  (RV128 = C.SQ)
	 * 101...........10 = C.FSDSP  RV32/64  (RV128 = C.SQSP)
	 * 111...........00 = C.FSW    RV32     (RV64/128 = C.SD)
	 * 111...........10 = C.FSWSP  RV32     (RV64/128 = C.SDSP)
	 *
	 * so must be .01............. on RV64 and ..1............. on RV32.
	 */
	srli t0, t2, 8
#if defined(CONFIG_64BIT)
	andi t1, t0, 0b01100000
	xori t1, t1, 0b00100000
	bnez t1, no_fp
#else
	andi t1, t0, 0b00100000
	beqz t1, no_fp
#endif
#endif /* CONFIG_RISCV_ISA_EXT_C */

is_fp:	/* Process the FP trap and quickly return from exception */
	la ra, fp_trap_exit
	mv a0, sp
	tail z_riscv_fpu_trap
2:
no_fp:	/* increment arch_current_thread()->arch.exception_depth */
	lr t0, ___cpu_t_current_OFFSET(s0)
	lb t1, _thread_offset_to_exception_depth(t0)
	add t1, t1, 1
	sb t1, _thread_offset_to_exception_depth(t0)

	/* configure the FPU for exception mode */
	call z_riscv_fpu_enter_exc
#endif /* CONFIG_FPU_SHARING */

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
	addi a0, sp, __struct_arch_esf_soc_context_OFFSET
	jal ra, __soc_save_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/*
	 * Check if exception is the result of an interrupt or not.
	 * (SOC dependent). Following the RISC-V architecture spec, the MSB
	 * of the mcause register is used to indicate whether an exception
	 * is the result of an interrupt or an exception/fault. But for some
	 * SOCs (like pulpino or riscv-qemu), the MSB is never set to indicate
	 * interrupt. Hence, check for interrupt/exception via the __soc_is_irq
	 * function (that needs to be implemented by each SOC). The result is
	 * returned via register a0 (1: interrupt, 0 exception)
	 */
#ifdef CONFIG_RISCV_SOC_EXCEPTION_FROM_IRQ
	jal ra, __soc_is_irq
	bnez a0, is_interrupt
#else
	csrr t0, mcause
	srli t0, t0, RISCV_MCAUSE_IRQ_POS
	bnez t0, is_interrupt
#endif

	/*
	 * If the exception is the result of an ECALL, check whether to
	 * perform a context-switch or an IRQ offload. Otherwise call _Fault
	 * to report the exception.
	 */
	csrr t0, mcause
	li t2, CONFIG_RISCV_MCAUSE_EXCEPTION_MASK
	and t0, t0, t2

	/*
	 * If mcause == RISCV_EXC_ECALLM, handle system call from
	 * kernel thread.
	 */
	li t1, RISCV_EXC_ECALLM
	beq t0, t1, is_kernel_syscall

#ifdef CONFIG_USERSPACE
	/*
	 * If mcause == RISCV_EXC_ECALLU, handle system call
	 * for user mode thread.
	 */
	li t1, RISCV_EXC_ECALLU
	beq t0, t1, is_user_syscall

#if defined(CONFIG_PMP_STACK_GUARD) && defined(CONFIG_MULTITHREADING)
	/*
	 * Determine if we come from user space. If so, reconfigure the PMP for
	 * kernel mode stack guard.
	 */
	csrr t0, mstatus
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
1:
#endif /* CONFIG_PMP_STACK_GUARD */

#endif /* CONFIG_USERSPACE */

	/*
	 * Call _Fault to handle exception.
	 * Stack pointer is pointing to a struct_arch_esf structure, pass it
	 * to _Fault (via register a0).
	 * If _Fault shall return, set return address to
	 * no_reschedule to restore stack.
	 */
	mv a0, sp
	la ra, no_reschedule
	tail _Fault

is_kernel_syscall:
	/*
	 * A syscall is the result of an ecall instruction, in which case the
	 * MEPC will contain the address of the ecall instruction.
	 * Increment saved MEPC by 4 to prevent triggering the same ecall
	 * again upon exiting the ISR.
	 *
	 * It's safe to always increment by 4, even with compressed
	 * instructions, because the ecall instruction is always 4 bytes.
	 */
	lr t0, __struct_arch_esf_mepc_OFFSET(sp)
	addi t0, t0, 4
	sr t0, __struct_arch_esf_mepc_OFFSET(sp)

#if defined(CONFIG_PMP_STACK_GUARD) && defined(CONFIG_MULTITHREADING)
	/* Re-activate PMP for m-mode */
	li t1, MSTATUS_MPP
	csrc mstatus, t1
	li t1, MSTATUS_MPRV
	csrs mstatus, t1
#endif

	/* Determine what to do. Operation code is in t0. */
	lr t0, __struct_arch_esf_t0_OFFSET(sp)

	.if RV_ECALL_RUNTIME_EXCEPT != 0; .err; .endif
	beqz t0, do_fault

#if defined(CONFIG_IRQ_OFFLOAD)
	li t1, RV_ECALL_IRQ_OFFLOAD
	beq t0, t1, do_irq_offload
#endif

#ifdef CONFIG_RISCV_ALWAYS_SWITCH_THROUGH_ECALL
	li t1, RV_ECALL_SCHEDULE
	bne t0, t1, skip_schedule
	lr a0, __struct_arch_esf_a0_OFFSET(sp)
	lr a1, __struct_arch_esf_a1_OFFSET(sp)

#ifdef CONFIG_FPU_SHARING
	/*
	 * When an ECALL is used for a context-switch, the current thread has
	 * been updated to the next thread.
	 * Add the exception_depth back to the previous thread.
	 */
	lb t1, _thread_offset_to_exception_depth(a0)
	add t1, t1, -1
	sb t1, _thread_offset_to_exception_depth(a0)

	lb t1, _thread_offset_to_exception_depth(a1)
	add t1, t1, 1
	sb t1, _thread_offset_to_exception_depth(a1)
#endif

	j reschedule
skip_schedule:
#endif

	/* default fault code is K_ERR_KERNEL_OOPS */
	li a0, 3
	j 1f

do_fault:
	/* Handle RV_ECALL_RUNTIME_EXCEPT. Retrieve reason in a0, esf in A1. */
	lr a0, __struct_arch_esf_a0_OFFSET(sp)
1:	mv a1, sp

#ifdef CONFIG_EXCEPTION_DEBUG
	/*
	 * Restore the s0 we saved early in ISR entry
	 * so it shows up properly in the CSF.
	 */
	lr s0, __struct_arch_esf_s0_OFFSET(sp)

	/* Allocate space for caller-saved registers on current thread stack */
	addi sp, sp, -__callee_saved_t_SIZEOF

	/* Save callee-saved registers to be passed as 3rd arg */
	STORE_CALLEE_SAVED()		;
	mv a2, sp

#ifdef CONFIG_EXTRA_EXCEPTION_INFO
	/* Store csf's addr into esf (a1 still holds the pointer to the esf at this point) */
	sr a2 __struct_arch_esf_csf_OFFSET(a1)
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */

	tail z_riscv_fatal_error_csf
#else
	tail z_riscv_fatal_error
#endif /* CONFIG_EXCEPTION_DEBUG */

#if defined(CONFIG_IRQ_OFFLOAD)
do_irq_offload:
	/*
	 * Retrieve provided routine and argument from the stack.
	 * Routine pointer is in saved a0, argument in saved a1
	 * so we load them with a1/a0 (reversed).
	 */
	lr a1, __struct_arch_esf_a0_OFFSET(sp)
	lr a0, __struct_arch_esf_a1_OFFSET(sp)

	/* Increment _current_cpu->nested */
	lw t1, ___cpu_t_nested_OFFSET(s0)
	addi t2, t1, 1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t1, 1f

	/* Switch to interrupt stack */
	mv t0, sp
	lr sp, ___cpu_t_irq_stack_OFFSET(s0)

	/* Save thread stack pointer on interrupt stack */
	addi sp, sp, -16
	sr t0, 0(sp)
1:
	/* Execute provided routine (argument is in a0 already). */
	jalr ra, a1, 0

	/* Leave through the regular IRQ exit path */
	j irq_done
#endif /* CONFIG_IRQ_OFFLOAD */

#ifdef CONFIG_USERSPACE
is_user_syscall:

#if defined(CONFIG_PMP_STACK_GUARD) && defined(CONFIG_MULTITHREADING)
	/*
	 * We came from userspace and need to reconfigure the
	 * PMP for kernel mode stack guard.
	 */
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
#endif

	/* It is safe to re-enable IRQs now */
	csrs mstatus, MSTATUS_IEN

	/*
	 * Same as for is_kernel_syscall: increment saved MEPC by 4 to
	 * prevent triggering the same ecall again upon exiting the ISR.
	 */
	lr t1, __struct_arch_esf_mepc_OFFSET(sp)
	addi t1, t1, 4
	sr t1, __struct_arch_esf_mepc_OFFSET(sp)

	/* Restore argument registers from user stack */
	lr a0, __struct_arch_esf_a0_OFFSET(sp)
	lr a1, __struct_arch_esf_a1_OFFSET(sp)
	lr a2, __struct_arch_esf_a2_OFFSET(sp)
	lr a3, __struct_arch_esf_a3_OFFSET(sp)
	lr a4, __struct_arch_esf_a4_OFFSET(sp)
	lr a5, __struct_arch_esf_a5_OFFSET(sp)
	lr t0, __struct_arch_esf_t0_OFFSET(sp)
#if defined(CONFIG_RISCV_ISA_RV32E)
	/* Stack alignment for RV32E is 4 bytes */
	addi sp, sp, -4
	mv t1, sp
	sw t1, 0(sp)
#else
	mv a6, sp
#endif /* CONFIG_RISCV_ISA_RV32E */

	/* validate syscall limit */
	li t1, K_SYSCALL_LIMIT
	bltu t0, t1, valid_syscall_id

	/* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
	mv a0, t0
	li t0, K_SYSCALL_BAD

valid_syscall_id:

	la t2, _k_syscall_table

	slli t1, t0, RV_REGSHIFT	# Determine offset from indice value
	add t2, t2, t1			# Table addr + offset = function addr
	lr t2, 0(t2)			# Load function address

	/* Execute syscall function */
	jalr ra, t2, 0

#if defined(CONFIG_RISCV_ISA_RV32E)
	addi sp, sp, 4
#endif /* CONFIG_RISCV_ISA_RV32E */

	/* Update a0 (return value) on the stack */
	sr a0, __struct_arch_esf_a0_OFFSET(sp)

	/* Disable IRQs again before leaving */
	csrc mstatus, MSTATUS_IEN
	j might_have_rescheduled
#endif /* CONFIG_USERSPACE */

is_interrupt:

#if defined(CONFIG_PMP_STACK_GUARD) && defined(CONFIG_MULTITHREADING)
#ifdef CONFIG_USERSPACE
	/*
	 * If we came from userspace then we need to reconfigure the
	 * PMP for kernel mode stack guard.
	 */
	lr t0, __struct_arch_esf_mstatus_OFFSET(sp)
	li t1, MSTATUS_MPP
	and t0, t0, t1
	bnez t0, 1f
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_stackguard_enable
	j 2f
#endif /* CONFIG_USERSPACE */
1:	/* Re-activate PMP for m-mode */
	li t1, MSTATUS_MPP
	csrc mstatus, t1
	li t1, MSTATUS_MPRV
	csrs mstatus, t1
2:
#endif

	/* Increment _current_cpu->nested */
	lw t1, ___cpu_t_nested_OFFSET(s0)
	addi t2, t1, 1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t1, on_irq_stack

	/* Switch to interrupt stack */
	mv t0, sp
	lr sp, ___cpu_t_irq_stack_OFFSET(s0)

	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	addi sp, sp, -16
	sr t0, 0(sp)

on_irq_stack:

#ifdef CONFIG_RISCV_SOC_HAS_CUSTOM_IRQ_HANDLING
	call __soc_handle_all_irqs
#else

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_enter
#endif

	/* Get IRQ causing interrupt */
	csrr a0, mcause
	li t0, CONFIG_RISCV_MCAUSE_EXCEPTION_MASK
	and a0, a0, t0

	/*
	 * Clear pending IRQ generating the interrupt at SOC level
	 * Pass IRQ number to __soc_handle_irq via register a0
	 */
	jal ra, __soc_handle_irq

	/*
	 * Call corresponding registered function in _sw_isr_table.
	 * (table is 2-word wide, we should shift index accordingly)
	 */
	la t0, _sw_isr_table
	slli a0, a0, (RV_REGSHIFT + 1)
	add t0, t0, a0

	/* Load argument in a0 register */
	lr a0, 0(t0)

	/* Load ISR function address in register t1 */
	lr t1, RV_REGSIZE(t0)

	/* Call ISR function */
	jalr ra, t1, 0

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_exit
#endif

#endif

irq_done:
	/* Decrement _current_cpu->nested */
	lw t2, ___cpu_t_nested_OFFSET(s0)
	addi t2, t2, -1
	sw t2, ___cpu_t_nested_OFFSET(s0)
	bnez t2, no_reschedule

	/* nested count is back to 0: Return to thread stack */
	lr sp, 0(sp)

#ifdef CONFIG_STACK_SENTINEL
	call z_check_stack_sentinel
#endif

check_reschedule:

#ifdef CONFIG_MULTITHREADING

	/* Get pointer to current thread on this CPU */
	lr a1, ___cpu_t_current_OFFSET(s0)

	/*
	 * Get next thread to schedule with z_get_next_switch_handle().
	 * We pass it a NULL as we didn't save the whole thread context yet.
	 * If no scheduling is necessary then NULL will be returned.
	 */
	addi sp, sp, -16
	sr a1, 0(sp)
	mv a0, zero
	call z_get_next_switch_handle
	lr a1, 0(sp)
	addi sp, sp, 16
	beqz a0, no_reschedule

reschedule:

	/*
	 * Perform context switch:
	 * a0 = new thread
	 * a1 = old thread
	 */
	call z_riscv_switch

z_riscv_thread_start:
might_have_rescheduled:
	/* reload s0 with &_current_cpu as it might have changed or be unset */
	get_current_cpu s0

#endif /* CONFIG_MULTITHREADING */

no_reschedule:

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	addi a0, sp, __struct_arch_esf_soc_context_OFFSET
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#if defined(CONFIG_FPU_SHARING)
	/* FPU handling upon exception mode exit */
	mv a0, sp
	call z_riscv_fpu_exit_exc

	/* decrement arch_current_thread()->arch.exception_depth */
	lr t0, ___cpu_t_current_OFFSET(s0)
	lb t1, _thread_offset_to_exception_depth(t0)
	add t1, t1, -1
	sb t1, _thread_offset_to_exception_depth(t0)
fp_trap_exit:
#endif

	/* Restore MEPC and MSTATUS registers */
	lr t0, __struct_arch_esf_mepc_OFFSET(sp)
	lr t2, __struct_arch_esf_mstatus_OFFSET(sp)
	csrw mepc, t0
	csrw mstatus, t2

#ifdef CONFIG_USERSPACE
	/*
	 * Check if we are returning to user mode. If so then we must
	 * set is_user_mode to true and preserve our kernel mode stack for
	 * the next exception to come.
	 */
	li t1, MSTATUS_MPP
	and t0, t2, t1
	bnez t0, 1f

#if defined(CONFIG_PMP_STACK_GUARD) && defined(CONFIG_MULTITHREADING)
	/* Remove kernel stack guard and Reconfigure PMP for user mode */
	lr a0, ___cpu_t_current_OFFSET(s0)
	call z_riscv_pmp_usermode_enable
#endif

	/* Set our per-thread usermode flag */
	li t1, 1
	lui t0, %tprel_hi(is_user_mode)
	add t0, t0, tp, %tprel_add(is_user_mode)
	sb t1, %tprel_lo(is_user_mode)(t0)

	/* preserve stack pointer for next exception entry */
	add t0, sp, __struct_arch_esf_SIZEOF
	sr t0, _curr_cpu_arch_user_exc_sp(s0)

	j 2f
1:
	/*
	 * We are returning to kernel mode. Store the stack pointer to
	 * be re-loaded further down.
	 */
	addi t0, sp, __struct_arch_esf_SIZEOF
	sr t0, __struct_arch_esf_sp_OFFSET(sp)
2:
#endif

	/* Restore s0 (it is no longer ours) */
	lr s0, __struct_arch_esf_s0_OFFSET(sp)

#ifdef CONFIG_RISCV_SOC_HAS_ISR_STACKING
	SOC_ISR_SW_UNSTACKING
#else
	/* Restore caller-saved registers from thread stack */
	DO_CALLER_SAVED(lr)

#ifdef CONFIG_USERSPACE
	/* retrieve saved stack pointer */
	lr sp, __struct_arch_esf_sp_OFFSET(sp)
#else
	/* remove esf from the stack */
	addi sp, sp, __struct_arch_esf_SIZEOF
#endif

#endif /* CONFIG_RISCV_SOC_HAS_ISR_STACKING */

	mret
