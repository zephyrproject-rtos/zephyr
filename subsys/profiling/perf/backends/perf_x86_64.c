/*
 *  Copyright (c) 2023 KNS Group LLC (YADRO)
 *  Copyright (c) 2020 Yonatan Goldschmidt <yon.goldschmidt@gmail.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

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
 * This function use frame pointers to unwind stack and get trace of return addresses.
 * Return addresses are translated in corresponding function's names using .elf file.
 * So we get function call trace
 */
size_t arch_perf_current_stack_trace(uintptr_t *buf, size_t size)
{
	if (size < 1U) {
		return 0;
	}

	size_t idx = 0;

	/*
	 * In x86_64 (arch/x86/core/intel64/locore.S) %rip and %rbp
	 * are always saved in _current->callee_saved before calling
	 * handler function if interrupt is not nested
	 *
	 * %rip points the location where interrupt was occurred
	 */
	buf[idx++] = (uintptr_t)_current->callee_saved.rip;
	void **fp = (void **)_current->callee_saved.rbp;

	/*
	 * %rbp is frame pointer.
	 *
	 * stack frame in memory:
	 * (addresses growth up)
	 *  ....
	 *  ra
	 *  %rbp (next) <- %rbp (curr)
	 *  ....
	 */
	while (valid_stack((uintptr_t)fp, _current)) {
		if (idx >= size) {
			return 0;
		}

		if (!in_text_region((uintptr_t)fp[1])) {
			break;
		}

		buf[idx++] = (uintptr_t)fp[1];
		void **new_fp = (void **)fp[0];

		/*
		 * anti-infinity-loop if
		 * new_fp can't be smaller than fp, cause the stack is growing down
		 * and trace moves deeper into the stack
		 */
		if (new_fp <= fp) {
			break;
		}
		fp = new_fp;
	}

	return idx;
}
