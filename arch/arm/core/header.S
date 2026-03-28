/*
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(image_header,_image_header_section,_image_header)
#ifdef CONFIG_CPU_CORTEX_M
	/*
	 * setting the _very_ early boot on the main stack allows to use memset
	 * on the interrupt stack when CONFIG_INIT_STACKS is enabled before
	 * switching to the interrupt stack for the rest of the early boot
	 */
	.long z_main_stack + CONFIG_MAIN_STACK_SIZE
	.long z_arm_reset
#else
	 b	__start                   // branch to kernel start
	.long	0                     // reserved
#endif
	.long	0                     // reserved
	.long	0                     // reserved
	.long	0                     // reserved
	.long	0                     // reserved
	.long	0                     // reserved
	.long	0                     // reserved
	.long	0                     // reserved
	.long   0x016f2818            // Magic number
	.long	__rom_region_start    // start address of zImage
	.long   __end                 // end address of zImage
