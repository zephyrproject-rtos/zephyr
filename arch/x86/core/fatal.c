/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <exc_handle.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

#if defined(CONFIG_BOARD_QEMU_X86) || defined(CONFIG_BOARD_QEMU_X86_64)
FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	ARG_UNUSED(reason);

	/* Causes QEMU to exit. We passed the following on the command line:
	 * -device isa-debug-exit,iobase=0xf4,iosize=0x04
	 *
	 * For any value of the first argument X, the return value of the
	 * QEMU process is (X * 2) + 1.
	 *
	 * It has been observed that if the emulator exits for a triple-fault
	 * (often due to bad page tables or other CPU structures) it will
	 * terminate with 0 error code.
	 */
	sys_out32(reason, 0xf4);
	CODE_UNREACHABLE;
}
#endif

#ifdef CONFIG_THREAD_STACK_INFO
static inline uintptr_t esf_get_sp(const z_arch_esf_t *esf)
{
#ifdef CONFIG_X86_64
	return esf->rsp;
#else
	return esf->esp;
#endif
}
#endif

#ifdef CONFIG_EXCEPTION_DEBUG
static inline uintptr_t esf_get_code(const z_arch_esf_t *esf)
{
#ifdef CONFIG_X86_64
	return esf->code;
#else
	return esf->errorCode;
#endif
}
#endif

#ifdef CONFIG_THREAD_STACK_INFO
bool z_x86_check_stack_bounds(uintptr_t addr, size_t size, uint16_t cs)
{
	uintptr_t start, end;

	if (_current == NULL || arch_is_in_isr()) {
		/* We were servicing an interrupt or in early boot environment
		 * and are supposed to be on the interrupt stack */
		int cpu_id;

#ifdef CONFIG_SMP
		cpu_id = arch_curr_cpu()->id;
#else
		cpu_id = 0;
#endif
		start = (uintptr_t)Z_KERNEL_STACK_BUFFER(
		    z_interrupt_stacks[cpu_id]);
		end = start + CONFIG_ISR_STACK_SIZE;
	} else if ((cs & 0x3U) != 0U ||
		   (_current->base.user_options & K_USER) == 0) {
		/* Thread was in user mode, or is not a user mode thread.
		 * The normal stack buffer is what we will check.
		 */
		start = _current->stack_info.start;
		end = Z_STACK_PTR_ALIGN(_current->stack_info.start +
				       _current->stack_info.size);
	} else {
		/* User thread was doing a syscall, check kernel stack bounds */
		start = _current->stack_info.start - MMU_PAGE_SIZE;
		end = _current->stack_info.start;
	}

	return (addr <= start) || (addr + size > end);
}
#endif

#ifdef CONFIG_EXCEPTION_DEBUG
#if defined(CONFIG_X86_EXCEPTION_STACK_TRACE)
struct stack_frame {
	uintptr_t next;
	uintptr_t ret_addr;
#ifndef CONFIG_X86_64
	uintptr_t args;
#endif
};

#define MAX_STACK_FRAMES 8

static void unwind_stack(uintptr_t base_ptr, uint16_t cs)
{
	struct stack_frame *frame;
	int i;

	if (base_ptr == 0U) {
		LOG_ERR("NULL base ptr");
		return;
	}

	for (i = 0; i < MAX_STACK_FRAMES; i++) {
		if (base_ptr % sizeof(base_ptr) != 0U) {
			LOG_ERR("unaligned frame ptr");
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
		if (z_x86_check_stack_bounds((uintptr_t)frame,
					     sizeof(*frame), cs)) {
			LOG_ERR("     corrupted? (bp=%p)", frame);
			break;
		}
#endif

		if (frame->ret_addr == 0U) {
			break;
		}
#ifdef CONFIG_X86_64
		LOG_ERR("     0x%016lx", frame->ret_addr);
#else
		LOG_ERR("     0x%08lx (0x%lx)", frame->ret_addr, frame->args);
#endif
		base_ptr = frame->next;
	}
}
#endif /* CONFIG_X86_EXCEPTION_STACK_TRACE */

static inline struct x86_page_tables *get_ptables(const z_arch_esf_t *esf)
{
#if defined(CONFIG_USERSPACE) && defined(CONFIG_X86_KPTI)
	/* If the interrupted thread was in user mode, we did a page table
	 * switch when we took the exception via z_x86_trampoline_to_kernel
	 */
	if ((esf->cs & 0x3) != 0) {
		return z_x86_thread_page_tables_get(_current);
	}
#else
	ARG_UNUSED(esf);
#endif
	return z_x86_page_tables_get();
}

#ifdef CONFIG_X86_64
static void dump_regs(const z_arch_esf_t *esf)
{
	LOG_ERR("RAX: 0x%016lx RBX: 0x%016lx RCX: 0x%016lx RDX: 0x%016lx",
		esf->rax, esf->rbx, esf->rcx, esf->rdx);
	LOG_ERR("RSI: 0x%016lx RDI: 0x%016lx RBP: 0x%016lx RSP: 0x%016lx",
		esf->rsi, esf->rdi, esf->rbp, esf->rsp);
	LOG_ERR(" R8: 0x%016lx  R9: 0x%016lx R10: 0x%016lx R11: 0x%016lx",
		esf->r8, esf->r9, esf->r10, esf->r11);
	LOG_ERR("R12: 0x%016lx R13: 0x%016lx R14: 0x%016lx R15: 0x%016lx",
		esf->r12, esf->r13, esf->r14, esf->r15);
	LOG_ERR("RSP: 0x%016lx RFLAGS: 0x%016lx CS: 0x%04lx CR3: %p", esf->rsp,
		esf->rflags, esf->cs & 0xFFFFU, get_ptables(esf));

#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	LOG_ERR("call trace:");
#endif
	LOG_ERR("RIP: 0x%016lx", esf->rip);
#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	unwind_stack(esf->rbp, esf->cs);
#endif
}
#else /* 32-bit */
static void dump_regs(const z_arch_esf_t *esf)
{
	LOG_ERR("EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x",
		esf->eax, esf->ebx, esf->ecx, esf->edx);
	LOG_ERR("ESI: 0x%08x, EDI: 0x%08x, EBP: 0x%08x, ESP: 0x%08x",
		esf->esi, esf->edi, esf->ebp, esf->esp);
	LOG_ERR("EFLAGS: 0x%08x CS: 0x%04x CR3: %p", esf->eflags,
		esf->cs & 0xFFFFU, get_ptables(esf));

#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	LOG_ERR("call trace:");
#endif
	LOG_ERR("EIP: 0x%08x", esf->eip);
#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	unwind_stack(esf->ebp, esf->cs);
#endif
}
#endif /* CONFIG_X86_64 */

static void log_exception(uintptr_t vector, uintptr_t code)
{
	switch (vector) {
	case IV_DIVIDE_ERROR:
		LOG_ERR("Divide by zero");
		break;
	case IV_DEBUG:
		LOG_ERR("Debug");
		break;
	case IV_NON_MASKABLE_INTERRUPT:
		LOG_ERR("Non-maskable interrupt");
		break;
	case IV_BREAKPOINT:
		LOG_ERR("Breakpoint");
		break;
	case IV_OVERFLOW:
		LOG_ERR("Overflow");
		break;
	case IV_BOUND_RANGE:
		LOG_ERR("Bound range exceeded");
		break;
	case IV_INVALID_OPCODE:
		LOG_ERR("Invalid opcode");
		break;
	case IV_DEVICE_NOT_AVAILABLE:
		LOG_ERR("Floating point unit device not available");
		break;
	case IV_DOUBLE_FAULT:
		LOG_ERR("Double fault (code 0x%lx)", code);
		break;
	case IV_COPROC_SEGMENT_OVERRUN:
		LOG_ERR("Co-processor segment overrun");
		break;
	case IV_INVALID_TSS:
		LOG_ERR("Invalid TSS (code 0x%lx)", code);
		break;
	case IV_SEGMENT_NOT_PRESENT:
		LOG_ERR("Segment not present (code 0x%lx)", code);
		break;
	case IV_STACK_FAULT:
		LOG_ERR("Stack segment fault");
		break;
	case IV_GENERAL_PROTECTION:
		LOG_ERR("General protection fault (code 0x%lx)", code);
		break;
	/* IV_PAGE_FAULT skipped, we have a dedicated handler */
	case IV_X87_FPU_FP_ERROR:
		LOG_ERR("x87 floating point exception");
		break;
	case IV_ALIGNMENT_CHECK:
		LOG_ERR("Alignment check (code 0x%lx)", code);
		break;
	case IV_MACHINE_CHECK:
		LOG_ERR("Machine check");
		break;
	case IV_SIMD_FP:
		LOG_ERR("SIMD floating point exception");
		break;
	case IV_VIRT_EXCEPTION:
		LOG_ERR("Virtualization exception");
		break;
	case IV_SECURITY_EXCEPTION:
		LOG_ERR("Security exception");
		break;
	default:
		break;
	}
}

/* Page fault error code flags */
#define PRESENT	BIT(0)
#define WR	BIT(1)
#define US	BIT(2)
#define RSVD	BIT(3)
#define ID	BIT(4)
#define PK	BIT(5)
#define SGX	BIT(15)

static void dump_page_fault(z_arch_esf_t *esf)
{
	uintptr_t err, cr2;

	/* See Section 6.15 of the IA32 Software Developer's Manual vol 3 */
	__asm__ ("mov %%cr2, %0" : "=r" (cr2));

	err = esf_get_code(esf);
	LOG_ERR("Page fault at address 0x%lx (error code 0x%lx)", cr2, err);

	if ((err & RSVD) != 0) {
		LOG_ERR("Reserved bits set in page tables");
	} else if ((err & PRESENT) == 0) {
		LOG_ERR("Linear address not present in page tables");
	} else {
		LOG_ERR("Access violation: %s thread not allowed to %s",
			(err & US) != 0U ? "user" : "supervisor",
			(err & ID) != 0U ? "execute" : ((err & WR) != 0U ?
							"write" :
							"read"));
		if ((err & PK) != 0) {
			LOG_ERR("Protection key disallowed");
		} else if ((err & SGX) != 0) {
			LOG_ERR("SGX access control violation");
		}
	}

#ifdef CONFIG_X86_MMU
	z_x86_dump_mmu_flags(get_ptables(esf), cr2);
#endif /* CONFIG_X86_MMU */
}
#endif /* CONFIG_EXCEPTION_DEBUG */

FUNC_NORETURN void z_x86_fatal_error(unsigned int reason,
				     const z_arch_esf_t *esf)
{
	if (esf != NULL) {
#ifdef CONFIG_EXCEPTION_DEBUG
		dump_regs(esf);
#endif
#if defined(CONFIG_ASSERT) && defined(CONFIG_X86_64)
		if (esf->rip == 0xb9) {
			/* See implementation of __resume in locore.S. This is
			 * never a valid RIP value. Treat this as a kernel
			 * panic.
			 */
			LOG_ERR("Attempt to resume un-suspended thread object");
			reason = K_ERR_KERNEL_PANIC;
		}
#endif
	}
	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

FUNC_NORETURN void z_x86_unhandled_cpu_exception(uintptr_t vector,
						 const z_arch_esf_t *esf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	log_exception(vector, esf_get_code(esf));
#else
	ARG_UNUSED(vector);
#endif
	z_x86_fatal_error(K_ERR_CPU_EXCEPTION, esf);
}

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_x86_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_x86_user_string_nlen)
};
#endif

void z_x86_page_fault_handler(z_arch_esf_t *esf)
{
#ifdef CONFIG_USERSPACE
	int i;

	for (i = 0; i < ARRAY_SIZE(exceptions); i++) {
#ifdef CONFIG_X86_64
		if ((void *)esf->rip >= exceptions[i].start &&
		    (void *)esf->rip < exceptions[i].end) {
			esf->rip = (uint64_t)(exceptions[i].fixup);
			return;
		}
#else
		if ((void *)esf->eip >= exceptions[i].start &&
		    (void *)esf->eip < exceptions[i].end) {
			esf->eip = (unsigned int)(exceptions[i].fixup);
			return;
		}
#endif /* CONFIG_X86_64 */
	}
#endif
#ifdef CONFIG_EXCEPTION_DEBUG
	dump_page_fault(esf);
#endif
#ifdef CONFIG_THREAD_STACK_INFO
	if (z_x86_check_stack_bounds(esf_get_sp(esf), 0, esf->cs)) {
		z_x86_fatal_error(K_ERR_STACK_CHK_FAIL, esf);
	}
#endif
	z_x86_fatal_error(K_ERR_CPU_EXCEPTION, esf);
	CODE_UNREACHABLE;
}

void z_x86_do_kernel_oops(const z_arch_esf_t *esf)
{
	uintptr_t reason;

#ifdef CONFIG_X86_64
	reason = esf->rax;
#else
	uintptr_t *stack_ptr = (uintptr_t *)esf->esp;

	reason = *stack_ptr;
#endif

#ifdef CONFIG_USERSPACE
	/* User mode is only allowed to induce oopses and stack check
	 * failures via this software interrupt
	 */
	if ((esf->cs & 0x3) != 0 && !(reason == K_ERR_KERNEL_OOPS ||
				      reason == K_ERR_STACK_CHK_FAIL)) {
		reason = K_ERR_KERNEL_OOPS;
	}
#endif

	z_x86_fatal_error(reason, esf);
}
