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

#ifdef CONFIG_X86_64
#define PR_UPTR	"0x%016lx"
#else
#define PR_UPTR "0x%08lx"
#endif

static inline uintptr_t esf_get_sp(const z_arch_esf_t *esf)
{
#ifdef CONFIG_X86_64
	return esf->rsp;
#else
	return esf->esp;
#endif
}

static inline uintptr_t esf_get_code(const z_arch_esf_t *esf)
{
#ifdef CONFIG_X86_64
	return esf->code;
#else
	return esf->errorCode;
#endif
}

#ifdef CONFIG_THREAD_STACK_INFO
bool z_x86_check_stack_bounds(uintptr_t addr, size_t size, u16_t cs)
{
	uintptr_t start, end;

	if (arch_is_in_isr()) {
		/* We were servicing an interrupt */
		start = (uintptr_t)ARCH_THREAD_STACK_BUFFER(_interrupt_stack);
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

/*
 * PAGE FAULT HANDLING
 */

#ifdef CONFIG_EXCEPTION_DEBUG
/* Page fault error code flags */
#define PRESENT	BIT(0)
#define WR	BIT(1)
#define US	BIT(2)
#define RSVD	BIT(3)
#define ID	BIT(4)
#define PK	BIT(5)
#define SGX	BIT(15)

#ifdef CONFIG_X86_MMU
static bool dump_entry_flags(const char *name, u64_t flags)
{
	if ((flags & Z_X86_MMU_P) == 0) {
		LOG_ERR("%s: Non-present", name);
		return false;
	} else {
		LOG_ERR("%s: 0x%016llx %s, %s, %s", name, flags,
			flags & MMU_ENTRY_WRITE ?
			"Writable" : "Read-only",
			flags & MMU_ENTRY_USER ?
			"User" : "Supervisor",
			flags & MMU_ENTRY_EXECUTE_DISABLE ?
			"Execute Disable" : "Execute Enabled");
		return true;
	}
}

static void dump_mmu_flags(struct x86_page_tables *ptables, uintptr_t addr)
{
	u64_t entry;

#ifdef CONFIG_X86_64
	entry = *z_x86_get_pml4e(ptables, addr);
	if (!dump_entry_flags("PML4E", entry)) {
		return;
	}

	entry = *z_x86_pdpt_get_pdpte(z_x86_pml4e_get_pdpt(entry), addr);
	if (!dump_entry_flags("PDPTE", entry)) {
		return;
	}
#else
	/* 32-bit doesn't have anything interesting in the PDPTE except
	 * the present bit
	 */
	entry = *z_x86_get_pdpte(ptables, addr);
	if ((entry & Z_X86_MMU_P) == 0) {
		LOG_ERR("PDPTE: Non-present");
		return;
	}
#endif

	entry = *z_x86_pd_get_pde(z_x86_pdpte_get_pd(entry), addr);
	if (!dump_entry_flags("  PDE", entry)) {
		return;
	}

	entry = *z_x86_pt_get_pte(z_x86_pde_get_pt(entry), addr);
	if (!dump_entry_flags("  PTE", entry)) {
		return;
	}
}
#endif /* CONFIG_X86_MMU */

static void dump_page_fault(z_arch_esf_t *esf)
{
	uintptr_t err, cr2;

	/* See Section 6.15 of the IA32 Software Developer's Manual vol 3 */
	__asm__ ("mov %%cr2, %0" : "=r" (cr2));

	err = esf_get_code(esf);
	LOG_ERR("***** CPU Page Fault (error code " PR_UPTR ")", err);

	LOG_ERR("%s thread %s address " PR_UPTR,
		(err & US) != 0U ? "User" : "Supervisor",
		(err & ID) != 0U ? "executed" : ((err & WR) != 0U ?
						 "wrote" :
						 "read"), cr2);

#ifdef CONFIG_X86_MMU
#ifdef CONFIG_USERSPACE
	if (err & US) {
		dump_mmu_flags(z_x86_thread_page_tables_get(_current), cr2);
	} else
#endif /* CONFIG_USERSPACE */
	{
		dump_mmu_flags(&z_x86_kernel_ptables, cr2);
	}
#endif /* CONFIG_X86_MMU */
}
#endif /* CONFIG_EXCEPTION_DEBUG */

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
	if (z_x86_check_stack_bounds(esf_get_sp(esf), 0, esf->cs)) {
		z_x86_fatal_error(K_ERR_STACK_CHK_FAIL, esf);
	}
#endif
	z_x86_fatal_error(K_ERR_CPU_EXCEPTION, esf);
	CODE_UNREACHABLE;
}
