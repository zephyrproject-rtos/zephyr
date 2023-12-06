/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_CORE_INCLUDE_XTENSA_BACKTRACE_H_
#define ZEPHYR_ARCH_XTENSA_CORE_INCLUDE_XTENSA_BACKTRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stdint.h>

/*
 * @brief   Structure used for backtracing
 *
 * This structure stores the backtrace information of a particular stack frame
 * (i.e. the PC and SP). This structure is used iteratively with the
 * z_xtensa_cpu_get_next_backtrace_frame() function to traverse each frame
 * within a single stack. The next_pc represents the PC of the current
 * frame's caller, thus a next_pc of 0 indicates that the current frame
 * is the last frame on the stack.
 *
 * @note    Call esp_backtrace_get_start() to obtain initialization values for
 *          this structure
 */
struct z_xtensa_backtrace_frame_t {
	uint32_t pc;       /* PC of the current frame */
	uint32_t sp;       /* SP of the current frame */
	uint32_t next_pc;  /* PC of the current frame's caller */
};

/**
 * Get the first frame of the current stack's backtrace
 *
 * Given the following function call flow
 * (B -> A -> X -> esp_backtrace_get_start),
 * this function will do the following.
 * - Flush CPU registers and window frames onto the current stack
 * - Return PC and SP of function A (i.e. start of the stack's backtrace)
 * - Return PC of function B (i.e. next_pc)
 *
 * @note This function is implemented in assembly
 *
 * @param[out] pc       PC of the first frame in the backtrace
 * @param[out] sp       SP of the first frame in the backtrace
 * @param[out] next_pc  PC of the first frame's caller
 * @param[in] interrupted_stack Pointer to interrupted stack
 */
void z_xtensa_backtrace_get_start(uint32_t *pc,
				uint32_t *sp,
				uint32_t *next_pc,
				int *interrupted_stack);

/**
 * Get the next frame on a stack for backtracing
 *
 * Given a stack frame(i), this function will obtain the next
 * stack frame(i-1) on the same call stack (i.e. the caller of frame(i)).
 * This function is meant to be called iteratively when doing a backtrace.
 *
 * Entry Conditions: Frame structure containing valid SP and next_pc
 * Exit Conditions:
 *  - Frame structure updated with SP and PC of frame(i-1).
 *    next_pc now points to frame(i-2).
 *  - If a next_pc of 0 is returned, it indicates that frame(i-1)
 *    is last frame on the stack
 *
 * @param[inout] frame  Pointer to frame structure
 *
 * @return
 *  - True if the SP and PC of the next frame(i-1) are sane
 *  - False otherwise
 */
bool z_xtensa_backtrace_get_next_frame(struct z_xtensa_backtrace_frame_t *frame);

/**
 * @brief Print the backtrace of the current stack
 *
 * @param depth The maximum number of stack frames to print (should be > 0)
 * @param interrupted_stack Pointer to interrupted stack
 *
 * @return
 *      - 0    Backtrace successfully printed to completion or to depth limit
 *      - -1   Backtrace is corrupted
 */
int z_xtensa_backtrace_print(int depth, int *interrupted_stack);

#endif
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_XTENSA_CORE_INCLUDE_XTENSA_BACKTRACE_H_ */
