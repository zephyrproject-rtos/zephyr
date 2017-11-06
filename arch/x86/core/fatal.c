/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel fatal error handler
 *
 * This module provides the _NanoFatalErrorHandler() routine.
 */

#include <toolchain.h>
#include <linker/sections.h>

#include <kernel.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <arch/x86/irq_controller.h>
#include <arch/x86/segmentation.h>
#include <exception.h>
#include <inttypes.h>

__weak void _debug_fatal_hook(const NANO_ESF *esf) { ARG_UNUSED(esf); }


/**
 *
 * @brief Kernel fatal error handler
 *
 * This routine is called when a fatal error condition is detected by either
 * hardware or software.
 *
 * The caller is expected to always provide a usable ESF.  In the event that the
 * fatal error does not have a hardware generated ESF, the caller should either
 * create its own or use a pointer to the global default ESF <_default_esf>.
 *
 * @param reason the reason that the handler was called
 * @param pEsf pointer to the exception stack frame
 *
 * @return This function does not return.
 */
FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *pEsf)
{
	_debug_fatal_hook(pEsf);

#ifdef CONFIG_PRINTK

	/* Display diagnostic information about the error */

	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
		break;

	case _NANO_ERR_SPURIOUS_INT: {
		int vector = _irq_controller_isr_vector_get();

		printk("***** Unhandled interrupt vector ");
		if (vector >= 0) {
			printk("%d ", vector);
		}
		printk("*****\n");
		break;
	}
#if defined(CONFIG_STACK_CANARIES) || defined(CONFIG_STACK_SENTINEL) || \
		defined(CONFIG_HW_STACK_PROTECTION) || \
		defined(CONFIG_USERSPACE)
	case _NANO_ERR_STACK_CHK_FAIL:
		printk("***** Stack Check Fail! *****\n");
		break;
#endif /* CONFIG_STACK_CANARIES */

	case _NANO_ERR_KERNEL_OOPS:
		printk("***** Kernel OOPS! *****\n");
		break;

	case _NANO_ERR_KERNEL_PANIC:
		printk("***** Kernel Panic! *****\n");
		break;

	case _NANO_ERR_ALLOCATION_FAIL:
		printk("**** Kernel Allocation Failure! ****\n");
		break;

	default:
		printk("**** Unknown Fatal Error %d! ****\n", reason);
		break;
	}

	printk("Current thread ID = %p\n"
	       "Faulting segment:address = 0x%04x:0x%08x\n"
	       "eax: 0x%08x, ebx: 0x%08x, ecx: 0x%08x, edx: 0x%08x\n"
	       "esi: 0x%08x, edi: 0x%08x, ebp: 0x%08x, esp: 0x%08x\n"
	       "eflags: 0x%x\n",
	       k_current_get(),
	       pEsf->cs & 0xFFFF, pEsf->eip,
	       pEsf->eax, pEsf->ebx, pEsf->ecx, pEsf->edx,
	       pEsf->esi, pEsf->edi, pEsf->ebp, pEsf->esp,
	       pEsf->eflags);
#endif /* CONFIG_PRINTK */


	/*
	 * Error was fatal to a kernel task or a thread; invoke the system
	 * fatal error handling policy defined for the platform.
	 */

	_SysFatalErrorHandler(reason, pEsf);
}

FUNC_NORETURN void _arch_syscall_oops(void *ssf_ptr)
{
	struct _x86_syscall_stack_frame *ssf =
		(struct _x86_syscall_stack_frame *)ssf_ptr;
	NANO_ESF oops_esf = {
		.eip = ssf->eip,
		.cs = ssf->cs,
		.eflags = ssf->eflags
	};

	if (oops_esf.cs == USER_CODE_SEG) {
		oops_esf.esp = ssf->esp;
	}

	_NanoFatalErrorHandler(_NANO_ERR_KERNEL_OOPS, &oops_esf);
}

#ifdef CONFIG_X86_KERNEL_OOPS
/* The reason code gets pushed onto the stack right before the exception is
 * triggered, so it would be after the nano_esf data
 */
struct oops_esf {
	NANO_ESF nano_esf;
	unsigned int reason;
};

FUNC_NORETURN void _do_kernel_oops(const struct oops_esf *esf)
{
	_NanoFatalErrorHandler(esf->reason, &esf->nano_esf);
}

extern void (*_kernel_oops_handler)(void);
NANO_CPU_INT_REGISTER(_kernel_oops_handler, NANO_SOFT_IRQ,
		      CONFIG_X86_KERNEL_OOPS_VECTOR / 16,
		      CONFIG_X86_KERNEL_OOPS_VECTOR, 0);
#endif

/*
 * Define a default ESF for use with _NanoFatalErrorHandler() in the event
 * the caller does not have a NANO_ESF to pass
 */
const NANO_ESF _default_esf = {
	0xdeaddead, /* ESP */
	0xdeaddead, /* EBP */
	0xdeaddead, /* EBX */
	0xdeaddead, /* ESI */
	0xdeaddead, /* EDI */
	0xdeaddead, /* EDX */
	0xdeaddead, /* ECX */
	0xdeaddead, /* EAX */
	0xdeaddead, /* error code */
	0xdeaddead, /* EIP */
	0xdeaddead, /* CS */
	0xdeaddead, /* EFLAGS */
};

#if CONFIG_EXCEPTION_DEBUG

static FUNC_NORETURN void generic_exc_handle(unsigned int vector,
					     const NANO_ESF *pEsf)
{
	printk("***** ");
	if (vector == 13) {
		printk("General Protection Fault\n");
	} else {
		printk("CPU exception %d\n", vector);
	}
	if ((1 << vector) & _EXC_ERROR_CODE_FAULTS) {
		printk("***** Exception code: 0x%x\n", pEsf->errorCode);
	}
	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, pEsf);
}

#define _EXC_FUNC(vector) \
FUNC_NORETURN void handle_exc_##vector(const NANO_ESF *pEsf) \
{ \
	generic_exc_handle(vector, pEsf); \
}

#define _EXC_FUNC_CODE(vector) \
	_EXC_FUNC(vector) \
	_EXCEPTION_CONNECT_CODE(handle_exc_##vector, vector)

#define _EXC_FUNC_NOCODE(vector) \
	_EXC_FUNC(vector) \
	_EXCEPTION_CONNECT_NOCODE(handle_exc_##vector, vector)

/* Necessary indirection to ensure 'vector' is expanded before we expand
 * the handle_exc_##vector
 */
#define EXC_FUNC_NOCODE(vector) \
	_EXC_FUNC_NOCODE(vector)

#define EXC_FUNC_CODE(vector) \
	_EXC_FUNC_CODE(vector)

EXC_FUNC_NOCODE(IV_DIVIDE_ERROR);
EXC_FUNC_NOCODE(IV_NON_MASKABLE_INTERRUPT);
EXC_FUNC_NOCODE(IV_OVERFLOW);
EXC_FUNC_NOCODE(IV_BOUND_RANGE);
EXC_FUNC_NOCODE(IV_INVALID_OPCODE);
EXC_FUNC_NOCODE(IV_DEVICE_NOT_AVAILABLE);
#ifndef CONFIG_X86_ENABLE_TSS
EXC_FUNC_NOCODE(IV_DOUBLE_FAULT);
#endif
EXC_FUNC_CODE(IV_INVALID_TSS);
EXC_FUNC_CODE(IV_SEGMENT_NOT_PRESENT);
EXC_FUNC_CODE(IV_STACK_FAULT);
EXC_FUNC_CODE(IV_GENERAL_PROTECTION);
EXC_FUNC_NOCODE(IV_X87_FPU_FP_ERROR);
EXC_FUNC_CODE(IV_ALIGNMENT_CHECK);
EXC_FUNC_NOCODE(IV_MACHINE_CHECK);

/* Page fault error code flags */
#define PRESENT	BIT(0)
#define WR	BIT(1)
#define US	BIT(2)
#define RSVD	BIT(3)
#define ID	BIT(4)
#define PK	BIT(5)
#define SGX	BIT(15)

#ifdef CONFIG_X86_MMU
static void dump_entry_flags(x86_page_entry_data_t flags)
{
#ifdef CONFIG_X86_PAE_MODE
	printk("0x%x%x %s, %s, %s, %s\n", (u32_t)(flags>>32),
	       (u32_t)(flags),
	       flags & (x86_page_entry_data_t)MMU_ENTRY_PRESENT ?
	       "Present" : "Non-present",
	       flags & (x86_page_entry_data_t)MMU_ENTRY_WRITE ?
	       "Writable" : "Read-only",
	       flags & (x86_page_entry_data_t)MMU_ENTRY_USER ?
	       "User" : "Supervisor",
	       flags & (x86_page_entry_data_t)MMU_ENTRY_EXECUTE_DISABLE ?
	       "Execute Disable" : "Execute Enabled");
#else
	printk("0x%03x %s, %s, %s\n", flags,
	       flags & (x86_page_entry_data_t)MMU_ENTRY_PRESENT ?
	       "Present" : "Non-present",
	       flags & (x86_page_entry_data_t)MMU_ENTRY_WRITE ?
	       "Writable" : "Read-only",
	       flags & (x86_page_entry_data_t)MMU_ENTRY_USER ?
	       "User" : "Supervisor");
#endif
}

static void dump_mmu_flags(void *addr)
{
	x86_page_entry_data_t pde_flags, pte_flags;

	_x86_mmu_get_flags(addr, &pde_flags, &pte_flags);

	printk("PDE: ");
	dump_entry_flags(pde_flags);

	printk("PTE: ");
	dump_entry_flags(pte_flags);
}
#endif

FUNC_NORETURN void page_fault_handler(const NANO_ESF *pEsf)
{
	u32_t err, cr2;

	/* See Section 6.15 of the IA32 Software Developer's Manual vol 3 */
	__asm__ ("mov %%cr2, %0" : "=r" (cr2));

	err = pEsf->errorCode;
	printk("***** CPU Page Fault (error code 0x%08x)\n", err);

	printk("%s thread %s address 0x%08x\n",
	       err & US ? "User" : "Supervisor",
	       err & ID ? "executed" : (err & WR ? "wrote" : "read"),
	       cr2);

#ifdef CONFIG_X86_MMU
	dump_mmu_flags((void *)cr2);
#endif

	_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, pEsf);
}
_EXCEPTION_CONNECT_CODE(page_fault_handler, IV_PAGE_FAULT);
#endif /* CONFIG_EXCEPTION_DEBUG */

#ifdef CONFIG_X86_ENABLE_TSS
static __noinit volatile NANO_ESF _df_esf;

/* Very tiny stack; just enough for the bogus error code pushed by the CPU
 * and a frame pointer push by the compiler. All _df_handler_top does is
 * shuffle some data around with 'mov' statements and then 'iret'.
 */
static __noinit char _df_stack[8];

static FUNC_NORETURN __used void _df_handler_top(void);

_GENERIC_SECTION(.tss)
struct task_state_segment _main_tss = {
	.ss0 = DATA_SEG
};

/* Special TSS for handling double-faults with a known good stack */
_GENERIC_SECTION(.tss)
struct task_state_segment _df_tss = {
	.esp = (u32_t)(_df_stack + sizeof(_df_stack)),
	.cs = CODE_SEG,
	.ds = DATA_SEG,
	.es = DATA_SEG,
	.ss = DATA_SEG,
	.eip = (u32_t)_df_handler_top,
#ifdef CONFIG_X86_PAE_MODE
	.cr3 = (u32_t)X86_MMU_PDPT
#else
	.cr3 = (u32_t)X86_MMU_PD
#endif
};

static FUNC_NORETURN __used void _df_handler_bottom(void)
{
	/* We're back in the main hardware task on the interrupt stack */
	x86_page_entry_data_t pte_flags, pde_flags;
	int reason;

	/* Restore the top half so it is runnable again */
	_df_tss.esp = (u32_t)(_df_stack + sizeof(_df_stack));
	_df_tss.eip = (u32_t)_df_handler_top;

	/* Now check if the stack pointer is inside a guard area. Subtract
	 * one byte, since if a single push operation caused the fault ESP
	 * wouldn't be decremented
	 */
	_x86_mmu_get_flags((void *)_df_esf.esp - 1, &pde_flags, &pte_flags);
	if (pte_flags & MMU_ENTRY_PRESENT) {
		printk("***** Double Fault *****\n");
		reason = _NANO_ERR_CPU_EXCEPTION;
	} else {
		reason = _NANO_ERR_STACK_CHK_FAIL;
	}

	_NanoFatalErrorHandler(reason, (NANO_ESF *)&_df_esf);
}

static FUNC_NORETURN __used void _df_handler_top(void)
{
	/* State of the system when the double-fault forced a task switch
	 * will be in _main_tss. Set up a NANO_ESF and copy system state into
	 * it
	 */
	_df_esf.esp = _main_tss.esp;
	_df_esf.ebp = _main_tss.ebp;
	_df_esf.ebx = _main_tss.ebx;
	_df_esf.esi = _main_tss.esi;
	_df_esf.edi = _main_tss.edi;
	_df_esf.edx = _main_tss.edx;
	_df_esf.eax = _main_tss.eax;
	_df_esf.ecx = _main_tss.ecx;
	_df_esf.errorCode = 0;
	_df_esf.eip = _main_tss.eip;
	_df_esf.cs = _main_tss.cs;
	_df_esf.eflags = _main_tss.eflags;

	/* Restore the main IA task to a runnable state */
	_main_tss.esp = (u32_t)(_ARCH_THREAD_STACK_BUFFER(_interrupt_stack) +
				CONFIG_ISR_STACK_SIZE);
	_main_tss.cs = CODE_SEG;
	_main_tss.ds = DATA_SEG;
	_main_tss.es = DATA_SEG;
	_main_tss.ss = DATA_SEG;
	_main_tss.eip = (u32_t)_df_handler_bottom;
#ifdef CONFIG_X86_PAE_MODE
	_main_tss.cr3 = (u32_t)X86_MMU_PDPT;
#else
	_main_tss.cr3 = (u32_t)X86_MMU_PD;
#endif

	/* NT bit is set in EFLAGS so we will task switch back to _main_tss
	 * and run _df_handler_bottom
	 */
	__asm__ volatile ("iret");
	CODE_UNREACHABLE;
}

/* Configure a task gate descriptor in the IDT for the double fault
 * exception
 */
_X86_IDT_TSS_REGISTER(DF_TSS, -1, -1, IV_DOUBLE_FAULT, 0);

#endif /* CONFIG_X86_ENABLE_TSS */
