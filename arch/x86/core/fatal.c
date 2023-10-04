/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <zephyr/exc_handle.h>
#include <zephyr/logging/log.h>
#include <x86_mmu.h>
#include <mmu.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

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

__pinned_func
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
#ifdef CONFIG_USERSPACE
	} else if ((cs & 0x3U) == 0U &&
		   (_current->base.user_options & K_USER) != 0) {
		/* The low two bits of the CS register is the privilege
		 * level. It will be 0 in supervisor mode and 3 in user mode
		 * corresponding to ring 0 / ring 3.
		 *
		 * If we get here, we must have been doing a syscall, check
		 * privilege elevation stack bounds
		 */
		start = _current->stack_info.start - CONFIG_MMU_PAGE_SIZE;
		end = _current->stack_info.start;
#endif /* CONFIG_USERSPACE */
	} else {
		/* Normal thread operation, check its stack buffer */
		start = _current->stack_info.start;
		end = Z_STACK_PTR_ALIGN(_current->stack_info.start +
					_current->stack_info.size);
	}

	return (addr <= start) || (addr + size > end);
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

#if defined(CONFIG_X86_EXCEPTION_STACK_TRACE)
struct stack_frame {
	uintptr_t next;
	uintptr_t ret_addr;
#ifndef CONFIG_X86_64
	uintptr_t args;
#endif
};

#define MAX_STACK_FRAMES 8

__pinned_func
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

static inline uintptr_t get_cr3(const z_arch_esf_t *esf)
{
#if defined(CONFIG_USERSPACE) && defined(CONFIG_X86_KPTI)
	/* If the interrupted thread was in user mode, we did a page table
	 * switch when we took the exception via z_x86_trampoline_to_kernel
	 */
	if ((esf->cs & 0x3) != 0) {
		return _current->arch.ptables;
	}
#else
	ARG_UNUSED(esf);
#endif
	/* Return the current CR3 value, it didn't change when we took
	 * the exception
	 */
	return z_x86_cr3_get();
}

static inline pentry_t *get_ptables(const z_arch_esf_t *esf)
{
	return z_mem_virt_addr(get_cr3(esf));
}

#ifdef CONFIG_X86_64
__pinned_func
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
	LOG_ERR("RSP: 0x%016lx RFLAGS: 0x%016lx CS: 0x%04lx CR3: 0x%016lx",
		esf->rsp, esf->rflags, esf->cs & 0xFFFFU, get_cr3(esf));

#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	LOG_ERR("call trace:");
#endif
	LOG_ERR("RIP: 0x%016lx", esf->rip);
#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	unwind_stack(esf->rbp, esf->cs);
#endif
}
#else /* 32-bit */
__pinned_func
static void dump_regs(const z_arch_esf_t *esf)
{
	LOG_ERR("EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x",
		esf->eax, esf->ebx, esf->ecx, esf->edx);
	LOG_ERR("ESI: 0x%08x, EDI: 0x%08x, EBP: 0x%08x, ESP: 0x%08x",
		esf->esi, esf->edi, esf->ebp, esf->esp);
	LOG_ERR("EFLAGS: 0x%08x CS: 0x%04x CR3: 0x%08lx", esf->eflags,
		esf->cs & 0xFFFFU, get_cr3(esf));

#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	LOG_ERR("call trace:");
#endif
	LOG_ERR("EIP: 0x%08x", esf->eip);
#ifdef CONFIG_X86_EXCEPTION_STACK_TRACE
	unwind_stack(esf->ebp, esf->cs);
#endif
}
#endif /* CONFIG_X86_64 */

__pinned_func
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
		LOG_ERR("Exception not handled (code 0x%lx)", code);
		break;
	}
}

__pinned_func
static void dump_page_fault(z_arch_esf_t *esf)
{
	uintptr_t err;
	void *cr2;

	cr2 = z_x86_cr2_get();
	err = esf_get_code(esf);
	LOG_ERR("Page fault at address %p (error code 0x%lx)", cr2, err);

	if ((err & PF_RSVD) != 0) {
		LOG_ERR("Reserved bits set in page tables");
	} else {
		if ((err & PF_P) == 0) {
			LOG_ERR("Linear address not present in page tables");
		}
		LOG_ERR("Access violation: %s thread not allowed to %s",
			(err & PF_US) != 0U ? "user" : "supervisor",
			(err & PF_ID) != 0U ? "execute" : ((err & PF_WR) != 0U ?
							   "write" :
							   "read"));
		if ((err & PF_PK) != 0) {
			LOG_ERR("Protection key disallowed");
		} else if ((err & PF_SGX) != 0) {
			LOG_ERR("SGX access control violation");
		}
	}

#ifdef CONFIG_X86_MMU
	z_x86_dump_mmu_flags(get_ptables(esf), cr2);
#endif /* CONFIG_X86_MMU */
}
#endif /* CONFIG_EXCEPTION_DEBUG */

__pinned_func
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

__pinned_func
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

__pinned_func
void z_x86_page_fault_handler(z_arch_esf_t *esf)
{
#ifdef CONFIG_DEMAND_PAGING
	if ((esf->errorCode & PF_P) == 0) {
		/* Page was non-present at time exception happened.
		 * Get faulting virtual address from CR2 register
		 */
		void *virt = z_x86_cr2_get();
		bool was_valid_access;

#ifdef CONFIG_X86_KPTI
		/* Protection ring is lowest 2 bits in interrupted CS */
		bool was_user = ((esf->cs & 0x3) != 0U);

		/* Need to check if the interrupted context was a user thread
		 * that hit a non-present page that was flipped due to KPTI in
		 * the thread's page tables, in which case this is an access
		 * violation and we should treat this as an error.
		 *
		 * We're probably not locked, but if there is a race, we will
		 * be fine, the kernel page fault code will later detect that
		 * the page is present in the kernel's page tables and the
		 * instruction will just be re-tried, producing another fault.
		 */
		if (was_user &&
		    !z_x86_kpti_is_access_ok(virt, get_ptables(esf))) {
			was_valid_access = false;
		} else
#else
		{
			was_valid_access = z_page_fault(virt);
		}
#endif /* CONFIG_X86_KPTI */
		if (was_valid_access) {
			/* Page fault handled, re-try */
			return;
		}
	}
#endif /* CONFIG_DEMAND_PAGING */

#if !defined(CONFIG_X86_64) && defined(CONFIG_DEBUG_COREDUMP)
	z_x86_exception_vector = IV_PAGE_FAULT;
#endif

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

__pinned_func
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
