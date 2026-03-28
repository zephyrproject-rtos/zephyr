/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-tool.h>


GROUP_START(FLEXSPI2)
	SECTION_PROLOGUE(.flexspi2_bss,(NOLOAD),SUBALIGN(4))
	{
		__flexspi2_start = .;
		*(.lvgl_buf);
		__flexspi2_end = .;
	} GROUP_LINK_IN(FLEXSPI2)
