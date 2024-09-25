/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/x86/ia32/asm.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel/mm.h>
#include <x86_mmu.h>

/* Exports */
GTEXT(z_x86_syscall_entry_stub)
GTEXT(z_x86_userspace_enter)
GTEXT(arch_user_string_nlen)
GTEXT(z_x86_user_string_nlen_fault_start)
GTEXT(z_x86_user_string_nlen_fault_end)
GTEXT(z_x86_user_string_nlen_fixup)

/* Imports */
GDATA(_k_syscall_table)

#ifdef CONFIG_X86_KPTI
/* Switch from the shadow to the kernel page table, switch to the interrupted
 * thread's kernel stack, and copy all context from the trampoline stack.
 *
 * Assumes all registers are callee-saved since this gets called from other
 * ASM code. Assumes a particular stack layout which is correct for
 * _exception_enter and _interrupt_enter when invoked with a call instruction:
 *
 *  28 SS
 *  24 ES
 *  20 EFLAGS
 *  16 CS
 *  12 EIP
 *  8  isr_param or exc code
 *  4  isr or exc handler
 *  0  return address
 */
SECTION_FUNC(PINNED_TEXT, z_x86_trampoline_to_kernel)
	/* Check interrupted code segment to see if we came from ring 3
	 * and hence on the trampoline stack
	 */
	testb $3, 16(%esp) /* Offset of CS */
	jz 1f

	/* Stash these regs as we need to use them */
	pushl	%esi
	pushl	%edi

	/* Switch to kernel page table */
	movl	$K_MEM_PHYS_ADDR(z_x86_kernel_ptables), %esi
	movl	%esi, %cr3

	/* Save old trampoline stack pointer in %edi */
	movl	%esp, %edi

	/* Switch to privilege mode stack */
	movl	$_kernel, %esi
	movl	_kernel_offset_to_current(%esi), %esi
	movl	_thread_offset_to_psp(%esi), %esp

	/* Transplant stack context and restore ESI/EDI. Taking care to zero
	 * or put uninteresting values where we stashed ESI/EDI since the
	 * trampoline page is insecure and there might a context switch
	 * on the way out instead of returning to the original thread
	 * immediately.
	 */
	pushl	36(%edi)	/* SS */
	pushl	32(%edi)	/* ESP */
	pushl	28(%edi)	/* EFLAGS */
	pushl	24(%edi)	/* CS */
	pushl	20(%edi)	/* EIP */
	pushl	16(%edi)	/* error code or isr parameter */
	pushl	12(%edi)	/* exception/irq handler */
	pushl   8(%edi)		/* return address */
	movl	4(%edi), %esi	/* restore ESI */
	movl	$0, 4(%edi)	/* Zero old esi storage area */
	xchgl	%edi, (%edi)	/* Exchange old edi to restore it and put
				   old sp in the storage area */

	/* Trampoline stack should have nothing sensitive in it at this point */
1:
	ret

/* Copy interrupt return stack context to the trampoline stack, switch back
 * to the user page table, and only then 'iret'. We jump to this instead
 * of calling 'iret' if KPTI is turned on.
 *
 * Stack layout is expected to be as follows:
 *
 * 16 SS
 * 12 ESP
 * 8 EFLAGS
 * 4 CS
 * 0 EIP
 *
 * This function is conditionally macroed to KPTI_IRET/KPTI_IRET_USER
 */
SECTION_FUNC(PINNED_TEXT, z_x86_trampoline_to_user)
	/* Check interrupted code segment to see if we came from ring 3
	 * and hence on the trampoline stack
	 */
	testb $3, 4(%esp) /* Offset of CS */
	jz 1f

	/* Otherwise, fall through ... */

SECTION_FUNC(PINNED_TEXT, z_x86_trampoline_to_user_always)
	/* Stash EDI, need a free register */
	pushl	%edi

	/* Store old stack pointer and switch to trampoline stack.
	 * Lock IRQs before changing stack pointer to the trampoline stack,
	 * we don't want any interrupts also using the trampoline stack
	 * during this time.
	 */
	movl	%esp, %edi
	cli
	movl	$z_trampoline_stack_end, %esp

	/* Copy context */
	pushl	20(%edi)	/* SS */
	pushl	16(%edi)	/* ESP */
	pushl	12(%edi)	/* EFLAGS */
	pushl   8(%edi)		/* CS */
	pushl   4(%edi)		/* EIP */
	xchgl	%edi, (%edi)	/* Exchange old edi to restore it and put
				   trampoline stack address in its old storage
				   area */
	/* Switch to user page table */
	pushl	%eax
	movl	$_kernel, %eax
	movl	_kernel_offset_to_current(%eax), %eax
	movl	_thread_offset_to_ptables(%eax), %eax
	movl	%eax, %cr3
	popl	%eax
	movl	$0, -4(%esp)	/* Delete stashed EAX data */

	/* Trampoline stack should have nothing sensitive in it at this point */
1:
	iret
#endif /* CONFIG_X86_KPTI */

/* Landing site for syscall SW IRQ. Marshal arguments and call C function for
 * further processing. We're on the kernel stack for the invoking thread,
 * unless KPTI is enabled, in which case we're on the trampoline stack and
 * need to get off it before enabling interrupts.
 */
SECTION_FUNC(TEXT, z_x86_syscall_entry_stub)
#ifdef CONFIG_X86_KPTI
	/* Stash these regs as we need to use them */
	pushl	%esi
	pushl	%edi

	/* Switch to kernel page table */
	movl	$K_MEM_PHYS_ADDR(z_x86_kernel_ptables), %esi
	movl	%esi, %cr3

	/* Save old trampoline stack pointer in %edi */
	movl	%esp, %edi

	/* Switch to privilege elevation stack */
	movl	$_kernel, %esi
	movl	_kernel_offset_to_current(%esi), %esi
	movl	_thread_offset_to_psp(%esi), %esp

	/* Transplant context according to layout above. Variant of logic
	 * in x86_trampoline_to_kernel */
	pushl	24(%edi)	/* SS */
	pushl	20(%edi)	/* ESP */
	pushl	16(%edi)	/* EFLAGS */
	pushl	12(%edi)	/* CS */
	pushl	8(%edi)		/* EIP */
	movl	4(%edi), %esi	/* restore ESI */
	movl	$0, 4(%edi)	/* Zero old esi storage area */
	xchgl	%edi, (%edi)	/* Exchange old edi to restore it and put
				   old sp in the storage area */

	/* Trampoline stack should have nothing sensitive in it at this point */
#endif /* CONFIG_X86_KPTI */

	sti			/* re-enable interrupts */
	cld			/* clear direction flag, restored on 'iret' */

	/* call_id is in ESI. bounds-check it, must be less than
	 * K_SYSCALL_LIMIT
	 */
	cmp	$K_SYSCALL_LIMIT, %esi
	jae	_bad_syscall

_id_ok:
#ifdef CONFIG_X86_BOUNDS_CHECK_BYPASS_MITIGATION
	/* Prevent speculation with bogus system call IDs */
	lfence
#endif
	/* Marshal arguments per calling convention to match what is expected
	 * for _k_syscall_handler_t functions
	 */
	push	%esp		/* ssf */
	push	%ebp		/* arg6 */
	push	%edi		/* arg5 */
	push	%ebx		/* arg4 */
	push	%ecx		/* arg3 */
	push	%edx		/* arg2	*/
	push	%eax		/* arg1 */

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
	pop	%edx		/* old arg1 value, discard it */
	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%edi
	/* Discard ssf and arg6 */
	add	$8, %esp
	KPTI_IRET_USER

_bad_syscall:
	/* ESI had a bogus syscall value in it, replace with the bad syscall
	 * handler's ID, and put the bad ID as its first argument.  This
	 * clobbers ESI but the bad syscall handler never returns
	 * anyway, it's going to generate a kernel oops
	 */
	mov	%esi, %eax
	mov	$K_SYSCALL_BAD, %esi
	jmp	_id_ok


/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */
SECTION_FUNC(TEXT, arch_user_string_nlen)
	push	%ebp
	mov	%esp, %ebp

	/* error value, set to -1 initially. This location is -4(%ebp) */
	push	$-1

	/* Do the strlen operation, based on disassembly of minimal libc */
	xor	%eax, %eax		/* EAX = 0, length count */
	mov	0x8(%ebp), %edx		/* EDX base of string */

	/* This code might page fault */
strlen_loop:
z_x86_user_string_nlen_fault_start:
	cmpb	$0x0, (%edx, %eax, 1)	/* *(EDX + EAX) == 0? Could fault. */

z_x86_user_string_nlen_fault_end:
	je	strlen_done
	cmp	0xc(%ebp), %eax		/* Max length reached? */
	je	strlen_done
	inc	%eax			/* EAX++ and loop again */
	jmp	strlen_loop

strlen_done:
	/* Set error value to 0 since we succeeded */
	movl	$0, -4(%ebp)

z_x86_user_string_nlen_fixup:
	/* Write error value to err pointer parameter */
	movl	0x10(%ebp), %ecx
	pop	%edx
	movl	%edx, (%ecx)

	pop	%ebp
	ret


/* FUNC_NORETURN void z_x86_userspace_enter(k_thread_entry_t user_entry,
 *					   void *p1, void *p2, void *p3,
 *					   uint32_t stack_end,
 *					   uint32_t stack_start)
 *
 * A one-way trip to userspace.
 */
SECTION_FUNC(TEXT, z_x86_userspace_enter)
	pop	%esi	/* Discard return address on stack */

	/* Fetch parameters on the stack */
	pop	%eax	/* user_entry */
	pop	%edx	/* p1 */
	pop	%ecx	/* p2 */
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

	/* Erase and enable US bit in page tables for the stack buffer */
	push	%ecx
	push	%eax
	push	%edx
	call	z_x86_current_stack_perms
	pop	%edx
	pop	%eax
	pop	%ecx

	/* Set stack pointer to the base of the freshly-erased user stack.
	 * Now that this is set we won't need EBX any more.
	 */
	mov	%ebx, %esp

	/* Set segment registers (except CS and SS which are done in
	 * a special way by 'iret' below)
	 */
	mov	$USER_DATA_SEG, %bx
	mov	%bx, %ds
	mov	%bx, %es

	/* Push arguments to z_thread_entry() */
	push	%esi	/* p3 */
	push	%ecx	/* p2 */
	push	%edx	/* p1 */
	push	%eax	/* user_entry */
	/* NULL return address */
	push	$0

	/* Save stack pointer at this position, this is where it will be
	 * when we land in z_thread_entry()
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
	push	$z_thread_entry	/* EIP */

	/* We will land in z_thread_entry() in user mode after this */
	KPTI_IRET_USER
