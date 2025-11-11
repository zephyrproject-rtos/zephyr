/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram1), okay)

SECTION_DATA_PROLOGUE(.sdram,(NOLOAD),)
{
	__SDRAM_Start = .;
	KEEP(*(.sdram*))
#ifdef CONFIG_LVGL
	KEEP(*(.lvgl_buf*))
#endif
	__SDRAM_End = .;
} GROUP_LINK_IN(SDRAM)

#endif
