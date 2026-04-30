/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(GFX_MEMORY)

	SECTION_PROLOGUE(.cy_gpu_buf,(NOLOAD),)
	{
		. = ALIGN(4);
		__gfx_mem_start__ = .;
		KEEP(*(.cy_gpu_buf))
		. = ALIGN(4);
		__gfx_mem_end__ = .;
	} GROUP_LINK_IN(GFX_MEMORY)

GROUP_END(GFX_MEMORY)
