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

typedef bool (*riscv_stacktrace_cb)(void *cookie, unsigned long addr, unsigned long sfp);

#define MAX_STACK_FRAMES CONFIG_ARCH_STACKWALK_MAX_FRAMES

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
		start = thread->arch.priv_stack_start + Z_RISCV_STACK_GUARD_SIZE;
	} else {
		start = thread->stack_info.start - CONFIG_PRIVILEGED_STACK_SIZE;
	}
	end = Z_STACK_PTR_ALIGN(thread->arch.priv_stack_start + K_KERNEL_STACK_RESERVED +
				CONFIG_PRIVILEGED_STACK_SIZE);

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

static inline bool in_text_region(uintptr_t addr)
{
	extern uintptr_t __text_region_start, __text_region_end;

	return (addr >= (uintptr_t)&__text_region_start) && (addr < (uintptr_t)&__text_region_end);
}

#ifdef CONFIG_FRAME_POINTER
static void walk_stackframe(riscv_stacktrace_cb cb, void *cookie, const struct k_thread *thread,
			    const struct arch_esf *esf, stack_verify_fn vrfy,
			    const _callee_saved_t *csf)
{
	uintptr_t fp, last_fp = 0;
	uintptr_t ra;
	struct stackframe *frame;

	if (esf != NULL) {
		/* Unwind the provided exception stack frame */
		fp = esf->s0;
		ra = esf->mepc;
	} else if ((csf == NULL) || (csf == &arch_current_thread()->callee_saved)) {
		/* Unwind current thread (default case when nothing is provided ) */
		fp = (uintptr_t)__builtin_frame_address(0);
		ra = (uintptr_t)walk_stackframe;
	} else {
		/* Unwind the provided thread */
		fp = csf->s0;
		ra = csf->ra;
	}

	for (int i = 0; (i < MAX_STACK_FRAMES) && vrfy(fp, thread, esf) && (fp > last_fp); i++) {
		if (in_text_region(ra) && !cb(cookie, ra, fp)) {
			break;
		}
		last_fp = fp;

		/* Unwind to the previous frame */
		frame = (struct stackframe *)fp - 1;

		if ((i == 0) && (esf != NULL)) {
			/* Print `esf->ra` if we are at the top of the stack */
			if (in_text_region(esf->ra) && !cb(cookie, esf->ra, fp)) {
				break;
			}
			/**
			 * For the first stack frame, the `ra` is not stored in the frame if the
			 * preempted function doesn't call any other function, we can observe:
			 *
			 *                     .-------------.
			 *   frame[0]->fp ---> | frame[0] fp |
			 *                     :-------------:
			 *   frame[0]->ra ---> | frame[1] fp |
			 *                     | frame[1] ra |
			 *                     :~~~~~~~~~~~~~:
			 *                     | frame[N] fp |
			 *
			 * Instead of:
			 *
			 *                     .-------------.
			 *   frame[0]->fp ---> | frame[0] fp |
			 *   frame[0]->ra ---> | frame[1] ra |
			 *                     :-------------:
			 *                     | frame[1] fp |
			 *                     | frame[1] ra |
			 *                     :~~~~~~~~~~~~~:
			 *                     | frame[N] fp |
			 *
			 * Check if `frame->ra` actually points to a `fp`, and adjust accordingly
			 */
			if (vrfy(frame->ra, thread, esf)) {
				fp = frame->ra;
				frame = (struct stackframe *)fp;
			}
		}

		fp = frame->fp;
		ra = frame->ra;
	}
}
#else  /* !CONFIG_FRAME_POINTER */
register uintptr_t current_stack_pointer __asm__("sp");
static void walk_stackframe(riscv_stacktrace_cb cb, void *cookie, const struct k_thread *thread,
			    const struct arch_esf *esf, stack_verify_fn vrfy,
			    const _callee_saved_t *csf)
{
	uintptr_t sp;
	uintptr_t ra;
	uintptr_t *ksp, last_ksp = 0;

	if (esf != NULL) {
		/* Unwind the provided exception stack frame */
		sp = z_riscv_get_sp_before_exc(esf);
		ra = esf->mepc;
	} else if ((csf == NULL) || (csf == &arch_current_thread()->callee_saved)) {
		/* Unwind current thread (default case when nothing is provided ) */
		sp = current_stack_pointer;
		ra = (uintptr_t)walk_stackframe;
	} else {
		/* Unwind the provided thread */
		sp = csf->sp;
		ra = csf->ra;
	}

	ksp = (uintptr_t *)sp;
	for (int i = 0; (i < MAX_STACK_FRAMES) && vrfy((uintptr_t)ksp, thread, esf) &&
			((uintptr_t)ksp > last_ksp);) {
		if (in_text_region(ra)) {
			if (!cb(cookie, ra, POINTER_TO_UINT(ksp))) {
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
		/* In case `thread` is NULL, default that to `arch_current_thread()`
		 * and try to unwind
		 */
		thread = arch_current_thread();
	}

	walk_stackframe((riscv_stacktrace_cb)callback_fn, cookie, thread, esf, in_stack_bound,
			&thread->callee_saved);
}

#ifdef CONFIG_EXCEPTION_STACK_TRACE
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

#if __riscv_xlen == 32
#define PR_REG "%08" PRIxPTR
#elif __riscv_xlen == 64
#define PR_REG "%016" PRIxPTR
#endif

#ifdef CONFIG_FRAME_POINTER
#define SFP "fp"
#else
#define SFP "sp"
#endif /* CONFIG_FRAME_POINTER */

#ifdef CONFIG_SYMTAB
#define LOG_STACK_TRACE(idx, sfp, ra, name, offset)                                                \
	LOG_ERR("     %2d: " SFP ": " PR_REG " ra: " PR_REG " [%s+0x%x]", idx, sfp, ra, name,      \
		offset)
#else
#define LOG_STACK_TRACE(idx, sfp, ra, name, offset)                                                \
	LOG_ERR("     %2d: " SFP ": " PR_REG " ra: " PR_REG, idx, sfp, ra)
#endif /* CONFIG_SYMTAB */

static bool print_trace_address(void *arg, unsigned long ra, unsigned long sfp)
{
	int *i = arg;
#ifdef CONFIG_SYMTAB
	uint32_t offset = 0;
	const char *name = symtab_find_symbol_name(ra, &offset);
#endif /* CONFIG_SYMTAB */

	LOG_STACK_TRACE((*i)++, sfp, ra, name, offset);

	return true;
}

void z_riscv_unwind_stack(const struct arch_esf *esf, const _callee_saved_t *csf)
{
	int i = 0;

	LOG_ERR("call trace:");
	walk_stackframe(print_trace_address, &i, arch_current_thread(), esf, in_fatal_stack_bound,
			csf);
	LOG_ERR("");
}
#endif /* CONFIG_EXCEPTION_STACK_TRACE */
