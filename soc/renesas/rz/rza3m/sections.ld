/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.header, CONFIG_FLASH_BASE_ADDRESS,)
{
	QUAD(__start)
	QUAD(0xFFFFFFFFFFFFFFFF-__start)
	QUAD(CONFIG_SRAM_BASE_ADDRESS)
	QUAD(0xFFFFFFFFFFFFFFFF-CONFIG_SRAM_BASE_ADDRESS)
	QUAD(z_mapped_size)
	QUAD(0xFFFFFFFFFFFFFFFF-z_mapped_size)
	FILL(0x00)
	. += 0x1B0;
	QUAD(0x4120505346205a52)
	QUAD(0x69746163696c7070)
	QUAD(0x0000000000006e6f)
	QUAD(0x0000000000000000)
} > FLASH

z_mapped_size = z_mapped_end - z_mapped_start;
