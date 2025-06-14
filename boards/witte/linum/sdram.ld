/*
 * Copyright (c) 2023 BrainCo Inc.
 * Updated for Zephyr v4.1.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram1), okay)
GROUP_START(SDRAM1)

	SECTION_PROLOGUE(_STM32_SDRAM1_SECTION_NAME, (NOLOAD),)
	{
		*(.lvgl_buf)
		*(.lvgl_heap)
	} GROUP_LINK_IN(SDRAM1)

GROUP_END(SDRAM1)
#endif
