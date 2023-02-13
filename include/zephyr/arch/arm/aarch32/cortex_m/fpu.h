/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_M_FPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_M_FPU_H_

struct fpu_ctx_full {
	uint32_t caller_saved[16];
	uint32_t callee_saved[16];
	uint32_t fpscr;
	bool ctx_saved;
};

void z_arm_save_fp_context(struct fpu_ctx_full *buffer);
void z_arm_restore_fp_context(const struct fpu_ctx_full *buffer);

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_M_FPU_H_ */
