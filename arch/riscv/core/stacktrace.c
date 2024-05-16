/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

uintptr_t z_riscv_get_sp_before_exc(const z_arch_esf_t *esf);

#if __riscv_xlen == 32
 #define PR_REG "%08" PRIxPTR
#elif __riscv_xlen == 64
 #define PR_REG "%016" PRIxPTR
#endif

#define MAX_STACK_FRAMES 8

struct stackframe {
	uintptr_t fp;
	uintptr_t ra;
};

static bool in_stack_bound(uintptr_t addr)
{
#ifdef CONFIG_THREAD_STACK_INFO
	uintptr_t start, end;

	if (_current == NULL || arch_is_in_isr()) {
		/* We were servicing an interrupt */
		int cpu_id;

#ifdef CONFIG_SMP
		cpu_id = arch_curr_cpu()->id;
#else
		cpu_id = 0;
#endif

		start = (uintptr_t)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[cpu_id]);
		end = start + CONFIG_ISR_STACK_SIZE;
#ifdef CONFIG_USERSPACE
		/* TODO: handle user threads */
#endif
	} else {
		start = _current->stack_info.start;
		end = Z_STACK_PTR_ALIGN(_current->stack_info.start + _current->stack_info.size);
	}

	return (addr >= start) && (addr < end);
#else
	ARG_UNUSED(addr);
	return true;
#endif /* CONFIG_THREAD_STACK_INFO */
}

static inline bool in_text_region(uintptr_t addr)
{
	extern uintptr_t __text_region_start, __text_region_end;

	return (addr >= (uintptr_t)&__text_region_start) && (addr < (uintptr_t)&__text_region_end);
}

#ifdef CONFIG_RISCV_ENABLE_FRAME_POINTER
void z_riscv_unwind_stack(const z_arch_esf_t *esf)
{
	uintptr_t fp = esf->s0;
	uintptr_t ra;
	struct stackframe *frame;

	if (esf == NULL) {
		return;
	}

	LOG_ERR("call trace:");

	for (int i = 0; (i < MAX_STACK_FRAMES) && (fp != 0U) && in_stack_bound(fp);) {
		frame = (struct stackframe *)fp - 1;
		ra = frame->ra;
		if (in_text_region(ra)) {
			LOG_ERR("     %2d: fp: " PR_REG "   ra: " PR_REG, i, fp, ra);
			/*
			 * Increment the iterator only if `ra` is within the text region to get the
			 * most out of it
			 */
			i++;
		}
		fp = frame->fp;
	}

	LOG_ERR("");
}
#else /* !CONFIG_RISCV_ENABLE_FRAME_POINTER */
void z_riscv_unwind_stack(const z_arch_esf_t *esf)
{
	uintptr_t sp = z_riscv_get_sp_before_exc(esf);
	uintptr_t ra;
	uintptr_t *ksp = (uintptr_t *)sp;

	if (esf == NULL) {
		return;
	}

	LOG_ERR("call trace:");

	for (int i = 0;
	     (i < MAX_STACK_FRAMES) && ((uintptr_t)ksp != 0U) && in_stack_bound((uintptr_t)ksp);
	     ksp++) {
		ra = *ksp;
		if (in_text_region(ra)) {
			LOG_ERR("     %2d: sp: " PR_REG "   ra: " PR_REG, i, (uintptr_t)ksp, ra);
			/*
			 * Increment the iterator only if `ra` is within the text region to get the
			 * most out of it
			 */
			i++;
		}
	}

	LOG_ERR("");
}
#endif /* CONFIG_RISCV_ENABLE_FRAME_POINTER */
