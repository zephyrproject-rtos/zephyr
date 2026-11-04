/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */

#ifndef CORTEX_A_R_FPU_CONTEXT_H_
#define CORTEX_A_R_FPU_CONTEXT_H_

#include <zephyr/types.h>

#if defined(CONFIG_VFP_FEATURE_REGS_S64_D32)
#define FP_DOUBLE_REGISTERS 32U
#else
#define FP_DOUBLE_REGISTERS 16U
#endif
struct fp_context {
	uint64_t registers[FP_DOUBLE_REGISTERS];
	uint32_t fpscr;
};

void fp_context_call(const struct fp_context *input,
		     struct fp_context *output, void (*callback)(void));
void fp_capture_callee(uint64_t *registers);

#endif /* CORTEX_A_R_FPU_CONTEXT_H_ */
