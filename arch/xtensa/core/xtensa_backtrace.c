/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xtensa/corebits.h"
#include "xtensa_backtrace.h"
#include <zephyr/sys/printk.h>
#if defined(CONFIG_SOC_ESP32)
#include "soc/soc_memory_layout.h"
#elif defined(CONFIG_SOC_FAMILY_INTEL_ADSP)
#include "debug_helpers.h"
#endif
static int mask, cause;

static inline uint32_t z_xtensa_cpu_process_stack_pc(uint32_t pc)
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

static inline bool z_xtensa_stack_ptr_is_sane(uint32_t sp)
{
#if defined(CONFIG_SOC_ESP32)
	return esp_stack_ptr_is_sane(sp);
#elif defined(CONFIG_SOC_FAMILY_INTEL_ADSP)
	return intel_adsp_ptr_is_sane(sp);
#else
#warning "z_xtensa_stack_ptr_is_sane is not defined for this platform"
#endif
}

static inline bool z_xtensa_ptr_executable(const void *p)
{
#if defined(CONFIG_SOC_ESP32)
	return esp_ptr_executable(p);
#elif defined(CONFIG_SOC_FAMILY_INTEL_ADSP)
	return intel_adsp_ptr_executable(p);
#else
#warning "z_xtensa_ptr_executable is not defined for this platform"
#endif
}

bool z_xtensa_backtrace_get_next_frame(struct z_xtensa_backtrace_frame_t *frame)
{
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
	return (z_xtensa_stack_ptr_is_sane(frame->sp) &&
			z_xtensa_ptr_executable((void *)
				z_xtensa_cpu_process_stack_pc(frame->pc)));
}

int z_xtensa_backtrace_print(int depth, int *interrupted_stack)
{
	/* Check arguments */
	if (depth <= 0) {
		return -1;
	}

	/* Initialize stk_frame with first frame of stack */
	struct z_xtensa_backtrace_frame_t stk_frame;

	z_xtensa_backtrace_get_start(&(stk_frame.pc), &(stk_frame.sp),
			&(stk_frame.next_pc), interrupted_stack);
	__asm__ volatile("l32i a4, a3, 0");
	__asm__ volatile("l32i a4, a4, 4");
	__asm__ volatile("mov %0, a4" : "=r"(cause));
	if (cause != EXCCAUSE_INSTR_PROHIBITED) {
		mask = stk_frame.pc & 0xc0000000;
	}
	printk("\r\n\r\nBacktrace:");
	printk("0x%08X:0x%08X ",
			z_xtensa_cpu_process_stack_pc(stk_frame.pc),
			stk_frame.sp);

	/* Check if first frame is valid */
	bool corrupted = !(z_xtensa_stack_ptr_is_sane(stk_frame.sp) &&
				(z_xtensa_ptr_executable((void *)
				z_xtensa_cpu_process_stack_pc(stk_frame.pc)) ||
	/* Ignore the first corrupted PC in case of InstrFetchProhibited */
				cause == EXCCAUSE_INSTR_PROHIBITED));

	uint32_t i = (depth <= 0) ? INT32_MAX : depth;

	while (i-- > 0 && stk_frame.next_pc != 0 && !corrupted) {
		/* Get previous stack frame */
		if (!z_xtensa_backtrace_get_next_frame(&stk_frame)) {
			corrupted = true;
		}
		printk("0x%08X:0x%08X ", z_xtensa_cpu_process_stack_pc(stk_frame.pc), stk_frame.sp);
	}

	/* Print backtrace termination marker */
	int ret = 0;

	if (corrupted) {
		printk(" |<-CORRUPTED");
		ret =  -1;
	} else if (stk_frame.next_pc != 0) {    /* Backtrace continues */
		printk(" |<-CONTINUES");
	}
	printk("\r\n\r\n");
	return ret;
}
