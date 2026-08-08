/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define RAM_BASE DT_CHOSEN_SRAM_ADDR

SECTION_PROLOGUE(.header, DT_REG_ADDR(DT_CHOSEN(zephyr_flash)),)
{
	QUAD(__start)
	QUAD(0xFFFFFFFFFFFFFFFF-__start)
	QUAD(RAM_BASE)
	QUAD(0xFFFFFFFFFFFFFFFF-RAM_BASE)
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
