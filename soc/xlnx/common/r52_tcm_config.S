/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

_ASM_FILE_PROLOGUE

/*
 * Default split-mode Cortex-R52 TCM mappings for AMD R52 platforms. These
 * weak data symbols form an overrideable startup ABI. A generated linker
 * script may replace any of them with a strong symbol pointing at a 32-bit
 * configuration word.
 */
	.pushsection .rodata.tcm_config, "a", %progbits
	.balign 4

	.weak z_arm_tcm_a_region
	.type z_arm_tcm_a_region, %object
z_arm_tcm_a_region:
	.word 0x00000000
	.size z_arm_tcm_a_region, . - z_arm_tcm_a_region

	.weak z_arm_tcm_b_region
	.type z_arm_tcm_b_region, %object
z_arm_tcm_b_region:
	.word 0x00010001
	.size z_arm_tcm_b_region, . - z_arm_tcm_b_region

	.weak z_arm_tcm_c_region
	.type z_arm_tcm_c_region, %object
z_arm_tcm_c_region:
	.word 0x00018001
	.size z_arm_tcm_c_region, . - z_arm_tcm_c_region

	.popsection
