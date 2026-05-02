/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>

SECTION_PROLOGUE(.loader, CONFIG_FLASH_BASE_ADDRESS,)
{
	__loader_program_start = .;
	KEEP(*(.loader_text.*))
	__loader_program_end = .;
} GROUP_LINK_IN(FLASH)
