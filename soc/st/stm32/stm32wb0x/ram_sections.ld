/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** Refer to `soc.c` for more information about these areas. */
SECTION_PROLOGUE(stm32wb0_RAM_VR, 0x20000000 (NOLOAD), )
{
	/* For historical reasons, leave the first word of
	 * SRAM0 unused, even though it could store data.
	 * The structure MUST start at address 0x2000_0004.
	 */
	. += 4;

	KEEP(*(stm32wb0_RAM_VR));
} GROUP_LINK_IN(RAMABLE_REGION)

SECTION_PROLOGUE(stm32wb0_BLUE_RAM, 0x200000C0 (NOLOAD), )
{
	KEEP(*(stm32wb0_BLUE_RAM));
} GROUP_LINK_IN(RAMABLE_REGION)
