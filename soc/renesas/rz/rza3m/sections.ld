/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This header comply with IPL application header layout described in
 * the Renesas Initial Program Loader documentation, section "4.7 Application Header"
 * Offset  Size    Description
 * ------  ------  ---------------------------------------------------------
 * 0x000   8       Entry point address
 * 0x008   8       Inverted entry point address
 * 0x010   8       Destination address to copy the image payload
 * 0x018   8       Inverted destination address
 * 0x020   8       Image payload size in bytes
 * 0x028   8       Inverted image payload size
 * 0x030   0x1B0   Reserved (must be zero)
 * 0x1E0   32      Signature string: "RZ FSP application" and fill gap as 0
 * 0x200   -       The image payload
 */

SECTION_PROLOGUE(.header, CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET,)
{
	QUAD(__start)
	QUAD(0xFFFFFFFFFFFFFFFF-__start)
	QUAD(DT_CHOSEN_SRAM_ADDR)
	QUAD(0xFFFFFFFFFFFFFFFF-DT_CHOSEN_SRAM_ADDR)
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
