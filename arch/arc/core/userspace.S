/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets_short.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <syscall.h>

GTEXT(_arc_userspace_enter)
GTEXT(_arc_do_syscall)
GTEXT(_arch_is_user_context)


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
	/* the end of user stack in r5 */
	add r5, r4, r5
	/* start of privilege stack */
	add r2, r5, CONFIG_PRIVILEGED_STACK_SIZE
	sub r5, r5, 16 /* skip r0, r1, r2, r3 */

#ifdef CONFIG_INIT_STACKS
	mov r0, 0xaaaaaaaa
#else
	mov r0, 0x0
#endif
_clear_user_stack:
	st.ab r0, [r4, 4]
	cmp r4, r5
	jlt _clear_user_stack

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
	mov sp, r2
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
	push_s r0

	mov r0, _k_syscall_table
	ld.as blink, [r0, r6]

	pop_s r0

	jl [blink]


	pop_s blink


	/* through fake exception return, go back to the caller */
	kflag _ARC_V2_STATUS32_AE

#ifdef CONFIG_ARC_HAS_SECURE
	lr r6, [_ARC_V2_SEC_STAT]
	/* the mode returns from exception return is secure mode */
	bset r6, r6, 31
	sr r6, [_ARC_V2_ERSEC_STAT]
#endif
	/* the status and return addesss are saved in trap_s handler */
	pop r6
	sr r6, [_ARC_V2_ERSTATUS]
	pop r6
	sr r6, [_ARC_V2_ERET]
	rtie

SECTION_FUNC(TEXT, _arch_is_user_context)
	lr r0, [_ARC_V2_STATUS32]
	bbit1 r0, 20, 1f
	bset r1, r0, 20
	bclr r1, r1, 31
	kflag r1
	lr r1, [_ARC_V2_STATUS32]
	bbit0 r1, 20, 2f
	kflag r0
1:
	j_s.d [blink]
	mov r0, 0
2:
	j_s.d [blink]
	mov r0, 1
