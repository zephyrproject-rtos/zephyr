/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>

SECTION_PROLOGUE(.loader, DT_REG_ADDR(DT_CHOSEN(zephyr_flash)),)
{
	__loader_program_start = .;
	KEEP(*(.loader_text.*))
	__loader_program_end = .;
} GROUP_LINK_IN(FLASH)
