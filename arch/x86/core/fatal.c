/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(os);

#ifdef CONFIG_THREAD_STACK_INFO
bool z_x86_check_stack_bounds(uintptr_t addr, size_t size, u16_t cs)
{
	uintptr_t start, end;

	if (z_arch_is_in_isr()) {
		/* We were servicing an interrupt */
		start = (uintptr_t)Z_ARCH_THREAD_STACK_BUFFER(_interrupt_stack);
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
	uintptr_t next;
	uintptr_t ret_addr;
#ifndef CONFIG_X86_64
	uintptr_t args;
#endif
};

#define MAX_STACK_FRAMES 8

static void unwind_stack(uintptr_t base_ptr, u16_t cs)
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
#endif /* CONFIG_EXCEPTION_STACK_TRACE */

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
		esf->rflags, esf->cs & 0xFFFFU, z_x86_page_tables_get());

#ifdef CONFIG_EXCEPTION_STACK_TRACE
	LOG_ERR("call trace:");
#endif
	LOG_ERR("RIP: 0x%016lx", esf->rip);
#ifdef CONFIG_EXCEPTION_STACK_TRACE
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
		esf->cs & 0xFFFFU, z_x86_page_tables_get());

#ifdef CONFIG_EXCEPTION_STACK_TRACE
	LOG_ERR("call trace:");
#endif
	LOG_ERR("EIP: 0x%08x", esf->eip);
#ifdef CONFIG_EXCEPTION_STACK_TRACE
	unwind_stack(esf->ebp, esf->cs);
#endif
}
#endif /* CONFIG_X86_64 */

FUNC_NORETURN void z_x86_fatal_error(unsigned int reason,
				     const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		dump_regs(esf);
	}

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}
