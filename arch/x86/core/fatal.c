/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel fatal error handler
 *
 * This module provides the z_NanoFatalErrorHandler() routine.
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
#include <exc_handle.h>
#include <logging/log_ctrl.h>

__weak void z_debug_fatal_hook(const NANO_ESF *esf) { ARG_UNUSED(esf); }

#ifdef CONFIG_THREAD_STACK_INFO
/**
 * @brief Check if a memory address range falls within the stack
 *
 * Given a memory address range, ensure that it falls within the bounds
 * of the faulting context's stack.
 *
 * @param addr Starting address
 * @param size Size of the region, or 0 if we just want to see if addr is
 *             in bounds
 * @param cs Code segment of faulting context
 * @return true if addr/size region is not within the thread stack
 */
static bool check_stack_bounds(u32_t addr, size_t size, u16_t cs)
{
	u32_t start, end;

	if (z_is_in_isr()) {
		/* We were servicing an interrupt */
		start = (u32_t)Z_ARCH_THREAD_STACK_BUFFER(_interrupt_stack);
		end = start + CONFIG_ISR_STACK_SIZE;
	} else if ((cs & 0x3U) != 0U ||
		   (_current->base.user_options & K_USER) == 0) {
		/* Thread was in user mode, or is not a user mode thread.
		 * The normal stack buffer is what we will check.
		 */
		start = _current->stack_info.start;
		end = STACK_ROUND_DOWN(_current->stack_info.start +
				       _current->stack_info.size);
	} else {
		/* User thread was doing a syscall, check kernel stack bounds */
		start = _current->stack_info.start - MMU_PAGE_SIZE;
		end = _current->stack_info.start;
	}

	return (addr <= start) || (addr + size > end);
}
#endif

#if defined(CONFIG_EXCEPTION_STACK_TRACE)
struct stack_frame {
	u32_t next;
	u32_t ret_addr;
	u32_t args;
};

#define MAX_STACK_FRAMES 8

static void unwind_stack(u32_t base_ptr, u16_t cs)
{
	struct stack_frame *frame;
	int i;

	if (base_ptr == 0U) {
		printk("NULL base ptr\n");
		return;
	}

	for (i = 0; i < MAX_STACK_FRAMES; i++) {
		if (base_ptr % sizeof(base_ptr) != 0U) {
			printk("unaligned frame ptr\n");
			return;
		}

		frame = (struct stack_frame *)base_ptr;
		if (frame == NULL) {
			break;
		}

#ifdef CONFIG_THREAD_STACK_INFO
		/* Ensure the stack frame is within the faulting context's
		 * stack buffer
		 */
		if (check_stack_bounds((u32_t)frame, sizeof(*frame), cs)) {
			printk("     corrupted? (bp=%p)\n", frame);
			break;
		}
#endif

		if (frame->ret_addr == 0U) {
			break;
		}
#ifdef CONFIG_X86_IAMCU
		printk("     0x%08x\n", frame->ret_addr);
#else
		printk("     0x%08x (0x%x)\n", frame->ret_addr, frame->args);
#endif
		base_ptr = frame->next;
	}
}
#endif /* CONFIG_EXCEPTION_STACK_TRACE */

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
FUNC_NORETURN void z_NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *pEsf)
{
#ifdef CONFIG_THREAD_NAME
	const char *thread_name = k_thread_name_get(k_current_get());
#endif

	LOG_PANIC();

	z_debug_fatal_hook(pEsf);

#ifdef CONFIG_PRINTK

	/* Display diagnostic information about the error */

	switch (reason) {
	case _NANO_ERR_CPU_EXCEPTION:
		break;

	case _NANO_ERR_SPURIOUS_INT: {
		int vector = z_irq_controller_isr_vector_get();

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

	printk("Current thread ID = %p"
#ifdef CONFIG_THREAD_NAME
	       " (%s)"
#endif
	       "\n"
	       "eax: 0x%08x, ebx: 0x%08x, ecx: 0x%08x, edx: 0x%08x\n"
	       "esi: 0x%08x, edi: 0x%08x, ebp: 0x%08x, esp: 0x%08x\n"
	       "eflags: 0x%08x cs: 0x%04x\n"
#ifdef CONFIG_EXCEPTION_STACK_TRACE
	       "call trace:\n"
#endif
	       "eip: 0x%08x\n",
	       k_current_get(),
#ifdef CONFIG_THREAD_NAME
	       thread_name ? thread_name : "unknown",
#endif
	       pEsf->eax, pEsf->ebx, pEsf->ecx, pEsf->edx,
	       pEsf->esi, pEsf->edi, pEsf->ebp, pEsf->esp,
	       pEsf->eflags, pEsf->cs & 0xFFFFU, pEsf->eip);
#ifdef CONFIG_EXCEPTION_STACK_TRACE
	unwind_stack(pEsf->ebp, pEsf->cs);
#endif

#endif /* CONFIG_PRINTK */


	/*
	 * Error was fatal to a kernel task or a thread; invoke the system
	 * fatal error handling policy defined for the platform.
	 */

	z_SysFatalErrorHandler(reason, pEsf);
}

FUNC_NORETURN void z_arch_syscall_oops(void *ssf_ptr)
{
	struct _x86_syscall_stack_frame *ssf =
		(struct _x86_syscall_stack_frame *)ssf_ptr;
	NANO_ESF oops = {
		.eip = ssf->eip,
		.cs = ssf->cs,
		.eflags = ssf->eflags
	};

	if (oops.cs == USER_CODE_SEG) {
		oops.esp = ssf->esp;
	}

	z_NanoFatalErrorHandler(_NANO_ERR_KERNEL_OOPS, &oops);
}

#ifdef CONFIG_X86_KERNEL_OOPS
FUNC_NORETURN void z_do_kernel_oops(const NANO_ESF *esf)
{
	u32_t *stack_ptr = (u32_t *)esf->esp;
	z_NanoFatalErrorHandler(*stack_ptr, esf);
}

extern void (*_kernel_oops_handler)(void);
NANO_CPU_INT_REGISTER(_kernel_oops_handler, NANO_SOFT_IRQ,
		      CONFIG_X86_KERNEL_OOPS_VECTOR / 16,
		      CONFIG_X86_KERNEL_OOPS_VECTOR, 0);
#endif

/*
 * Define a default ESF for use with z_NanoFatalErrorHandler() in the event
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
	switch (vector) {
	case IV_GENERAL_PROTECTION:
		printk("General Protection Fault\n");
		break;
	case IV_DEVICE_NOT_AVAILABLE:
		printk("Floating point unit not enabled\n");
		break;
	default:
		printk("CPU exception %d\n", vector);
		break;
	}
	if ((BIT(vector) & _EXC_ERROR_CODE_FAULTS) != 0) {
		printk("***** Exception code: 0x%x\n", pEsf->errorCode);
	}
	z_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, pEsf);
}

#define _EXC_FUNC(vector) \
FUNC_NORETURN void handle_exc_##vector(const NANO_ESF *pEsf) \
{ \
	generic_exc_handle(vector, pEsf); \
}

#define Z_EXC_FUNC_CODE(vector) \
	_EXC_FUNC(vector) \
	_EXCEPTION_CONNECT_CODE(handle_exc_##vector, vector)

#define Z_EXC_FUNC_NOCODE(vector) \
	_EXC_FUNC(vector) \
	_EXCEPTION_CONNECT_NOCODE(handle_exc_##vector, vector)

/* Necessary indirection to ensure 'vector' is expanded before we expand
 * the handle_exc_##vector
 */
#define EXC_FUNC_NOCODE(vector) \
	Z_EXC_FUNC_NOCODE(vector)

#define EXC_FUNC_CODE(vector) \
	Z_EXC_FUNC_CODE(vector)

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
}

static void dump_mmu_flags(struct x86_mmu_pdpt *pdpt, void *addr)
{
	x86_page_entry_data_t pde_flags, pte_flags;

	z_x86_mmu_get_flags(pdpt, addr, &pde_flags, &pte_flags);

	printk("PDE: ");
	dump_entry_flags(pde_flags);

	printk("PTE: ");
	dump_entry_flags(pte_flags);
}
#endif /* CONFIG_X86_MMU */

static void dump_page_fault(NANO_ESF *esf)
{
	u32_t err, cr2;

	/* See Section 6.15 of the IA32 Software Developer's Manual vol 3 */
	__asm__ ("mov %%cr2, %0" : "=r" (cr2));

	err = esf->errorCode;
	printk("***** CPU Page Fault (error code 0x%08x)\n", err);

	printk("%s thread %s address 0x%08x\n",
	       (err & US) != 0U ? "User" : "Supervisor",
	       (err & ID) != 0U ? "executed" : ((err & WR) != 0U ? "wrote" :
						"read"), cr2);

#ifdef CONFIG_X86_MMU
#ifdef CONFIG_X86_KPTI
	if (err & US) {
		dump_mmu_flags(&z_x86_user_pdpt, (void *)cr2);
		return;
	}
#endif
	dump_mmu_flags(&z_x86_kernel_pdpt, (void *)cr2);
#endif
}
#endif /* CONFIG_EXCEPTION_DEBUG */

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_arch_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_arch_user_string_nlen)
};
#endif

void page_fault_handler(NANO_ESF *esf)
{
#ifdef CONFIG_USERSPACE
	int i;

	for (i = 0; i < ARRAY_SIZE(exceptions); i++) {
		if ((void *)esf->eip >= exceptions[i].start &&
		    (void *)esf->eip < exceptions[i].end) {
			esf->eip = (unsigned int)(exceptions[i].fixup);
			return;
		}
	}
#endif
#ifdef CONFIG_EXCEPTION_DEBUG
	dump_page_fault(esf);
#endif
#ifdef CONFIG_THREAD_STACK_INFO
	if (check_stack_bounds(esf->esp, 0, esf->cs)) {
		z_NanoFatalErrorHandler(_NANO_ERR_STACK_CHK_FAIL, esf);
	}
#endif
	z_NanoFatalErrorHandler(_NANO_ERR_CPU_EXCEPTION, esf);
	CODE_UNREACHABLE;
}
_EXCEPTION_CONNECT_CODE(page_fault_handler, IV_PAGE_FAULT);

#ifdef CONFIG_X86_ENABLE_TSS
static __noinit volatile NANO_ESF _df_esf;

/* Very tiny stack; just enough for the bogus error code pushed by the CPU
 * and a frame pointer push by the compiler. All df_handler_top does is
 * shuffle some data around with 'mov' statements and then 'iret'.
 */
static __noinit char _df_stack[8];

static FUNC_NORETURN __used void df_handler_top(void);

#ifdef CONFIG_X86_KPTI
extern char z_trampoline_stack_end[];
#endif

Z_GENERIC_SECTION(.tss)
struct task_state_segment _main_tss = {
	.ss0 = DATA_SEG,
#ifdef CONFIG_X86_KPTI
	/* Stack to land on when we get a soft/hard IRQ in user mode.
	 * In a special kernel page that, unlike all other kernel pages,
	 * is marked present in the user page table.
	 */
	.esp0 = (u32_t)&z_trampoline_stack_end
#endif
};

/* Special TSS for handling double-faults with a known good stack */
Z_GENERIC_SECTION(.tss)
struct task_state_segment _df_tss = {
	.esp = (u32_t)(_df_stack + sizeof(_df_stack)),
	.cs = CODE_SEG,
	.ds = DATA_SEG,
	.es = DATA_SEG,
	.ss = DATA_SEG,
	.eip = (u32_t)df_handler_top,
	.cr3 = (u32_t)&z_x86_kernel_pdpt
};

static FUNC_NORETURN __used void df_handler_bottom(void)
{
	/* We're back in the main hardware task on the interrupt stack */
	int reason = _NANO_ERR_CPU_EXCEPTION;

	/* Restore the top half so it is runnable again */
	_df_tss.esp = (u32_t)(_df_stack + sizeof(_df_stack));
	_df_tss.eip = (u32_t)df_handler_top;

	printk("***** Double Fault *****\n");
#ifdef CONFIG_THREAD_STACK_INFO
	if (check_stack_bounds(_df_esf.esp, 0, _df_esf.cs)) {
		reason = _NANO_ERR_STACK_CHK_FAIL;
	}
#endif
	z_NanoFatalErrorHandler(reason, (NANO_ESF *)&_df_esf);
}

static FUNC_NORETURN __used void df_handler_top(void)
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
	_main_tss.esp = (u32_t)(Z_ARCH_THREAD_STACK_BUFFER(_interrupt_stack) +
				CONFIG_ISR_STACK_SIZE);
	_main_tss.cs = CODE_SEG;
	_main_tss.ds = DATA_SEG;
	_main_tss.es = DATA_SEG;
	_main_tss.ss = DATA_SEG;
	_main_tss.eip = (u32_t)df_handler_bottom;
	_main_tss.cr3 = (u32_t)&z_x86_kernel_pdpt;
	_main_tss.eflags = 0U;

	/* NT bit is set in EFLAGS so we will task switch back to _main_tss
	 * and run df_handler_bottom
	 */
	__asm__ volatile ("iret");
	CODE_UNREACHABLE;
}

/* Configure a task gate descriptor in the IDT for the double fault
 * exception
 */
_X86_IDT_TSS_REGISTER(DF_TSS, -1, -1, IV_DOUBLE_FAULT, 0);

#endif /* CONFIG_X86_ENABLE_TSS */
