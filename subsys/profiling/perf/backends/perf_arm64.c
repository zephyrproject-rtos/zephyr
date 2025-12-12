/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/arch/arm64/exception.h>

static bool valid_stack(uintptr_t addr, k_tid_t current)
{
	return current->stack_info.start <= addr &&
	       addr < current->stack_info.start + current->stack_info.size;
}

static inline bool in_text_region(uintptr_t addr)
{
	return (addr >= (uintptr_t)__text_region_start) && (addr < (uintptr_t)__text_region_end);
}

/*
 * This function uses frame pointers to unwind stack and get trace of return addresses.
 * Return addresses are translated to corresponding function's names using .elf file.
 * So we get function call trace.
 */
size_t arch_perf_current_stack_trace(uintptr_t *buf, size_t size)
{
	if (size < 2U) {
		return 0;
	}

	size_t idx = 0;

	/*
	 * In ARM64 (arch/arm64/core/isr_wrapper.S and vector_table.S),
	 * when an interrupt occurs, the exception handler saves all volatile registers
	 * (x0-x18, lr, fp, spsr, elr) into an arch_esf structure on the thread's stack.
	 * Then it switches to the IRQ stack and saves the thread's SP at [irq_stack - 16].
	 *
	 * The following code retrieves the saved arch_esf from the thread stack
	 * to access the frame pointer and link register.
	 */

	/*
	 * Get the thread's stack pointer that was saved when switching to IRQ stack.
	 * In isr_wrapper.S:
	 *   ldr x1, [x0, #___cpu_t_irq_stack_OFFSET]  // x1 = irq_stack (top)
	 *   mov x2, sp                                  // x2 = thread sp
	 *   mov sp, x1                                  // switch to irq stack
	 *   str x2, [sp, #-16]!                        // save thread sp at [irq_stack - 16]
	 */
	uintptr_t thread_sp = *((uintptr_t *)(((uintptr_t)_current_cpu->irq_stack) - 16));

	/*
	 * The arch_esf structure is at the top of the thread stack.
	 * In vector_table.S, z_arm64_enter_exc macro does:
	 *   sub sp, sp, ___esf_t_SIZEOF
	 *   stp x0, x1, [sp, ___esf_t_x0_x1_OFFSET]
	 *   ...
	 * So the esf pointer is the current thread SP.
	 */
	const struct arch_esf *const esf = (struct arch_esf *)thread_sp;

#ifdef CONFIG_FRAME_POINTER
	/*
	 * In ARM64, x29 is used as the frame pointer (fp).
	 *
	 * Stack frame layout in memory (stack grows downward):
	 * (lower addresses)
	 *   ...
	 *   [fp - 16] previous fp
	 *   [fp - 8]  return address (lr)
	 *   [fp]      <- current fp (x29)
	 *   ...
	 * (higher addresses)
	 *
	 * The frame pointer points to the location where the previous fp is saved,
	 * and the return address (lr) is at fp + 8 (next to the saved fp).
	 */
	void **fp = (void **)esf->fp;

	/* Save the exception return address (PC at the time of interrupt) */
	buf[idx++] = (uintptr_t)esf->elr;

	/*
	 * Save the link register from the exception frame.
	 * This helps capture the return address in case the interrupt occurred
	 * during function prologue or epilogue where fp might not be set up yet.
	 */
	buf[idx++] = (uintptr_t)esf->lr;

	/*
	 * Walk the frame pointer chain to unwind the stack.
	 * Each frame has:
	 *   [fp]      saved previous fp
	 *   [fp + 8]  saved lr (return address)
	 */
	while (valid_stack((uintptr_t)fp, _current)) {
		if (idx >= size) {
			return 0;
		}

		void **new_fp = (void **)fp[0]; /* Load previous fp */
		void *ret_addr = fp[1];         /* Load return address */

		/* Verify the return address is in the text region */
		if (!in_text_region((uintptr_t)ret_addr)) {
			break;
		}

		buf[idx++] = (uintptr_t)ret_addr;

		/*
		 * Anti-infinity-loop protection:
		 * The new frame pointer must be at a higher address than the current one
		 * because the stack grows downward (toward lower addresses), so when we
		 * walk back through callers, fp values increase.
		 */
		if (new_fp <= fp) {
			break;
		}

		fp = new_fp;
	}
#else
	/* Without frame pointers, we can only capture the exception return address */
	buf[idx++] = (uintptr_t)esf->elr;
	buf[idx++] = (uintptr_t)esf->lr;
#endif

	return idx;
}
