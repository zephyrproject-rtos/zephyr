/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <arch/x86/asm.h>
#include <arch/cpu.h>
#include <offsets_short.h>
#include <syscall.h>

/* Exports */
GTEXT(_x86_syscall_entry_stub)
GTEXT(_x86_userspace_enter)

/* Imports */
GTEXT(_k_syscall_table)

/* Landing site for syscall SW IRQ. Marshal arguments and call C function for
 * further processing. We're on the kernel stack for the invoking thread.
 */
SECTION_FUNC(TEXT, _x86_syscall_entry_stub)
	sti			/* re-enable interrupts */
	cld			/* clear direction flag, restored on 'iret' */

	/* call_id is in ESI. bounds-check it, must be less than
	 * K_SYSCALL_LIMIT
	 */
	cmp	$_SYSCALL_LIMIT, %esi
	jae	_bad_syscall

_id_ok:
	/* Marshal arguments per calling convention to match what is expected
	 * for _k_syscall_handler_t functions
	 */
	push	%esp		/* ssf */
	push	%ebp		/* arg6 */
	push	%edi		/* arg5 */
	push	%ebx		/* arg4 */
#ifndef CONFIG_X86_IAMCU
	push	%ecx		/* arg3 */
	push	%edx		/* arg2	*/
	push	%eax		/* arg1 */
#endif

	/* from the call ID in ESI, load EBX with the actual function pointer
	 * to call by looking it up in the system call dispatch table
	 */
	xor	%edi, %edi
	mov	_k_syscall_table(%edi, %esi, 4), %ebx

	/* Run the handler, which is some entry in _k_syscall_table */
	call	*%ebx

	/* EAX now contains return value. Pop or xor everything else to prevent
	 * information leak from kernel mode.
	 */
#ifndef CONFIG_X86_IAMCU
	pop	%edx		/* old arg1 value, discard it */
	pop	%edx
	pop	%ecx
#endif
	pop	%ebx
	pop	%edi
#ifndef CONFIG_X86_IAMCU
	/* Discard ssf and arg6 */
	add	$8, %esp
#else
	pop	%ecx		/* Clean ECX and get arg6 off the stack */
	pop	%edx		/* Clean EDX and get ssf off the stack */
#endif
	iret

_bad_syscall:
	/* ESI had a bogus syscall value in it, replace with the bad syscall
	 * handler's ID, and put the bad ID as its first argument.  This
	 * clobbers ESI but the bad syscall handler never returns
	 * anyway, it's going to generate a kernel oops
	 */
	mov	%esi, %eax
	mov	$_SYSCALL_BAD, %esi
	jmp	_id_ok


/* FUNC_NORETURN void _x86_userspace_enter(k_thread_entry_t user_entry,
 *					   void *p1, void *p2, void *p3,
 *					   u32_t stack_end,
 *					   u32_t stack_start)
 *
 * A one-way trip to userspace.
 */
SECTION_FUNC(TEXT, _x86_userspace_enter)
	pop	%esi	/* Discard return address on stack */

	/* Fetch parameters on the stack */
#ifndef CONFIG_X86_IAMCU
	pop	%eax	/* user_entry */
	pop	%edx	/* p1 */
	pop	%ecx	/* p2 */
#endif
	pop	%esi	/* p3 */
	pop	%ebx	/* stack_end (high address) */
	pop	%edi	/* stack_start (low address) */

	/* Move to the kernel stack for this thread, so we can erase the
	 * user stack. The kernel stack is the page immediately before
	 * the user stack.
	 *
	 * For security reasons, we must erase the entire user stack.
	 * We don't know what previous contexts it was used and do not
	 * want to leak any information.
	 */
	mov	%edi, %esp

	/* Stash some registers we are going to need to erase the user
	 * stack.
	 */
	push	%ecx
	push	%edi
	push	%eax

	/* Compute size of user stack in 4-byte chunks and put in ECX */
	mov	%ebx, %ecx
	sub	%edi, %ecx
	shr	$2, %ecx	/* Divide by 4 */

#ifdef CONFIG_INIT_STACKS
	mov	$0xAAAAAAAA, %eax
#else
	xor	%eax, %eax
#endif
	/* Copy 4 bytes of memory at a time, starting at ES:EDI, with whatever
	 * is in EAX. Repeat this ECX times.  Stack sizes are always at least
	 * 4-byte aligned.
	 */
	cld
	rep stosl

	/* Restore registers */
	pop	%eax
	pop	%edi
	pop	%ecx

	/* Now set stack pointer to the base of the user stack. Now that this
	 * is set we won't need EBX any more.
	 */
	mov	%ebx, %esp

	/* Set segment registers (except CS and SS which are done in
	 * a special way by 'iret' below)
	 */
	mov	$USER_DATA_SEG, %bx
	mov	%bx, %ds
	mov	%bx, %es

	/* Push arguments to _thread_entry() */
	push	%esi	/* p3 */
#ifndef CONFIG_X86_IAMCU
	push	%ecx	/* p2 */
	push	%edx	/* p1 */
	push	%eax	/* user_entry */
#endif
	/* NULL return address */
	push	$0

	/* Save stack pointer at this position, this is where it will be
	 * when we land in _thread_entry()
	 */
	mov	%esp, %edi

	/* Inter-privilege 'iret' pops all of these. Need to fake an interrupt
	 * return to enter user mode as far calls cannot change privilege
	 * level
	 */
	push	$USER_DATA_SEG	/* SS */
	push	%edi		/* ESP */
	pushfl			/* EFLAGS */
	push	$USER_CODE_SEG	/* CS */
	push	$_thread_entry	/* EIP */

	/* We will land in _thread_entry() in user mode after this */
	iret
