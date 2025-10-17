/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the et171 platform
 */

#include <zephyr/arch/riscv/common/linker.ld>

SECTIONS
{
	SECTION_PROLOGUE(.egis_padding_section,,)
	{
	BYTE(0);
	. = ALIGN(32);
	} GROUP_LINK_IN(ROMABLE_REGION)
	__egis_binary_end = LOADADDR(.egis_padding_section) + SIZEOF(.egis_padding_section);
	__egis_binary_size = __egis_binary_end - __rom_region_start;
}
