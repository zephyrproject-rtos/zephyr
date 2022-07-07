/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/spinlock.h>
#include <zephyr/arch/x86/efi.h>
#include <zephyr/sys/mem_manage.h>
#include "../zefi/efi.h" /* ZEFI not on include path */

#define EFI_CON_BUFSZ 128

/* Big stack for the EFI code to use */
static uint64_t __aligned(64) efi_stack[1024];

struct efi_boot_arg *efi;

void *efi_get_acpi_rsdp(void)
{
	if (efi == NULL) {
		return NULL;
	}

	return efi->acpi_rsdp;
}

void efi_init(struct efi_boot_arg *efi_arg)
{
	if (efi_arg == NULL) {
		return;
	}

	z_phys_map((uint8_t **)&efi, (uintptr_t)efi_arg,
		   sizeof(struct efi_boot_arg), 0);
}

/* EFI thunk.  Not a lot of code, but lots of context:
 *
 * We need to swap in the original EFI page tables for this to work,
 * as Zephyr has only mapped memory it uses and IO it knows about.  In
 * theory we might need to restore more state too (maybe the EFI code
 * uses special segment descriptors from its own GDT, maybe it relies
 * on interrupts in its own IDT, maybe it twiddles custom MSRs or
 * plays with the IO-MMU... the posibilities are endless).  But
 * experimentally, only the memory state seems to be required on known
 * hardware.  This is safe because in the existing architecture Zephyr
 * has already initialized all its own memory and left the rest of the
 * system as-is; we already know it doesn't overlap with the EFI
 * environment (because we've always just assumed that's the case,
 * heh).
 *
 * Similarly we need to swap the stack: EFI firmware was written in an
 * environment where it would be running on multi-gigabyte systems and
 * likes to overflow the tiny stacks Zephyr code uses.  (There is also
 * the problem of the red zone -- SysV reserves 128 bytes of
 * unpreserved data "under" the stack pointer for the use of the
 * current function.  Our compiler would be free to write things there
 * that might be clobbered by the EFI call, which doesn't understand
 * that rule.  Inspection of generated code shows that we're safe, but
 * still, best to swap stacks explicitly.)
 *
 * And the calling conventions are different: the EFI function uses
 * Microsoft's ABI, not SysV.  Parameters go in RCX/RDX/R8/R9 (though
 * we only pass two here), and return value is in RAX (which we
 * multiplex as an input to hold the function pointer).  R10 and R11
 * are also caller-save.  Technically X/YMM0-5 are caller-save too,
 * but as long as this (SysV) function was called per its own ABI they
 * have already been saved by our own caller.  Also note that there is
 * a 32 byte region ABOVE the return value that must be allocated by
 * the caller as spill space for the 4 register-passed arguments (this
 * ABI is so weird...).  We also need two call-preserved scratch
 * registers (for preserving the stack pointer and page table), those
 * are R12/R13.
 *
 * Finally: note that the firmware on at least one board (an Up
 * Squared APL device) will internally ENABLE INTERRUPTS before
 * returing from its OutputString method.  This is... unfortunate, and
 * says poor things about reliability using this code as it will
 * implicitly break the spinlock we're using.  The OS will be able to
 * take an interrupt just fine, but if the resulting ISR tries to log,
 * we'll end up in EFI firmware reentrantly!  The best we can do is an
 * unconditional CLI immediately after returning.
 */
static uint64_t efi_call(void *fn, uint64_t arg1, uint64_t arg2)
{
	void *stack_top = &efi_stack[ARRAY_SIZE(efi_stack) - 4];

	__asm__ volatile("movq %%cr3, %%r12;" /* save zephyr page table */
			 "movq %%rsp, %%r13;" /* save stack pointer */
			 "movq %%rsi, %%rsp;" /* set stack */
			 "movq %%rdi, %%cr3;" /* set EFI page table */
			 "callq *%%rax;"
			 "cli;"
			 "movq %%r12, %%cr3;" /* reset paging */
			 "movq %%r13, %%rsp;" /* reset stack */
			 : "+a"(fn)
			 : "c"(arg1), "d"(arg2), "S"(stack_top), "D"(efi->efi_cr3)
			 : "r8", "r9", "r10", "r11", "r12", "r13");

	return (uint64_t) fn;
}

int efi_console_putchar(int c)
{
	static struct k_spinlock lock;
	static uint16_t efibuf[EFI_CON_BUFSZ + 1];
	static int n;
	static void *conout;
	static void *output_string_fn;
	struct efi_system_table *efist = efi->efi_systab;

	if (c == '\n') {
		efi_console_putchar('\r');
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* These structs live in EFI memory and aren't mapped by
	 * Zephyr.  Extract the needed pointers by swapping page
	 * tables.  Do it via lazy evaluation because this code is
	 * routinely needed much earlier than any feasible init hook.
	 */
	if (conout == NULL) {
		uint64_t cr3;

		__asm__ volatile("movq %%cr3, %0" : "=r"(cr3));
		__asm__ volatile("movq %0, %%cr3" :: "r"(efi->efi_cr3));
		conout = efist->ConOut;
		output_string_fn = efist->ConOut->OutputString;
		__asm__ volatile("movq %0, %%cr3" :: "r"(cr3));
	}

	/* Buffer, to reduce trips through the thunking layer.
	 * Flushes when full and at newlines.
	 */
	efibuf[n++] = c;
	if (c == '\n' || n == EFI_CON_BUFSZ) {
		efibuf[n] = 0U;
		(void)efi_call(output_string_fn, (uint64_t)conout, (uint64_t)efibuf);
		n = 0;
	}

	k_spin_unlock(&lock, key);
	return 0;
}

#ifdef CONFIG_X86_EFI_CONSOLE
int arch_printk_char_out(int c)
{
	return efi_console_putchar(c);
}
#endif
