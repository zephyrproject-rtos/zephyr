/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sw_isr_table.h>
#include <kernel_arch_func.h>

/*
 * VPR does not clear MSTATUS.MIE on trap entry, so an interrupt of a higher level can preempt
 * another interrupt or an exception before the first instruction of the trap handler. The
 * preempted trap is not accounted in the nesting counter yet, so the preempting interrupt returns
 * as if it was the outermost one, with _isr_wrapper as its return address. It must resume the
 * preempted trap on the stack pointer the hardware stacked it on, which then reschedules itself
 * if needed.
 */
bool z_riscv_custom_check_reschedule(const struct arch_esf *esf)
{
	return esf->mepc != (unsigned long)_isr_wrapper;
}
