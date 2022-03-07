/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <sys/util.h>
#include <kernel.h>
#include <syscall.h>
#include <arch/riscv/csr.h>

/* Convenience macros for loading/storing register states. */

#define DO_FP_CALLER_SAVED(op, reg) \
	op ft0, __z_arch_esf_t_ft0_OFFSET(reg)	 ;\
	op ft1, __z_arch_esf_t_ft1_OFFSET(reg)	 ;\
	op ft2, __z_arch_esf_t_ft2_OFFSET(reg)	 ;\
	op ft3, __z_arch_esf_t_ft3_OFFSET(reg)	 ;\
	op ft4, __z_arch_esf_t_ft4_OFFSET(reg)	 ;\
	op ft5, __z_arch_esf_t_ft5_OFFSET(reg)	 ;\
	op ft6, __z_arch_esf_t_ft6_OFFSET(reg)	 ;\
	op ft7, __z_arch_esf_t_ft7_OFFSET(reg)	 ;\
	op ft8, __z_arch_esf_t_ft8_OFFSET(reg)	 ;\
	op ft9, __z_arch_esf_t_ft9_OFFSET(reg)	 ;\
	op ft10, __z_arch_esf_t_ft10_OFFSET(reg) ;\
	op ft11, __z_arch_esf_t_ft11_OFFSET(reg) ;\
	op fa0, __z_arch_esf_t_fa0_OFFSET(reg)	 ;\
	op fa1, __z_arch_esf_t_fa1_OFFSET(reg)	 ;\
	op fa2, __z_arch_esf_t_fa2_OFFSET(reg)	 ;\
	op fa3, __z_arch_esf_t_fa3_OFFSET(reg)	 ;\
	op fa4, __z_arch_esf_t_fa4_OFFSET(reg)	 ;\
	op fa5, __z_arch_esf_t_fa5_OFFSET(reg)	 ;\
	op fa6, __z_arch_esf_t_fa6_OFFSET(reg)	 ;\
	op fa7, __z_arch_esf_t_fa7_OFFSET(reg)	 ;

#define STORE_FP_CALLER_SAVED(reg) \
	DO_FP_CALLER_SAVED(RV_OP_STOREFPREG, reg)

#define LOAD_FP_CALLER_SAVED(reg) \
	DO_FP_CALLER_SAVED(RV_OP_LOADFPREG, reg)

#define DO_FP_CALLEE_SAVED(op, reg) \
	op fs0, _thread_offset_to_fs0(reg)   ;\
	op fs1, _thread_offset_to_fs1(reg)   ;\
	op fs2, _thread_offset_to_fs2(reg)   ;\
	op fs3, _thread_offset_to_fs3(reg)   ;\
	op fs4, _thread_offset_to_fs4(reg)   ;\
	op fs5, _thread_offset_to_fs5(reg)   ;\
	op fs6, _thread_offset_to_fs6(reg)   ;\
	op fs7, _thread_offset_to_fs7(reg)   ;\
	op fs8, _thread_offset_to_fs8(reg)   ;\
	op fs9, _thread_offset_to_fs9(reg)   ;\
	op fs10, _thread_offset_to_fs10(reg) ;\
	op fs11, _thread_offset_to_fs11(reg) ;

#define STORE_FP_CALLEE_SAVED(reg) \
	frcsr t2				       ;\
	RV_OP_STOREREG t2, _thread_offset_to_fcsr(reg) ;\
	DO_FP_CALLEE_SAVED(RV_OP_STOREFPREG, reg)

#define LOAD_FP_CALLEE_SAVED(reg) \
	RV_OP_LOADREG t2, _thread_offset_to_fcsr(reg) ;\
	fscsr x0, t2				      ;\
	DO_FP_CALLEE_SAVED(RV_OP_LOADFPREG, reg)

#define COPY_ESF_FP_STATE(to_reg, from_reg, temp)			\
	RV_OP_LOADREG temp, __z_arch_esf_t_fp_state_OFFSET(from_reg)	;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fp_state_OFFSET(to_reg)	;

#define COPY_ESF_FP(to_reg, from_reg, temp)				\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft0_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft0_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft1_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft1_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft2_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft2_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft3_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft3_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft4_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft4_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft5_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft5_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft6_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft6_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft7_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft7_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft8_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft8_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft9_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft9_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft10_OFFSET(from_reg)	;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft10_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ft11_OFFSET(from_reg)	;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ft11_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa0_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa0_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa1_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa1_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa2_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa2_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa3_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa3_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa4_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa4_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa5_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa5_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa6_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa6_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_fa7_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_fa7_OFFSET(to_reg)		;

#define COPY_ESF(to_reg, from_reg, temp)				\
	RV_OP_LOADREG temp, __z_arch_esf_t_mepc_OFFSET(from_reg)	;\
	RV_OP_STOREREG temp, __z_arch_esf_t_mepc_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_mstatus_OFFSET(from_reg)	;\
	RV_OP_STOREREG temp, __z_arch_esf_t_mstatus_OFFSET(to_reg)	;\
	RV_OP_LOADREG temp, __z_arch_esf_t_ra_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_ra_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_tp_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_tp_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_t0_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_t0_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_t1_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_t1_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_t2_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_t2_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_t3_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_t3_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_t4_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_t4_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_t5_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_t5_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_t6_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_t6_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a0_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a0_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a1_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a1_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a2_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a2_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a3_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a3_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a4_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a4_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a5_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a5_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a6_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a6_OFFSET(to_reg)		;\
	RV_OP_LOADREG temp, __z_arch_esf_t_a7_OFFSET(from_reg)		;\
	RV_OP_STOREREG temp, __z_arch_esf_t_a7_OFFSET(to_reg)		;

#define DO_CALLEE_SAVED(op, reg) \
	op s0, _thread_offset_to_s0(reg)		;\
	op s1, _thread_offset_to_s1(reg)		;\
	op s2, _thread_offset_to_s2(reg)		;\
	op s3, _thread_offset_to_s3(reg)		;\
	op s4, _thread_offset_to_s4(reg)		;\
	op s5, _thread_offset_to_s5(reg)		;\
	op s6, _thread_offset_to_s6(reg)		;\
	op s7, _thread_offset_to_s7(reg)		;\
	op s8, _thread_offset_to_s8(reg)		;\
	op s9, _thread_offset_to_s9(reg)		;\
	op s10, _thread_offset_to_s10(reg)		;\
	op s11, _thread_offset_to_s11(reg)		;

#define STORE_CALLEE_SAVED(reg) \
	DO_CALLEE_SAVED(RV_OP_STOREREG, reg)

#define LOAD_CALLEE_SAVED(reg) \
	DO_CALLEE_SAVED(RV_OP_LOADREG, reg)

#define DO_CALLER_SAVED(op) \
	op ra, __z_arch_esf_t_ra_OFFSET(sp)		;\
	op tp, __z_arch_esf_t_tp_OFFSET(sp)		;\
	op t0, __z_arch_esf_t_t0_OFFSET(sp)		;\
	op t1, __z_arch_esf_t_t1_OFFSET(sp)		;\
	op t2, __z_arch_esf_t_t2_OFFSET(sp)		;\
	op t3, __z_arch_esf_t_t3_OFFSET(sp)		;\
	op t4, __z_arch_esf_t_t4_OFFSET(sp)		;\
	op t5, __z_arch_esf_t_t5_OFFSET(sp)		;\
	op t6, __z_arch_esf_t_t6_OFFSET(sp)		;\
	op a0, __z_arch_esf_t_a0_OFFSET(sp)		;\
	op a1, __z_arch_esf_t_a1_OFFSET(sp)		;\
	op a2, __z_arch_esf_t_a2_OFFSET(sp)		;\
	op a3, __z_arch_esf_t_a3_OFFSET(sp)		;\
	op a4, __z_arch_esf_t_a4_OFFSET(sp)		;\
	op a5, __z_arch_esf_t_a5_OFFSET(sp)		;\
	op a6, __z_arch_esf_t_a6_OFFSET(sp)		;\
	op a7, __z_arch_esf_t_a7_OFFSET(sp)		;

#define STORE_CALLER_SAVED() \
	addi sp, sp, -__z_arch_esf_t_SIZEOF	;\
	DO_CALLER_SAVED(RV_OP_STOREREG)		;

#define LOAD_CALLER_SAVED() \
	DO_CALLER_SAVED(RV_OP_LOADREG)		;\
	addi sp, sp, __z_arch_esf_t_SIZEOF	;

/*
 * @brief Check previous mode.
 *
 * @param ret Register to return value.
 * @param temp Register used foor temporary value.
 *
 * @return 0 if previous mode is user.
 */
#define WAS_NOT_USER(ret, temp) \
	RV_OP_LOADREG ret, __z_arch_esf_t_mstatus_OFFSET(sp)	;\
	li temp, MSTATUS_MPP					;\
	and ret, ret, temp					;

#define GET_CPU(ret, temp) \
	csrr ret, mhartid		;\
	li temp, ___cpu_t_SIZEOF	;\
	mul temp, temp, ret		;\
	la ret, _kernel			;\
	add ret, ret, temp		;

#define RISCV_IRQ_INTERRUPT		0x1
#define RISCV_IRQ_USER_ECALL		0x2
#define RISCV_IRQ_EXCEPTION		0x3
#define RISCV_IRQ_RETURN_FROM_SYSCALL	0x4
#define RISCV_IRQ_FORCED_SYSCALL	0x5
#define RISCV_IRQ_OFFLOAD		0x6
#define RISCV_IRQ_CONTEXT_SWITCH	0x7

/* imports */
GDATA(_sw_isr_table)
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)
GTEXT(_Fault)
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

GTEXT(z_get_next_switch_handle)

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
GTEXT(z_thread_mark_switched_in)
GTEXT(z_thread_mark_switched_out)
#ifdef CONFIG_TRACING
GTEXT(sys_trace_isr_enter)
#endif
#endif

#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(_offload_routine)
#endif

#ifdef CONFIG_USERSPACE
GTEXT(z_riscv_do_syscall)
GTEXT(z_riscv_configure_user_allowed_stack)
GTEXT(z_interrupt_stacks)
GTEXT(z_riscv_do_syscall_start)
GTEXT(z_riscv_do_syscall_end)
#endif

#ifdef CONFIG_PMP_STACK_GUARD
GTEXT(z_riscv_configure_stack_guard)
#endif

/* exports */
GTEXT(__irq_wrapper)

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
 *     - __soc_is_irq: decide if we're handling an interrupt or an exception
 *     - __soc_handle_irq: handle SoC-specific details for a pending IRQ
 *       (e.g. clear a pending bit in a SoC-specific register)
 *
 * If CONFIG_RISCV_SOC_CONTEXT_SAVE=y, calls to SoC-level context save/restore
 * routines are also made here. For details, see the Kconfig help text.
 */

/*
 * Handler called upon each exception/interrupt/fault
 * In this architecture, system call (ECALL) is used to perform context
 * switching or IRQ offloading (when enabled).
 */
SECTION_FUNC(exception.entry, __irq_wrapper)

#ifdef CONFIG_PMP_STACK_GUARD
	/* Jump at the beginning of IRQ stack to avoid stack overflow */
	csrrw sp, mscratch, sp
#endif /* CONFIG_PMP_STACK_GUARD */

	/*
	 * Save caller-saved registers on current thread stack.
	 */
	STORE_CALLER_SAVED()

	/* Let's quickly figure out why we're here: context switch, IRQ offload,
	 * user syscall, forced syscall, IRQ or returning from syscall.
	 * Save this information in a0 to guide flow after.
	 */

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
	jal ra, __soc_is_irq
	bnez a0, dispatch_end

	/* Is our exception an ECALL? From machine or user mode? */
	csrr t0, mcause
	li t1, SOC_MCAUSE_EXP_MASK
	and t0, t1, t0

	li t1, SOC_MCAUSE_ECALL_EXP
	beq t0, t1, dispatch_kernel_syscall

#ifdef CONFIG_USERSPACE
	li t1, SOC_MCAUSE_USER_ECALL_EXP
	bne t0, t1, dispatch_exception
	li a0, RISCV_IRQ_USER_ECALL
	j dispatch_end
dispatch_exception:
#endif

	/* If nothing else, this is an exception */
	li a0, RISCV_IRQ_EXCEPTION
	j dispatch_end

dispatch_kernel_syscall:
	/* Kernel syscall, it can still be context switch, IRQ offload,
	 forced syscall or returning from syscall. */

#ifdef CONFIG_USERSPACE
	/* Check if it is a return from user syscall */
	csrr t0, mepc
	la t1, z_riscv_do_syscall_start
	bltu t0, t1, dispatch_not_return_from_syscall
	la t1, z_riscv_do_syscall_end
	bgtu t0, t1, dispatch_not_return_from_syscall
	li a0, RISCV_IRQ_RETURN_FROM_SYSCALL
	j dispatch_end
dispatch_not_return_from_syscall:
	/* Could still be forced syscall. */
	li t0, FORCE_SYSCALL_ID
	bne a7, t0, dispatch_not_forced_syscall
	li a0, RISCV_IRQ_FORCED_SYSCALL
	j dispatch_end
dispatch_not_forced_syscall:
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_IRQ_OFFLOAD
	/*
	 * Determine if the system call is the result of an IRQ offloading.
	 * Done by checking if _offload_routine is not pointing to NULL.
	 */
	la t0, _offload_routine
	RV_OP_LOADREG t1, 0x00(t0)
	beqz t1, dispatch_not_irq_offload
	li a0, RISCV_IRQ_OFFLOAD
	j dispatch_end
dispatch_not_irq_offload:
#endif

	/* Context switch be it then. */
	li a0, RISCV_IRQ_CONTEXT_SWITCH

dispatch_end:
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Assess whether floating-point registers need to be saved.
	 * Note that there's a catch here: if we're performing a
	 * context switch, _current is *not* the outgoing thread - that
	 * can be found via CONTAINER_OF(a1).
	 */
	li t0, RISCV_IRQ_CONTEXT_SWITCH
	beq a0, t0, store_fp_caller_context_switch

	GET_CPU(t0, t1)
	RV_OP_LOADREG t0, ___cpu_t_current_OFFSET(t0)
	j store_fp_caller_saved

store_fp_caller_context_switch:
	RV_OP_LOADREG a1, __z_arch_esf_t_a1_OFFSET(sp)
	addi t0, a1, -___thread_t_switch_handle_OFFSET

store_fp_caller_saved:
	/* t0 should be the thread to have its context saved */
	RV_OP_LOADREG t0, _thread_offset_to_user_options(t0)
	andi t0, t0, K_FP_REGS
	RV_OP_STOREREG t0, __z_arch_esf_t_fp_state_OFFSET(sp)
	beqz t0, skip_store_fp_caller_saved
	STORE_FP_CALLER_SAVED(sp)
skip_store_fp_caller_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Save MEPC register */
	csrr t0, mepc
	RV_OP_STOREREG t0, __z_arch_esf_t_mepc_OFFSET(sp)

	/* Save SOC-specific MSTATUS register */
	csrr t0, mstatus
	RV_OP_STOREREG t0, __z_arch_esf_t_mstatus_OFFSET(sp)

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Handle context saving at SOC level. */
	addi sp, sp, -16
	RV_OP_STOREREG a0, 0x00(sp)

	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_save_context

	RV_OP_LOADREG a0, 0x00(sp)
	addi sp, sp, 16
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

#ifdef CONFIG_USERSPACE
	/* Check if we are in user stack by checking previous privilege mode */
	WAS_NOT_USER(t0, t1)
	bnez t0, is_priv_sp

	GET_CPU(t0, t1)
	RV_OP_LOADREG t1, ___cpu_t_current_OFFSET(t0)

	/* Save user stack pointer */
#ifdef CONFIG_PMP_STACK_GUARD
	csrr t2, mscratch
#else
	mv t2, sp
#endif /* CONFIG_PMP_STACK_GUARD */
	RV_OP_STOREREG t2, _thread_offset_to_user_sp(t1)
	/*
	 * Save callee-saved registers of user thread here
	 * because rescheduling will occur in nested ecall,
	 * that mean these registers will be out of context
	 * at reschedule time.
	 */
	STORE_CALLEE_SAVED(t1)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Assess whether floating-point registers need to be saved. */
	RV_OP_LOADREG t2, _thread_offset_to_user_options(t1)
	andi t2, t2, K_FP_REGS
	beqz t2, skip_store_fp_callee_saved_user
	STORE_FP_CALLEE_SAVED(t1)
skip_store_fp_callee_saved_user:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

is_priv_sp:
	/* Clear user mode variable */
	la t0, is_user_mode
	sb zero, 0x00(t0)
#endif /* CONFIG_USERSPACE */

	/* Jump to is_interrupt to handle interrupts. */
	li t0, RISCV_IRQ_INTERRUPT
	beq a0, t0, is_interrupt

#ifdef CONFIG_USERSPACE
	/* Reset IRQ flag */
	la t1, irq_flag
	sb zero, 0x00(t1)
#endif /* CONFIG_USERSPACE */

	li t0, RISCV_IRQ_EXCEPTION
	beq a0, t0, handle_exception

#ifdef CONFIG_USERSPACE
	li t0, RISCV_IRQ_RETURN_FROM_SYSCALL
	beq a0, t0, return_from_syscall
#endif /* CONFIG_USERSPACE */

	/* At this point, we're sure to be handling a syscall, which
	 * is a result of an ECALL instruction. Increment MEPC to
	 * to avoid triggering the same ECALL again when leaving the
	 * ISR.
	 *
	 * It's safe to always increment by 4, even with compressed
	 * instructions, because the ecall instruction is always 4 bytes.
	 */
	RV_OP_LOADREG t1, __z_arch_esf_t_mepc_OFFSET(sp)
	addi t1, t1, 4
	RV_OP_STOREREG t1, __z_arch_esf_t_mepc_OFFSET(sp)

#ifdef CONFIG_USERSPACE
	li t0, RISCV_IRQ_USER_ECALL
	beq a0, t0, is_user_syscall
#endif

	/* IRQ offload is handled by is_interrupt */
	li t0, RISCV_IRQ_OFFLOAD
	beq a0, t0, is_interrupt

	/* Both forced syscall and context switches are handled by
	 * handle_kernel_syscall.
	 */
	j handle_kernel_syscall

handle_exception:
	/*
	 * Call _Fault to handle exception.
	 * Stack pointer is pointing to a z_arch_esf_t structure, pass it
	 * to _Fault (via register a0).
	 * If _Fault shall return, set return address to
	 * no_reschedule to restore stack.
	 */
	addi a0, sp, 0

#ifdef CONFIG_USERSPACE
	/* Check if we are in user thread */
	WAS_NOT_USER(t0, t1)
	bnez t0, supervisor_fault

user_fault:
	/* Fault at user mode */
	la ra, no_reschedule_user_fault

	/* Switch to privilege stack */
	GET_CPU(t0, t1)
	RV_OP_LOADREG t1, ___cpu_t_current_OFFSET(t0)
	RV_OP_LOADREG t0, _thread_offset_to_priv_stack_start(t1)
	RV_OP_STOREREG sp, _thread_offset_to_user_sp(t1) /* Update user SP */
	li t2, CONFIG_PRIVILEGED_STACK_SIZE
	add sp, t0, t2
	tail _Fault

supervisor_fault:
#endif /* CONFIG_USERSPACE */
	/* Fault at supervisor mode */
	la ra, no_reschedule
	tail _Fault

handle_kernel_syscall:
#ifdef CONFIG_PMP_STACK_GUARD
	li t0, MSTATUS_MPRV
	csrs mstatus, t0

	/* Move to current thread SP and move ESF */
	csrrw sp, mscratch, sp
	csrr t0, mscratch
	addi sp, sp, -__z_arch_esf_t_SIZEOF

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	RV_OP_LOADREG t1, __z_arch_esf_t_fp_state_OFFSET(t0)
	beqz t1, skip_fp_move_kernel_syscall
	COPY_ESF_FP(sp, t0, t1)
skip_fp_move_kernel_syscall:
	COPY_ESF_FP_STATE(sp, t0, t1)
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	COPY_ESF(sp, t0, t1)
	addi t0, t0, __z_arch_esf_t_SIZEOF
	csrw mscratch, t0
#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_USERSPACE
	/*
	 * Check for forced syscall,
	 * otherwise go to riscv_switch to handle context-switch
	 */
	li t0, FORCE_SYSCALL_ID
	bne a7, t0, riscv_switch

	RV_OP_LOADREG a0, __z_arch_esf_t_a0_OFFSET(sp)

	/* Check for user_mode_enter function */
	la t0, arch_user_mode_enter
	bne t0, a0, riscv_switch

	RV_OP_LOADREG a0, __z_arch_esf_t_a1_OFFSET(sp)
	RV_OP_LOADREG a1, __z_arch_esf_t_a2_OFFSET(sp)
	RV_OP_LOADREG a2, __z_arch_esf_t_a3_OFFSET(sp)
	RV_OP_LOADREG a3, __z_arch_esf_t_a4_OFFSET(sp)

	/*
	 * MRET will be done in the following function because
	 * restore caller-saved registers is not need anymore
	 * due to user mode jump (new stack/context).
	 */
	j z_riscv_user_mode_enter_syscall
#endif /* CONFIG_USERSPACE */
	/*
	 * Handle context-switch.
	 */

riscv_switch:
	RV_OP_LOADREG a0, __z_arch_esf_t_a0_OFFSET(sp)
	RV_OP_LOADREG a1, __z_arch_esf_t_a1_OFFSET(sp)
	addi t1, a1, -___thread_t_switch_handle_OFFSET
	j do_switch

#ifdef CONFIG_USERSPACE
is_user_syscall:
#ifdef CONFIG_PMP_STACK_GUARD
	GET_CPU(t0, t1)
	RV_OP_LOADREG a0, ___cpu_t_current_OFFSET(t0)
	jal ra, z_riscv_configure_stack_guard

	/*
	 * Copy ESF to user stack in case of rescheduling
	 * directly from kernel ECALL (nested ECALL)
	 */
	csrrw sp, mscratch, sp
	csrr t0, mscratch
	addi sp, sp, -__z_arch_esf_t_SIZEOF
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	RV_OP_LOADREG t1, __z_arch_esf_t_fp_state_OFFSET(t0)
	beqz t1, skip_fp_copy_user_syscall
	COPY_ESF_FP(sp, t0, t1)
skip_fp_copy_user_syscall:
	COPY_ESF_FP_STATE(sp, t0, t1)
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
	COPY_ESF(sp, t0, t1)
#endif /* CONFIG_PMP_STACK_GUARD */
	/* Restore argument registers from user stack */
	RV_OP_LOADREG a0, __z_arch_esf_t_a0_OFFSET(sp)
	RV_OP_LOADREG a1, __z_arch_esf_t_a1_OFFSET(sp)
	RV_OP_LOADREG a2, __z_arch_esf_t_a2_OFFSET(sp)
	RV_OP_LOADREG a3, __z_arch_esf_t_a3_OFFSET(sp)
	RV_OP_LOADREG a4, __z_arch_esf_t_a4_OFFSET(sp)
	RV_OP_LOADREG a5, __z_arch_esf_t_a5_OFFSET(sp)
	mv a6, sp
	RV_OP_LOADREG a7, __z_arch_esf_t_a7_OFFSET(sp)

	/* Switch to privilege stack */
	GET_CPU(t0, t1)
	RV_OP_LOADREG t1, ___cpu_t_current_OFFSET(t0)
	RV_OP_LOADREG t0, _thread_offset_to_priv_stack_start(t1)
	RV_OP_STOREREG sp, _thread_offset_to_user_sp(t1) /* Update user SP */
	li t2, CONFIG_PRIVILEGED_STACK_SIZE
	add sp, t0, t2

	/* validate syscall limit */
	li t0, K_SYSCALL_LIMIT
	bltu a7, t0, valid_syscall_id

	/* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
	mv a0, a7
	li a7, K_SYSCALL_BAD

valid_syscall_id:

	/* Prepare to jump into do_syscall function */
	la t0, z_riscv_do_syscall
	csrw mepc, t0

	/* Force kernel mode for syscall execution */
	li t0, MSTATUS_MPP
	csrs mstatus, t0
	SOC_ERET

return_from_syscall:
	/*
	 * Retrieve a0 (return value) from privilege stack
	 * (or IRQ stack if stack guard is enabled).
	 */
	RV_OP_LOADREG a0, __z_arch_esf_t_a0_OFFSET(sp)

no_reschedule_user_fault:
	/* Restore user stack */
	GET_CPU(t0, t1)
	RV_OP_LOADREG t1, ___cpu_t_current_OFFSET(t0)
	RV_OP_LOADREG sp, _thread_offset_to_user_sp(t1)

	/* Update a0 (return value) to user stack. */
	RV_OP_STOREREG a0, __z_arch_esf_t_a0_OFFSET(sp)

#ifdef CONFIG_PMP_STACK_GUARD
	/* Move to IRQ stack start */
	csrw mscratch, sp /* Save user sp */
	la t2, z_interrupt_stacks
	li t3, CONFIG_ISR_STACK_SIZE
	add sp, t2, t3

	/*
	 * Copy ESF to IRQ stack from user stack
	 * to execute "no_reschedule" properly.
	 */
	csrr t0, mscratch
	addi sp, sp, -__z_arch_esf_t_SIZEOF
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	RV_OP_LOADREG t1, __z_arch_esf_t_fp_state_OFFSET(t0)
	beqz t1, skip_fp_copy_return_user_syscall
	COPY_ESF_FP(sp, t0, t1)
skip_fp_copy_return_user_syscall:
	COPY_ESF_FP_STATE(sp, t0, t1)
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
	COPY_ESF(sp, t0, t1)

#endif /* CONFIG_PMP_STACK_GUARD */

	j no_reschedule

#endif /* CONFIG_USERSPACE */

is_interrupt:
#ifdef CONFIG_USERSPACE
	la t0, irq_flag
	li t2, 0x1
	sb t2, 0x00(t0)
#endif /* CONFIG_USERSPACE */

#if (CONFIG_USERSPACE == 0) && (CONFIG_PMP_STACK_GUARD == 0)
	/*
	 * Save current thread stack pointer and switch
	 * stack pointer to interrupt stack.
	 */

	/* Save thread stack pointer to temp register t0 */
	addi t0, sp, 0

	/* Switch to interrupt stack */
	GET_CPU(t2, t3)
	RV_OP_LOADREG sp, ___cpu_t_irq_stack_OFFSET(t2)

	/*
	 * Save thread stack pointer on interrupt stack
	 * In RISC-V, stack pointer needs to be 16-byte aligned
	 */
	addi sp, sp, -16
	RV_OP_STOREREG t0, 0x00(sp)
#else
	GET_CPU(t2, t3)
#endif /* !CONFIG_USERSPACE && !CONFIG_PMP_STACK_GUARD */

on_irq_stack:
	/* Increment _current_cpu.nested variable */
	lw t3, ___cpu_t_nested_OFFSET(t2)
	addi t3, t3, 1
	sw t3, ___cpu_t_nested_OFFSET(t2)

#ifdef CONFIG_IRQ_OFFLOAD
	/*
	 * Are we here to perform IRQ offloading?
	 */
	li t0, RISCV_IRQ_OFFLOAD
	bne a0, t0, call_irq

	/*
	 * Call z_irq_do_offload to handle IRQ offloading.
	 * Set return address to on_thread_stack in order to jump there
	 * upon returning from z_irq_do_offload
	 */
	la ra, on_thread_stack
	tail z_irq_do_offload

call_irq:
#endif /* CONFIG_IRQ_OFFLOAD */
#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_enter
#endif

	/* Get IRQ causing interrupt */
	csrr a0, mcause
	li t0, SOC_MCAUSE_EXP_MASK
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
	RV_OP_LOADREG a0, 0x00(t0)

	/* Load ISR function address in register t1 */
	RV_OP_LOADREG t1, RV_REGSIZE(t0)

	/* Call ISR function */
	jalr ra, t1, 0

on_thread_stack:
	/* Get reference to _current_cpu */
	GET_CPU(t1, t2)

	/* Decrement _current_cpu.nested variable */
	lw t2, ___cpu_t_nested_OFFSET(t1)
	addi t2, t2, -1
	sw t2, ___cpu_t_nested_OFFSET(t1)

#if !defined(CONFIG_USERSPACE) && !defined(CONFIG_PMP_STACK_GUARD)
	/* Restore thread stack pointer */
	RV_OP_LOADREG t0, 0x00(sp)
	addi sp, t0, 0
#endif /* !CONFIG_USERSPACE && !CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_STACK_SENTINEL
	call z_check_stack_sentinel
#endif

reschedule:
	/* Get next thread to run and switch to it - or not, if the same */
	GET_CPU(t0, t1)
	RV_OP_LOADREG t1, ___cpu_t_current_OFFSET(t0)

	addi sp, sp, -16
	RV_OP_STOREREG t1, 0x00(sp)

#ifdef CONFIG_SMP
	/* Send NULL so this function doesn't mark outgoing thread
	 * as available for pickup by another CPU. The catch: it will
	 * also return NULL. But luckily, it will update _current.
	 */
	li a0, 0
	call z_get_next_switch_handle
	GET_CPU(t0, t1)
	RV_OP_LOADREG a0, ___cpu_t_current_OFFSET(t0)
#else
	mv a0, t1
	call z_get_next_switch_handle
#endif

	RV_OP_LOADREG t1, 0x00(sp)
	addi sp, sp, 16

	/* From now on, t1 is the outgoing thread */
	beq a0, t1, no_reschedule
	mv a1, x0

#ifdef CONFIG_PMP_STACK_GUARD
	addi sp, sp, -32
	RV_OP_STOREREG a0, 0x00(sp)
	RV_OP_STOREREG a1, 0x08(sp)
	RV_OP_STOREREG t1, 0x10(sp)

	GET_CPU(t2, t3)
	mv a0, t1
	jal ra, z_riscv_configure_stack_guard

	RV_OP_LOADREG a0, 0x00(sp)
	RV_OP_LOADREG a1, 0x08(sp)
	RV_OP_LOADREG t1, 0x10(sp)
	addi sp, sp, 32
	/*
	 * Move to saved SP and move ESF to retrieve it
	 * after reschedule.
	 */
	csrrw sp, mscratch, sp
	csrr t0, mscratch
	addi sp, sp, -__z_arch_esf_t_SIZEOF
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	RV_OP_LOADREG t2, __z_arch_esf_t_fp_state_OFFSET(t0)
	beqz t2, skip_fp_move_irq
	COPY_ESF_FP(sp, t0, t2)
skip_fp_move_irq:
	COPY_ESF_FP_STATE(sp, t0, t2)
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
	COPY_ESF(sp, t0, t2)
	addi t0, t0, __z_arch_esf_t_SIZEOF
	csrw mscratch, t0
#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_USERSPACE
	/* Check if we are in user thread */
	WAS_NOT_USER(t3, t4)
	bnez t3, do_switch

	/*
	 * Switch to privilege stack because we want
	 * this starting point after reschedule.
	 */
	RV_OP_LOADREG t2, _thread_offset_to_priv_stack_start(t1)
	RV_OP_STOREREG sp, _thread_offset_to_user_sp(t1) /* Save user SP */
	mv t0, sp
	li t3, CONFIG_PRIVILEGED_STACK_SIZE
	add sp, t2, t3

	/*
	 * Copy Saved ESF to priv stack, that will allow us to know during
	 * rescheduling if the thread was working on user mode.
	 */
	addi sp, sp, -__z_arch_esf_t_SIZEOF
	COPY_ESF(sp, t0, t2)

#endif /* CONFIG_USERSPACE */

do_switch:
	/* Expectations:
	 * a0: handle for next thread
	 * a1: address of handle for outgoing thread or 0, if not handling arch_switch
	 * t1: k_thread for outgoing thread
	 */

#ifdef CONFIG_USERSPACE
	/*
	 * Check the thread mode and skip callee saved storing
	 * because it is already done for user
	 */
	WAS_NOT_USER(t6, t4)
	beqz t6, skip_callee_saved_reg
#endif /* CONFIG_USERSPACE */

	/*
	 * Save callee-saved registers of current kernel thread
	 * prior to handle context-switching
	 */
	STORE_CALLEE_SAVED(t1)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Assess whether floating-point registers need to be saved. */
	RV_OP_LOADREG t2, _thread_offset_to_user_options(t1)
	andi t2, t2, K_FP_REGS
	beqz t2, skip_store_fp_callee_saved
	STORE_FP_CALLEE_SAVED(t1)

skip_store_fp_callee_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

skip_callee_saved_reg:
#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * Reset mscratch value because is simpler
	 * than remove user ESF, and prevent unknown corner cases
	 */
	la t2, z_interrupt_stacks
	li t3, CONFIG_ISR_STACK_SIZE
	add t2, t2, t3
	csrw mscratch, t2
#endif /* CONFIG_PMP_STACK_GUARD */

	/* Save stack pointer of current thread. */
	RV_OP_STOREREG sp, _thread_offset_to_sp(t1)

	/*
	 * Current thread is saved. If a1 != 0, we're coming from riscv_switch
	 * and need to update switched_from, as it's a synchronization signal
	 * that old thread is saved.
	 */
	beqz a1, clear_old_thread_switch_handle
	addi t2, a1, -___thread_t_switch_handle_OFFSET
	RV_OP_STOREREG t2, 0x00(a1)
	j load_new_thread

clear_old_thread_switch_handle:
#ifdef CONFIG_SMP
	/* Signal that old thread can be picked up by any CPU to be run again */
	RV_OP_STOREREG t1, ___thread_t_switch_handle_OFFSET(t1)
#endif

load_new_thread:
	/*
	 * At this point, a0 contains the new thread. Set
	 * t0 to be current CPU and t1 to be the new thread.
	 */
	GET_CPU(t0, t1)
	mv t1, a0

	/* Switch to new thread stack */
	RV_OP_LOADREG sp, _thread_offset_to_sp(t1)

	/* Restore callee-saved registers of new thread */
	LOAD_CALLEE_SAVED(t1)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Determine if we need to restore floating-point registers. */
	RV_OP_LOADREG t2, _thread_offset_to_user_options(t1)
	andi t2, t2, K_FP_REGS
	beqz t2, skip_load_fp_callee_saved

	/*
	 * If we are switching from a thread with floating-point disabled the
	 * mstatus FS bits will still be cleared, which can cause an illegal
	 * instruction fault. Set the FS state before restoring the registers.
	 * mstatus will be restored later on.
	 */
	li t2, MSTATUS_FS_INIT
	csrrs x0, mstatus, t2

	LOAD_FP_CALLEE_SAVED(t1)

skip_load_fp_callee_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_PMP_STACK_GUARD
	mv a0, t1	/* kernel current */

	/* Save t0/t1 caller registers for function call */
	addi sp, sp, -16
	RV_OP_STOREREG t0, 0(sp)
	RV_OP_STOREREG t1, 8(sp)
	jal ra, z_riscv_configure_stack_guard
	RV_OP_LOADREG t0, 0(sp)
	RV_OP_LOADREG t1, 8(sp)
	addi sp, sp, 16
#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_USERSPACE
	/* t0 still reference to _current_cpu */
	/* t1 still pointer to _current_cpu.current */

	/* Check the thread mode */
	WAS_NOT_USER(t2, t4)
	bnez t2, kernel_swap

	/* Switch to user stack */
	RV_OP_LOADREG sp, _thread_offset_to_user_sp(t1)

	/* Setup User allowed stack */
	li t0, MSTATUS_MPRV
	csrc mstatus, t0
	mv a0, t1
	jal ra, z_riscv_configure_user_allowed_stack

	/* Set user mode variable */
	li t2, 0x1
	la t3, is_user_mode
	sb t2, 0x00(t3)

kernel_swap:
#endif /* CONFIG_USERSPACE */

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	call z_thread_mark_switched_in
#endif

	/*
	 * no_reschedule_resched is an another interrupt return path.
	 *
	 * When CONFIG_PMP_STACK_GUARD=y, reschedule & no_reschedule
	 * code paths use different sp and only no_reschedule code path
	 * needs to switch sp before interrupt return. Thus, we use
	 * another interrupt return path for reschedule path.
	 */
no_reschedule_resched:
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/* Restore MEPC register */
	RV_OP_LOADREG t0, __z_arch_esf_t_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore SOC-specific MSTATUS register */
	RV_OP_LOADREG t0, __z_arch_esf_t_mstatus_OFFSET(sp)
	csrw mstatus, t0

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/*
	 * Determine if we need to restore floating-point registers. This needs
	 * to happen before restoring integer registers to avoid stomping on
	 * t0.
	 */
	RV_OP_LOADREG t0, __z_arch_esf_t_fp_state_OFFSET(sp)
	beqz t0, skip_load_fp_caller_saved_resched
	LOAD_FP_CALLER_SAVED(sp)

skip_load_fp_caller_saved_resched:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Restore caller-saved registers from thread stack */
	LOAD_CALLER_SAVED()

	/* Call SOC_ERET to exit ISR */
	SOC_ERET

no_reschedule:

#ifdef CONFIG_USERSPACE

	/* Check if we are in user thread */
	WAS_NOT_USER(t2, t4)
	bnez t2, no_enter_user

	li t0, MSTATUS_MPRV
	csrc mstatus, t0

	GET_CPU(t0, t1)
	RV_OP_LOADREG a0, ___cpu_t_current_OFFSET(t0)
	jal ra, z_riscv_configure_user_allowed_stack

	/* Set user mode variable */
	li t1, 0x1
	la t0, is_user_mode
	sb t1, 0x00(t0)

	la t0, irq_flag
	lb t0, 0x00(t0)
	bnez t0, no_enter_user

	/* Clear ESF saved in User Stack */
	csrr t0, mscratch
	addi t0, t0, __z_arch_esf_t_SIZEOF
	csrw mscratch, t0

no_enter_user:
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	/* Restore context at SOC level */
	addi a0, sp, __z_arch_esf_t_soc_context_OFFSET
	jal ra, __soc_restore_context
#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */

	/* Restore MEPC register */
	RV_OP_LOADREG t0, __z_arch_esf_t_mepc_OFFSET(sp)
	csrw mepc, t0

	/* Restore SOC-specific MSTATUS register */
	RV_OP_LOADREG t0, __z_arch_esf_t_mstatus_OFFSET(sp)
	csrw mstatus, t0

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/*
	 * Determine if we need to restore floating-point registers. This needs
	 * to happen before restoring integer registers to avoid stomping on
	 * t0.
	 */
	RV_OP_LOADREG t0, __z_arch_esf_t_fp_state_OFFSET(sp)
	beqz t0, skip_load_fp_caller_saved
	LOAD_FP_CALLER_SAVED(sp)

skip_load_fp_caller_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Restore caller-saved registers from thread stack */
	LOAD_CALLER_SAVED()

#ifdef CONFIG_PMP_STACK_GUARD
	csrrw sp, mscratch, sp
#endif /* CONFIG_PMP_STACK_GUARD */
	/* Call SOC_ERET to exit ISR */
	SOC_ERET
