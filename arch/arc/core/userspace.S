/*
 * Copyright (c) 2018 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets_short.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <syscall.h>
#include <swap_macros.h>

.macro clear_scratch_regs
	mov r1, 0
	mov r2, 0
	mov r3, 0
	mov r4, 0
	mov r5, 0
	mov r6, 0
	mov r7, 0
	mov r8, 0
	mov r9, 0
	mov r10, 0
	mov r11, 0
	mov r12, 0
.endm

.macro clear_callee_regs
	mov r25, 0
	mov r24, 0
	mov r23, 0
	mov r22, 0
	mov r21, 0
	mov r20, 0
	mov r19, 0
	mov r18, 0
	mov r17, 0
	mov r16, 0

	mov r15, 0
	mov r14, 0
	mov r13, 0
.endm

GTEXT(_arc_userspace_enter)
GTEXT(_arc_do_syscall)
GTEXT(_user_thread_entry_wrapper)
GTEXT(z_arch_user_string_nlen)
GTEXT(z_arch_user_string_nlen_fault_start)
GTEXT(z_arch_user_string_nlen_fault_end)
GTEXT(z_arch_user_string_nlen_fixup)
/*
 * @brief Wrapper for _thread_entry in the case of user thread
 * The init parameters are in privileged stack
 *
 * @return N/A
 */
SECTION_FUNC(TEXT, _user_thread_entry_wrapper)
	pop_s r3
	pop_s r2
	pop_s r1
	pop_s r0
/* the start of user sp is in r5 */
	pop r5
/* start of privilege stack in blink */
	mov blink, sp

	st.aw r0, [r5, -4]
	st.aw r1, [r5, -4]
	st.aw r2, [r5, -4]
	st.aw r3, [r5, -4]

/*
 * when CONFIG_INIT_STACKS is enable, stack will be initialized
 * in _new_thread_init.
 */
	j _arc_go_to_user_space

/**
 *
 * User space entry function
 *
 * This function is the entry point to user mode from privileged execution.
 * The conversion is one way, and threads which transition to user mode do
 * not transition back later, unless they are doing system calls.
 *
 */
SECTION_FUNC(TEXT, _arc_userspace_enter)
	/*
	 * In ARCv2, the U bit can only be set through exception return
	 */
#ifdef CONFIG_ARC_STACK_CHECKING
	/* disable stack checking as the stack should be initialized */
#ifdef CONFIG_ARC_HAS_SECURE
	lr blink, [_ARC_V2_SEC_STAT]
	bclr blink, blink, _ARC_V2_SEC_STAT_SSC_BIT
	/* sflag blink */
	/* sflag instruction is not supported in current ARC GNU */
	.long 0x07ff302f
#else
	lr blink, [_ARC_V2_STATUS32]
	bclr blink, blink, _ARC_V2_STATUS32_SC_BIT
	kflag blink
#endif
#endif
	/* the end of user stack in r5 */
	add r5, r4, r5
	/* start of privilege stack */
	add blink, r5, CONFIG_PRIVILEGED_STACK_SIZE+STACK_GUARD_SIZE
	mov sp, r5

	push_s r0
	push_s r1
	push_s r2
	push_s r3

	mov r5, sp /* skip r0, r1, r2, r3 */

#ifdef CONFIG_INIT_STACKS
	mov r0, 0xaaaaaaaa
#else
	mov r0, 0x0
#endif
_clear_user_stack:
	st.ab r0, [r4, 4]
	cmp r4, r5
	jlt _clear_user_stack

#ifdef CONFIG_ARC_STACK_CHECKING
	mov r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	_load_stack_check_regs

#ifdef CONFIG_ARC_HAS_SECURE
	lr r0, [_ARC_V2_SEC_STAT]
	bset r0, r0, _ARC_V2_SEC_STAT_SSC_BIT
	/* sflag r0 */
	/* sflag instruction is not supported in current ARC GNU */
	.long 0x003f302f
#else
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_SC_BIT
	kflag r0
#endif
#endif

_arc_go_to_user_space:
	lr r0, [_ARC_V2_STATUS32]
	bset r0, r0, _ARC_V2_STATUS32_U_BIT

	mov r1, _thread_entry_wrapper

	/* fake exception return */
	kflag _ARC_V2_STATUS32_AE

	sr r0, [_ARC_V2_ERSTATUS]
	sr r1, [_ARC_V2_ERET]

#ifdef CONFIG_ARC_HAS_SECURE
	lr r0, [_ARC_V2_SEC_STAT]
	/* the mode returns from exception return is secure mode */
	bset r0, r0, 31
	sr r0, [_ARC_V2_ERSEC_STAT]
	sr r5, [_ARC_V2_SEC_U_SP]
#else
	/* when exception returns from kernel to user, sp and _ARC_V2_USER_SP
	 * will be switched
	 */
	sr r5, [_ARC_V2_USER_SP]
#endif
	mov sp, blink

	mov r0, 0

	clear_callee_regs

	clear_scratch_regs

	mov fp, 0
	mov r29, 0
	mov r30, 0
	mov blink, 0

#ifdef CONFIG_EXECUTION_BENCHMARKING
	b _capture_value_for_benchmarking_userspace
return_loc_userspace_enter:
#endif /* CONFIG_EXECUTION_BENCHMARKING */

	rtie

/**
 *
 * Userspace system call function
 *
 * This function is used to do system calls from unprivileged code.  This
 * function is responsible for the following:
 * 1) Dispatching the system call
 * 2) Restoring stack and calling back to the caller of the system call
 *
 */
SECTION_FUNC(TEXT, _arc_do_syscall)
	/* r0-r5: arg1-arg6, r6 is call id */
	/* the call id is already checked in trap_s handler */
	push_s blink

	mov blink, _k_syscall_table
	ld.as r6, [blink, r6]

	jl [r6]

	/*
	 * no need to clear callee regs, as they will be saved and restored
	 * automatically
	 */
	clear_scratch_regs

	mov r29, 0
	mov r30, 0

	pop_s blink

	/* through fake exception return, go back to the caller */
	kflag _ARC_V2_STATUS32_AE

	/* the status and return address are saved in trap_s handler */
	pop r6
	sr r6, [_ARC_V2_ERSTATUS]
	pop r6
	sr r6, [_ARC_V2_ERET]
#ifdef CONFIG_ARC_HAS_SECURE
	pop r6
	sr r6, [_ARC_V2_ERSEC_STAT]
#endif

	mov r6, 0

	rtie


/*
 * size_t z_arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */
SECTION_FUNC(TEXT, z_arch_user_string_nlen)
	/* int err; */
	sub_s sp,sp,0x4

	/* Initial error value (-1 failure), store at [sp,0] */
	mov_s r3, -1
	st_s r3, [sp, 0]

	/* Loop setup.
	 * r12 (position locator) = s - 1
	 * r0 (length counter return value)) = 0
	 * lp_count = maxsize + 1
	 * */
	sub r12, r0, 0x1
	mov_s r0, 0
	add_s r1, r1, 1
	mov lp_count, r1

strlen_loop:
z_arch_user_string_nlen_fault_start:
	/* is the byte at ++r12 a NULL? if so, we're done. Might fault! */
	ldb.aw r1, [r12, 1]

z_arch_user_string_nlen_fault_end:
	brne_s r1, 0, not_null

strlen_done:
	/* Success, set err to 0 */
	mov_s r1, 0
	st_s r1, [sp, 0]

z_arch_user_string_nlen_fixup:
	/* *err_arg = err; Pop stack and return */
	ld_s r1, [sp, 0]
	add_s sp, sp, 4
	j_s.d [blink]
	st_s r1, [r2, 0]

not_null:
	/* check if we've hit the maximum, if so we're done. */
	brne.d.nt lp_count, 0x1, inc_len
	sub lp_count, lp_count, 0x1
	b_s strlen_done

inc_len:
	/* increment length measurement, loop again */
	add_s r0, r0, 1
	b_s strlen_loop

#ifdef CONFIG_EXECUTION_BENCHMARKING
	.balign 4
_capture_value_for_benchmarking_userspace:
	mov r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]
	_save_callee_saved_regs
	push_s blink

	bl read_timer_end_of_userspace_enter

	pop_s blink
	mov r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]
	_load_callee_saved_regs
	b return_loc_userspace_enter
#endif
