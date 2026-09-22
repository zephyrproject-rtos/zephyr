/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xtensa/corebits.h"
#include "xtensa_backtrace.h"
#include <zephyr/arch/xtensa/xtensa_ptr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/arch/exception.h>

#include <xtensa_asm2_context.h>
#include <xtensa_stack.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

static int mask, cause;

__weak bool xtensa_soc_stack_ptr_is_sane(uint32_t sp)
{
	return true;
}

__weak bool xtensa_soc_ptr_executable(const void *p)
{
	return true;
}

static inline uint32_t xtensa_cpu_process_stack_pc(uint32_t pc)
{
	if (pc & 0x80000000) {
		/* Top two bits of a0 (return address) specify window increment.
		 * Overwrite to map to address space.
		 */
		if (cause != EXCCAUSE_INSTR_PROHIBITED) {
			pc = (pc & 0x3fffffff) | mask;
		} else {
			pc = (pc & 0x3fffffff) | 0x40000000;
		}
	}
	/* Minus 3 to get PC of previous instruction
	 * (i.e. instruction executed before return address)
	 */
	return pc - 3;
}

static inline bool xtensa_stack_ptr_is_sane(uint32_t sp)
{
	bool valid;

	valid = xtensa_soc_stack_ptr_is_sane(sp);
	if (valid) {
		valid = !xtensa_is_outside_stack_bounds(sp, 0, UINT32_MAX);
	}

	return valid;
}

static inline bool xtensa_ptr_executable(const void *p)
{
	return xtensa_soc_ptr_executable(p);
}

bool xtensa_backtrace_get_next_frame(struct xtensa_backtrace_frame_t *frame)
{
	/* Do not continue backtrace when we encounter an invalid stack
	 * frame pointer.
	 */
	if (xtensa_is_outside_stack_bounds((uintptr_t)frame->sp, 0, UINT32_MAX)) {
		return false;
	}

	/* Use frame(i-1)'s BS area located below frame(i)'s
	 * sp to get frame(i-1)'s sp and frame(i-2)'s pc
	 */

	/* Base save area consists of 4 words under SP */
	char *base_save = (char *)frame->sp;

	frame->pc = frame->next_pc;
	/* If next_pc = 0, indicates frame(i-1) is the last
	 * frame on the stack
	 */
	frame->next_pc = *((uint32_t *)(base_save - 16));
	frame->sp =  *((uint32_t *)(base_save - 12));

	/* Return true if both sp and pc of frame(i-1) are sane,
	 * false otherwise
	 */
	return (xtensa_stack_ptr_is_sane(frame->sp) &&
			xtensa_ptr_executable((void *)
				xtensa_cpu_process_stack_pc(frame->pc)));
}

int xtensa_backtrace_print(int depth, int *interrupted_stack)
{
	/* Check arguments */
	if (depth <= 0) {
		return -1;
	}

	_xtensa_irq_stack_frame_raw_t *frame = (void *)interrupted_stack;
	_xtensa_irq_bsa_t *bsa;

	/* Don't dump stack if the stack pointer is invalid as
	 * any frame elements obtained via de-referencing the
	 * frame pointer are probably also invalid. Or worse,
	 * cause another access violation.
	 */
	if (!xtensa_is_frame_pointer_valid(frame)) {
		return -1;
	}

	bsa = frame->ptr_to_bsa;
	cause = bsa->exccause;

	/* Initialize stk_frame with first frame of stack */
	struct xtensa_backtrace_frame_t stk_frame;

	xtensa_backtrace_get_start(&(stk_frame.pc), &(stk_frame.sp),
			&(stk_frame.next_pc), interrupted_stack);

	if (cause != EXCCAUSE_INSTR_PROHIBITED) {
		mask = stk_frame.pc & 0xc0000000;
	}
	EXCEPTION_DUMP("Backtrace:");
	EXCEPTION_DUMP("0x%08x:0x%08x",
		       xtensa_cpu_process_stack_pc(stk_frame.pc),
		       stk_frame.sp);

	/* Check if first frame is valid */
	bool corrupted = !(xtensa_stack_ptr_is_sane(stk_frame.sp) &&
				(xtensa_ptr_executable((void *)
				xtensa_cpu_process_stack_pc(stk_frame.pc)) ||
	/* Ignore the first corrupted PC in case of InstrFetchProhibited */
				cause == EXCCAUSE_INSTR_PROHIBITED));

	while (depth-- > 0 && stk_frame.next_pc != 0 && !corrupted) {
		/* Get previous stack frame */
		if (!xtensa_backtrace_get_next_frame(&stk_frame)) {
			corrupted = true;
		}
		EXCEPTION_DUMP("0x%08x:0x%08x",
			       xtensa_cpu_process_stack_pc(stk_frame.pc),
			       stk_frame.sp);
	}

	/* Print backtrace termination marker */
	int ret = 0;

	if (corrupted) {
		EXCEPTION_DUMP("CORRUPTED");
		ret = -1;
	} else if (stk_frame.next_pc != 0) {    /* Backtrace continues */
		EXCEPTION_DUMP("CONTINUES");
	}

	return ret;
}
