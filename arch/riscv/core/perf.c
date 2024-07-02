/*
 *  Copyright (c) 2023 KNS Group LLC (YADRO)
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

static bool valid_stack(uintptr_t addr, k_tid_t current)
{
	return current->stack_info.start <= addr &&
		addr < current->stack_info.start + current->stack_info.size;
}

/*
 * This function use frame pointers to unwind stack and get trace of return addresses.
 * Return addresses are translated in corresponding function's names using .elf file.
 * So we get function call trace
 */
size_t arch_perf_current_stack_trace(uintptr_t *buf, size_t size)
{
	if (size < 2U)
		return 0;

	size_t idx = 0;

	/*
	 * In riscv (arch/riscv/core/isr.S) ra, ip($mepc) and fp($s0) are saved
	 * at the beginning of _isr_wrapper in order, specified by z_arch_esf_t.
	 * Then, before calling interruption handler, core switch $sp to
	 * _current_cpu->irq_stack and save $sp with offset -16 on irq stack
	 *
	 * The following lines lines do the reverse things to get ra, ip anf fp
	 * from thread stack
	 */
	const z_arch_esf_t * const esf =
		*((z_arch_esf_t **)(((uintptr_t)_current_cpu->irq_stack) - 16));

	buf[idx++] = (uintptr_t)esf->mepc;

	void **fp = (void **)esf->s0;
	/*
	 * $s0 is used as frame pointer.
	 *
	 * stack frame in memory (commonly):
	 * (addresses growth up)
	 *  ....
	 *  [-] <- $fp($s0) (curr)
	 *  $ra
	 *  $fp($s0) (next)
	 *  ....
	 *
	 * If function do not call any other function, compiller may not save $ra,
	 * then stack frame will be:
	 *  ....
	 *  [-] <- $fp($s0) (curr)
	 *  $fp($s0) (next)
	 *  ....
	 *
	 */
	void **new_fp = (void **)fp[-1];

	if (valid_stack((uintptr_t)new_fp, _current)) {
		fp = new_fp;
		buf[idx++] = (uintptr_t)esf->ra;
	}
	while (valid_stack((uintptr_t)fp, _current)) {
		if (idx >= size)
			return 0;

		buf[idx++] = (uintptr_t)fp[-1];
		new_fp = (void **)fp[-2];

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
