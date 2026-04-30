/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-tool.h>

GROUP_START(LVGL_BUF)
	SECTION_PROLOGUE(.lvgl_buf,(NOLOAD),SUBALIGN(64))
	{
		__lvgl_buf_start = .;
		*(.lvgl_buf);
		__lvgl_buf_end = .;
	} GROUP_LINK_IN(LVGL_BUF)
