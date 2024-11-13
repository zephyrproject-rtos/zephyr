/*
 *  Copyright (c) 2023 KNS Group LLC (YADRO)
 *  Copyright (c) 2020 Yonatan Goldschmidt <yon.goldschmidt@gmail.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

static bool valid_stack(uintptr_t addr, k_tid_t current)
{
	return current->stack_info.start <= addr &&
		addr < current->stack_info.start + current->stack_info.size;
}

static inline bool in_text_region(uintptr_t addr)
{
	extern uintptr_t __text_region_start, __text_region_end;

	return (addr >= (uintptr_t)&__text_region_start) && (addr < (uintptr_t)&__text_region_end);
}

/* interruption stack frame */
struct isf {
	uint32_t ebp;
	uint32_t ecx;
	uint32_t edx;
	uint32_t eax;
	uint32_t eip;
};

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

	const struct isf * const isf =
		*((struct isf **)(((void **)arch_curr_cpu()->irq_stack)-1));
	/*
	 * In x86 (arch/x86/core/ia32/intstub.S) %eip and %ebp
	 * are saved at the beginning of _interrupt_enter in order, that described
	 * in struct esf. Core switch %esp to
	 * arch_curr_cpu()->irq_stack and push %esp on irq stack
	 *
	 * The following lines lines do the reverse things to get %eip and %ebp
	 * from thread stack
	 */
	void **fp = (void **)isf->ebp;

	/*
	 * %ebp is frame pointer.
	 *
	 * stack frame in memory:
	 * (addresses growth up)
	 *  ....
	 *  ra
	 *  %ebp (next) <- %ebp (curr)
	 *  ....
	 */

	buf[idx++] = (uintptr_t)isf->eip;
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
