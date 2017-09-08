/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

MEMORY
    {
    AES_RAM (Rwx) : ORIGIN = 0x40118C00,  LENGTH = 1K
    }

#include <arch/arm/cortex_m/scripts/linker.ld>

SECTIONS
    {
    GROUP_START(AES_RAM)
    SECTION_PROLOGUE(.AHB1SMEM0,(NOLOAD),)
	{
	/*
	 * The leading '.' in the ".scs" section name indicates that section is
	 * mapped to neither normal ROM nor normal RAM space.
	 */

	*(.AHB1SMEM0)
	*(".AHB1SMEM0.*")
	 } GROUP_LINK_IN(AES_RAM)
    GROUP_END(AES_RAM)
    }
