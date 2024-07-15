/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/symtab.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

uintptr_t z_riscv_get_sp_before_exc(const struct arch_esf *esf);

struct stackframe {
	uintptr_t fp;
	uintptr_t ra;
};

typedef bool (*stack_verify_fn)(uintptr_t, const struct k_thread *const, const struct arch_esf *);

static inline bool in_irq_stack_bound(uintptr_t addr, uint8_t cpu_id)
{
	uintptr_t start, end;

	start = (uintptr_t)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[cpu_id]);
	end = start + CONFIG_ISR_STACK_SIZE;

	return (addr >= start) && (addr < end);
}

static inline bool in_kernel_thread_stack_bound(uintptr_t addr, const struct k_thread *const thread)
{
#ifdef CONFIG_THREAD_STACK_INFO
	uintptr_t start, end;

	start = thread->stack_info.start;
	end = Z_STACK_PTR_ALIGN(thread->stack_info.start + thread->stack_info.size);

	return (addr >= start) && (addr < end);
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(thread);
	/* Return false as we can't check if the addr is in the thread stack without stack info */
	return false;
#endif
}

#ifdef CONFIG_USERSPACE
static inline bool in_user_thread_stack_bound(uintptr_t addr, const struct k_thread *const thread)
{
	uintptr_t start, end;

	/* See: zephyr/include/zephyr/arch/riscv/arch.h */
	if (IS_ENABLED(CONFIG_PMP_POWER_OF_TWO_ALIGNMENT)) {
		start = thread->arch.priv_stack_start - CONFIG_PRIVILEGED_STACK_SIZE;
		end = thread->arch.priv_stack_start;
	} else {
		start = thread->stack_info.start - CONFIG_PRIVILEGED_STACK_SIZE;
		end = thread->stack_info.start;
	}

	return (addr >= start) && (addr < end);
}
#endif /* CONFIG_USERSPACE */

static bool in_stack_bound(uintptr_t addr, const struct k_thread *const thread,
			   const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	if (!IS_ALIGNED(addr, sizeof(uintptr_t))) {
		return false;
	}

#ifdef CONFIG_USERSPACE
	if ((thread->base.user_options & K_USER) != 0) {
		return in_user_thread_stack_bound(addr, thread);
	}
#endif /* CONFIG_USERSPACE */

	return in_kernel_thread_stack_bound(addr, thread);
}

static bool in_fatal_stack_bound(uintptr_t addr, const struct k_thread *const thread,
				 const struct arch_esf *esf)
{
	if (!IS_ALIGNED(addr, sizeof(uintptr_t))) {
		return false;
	}

	if ((thread == NULL) || arch_is_in_isr()) {
		/* We were servicing an interrupt */
		uint8_t cpu_id = IS_ENABLED(CONFIG_SMP) ? arch_curr_cpu()->id : 0U;

		return in_irq_stack_bound(addr, cpu_id);
	}

	return in_stack_bound(addr, thread, esf);
}

static inline bool in_text_region(uintptr_t addr)
{
	extern uintptr_t __text_region_start, __text_region_end;

	return (addr >= (uintptr_t)&__text_region_start) && (addr < (uintptr_t)&__text_region_end);
}

#ifdef CONFIG_FRAME_POINTER
static void walk_stackframe(stack_trace_callback_fn cb, void *cookie, const struct k_thread *thread,
			    const struct arch_esf *esf, stack_verify_fn vrfy,
			    const _callee_saved_t *csf, unsigned int max_frames)
{
	uintptr_t fp, last_fp = 0;
	uintptr_t ra;
	struct stackframe *frame;

	if (esf != NULL) {
		/* Unwind the provided exception stack frame */
		fp = esf->s0;
		ra = esf->mepc;
	} else if ((csf == NULL) || (csf == &_current->callee_saved)) {
		/* Unwind current thread (default case when nothing is provided ) */
		fp = (uintptr_t)__builtin_frame_address(0);
		ra = (uintptr_t)walk_stackframe;
	} else {
		/* Unwind the provided thread */
		fp = csf->s0;
		ra = csf->ra;
	}

	for (int i = 0; (i < max_frames) && vrfy(fp, thread, esf) && (fp > last_fp);) {
		if (in_text_region(ra)) {
			if (!cb(cookie, ra)) {
				break;
			}
			/*
			 * Increment the iterator only if `ra` is within the text region to get the
			 * most out of it
			 */
			i++;
		}
		last_fp = fp;
		/* Unwind to the previous frame */
		frame = (struct stackframe *)fp - 1;
		ra = frame->ra;
		fp = frame->fp;
	}
}
#else  /* !CONFIG_FRAME_POINTER */
register uintptr_t current_stack_pointer __asm__("sp");
static void walk_stackframe(stack_trace_callback_fn cb, void *cookie, const struct k_thread *thread,
			    const struct arch_esf *esf, stack_verify_fn vrfy,
			    const _callee_saved_t *csf, unsigned int max_frames)
{
	uintptr_t sp;
	uintptr_t ra;
	uintptr_t *ksp, last_ksp = 0;

	if (esf != NULL) {
		/* Unwind the provided exception stack frame */
		sp = z_riscv_get_sp_before_exc(esf);
		ra = esf->mepc;
	} else if ((csf == NULL) || (csf == &_current->callee_saved)) {
		/* Unwind current thread (default case when nothing is provided ) */
		sp = current_stack_pointer;
		ra = (uintptr_t)walk_stackframe;
	} else {
		/* Unwind the provided thread */
		sp = csf->sp;
		ra = csf->ra;

	}

	ksp = (uintptr_t *)sp;
	for (int i = 0; (i < max_frames) && vrfy((uintptr_t)ksp, thread, esf) &&
			((uintptr_t)ksp > last_ksp);) {
		if (in_text_region(ra)) {
			if (!cb(cookie, ra)) {
				break;
			}
			/*
			 * Increment the iterator only if `ra` is within the text region to get the
			 * most out of it
			 */
			i++;
		}
		last_ksp = (uintptr_t)ksp;
		/* Unwind to the previous frame */
		ra = ((struct arch_esf *)ksp++)->ra;
	}
}
#endif /* CONFIG_FRAME_POINTER */

void arch_stack_walk(stack_trace_callback_fn callback_fn, void *cookie,
		     const struct k_thread *thread, const struct arch_esf *esf)
{
	if (thread == NULL) {
		/* In case `thread` is NULL, default that to `_current` and try to unwind */
		thread = _current;
	}

	walk_stackframe(callback_fn, cookie, thread, esf, in_stack_bound, &thread->callee_saved,
			CONFIG_ARCH_STACKWALK_MAX_FRAMES);
}

#if __riscv_xlen == 32
#define PR_REG "%08" PRIxPTR
#elif __riscv_xlen == 64
#define PR_REG "%016" PRIxPTR
#endif

#ifdef CONFIG_EXCEPTION_STACK_TRACE_SYMTAB
#define LOG_STACK_TRACE(idx, ra, name, offset)                                                     \
	LOG_ERR("     %2d: ra: " PR_REG " [%s+0x%x]", idx, ra, name, offset)
#else
#define LOG_STACK_TRACE(idx, ra, name, offset) LOG_ERR("     %2d: ra: " PR_REG, idx, ra)
#endif /* CONFIG_EXCEPTION_STACK_TRACE_SYMTAB */

static bool print_trace_address(void *arg, unsigned long ra)
{
	int *i = arg;
#ifdef CONFIG_EXCEPTION_STACK_TRACE_SYMTAB
	uint32_t offset = 0;
	const char *name = symtab_find_symbol_name(ra, &offset);
#endif

	LOG_STACK_TRACE((*i)++, ra, name, offset);

	return true;
}

void z_riscv_unwind_stack(const struct arch_esf *esf, const _callee_saved_t *csf)
{
	int i = 0;

	LOG_ERR("call trace:");
	walk_stackframe(print_trace_address, &i, _current, esf, in_fatal_stack_bound, csf,
			CONFIG_EXCEPTION_STACK_TRACE_MAX_FRAMES);
	LOG_ERR("");
}
