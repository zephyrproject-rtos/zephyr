/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

#ifdef CONFIG_THREAD_LOCAL_STORAGE
#include <kernel_tls.h>

/* GCC's m68k local-exec ABI biases TP by 0x7000 from the TLS block. */
#define M68K_TLS_TP_OFFSET 0x7000U

/* Initialized by reset.S before BSS is cleared. */
__noinit uintptr_t z_m68k_tls_ptr;

void FUNC_NO_STACK_PROTECTOR z_m68k_tls_init(char *tls);

size_t arch_tls_stack_setup(struct k_thread *new_thread, char *stack_ptr)
{
	size_t tls_size = z_tls_data_size();

	stack_ptr -= tls_size;
	z_tls_copy(stack_ptr);
	new_thread->tls = POINTER_TO_UINT(stack_ptr) + M68K_TLS_TP_OFFSET;

	return tls_size;
}

void FUNC_NO_STACK_PROTECTOR z_m68k_tls_init(char *tls)
{
	z_tls_copy(tls);
	z_m68k_tls_ptr = POINTER_TO_UINT(tls) + M68K_TLS_TP_OFFSET;
}

#endif /* CONFIG_THREAD_LOCAL_STORAGE */

/*
 * Synthetic frame consumed by z_m68k_switch(): MOVEM restores d2-d7/a2-a6,
 * then RTS enters z_m68k_thread_start(). Remaining words form its GCC stack
 * argument frame.
 */
struct m68k_context_frame {
	uintptr_t d2;
	uintptr_t d3;
	uintptr_t d4;
	uintptr_t d5;
	uintptr_t d6;
	uintptr_t d7;

	uintptr_t a2;
	uintptr_t a3;
	uintptr_t a4;
	uintptr_t a5;
	uintptr_t a6;

	uintptr_t pc;
	uintptr_t ret_addr;
	uintptr_t entry;
	uintptr_t p1;
	uintptr_t p2;
	uintptr_t p3;
};

BUILD_ASSERT(sizeof(uintptr_t) == sizeof(uint32_t),
	     "m68k context frame slots must be 32-bit longwords");

static FUNC_NORETURN void z_m68k_thread_return(void)
{
	k_panic();
	CODE_UNREACHABLE;
}

static FUNC_NORETURN void z_m68k_thread_start(k_thread_entry_t entry,
					      void *p1, void *p2, void *p3)
{
	/* New threads must not inherit the creator's interrupt mask. */
	arch_irq_unlock(0);

	z_thread_entry(entry, p1, p2, p3);
	CODE_UNREACHABLE;
}

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct m68k_context_frame *frame;

	frame = (struct m68k_context_frame *)Z_STACK_PTR_ALIGN(
		Z_STACK_PTR_TO_FRAME(struct m68k_context_frame, stack_ptr));

	*frame = (struct m68k_context_frame){0};

	frame->pc = (uintptr_t)z_m68k_thread_start;
	frame->ret_addr = (uintptr_t)z_m68k_thread_return;
	frame->entry = (uintptr_t)entry;
	frame->p1 = (uintptr_t)p1;
	frame->p2 = (uintptr_t)p2;
	frame->p3 = (uintptr_t)p3;

	thread->callee_saved.sp = (uintptr_t)frame;
	thread->switch_handle = thread;

	ARG_UNUSED(stack);
}

int arch_coprocessors_disable(struct k_thread *thread)
{
	ARG_UNUSED(thread);
	return -ENOTSUP;
}
